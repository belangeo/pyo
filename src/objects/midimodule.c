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

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct
{
    pyo_audio_HEAD
    PyObject *callable;
    int ctlnumber;
    int toprint;
} CtlScan;

static void
CtlScan_setProcMode(CtlScan *self) {}

static void
CtlScan_compute_next_data_frame(CtlScan *self)
{
    PyoMidiEvent *buffer;
    int i, count;

    buffer = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
    {
        PyObject *tup;

        for (i = 0; i < count; i++)
        {
            int status = PyoMidi_MessageStatus(buffer[i].message);    // Temp note event holders
            int number = PyoMidi_MessageData1(buffer[i].message);
            int value = PyoMidi_MessageData2(buffer[i].message);

            if ((status & 0xF0) == 0xB0)
            {
                if (number != self->ctlnumber)
                {
                    PyObject *result;
                    self->ctlnumber = number;
                    tup = PyTuple_New(1);
                    PyTuple_SetItem(tup, 0, PyLong_FromLong(self->ctlnumber));
                    result = PyObject_Call((PyObject *)self->callable, tup, NULL);
                    Py_DECREF(tup);
                    if (result == NULL)
                        PyErr_Print();
                    else
                        Py_DECREF(result);
                }

                if (self->toprint == 1)
                    PySys_WriteStdout("ctl number : %i, ctl value : %i, midi channel : %i\n", self->ctlnumber, value, status - 0xB0 + 1);
            }
        }
    }
}

static int
CtlScan_traverse(CtlScan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    return 0;
}

static int
CtlScan_clear(CtlScan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    return 0;
}

static void
CtlScan_dealloc(CtlScan* self)
{
    pyo_DEALLOC
    CtlScan_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CtlScan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *calltmp = NULL;
    CtlScan *self;
    self = (CtlScan *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->ctlnumber = -1;
    self->toprint = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(CtlScan_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(CtlScan_setProcMode);

    static char *kwlist[] = {"callable", "toprint", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &calltmp, &self->toprint)) {
        Py_DECREF(self);
        return NULL;
    }

    if (calltmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setFunction", calltmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    return (PyObject *)self;
}

static PyObject * CtlScan_getServer(CtlScan* self) { GET_SERVER };
static PyObject * CtlScan_getStream(CtlScan* self) { GET_STREAM };

static PyObject * CtlScan_play(CtlScan *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CtlScan_stop(CtlScan *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
CtlScan_reset(CtlScan *self)
{
    self->ctlnumber = -1;
    Py_RETURN_NONE;
};

static PyObject *
CtlScan_setFunction(CtlScan *self, PyObject *arg)
{
    if (! PyCallable_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->callable);
    self->callable = arg;
    Py_INCREF(self->callable);

    Py_RETURN_NONE;
}

static PyObject *
CtlScan_setToprint(CtlScan *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->toprint = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}
static PyMemberDef CtlScan_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CtlScan, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(CtlScan, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CtlScan_methods[] =
{
    {"getServer", (PyCFunction)CtlScan_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)CtlScan_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)CtlScan_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)CtlScan_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"reset", (PyCFunction)CtlScan_reset, METH_NOARGS, "Resets the scanned number."},
    {"setFunction", (PyCFunction)CtlScan_setFunction, METH_O, "Sets the function to be called."},
    {"setToprint", (PyCFunction)CtlScan_setToprint, METH_O, "If True, print values to the console."},
    {NULL}  /* Sentinel */
};

static PyType_Slot CtlScanType_slots[] = {
    {Py_tp_dealloc, CtlScan_dealloc},
    {Py_tp_doc, "CtlScan objects. Retreive controller numbers from a Midi input."},
    {Py_tp_traverse, CtlScan_traverse},
    {Py_tp_clear, CtlScan_clear},
    {Py_tp_methods, CtlScan_methods},
    {Py_tp_members, CtlScan_members},
    {Py_tp_new, CtlScan_new},
    {0, NULL}
};

static PyType_Spec CtlScanType_spec =
{
    "_pyo.CtlScan_base",
    sizeof(CtlScan),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    CtlScanType_slots
};

PyTypeObject *
PyoCreateCtlScanType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &CtlScanType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *callable;
    int ctlnumber;
    int midichnl;
    int toprint;
} CtlScan2;

static void
CtlScan2_setProcMode(CtlScan2 *self) {}

static void
CtlScan2_compute_next_data_frame(CtlScan2 *self)
{
    PyoMidiEvent *buffer;
    int i, count, midichnl;

    buffer = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
    {
        PyObject *tup;

        for (i = 0; i < count; i++)
        {
            int status = PyoMidi_MessageStatus(buffer[i].message); // Temp note event holders
            int number = PyoMidi_MessageData1(buffer[i].message);
            int value = PyoMidi_MessageData2(buffer[i].message);

            if ((status & 0xF0) == 0xB0)
            {
                midichnl = status - 0xB0 + 1;

                if (number != self->ctlnumber || midichnl != self->midichnl)
                {
                    PyObject *result;
                    self->ctlnumber = number;
                    self->midichnl = midichnl;
                    tup = PyTuple_New(2);
                    PyTuple_SetItem(tup, 0, PyLong_FromLong(self->ctlnumber));
                    PyTuple_SetItem(tup, 1, PyLong_FromLong(self->midichnl));
                    result = PyObject_Call((PyObject *)self->callable, tup, NULL);
                    Py_DECREF(tup);
                    if (result == NULL)
                        PyErr_Print();
                    else
                        Py_DECREF(result);
                }

                if (self->toprint == 1)
                    PySys_WriteStdout("ctl number : %i, ctl value : %i, midi channel : %i\n", self->ctlnumber, value, midichnl);
            }
        }
    }
}

static int
CtlScan2_traverse(CtlScan2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    return 0;
}

static int
CtlScan2_clear(CtlScan2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    return 0;
}

static void
CtlScan2_dealloc(CtlScan2* self)
{
    pyo_DEALLOC
    CtlScan2_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CtlScan2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *calltmp = NULL;
    CtlScan2 *self;
    self = (CtlScan2 *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->ctlnumber = self->midichnl = -1;
    self->toprint = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(CtlScan2_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(CtlScan2_setProcMode);

    static char *kwlist[] = {"callable", "toprint", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &calltmp, &self->toprint)) {
        Py_DECREF(self);
        return NULL;
    }

    if (calltmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setFunction", calltmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    return (PyObject *)self;
}

static PyObject * CtlScan2_getServer(CtlScan2* self) { GET_SERVER };
static PyObject * CtlScan2_getStream(CtlScan2* self) { GET_STREAM };

static PyObject * CtlScan2_play(CtlScan2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CtlScan2_stop(CtlScan2 *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
CtlScan2_reset(CtlScan2 *self)
{
    self->ctlnumber = self->midichnl = -1;
    Py_RETURN_NONE;
};

static PyObject *
CtlScan2_setFunction(CtlScan2 *self, PyObject *arg)
{
    if (! PyCallable_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->callable);
    self->callable = arg;
    Py_INCREF(self->callable);

    Py_RETURN_NONE;
}

static PyObject *
CtlScan2_setToprint(CtlScan2 *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->toprint = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}
static PyMemberDef CtlScan2_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CtlScan2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(CtlScan2, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CtlScan2_methods[] =
{
    {"getServer", (PyCFunction)CtlScan2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)CtlScan2_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)CtlScan2_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)CtlScan2_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"reset", (PyCFunction)CtlScan2_reset, METH_NOARGS, "Reset scanned numbers."},
    {"setFunction", (PyCFunction)CtlScan2_setFunction, METH_O, "Sets the function to be called."},
    {"setToprint", (PyCFunction)CtlScan2_setToprint, METH_O, "If True, print values to the console."},
    {NULL}  /* Sentinel */
};

static PyType_Slot CtlScan2Type_slots[] = {
    {Py_tp_dealloc, CtlScan2_dealloc},
    {Py_tp_doc, "CtlScan2 objects. Retreive midi channel and controller numbers from a midi input."},
    {Py_tp_traverse, CtlScan2_traverse},
    {Py_tp_clear, CtlScan2_clear},
    {Py_tp_methods, CtlScan2_methods},
    {Py_tp_members, CtlScan2_members},
    {Py_tp_new, CtlScan2_new},
    {0, NULL}
};

static PyType_Spec CtlScan2Type_spec =
{
    "_pyo.CtlScan2_base",
    sizeof(CtlScan2),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    CtlScan2Type_slots
};

PyTypeObject *
PyoCreateCtlScan2Type(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &CtlScan2Type_spec, NULL);
}

int
getPosToWrite(long timestamp, Server *server, double sr, int bufsize)
{
    /* With JackMidi, the timestamp is already a sample offset inside the buffer size. */
    if (server->withJackMidi)
    {
        return (int)timestamp;
    }
    else
    {
        int offset = 0;
        long realtimestamp, elapsed, ms;
        realtimestamp = timestamp - Server_getMidiTimeOffset(server);

        if (realtimestamp < 0)
            return 0;

        elapsed = (long)(Server_getElapsedTime(server) / sr * 1000);
        ms = realtimestamp - (elapsed - (long)(bufsize / sr * 1000));
        offset = (int)(ms * 0.001 * sr);

        if (offset < 0)
            offset = 0;
        else if (offset >= bufsize)
            offset = bufsize - 1;

        return offset;
    }
}

typedef struct
{
    pyo_audio_HEAD
    int ctlnumber;
    int channel;
    MYFLT minscale;
    MYFLT maxscale;
    MYFLT value;
    int modebuffer[2];
} Midictl;

static void Midictl_postprocessing_ii(Midictl *self) { POST_PROCESSING_II };
static void Midictl_postprocessing_ai(Midictl *self) { POST_PROCESSING_AI };
static void Midictl_postprocessing_ia(Midictl *self) { POST_PROCESSING_IA };
static void Midictl_postprocessing_aa(Midictl *self) { POST_PROCESSING_AA };
static void Midictl_postprocessing_ireva(Midictl *self) { POST_PROCESSING_IREVA };
static void Midictl_postprocessing_areva(Midictl *self) { POST_PROCESSING_AREVA };
static void Midictl_postprocessing_revai(Midictl *self) { POST_PROCESSING_REVAI };
static void Midictl_postprocessing_revaa(Midictl *self) { POST_PROCESSING_REVAA };
static void Midictl_postprocessing_revareva(Midictl *self) { POST_PROCESSING_REVAREVA };

static void
Midictl_setProcMode(Midictl *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Midictl_postprocessing_revareva);
            break;
    }
}

int
Midictl_translateMidi(Midictl *self, PyoMidiEvent *buffer, int j)
{
    int ok, posto = -1;
    int status = PyoMidi_MessageStatus(buffer[j].message);    // Temp note event holders
    int number = PyoMidi_MessageData1(buffer[j].message);
    int value = PyoMidi_MessageData2(buffer[j].message);

    if (self->channel == 0)
    {
        if ((status & 0xF0) == 0xB0)
            ok = 1;
        else
            ok = 0;
    }
    else
    {
        if (status == (0xB0 | (self->channel - 1)))
            ok = 1;
        else
            ok = 0;
    }

    if (ok == 1 && number == self->ctlnumber)
    {
        self->value = (value / 127.) * (self->maxscale - self->minscale) + self->minscale;
        posto = getPosToWrite(buffer[j].timestamp, (Server *)self->server, self->sr, self->bufsize);
    }

    return posto;
}

static void
Midictl_compute_next_data_frame(Midictl *self)
{
    PyoMidiEvent *tmp;
    int i, j, count, posto, oldpos = 0;
    MYFLT oldval = 0.0;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count == 0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }
    else
    {
        for (j = 0; j < count; j++)
        {
            oldval = self->value;
            posto = Midictl_translateMidi((Midictl *)self, tmp, j);

            if (posto == -1)
                continue;

            for (i = oldpos; i < posto; i++)
            {
                self->data[i] = oldval;
            }

            oldpos = posto;
        }

        for (i = oldpos; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }

    (*self->muladd_func_ptr)(self);
}

