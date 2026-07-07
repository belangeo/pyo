/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
 *                                                                        *
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *
 *                                                                        *
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with pyo.  If not, see <http://www.gnu.org/licenses/>.   *
 *************************************************************************/

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"
#include "interpolation.h"

/************/
/* Record */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input_list;
    int chnls;
    int buffering;
    int count;
    int listlen;
    char *recpath;
    SNDFILE *recfile;
    SF_INFO recinfo;
    MYFLT *buffer;
} Record;

static void
Record_process(Record *self)
{
    int i, j, chnl, offset, totlen;
    MYFLT *in;

    totlen = self->chnls * self->bufsize * self->buffering;

    if (self->count == self->buffering)
    {
        self->count = 0;

        for (i = 0; i < totlen; i++)
        {
            self->buffer[i] = 0.0;
        }
    }

    offset = self->bufsize * self->chnls * self->count;

    for (j = 0; j < self->listlen; j++)
    {
        chnl = j % self->chnls;
        PyObject *streamobj = PYO_CALL_METHOD_RET(PyList_GET_ITEM(self->input_list, j), "_getStream", NULL);
        in = Stream_getData((Stream *)streamobj);
        Py_DECREF(streamobj);

        for (i = 0; i < self->bufsize; i++)
        {
            self->buffer[i * self->chnls + chnl + offset] += in[i];
        }
    }

    self->count++;

    if (self->count == self->buffering)
        SF_WRITE(self->recfile, self->buffer, totlen);
}

static void
Record_setProcMode(Record *self)
{
    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Record_process);
}

static void
Record_compute_next_data_frame(Record *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Record_traverse(Record *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input_list);
    return 0;
}

static int
Record_clear(Record *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input_list);
    return 0;
}