static int
Midictl_traverse(Midictl *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Midictl_clear(Midictl *self)
{
    pyo_CLEAR
    return 0;
}

static void
Midictl_dealloc(Midictl* self)
{
    pyo_DEALLOC
    Midictl_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Midictl_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp = NULL, *addtmp = NULL;
    Midictl *self;
    self = (Midictl *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->channel = 0;
    self->value = 0.;
    self->minscale = 0.;
    self->maxscale = 1.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Midictl_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Midictl_setProcMode);

    static char *kwlist[] = {"ctlnumber", "minscale", "maxscale", "init", "channel", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_I_FFFIOO, kwlist, &self->ctlnumber, &self->minscale, &self->maxscale, &self->value, &self->channel, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
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

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Midictl_getServer(Midictl* self) { GET_SERVER };
static PyObject * Midictl_getStream(Midictl* self) { GET_STREAM };
static PyObject * Midictl_setMul(Midictl *self, PyObject *arg) { SET_MUL };
static PyObject * Midictl_setAdd(Midictl *self, PyObject *arg) { SET_ADD };
static PyObject * Midictl_setSub(Midictl *self, PyObject *arg) { SET_SUB };
static PyObject * Midictl_setDiv(Midictl *self, PyObject *arg) { SET_DIV };

static PyObject * Midictl_play(Midictl *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Midictl_stop(Midictl *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Midictl_multiply(Midictl *self, PyObject *arg) { MULTIPLY };
static PyObject * Midictl_inplace_multiply(Midictl *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Midictl_add(Midictl *self, PyObject *arg) { ADD };
static PyObject * Midictl_inplace_add(Midictl *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Midictl_sub(Midictl *self, PyObject *arg) { SUB };
static PyObject * Midictl_inplace_sub(Midictl *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Midictl_div(Midictl *self, PyObject *arg) { DIV };
static PyObject * Midictl_inplace_div(Midictl *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Midictl_setValue(Midictl *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->value = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Midictl_setMinScale(Midictl *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->minscale = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Midictl_setMaxScale(Midictl *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->maxscale = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Midictl_setCtlNumber(Midictl *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->ctlnumber = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
Midictl_setChannel(Midictl *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->channel = tmp;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Midictl_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Midictl, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Midictl, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Midictl, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Midictl, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Midictl_methods[] =
{
    {"getServer", (PyCFunction)Midictl_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Midictl_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Midictl_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Midictl_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setValue", (PyCFunction)Midictl_setValue, METH_O, "Resets audio stream to value in argument."},
    {"setMinScale", (PyCFunction)Midictl_setMinScale, METH_O, "Sets the minimum value of scaling."},
    {"setMaxScale", (PyCFunction)Midictl_setMaxScale, METH_O, "Sets the maximum value of scaling."},
    {"setCtlNumber", (PyCFunction)Midictl_setCtlNumber, METH_O, "Sets the controller number."},
    {"setChannel", (PyCFunction)Midictl_setChannel, METH_O, "Sets the midi channel."},
    {"setMul", (PyCFunction)Midictl_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Midictl_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Midictl_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Midictl_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot MidictlType_slots[] = {
    {Py_tp_dealloc, Midictl_dealloc},
    {Py_tp_doc, "Midictl objects. Retreive audio from an input channel."},
    {Py_tp_traverse, Midictl_traverse},
    {Py_tp_clear, Midictl_clear},
    {Py_tp_methods, Midictl_methods},
    {Py_tp_members, Midictl_members},
    {Py_tp_new, Midictl_new},
    {Py_nb_add, Midictl_add},
    {Py_nb_subtract, Midictl_sub},
    {Py_nb_multiply, Midictl_multiply},
    {Py_nb_true_divide, Midictl_div},
    {Py_nb_inplace_add, Midictl_inplace_add},
    {Py_nb_inplace_subtract, Midictl_inplace_sub},
    {Py_nb_inplace_multiply, Midictl_inplace_multiply},
    {Py_nb_inplace_true_divide, Midictl_inplace_div},
    {0, NULL}
};

static PyType_Spec MidictlType_spec =
{
    "_pyo.Midictl_base",
    sizeof(Midictl),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    MidictlType_slots
};

PyTypeObject *
PyoCreateMidictlType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &MidictlType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    int channel;
    int scale; /* 0 = midi, 1 = transpo */
    MYFLT range;
    MYFLT value;
    int modebuffer[2];
} Bendin;

static void Bendin_postprocessing_ii(Bendin *self) { POST_PROCESSING_II };
static void Bendin_postprocessing_ai(Bendin *self) { POST_PROCESSING_AI };
static void Bendin_postprocessing_ia(Bendin *self) { POST_PROCESSING_IA };
static void Bendin_postprocessing_aa(Bendin *self) { POST_PROCESSING_AA };
static void Bendin_postprocessing_ireva(Bendin *self) { POST_PROCESSING_IREVA };
static void Bendin_postprocessing_areva(Bendin *self) { POST_PROCESSING_AREVA };
static void Bendin_postprocessing_revai(Bendin *self) { POST_PROCESSING_REVAI };
static void Bendin_postprocessing_revaa(Bendin *self) { POST_PROCESSING_REVAA };
static void Bendin_postprocessing_revareva(Bendin *self) { POST_PROCESSING_REVAREVA };

static void
Bendin_setProcMode(Bendin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Bendin_postprocessing_revareva);
            break;
    }
}

int
Bendin_translateMidi(Bendin *self, PyoMidiEvent *buffer, int j)
{
    int ok, posto = -1;
    MYFLT val;

    int status = PyoMidi_MessageStatus(buffer[j].message);    // Temp note event holders
    int number = PyoMidi_MessageData1(buffer[j].message);
    int value = PyoMidi_MessageData2(buffer[j].message);

    if (self->channel == 0)
    {
        if ((status & 0xF0) == 0xe0)
            ok = 1;
        else
            ok = 0;
    }
    else
    {
        if (status == (0xe0 | (self->channel - 1)))
            ok = 1;
        else
            ok = 0;
    }

    if (ok == 1)
    {
        val = (number + (value << 7) - 8192) / 8192.0 * self->range;

        if (self->scale == 0)
            self->value = val;
        else
            self->value = MYPOW(1.0594630943593, val);

        posto = getPosToWrite(buffer[j].timestamp, (Server *)self->server, self->sr, self->bufsize);
    }

    return posto;
}

static void
Bendin_compute_next_data_frame(Bendin *self)
{
    PyoMidiEvent *tmp;
    int i, j, count, posto, oldpos = 0;
    MYFLT oldval = 0.0;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count == 0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }
    else
    {
        for (j = 0; j < count; j++)
        {
            oldval = self->value;
            posto = Bendin_translateMidi((Bendin *)self, tmp, j);

            if (posto == -1)
                continue;

            for (i = oldpos; i < posto; i++)
            {
                self->data[i] = oldval;
            }

            oldpos = posto;
        }

        for (i = oldpos; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }

    (*self->muladd_func_ptr)(self);
}

static int
Bendin_traverse(Bendin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Bendin_clear(Bendin *self)
{
    pyo_CLEAR
    return 0;
}

static void
Bendin_dealloc(Bendin* self)
{
    pyo_DEALLOC
    Bendin_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Bendin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp = NULL, *addtmp = NULL;
    Bendin *self;
    self = (Bendin *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->channel = 0;
    self->scale = 0;
    self->value = 0.;
    self->range = 2.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Bendin_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Bendin_setProcMode);

    static char *kwlist[] = {"brange", "scale", "channel", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FIIOO, kwlist, &self->range, &self->scale, &self->channel, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
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

    if (self->scale == 1)
        self->value = 1.0;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Bendin_getServer(Bendin* self) { GET_SERVER };
static PyObject * Bendin_getStream(Bendin* self) { GET_STREAM };
static PyObject * Bendin_setMul(Bendin *self, PyObject *arg) { SET_MUL };
static PyObject * Bendin_setAdd(Bendin *self, PyObject *arg) { SET_ADD };
static PyObject * Bendin_setSub(Bendin *self, PyObject *arg) { SET_SUB };
static PyObject * Bendin_setDiv(Bendin *self, PyObject *arg) { SET_DIV };

static PyObject * Bendin_play(Bendin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Bendin_stop(Bendin *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Bendin_multiply(Bendin *self, PyObject *arg) { MULTIPLY };
static PyObject * Bendin_inplace_multiply(Bendin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Bendin_add(Bendin *self, PyObject *arg) { ADD };
static PyObject * Bendin_inplace_add(Bendin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Bendin_sub(Bendin *self, PyObject *arg) { SUB };
static PyObject * Bendin_inplace_sub(Bendin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Bendin_div(Bendin *self, PyObject *arg) { DIV };
static PyObject * Bendin_inplace_div(Bendin *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Bendin_setBrange(Bendin *self, PyObject *arg)
{
    MYFLT tmp;

    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        tmp = PyFloat_AsDouble(arg);

        if (tmp >= 0.0 && tmp < 128.0)
            self->range = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
Bendin_setChannel(Bendin *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->channel = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
Bendin_setScale(Bendin *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp == 0)
            self->scale = 0;
        else if (tmp == 1)
            self->scale = 1;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Bendin_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Bendin, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Bendin, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Bendin, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Bendin, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Bendin_methods[] =
{
    {"getServer", (PyCFunction)Bendin_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Bendin_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Bendin_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Bendin_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setBrange", (PyCFunction)Bendin_setBrange, METH_O, "Sets the bending bipolar range."},
    {"setScale", (PyCFunction)Bendin_setScale, METH_O, "Sets the output type, midi vs transpo."},
    {"setChannel", (PyCFunction)Bendin_setChannel, METH_O, "Sets the midi channel."},
    {"setMul", (PyCFunction)Bendin_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Bendin_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Bendin_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Bendin_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot BendinType_slots[] = {
    {Py_tp_dealloc, Bendin_dealloc},
    {Py_tp_doc, "Bendin objects. Retreive audio from an input channel."},
    {Py_tp_traverse, Bendin_traverse},
    {Py_tp_clear, Bendin_clear},
    {Py_tp_methods, Bendin_methods},
    {Py_tp_members, Bendin_members},
    {Py_tp_new, Bendin_new},
    {Py_nb_add, Bendin_add},
    {Py_nb_subtract, Bendin_sub},
    {Py_nb_multiply, Bendin_multiply},
    {Py_nb_true_divide, Bendin_div},
    {Py_nb_inplace_add, Bendin_inplace_add},
    {Py_nb_inplace_subtract, Bendin_inplace_sub},
    {Py_nb_inplace_multiply, Bendin_inplace_multiply},
    {Py_nb_inplace_true_divide, Bendin_inplace_div},
    {0, NULL}
};

static PyType_Spec BendinType_spec =
{
    "_pyo.Bendin_base",
    sizeof(Bendin),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    BendinType_slots
};

PyTypeObject *
PyoCreateBendinType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &BendinType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    int channel;
    MYFLT minscale;
    MYFLT maxscale;
    MYFLT value;
    int modebuffer[2];
} Touchin;

static void Touchin_postprocessing_ii(Touchin *self) { POST_PROCESSING_II };
static void Touchin_postprocessing_ai(Touchin *self) { POST_PROCESSING_AI };
static void Touchin_postprocessing_ia(Touchin *self) { POST_PROCESSING_IA };
static void Touchin_postprocessing_aa(Touchin *self) { POST_PROCESSING_AA };
static void Touchin_postprocessing_ireva(Touchin *self) { POST_PROCESSING_IREVA };
static void Touchin_postprocessing_areva(Touchin *self) { POST_PROCESSING_AREVA };
static void Touchin_postprocessing_revai(Touchin *self) { POST_PROCESSING_REVAI };
static void Touchin_postprocessing_revaa(Touchin *self) { POST_PROCESSING_REVAA };
static void Touchin_postprocessing_revareva(Touchin *self) { POST_PROCESSING_REVAREVA };

static void
Touchin_setProcMode(Touchin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Touchin_postprocessing_revareva);
            break;
    }
}

int
Touchin_translateMidi(Touchin *self, PyoMidiEvent *buffer, int j)
{
    int ok, posto = -1;
    int status = PyoMidi_MessageStatus(buffer[j].message);    // Temp note event holders
    int number = PyoMidi_MessageData1(buffer[j].message);

    if (self->channel == 0)
    {
        if ((status & 0xF0) == 0xd0)
            ok = 1;
        else
            ok = 0;
    }
    else
    {
        if (status == (0xd0 | (self->channel - 1)))
            ok = 1;
        else
            ok = 0;
    }

    if (ok == 1)
    {
        self->value = (number / 127.) * (self->maxscale - self->minscale) + self->minscale;
        posto = getPosToWrite(buffer[j].timestamp, (Server *)self->server, self->sr, self->bufsize);
    }

    return posto;
}

static void
Touchin_compute_next_data_frame(Touchin *self)
{
    PyoMidiEvent *tmp;
    int i, j, count, posto, oldpos = 0;
    MYFLT oldval = 0.0;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count == 0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }
    else
    {
        for (j = 0; j < count; j++)
        {
            oldval = self->value;
            posto = Touchin_translateMidi((Touchin *)self, tmp, j);

            if (posto == -1)
                continue;

            for (i = oldpos; i < posto; i++)
            {
                self->data[i] = oldval;
            }

            oldpos = posto;
        }

        for (i = oldpos; i < self->bufsize; i++)
        {
            self->data[i] = self->value;
        }
    }

    (*self->muladd_func_ptr)(self);
}

static int
Touchin_traverse(Touchin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Touchin_clear(Touchin *self)
{
    pyo_CLEAR
    return 0;
}

static void
Touchin_dealloc(Touchin* self)
{
    pyo_DEALLOC
    Touchin_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Touchin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp = NULL, *addtmp = NULL;
    Touchin *self;
    self = (Touchin *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->channel = 0;
    self->value = 0.;
    self->minscale = 0.;
    self->maxscale = 1.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Touchin_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Touchin_setProcMode);

    static char *kwlist[] = {"minscale", "maxscale", "init", "channel", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFFIOO, kwlist, &self->minscale, &self->maxscale, &self->value, &self->channel, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
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

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Touchin_getServer(Touchin* self) { GET_SERVER };
static PyObject * Touchin_getStream(Touchin* self) { GET_STREAM };
static PyObject * Touchin_setMul(Touchin *self, PyObject *arg) { SET_MUL };
static PyObject * Touchin_setAdd(Touchin *self, PyObject *arg) { SET_ADD };
static PyObject * Touchin_setSub(Touchin *self, PyObject *arg) { SET_SUB };
static PyObject * Touchin_setDiv(Touchin *self, PyObject *arg) { SET_DIV };

static PyObject * Touchin_play(Touchin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Touchin_stop(Touchin *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Touchin_multiply(Touchin *self, PyObject *arg) { MULTIPLY };
static PyObject * Touchin_inplace_multiply(Touchin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Touchin_add(Touchin *self, PyObject *arg) { ADD };
static PyObject * Touchin_inplace_add(Touchin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Touchin_sub(Touchin *self, PyObject *arg) { SUB };
static PyObject * Touchin_inplace_sub(Touchin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Touchin_div(Touchin *self, PyObject *arg) { DIV };
static PyObject * Touchin_inplace_div(Touchin *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Touchin_setMinScale(Touchin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg) == 1)
    {
        self->minscale = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Touchin_setMaxScale(Touchin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg) == 1)
    {
        self->maxscale = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Touchin_setChannel(Touchin *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->channel = tmp;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Touchin_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Touchin, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Touchin, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Touchin, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Touchin, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Touchin_methods[] =
{
    {"getServer", (PyCFunction)Touchin_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Touchin_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Touchin_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Touchin_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMinScale", (PyCFunction)Touchin_setMinScale, METH_O, "Sets the minimum value of scaling."},
    {"setMaxScale", (PyCFunction)Touchin_setMaxScale, METH_O, "Sets the maximum value of scaling."},
    {"setChannel", (PyCFunction)Touchin_setChannel, METH_O, "Sets the midi channel."},
    {"setMul", (PyCFunction)Touchin_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Touchin_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Touchin_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Touchin_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TouchinType_slots[] = {
    {Py_tp_dealloc, Touchin_dealloc},
    {Py_tp_doc, "Touchin objects. Retrieve the signal of an aftertouch midi controller."},
    {Py_tp_traverse, Touchin_traverse},
    {Py_tp_clear, Touchin_clear},
    {Py_tp_methods, Touchin_methods},
    {Py_tp_members, Touchin_members},
    {Py_tp_new, Touchin_new},
    {Py_nb_add, Touchin_add},
    {Py_nb_subtract, Touchin_sub},
    {Py_nb_multiply, Touchin_multiply},
    {Py_nb_true_divide, Touchin_div},
    {Py_nb_inplace_add, Touchin_inplace_add},
    {Py_nb_inplace_subtract, Touchin_inplace_sub},
    {Py_nb_inplace_multiply, Touchin_inplace_multiply},
    {Py_nb_inplace_true_divide, Touchin_inplace_div},
    {0, NULL}
};

static PyType_Spec TouchinType_spec =
{
    "_pyo.Touchin_base",
    sizeof(Touchin),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TouchinType_slots
};

PyTypeObject *
PyoCreateTouchinType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TouchinType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    int channel;
    MYFLT value;
    int modebuffer[2];
} Programin;

static void Programin_postprocessing_ii(Programin *self) { POST_PROCESSING_II };
static void Programin_postprocessing_ai(Programin *self) { POST_PROCESSING_AI };
static void Programin_postprocessing_ia(Programin *self) { POST_PROCESSING_IA };
static void Programin_postprocessing_aa(Programin *self) { POST_PROCESSING_AA };
static void Programin_postprocessing_ireva(Programin *self) { POST_PROCESSING_IREVA };
static void Programin_postprocessing_areva(Programin *self) { POST_PROCESSING_AREVA };
static void Programin_postprocessing_revai(Programin *self) { POST_PROCESSING_REVAI };
static void Programin_postprocessing_revaa(Programin *self) { POST_PROCESSING_REVAA };
static void Programin_postprocessing_revareva(Programin *self) { POST_PROCESSING_REVAREVA };

static void
Programin_setProcMode(Programin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Programin_postprocessing_revareva);
            break;
    }
}

// Take MIDI events and translate them...
void Programin_translateMidi(Programin *self, PyoMidiEvent *buffer, int count)
{
    int i, ok;

    for (i = 0; i < count; i++)
    {
        int status = PyoMidi_MessageStatus(buffer[i].message);    // Temp note event holders
        int number = PyoMidi_MessageData1(buffer[i].message);

        if (self->channel == 0)
        {
            if ((status & 0xF0) == 0xc0)
                ok = 1;
            else
                ok = 0;
        }
        else
        {
            if (status == (0xc0 | (self->channel - 1)))
                ok = 1;
            else
                ok = 0;
        }

        if (ok == 1)
        {
            self->value = (MYFLT)number;
            break;
        }
    }
}

static void
Programin_compute_next_data_frame(Programin *self)
{
    PyoMidiEvent *tmp;
    int i, count;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
        Programin_translateMidi((Programin *)self, tmp, count);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = self->value;
    }

    (*self->muladd_func_ptr)(self);
}

static int
Programin_traverse(Programin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Programin_clear(Programin *self)
{
    pyo_CLEAR
    return 0;
}

static void
Programin_dealloc(Programin* self)
{
    pyo_DEALLOC
    Programin_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Programin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp = NULL, *addtmp = NULL;
    Programin *self;
    self = (Programin *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->channel = 0;
    self->value = 0.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Programin_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Programin_setProcMode);

    static char *kwlist[] = {"channel", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iOO", kwlist, &self->channel, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
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

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Programin_getServer(Programin* self) { GET_SERVER };
static PyObject * Programin_getStream(Programin* self) { GET_STREAM };
static PyObject * Programin_setMul(Programin *self, PyObject *arg) { SET_MUL };
static PyObject * Programin_setAdd(Programin *self, PyObject *arg) { SET_ADD };
static PyObject * Programin_setSub(Programin *self, PyObject *arg) { SET_SUB };
static PyObject * Programin_setDiv(Programin *self, PyObject *arg) { SET_DIV };

static PyObject * Programin_play(Programin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Programin_stop(Programin *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Programin_multiply(Programin *self, PyObject *arg) { MULTIPLY };
static PyObject * Programin_inplace_multiply(Programin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Programin_add(Programin *self, PyObject *arg) { ADD };
static PyObject * Programin_inplace_add(Programin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Programin_sub(Programin *self, PyObject *arg) { SUB };
static PyObject * Programin_inplace_sub(Programin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Programin_div(Programin *self, PyObject *arg) { DIV };
static PyObject * Programin_inplace_div(Programin *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Programin_setChannel(Programin *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->channel = tmp;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Programin_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Programin, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Programin, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Programin, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Programin, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Programin_methods[] =
{
    {"getServer", (PyCFunction)Programin_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Programin_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Programin_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Programin_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setChannel", (PyCFunction)Programin_setChannel, METH_O, "Sets the midi channel."},
    {"setMul", (PyCFunction)Programin_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Programin_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Programin_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Programin_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot PrograminType_slots[] = {
    {Py_tp_dealloc, Programin_dealloc},
    {Py_tp_doc, "Programin objects. Retrieve the signal of a program change midi controller."},
    {Py_tp_traverse, Programin_traverse},
    {Py_tp_clear, Programin_clear},
    {Py_tp_methods, Programin_methods},
    {Py_tp_members, Programin_members},
    {Py_tp_new, Programin_new},
    {Py_nb_add, Programin_add},
    {Py_nb_subtract, Programin_sub},
    {Py_nb_multiply, Programin_multiply},
    {Py_nb_true_divide, Programin_div},
    {Py_nb_inplace_add, Programin_inplace_add},
    {Py_nb_inplace_subtract, Programin_inplace_sub},
    {Py_nb_inplace_multiply, Programin_inplace_multiply},
    {Py_nb_inplace_true_divide, Programin_inplace_div},
    {0, NULL}
};

static PyType_Spec PrograminType_spec =
{
    "_pyo.Programin_base",
    sizeof(Programin),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PrograminType_slots
};

PyTypeObject *
PyoCreatePrograminType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &PrograminType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    int *notebuf; /* pitch, velocity, posToWrite */
    int voices;
    int vcount;
    int scale; /* 0 = midi, 1 = hertz, 2 = transpo */
    int first;
    int last;
    int centralkey;
    int channel;
    int stealing;
    int holdmode;
    int firstvelocity;
    int lastvelocity;
    PyoMidiEvent midiEvents[64];
    int eventcount;
    MYFLT *trigger_streams;
} MidiNote;

static void
MidiNote_setProcMode(MidiNote *self) {};

int
pitchIsIn(int *buf, int pitch, int len)
{
    int i;
    int isIn = 0;

    for (i = 0; i < len; i++)
    {
        if (buf[i * 3] == pitch)
        {
            isIn = 1;
            break;
        }
    }

    return isIn;
}

/* no more used */
int firstEmpty(int *buf, int len)
{
    int i;
    int voice = -1;

    for (i = 0; i < len; i++)
    {
        if (buf[i * 3 + 1] == 0)
        {
            voice = i;
            break;
        }
    }

    return voice;
}

int nextEmptyVoice(int *buf, int voice, int len)
{
    int i, tmp;
    int next = -1;

    for (i = 1; i <= len; i++)
    {
        tmp = (i + voice) % len;

        if (buf[tmp * 3 + 1] == 0)
        {
            next = tmp;
            break;
        }
    }

    return next;
}

int whichVoice(int *buf, int pitch, int len)
{
    int i;
    int voice = 0;

    for (i = 0; i < len; i++)
    {
        if (buf[i * 3] == pitch)
        {
            voice = i;
            break;
        }
    }

    return voice;
}



// Sends a noteoff MIDI event for all pitches in MidiEventBuffer
// but not for current_pitch if current_pitch > -1
void allNotesOff(MidiNote *self, int current_pitch)
{
    int i, pitch, samp;
    PyoMidiEvent *buf = Server_getMidiEventBuffer((Server *)self->server);

    for (i = 0; i < self->voices; i++)
    {
        pitch = self->notebuf[i * 3];
        if (pitch != -1 && (pitch != current_pitch || current_pitch == -1))
        {
            samp = getPosToWrite(buf[i].timestamp, (Server *)self->server, self->sr, self->bufsize);
            self->notebuf[i * 3] = -1;
            self->notebuf[i * 3 + 1] = 0;
            self->notebuf[i * 3 + 2] = samp;
            self->trigger_streams[self->bufsize * (i * 2 + 1) + samp] = 1.0;
        }
    }

}

// Take MIDI events and keep track of notes
void grabMidiNotes(MidiNote *self, PyoMidiEvent *buffer, int count)
{
    int i, ok, voice, kind, samp, pitchIsInBuffer = 0;

    for (i = 0; i < count; i++)
    {
        int status = PyoMidi_MessageStatus(buffer[i].message);    // Temp note event holders
        int pitch = PyoMidi_MessageData1(buffer[i].message);
        int velocity = PyoMidi_MessageData2(buffer[i].message);

        if (self->channel == 0)
        {
            if (((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80)
                && pitch >= self->first && pitch <= self->last
                && (velocity == 0 || (velocity >= self->firstvelocity && velocity <= self->lastvelocity)))
                ok = 1;
            else
                ok = 0;
        }
        else
        {
            if ((status == (0x90 | (self->channel - 1)) || status == (0x80 | (self->channel - 1)))
                && pitch >= self->first && pitch <= self->last
                && (velocity == 0 || (velocity >= self->firstvelocity && velocity <= self->lastvelocity)))
                ok = 1;
            else
                ok = 0;
        }

        if (ok == 1)
        {
            samp = getPosToWrite(buffer[i].timestamp, (Server *)self->server, self->sr, self->bufsize);

            if ((status & 0xF0) == 0x80)
                kind = 0;
            else if ((status & 0xF0) == 0x90 && velocity == 0)
                kind = 0;
            else
            {
                kind = 1;
                if (self->holdmode > 2)
                    allNotesOff((MidiNote *)self, pitch);
            }

            pitchIsInBuffer = pitchIsIn(self->notebuf, pitch, self->voices);

            if (pitchIsInBuffer == 0 && kind == 1)
            {

                if (self->holdmode == 2 || self->holdmode == 4)
                    velocity = 127;

                //PySys_WriteStdout("%i, %i, %i\n", status, pitch, velocity);
                if (!self->stealing)
                {
                    voice = nextEmptyVoice(self->notebuf, self->vcount, self->voices);

                    if (voice != -1)
                    {
                        self->vcount = voice;
                        self->notebuf[voice * 3] = pitch;
                        self->notebuf[voice * 3 + 1] = velocity;
                        self->notebuf[voice * 3 + 2] = samp;
                        self->trigger_streams[self->bufsize * (self->vcount * 2) + samp] = 1.0;
                    }
                }
                else
                {
                    self->vcount = (self->vcount + 1) % self->voices;
                    self->notebuf[self->vcount * 3] = pitch;
                    self->notebuf[self->vcount * 3 + 1] = velocity;
                    self->notebuf[self->vcount * 3 + 2] = samp;
                    self->trigger_streams[self->bufsize * (self->vcount * 2) + samp] = 1.0;
                }
            }
            else if (pitchIsInBuffer == 1 && ((kind == 0 && self->holdmode == 0) || (kind == 1 && self->holdmode > 0)))
            {
                //PySys_WriteStdout("%i, %i, %i\n", status, pitch, velocity);
                voice = whichVoice(self->notebuf, pitch, self->voices);
                self->notebuf[voice * 3] = -1;
                self->notebuf[voice * 3 + 1] = 0;
                self->notebuf[voice * 3 + 2] = samp;
                self->trigger_streams[self->bufsize * (voice * 2 + 1) + samp] = 1.0;
            }
        }
    }
}

static void
MidiNote_compute_next_data_frame(MidiNote *self)
{
    PyoMidiEvent *tmp;
    int i, count;

    for (i = 0; i < self->bufsize * self->voices * 2; i++)
    {
        self->trigger_streams[i] = 0.0;
    }

    if (self->eventcount > 0)
    {
        grabMidiNotes((MidiNote *)self, self->midiEvents, self->eventcount);
    }

    self->eventcount = 0;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
        grabMidiNotes((MidiNote *)self, tmp, count);
}

static int
MidiNote_traverse(MidiNote *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
MidiNote_clear(MidiNote *self)
{
    pyo_CLEAR
    return 0;
}

static void
MidiNote_dealloc(MidiNote* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->notebuf);
    PyMem_RawFree(self->trigger_streams);
    MidiNote_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static MYFLT *
MidiNote_get_trigger_buffer(MidiNote *self)
{
    return self->trigger_streams;
}

static PyObject *
MidiNote_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;

    MidiNote *self;
    self = (MidiNote *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->voices = 10;
    self->vcount = 0;
    self->scale = 0;
    self->first = 0;
    self->last = 127;
    self->channel = 0;
    self->stealing = 0;
    self->holdmode = 0;
    self->firstvelocity = 1;
    self->lastvelocity = 127;
    self->eventcount = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(MidiNote_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(MidiNote_setProcMode);

    static char *kwlist[] = {"voices", "scale", "first", "last", "channel", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iiiii", kwlist, &self->voices, &self->scale, &self->first, &self->last, &self->channel)) {
        Py_DECREF(self);
        return NULL;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->notebuf = (int *)PyMem_RawRealloc(self->notebuf, self->voices * 3 * sizeof(int));
    self->trigger_streams = (MYFLT *)PyMem_RawRealloc(self->trigger_streams, self->bufsize * self->voices * 2 * sizeof(MYFLT));

    for (i = 0; i < self->bufsize * self->voices * 2; i++)
    {
        self->trigger_streams[i] = 0.0;
    }

    for (i = 0; i < self->voices; i++)
    {
        self->notebuf[i * 3] = -1;
        self->notebuf[i * 3 + 1] = 0;
        self->notebuf[i * 3 + 2] = 0;
    }

    self->centralkey = (self->first + self->last) / 2;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

MYFLT
MidiNote_getValue(MidiNote *self, int voice, int which, int *posto)
{
    MYFLT val = -1.0;
    int midival = self->notebuf[voice * 3 + which];

    if (which == 0 && midival != -1)
    {
        if (self->scale == 0)
            val = midival;
        else if (self->scale == 1)
            val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
        else if (self->scale == 2)
            val = MYPOW(1.0594630943593, midival - self->centralkey);
    }
    else if (which == 0)
        val = (MYFLT)midival;
    else if (which == 1)
        val = (MYFLT)midival / 127.;

    *posto = self->notebuf[voice * 3 + 2];

    return val;
}

static PyObject *
MidiNote_sendAllNotesOff(MidiNote *self)
{
    allNotesOff((MidiNote *)self, -1);
    Py_RETURN_NONE;
}

static PyObject *
MidiNote_addMidiEvent(MidiNote *self, PyObject *args)
{
    int status, data1, data2;
    PyoMidiEvent buffer;

    if (! PyArg_ParseTuple(args, "ii", &data1, &data2))
        return PyLong_FromLong(-1);

    if (self->channel == 0)
    {
        status = 0x90;
    }
    else
    {
        status = 0x90 | (self->channel - 1);
    }

    buffer.timestamp = 0;
    buffer.message = PyoMidi_Message(status, data1, data2);
    self->midiEvents[self->eventcount++] = buffer;

    if (self->eventcount == 64)
    {
        self->eventcount = 0;
    }

    Py_RETURN_NONE;
}

static PyObject * MidiNote_getServer(MidiNote* self) { GET_SERVER };
static PyObject * MidiNote_getStream(MidiNote* self) { GET_STREAM };

static PyObject * MidiNote_play(MidiNote *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiNote_stop(MidiNote *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
MidiNote_setScale(MidiNote *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        int tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 3)
            self->scale = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setFirst(MidiNote *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        int tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->first = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setLast(MidiNote *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        int tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->last = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setChannel(MidiNote *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        int tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 128)
            self->channel = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setCentralKey(MidiNote *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= self->first && tmp <= self->last)
            self->centralkey = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setStealing(MidiNote *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
        self->stealing = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setHoldmode(MidiNote *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 5)
            self->holdmode = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setFirstVelocity(MidiNote *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp > 0 && tmp < 128)
        {
            self->firstvelocity = tmp;
            if (self->firstvelocity > self->lastvelocity)
            {
                tmp = self->lastvelocity;
                self->lastvelocity = self->firstvelocity;
                self->firstvelocity = tmp;
            }
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiNote_setLastVelocity(MidiNote *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp > 0 && tmp < 128)
        {
            self->lastvelocity = tmp;
            if (self->firstvelocity > self->lastvelocity)
            {
                tmp = self->lastvelocity;
                self->lastvelocity = self->firstvelocity;
                self->firstvelocity = tmp;
            }
        }
    }

    Py_RETURN_NONE;
}

static PyMemberDef MidiNote_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MidiNote, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MidiNote, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiNote_methods[] =
{
    {"getServer", (PyCFunction)MidiNote_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MidiNote_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MidiNote_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiNote_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setScale", (PyCFunction)MidiNote_setScale, METH_O, "Sets the scale factor."},
    {"setFirst", (PyCFunction)MidiNote_setFirst, METH_O, "Sets the lowest midi note."},
    {"setLast", (PyCFunction)MidiNote_setLast, METH_O, "Sets the highest midi note."},
    {"setChannel", (PyCFunction)MidiNote_setChannel, METH_O, "Sets the midi channel."},
    {"setCentralKey", (PyCFunction)MidiNote_setCentralKey, METH_O, "Sets the midi key where there is no transposition."},
    {"setStealing", (PyCFunction)MidiNote_setStealing, METH_O, "Sets the stealing mode."},
    {"setHoldmode", (PyCFunction)MidiNote_setHoldmode, METH_O, "Sets the key hold mode."},
    {"setFirstVelocity", (PyCFunction)MidiNote_setFirstVelocity, METH_O, "Sets the lowest midi velocity."},
    {"setLastVelocity", (PyCFunction)MidiNote_setLastVelocity, METH_O, "Sets the highest midi velocity."},
    {"sendAllNotesOff", (PyCFunction)MidiNote_sendAllNotesOff, METH_NOARGS, "Sends all notes off event."},
    {"addMidiEvent", (PyCFunction)MidiNote_addMidiEvent, METH_VARARGS | METH_KEYWORDS, "Add a new midi event in the internal queue."},
    {NULL}  /* Sentinel */
};

static PyType_Slot MidiNoteType_slots[] = {
    {Py_tp_dealloc, MidiNote_dealloc},
    {Py_tp_doc, "MidiNote objects. Retreive midi note from a midi input."},
    {Py_tp_traverse, MidiNote_traverse},
    {Py_tp_clear, MidiNote_clear},
    {Py_tp_methods, MidiNote_methods},
    {Py_tp_members, MidiNote_members},
    {Py_tp_new, MidiNote_new},
    {0, NULL}
};

static PyType_Spec MidiNoteType_spec =
{
    "_pyo.MidiNote_base",
    sizeof(MidiNote),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    MidiNoteType_slots
};

PyTypeObject *
PyoCreateMidiNoteType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &MidiNoteType_spec, NULL);
}


/* Notein streamer */
typedef struct
{
    pyo_audio_HEAD
    MidiNote *handler;
    int modebuffer[2];
    int voice;
    int mode; /* 0 = pitch, 1 = velocity */
    MYFLT lastval;
    MYFLT lastpitch;
} Notein;

static void Notein_postprocessing_ii(Notein *self) { POST_PROCESSING_II };
static void Notein_postprocessing_ai(Notein *self) { POST_PROCESSING_AI };
static void Notein_postprocessing_ia(Notein *self) { POST_PROCESSING_IA };
static void Notein_postprocessing_aa(Notein *self) { POST_PROCESSING_AA };
static void Notein_postprocessing_ireva(Notein *self) { POST_PROCESSING_IREVA };
static void Notein_postprocessing_areva(Notein *self) { POST_PROCESSING_AREVA };
static void Notein_postprocessing_revai(Notein *self) { POST_PROCESSING_REVAI };
static void Notein_postprocessing_revaa(Notein *self) { POST_PROCESSING_REVAA };
static void Notein_postprocessing_revareva(Notein *self) { POST_PROCESSING_REVAREVA };

static void
Notein_setProcMode(Notein *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Notein_postprocessing_revareva);
            break;
    }
}

static void
Notein_compute_next_data_frame(Notein *self)
{
    int i, posto;
    MYFLT tmp = MidiNote_getValue(self->handler, self->voice, self->mode, &posto);

    if (self->lastval == tmp)
    {
        if (self->mode == 0 && tmp != -1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = tmp;
            }
        }
        else if (self->mode == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = tmp;
            }

            (*self->muladd_func_ptr)(self);
        }
    }
    else   /* There is a new note to compute. */
    {
        if (self->mode == 0 && tmp != -1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                if (i < posto)
                    self->data[i] = self->lastpitch;
                else
                    self->data[i] = tmp;
            }
        }
        else if (self->mode == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                if (i < posto)
                    self->data[i] = self->lastval;
                else
                    self->data[i] = tmp;
            }

            (*self->muladd_func_ptr)(self);
        }

        self->lastval = tmp;

        if (tmp != -1)
            self->lastpitch = tmp;
    }
}

static int
Notein_traverse(Notein *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->handler);
    return 0;
}

static int
Notein_clear(Notein *self)
{
    pyo_CLEAR
    Py_CLEAR(self->handler);
    return 0;
}

static void
Notein_dealloc(Notein* self)
{
    pyo_DEALLOC
    Notein_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Notein_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *handlertmp = NULL, *multmp = NULL, *addtmp = NULL;
    Notein *self;
    self = (Notein *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->voice = 0;
    self->mode = 0;
    self->lastval = -1.0;
    self->lastpitch = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Notein_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Notein_setProcMode);

    static char *kwlist[] = {"handler", "voice", "mode", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiOO", kwlist, &handlertmp, &self->voice, &self->mode, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    Py_XDECREF(self->handler);
    Py_INCREF(handlertmp);
    self->handler = (MidiNote *)handlertmp;

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Notein_getServer(Notein* self) { GET_SERVER };
static PyObject * Notein_getStream(Notein* self) { GET_STREAM };
static PyObject * Notein_setMul(Notein *self, PyObject *arg) { SET_MUL };
static PyObject * Notein_setAdd(Notein *self, PyObject *arg) { SET_ADD };
static PyObject * Notein_setSub(Notein *self, PyObject *arg) { SET_SUB };
static PyObject * Notein_setDiv(Notein *self, PyObject *arg) { SET_DIV };

static PyObject * Notein_play(Notein *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Notein_stop(Notein *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Notein_multiply(Notein *self, PyObject *arg) { MULTIPLY };
static PyObject * Notein_inplace_multiply(Notein *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Notein_add(Notein *self, PyObject *arg) { ADD };
static PyObject * Notein_inplace_add(Notein *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Notein_sub(Notein *self, PyObject *arg) { SUB };
static PyObject * Notein_inplace_sub(Notein *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Notein_div(Notein *self, PyObject *arg) { DIV };
static PyObject * Notein_inplace_div(Notein *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Notein_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Notein, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Notein, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Notein, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Notein, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Notein_methods[] =
{
    {"getServer", (PyCFunction)Notein_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Notein_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Notein_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Notein_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Notein_setMul, METH_O, "Sets Notein mul factor."},
    {"setAdd", (PyCFunction)Notein_setAdd, METH_O, "Sets Notein add factor."},
    {"setSub", (PyCFunction)Notein_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Notein_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot NoteinType_slots[] = {
    {Py_tp_dealloc, Notein_dealloc},
    {Py_tp_doc, "Notein objects. Stream pitch or velocity from a Notein voice."},
    {Py_tp_traverse, Notein_traverse},
    {Py_tp_clear, Notein_clear},
    {Py_tp_methods, Notein_methods},
    {Py_tp_members, Notein_members},
    {Py_tp_new, Notein_new},
    {Py_nb_add, Notein_add},
    {Py_nb_subtract, Notein_sub},
    {Py_nb_multiply, Notein_multiply},
    {Py_nb_true_divide, Notein_div},
    {Py_nb_inplace_add, Notein_inplace_add},
    {Py_nb_inplace_subtract, Notein_inplace_sub},
    {Py_nb_inplace_multiply, Notein_inplace_multiply},
    {Py_nb_inplace_true_divide, Notein_inplace_div},
    {0, NULL}
};

static PyType_Spec NoteinType_spec =
{
    "_pyo.Notein_base",
    sizeof(Notein),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    NoteinType_slots
};

PyTypeObject *
PyoCreateNoteinType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &NoteinType_spec, NULL);
}

/* NoteinTrig trig streamer */
typedef struct
{
    pyo_audio_HEAD
    MidiNote *handler;
    int modebuffer[2];
    int voice;
    int mode; /* 0 = noteon, 1 = noteoff */
} NoteinTrig;

static void NoteinTrig_postprocessing_ii(NoteinTrig *self) { POST_PROCESSING_II };
static void NoteinTrig_postprocessing_ai(NoteinTrig *self) { POST_PROCESSING_AI };
static void NoteinTrig_postprocessing_ia(NoteinTrig *self) { POST_PROCESSING_IA };
static void NoteinTrig_postprocessing_aa(NoteinTrig *self) { POST_PROCESSING_AA };
static void NoteinTrig_postprocessing_ireva(NoteinTrig *self) { POST_PROCESSING_IREVA };
static void NoteinTrig_postprocessing_areva(NoteinTrig *self) { POST_PROCESSING_AREVA };
static void NoteinTrig_postprocessing_revai(NoteinTrig *self) { POST_PROCESSING_REVAI };
static void NoteinTrig_postprocessing_revaa(NoteinTrig *self) { POST_PROCESSING_REVAA };
static void NoteinTrig_postprocessing_revareva(NoteinTrig *self) { POST_PROCESSING_REVAREVA };

static void
NoteinTrig_setProcMode(NoteinTrig *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_postprocessing_revareva);
            break;
    }
}

static void
NoteinTrig_compute_next_data_frame(NoteinTrig *self)
{
    int i;
    MYFLT *tmp = MidiNote_get_trigger_buffer(self->handler);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = tmp[self->bufsize * (self->voice * 2 + self->mode) + i];
    }

    (*self->muladd_func_ptr)(self);
}

static int
NoteinTrig_traverse(NoteinTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->handler);
    return 0;
}

static int
NoteinTrig_clear(NoteinTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->handler);
    return 0;
}

static void
NoteinTrig_dealloc(NoteinTrig* self)
{
    pyo_DEALLOC
    NoteinTrig_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NoteinTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *handlertmp = NULL, *multmp = NULL, *addtmp = NULL;
    NoteinTrig *self;
    self = (NoteinTrig *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->voice = 0;
    self->mode = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(NoteinTrig_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(NoteinTrig_setProcMode);

    static char *kwlist[] = {"handler", "voice", "mode", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiOO", kwlist, &handlertmp, &self->voice, &self->mode, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    Py_XDECREF(self->handler);
    Py_INCREF(handlertmp);
    self->handler = (MidiNote *)handlertmp;

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NoteinTrig_getServer(NoteinTrig* self) { GET_SERVER };
static PyObject * NoteinTrig_getStream(NoteinTrig* self) { GET_STREAM };
static PyObject * NoteinTrig_setMul(NoteinTrig *self, PyObject *arg) { SET_MUL };
static PyObject * NoteinTrig_setAdd(NoteinTrig *self, PyObject *arg) { SET_ADD };
static PyObject * NoteinTrig_setSub(NoteinTrig *self, PyObject *arg) { SET_SUB };
static PyObject * NoteinTrig_setDiv(NoteinTrig *self, PyObject *arg) { SET_DIV };

static PyObject * NoteinTrig_play(NoteinTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * NoteinTrig_stop(NoteinTrig *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * NoteinTrig_multiply(NoteinTrig *self, PyObject *arg) { MULTIPLY };
static PyObject * NoteinTrig_inplace_multiply(NoteinTrig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NoteinTrig_add(NoteinTrig *self, PyObject *arg) { ADD };
static PyObject * NoteinTrig_inplace_add(NoteinTrig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NoteinTrig_sub(NoteinTrig *self, PyObject *arg) { SUB };
static PyObject * NoteinTrig_inplace_sub(NoteinTrig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NoteinTrig_div(NoteinTrig *self, PyObject *arg) { DIV };
static PyObject * NoteinTrig_inplace_div(NoteinTrig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef NoteinTrig_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NoteinTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinTrig, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(NoteinTrig, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(NoteinTrig, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinTrig_methods[] =
{
    {"getServer", (PyCFunction)NoteinTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)NoteinTrig_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinTrig_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)NoteinTrig_setMul, METH_O, "Sets NoteinTrig mul factor."},
    {"setAdd", (PyCFunction)NoteinTrig_setAdd, METH_O, "Sets NoteinTrig add factor."},
    {"setSub", (PyCFunction)NoteinTrig_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)NoteinTrig_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot NoteinTrigType_slots[] = {
    {Py_tp_dealloc, NoteinTrig_dealloc},
    {Py_tp_doc, "NoteinTrig objects. Stream noteon or noteoff trigger from a Notein voice."},
    {Py_tp_traverse, NoteinTrig_traverse},
    {Py_tp_clear, NoteinTrig_clear},
    {Py_tp_methods, NoteinTrig_methods},
    {Py_tp_members, NoteinTrig_members},
    {Py_tp_new, NoteinTrig_new},
    {Py_nb_add, NoteinTrig_add},
    {Py_nb_subtract, NoteinTrig_sub},
    {Py_nb_multiply, NoteinTrig_multiply},
    {Py_nb_true_divide, NoteinTrig_div},
    {Py_nb_inplace_add, NoteinTrig_inplace_add},
    {Py_nb_inplace_subtract, NoteinTrig_inplace_sub},
    {Py_nb_inplace_multiply, NoteinTrig_inplace_multiply},
    {Py_nb_inplace_true_divide, NoteinTrig_inplace_div},
    {0, NULL}
};

static PyType_Spec NoteinTrigType_spec =
{
    "_pyo.NoteinTrig_base",
    sizeof(NoteinTrig),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    NoteinTrigType_slots
};

PyTypeObject *
PyoCreateNoteinTrigType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &NoteinTrigType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    int fademode;
    int changed;
    MYFLT topValue;
    MYFLT offsetAmp;
    MYFLT initAmp;
    MYFLT sustainAmp;
    MYFLT attack;
    MYFLT decay;
    MYFLT sustain;
    MYFLT release;
    MYFLT exp;
    MYFLT expscl;
    MYFLT invAttack;
    MYFLT initAmpMinusOffsetAmp;
    MYFLT attackPlusDecay;
    MYFLT invDecay;
    MYFLT initAmpMinusSustainAmp;
    MYFLT invRelease;
    double currentTime;
    MYFLT sampleToSec;
    MYFLT *buf;
} MidiAdsr;

static void
MidiAdsr_generates(MidiAdsr *self)
{
    MYFLT val;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->fademode == 0 && in[i] > 0.0)
        {
            self->fademode = 1;
            self->initAmp = in[i];
            self->expscl = MYPOW(self->initAmp, 1.0 / self->exp) / self->initAmp;
            self->offsetAmp = self->buf[i];
            self->sustainAmp = self->initAmp * self->sustain;
            self->currentTime = 0.0;
            self->invAttack = 1.0 / self->attack;
            self->invDecay = 1.0 / self->decay;
            self->attackPlusDecay = self->attack + self->decay;
            self->initAmpMinusOffsetAmp = self->initAmp - self->offsetAmp;
            self->initAmpMinusSustainAmp = self->initAmp - self->sustainAmp;
        }
        else if (self->fademode == 1 && in[i] == 0.0)
        {
            self->fademode = 0;
            self->currentTime = 0.0;
            self->invRelease = 1.0 / self->release;
        }

        if (self->fademode == 1)
        {
            if (self->currentTime <= self->attack)
                val = self->currentTime * self->invAttack * self->initAmpMinusOffsetAmp + self->offsetAmp;
            else if (self->currentTime <= self->attackPlusDecay)
                val = (self->decay - (self->currentTime - self->attack)) * self->invDecay * self->initAmpMinusSustainAmp + self->sustainAmp;
            else
                val = self->sustainAmp;

            self->topValue = val;
        }
        else
        {
            if (self->currentTime <= self->release)
                val = self->topValue * (1. - self->currentTime * self->invRelease);
            else
                val = 0.;
        }

        self->buf[i] = val;
        self->currentTime += self->sampleToSec;
    }

    if (self->exp != 1.0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = MYPOW(self->buf[i] * self->expscl, self->exp);
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->buf[i];
        }
    }
}

static void MidiAdsr_postprocessing_ii(MidiAdsr *self) { POST_PROCESSING_II };
static void MidiAdsr_postprocessing_ai(MidiAdsr *self) { POST_PROCESSING_AI };
static void MidiAdsr_postprocessing_ia(MidiAdsr *self) { POST_PROCESSING_IA };
static void MidiAdsr_postprocessing_aa(MidiAdsr *self) { POST_PROCESSING_AA };
static void MidiAdsr_postprocessing_ireva(MidiAdsr *self) { POST_PROCESSING_IREVA };
static void MidiAdsr_postprocessing_areva(MidiAdsr *self) { POST_PROCESSING_AREVA };
static void MidiAdsr_postprocessing_revai(MidiAdsr *self) { POST_PROCESSING_REVAI };
static void MidiAdsr_postprocessing_revaa(MidiAdsr *self) { POST_PROCESSING_REVAA };
static void MidiAdsr_postprocessing_revareva(MidiAdsr *self) { POST_PROCESSING_REVAREVA };

static void
MidiAdsr_setProcMode(MidiAdsr *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_postprocessing_revareva);
            break;
    }
}

static void
MidiAdsr_compute_next_data_frame(MidiAdsr *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
MidiAdsr_traverse(MidiAdsr *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
MidiAdsr_clear(MidiAdsr *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
MidiAdsr_dealloc(MidiAdsr* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->buf);
    MidiAdsr_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiAdsr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    MidiAdsr *self;
    self = (MidiAdsr *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->topValue = 0.0;
    self->fademode = 0;
    self->changed = 0;
    self->attack = 0.01;
    self->decay = 0.05;
    self->sustain = 0.707;
    self->release = 0.1;
    self->currentTime = 0.0;
    self->exp = self->expscl = 1.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(MidiAdsr_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(MidiAdsr_setProcMode);

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"input", "attack", "decay", "sustain", "release", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFOO, kwlist, &inputtmp, &self->attack, &self->decay, &self->sustain, &self->release, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->buf = (MYFLT *)PyMem_RawRealloc(self->buf, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->buf[i] = 0.0;
    }

    if (self->attack < 0.000001)
        self->attack = 0.000001;

    if (self->decay < 0.000001)
        self->decay = 0.000001;

    if (self->release < 0.000001)
        self->release = 0.000001;

    if (self->sustain < 0.0)
        self->sustain = 0.0;
    else if (self->sustain > 1.0)
        self->sustain = 1.0;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MidiAdsr_getServer(MidiAdsr* self) { GET_SERVER };
static PyObject * MidiAdsr_getStream(MidiAdsr* self) { GET_STREAM };
static PyObject * MidiAdsr_setMul(MidiAdsr *self, PyObject *arg) { SET_MUL };
static PyObject * MidiAdsr_setAdd(MidiAdsr *self, PyObject *arg) { SET_ADD };
static PyObject * MidiAdsr_setSub(MidiAdsr *self, PyObject *arg) { SET_SUB };
static PyObject * MidiAdsr_setDiv(MidiAdsr *self, PyObject *arg) { SET_DIV };

static PyObject * MidiAdsr_play(MidiAdsr *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiAdsr_stop(MidiAdsr *self, PyObject *args, PyObject *kwds) { STOP }

static PyObject * MidiAdsr_multiply(MidiAdsr *self, PyObject *arg) { MULTIPLY };
static PyObject * MidiAdsr_inplace_multiply(MidiAdsr *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MidiAdsr_add(MidiAdsr *self, PyObject *arg) { ADD };
static PyObject * MidiAdsr_inplace_add(MidiAdsr *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MidiAdsr_sub(MidiAdsr *self, PyObject *arg) { SUB };
static PyObject * MidiAdsr_inplace_sub(MidiAdsr *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MidiAdsr_div(MidiAdsr *self, PyObject *arg) { DIV };
static PyObject * MidiAdsr_inplace_div(MidiAdsr *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
MidiAdsr_setAttack(MidiAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->attack = PyFloat_AsDouble(arg);

        if (self->attack < 0.000001)
            self->attack = 0.000001;

        self->invAttack = 1.0 / self->attack;
        self->attackPlusDecay = self->attack + self->decay;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiAdsr_setDecay(MidiAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->decay = PyFloat_AsDouble(arg);

        if (self->decay < 0.000001)
            self->decay = 0.000001;

        self->invDecay = 1.0 / self->decay;
        self->attackPlusDecay = self->attack + self->decay;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiAdsr_setSustain(MidiAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->sustain = PyFloat_AsDouble(arg);

        if (self->sustain < 0.0)
            self->sustain = 0.0;
        else if (self->sustain > 1.0)
            self->sustain = 1.0;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiAdsr_setRelease(MidiAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->release = PyFloat_AsDouble(arg);

        if (self->release < 0.000001)
            self->release = 0.000001;

        self->invRelease = 1.0 / self->release;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiAdsr_setExp(MidiAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        MYFLT tmp = PyFloat_AsDouble(arg);

        if (tmp > 0.0)
            self->exp = tmp;
    }

    Py_RETURN_NONE;
}

static PyMemberDef MidiAdsr_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MidiAdsr, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MidiAdsr, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MidiAdsr, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(MidiAdsr, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MidiAdsr, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiAdsr_methods[] =
{
    {"getServer", (PyCFunction)MidiAdsr_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MidiAdsr_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MidiAdsr_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiAdsr_stop, METH_VARARGS | METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"setMul", (PyCFunction)MidiAdsr_setMul, METH_O, "Sets MidiAdsr mul factor."},
    {"setAdd", (PyCFunction)MidiAdsr_setAdd, METH_O, "Sets MidiAdsr add factor."},
    {"setSub", (PyCFunction)MidiAdsr_setSub, METH_O, "Sets inverse add factor."},
    {"setAttack", (PyCFunction)MidiAdsr_setAttack, METH_O, "Sets attack time in seconds."},
    {"setDecay", (PyCFunction)MidiAdsr_setDecay, METH_O, "Sets decay time in seconds."},
    {"setSustain", (PyCFunction)MidiAdsr_setSustain, METH_O, "Sets sustain level in percent of note amplitude."},
    {"setRelease", (PyCFunction)MidiAdsr_setRelease, METH_O, "Sets release time in seconds."},
    {"setExp", (PyCFunction)MidiAdsr_setExp, METH_O, "Sets the exponent factor for exponential envelope."},
    {"setDiv", (PyCFunction)MidiAdsr_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot MidiAdsrType_slots[] = {
    {Py_tp_dealloc, MidiAdsr_dealloc},
    {Py_tp_doc, "MidiAdsr objects. Generates MidiAdsr envelope signal."},
    {Py_tp_traverse, MidiAdsr_traverse},
    {Py_tp_clear, MidiAdsr_clear},
    {Py_tp_methods, MidiAdsr_methods},
    {Py_tp_members, MidiAdsr_members},
    {Py_tp_new, MidiAdsr_new},
    {Py_nb_add, MidiAdsr_add},
    {Py_nb_subtract, MidiAdsr_sub},
    {Py_nb_multiply, MidiAdsr_multiply},
    {Py_nb_true_divide, MidiAdsr_div},
    {Py_nb_inplace_add, MidiAdsr_inplace_add},
    {Py_nb_inplace_subtract, MidiAdsr_inplace_sub},
    {Py_nb_inplace_multiply, MidiAdsr_inplace_multiply},
    {Py_nb_inplace_true_divide, MidiAdsr_inplace_div},
    {0, NULL}
};

static PyType_Spec MidiAdsrType_spec =
{
    "_pyo.MidiAdsr_base",
    sizeof(MidiAdsr),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    MidiAdsrType_slots
};

PyTypeObject *
PyoCreateMidiAdsrType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &MidiAdsrType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    int fademode;
    int changed;
    MYFLT topValue;
    MYFLT offsetAmp;
    MYFLT initAmp;
    MYFLT sustainAmp;
    MYFLT delay;
    MYFLT attack;
    MYFLT decay;
    MYFLT sustain;
    MYFLT release;
    MYFLT exp;
    MYFLT expscl;
    MYFLT invAttack;
    MYFLT initAmpMinusOffsetAmp;
    MYFLT invDecay;
    MYFLT delayPlusAttack;
    MYFLT delayPlusAttackPlusDecay;
    MYFLT initAmpMinusSustainAmp;
    MYFLT invRelease;
    double currentTime;
    MYFLT sampleToSec;
    MYFLT *buf;
} MidiDelAdsr;

static void
MidiDelAdsr_generates(MidiDelAdsr *self)
{
    MYFLT val;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->fademode == 0 && in[i] > 0.0)
        {
            self->fademode = 1;
            self->initAmp = in[i];
            self->expscl = MYPOW(self->initAmp, 1.0 / self->exp) / self->initAmp;
            self->offsetAmp = self->buf[i];
            self->sustainAmp = self->initAmp * self->sustain;
            self->currentTime = 0.0;
            self->invAttack = 1.0 / self->attack;
            self->invDecay = 1.0 / self->decay;
            self->delayPlusAttack = self->delay + self->attack;
            self->delayPlusAttackPlusDecay = self->delay + self->attack + self->decay;
            self->initAmpMinusOffsetAmp = self->initAmp - self->offsetAmp;
            self->initAmpMinusSustainAmp = self->initAmp - self->sustainAmp;
        }
        else if (self->fademode == 1 && in[i] == 0.0)
        {
            self->fademode = 0;
            self->currentTime = 0.0;
            self->invRelease = 1.0 / self->release;
        }

        if (self->fademode == 1)
        {
            if (self->currentTime < self->delay)
                val = 0.0;
            else if (self->currentTime <= self->delayPlusAttack)
                val = (self->currentTime - self->delay) * self->invAttack * self->initAmpMinusOffsetAmp + self->offsetAmp;
            else if (self->currentTime <= self->delayPlusAttackPlusDecay)
                val = (self->decay - (self->currentTime - self->delay - self->attack)) * self->invDecay * self->initAmpMinusSustainAmp + self->sustainAmp;
            else
                val = self->sustainAmp;

            self->topValue = val;
        }
        else
        {
            if (self->currentTime <= self->release)
                val = self->topValue * (1. - self->currentTime * self->invRelease);
            else
                val = 0.;
        }

        self->buf[i] = val;
        self->currentTime += self->sampleToSec;
    }

    if (self->exp != 1.0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = MYPOW(self->buf[i] * self->expscl, self->exp);
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->buf[i];
        }
    }
}

static void MidiDelAdsr_postprocessing_ii(MidiDelAdsr *self) { POST_PROCESSING_II };
static void MidiDelAdsr_postprocessing_ai(MidiDelAdsr *self) { POST_PROCESSING_AI };
static void MidiDelAdsr_postprocessing_ia(MidiDelAdsr *self) { POST_PROCESSING_IA };
static void MidiDelAdsr_postprocessing_aa(MidiDelAdsr *self) { POST_PROCESSING_AA };
static void MidiDelAdsr_postprocessing_ireva(MidiDelAdsr *self) { POST_PROCESSING_IREVA };
static void MidiDelAdsr_postprocessing_areva(MidiDelAdsr *self) { POST_PROCESSING_AREVA };
static void MidiDelAdsr_postprocessing_revai(MidiDelAdsr *self) { POST_PROCESSING_REVAI };
static void MidiDelAdsr_postprocessing_revaa(MidiDelAdsr *self) { POST_PROCESSING_REVAA };
static void MidiDelAdsr_postprocessing_revareva(MidiDelAdsr *self) { POST_PROCESSING_REVAREVA };

static void
MidiDelAdsr_setProcMode(MidiDelAdsr *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_postprocessing_revareva);
            break;
    }
}

static void
MidiDelAdsr_compute_next_data_frame(MidiDelAdsr *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
MidiDelAdsr_traverse(MidiDelAdsr *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
MidiDelAdsr_clear(MidiDelAdsr *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
MidiDelAdsr_dealloc(MidiDelAdsr* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->buf);
    MidiDelAdsr_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiDelAdsr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    MidiDelAdsr *self;
    self = (MidiDelAdsr *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->topValue = 0.0;
    self->fademode = 0;
    self->changed = 0;
    self->delay = 0.;
    self->attack = 0.01;
    self->decay = 0.05;
    self->sustain = 0.707;
    self->release = 0.1;
    self->currentTime = 0.0;
    self->exp = self->expscl = 1.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(MidiDelAdsr_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(MidiDelAdsr_setProcMode);

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"input", "delay", "attack", "decay", "sustain", "release", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFFOO, kwlist, &inputtmp, &self->delay, &self->attack, &self->decay, &self->sustain, &self->release, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->buf = (MYFLT *)PyMem_RawRealloc(self->buf, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->buf[i] = 0.0;
    }

    if (self->attack < 0.000001)
        self->attack = 0.000001;

    if (self->decay < 0.000001)
        self->decay = 0.000001;

    if (self->release < 0.000001)
        self->release = 0.000001;

    if (self->sustain < 0.0)
        self->sustain = 0.0;
    else if (self->sustain > 1.0)
        self->sustain = 1.0;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MidiDelAdsr_getServer(MidiDelAdsr* self) { GET_SERVER };
static PyObject * MidiDelAdsr_getStream(MidiDelAdsr* self) { GET_STREAM };
static PyObject * MidiDelAdsr_setMul(MidiDelAdsr *self, PyObject *arg) { SET_MUL };
static PyObject * MidiDelAdsr_setAdd(MidiDelAdsr *self, PyObject *arg) { SET_ADD };
static PyObject * MidiDelAdsr_setSub(MidiDelAdsr *self, PyObject *arg) { SET_SUB };
static PyObject * MidiDelAdsr_setDiv(MidiDelAdsr *self, PyObject *arg) { SET_DIV };

static PyObject * MidiDelAdsr_play(MidiDelAdsr *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiDelAdsr_stop(MidiDelAdsr *self, PyObject *args, PyObject *kwds) { STOP }

static PyObject * MidiDelAdsr_multiply(MidiDelAdsr *self, PyObject *arg) { MULTIPLY };
static PyObject * MidiDelAdsr_inplace_multiply(MidiDelAdsr *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MidiDelAdsr_add(MidiDelAdsr *self, PyObject *arg) { ADD };
static PyObject * MidiDelAdsr_inplace_add(MidiDelAdsr *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MidiDelAdsr_sub(MidiDelAdsr *self, PyObject *arg) { SUB };
static PyObject * MidiDelAdsr_inplace_sub(MidiDelAdsr *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MidiDelAdsr_div(MidiDelAdsr *self, PyObject *arg) { DIV };
static PyObject * MidiDelAdsr_inplace_div(MidiDelAdsr *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
MidiDelAdsr_setDelay(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->delay = PyFloat_AsDouble(arg);
        self->delayPlusAttack = self->delay + self->attack;
        self->delayPlusAttackPlusDecay = self->delay + self->attack + self->decay;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiDelAdsr_setAttack(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->attack = PyFloat_AsDouble(arg);

        if (self->attack < 0.000001)
            self->attack = 0.000001;

        self->invAttack = 1.0 / self->attack;
        self->delayPlusAttack = self->delay + self->attack;
        self->delayPlusAttackPlusDecay = self->delay + self->attack + self->decay;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiDelAdsr_setDecay(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->decay = PyFloat_AsDouble(arg);

        if (self->decay < 0.000001)
            self->decay = 0.000001;

        self->invDecay = 1.0 / self->decay;
        self->delayPlusAttackPlusDecay = self->delay + self->attack + self->decay;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiDelAdsr_setSustain(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->sustain = PyFloat_AsDouble(arg);

        if (self->sustain < 0.0)
            self->sustain = 0.0;
        else if (self->sustain > 1.0)
            self->sustain = 1.0;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiDelAdsr_setRelease(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->release = PyFloat_AsDouble(arg);

        if (self->release < 0.000001)
            self->release = 0.000001;

        self->invRelease = 1.0 / self->release;
    }

    Py_RETURN_NONE;
}

static PyObject *
MidiDelAdsr_setExp(MidiDelAdsr *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        MYFLT tmp = PyFloat_AsDouble(arg);

        if (tmp > 0.0)
            self->exp = tmp;
    }

    Py_RETURN_NONE;
}

static PyMemberDef MidiDelAdsr_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MidiDelAdsr, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MidiDelAdsr, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MidiDelAdsr, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(MidiDelAdsr, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MidiDelAdsr, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiDelAdsr_methods[] =
{
    {"getServer", (PyCFunction)MidiDelAdsr_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MidiDelAdsr_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MidiDelAdsr_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiDelAdsr_stop, METH_VARARGS | METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"setMul", (PyCFunction)MidiDelAdsr_setMul, METH_O, "Sets MidiDelAdsr mul factor."},
    {"setAdd", (PyCFunction)MidiDelAdsr_setAdd, METH_O, "Sets MidiDelAdsr add factor."},
    {"setSub", (PyCFunction)MidiDelAdsr_setSub, METH_O, "Sets inverse add factor."},
    {"setDelay", (PyCFunction)MidiDelAdsr_setDelay, METH_O, "Sets delay time in seconds."},
    {"setAttack", (PyCFunction)MidiDelAdsr_setAttack, METH_O, "Sets attack time in seconds."},
    {"setDecay", (PyCFunction)MidiDelAdsr_setDecay, METH_O, "Sets decay time in seconds."},
    {"setSustain", (PyCFunction)MidiDelAdsr_setSustain, METH_O, "Sets sustain level in percent of note amplitude."},
    {"setRelease", (PyCFunction)MidiDelAdsr_setRelease, METH_O, "Sets release time in seconds."},
    {"setExp", (PyCFunction)MidiDelAdsr_setExp, METH_O, "Sets the exponent factor for exponential envelope."},
    {"setDiv", (PyCFunction)MidiDelAdsr_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot MidiDelAdsrType_slots[] = {
    {Py_tp_dealloc, MidiDelAdsr_dealloc},
    {Py_tp_doc, "MidiDelAdsr objects. Generates MidiDelAdsr envelope signal."},
    {Py_tp_traverse, MidiDelAdsr_traverse},
    {Py_tp_clear, MidiDelAdsr_clear},
    {Py_tp_methods, MidiDelAdsr_methods},
    {Py_tp_members, MidiDelAdsr_members},
    {Py_tp_new, MidiDelAdsr_new},
    {Py_nb_add, MidiDelAdsr_add},
    {Py_nb_subtract, MidiDelAdsr_sub},
    {Py_nb_multiply, MidiDelAdsr_multiply},
    {Py_nb_true_divide, MidiDelAdsr_div},
    {Py_nb_inplace_add, MidiDelAdsr_inplace_add},
    {Py_nb_inplace_subtract, MidiDelAdsr_inplace_sub},
    {Py_nb_inplace_multiply, MidiDelAdsr_inplace_multiply},
    {Py_nb_inplace_true_divide, MidiDelAdsr_inplace_div},
    {0, NULL}
};

static PyType_Spec MidiDelAdsrType_spec =
{
    "_pyo.MidiDelAdsr_base",
    sizeof(MidiDelAdsr),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    MidiDelAdsrType_slots
};

PyTypeObject *
PyoCreateMidiDelAdsrType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &MidiDelAdsrType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *callable;
} RawMidi;

static void
RawMidi_setProcMode(RawMidi *self) {}

static void
RawMidi_compute_next_data_frame(RawMidi *self)
{
    PyoMidiEvent *buffer;
    int i, count, status, data1, data2;

    buffer = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
    {
        PyObject *tup;

        for (i = 0; i < count; i++)
        {
            status = PyoMidi_MessageStatus(buffer[i].message);    // Temp note event holders
            data1 = PyoMidi_MessageData1(buffer[i].message);
            data2 = PyoMidi_MessageData2(buffer[i].message);
            PyObject *result;
            tup = PyTuple_New(3);
            PyTuple_SetItem(tup, 0, PyLong_FromLong(status));
            PyTuple_SetItem(tup, 1, PyLong_FromLong(data1));
            PyTuple_SetItem(tup, 2, PyLong_FromLong(data2));
            result = PyObject_Call((PyObject *)self->callable, tup, NULL);
            Py_DECREF(tup);
            if (result == NULL)
                PyErr_Print();
            else
                Py_DECREF(result);
        }
    }
}

static int
RawMidi_traverse(RawMidi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    return 0;
}

static int
RawMidi_clear(RawMidi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    return 0;
}

static void
RawMidi_dealloc(RawMidi* self)
{
    pyo_DEALLOC
    RawMidi_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
RawMidi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *calltmp = NULL;
    RawMidi *self;
    self = (RawMidi *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(RawMidi_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(RawMidi_setProcMode);

    static char *kwlist[] = {"callable", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &calltmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if (calltmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setFunction", calltmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    return (PyObject *)self;
}

static PyObject * RawMidi_getServer(RawMidi* self) { GET_SERVER };
static PyObject * RawMidi_getStream(RawMidi* self) { GET_STREAM };

static PyObject * RawMidi_play(RawMidi *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * RawMidi_stop(RawMidi *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
RawMidi_setFunction(RawMidi *self, PyObject *arg)
{
    if (! PyCallable_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->callable);
    self->callable = arg;
    Py_INCREF(self->callable);

    Py_RETURN_NONE;
}

static PyMemberDef RawMidi_members[] =
{
    {"server", T_OBJECT_EX, offsetof(RawMidi, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(RawMidi, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef RawMidi_methods[] =
{
    {"getServer", (PyCFunction)RawMidi_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)RawMidi_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)RawMidi_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)RawMidi_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setFunction", (PyCFunction)RawMidi_setFunction, METH_O, "Sets the function to be called."},
    {NULL}  /* Sentinel */
};

static PyType_Slot RawMidiType_slots[] = {
    {Py_tp_dealloc, RawMidi_dealloc},
    {Py_tp_doc, "RawMidi objects. Calls a function with midi data as arguments."},
    {Py_tp_traverse, RawMidi_traverse},
    {Py_tp_clear, RawMidi_clear},
    {Py_tp_methods, RawMidi_methods},
    {Py_tp_members, RawMidi_members},
    {Py_tp_new, RawMidi_new},
    {0, NULL}
};

static PyType_Spec RawMidiType_spec =
{
    "_pyo.RawMidi_base",
    sizeof(RawMidi),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    RawMidiType_slots
};

PyTypeObject *
PyoCreateRawMidiType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &RawMidiType_spec, NULL);
}

/*********************************************************************************************/
/* MidiLinseg *********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *pointslist;
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    double currentTime;
    double currentValue;
    MYFLT sampleToSec;
    double increment;
    MYFLT *targets;
    MYFLT *times;
    MYFLT ampscaling;
    int which;
    int flag;
    int noteon;
    int hold;
    int tmphold;
    int waiting;
    int newlist;
    int listsize;
    int startFromLast;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} MidiLinseg;

static void
MidiLinseg_convert_pointslist(MidiLinseg *self)
{
    int i;
    PyObject *tup;

    self->listsize = PyList_Size(self->pointslist);
    self->targets = (MYFLT *)PyMem_RawRealloc(self->targets, self->listsize * sizeof(MYFLT));
    self->times = (MYFLT *)PyMem_RawRealloc(self->times, self->listsize * sizeof(MYFLT));

    for (i = 0; i < self->listsize; i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        self->times[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
        self->targets[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
    }
}

static void
MidiLinseg_reinit(MidiLinseg *self)
{
    if (self->newlist == 1)
    {
        MidiLinseg_convert_pointslist((MidiLinseg *)self);
        self->newlist = 0;
    }

    if (self->tmphold != self->hold)
    {
        self->hold = self->tmphold;
    }

    if (self->hold < 1 || self->hold >= self->listsize)
    {
        self->hold = self->listsize / 2;
    }

    self->currentTime = 0.0;

    if (self->currentValue == 0.0)
    {
        self->currentValue = self->targets[0];
        self->startFromLast = 0;
    }
    else
    {
        self->startFromLast = 1;
    }

    self->which = 0;
    self->flag = 1;
    self->noteon = 1;
    self->waiting = 0;
}

static void
MidiLinseg_generate(MidiLinseg *self)
{
    int i;
    MYFLT timefactor = 1.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] > 0.0 && self->noteon == 0)
        {
            MidiLinseg_reinit((MidiLinseg *)self);
            self->ampscaling = in[i];
        }
        else if (in[i] == 0.0 && self->noteon == 1)
        {
            self->noteon = 0;
            self->waiting = 0;
        }

        if (self->flag == 1)
        {
            if (self->currentTime >= self->times[self->which])
            {
                self->which++;

                if (self->which == self->listsize)
                {
                    self->trigsBuffer[i] = 1.0;
                    self->flag = 0;
                    self->currentValue = self->targets[self->which - 1] * self->ampscaling;
                }
                else
                {
                    if ((self->which - 1) == self->hold && self->noteon)
                    {
                        self->currentValue = self->targets[self->which - 1] * self->ampscaling;
                        self->waiting = 1;
                    }

                    if ((self->times[self->which] - self->times[self->which - 1]) <= 0)
                    {
                        self->increment = self->targets[self->which] * self->ampscaling - self->currentValue;
                    }
                    else
                    {
                        timefactor = (self->times[self->which] - self->times[self->which - 1]) / self->sampleToSec;

                        if (self->startFromLast)
                        {
                            self->increment = (self->targets[self->which] * self->ampscaling - self->currentValue) / timefactor;
                            self->startFromLast = 0;
                        }
                        else
                        {
                            self->increment = (self->targets[self->which] - self->targets[self->which - 1]) * self->ampscaling / timefactor;
                        }
                    }
                }
            }

            if (self->waiting == 0 && self->currentTime <= self->times[self->listsize - 1])
            {
                self->currentValue += self->increment;
            }

            self->data[i] = (MYFLT)self->currentValue;

            if (self->waiting == 0)
            {
                self->currentTime += self->sampleToSec;
            }
        }
        else
            self->data[i] = (MYFLT)self->currentValue;
    }
}

static void MidiLinseg_postprocessing_ii(MidiLinseg *self) { POST_PROCESSING_II };
static void MidiLinseg_postprocessing_ai(MidiLinseg *self) { POST_PROCESSING_AI };
static void MidiLinseg_postprocessing_ia(MidiLinseg *self) { POST_PROCESSING_IA };
static void MidiLinseg_postprocessing_aa(MidiLinseg *self) { POST_PROCESSING_AA };
static void MidiLinseg_postprocessing_ireva(MidiLinseg *self) { POST_PROCESSING_IREVA };
static void MidiLinseg_postprocessing_areva(MidiLinseg *self) { POST_PROCESSING_AREVA };
static void MidiLinseg_postprocessing_revai(MidiLinseg *self) { POST_PROCESSING_REVAI };
static void MidiLinseg_postprocessing_revaa(MidiLinseg *self) { POST_PROCESSING_REVAA };
static void MidiLinseg_postprocessing_revareva(MidiLinseg *self) { POST_PROCESSING_REVAREVA };

static void
MidiLinseg_setProcMode(MidiLinseg *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_generate);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_postprocessing_revareva);
            break;
    }
}

static void
MidiLinseg_compute_next_data_frame(MidiLinseg *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
MidiLinseg_traverse(MidiLinseg *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->pointslist);
    Py_VISIT(self->input);
    return 0;
}

static int
MidiLinseg_clear(MidiLinseg *self)
{
    pyo_CLEAR
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->input);
    return 0;
}

static void
MidiLinseg_dealloc(MidiLinseg* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->targets);
    PyMem_RawFree(self->times);
    PyMem_RawFree(self->trigsBuffer);
    MidiLinseg_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiLinseg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pointslist = NULL, *multmp = NULL, *addtmp = NULL;
    MidiLinseg *self;
    self = (MidiLinseg *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->noteon = 0;
    self->newlist = 1;
    self->waiting = 0;
    self->ampscaling = 1.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(MidiLinseg_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(MidiLinseg_setProcMode);

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"input", "list", "hold", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &inputtmp, &pointslist, &self->hold, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    self->pointslist = pointslist;
    Py_INCREF(self->pointslist);
    MidiLinseg_convert_pointslist((MidiLinseg *)self);

    self->tmphold = self->hold;

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

static PyObject * MidiLinseg_getServer(MidiLinseg* self) { GET_SERVER };
static PyObject * MidiLinseg_getStream(MidiLinseg* self) { GET_STREAM };
static PyObject * MidiLinseg_getTriggerStream(MidiLinseg* self) { GET_TRIGGER_STREAM };
static PyObject * MidiLinseg_setMul(MidiLinseg *self, PyObject *arg) { SET_MUL };
static PyObject * MidiLinseg_setAdd(MidiLinseg *self, PyObject *arg) { SET_ADD };
static PyObject * MidiLinseg_setSub(MidiLinseg *self, PyObject *arg) { SET_SUB };
static PyObject * MidiLinseg_setDiv(MidiLinseg *self, PyObject *arg) { SET_DIV };

static PyObject * MidiLinseg_play(MidiLinseg *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiLinseg_stop(MidiLinseg *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MidiLinseg_multiply(MidiLinseg *self, PyObject *arg) { MULTIPLY };
static PyObject * MidiLinseg_inplace_multiply(MidiLinseg *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MidiLinseg_add(MidiLinseg *self, PyObject *arg) { ADD };
static PyObject * MidiLinseg_inplace_add(MidiLinseg *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MidiLinseg_sub(MidiLinseg *self, PyObject *arg) { SUB };
static PyObject * MidiLinseg_inplace_sub(MidiLinseg *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MidiLinseg_div(MidiLinseg *self, PyObject *arg) { DIV };
static PyObject * MidiLinseg_inplace_div(MidiLinseg *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
MidiLinseg_setList(MidiLinseg *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The points list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    self->newlist = 1;

    Py_RETURN_NONE;
}

static PyObject *
MidiLinseg_setHold(MidiLinseg *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->tmphold = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyMemberDef MidiLinseg_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MidiLinseg, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MidiLinseg, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(MidiLinseg, trig_stream), 0, "Trigger Stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(MidiLinseg, pointslist), 0, "List of target points."},
    {"mul", T_OBJECT_EX, offsetof(MidiLinseg, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MidiLinseg, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiLinseg_methods[] =
{
    {"getServer", (PyCFunction)MidiLinseg_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MidiLinseg_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)MidiLinseg_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)MidiLinseg_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiLinseg_stop, METH_VARARGS | METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"setList", (PyCFunction)MidiLinseg_setList, METH_O, "Sets target points list."},
    {"setHold", (PyCFunction)MidiLinseg_setHold, METH_O, "Sets hold point."},
    {"setMul", (PyCFunction)MidiLinseg_setMul, METH_O, "Sets MidiLinseg mul factor."},
    {"setAdd", (PyCFunction)MidiLinseg_setAdd, METH_O, "Sets MidiLinseg add factor."},
    {"setSub", (PyCFunction)MidiLinseg_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MidiLinseg_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot MidiLinsegType_slots[] = {
    {Py_tp_dealloc, MidiLinseg_dealloc},
    {Py_tp_doc, "MidiLinseg objects. Generates a linear segments break-points line."},
    {Py_tp_traverse, MidiLinseg_traverse},
    {Py_tp_clear, MidiLinseg_clear},
    {Py_tp_methods, MidiLinseg_methods},
    {Py_tp_members, MidiLinseg_members},
    {Py_tp_new, MidiLinseg_new},
    {Py_nb_add, MidiLinseg_add},
    {Py_nb_subtract, MidiLinseg_sub},
    {Py_nb_multiply, MidiLinseg_multiply},
    {Py_nb_true_divide, MidiLinseg_div},
    {Py_nb_inplace_add, MidiLinseg_inplace_add},
    {Py_nb_inplace_subtract, MidiLinseg_inplace_sub},
    {Py_nb_inplace_multiply, MidiLinseg_inplace_multiply},
    {Py_nb_inplace_true_divide, MidiLinseg_inplace_div},
    {0, NULL}
};

static PyType_Spec MidiLinsegType_spec =
{
    "_pyo.MidiLinseg_base",
    sizeof(MidiLinseg),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    MidiLinsegType_slots
};

PyTypeObject *
PyoCreateMidiLinsegType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &MidiLinsegType_spec, NULL);
}