static void
Record_dealloc(Record* self)
{
    if (Stream_getStreamActive(self->stream))
        PYO_CALL_METHOD(self, "stop", NULL);

    pyo_DEALLOC
    PyMem_RawFree(self->buffer);
    Record_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Record_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, buflen;
    int fileformat = 0;
    int sampletype = 0;
    double quality = 0.4;
    Py_ssize_t psize;
    PyObject *input_listtmp;
    Record *self;
    self = (Record *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->chnls = 2;
    self->buffering = 4;
    self->count = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Record_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Record_setProcMode);

    static char *kwlist[] = {"input", "filename", "chnls", "fileformat", "sampletype", "buffering", "quality", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os#|iiiid", kwlist, &input_listtmp, &self->recpath, &psize, &self->chnls, &fileformat, &sampletype, &self->buffering, &quality)) {
        Py_DECREF(self);
        return NULL;
    }

    self->input_list = input_listtmp;
    Py_INCREF(self->input_list);
    self->listlen = PyList_Size(self->input_list);

    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->sr;
    self->recinfo.channels = self->chnls;

    switch (fileformat)
    {
        case 0:
            self->recinfo.format = SF_FORMAT_WAV;
            break;

        case 1:
            self->recinfo.format = SF_FORMAT_AIFF;
            break;

        case 2:
            self->recinfo.format = SF_FORMAT_AU;
            break;

        case 3:
            self->recinfo.format = SF_FORMAT_RAW;
            break;

        case 4:
            self->recinfo.format = SF_FORMAT_SD2;
            break;

        case 5:
            self->recinfo.format = SF_FORMAT_FLAC;
            break;

        case 6:
            self->recinfo.format = SF_FORMAT_CAF;
            break;

        case 7:
            self->recinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            break;
    }

    if (fileformat != 7)
    {
        switch (sampletype)
        {
            case 0:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_16;
                break;

            case 1:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_24;
                break;

            case 2:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_32;
                break;

            case 3:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_FLOAT;
                break;

            case 4:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_DOUBLE;
                break;

            case 5:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_ULAW;
                break;

            case 6:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_ALAW;
                break;
        }
    }

    /* Open the output file. */
    if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo)))
    {
        PySys_WriteStdout("Record: not able to open output file %s.\n", self->recpath);
        Py_RETURN_NONE;
    }

    // Sets the encoding quality for FLAC and OGG compressed formats
    if (fileformat == 5 || fileformat == 7)
    {
        sf_command(self->recfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(double));
    }

    buflen = self->bufsize * self->chnls * self->buffering;
    self->buffer = (MYFLT *)PyMem_RawRealloc(self->buffer, buflen * sizeof(MYFLT));

    for (i = 0; i < buflen; i++)
    {
        self->buffer[i] = 0.;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Record_getServer(Record* self) { GET_SERVER };
static PyObject * Record_getStream(Record* self) { GET_STREAM };

static PyObject * Record_play(Record *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Record_stop(Record *self, PyObject *args, PyObject *kwds)
{
    int i, nearestBuf = 0;
    float wait = 0.0;

    static char *kwlist[] = {"wait", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|f", kwlist, &wait))
        return PyLong_FromLong(-1);

    if (wait == 0)
    {
        sf_close(self->recfile);
        Stream_setStreamActive(self->stream, 0);
        Stream_setStreamChnl(self->stream, 0);
        Stream_setStreamToDac(self->stream, 0);

        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = 0;
        }
    }
    else
    {
        Stream_resetBufferCount(self->stream);
        nearestBuf = (int)roundf((wait * self->sr) / self->bufsize + 0.5);
        Stream_setDuration(self->stream, nearestBuf);
    }

    Py_RETURN_NONE;
};

static PyMemberDef Record_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Record, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Record, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Record, input_list), 0, "Input sound base object list."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Record_methods[] =
{
    {"getServer", (PyCFunction)Record_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Record_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Record_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Record_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

static PyType_Slot RecordType_slots[] = {
    {Py_tp_dealloc, Record_dealloc},
    {Py_tp_doc, "Record objects. Records its audio input in a file."},
    {Py_tp_traverse, Record_traverse},
    {Py_tp_clear, Record_clear},
    {Py_tp_methods, Record_methods},
    {Py_tp_members, Record_members},
    {Py_tp_new, Record_new},
    {0, NULL}
};

static PyType_Spec RecordType_spec =
{
    "_pyo.Record_base",
    sizeof(Record),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    RecordType_slots
};

PyTypeObject *
PyoCreateRecordType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &RecordType_spec, NULL);
}

/************/
/* ControlRec */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *tmp_list;
    MYFLT dur;
    int rate;
    int modulo;
    long count;
    long time;
    long size;
    MYFLT *buffer;
} ControlRec;

static void
ControlRec_process(ControlRec *self)
{
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->dur > 0.0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            if ((self->time % self->modulo) == 0 && self->count < self->size)
            {
                self->buffer[self->count] = in[i];
                self->count++;
            }

            self->time++;

            if (self->count >= self->size)
                PYO_CALL_METHOD(self, "stop", NULL);
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            if ((self->time % self->modulo) == 0)
            {
                PyList_Append(self->tmp_list, PyFloat_FromDouble(in[i]));
            }

            self->time++;
        }
    }
}

static void
ControlRec_setProcMode(ControlRec *self)
{
    self->proc_func_ptr = PYO_AUDIO_CALLBACK(ControlRec_process);
}

static void
ControlRec_compute_next_data_frame(ControlRec *self)
{
    (*self->proc_func_ptr)(self);
}

static int
ControlRec_traverse(ControlRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->tmp_list);
    return 0;
}

static int
ControlRec_clear(ControlRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->tmp_list);
    return 0;
}

static void
ControlRec_dealloc(ControlRec* self)
{
    pyo_DEALLOC

    if (self->buffer != NULL)
        PyMem_RawFree(self->buffer);

    ControlRec_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ControlRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    long j;
    PyObject *inputtmp, *input_streamtmp;
    ControlRec *self;
    self = (ControlRec *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->dur = 0.0;
    self->rate = 1000;
    self->tmp_list = PyList_New(0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(ControlRec_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(ControlRec_setProcMode);

    static char *kwlist[] = {"input", "rate", "dur", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IF, kwlist, &inputtmp, &self->rate, &self->dur)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (self->dur > 0.0)
    {
        self->size = (long)(self->dur * self->rate + 1);
        self->buffer = (MYFLT *)PyMem_RawRealloc(self->buffer, self->size * sizeof(MYFLT));

        for (j = 0; j < self->size; j++)
        {
            self->buffer[j] = 0.0;
        }
    }

    self->modulo = (int)(self->sr / self->rate);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ControlRec_getServer(ControlRec* self) { GET_SERVER };
static PyObject * ControlRec_getStream(ControlRec* self) { GET_STREAM };

static PyObject * ControlRec_play(ControlRec *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    PLAY
};

static PyObject * ControlRec_stop(ControlRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
ControlRec_getData(ControlRec *self)
{
    int i;
    PyObject *data, *point;
    MYFLT time, timescl = 1.0 / self->rate;

    if (self->dur > 0.0)
    {
        data = PyList_New(self->size);

        for (i = 0; i < self->size; i++)
        {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyFloat_FromDouble(self->buffer[i]));
            PyList_SetItem(data, i, point);
        }
    }
    else
    {
        if (Stream_getStreamActive(self->stream))
        {
            PYO_CALL_METHOD(self, "stop", NULL);
        }
        Py_ssize_t size = PyList_Size(self->tmp_list);
        data = PyList_New(size);

        for (i = 0; i < size; i++)
        {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list, i));
            PyList_SetItem(data, i, point);
        }
    }

    return data;
}

static PyMemberDef ControlRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ControlRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ControlRec, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(ControlRec, input), 0, "Input sound."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRec_methods[] =
{
    {"getServer", (PyCFunction)ControlRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ControlRec_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)ControlRec_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)ControlRec_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"getData", (PyCFunction)ControlRec_getData, METH_NOARGS, "Returns list of sampled points."},
    {NULL}  /* Sentinel */
};

static PyType_Slot ControlRecType_slots[] = {
    {Py_tp_dealloc, ControlRec_dealloc},
    {Py_tp_doc, "ControlRec objects. Records control signal with user-defined sampling rate."},
    {Py_tp_traverse, ControlRec_traverse},
    {Py_tp_clear, ControlRec_clear},
    {Py_tp_methods, ControlRec_methods},
    {Py_tp_members, ControlRec_members},
    {Py_tp_new, ControlRec_new},
    {0, NULL}
};

static PyType_Spec ControlRecType_spec =
{
    "_pyo.ControlRec_base",
    sizeof(ControlRec),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    ControlRecType_slots
};

PyTypeObject *
PyoCreateControlRecType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &ControlRecType_spec, NULL);
}

/**************/
/* ControlRead object */
/**************/
typedef struct
{
    pyo_audio_HEAD
    MYFLT *values;
    int rate;
    int modulo;
    int loop;
    int go;
    int modebuffer[2];
    T_SIZE_T count;
    long time;
    T_SIZE_T size;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, T_SIZE_T, MYFLT, T_SIZE_T);
} ControlRead;

static void
ControlRead_readframes_i(ControlRead *self)
{
    MYFLT fpart;
    long i, mod;
    MYFLT invmodulo = 1.0 / self->modulo;

    if (self->go == 0)
        PYO_CALL_METHOD(self, "stop", NULL);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (self->go == 1)
        {
            mod = self->time % self->modulo;
            fpart = mod * invmodulo;
            self->data[i] = (*self->interp_func_ptr)(self->values, self->count, fpart, self->size);
        }
        else
        {
            mod = -1;
            self->data[i] = 0.0;
        }

        if (mod == 0)
        {
            self->count++;

            if (self->count >= (self->size - 1))
            {
                self->trigsBuffer[i] = 1.0;

                if (self->loop == 1)
                    self->count = 0;
                else
                    self->go = 0;
            }
        }

        self->time++;
    }
}

static void ControlRead_postprocessing_ii(ControlRead *self) { POST_PROCESSING_II };
static void ControlRead_postprocessing_ai(ControlRead *self) { POST_PROCESSING_AI };
static void ControlRead_postprocessing_ia(ControlRead *self) { POST_PROCESSING_IA };
static void ControlRead_postprocessing_aa(ControlRead *self) { POST_PROCESSING_AA };
static void ControlRead_postprocessing_ireva(ControlRead *self) { POST_PROCESSING_IREVA };
static void ControlRead_postprocessing_areva(ControlRead *self) { POST_PROCESSING_AREVA };
static void ControlRead_postprocessing_revai(ControlRead *self) { POST_PROCESSING_REVAI };
static void ControlRead_postprocessing_revaa(ControlRead *self) { POST_PROCESSING_REVAA };
static void ControlRead_postprocessing_revareva(ControlRead *self) { POST_PROCESSING_REVAREVA };

static void
ControlRead_setProcMode(ControlRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_readframes_i);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_postprocessing_revareva);
            break;
    }
}

static void
ControlRead_compute_next_data_frame(ControlRead *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ControlRead_traverse(ControlRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
ControlRead_clear(ControlRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
ControlRead_dealloc(ControlRead* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->values);
    PyMem_RawFree(self->trigsBuffer);
    ControlRead_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ControlRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *multmp = NULL, *addtmp = NULL;
    ControlRead *self;
    self = (ControlRead *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->loop = 0;
    self->rate = 1000;
    self->interp = 2;
    self->go = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(ControlRead_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(ControlRead_setProcMode);

    static char *kwlist[] = {"values", "rate", "loop", "interp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiOO", kwlist, &valuestmp, &self->rate, &self->loop, &self->interp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if (valuestmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setValues", valuestmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    self->modulo = (int)(self->sr / self->rate);

    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    return (PyObject *)self;
}

static PyObject * ControlRead_getServer(ControlRead* self) { GET_SERVER };
static PyObject * ControlRead_getStream(ControlRead* self) { GET_STREAM };
static PyObject * ControlRead_getTriggerStream(ControlRead* self) { GET_TRIGGER_STREAM };
static PyObject * ControlRead_setMul(ControlRead *self, PyObject *arg) { SET_MUL };
static PyObject * ControlRead_setAdd(ControlRead *self, PyObject *arg) { SET_ADD };
static PyObject * ControlRead_setSub(ControlRead *self, PyObject *arg) { SET_SUB };
static PyObject * ControlRead_setDiv(ControlRead *self, PyObject *arg) { SET_DIV };

static PyObject * ControlRead_play(ControlRead *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    self->go = 1;
    PLAY
};

static PyObject * ControlRead_stop(ControlRead *self, PyObject *args, PyObject *kwds)
{
    self->go = 0;
    STOP
};

static PyObject * ControlRead_multiply(ControlRead *self, PyObject *arg) { MULTIPLY };
static PyObject * ControlRead_inplace_multiply(ControlRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ControlRead_add(ControlRead *self, PyObject *arg) { ADD };
static PyObject * ControlRead_inplace_add(ControlRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ControlRead_sub(ControlRead *self, PyObject *arg) { SUB };
static PyObject * ControlRead_inplace_sub(ControlRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ControlRead_div(ControlRead *self, PyObject *arg) { DIV };
static PyObject * ControlRead_inplace_div(ControlRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ControlRead_setValues(ControlRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->values = (MYFLT *)PyMem_RawRealloc(self->values, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
    {
        self->values[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
    }

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setRate(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->rate = PyLong_AsLong(arg);
    self->modulo = (int)(self->sr / self->rate);

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setLoop(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setInterp(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->interp = PyLong_AsLong(PyNumber_Long(arg));
    }

    SET_INTERP_POINTER

    Py_RETURN_NONE;
}

static PyMemberDef ControlRead_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ControlRead, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ControlRead, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(ControlRead, trig_stream), 0, "Trigger Stream object."},
    {"mul", T_OBJECT_EX, offsetof(ControlRead, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(ControlRead, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRead_methods[] =
{
    {"getServer", (PyCFunction)ControlRead_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ControlRead_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)ControlRead_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)ControlRead_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)ControlRead_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setValues", (PyCFunction)ControlRead_setValues, METH_O, "Fill buffer with values in input."},
    {"setRate", (PyCFunction)ControlRead_setRate, METH_O, "Sets reading rate."},
    {"setLoop", (PyCFunction)ControlRead_setLoop, METH_O, "Sets the looping mode."},
    {"setInterp", (PyCFunction)ControlRead_setInterp, METH_O, "Sets reader interpolation mode."},
    {"setMul", (PyCFunction)ControlRead_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)ControlRead_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)ControlRead_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)ControlRead_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot ControlReadType_slots[] = {
    {Py_tp_dealloc, ControlRead_dealloc},
    {Py_tp_doc, "ControlRead objects. Generates an oscillatory waveform."},
    {Py_tp_traverse, ControlRead_traverse},
    {Py_tp_clear, ControlRead_clear},
    {Py_tp_methods, ControlRead_methods},
    {Py_tp_members, ControlRead_members},
    {Py_tp_new, ControlRead_new},
    {Py_nb_add, ControlRead_add},
    {Py_nb_subtract, ControlRead_sub},
    {Py_nb_multiply, ControlRead_multiply},
    {Py_nb_true_divide, ControlRead_div},
    {Py_nb_inplace_add, ControlRead_inplace_add},
    {Py_nb_inplace_subtract, ControlRead_inplace_sub},
    {Py_nb_inplace_multiply, ControlRead_inplace_multiply},
    {Py_nb_inplace_true_divide, ControlRead_inplace_div},
    {0, NULL}
};

static PyType_Spec ControlReadType_spec =
{
    "_pyo.ControlRead_base",
    sizeof(ControlRead),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    ControlReadType_slots
};

PyTypeObject *
PyoCreateControlReadType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &ControlReadType_spec, NULL);
}

/************/
/* NoteinRec */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *inputp;
    Stream *inputp_stream;
    PyObject *inputv;
    Stream *inputv_stream;
    PyObject *tmp_list_p;
    PyObject *tmp_list_v;
    PyObject *tmp_list_t;
    MYFLT last_pitch;
    MYFLT last_vel;
    long time;
} NoteinRec;

static void
NoteinRec_process(NoteinRec *self)
{
    int i;
    MYFLT pit, vel;

    MYFLT *inp = Stream_getData((Stream *)self->inputp_stream);
    MYFLT *inv = Stream_getData((Stream *)self->inputv_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        pit = inp[i];
        vel = inv[i];

        if (pit != self->last_pitch || vel != self->last_vel)
        {
            self->last_pitch = pit;
            self->last_vel = vel;
            PyList_Append(self->tmp_list_p, PyFloat_FromDouble(pit));
            PyList_Append(self->tmp_list_v, PyFloat_FromDouble(vel));
            PyList_Append(self->tmp_list_t, PyFloat_FromDouble( (float)self->time / self->sr) );
        }

        self->time++;
    }
}

static void
NoteinRec_setProcMode(NoteinRec *self)
{
    self->proc_func_ptr = PYO_AUDIO_CALLBACK(NoteinRec_process);
}

static void
NoteinRec_compute_next_data_frame(NoteinRec *self)
{
    (*self->proc_func_ptr)(self);
}

static int
NoteinRec_traverse(NoteinRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inputp);
    Py_VISIT(self->inputp_stream);
    Py_VISIT(self->inputv);
    Py_VISIT(self->inputv_stream);
    Py_VISIT(self->tmp_list_p);
    Py_VISIT(self->tmp_list_v);
    Py_VISIT(self->tmp_list_t);
    return 0;
}

static int
NoteinRec_clear(NoteinRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inputp);
    Py_CLEAR(self->inputp_stream);
    Py_CLEAR(self->inputv);
    Py_CLEAR(self->inputv_stream);
    Py_CLEAR(self->tmp_list_p);
    Py_CLEAR(self->tmp_list_v);
    Py_CLEAR(self->tmp_list_t);
    return 0;
}

static void
NoteinRec_dealloc(NoteinRec* self)
{
    pyo_DEALLOC
    NoteinRec_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NoteinRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputptmp, *inputp_streamtmp, *inputvtmp, *inputv_streamtmp;
    NoteinRec *self;
    self = (NoteinRec *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->tmp_list_p = PyList_New(0);
    self->tmp_list_v = PyList_New(0);
    self->tmp_list_t = PyList_New(0);
    self->last_pitch = self->last_vel = 0.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(NoteinRec_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(NoteinRec_setProcMode);

    static char *kwlist[] = {"inputp", "inputv", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputptmp, &inputvtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    self->inputp = inputptmp;
    Py_INCREF(self->inputp);
    inputp_streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->inputp, "_getStream", NULL);
    self->inputp_stream = (Stream *)inputp_streamtmp;
    Py_INCREF(self->inputp_stream);

    self->inputv = inputvtmp;
    Py_INCREF(self->inputv);
    inputv_streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->inputv, "_getStream", NULL);
    self->inputv_stream = (Stream *)inputv_streamtmp;
    Py_INCREF(self->inputv_stream);

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NoteinRec_getServer(NoteinRec* self) { GET_SERVER };
static PyObject * NoteinRec_getStream(NoteinRec* self) { GET_STREAM };

static PyObject * NoteinRec_play(NoteinRec *self, PyObject *args, PyObject *kwds)
{
    self->time = 0;
    PLAY
};

static PyObject * NoteinRec_stop(NoteinRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
NoteinRec_getData(NoteinRec *self)
{
    int i;
    PyObject *data, *point;

    Py_ssize_t size = PyList_Size(self->tmp_list_p);
    data = PyList_New(size);

    for (i = 0; i < size; i++)
    {
        point = PyTuple_New(3);
        PyTuple_SET_ITEM(point, 0, PyList_GET_ITEM(self->tmp_list_t, i));
        PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list_p, i));
        PyTuple_SET_ITEM(point, 2, PyList_GET_ITEM(self->tmp_list_v, i));
        PyList_SetItem(data, i, point);
    }

    return data;
}

static PyMemberDef NoteinRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NoteinRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinRec, stream), 0, "Stream object."},
    {"inputp", T_OBJECT_EX, offsetof(NoteinRec, inputp), 0, "Pitch input."},
    {"inputv", T_OBJECT_EX, offsetof(NoteinRec, inputv), 0, "Velocity input."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRec_methods[] =
{
    {"getServer", (PyCFunction)NoteinRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinRec_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)NoteinRec_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinRec_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"getData", (PyCFunction)NoteinRec_getData, METH_NOARGS, "Returns list of sampled points."},
    {NULL}  /* Sentinel */
};

static PyType_Slot NoteinRecType_slots[] = {
    {Py_tp_dealloc, NoteinRec_dealloc},
    {Py_tp_doc, "NoteinRec objects. Records Notein signal with user-defined sampling rate."},
    {Py_tp_traverse, NoteinRec_traverse},
    {Py_tp_clear, NoteinRec_clear},
    {Py_tp_methods, NoteinRec_methods},
    {Py_tp_members, NoteinRec_members},
    {Py_tp_new, NoteinRec_new},
    {0, NULL}
};

static PyType_Spec NoteinRecType_spec =
{
    "_pyo.NoteinRec_base",
    sizeof(NoteinRec),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    NoteinRecType_slots
};

PyTypeObject *
PyoCreateNoteinRecType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &NoteinRecType_spec, NULL);
}

/**************/
/* NoteinRead object */
/**************/
typedef struct
{
    pyo_audio_HEAD
    MYFLT *values;
    long *timestamps;
    MYFLT value;
    int loop;
    int go;
    int modebuffer[2];
    long count;
    long time;
    long size;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} NoteinRead;

static void
NoteinRead_readframes_i(NoteinRead *self)
{
    long i;

    if (self->go == 0)
        PYO_CALL_METHOD(self, "stop", NULL);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (self->go == 1)
        {
            if (self->time >= self->timestamps[self->count])
            {
                self->value = self->values[self->count];
                self->data[i] = self->value;
                self->count++;
            }
            else
                self->data[i] = self->value;
        }
        else
            self->data[i] = 0.0;

        if (self->count >= self->size)
        {
            self->trigsBuffer[i] = 1.0;

            if (self->loop == 1)
                self->time = self->count = 0;
            else
                self->go = 0;
        }

        self->time++;
    }
}

static void NoteinRead_postprocessing_ii(NoteinRead *self) { POST_PROCESSING_II };
static void NoteinRead_postprocessing_ai(NoteinRead *self) { POST_PROCESSING_AI };
static void NoteinRead_postprocessing_ia(NoteinRead *self) { POST_PROCESSING_IA };
static void NoteinRead_postprocessing_aa(NoteinRead *self) { POST_PROCESSING_AA };
static void NoteinRead_postprocessing_ireva(NoteinRead *self) { POST_PROCESSING_IREVA };
static void NoteinRead_postprocessing_areva(NoteinRead *self) { POST_PROCESSING_AREVA };
static void NoteinRead_postprocessing_revai(NoteinRead *self) { POST_PROCESSING_REVAI };
static void NoteinRead_postprocessing_revaa(NoteinRead *self) { POST_PROCESSING_REVAA };
static void NoteinRead_postprocessing_revareva(NoteinRead *self) { POST_PROCESSING_REVAREVA };

static void
NoteinRead_setProcMode(NoteinRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_readframes_i);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_postprocessing_revareva);
            break;
    }
}

static void
NoteinRead_compute_next_data_frame(NoteinRead *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
NoteinRead_traverse(NoteinRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
NoteinRead_clear(NoteinRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
NoteinRead_dealloc(NoteinRead* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->values);
    PyMem_RawFree(self->timestamps);
    PyMem_RawFree(self->trigsBuffer);
    NoteinRead_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NoteinRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *timestampstmp, *multmp = NULL, *addtmp = NULL;
    NoteinRead *self;
    self = (NoteinRead *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = 0.0;
    self->loop = 0;
    self->go = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(NoteinRead_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(NoteinRead_setProcMode);

    static char *kwlist[] = {"values", "timestamps", "loop", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &valuestmp, &timestampstmp, &self->loop, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if (valuestmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setValues", valuestmp);
    }

    if (timestampstmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setTimestamps", timestampstmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NoteinRead_getServer(NoteinRead* self) { GET_SERVER };
static PyObject * NoteinRead_getStream(NoteinRead* self) { GET_STREAM };
static PyObject * NoteinRead_getTriggerStream(NoteinRead* self) { GET_TRIGGER_STREAM };
static PyObject * NoteinRead_setMul(NoteinRead *self, PyObject *arg) { SET_MUL };
static PyObject * NoteinRead_setAdd(NoteinRead *self, PyObject *arg) { SET_ADD };
static PyObject * NoteinRead_setSub(NoteinRead *self, PyObject *arg) { SET_SUB };
static PyObject * NoteinRead_setDiv(NoteinRead *self, PyObject *arg) { SET_DIV };

static PyObject * NoteinRead_play(NoteinRead *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    self->go = 1;
    PLAY
};

static PyObject * NoteinRead_stop(NoteinRead *self, PyObject *args, PyObject *kwds)
{
    self->go = 0;
    STOP
};

static PyObject * NoteinRead_multiply(NoteinRead *self, PyObject *arg) { MULTIPLY };
static PyObject * NoteinRead_inplace_multiply(NoteinRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NoteinRead_add(NoteinRead *self, PyObject *arg) { ADD };
static PyObject * NoteinRead_inplace_add(NoteinRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NoteinRead_sub(NoteinRead *self, PyObject *arg) { SUB };
static PyObject * NoteinRead_inplace_sub(NoteinRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NoteinRead_div(NoteinRead *self, PyObject *arg) { DIV };
static PyObject * NoteinRead_inplace_div(NoteinRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
NoteinRead_setValues(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->values = (MYFLT *)PyMem_RawRealloc(self->values, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
    {
        self->values[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
    }

    Py_RETURN_NONE;
}

static PyObject *
NoteinRead_setTimestamps(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->timestamps = (long *)PyMem_RawRealloc(self->timestamps, self->size * sizeof(long));

    for (i = 0; i < self->size; i++)
    {
        self->timestamps[i] = (long)(PyFloat_AsDouble(PyList_GET_ITEM(arg, i)) * self->sr);
    }

    Py_RETURN_NONE;
}

static PyObject *
NoteinRead_setLoop(NoteinRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyMemberDef NoteinRead_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NoteinRead, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinRead, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(NoteinRead, trig_stream), 0, "Trigger Stream object."},
    {"mul", T_OBJECT_EX, offsetof(NoteinRead, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(NoteinRead, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRead_methods[] =
{
    {"getServer", (PyCFunction)NoteinRead_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinRead_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)NoteinRead_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)NoteinRead_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinRead_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setValues", (PyCFunction)NoteinRead_setValues, METH_O, "Fill buffer with values in input."},
    {"setTimestamps", (PyCFunction)NoteinRead_setTimestamps, METH_O, "Fill buffer with timestamps in input."},
    {"setLoop", (PyCFunction)NoteinRead_setLoop, METH_O, "Sets the looping mode."},
    {"setMul", (PyCFunction)NoteinRead_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)NoteinRead_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)NoteinRead_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)NoteinRead_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot NoteinReadType_slots[] = {
    {Py_tp_dealloc, NoteinRead_dealloc},
    {Py_tp_doc, "NoteinRead objects. Reads a NoteinRec file."},
    {Py_tp_traverse, NoteinRead_traverse},
    {Py_tp_clear, NoteinRead_clear},
    {Py_tp_methods, NoteinRead_methods},
    {Py_tp_members, NoteinRead_members},
    {Py_tp_new, NoteinRead_new},
    {Py_nb_add, NoteinRead_add},
    {Py_nb_subtract, NoteinRead_sub},
    {Py_nb_multiply, NoteinRead_multiply},
    {Py_nb_true_divide, NoteinRead_div},
    {Py_nb_inplace_add, NoteinRead_inplace_add},
    {Py_nb_inplace_subtract, NoteinRead_inplace_sub},
    {Py_nb_inplace_multiply, NoteinRead_inplace_multiply},
    {Py_nb_inplace_true_divide, NoteinRead_inplace_div},
    {0, NULL}
};

static PyType_Spec NoteinReadType_spec =
{
    "_pyo.NoteinRead_base",
    sizeof(NoteinRead),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    NoteinReadType_slots
};

PyTypeObject *
PyoCreateNoteinReadType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &NoteinReadType_spec, NULL);
}
