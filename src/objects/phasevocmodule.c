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
#include "pvstreammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"
#include "fft.h"
#include "wind.h"

static int
isPowerOfTwo(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *callback;
    PyObject *input;
    Stream *input_stream;
    PVStream *pv_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int wintype;
    int incount;
    int inputLatency;
    int overcount;
    MYFLT factor;
    MYFLT scale;
    MYFLT *input_buffer;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *real;
    MYFLT *imag;
    MYFLT *lastPhase;
    MYFLT **twiddle;
    MYFLT *window;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int allocated;
    int last_olaps;
} PVAnal;


static void
PVAnal_realloc_memories(PVAnal *self)
{
    int i, j, n8;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    self->factor = self->sr / (self->hopsize * TWOPI);
    self->scale = TWOPI * self->hopsize / self->size;
    self->inputLatency = self->size - self->hopsize;
    self->incount = self->inputLatency;
    self->overcount = 0;
    n8 = self->size >> 3;
    self->input_buffer = (MYFLT *)PyMem_RawRealloc(self->input_buffer, self->size * sizeof(MYFLT));
    self->inframe = (MYFLT *)PyMem_RawRealloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)PyMem_RawRealloc(self->outframe, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
        self->input_buffer[i] = self->inframe[i] = self->outframe[i] = 0.0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->lastPhase = (MYFLT *)PyMem_RawRealloc(self->lastPhase, self->hsize * sizeof(MYFLT));
    self->real = (MYFLT *)PyMem_RawRealloc(self->real, self->hsize * sizeof(MYFLT));
    self->imag = (MYFLT *)PyMem_RawRealloc(self->imag, self->hsize * sizeof(MYFLT));
    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->hsize; i++)
        self->lastPhase[i] = self->real[i] = self->imag[i] = 0.0;

    self->twiddle = (MYFLT **)PyMem_RawRealloc(self->twiddle, 4 * sizeof(MYFLT *));

    for (i = 0; i < 4; i++)
    {
        if (self->allocated)
        {
            PyMem_RawFree(self->twiddle[i]);
        }
        self->twiddle[i] = (MYFLT *)PyMem_RawMalloc(n8 * sizeof(MYFLT));
    }

    fft_compute_split_twiddle(self->twiddle, self->size);
    self->window = (MYFLT *)PyMem_RawRealloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = self->incount;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVAnal_data_callback(PVAnal *self)
{
    int i;
    PyObject *tuple, *result, *magnitudes, *frequencies;

    magnitudes = PyList_New(self->hsize);
    frequencies = PyList_New(self->hsize);

    for (i = 0; i < self->hsize; i++)
    {
        PyList_SET_ITEM(magnitudes, i, PyFloat_FromDouble(self->magn[self->overcount][i]));
        PyList_SET_ITEM(frequencies, i, PyFloat_FromDouble(self->freq[self->overcount][i]));
    }

    tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, magnitudes);
    PyTuple_SET_ITEM(tuple, 1, frequencies);
    result = PyObject_Call(self->callback, tuple, NULL);

    if (result == NULL)
    {
        PyErr_Print();
    }
}

static void
PVAnal_process(PVAnal *self)
{
    int i, k, mod;
    MYFLT real, imag, mag, phase, tmp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->input_buffer[self->incount] = in[i];
        self->count[i] = self->incount;
        self->incount++;

        if (self->incount >= self->size)
        {
            self->incount = self->inputLatency;
            mod = self->hopsize * self->overcount;

            for (k = 0; k < self->size; k++)
            {
                self->inframe[(k + mod) % self->size] = self->input_buffer[k] * self->window[k];
            }

            realfft_split(self->inframe, self->outframe, self->size, self->twiddle);
            self->real[0] = self->outframe[0];
            self->imag[0] = 0.0;

            for (k = 1; k < self->hsize; k++)
            {
                self->real[k] = self->outframe[k];
                self->imag[k] = self->outframe[self->size - k];
            }

            for (k = 0; k < self->hsize; k++)
            {
                real = self->real[k];
                imag = self->imag[k];
                mag = MYSQRT(real * real + imag * imag);
                phase = MYATAN2(imag, real);
                tmp = phase - self->lastPhase[k];
                self->lastPhase[k] = phase;

                while (tmp > PI) tmp -= TWOPI;

                while (tmp < -PI) tmp += TWOPI;

                self->magn[self->overcount][k] = mag;
                self->freq[self->overcount][k] = (tmp + k * self->scale) * self->factor;
            }

            if (self->callback != Py_None)
            {
                PVAnal_data_callback(self);
            }

            for (k = 0; k < self->inputLatency; k++)
            {
                self->input_buffer[k] = self->input_buffer[k + self->hopsize];
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAnal_setProcMode(PVAnal *self)
{
    self->proc_func_ptr = PVAnal_process;
}

static void
PVAnal_compute_next_data_frame(PVAnal *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVAnal_traverse(PVAnal *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);

    if (self->callback != Py_None)
    {
        Py_VISIT(self->callback);
    }

    return 0;
}

static int
PVAnal_clear(PVAnal *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);

    if (self->callback != Py_None)
    {
        Py_CLEAR(self->callback);
    }

    return 0;
}

static void
PVAnal_dealloc(PVAnal* self)
{
    int i;
    pyo_DEALLOC
    PyMem_RawFree(self->input_buffer);
    PyMem_RawFree(self->inframe);
    PyMem_RawFree(self->outframe);
    PyMem_RawFree(self->real);
    PyMem_RawFree(self->imag);
    PyMem_RawFree(self->lastPhase);

    for (i = 0; i < 4; i++)
    {
        PyMem_RawFree(self->twiddle[i]);
    }

    PyMem_RawFree(self->twiddle);
    PyMem_RawFree(self->window);

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVAnal_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVAnal_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, k;
    PyObject *inputtmp, *input_streamtmp, *callbacktmp = NULL;
    PVAnal *self;
    self = (PVAnal *)type->tp_alloc(type, 0);

    Py_INCREF(Py_None);
    self->callback = Py_None;
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->wintype = 2;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVAnal_compute_next_data_frame);
    self->mode_func_ptr = PVAnal_setProcMode;

    static char *kwlist[] = {"input", "size", "olaps", "wintype", "callback", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiO", kwlist, &inputtmp, &self->size, &self->olaps, &self->wintype, &callbacktmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (callbacktmp)
    {
        PyObject_CallMethod((PyObject *)self, "setCallback", "O", callbacktmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    if (!isPowerOfTwo(self->size))
    {
        k = 1;

        while (k < self->size)
            k *= 2;

        self->size = k;
        PySys_WriteStdout("FFT size must be a power-of-2, using the next power-of-2 greater than size : %d\n", self->size);
    }

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVAnal_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVAnal_getServer(PVAnal* self) { GET_SERVER };
static PyObject * PVAnal_getStream(PVAnal* self) { GET_STREAM };
static PyObject * PVAnal_getPVStream(PVAnal* self) { GET_PV_STREAM };

static PyObject * PVAnal_play(PVAnal *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVAnal_stop(PVAnal *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVAnal_setSize(PVAnal *self, PyObject *arg)
{
    int k;

    if (PyLong_Check(arg))
    {
        self->size = PyLong_AsLong(arg);

        if (!isPowerOfTwo(self->size))
        {
            k = 1;

            while (k < self->size)
                k *= 2;

            self->size = k;
            PySys_WriteStdout("FFT size must be a power-of-2, using the next power-of-2 greater than size : %d\n", self->size);
        }

        PVAnal_realloc_memories(self);
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAnal_setOverlaps(PVAnal *self, PyObject *arg)
{
    int k;

    if (PyLong_Check(arg))
    {
        self->olaps = PyLong_AsLong(arg);

        if (!isPowerOfTwo(self->olaps))
        {
            k = 1;

            while (k < self->olaps)
                k *= 2;

            self->olaps = k;
            PySys_WriteStdout("FFT overlaps must be a power-of-2, using the next power-of-2 greater than olaps : %d\n", self->olaps);
        }

        PVAnal_realloc_memories(self);
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAnal_setWinType(PVAnal *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAnal_setCallback(PVAnal *self, PyObject *arg)
{
    if (! PyCallable_Check(arg) && arg != Py_None)
    {
        PyErr_SetString(PyExc_TypeError, "The callback attribute must be callable.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->callback);
    self->callback = arg;
    Py_INCREF(self->callback);
 
    Py_RETURN_NONE;
}

static PyMemberDef PVAnal_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVAnal, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVAnal, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVAnal, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVAnal, input), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVAnal_methods[] =
{
    {"getServer", (PyCFunction)PVAnal_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVAnal_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVAnal_getPVStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVAnal_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVAnal_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)PVAnal_setSize, METH_O, NULL},
    {"setOverlaps", (PyCFunction)PVAnal_setOverlaps, METH_O, NULL},
    {"setWinType", (PyCFunction)PVAnal_setWinType, METH_O, NULL},
    {"setCallback", (PyCFunction)PVAnal_setCallback, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVAnalType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVAnal_base",                                   /*tp_name*/
    sizeof(PVAnal),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVAnal_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVAnal_traverse,                  /* tp_traverse */
    (inquiry)PVAnal_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVAnal_methods,                                 /* tp_methods */
    PVAnal_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVAnal_new,                                     /* tp_new */
};

/*****************/
/**** PVSynth ****/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    int size;
    int hsize;
    int olaps;
    int hopsize;
    int wintype;
    int inputLatency;
    int overcount;
    MYFLT ampscl;
    MYFLT factor;
    MYFLT scale;
    MYFLT *output_buffer;
    MYFLT *outputAccum;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *real;
    MYFLT *imag;
    MYFLT *sumPhase;
    MYFLT **twiddle;
    MYFLT *window;
    int modebuffer[2]; // need at least 2 slots for mul & add
    int allocated;
} PVSynth;


static void
PVSynth_realloc_memories(PVSynth *self)
{
    int i, n8;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    self->factor = self->hopsize * TWOPI / self->sr;
    self->scale = self->sr / self->size;
    self->inputLatency = self->size - self->hopsize;
    self->overcount = 0;
    self->ampscl = 1.0 / MYSQRT(self->olaps);
    n8 = self->size >> 3;
    self->output_buffer = (MYFLT *)PyMem_RawRealloc(self->output_buffer, self->size * sizeof(MYFLT));
    self->inframe = (MYFLT *)PyMem_RawRealloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)PyMem_RawRealloc(self->outframe, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
        self->output_buffer[i] = self->inframe[i] = self->outframe[i] = 0.0;

    self->sumPhase = (MYFLT *)PyMem_RawRealloc(self->sumPhase, self->hsize * sizeof(MYFLT));
    self->real = (MYFLT *)PyMem_RawRealloc(self->real, self->hsize * sizeof(MYFLT));
    self->imag = (MYFLT *)PyMem_RawRealloc(self->imag, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
        self->sumPhase[i] = self->real[i] = self->imag[i] = 0.0;

    self->outputAccum = (MYFLT *)PyMem_RawRealloc(self->outputAccum, (self->size + self->hopsize) * sizeof(MYFLT));

    for (i = 0; i < (self->size + self->hopsize); i++)
        self->outputAccum[i] = 0.0;

    self->twiddle = (MYFLT **)PyMem_RawRealloc(self->twiddle, 4 * sizeof(MYFLT *));

    for (i = 0; i < 4; i++)
    {
        if (self->allocated)
        {
            PyMem_RawFree(self->twiddle[i]);
        }
        self->twiddle[i] = (MYFLT *)PyMem_RawMalloc(n8 * sizeof(MYFLT));
    }

    fft_compute_split_twiddle(self->twiddle, self->size);
    self->window = (MYFLT *)PyMem_RawRealloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);

    self->allocated = 1;
}

static void
PVSynth_process(PVSynth *self)
{
    int i, k, mod;
    MYFLT mag, phase, tmp;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVSynth_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = self->output_buffer[count[i] - self->inputLatency];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                mag = magn[self->overcount][k];
                tmp = freq[self->overcount][k];
                tmp = (tmp - k * self->scale) * self->factor;
                self->sumPhase[k] += tmp;
                phase = self->sumPhase[k];
                self->real[k] = mag * MYCOS(phase);
                self->imag[k] = mag * MYSIN(phase);
            }

            self->inframe[0] = self->real[0];
            self->inframe[self->hsize] = 0.0;

            for (k = 1; k < self->hsize; k++)
            {
                self->inframe[k] = self->real[k];
                self->inframe[self->size - k] = self->imag[k];
            }

            irealfft_split(self->inframe, self->outframe, self->size, self->twiddle);
            mod = self->hopsize * self->overcount;

            for (k = 0; k < self->size; k++)
            {
                self->outputAccum[k] += self->outframe[(k + mod) % self->size] * self->window[k] * self->ampscl;
            }

            for (k = 0; k < self->hopsize; k++)
            {
                self->output_buffer[k] = self->outputAccum[k];
            }

            for (k = 0; k < self->size; k++)
            {
                self->outputAccum[k] = self->outputAccum[k + self->hopsize];
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void PVSynth_postprocessing_ii(PVSynth *self) { POST_PROCESSING_II };
static void PVSynth_postprocessing_ai(PVSynth *self) { POST_PROCESSING_AI };
static void PVSynth_postprocessing_ia(PVSynth *self) { POST_PROCESSING_IA };
static void PVSynth_postprocessing_aa(PVSynth *self) { POST_PROCESSING_AA };
static void PVSynth_postprocessing_ireva(PVSynth *self) { POST_PROCESSING_IREVA };
static void PVSynth_postprocessing_areva(PVSynth *self) { POST_PROCESSING_AREVA };
static void PVSynth_postprocessing_revai(PVSynth *self) { POST_PROCESSING_REVAI };
static void PVSynth_postprocessing_revaa(PVSynth *self) { POST_PROCESSING_REVAA };
static void PVSynth_postprocessing_revareva(PVSynth *self) { POST_PROCESSING_REVAREVA };

static void
PVSynth_setProcMode(PVSynth *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PVSynth_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PVSynth_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = PVSynth_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = PVSynth_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = PVSynth_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = PVSynth_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = PVSynth_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = PVSynth_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = PVSynth_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = PVSynth_postprocessing_revareva;
            break;
    }
}

static void
PVSynth_compute_next_data_frame(PVSynth *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
PVSynth_traverse(PVSynth *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
PVSynth_clear(PVSynth *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
PVSynth_dealloc(PVSynth* self)
{
    int i;
    pyo_DEALLOC
    PyMem_RawFree(self->output_buffer);
    PyMem_RawFree(self->outputAccum);
    PyMem_RawFree(self->inframe);
    PyMem_RawFree(self->outframe);
    PyMem_RawFree(self->real);
    PyMem_RawFree(self->imag);
    PyMem_RawFree(self->sumPhase);

    for (i = 0; i < 4; i++)
    {
        PyMem_RawFree(self->twiddle[i]);
    }

    PyMem_RawFree(self->twiddle);
    PyMem_RawFree(self->window);
    PVSynth_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVSynth_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    PVSynth *self;
    self = (PVSynth *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    self->wintype = 2;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVSynth_compute_next_data_frame);
    self->mode_func_ptr = PVSynth_setProcMode;

    static char *kwlist[] = {"input", "wintype", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->wintype, &multmp, &addtmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVSynth must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (multmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    PVSynth_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
PVSynth_setInput(PVSynth *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVSynth must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVSynth_setWinType(PVSynth *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }

    Py_RETURN_NONE;
}

static PyObject * PVSynth_getServer(PVSynth* self) { GET_SERVER };
static PyObject * PVSynth_getStream(PVSynth* self) { GET_STREAM };
//static PyObject * PVSynth_getPVStream(PVSynth* self) { GET_PV_STREAM };
static PyObject * PVSynth_setMul(PVSynth *self, PyObject *arg) { SET_MUL };
static PyObject * PVSynth_setAdd(PVSynth *self, PyObject *arg) { SET_ADD };
static PyObject * PVSynth_setSub(PVSynth *self, PyObject *arg) { SET_SUB };
static PyObject * PVSynth_setDiv(PVSynth *self, PyObject *arg) { SET_DIV };

static PyObject * PVSynth_play(PVSynth *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVSynth_out(PVSynth *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * PVSynth_stop(PVSynth *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * PVSynth_multiply(PVSynth *self, PyObject *arg) { MULTIPLY };
static PyObject * PVSynth_inplace_multiply(PVSynth *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PVSynth_add(PVSynth *self, PyObject *arg) { ADD };
static PyObject * PVSynth_inplace_add(PVSynth *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PVSynth_sub(PVSynth *self, PyObject *arg) { SUB };
static PyObject * PVSynth_inplace_sub(PVSynth *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PVSynth_div(PVSynth *self, PyObject *arg) { DIV };
static PyObject * PVSynth_inplace_div(PVSynth *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef PVSynth_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVSynth, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVSynth, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVSynth, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(PVSynth, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(PVSynth, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVSynth_methods[] =
{
    {"getServer", (PyCFunction)PVSynth_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVSynth_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVSynth_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)PVSynth_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVSynth_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setInput", (PyCFunction)PVSynth_setInput, METH_O, NULL},
    {"setWinType", (PyCFunction)PVSynth_setWinType, METH_O, NULL},
    {"setMul", (PyCFunction)PVSynth_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)PVSynth_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)PVSynth_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)PVSynth_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PVSynth_as_number =
{
    (binaryfunc)PVSynth_add,                         /*nb_add*/
    (binaryfunc)PVSynth_sub,                         /*nb_subtract*/
    (binaryfunc)PVSynth_multiply,                    /*nb_multiply*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)PVSynth_inplace_add,                 /*inplace_add*/
    (binaryfunc)PVSynth_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)PVSynth_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)PVSynth_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)PVSynth_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject PVSynthType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVSynth_base",                                   /*tp_name*/
    sizeof(PVSynth),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVSynth_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &PVSynth_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVSynth_traverse,                  /* tp_traverse */
    (inquiry)PVSynth_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVSynth_methods,                                 /* tp_methods */
    PVSynth_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVSynth_new,                                     /* tp_new */
};

/*****************/
/**** PVAddSynth ****/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PyObject *pitch;
    Stream *pitch_stream;
    int size;
    int hsize;
    int olaps;
    int hopsize;
    int inputLatency;
    int overcount;
    int num;
    int first;
    int inc;
    int update;
    MYFLT *ppos;
    MYFLT *amp;
    MYFLT *freq;
    MYFLT *outbuf;
    MYFLT *table;
    int modebuffer[3]; // need at least 2 slots for mul & add
} PVAddSynth;

static void
PVAddSynth_realloc_memories(PVAddSynth *self)
{
    int i;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    self->inputLatency = self->size - self->hopsize;
    self->overcount = 0;
    self->ppos = (MYFLT *)PyMem_RawRealloc(self->ppos, self->num * sizeof(MYFLT));
    self->amp = (MYFLT *)PyMem_RawRealloc(self->amp, self->num * sizeof(MYFLT));
    self->freq = (MYFLT *)PyMem_RawRealloc(self->freq, self->num * sizeof(MYFLT));

    for (i = 0; i < self->num; i++)
    {
        self->ppos[i] = self->amp[i] = 0.0;
        self->freq[i] = (i * self->inc + self->first) * self->size / self->sr;
    }

    self->outbuf = (MYFLT *)PyMem_RawRealloc(self->outbuf, self->hopsize * sizeof(MYFLT));

    for (i = 0; i < self->hopsize; i++)
        self->outbuf[i] = 0.0;
}

static void
PVAddSynth_process_i(PVAddSynth *self)
{
    int i, k, n, bin, ipart;
    MYFLT pitch, tamp, tfreq, inca, incf, ratio, fpart;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    pitch = PyFloat_AS_DOUBLE(self->pitch);

    if (self->size != size || self->olaps != olaps || self->update == 1)
    {
        self->size = size;
        self->olaps = olaps;
        self->update = 0;
        PVAddSynth_realloc_memories(self);
    }

    ratio = 8192.0 / self->sr;

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = self->outbuf[count[i] - self->inputLatency];

        if (count[i] >= (self->size - 1))
        {
            for (n = 0; n < self->hopsize; n++)
            {
                self->outbuf[n] = 0.0;
            }

            for (k = 0; k < self->num; k++)
            {
                bin = k * self->inc + self->first;

                if (bin < self->hsize)
                {
                    tamp = magn[self->overcount][bin];
                    tfreq = freq[self->overcount][bin] * pitch;
                    inca = (tamp - self->amp[k]) / self->hopsize;
                    incf = (tfreq - self->freq[k]) / self->hopsize;

                    for (n = 0; n < self->hopsize; n++)
                    {
                        self->ppos[k] += self->freq[k] * ratio;

                        while (self->ppos[k] < 0.0) self->ppos[k] += 8192.0;

                        while (self->ppos[k] >= 8192.0) self->ppos[k] -= 8192.0;

                        ipart = (int)self->ppos[k];
                        fpart = self->ppos[k] - ipart;
                        self->outbuf[n] += self->amp[k] * (self->table[ipart] + (self->table[ipart + 1] - self->table[ipart]) * fpart);
                        self->amp[k] += inca;
                        self->freq[k] += incf;
                    }
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAddSynth_process_a(PVAddSynth *self)
{
    int i, k, n, bin, ipart;
    MYFLT pitch, tamp, tfreq, inca, incf, ratio, fpart;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);

    if (self->size != size || self->olaps != olaps || self->update == 1)
    {
        self->size = size;
        self->olaps = olaps;
        self->update = 0;
        PVAddSynth_realloc_memories(self);
    }

    ratio = 8192.0 / self->sr;

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = self->outbuf[count[i] - self->inputLatency];

        if (count[i] >= (self->size - 1))
        {
            pitch = pit[i];

            for (n = 0; n < self->hopsize; n++)
            {
                self->outbuf[n] = 0.0;
            }

            for (k = 0; k < self->num; k++)
            {
                bin = k * self->inc + self->first;

                if (bin < self->hsize)
                {
                    tamp = magn[self->overcount][bin];
                    tfreq = freq[self->overcount][bin] * pitch;
                    inca = (tamp - self->amp[k]) / self->hopsize;
                    incf = (tfreq - self->freq[k]) / self->hopsize;

                    for (n = 0; n < self->hopsize; n++)
                    {
                        self->ppos[k] += self->freq[k] * ratio;

                        while (self->ppos[k] < 0.0) self->ppos[k] += 8192.0;

                        while (self->ppos[k] >= 8192.0) self->ppos[k] -= 8192.0;

                        ipart = (int)self->ppos[k];
                        fpart = self->ppos[k] - ipart;
                        self->outbuf[n] += self->amp[k] * (self->table[ipart] + (self->table[ipart + 1] - self->table[ipart]) * fpart);
                        self->amp[k] += inca;
                        self->freq[k] += incf;
                    }
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void PVAddSynth_postprocessing_ii(PVAddSynth *self) { POST_PROCESSING_II };
static void PVAddSynth_postprocessing_ai(PVAddSynth *self) { POST_PROCESSING_AI };
static void PVAddSynth_postprocessing_ia(PVAddSynth *self) { POST_PROCESSING_IA };
static void PVAddSynth_postprocessing_aa(PVAddSynth *self) { POST_PROCESSING_AA };
static void PVAddSynth_postprocessing_ireva(PVAddSynth *self) { POST_PROCESSING_IREVA };
static void PVAddSynth_postprocessing_areva(PVAddSynth *self) { POST_PROCESSING_AREVA };
static void PVAddSynth_postprocessing_revai(PVAddSynth *self) { POST_PROCESSING_REVAI };
static void PVAddSynth_postprocessing_revaa(PVAddSynth *self) { POST_PROCESSING_REVAA };
static void PVAddSynth_postprocessing_revareva(PVAddSynth *self) { POST_PROCESSING_REVAREVA };

static void
PVAddSynth_setProcMode(PVAddSynth *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVAddSynth_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVAddSynth_process_a;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PVAddSynth_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = PVAddSynth_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = PVAddSynth_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = PVAddSynth_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = PVAddSynth_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = PVAddSynth_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = PVAddSynth_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = PVAddSynth_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = PVAddSynth_postprocessing_revareva;
            break;
    }
}

static void
PVAddSynth_compute_next_data_frame(PVAddSynth *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
PVAddSynth_traverse(PVAddSynth *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->pitch);
    return 0;
}

static int
PVAddSynth_clear(PVAddSynth *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->pitch);
    return 0;
}

static void
PVAddSynth_dealloc(PVAddSynth* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->ppos);
    PyMem_RawFree(self->outbuf);
    PyMem_RawFree(self->table);
    PyMem_RawFree(self->amp);
    PyMem_RawFree(self->freq);
    PVAddSynth_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVAddSynth_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pitchtmp = NULL, *multmp = NULL, *addtmp = NULL;
    PVAddSynth *self;
    self = (PVAddSynth *)type->tp_alloc(type, 0);

    self->pitch = PyFloat_FromDouble(1);
    self->num = 100;
    self->first = 0;
    self->inc = 1;
    self->update = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVAddSynth_compute_next_data_frame);
    self->mode_func_ptr = PVAddSynth_setProcMode;

    static char *kwlist[] = {"input", "pitch", "num", "first", "inc", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OiiiOO", kwlist, &inputtmp, &pitchtmp, &self->num, &self->first, &self->inc, &multmp, &addtmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVAddSynth must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (pitchtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setPitch", "O", pitchtmp);
    }

    if (multmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->table = (MYFLT *)PyMem_RawRealloc(self->table, 8193 * sizeof(MYFLT));

    for (i = 0; i < 8192; i++)
        self->table[i] = (MYFLT)(MYSIN(TWOPI * i / 8192.0));

    self->table[8192] = 0.0;

    PVAddSynth_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
PVAddSynth_setInput(PVAddSynth *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVAddSynth must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVAddSynth_setPitch(PVAddSynth *self, PyObject *arg) { SET_PARAM(self->pitch, self->pitch_stream, 2); }

static PyObject *
PVAddSynth_setNum(PVAddSynth *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->num = PyLong_AsLong(arg);

        if (self->num < 1)
            self->num = 1;
        else if (self->num > self->hsize)
            self->num = self->hsize;

        self->update = 1;
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAddSynth_setFirst(PVAddSynth *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->first = PyLong_AsLong(arg);

        if (self->first < 0)
            self->first = 0;
        else if (self->first > self->hsize)
            self->first = self->hsize;

        self->update = 1;
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAddSynth_setInc(PVAddSynth *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->inc = PyLong_AsLong(arg);

        if (self->inc < 1)
            self->inc = 1;
        else if (self->inc > self->hsize)
            self->inc = self->hsize;

        self->update = 1;
    }

    Py_RETURN_NONE;
}

static PyObject * PVAddSynth_getServer(PVAddSynth* self) { GET_SERVER };
static PyObject * PVAddSynth_getStream(PVAddSynth* self) { GET_STREAM };
static PyObject * PVAddSynth_setMul(PVAddSynth *self, PyObject *arg) { SET_MUL };
static PyObject * PVAddSynth_setAdd(PVAddSynth *self, PyObject *arg) { SET_ADD };
static PyObject * PVAddSynth_setSub(PVAddSynth *self, PyObject *arg) { SET_SUB };
static PyObject * PVAddSynth_setDiv(PVAddSynth *self, PyObject *arg) { SET_DIV };

static PyObject * PVAddSynth_play(PVAddSynth *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVAddSynth_out(PVAddSynth *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * PVAddSynth_stop(PVAddSynth *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * PVAddSynth_multiply(PVAddSynth *self, PyObject *arg) { MULTIPLY };
static PyObject * PVAddSynth_inplace_multiply(PVAddSynth *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PVAddSynth_add(PVAddSynth *self, PyObject *arg) { ADD };
static PyObject * PVAddSynth_inplace_add(PVAddSynth *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PVAddSynth_sub(PVAddSynth *self, PyObject *arg) { SUB };
static PyObject * PVAddSynth_inplace_sub(PVAddSynth *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PVAddSynth_div(PVAddSynth *self, PyObject *arg) { DIV };
static PyObject * PVAddSynth_inplace_div(PVAddSynth *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef PVAddSynth_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVAddSynth, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVAddSynth, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVAddSynth, input), 0, NULL},
    {"pitch", T_OBJECT_EX, offsetof(PVAddSynth, pitch), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(PVAddSynth, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(PVAddSynth, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVAddSynth_methods[] =
{
    {"getServer", (PyCFunction)PVAddSynth_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVAddSynth_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVAddSynth_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)PVAddSynth_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVAddSynth_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setInput", (PyCFunction)PVAddSynth_setInput, METH_O, NULL},
    {"setPitch", (PyCFunction)PVAddSynth_setPitch, METH_O, NULL},
    {"setNum", (PyCFunction)PVAddSynth_setNum, METH_O, NULL},
    {"setFirst", (PyCFunction)PVAddSynth_setFirst, METH_O, NULL},
    {"setInc", (PyCFunction)PVAddSynth_setInc, METH_O, NULL},
    {"setMul", (PyCFunction)PVAddSynth_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)PVAddSynth_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)PVAddSynth_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)PVAddSynth_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PVAddSynth_as_number =
{
    (binaryfunc)PVAddSynth_add,                         /*nb_add*/
    (binaryfunc)PVAddSynth_sub,                         /*nb_subtract*/
    (binaryfunc)PVAddSynth_multiply,                    /*nb_multiply*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)PVAddSynth_inplace_add,                 /*inplace_add*/
    (binaryfunc)PVAddSynth_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)PVAddSynth_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)PVAddSynth_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)PVAddSynth_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject PVAddSynthType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVAddSynth_base",                                   /*tp_name*/
    sizeof(PVAddSynth),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVAddSynth_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &PVAddSynth_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVAddSynth_traverse,                  /* tp_traverse */
    (inquiry)PVAddSynth_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVAddSynth_methods,                                 /* tp_methods */
    PVAddSynth_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVAddSynth_new,                                     /* tp_new */
};

/*****************/
/** PVTranspose **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *transpo;
    Stream *transpo_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
} PVTranspose;

static void
PVTranspose_realloc_memories(PVTranspose *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVTranspose_process_i(PVTranspose *self)
{
    int i, k, index;
    MYFLT transpo;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    transpo = PyFloat_AS_DOUBLE(self->transpo);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVTranspose_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                index = (int)(k * transpo);

                if (index < self->hsize)
                {
                    self->magn[self->overcount][index] += magn[self->overcount][k];
                    self->freq[self->overcount][index] = freq[self->overcount][k] * transpo;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVTranspose_process_a(PVTranspose *self)
{
    int i, k, index;
    MYFLT transpo;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *tr = Stream_getData((Stream *)self->transpo_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVTranspose_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            transpo = tr[i];

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                index = (int)(k * transpo);

                if (index < self->hsize)
                {
                    self->magn[self->overcount][index] += magn[self->overcount][k];
                    self->freq[self->overcount][index] = freq[self->overcount][k] * transpo;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVTranspose_setProcMode(PVTranspose *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVTranspose_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVTranspose_process_a;
            break;
    }
}

static void
PVTranspose_compute_next_data_frame(PVTranspose *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVTranspose_traverse(PVTranspose *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->transpo);
    return 0;
}

static int
PVTranspose_clear(PVTranspose *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->transpo);
    return 0;
}

static void
PVTranspose_dealloc(PVTranspose* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVTranspose_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVTranspose_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *transpotmp;
    PVTranspose *self;
    self = (PVTranspose *)type->tp_alloc(type, 0);

    self->transpo = PyFloat_FromDouble(1);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVTranspose_compute_next_data_frame);
    self->mode_func_ptr = PVTranspose_setProcMode;

    static char *kwlist[] = {"input", "transpo", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &inputtmp, &transpotmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVTranspose must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (transpotmp)
    {
        PyObject_CallMethod((PyObject *)self, "setTranspo", "O", transpotmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVTranspose_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVTranspose_getServer(PVTranspose* self) { GET_SERVER };
static PyObject * PVTranspose_getStream(PVTranspose* self) { GET_STREAM };
static PyObject * PVTranspose_getPVStream(PVTranspose* self) { GET_PV_STREAM };

static PyObject * PVTranspose_play(PVTranspose *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVTranspose_stop(PVTranspose *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVTranspose_setInput(PVTranspose *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVTranspose must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVTranspose_setTranspo(PVTranspose *self, PyObject *arg) {SET_PARAM(self->transpo, self->transpo_stream, 0); }

static PyMemberDef PVTranspose_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVTranspose, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVTranspose, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVTranspose, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVTranspose, input), 0, NULL},
    {"transpo", T_OBJECT_EX, offsetof(PVTranspose, transpo), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVTranspose_methods[] =
{
    {"getServer", (PyCFunction)PVTranspose_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVTranspose_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVTranspose_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVTranspose_setInput, METH_O, NULL},
    {"setTranspo", (PyCFunction)PVTranspose_setTranspo, METH_O, NULL},
    {"play", (PyCFunction)PVTranspose_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVTranspose_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVTransposeType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVTranspose_base",                                   /*tp_name*/
    sizeof(PVTranspose),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVTranspose_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVTranspose_traverse,                  /* tp_traverse */
    (inquiry)PVTranspose_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVTranspose_methods,                                 /* tp_methods */
    PVTranspose_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVTranspose_new,                                     /* tp_new */
};

/*****************/
/** PVVerb **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *revtime;
    Stream *revtime_stream;
    PyObject *damp;
    Stream *damp_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT *l_magn;
    MYFLT *l_freq;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[2];
    int allocated;
    int last_olaps;
} PVVerb;

static void
PVVerb_realloc_memories(PVVerb *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;
    self->l_magn = (MYFLT *)PyMem_RawRealloc(self->l_magn, self->hsize * sizeof(MYFLT));
    self->l_freq = (MYFLT *)PyMem_RawRealloc(self->l_freq, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
        self->l_magn[i] = self->l_freq[i] = 0.0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVVerb_process_ii(PVVerb *self)
{
    int i, k;
    MYFLT revtime, damp, mag, amp, fre;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    revtime = PyFloat_AS_DOUBLE(self->revtime);
    damp = PyFloat_AS_DOUBLE(self->damp);

    if (revtime < 0.0)
        revtime = 0.0;
    else if (revtime > 1.0)
        revtime = 1.0;

    revtime = revtime * 0.25 + 0.75;

    if (damp < 0.0)
        damp = 0.0;
    else if (damp > 1.0)
        damp = 1.0;

    damp = damp * 0.003 + 0.997;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVVerb_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            amp = 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                mag = magn[self->overcount][k];
                fre = freq[self->overcount][k];

                if (mag > self->l_magn[k])
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre;
                }
                else
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag + (self->l_magn[k] - mag) * revtime * amp;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre + (self->l_freq[k] - fre) * revtime * amp;
                }

                amp *= damp;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVVerb_process_ai(PVVerb *self)
{
    int i, k;
    MYFLT revtime, damp, mag, amp, fre;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *rvt = Stream_getData((Stream *)self->revtime_stream);
    damp = PyFloat_AS_DOUBLE(self->damp);

    if (damp < 0.0)
        damp = 0.0;
    else if (damp > 1.0)
        damp = 1.0;

    damp = damp * 0.003 + 0.997;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVVerb_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            revtime = rvt[i];

            if (revtime < 0.0)
                revtime = 0.0;
            else if (revtime > 1.0)
                revtime = 1.0;

            revtime = revtime * 0.25 + 0.75;
            amp = 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                mag = magn[self->overcount][k];
                fre = freq[self->overcount][k];

                if (mag > self->l_magn[k])
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre;
                }
                else
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag + (self->l_magn[k] - mag) * revtime * amp;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre + (self->l_freq[k] - fre) * revtime * amp;
                }

                amp *= damp;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVVerb_process_ia(PVVerb *self)
{
    int i, k;
    MYFLT revtime, damp, mag, amp, fre;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    revtime = PyFloat_AS_DOUBLE(self->revtime);
    MYFLT *dmp = Stream_getData((Stream *)self->damp_stream);

    if (revtime < 0.0)
        revtime = 0.0;
    else if (revtime > 1.0)
        revtime = 1.0;

    revtime = revtime * 0.25 + 0.75;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVVerb_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            damp = dmp[i];

            if (damp < 0.0)
                damp = 0.0;
            else if (damp > 1.0)
                damp = 1.0;

            damp = damp * 0.003 + 0.997;
            amp = 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                mag = magn[self->overcount][k];
                fre = freq[self->overcount][k];

                if (mag > self->l_magn[k])
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre;
                }
                else
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag + (self->l_magn[k] - mag) * revtime * amp;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre + (self->l_freq[k] - fre) * revtime * amp;
                }

                amp *= damp;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVVerb_process_aa(PVVerb *self)
{
    int i, k;
    MYFLT revtime, damp, mag, amp, fre;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *rvt = Stream_getData((Stream *)self->revtime_stream);
    MYFLT *dmp = Stream_getData((Stream *)self->damp_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVVerb_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            revtime = rvt[i];

            if (revtime < 0.0)
                revtime = 0.0;
            else if (revtime > 1.0)
                revtime = 1.0;

            revtime = revtime * 0.25 + 0.75;
            damp = dmp[i];

            if (damp < 0.0)
                damp = 0.0;
            else if (damp > 1.0)
                damp = 1.0;

            damp = damp * 0.003 + 0.997;
            amp = 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                mag = magn[self->overcount][k];
                fre = freq[self->overcount][k];

                if (mag > self->l_magn[k])
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre;
                }
                else
                {
                    self->magn[self->overcount][k] = self->l_magn[k] = mag + (self->l_magn[k] - mag) * revtime * amp;
                    self->freq[self->overcount][k] = self->l_freq[k] = fre + (self->l_freq[k] - fre) * revtime * amp;
                }

                amp *= damp;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVVerb_setProcMode(PVVerb *self)
{
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVVerb_process_ii;
            break;

        case 1:
            self->proc_func_ptr = PVVerb_process_ai;
            break;

        case 10:
            self->proc_func_ptr = PVVerb_process_ia;
            break;

        case 11:
            self->proc_func_ptr = PVVerb_process_aa;
            break;
    }
}

static void
PVVerb_compute_next_data_frame(PVVerb *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVVerb_traverse(PVVerb *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->revtime);
    Py_VISIT(self->damp);
    return 0;
}

static int
PVVerb_clear(PVVerb *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->revtime);
    Py_CLEAR(self->damp);
    return 0;
}

static void
PVVerb_dealloc(PVVerb* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->l_magn);
    PyMem_RawFree(self->l_freq);
    PyMem_RawFree(self->count);
    PVVerb_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVVerb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *revtimetmp = NULL, *damptmp = NULL;
    PVVerb *self;
    self = (PVVerb *)type->tp_alloc(type, 0);

    self->revtime = PyFloat_FromDouble(0.75);
    self->damp = PyFloat_FromDouble(0.75);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVVerb_compute_next_data_frame);
    self->mode_func_ptr = PVVerb_setProcMode;

    static char *kwlist[] = {"input", "revtime", "damp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &revtimetmp, &damptmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVVerb must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (revtimetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setRevtime", "O", revtimetmp);
    }

    if (damptmp)
    {
        PyObject_CallMethod((PyObject *)self, "setDamp", "O", damptmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVVerb_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVVerb_getServer(PVVerb* self) { GET_SERVER };
static PyObject * PVVerb_getStream(PVVerb* self) { GET_STREAM };
static PyObject * PVVerb_getPVStream(PVVerb* self) { GET_PV_STREAM };

static PyObject * PVVerb_play(PVVerb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVVerb_stop(PVVerb *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVVerb_setInput(PVVerb *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVVerb must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVVerb_setRevtime(PVVerb *self, PyObject *arg) { SET_PARAM(self->revtime, self->revtime_stream, 0); }
static PyObject * PVVerb_setDamp(PVVerb *self, PyObject *arg) { SET_PARAM(self->damp, self->damp_stream, 1); }

static PyMemberDef PVVerb_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVVerb, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVVerb, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVVerb, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVVerb, input), 0, NULL},
    {"revtime", T_OBJECT_EX, offsetof(PVVerb, revtime), 0, NULL},
    {"damp", T_OBJECT_EX, offsetof(PVVerb, damp), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVVerb_methods[] =
{
    {"getServer", (PyCFunction)PVVerb_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVVerb_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVVerb_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVVerb_setInput, METH_O, NULL},
    {"setRevtime", (PyCFunction)PVVerb_setRevtime, METH_O, NULL},
    {"setDamp", (PyCFunction)PVVerb_setDamp, METH_O, NULL},
    {"play", (PyCFunction)PVVerb_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVVerb_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVVerbType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVVerb_base",                                   /*tp_name*/
    sizeof(PVVerb),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVVerb_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVVerb_traverse,                  /* tp_traverse */
    (inquiry)PVVerb_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVVerb_methods,                                 /* tp_methods */
    PVVerb_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVVerb_new,                                     /* tp_new */
};

/*****************/
/** PVGate **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *thresh;
    Stream *thresh_stream;
    PyObject *damp;
    Stream *damp_stream;
    int inverse;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[2];
    int allocated;
    int last_olaps;
} PVGate;

static void
PVGate_realloc_memories(PVGate *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVGate_process_ii(PVGate *self)
{
    int i, k;
    MYFLT thresh, damp, mag;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    thresh = PyFloat_AS_DOUBLE(self->thresh);
    damp = PyFloat_AS_DOUBLE(self->damp);
    thresh = MYPOW(10.0, thresh * 0.05);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVGate_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->inverse == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag < thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag > thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVGate_process_ai(PVGate *self)
{
    int i, k;
    MYFLT thresh, damp, mag;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *rvt = Stream_getData((Stream *)self->thresh_stream);
    damp = PyFloat_AS_DOUBLE(self->damp);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVGate_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            thresh = rvt[i];
            thresh = MYPOW(10.0, thresh * 0.05);

            if (self->inverse == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag < thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag > thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVGate_process_ia(PVGate *self)
{
    int i, k;
    MYFLT thresh, damp, mag;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    thresh = PyFloat_AS_DOUBLE(self->thresh);
    MYFLT *dmp = Stream_getData((Stream *)self->damp_stream);
    thresh = MYPOW(10.0, thresh * 0.05);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVGate_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            damp = dmp[i];

            if (self->inverse == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag < thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag > thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVGate_process_aa(PVGate *self)
{
    int i, k;
    MYFLT thresh, damp, mag;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *rvt = Stream_getData((Stream *)self->thresh_stream);
    MYFLT *dmp = Stream_getData((Stream *)self->damp_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVGate_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            thresh = rvt[i];
            thresh = MYPOW(10.0, thresh * 0.05);
            damp = dmp[i];

            if (self->inverse == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag < thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    mag = magn[self->overcount][k];

                    if (mag > thresh)
                        self->magn[self->overcount][k] = mag * damp;
                    else
                        self->magn[self->overcount][k] = mag;

                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVGate_setProcMode(PVGate *self)
{
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVGate_process_ii;
            break;

        case 1:
            self->proc_func_ptr = PVGate_process_ai;
            break;

        case 10:
            self->proc_func_ptr = PVGate_process_ia;
            break;

        case 11:
            self->proc_func_ptr = PVGate_process_aa;
            break;
    }
}

static void
PVGate_compute_next_data_frame(PVGate *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVGate_traverse(PVGate *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->thresh);
    Py_VISIT(self->damp);
    return 0;
}

static int
PVGate_clear(PVGate *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->thresh);
    Py_CLEAR(self->damp);
    return 0;
}

static void
PVGate_dealloc(PVGate* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVGate_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVGate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *threshtmp = NULL, *damptmp = NULL;
    PVGate *self;
    self = (PVGate *)type->tp_alloc(type, 0);

    self->thresh = PyFloat_FromDouble(-20);
    self->damp = PyFloat_FromDouble(0.0);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    self->inverse = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVGate_compute_next_data_frame);
    self->mode_func_ptr = PVGate_setProcMode;

    static char *kwlist[] = {"input", "thresh", "damp", "inverse", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOi", kwlist, &inputtmp, &threshtmp, &damptmp, &self->inverse))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVGate must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (threshtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setThresh", "O", threshtmp);
    }

    if (damptmp)
    {
        PyObject_CallMethod((PyObject *)self, "setDamp", "O", damptmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVGate_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVGate_getServer(PVGate* self) { GET_SERVER };
static PyObject * PVGate_getStream(PVGate* self) { GET_STREAM };
static PyObject * PVGate_getPVStream(PVGate* self) { GET_PV_STREAM };

static PyObject * PVGate_play(PVGate *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVGate_stop(PVGate *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVGate_setInput(PVGate *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVGate must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVGate_setThresh(PVGate *self, PyObject *arg) { SET_PARAM(self->thresh, self->thresh_stream, 0); }
static PyObject * PVGate_setDamp(PVGate *self, PyObject *arg) { SET_PARAM(self->damp, self->damp_stream, 1); }

static PyObject *
PVGate_setInverse(PVGate *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->inverse = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyMemberDef PVGate_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVGate, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVGate, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVGate, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVGate, input), 0, NULL},
    {"thresh", T_OBJECT_EX, offsetof(PVGate, thresh), 0, NULL},
    {"damp", T_OBJECT_EX, offsetof(PVGate, damp), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVGate_methods[] =
{
    {"getServer", (PyCFunction)PVGate_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVGate_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVGate_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVGate_setInput, METH_O, NULL},
    {"setThresh", (PyCFunction)PVGate_setThresh, METH_O, NULL},
    {"setDamp", (PyCFunction)PVGate_setDamp, METH_O, NULL},
    {"setInverse", (PyCFunction)PVGate_setInverse, METH_O, NULL},
    {"play", (PyCFunction)PVGate_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVGate_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVGateType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVGate_base",                                   /*tp_name*/
    sizeof(PVGate),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVGate_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVGate_traverse,                  /* tp_traverse */
    (inquiry)PVGate_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVGate_methods,                                 /* tp_methods */
    PVGate_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVGate_new,                                     /* tp_new */
};

/*****************/
/** PVCross **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PyObject *input2;
    PVStream *input2_stream;
    PVStream *pv_stream;
    PyObject *fade;
    Stream *fade_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
} PVCross;

static void
PVCross_realloc_memories(PVCross *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVCross_process_i(PVCross *self)
{
    int i, k;
    MYFLT fade;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    fade = PyFloat_AS_DOUBLE(self->fade);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVCross_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = magn[self->overcount][k] + (magn2[self->overcount][k] - magn[self->overcount][k]) * fade;
                self->freq[self->overcount][k] = freq[self->overcount][k];
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVCross_process_a(PVCross *self)
{
    int i, k;
    MYFLT fade;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *fd = Stream_getData((Stream *)self->fade_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVCross_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            fade = fd[i];

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = magn[self->overcount][k] + (magn2[self->overcount][k] - magn[self->overcount][k]) * fade;
                self->freq[self->overcount][k] = freq[self->overcount][k];
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVCross_setProcMode(PVCross *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVCross_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVCross_process_a;
            break;
    }
}

static void
PVCross_compute_next_data_frame(PVCross *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVCross_traverse(PVCross *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    Py_VISIT(self->fade);
    return 0;
}

static int
PVCross_clear(PVCross *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->fade);
    return 0;
}

static void
PVCross_dealloc(PVCross* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVCross_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVCross_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *fadetmp;
    PVCross *self;
    self = (PVCross *)type->tp_alloc(type, 0);

    self->fade = PyFloat_FromDouble(1);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVCross_compute_next_data_frame);
    self->mode_func_ptr = PVCross_setProcMode;

    static char *kwlist[] = {"input", "input2", "fade", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist, &inputtmp, &input2tmp, &fadetmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVCross must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    if ( PyObject_HasAttrString((PyObject *)input2tmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVCross must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT2_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (fadetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setFade", "O", fadetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVCross_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVCross_getServer(PVCross* self) { GET_SERVER };
static PyObject * PVCross_getStream(PVCross* self) { GET_STREAM };
static PyObject * PVCross_getPVStream(PVCross* self) { GET_PV_STREAM };

static PyObject * PVCross_play(PVCross *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVCross_stop(PVCross *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVCross_setInput(PVCross *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVCross must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVCross_setInput2(PVCross *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVCross must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input2);

    self->input2 = arg;
    Py_INCREF(self->input2);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getPVStream", NULL);
    self->input2_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input2_stream);

    Py_RETURN_NONE;
}

static PyObject * PVCross_setFade(PVCross *self, PyObject *arg) { SET_PARAM(self->fade, self->fade_stream, 0); }

static PyMemberDef PVCross_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVCross, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVCross, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVCross, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVCross, input), 0, NULL},
    {"input2", T_OBJECT_EX, offsetof(PVCross, input2), 0, NULL},
    {"fade", T_OBJECT_EX, offsetof(PVCross, fade), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVCross_methods[] =
{
    {"getServer", (PyCFunction)PVCross_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVCross_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVCross_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVCross_setInput, METH_O, NULL},
    {"setInput2", (PyCFunction)PVCross_setInput2, METH_O, NULL},
    {"setFade", (PyCFunction)PVCross_setFade, METH_O, NULL},
    {"play", (PyCFunction)PVCross_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVCross_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVCrossType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVCross_base",                                   /*tp_name*/
    sizeof(PVCross),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVCross_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVCross_traverse,                  /* tp_traverse */
    (inquiry)PVCross_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVCross_methods,                                 /* tp_methods */
    PVCross_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVCross_new,                                     /* tp_new */
};

/*****************/
/** PVMult **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PyObject *input2;
    PVStream *input2_stream;
    PVStream *pv_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int allocated;
    int last_olaps;
} PVMult;

static void
PVMult_realloc_memories(PVMult *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVMult_process_i(PVMult *self)
{
    int i, k;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVMult_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = magn[self->overcount][k] * magn2[self->overcount][k] * 10;
                self->freq[self->overcount][k] = freq[self->overcount][k];
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVMult_setProcMode(PVMult *self)
{
    self->proc_func_ptr = PVMult_process_i;
}

static void
PVMult_compute_next_data_frame(PVMult *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVMult_traverse(PVMult *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    return 0;
}

static int
PVMult_clear(PVMult *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    return 0;
}

static void
PVMult_dealloc(PVMult* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVMult_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVMult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp;
    PVMult *self;
    self = (PVMult *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVMult_compute_next_data_frame);
    self->mode_func_ptr = PVMult_setProcMode;

    static char *kwlist[] = {"input", "input2", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &input2tmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMult must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    if ( PyObject_HasAttrString((PyObject *)input2tmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMult must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT2_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVMult_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVMult_getServer(PVMult* self) { GET_SERVER };
static PyObject * PVMult_getStream(PVMult* self) { GET_STREAM };
static PyObject * PVMult_getPVStream(PVMult* self) { GET_PV_STREAM };

static PyObject * PVMult_play(PVMult *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVMult_stop(PVMult *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVMult_setInput(PVMult *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMult must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVMult_setInput2(PVMult *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMult must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input2);

    self->input2 = arg;
    Py_INCREF(self->input2);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getPVStream", NULL);
    self->input2_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input2_stream);

    Py_RETURN_NONE;
}

static PyMemberDef PVMult_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVMult, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVMult, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVMult, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVMult, input), 0, NULL},
    {"input2", T_OBJECT_EX, offsetof(PVMult, input2), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVMult_methods[] =
{
    {"getServer", (PyCFunction)PVMult_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVMult_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVMult_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVMult_setInput, METH_O, NULL},
    {"setInput2", (PyCFunction)PVMult_setInput2, METH_O, NULL},
    {"play", (PyCFunction)PVMult_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVMult_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVMultType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVMult_base",                                   /*tp_name*/
    sizeof(PVMult),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVMult_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVMult_traverse,                  /* tp_traverse */
    (inquiry)PVMult_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVMult_methods,                                 /* tp_methods */
    PVMult_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVMult_new,                                     /* tp_new */
};

/*****************/
/** PVMorph **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PyObject *input2;
    PVStream *input2_stream;
    PVStream *pv_stream;
    PyObject *fade;
    Stream *fade_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
} PVMorph;

static void
PVMorph_realloc_memories(PVMorph *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVMorph_process_i(PVMorph *self)
{
    int i, k;
    MYFLT fade, fr1, fr2, div;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    MYFLT **freq2 = PVStream_getFreq((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    fade = PyFloat_AS_DOUBLE(self->fade);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVMorph_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = magn[self->overcount][k] + (magn2[self->overcount][k] - magn[self->overcount][k]) * fade;
                fr1 = freq[self->overcount][k];
                fr2 = freq2[self->overcount][k];
                div = fr1 ? fr2 / fr1 : 1000000.0;
                div = div > 0 ? div : -div;
                self->freq[self->overcount][k] = fr1 * MYPOW(div, fade);
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVMorph_process_a(PVMorph *self)
{
    int i, k;
    MYFLT fade, fr1, fr2, div;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    MYFLT **freq2 = PVStream_getFreq((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *fd = Stream_getData((Stream *)self->fade_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVMorph_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            fade = fd[i];

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = magn[self->overcount][k] + (magn2[self->overcount][k] - magn[self->overcount][k]) * fade;
                fr1 = freq[self->overcount][k];
                fr2 = freq2[self->overcount][k];
                div = fr1 ? fr2 / fr1 : 1000000.0;
                div = div > 0 ? div : -div;
                self->freq[self->overcount][k] = fr1 * MYPOW(div, fade);
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVMorph_setProcMode(PVMorph *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVMorph_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVMorph_process_a;
            break;
    }
}

static void
PVMorph_compute_next_data_frame(PVMorph *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVMorph_traverse(PVMorph *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    Py_VISIT(self->fade);
    return 0;
}

static int
PVMorph_clear(PVMorph *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->fade);
    return 0;
}

static void
PVMorph_dealloc(PVMorph* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVMorph_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVMorph_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *fadetmp;
    PVMorph *self;
    self = (PVMorph *)type->tp_alloc(type, 0);

    self->fade = PyFloat_FromDouble(0.5);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVMorph_compute_next_data_frame);
    self->mode_func_ptr = PVMorph_setProcMode;

    static char *kwlist[] = {"input", "input2", "fade", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist, &inputtmp, &input2tmp, &fadetmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMorph must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    if ( PyObject_HasAttrString((PyObject *)input2tmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMorph must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT2_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (fadetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setFade", "O", fadetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVMorph_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVMorph_getServer(PVMorph* self) { GET_SERVER };
static PyObject * PVMorph_getStream(PVMorph* self) { GET_STREAM };
static PyObject * PVMorph_getPVStream(PVMorph* self) { GET_PV_STREAM };

static PyObject * PVMorph_play(PVMorph *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVMorph_stop(PVMorph *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVMorph_setInput(PVMorph *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMorph must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVMorph_setInput2(PVMorph *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMorph must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input2);

    self->input2 = arg;
    Py_INCREF(self->input2);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getPVStream", NULL);
    self->input2_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input2_stream);

    Py_RETURN_NONE;
}

static PyObject * PVMorph_setFade(PVMorph *self, PyObject *arg) { SET_PARAM(self->fade, self->fade_stream, 0); }

static PyMemberDef PVMorph_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVMorph, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVMorph, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVMorph, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVMorph, input), 0, NULL},
    {"input2", T_OBJECT_EX, offsetof(PVMorph, input2), 0, NULL},
    {"fade", T_OBJECT_EX, offsetof(PVMorph, fade), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVMorph_methods[] =
{
    {"getServer", (PyCFunction)PVMorph_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVMorph_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVMorph_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVMorph_setInput, METH_O, NULL},
    {"setInput2", (PyCFunction)PVMorph_setInput2, METH_O, NULL},
    {"setFade", (PyCFunction)PVMorph_setFade, METH_O, NULL},
    {"play", (PyCFunction)PVMorph_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVMorph_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVMorphType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVMorph_base",                                   /*tp_name*/
    sizeof(PVMorph),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVMorph_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVMorph_traverse,                  /* tp_traverse */
    (inquiry)PVMorph_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVMorph_methods,                                 /* tp_methods */
    PVMorph_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVMorph_new,                                     /* tp_new */
};

/*****************/
/** PVFilter **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *gain;
    Stream *gain_stream;
    PyObject *table;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    int mode;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
} PVFilter;

static void
PVFilter_realloc_memories(PVFilter *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVFilter_process_i(PVFilter *self)
{
    int i, k, ipart = 0;
    MYFLT gain, amp, binamp, factor, index = 0.0;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *tablelist = TableStream_getData((TableStream *)self->table);
    int tsize = TableStream_getSize((TableStream *)self->table);
    gain = PyFloat_AS_DOUBLE(self->gain);

    if (gain < 0)
        gain = 0.0;
    else if (gain > 1)
        gain = 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFilter_realloc_memories(self);
    }

    factor = (MYFLT)tsize / self->hsize;

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->mode == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    if (k < tsize)
                        binamp = tablelist[k];
                    else
                        binamp = 0.0;

                    amp = magn[self->overcount][k];
                    self->magn[self->overcount][k] = amp + ((binamp * amp) - amp) * gain;
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    index = k * factor;
                    ipart = (int)index;
                    binamp = tablelist[ipart] + (tablelist[ipart + 1] - tablelist[ipart]) * (index - ipart);
                    amp = magn[self->overcount][k];
                    self->magn[self->overcount][k] = amp + ((binamp * amp) - amp) * gain;
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFilter_process_a(PVFilter *self)
{
    int i, k, ipart = 0;
    MYFLT gain, amp, binamp, factor, index = 0.0;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *tablelist = TableStream_getData((TableStream *)self->table);
    int tsize = TableStream_getSize((TableStream *)self->table);
    MYFLT *gn = Stream_getData((Stream *)self->gain_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFilter_realloc_memories(self);
    }

    factor = (MYFLT)tsize / self->hsize;

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            gain = gn[i];

            if (gain < 0)
                gain = 0.0;
            else if (gain > 1)
                gain = 1.0;

            if (self->mode == 0)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    if (k < tsize)
                        binamp = tablelist[k];
                    else
                        binamp = 0.0;

                    amp = magn[self->overcount][k];
                    self->magn[self->overcount][k] = amp + ((binamp * amp) - amp) * gain;
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    index = k * factor;
                    ipart = (int)index;
                    binamp = tablelist[ipart] + (tablelist[ipart + 1] - tablelist[ipart]) * (index - ipart);
                    amp = magn[self->overcount][k];
                    self->magn[self->overcount][k] = amp + ((binamp * amp) - amp) * gain;
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFilter_setProcMode(PVFilter *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVFilter_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVFilter_process_a;
            break;
    }
}

static void
PVFilter_compute_next_data_frame(PVFilter *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVFilter_traverse(PVFilter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->gain);
    return 0;
}

static int
PVFilter_clear(PVFilter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->gain);
    return 0;
}

static void
PVFilter_dealloc(PVFilter* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVFilter_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVFilter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *gaintmp = NULL;
    PVFilter *self;
    self = (PVFilter *)type->tp_alloc(type, 0);

    self->gain = PyFloat_FromDouble(1);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    self->mode = 0; /* 0 : index outside table range clipped to 0
                       1 : index between 0 and hsize are scaled over table length */
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVFilter_compute_next_data_frame);
    self->mode_func_ptr = PVFilter_setProcMode;

    static char *kwlist[] = {"input", "table", "gain", "mode", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|Oi", kwlist, &inputtmp, &tabletmp, &gaintmp, &self->mode))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVFilter must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    if (gaintmp)
    {
        PyObject_CallMethod((PyObject *)self, "setGain", "O", gaintmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVFilter_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVFilter_getServer(PVFilter* self) { GET_SERVER };
static PyObject * PVFilter_getStream(PVFilter* self) { GET_STREAM };
static PyObject * PVFilter_getPVStream(PVFilter* self) { GET_PV_STREAM };

static PyObject * PVFilter_play(PVFilter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVFilter_stop(PVFilter *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVFilter_setInput(PVFilter *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVFilter must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVFilter_setGain(PVFilter *self, PyObject *arg) { SET_PARAM(self->gain, self->gain_stream, 0); }

static PyObject *
PVFilter_getTable(PVFilter* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
PVFilter_setTable(PVFilter *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject *
PVFilter_setMode(PVFilter *self, PyObject *arg)
{
    int tmp;

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp <= 0)
            self->mode = 0;
        else
            self->mode = 1;
    }

    Py_RETURN_NONE;
}

static PyMemberDef PVFilter_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVFilter, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVFilter, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVFilter, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVFilter, input), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(PVFilter, table), 0, NULL},
    {"gain", T_OBJECT_EX, offsetof(PVFilter, gain), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVFilter_methods[] =
{
    {"getServer", (PyCFunction)PVFilter_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVFilter_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVFilter_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVFilter_setInput, METH_O, NULL},
    {"getTable", (PyCFunction)PVFilter_getTable, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)PVFilter_setTable, METH_O, NULL},
    {"setGain", (PyCFunction)PVFilter_setGain, METH_O, NULL},
    {"setMode", (PyCFunction)PVFilter_setMode, METH_O, NULL},
    {"play", (PyCFunction)PVFilter_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVFilter_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVFilterType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVFilter_base",                                   /*tp_name*/
    sizeof(PVFilter),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVFilter_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVFilter_traverse,                  /* tp_traverse */
    (inquiry)PVFilter_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVFilter_methods,                                 /* tp_methods */
    PVFilter_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVFilter_new,                                     /* tp_new */
};

/*****************/
/** PVDelay **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *deltable;
    PyObject *feedtable;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT maxdelay;
    int numFrames;
    int framecount;
    MYFLT **magn;
    MYFLT **freq;
    MYFLT **magn_buf;
    MYFLT **freq_buf;
    int *count;
    int mode;
    int allocated;
    int last_olaps;
    int last_numFrames;
} PVDelay;

static void
PVDelay_realloc_memories(PVDelay *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->numFrames = (int)(self->maxdelay * self->sr / self->hopsize + 0.5);
    self->overcount = 0;
    self->framecount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
        for (i = 0; i < self->last_numFrames; i++)
        {
            PyMem_RawFree(self->magn_buf[i]);
            PyMem_RawFree(self->freq_buf[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    self->magn_buf = (MYFLT **)PyMem_RawRealloc(self->magn_buf, self->numFrames * sizeof(MYFLT *));
    self->freq_buf = (MYFLT **)PyMem_RawRealloc(self->freq_buf, self->numFrames * sizeof(MYFLT *));

    for (i = 0; i < self->numFrames; i++)
    {
        self->magn_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn_buf[i][j] = self->freq_buf[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->last_numFrames = self->numFrames;
    self->allocated = 1;
}

static void
PVDelay_process_zero(PVDelay *self)
{
    int i, k, bindel;
    MYFLT binfeed, mg, fr;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *dellist = TableStream_getData((TableStream *)self->deltable);
    int tsize = TableStream_getSize((TableStream *)self->deltable);
    MYFLT *feedlist = TableStream_getData((TableStream *)self->feedtable);
    int fsize = TableStream_getSize((TableStream *)self->feedtable);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVDelay_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                if (k < tsize)
                {
                    bindel = (int)dellist[k];

                    if (bindel < 0)
                        bindel = 0;
                    else if (bindel >= self->numFrames)
                        bindel = self->numFrames - 1;
                }
                else
                    bindel = 0;

                if (k < fsize)
                {
                    binfeed = feedlist[k];

                    if (binfeed < -1.0)
                        binfeed = -1.0;
                    else if (binfeed > 1.0)
                        binfeed = 1.0;
                }
                else
                    binfeed = 0;

                bindel = self->framecount - bindel;

                if (bindel < 0)
                    bindel += self->numFrames;

                if (bindel == self->framecount)
                {
                    self->magn[self->overcount][k] = magn[self->overcount][k];
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
                else
                {
                    self->magn[self->overcount][k] = mg = self->magn_buf[bindel][k];
                    self->freq[self->overcount][k] = fr = self->freq_buf[bindel][k];
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k] + mg * binfeed;
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k] + (fr - freq[self->overcount][k]) * binfeed;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;

            self->framecount++;

            if (self->framecount >= self->numFrames)
                self->framecount = 0;
        }
    }
}

static void
PVDelay_process_scaled(PVDelay *self)
{
    int i, k, bindel, ipart;
    MYFLT binfeed, mg, fr, tfac, ffac, index;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *dellist = TableStream_getData((TableStream *)self->deltable);
    int tsize = TableStream_getSize((TableStream *)self->deltable);
    MYFLT *feedlist = TableStream_getData((TableStream *)self->feedtable);
    int fsize = TableStream_getSize((TableStream *)self->feedtable);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVDelay_realloc_memories(self);
    }

    tfac = (MYFLT)tsize / self->hsize;
    ffac = (MYFLT)fsize / self->hsize;

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                index = k * tfac;
                ipart = (int)index;
                bindel = (int)(dellist[ipart] + (dellist[ipart + 1] - dellist[ipart]) * (index - ipart));

                if (bindel < 0)
                    bindel = 0;
                else if (bindel >= self->numFrames)
                    bindel = self->numFrames - 1;

                index = k * ffac;
                ipart = (int)index;
                binfeed = feedlist[ipart] + (feedlist[ipart + 1] - feedlist[ipart]) * (index - ipart);

                if (binfeed < -1.0)
                    binfeed = -1.0;
                else if (binfeed > 1.0)
                    binfeed = 1.0;

                bindel = self->framecount - bindel;

                if (bindel < 0)
                    bindel += self->numFrames;

                if (bindel == self->framecount)
                {
                    self->magn[self->overcount][k] = magn[self->overcount][k];
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
                else
                {
                    self->magn[self->overcount][k] = mg = self->magn_buf[bindel][k];
                    self->freq[self->overcount][k] = fr = self->freq_buf[bindel][k];
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k] + mg * binfeed;
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k] + (fr - freq[self->overcount][k]) * binfeed;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;

            self->framecount++;

            if (self->framecount >= self->numFrames)
                self->framecount = 0;
        }
    }
}

static void
PVDelay_setProcMode(PVDelay *self)
{
    if (self->mode == 0)
        self->proc_func_ptr = PVDelay_process_zero;
    else
        self->proc_func_ptr = PVDelay_process_scaled;
}

static void
PVDelay_compute_next_data_frame(PVDelay *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVDelay_traverse(PVDelay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
PVDelay_clear(PVDelay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
PVDelay_dealloc(PVDelay* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);

    for (i = 0; i < self->numFrames; i++)
    {
        PyMem_RawFree(self->magn_buf[i]);
        PyMem_RawFree(self->freq_buf[i]);
    }

    PyMem_RawFree(self->magn_buf);
    PyMem_RawFree(self->freq_buf);
    PyMem_RawFree(self->count);
    PVDelay_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVDelay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *deltabletmp, *feedtabletmp;
    PVDelay *self;
    self = (PVDelay *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->numFrames = self->last_numFrames = 0;
    self->allocated = 0;
    self->maxdelay = 1.0;
    self->mode = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVDelay_compute_next_data_frame);
    self->mode_func_ptr = PVDelay_setProcMode;

    static char *kwlist[] = {"input", "deltable", "feedtable", "maxdelay", "mode", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OOO_FI, kwlist, &inputtmp, &deltabletmp, &feedtabletmp, &self->maxdelay, &self->mode))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVDelay must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    Py_XDECREF(self->deltable);
    self->deltable = PyObject_CallMethod((PyObject *)deltabletmp, "getTableStream", "");

    Py_XDECREF(self->feedtable);
    self->feedtable = PyObject_CallMethod((PyObject *)feedtabletmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVDelay_realloc_memories(self);

    self->mode = self->mode <= 0 ? 0 : 1;
    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVDelay_getServer(PVDelay* self) { GET_SERVER };
static PyObject * PVDelay_getStream(PVDelay* self) { GET_STREAM };
static PyObject * PVDelay_getPVStream(PVDelay* self) { GET_PV_STREAM };

static PyObject * PVDelay_play(PVDelay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVDelay_stop(PVDelay *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVDelay_setInput(PVDelay *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVDelay must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVDelay_getDeltable(PVDelay* self)
{
    Py_INCREF(self->deltable);
    return self->deltable;
};

static PyObject *
PVDelay_setDeltable(PVDelay *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->deltable);
    self->deltable = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject *
PVDelay_getFeedtable(PVDelay* self)
{
    Py_INCREF(self->feedtable);
    return self->feedtable;
};

static PyObject *
PVDelay_setFeedtable(PVDelay *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->feedtable);
    self->feedtable = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject *
PVDelay_setMode(PVDelay *self, PyObject *arg)
{
    int tmp;

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp <= 0)
            self->mode = 0;
        else
            self->mode = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyMemberDef PVDelay_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVDelay, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVDelay, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVDelay, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVDelay, input), 0, NULL},
    {"deltable", T_OBJECT_EX, offsetof(PVDelay, deltable), 0, NULL},
    {"feedtable", T_OBJECT_EX, offsetof(PVDelay, feedtable), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVDelay_methods[] =
{
    {"getServer", (PyCFunction)PVDelay_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVDelay_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVDelay_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVDelay_setInput, METH_O, NULL},
    {"getDeltable", (PyCFunction)PVDelay_getDeltable, METH_NOARGS, NULL},
    {"setDeltable", (PyCFunction)PVDelay_setDeltable, METH_O, NULL},
    {"getFeedtable", (PyCFunction)PVDelay_getFeedtable, METH_NOARGS, NULL},
    {"setFeedtable", (PyCFunction)PVDelay_setFeedtable, METH_O, NULL},
    {"setMode", (PyCFunction)PVDelay_setMode, METH_O, NULL},
    {"play", (PyCFunction)PVDelay_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVDelay_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVDelayType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVDelay_base",                                   /*tp_name*/
    sizeof(PVDelay),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVDelay_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVDelay_traverse,                  /* tp_traverse */
    (inquiry)PVDelay_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVDelay_methods,                                 /* tp_methods */
    PVDelay_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVDelay_new,                                     /* tp_new */
};

/*****************/
/** PVBuffer **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *index;
    Stream *index_stream;
    PyObject *pitch;
    Stream *pitch_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT length;
    int numFrames;
    int framecount;
    MYFLT **magn;
    MYFLT **freq;
    MYFLT **magn_buf;
    MYFLT **freq_buf;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
    int last_numFrames;
} PVBuffer;

static void
PVBuffer_realloc_memories(PVBuffer *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->numFrames = (int)(self->length * self->sr / self->hopsize + 0.5);
    self->overcount = 0;
    self->framecount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
        for (i = 0; i < self->last_numFrames; i++)
        {
            PyMem_RawFree(self->magn_buf[i]);
            PyMem_RawFree(self->freq_buf[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    self->magn_buf = (MYFLT **)PyMem_RawRealloc(self->magn_buf, self->numFrames * sizeof(MYFLT *));
    self->freq_buf = (MYFLT **)PyMem_RawRealloc(self->freq_buf, self->numFrames * sizeof(MYFLT *));

    for (i = 0; i < self->numFrames; i++)
    {
        self->magn_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn_buf[i][j] = self->freq_buf[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->last_numFrames = self->numFrames;
    self->allocated = 1;
}

static void
PVBuffer_process_i(PVBuffer *self)
{
    int i, k, frame, indexi;
    MYFLT index, pitch;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    pitch = PyFloat_AS_DOUBLE(self->pitch);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVBuffer_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->framecount < self->numFrames)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k];
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k];
                }

                self->framecount++;
            }

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            index = ind[i];

            if (index < 0.0)
                index = 0.0;
            else if (index >= 1.0)
                index = 1.0;

            frame = (int)(index * self->numFrames);

            for (k = 0; k < self->hsize; k++)
            {
                indexi = (int)(k * pitch);

                if (indexi < self->hsize)
                {
                    self->magn[self->overcount][indexi] += self->magn_buf[frame][k];
                    self->freq[self->overcount][indexi] = self->freq_buf[frame][k] * pitch;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVBuffer_process_a(PVBuffer *self)
{
    int i, k, frame, indexi;
    MYFLT index, pitch;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *ind = Stream_getData((Stream *)self->index_stream);
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVBuffer_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->framecount < self->numFrames)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k];
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k];
                }

                self->framecount++;
            }

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            index = ind[i];
            pitch = pit[i];

            if (index < 0.0)
                index = 0.0;
            else if (index >= 1.0)
                index = 1.0;

            frame = (int)(index * self->numFrames);

            for (k = 0; k < self->hsize; k++)
            {
                indexi = (int)(k * pitch);

                if (indexi < self->hsize)
                {
                    self->magn[self->overcount][indexi] += self->magn_buf[frame][k];
                    self->freq[self->overcount][indexi] = self->freq_buf[frame][k] * pitch;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVBuffer_setProcMode(PVBuffer *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVBuffer_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVBuffer_process_a;
            break;
    }
}

static void
PVBuffer_compute_next_data_frame(PVBuffer *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVBuffer_traverse(PVBuffer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->index);
    Py_VISIT(self->pitch);
    return 0;
}

static int
PVBuffer_clear(PVBuffer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->index);
    Py_CLEAR(self->pitch);
    return 0;
}

static void
PVBuffer_dealloc(PVBuffer* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);

    for (i = 0; i < self->numFrames; i++)
    {
        PyMem_RawFree(self->magn_buf[i]);
        PyMem_RawFree(self->freq_buf[i]);
    }

    PyMem_RawFree(self->magn_buf);
    PyMem_RawFree(self->freq_buf);
    PyMem_RawFree(self->count);
    PVBuffer_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVBuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *indextmp, *pitchtmp = NULL;
    PVBuffer *self;
    self = (PVBuffer *)type->tp_alloc(type, 0);

    self->index = PyFloat_FromDouble(0.0);

    self->pitch = PyFloat_FromDouble(1);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->numFrames = self->last_numFrames = 0;
    self->allocated = 0;
    self->length = 1.0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVBuffer_compute_next_data_frame);
    self->mode_func_ptr = PVBuffer_setProcMode;

    static char *kwlist[] = {"input", "index", "pitch", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_OF, kwlist, &inputtmp, &indextmp, &pitchtmp, &self->length))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBuffer must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (indextmp)
    {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }

    if (pitchtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setPitch", "O", pitchtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVBuffer_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVBuffer_getServer(PVBuffer* self) { GET_SERVER };
static PyObject * PVBuffer_getStream(PVBuffer* self) { GET_STREAM };
static PyObject * PVBuffer_getPVStream(PVBuffer* self) { GET_PV_STREAM };

static PyObject * PVBuffer_play(PVBuffer *self, PyObject *args, PyObject *kwds)
{
    self->framecount = 0;
    PLAY
};

static PyObject * PVBuffer_stop(PVBuffer *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVBuffer_setInput(PVBuffer *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBuffer must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVBuffer_setIndex(PVBuffer *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyObject_HasAttrString((PyObject *)arg, "server") == 0)
    {
        PyErr_SetString(PyExc_TypeError, "\"index\" argument of PVBuffer must be a PyoObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->index);

    self->index = arg;
    Py_INCREF(self->index);
    PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
    self->index_stream = (Stream *)streamtmp;
    Py_INCREF(self->index_stream);

    Py_RETURN_NONE;
}

static PyObject * PVBuffer_setPitch(PVBuffer *self, PyObject *arg) { SET_PARAM(self->pitch, self->pitch_stream, 0); }

static PyObject *
PVBuffer_setLength(PVBuffer *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        PyObject *length = PyNumber_Float(arg); 
        self->length = PyFloat_AsDouble(length);
        Py_DECREF(length);
        PVBuffer_realloc_memories(self);
    }

    Py_RETURN_NONE;
}

static PyMemberDef PVBuffer_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVBuffer, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVBuffer, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVBuffer, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVBuffer, input), 0, NULL},
    {"index", T_OBJECT_EX, offsetof(PVBuffer, index), 0, NULL},
    {"pitch", T_OBJECT_EX, offsetof(PVBuffer, pitch), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVBuffer_methods[] =
{
    {"getServer", (PyCFunction)PVBuffer_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVBuffer_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVBuffer_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVBuffer_setInput, METH_O, NULL},
    {"setIndex", (PyCFunction)PVBuffer_setIndex, METH_O, NULL},
    {"setPitch", (PyCFunction)PVBuffer_setPitch, METH_O, NULL},
    {"setLength", (PyCFunction)PVBuffer_setLength, METH_O, NULL},
    {"play", (PyCFunction)PVBuffer_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVBuffer_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVBufferType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVBuffer_base",                                   /*tp_name*/
    sizeof(PVBuffer),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVBuffer_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVBuffer_traverse,                  /* tp_traverse */
    (inquiry)PVBuffer_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVBuffer_methods,                                 /* tp_methods */
    PVBuffer_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVBuffer_new,                                     /* tp_new */
};

/*****************/
/** PVShift **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *shift;
    Stream *shift_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[1];
    int allocated;
    int last_olaps;
} PVShift;

static void
PVShift_realloc_memories(PVShift *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVShift_process_i(PVShift *self)
{
    int i, k, index, dev;
    MYFLT shift, freqPerBin;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    shift = PyFloat_AS_DOUBLE(self->shift);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVShift_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            freqPerBin = self->sr / self->size;
            dev = (int)MYFLOOR(shift / freqPerBin);

            for (k = 0; k < self->hsize; k++)
            {
                index = k + dev;

                if (index >= 0 && index < self->hsize)
                {
                    self->magn[self->overcount][index] += magn[self->overcount][k];
                    self->freq[self->overcount][index] = freq[self->overcount][k] + shift;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVShift_process_a(PVShift *self)
{
    int i, k, index, dev;
    MYFLT shift, freqPerBin;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *sh = Stream_getData((Stream *)self->shift_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVShift_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            shift = sh[i];

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = 0.0;
                self->freq[self->overcount][k] = 0.0;
            }

            freqPerBin = self->sr / self->size;
            dev = (int)MYFLOOR(shift / freqPerBin);

            for (k = 0; k < self->hsize; k++)
            {
                index = k + dev;

                if (index >= 0 && index < self->hsize)
                {
                    self->magn[self->overcount][index] += magn[self->overcount][k];
                    self->freq[self->overcount][index] = freq[self->overcount][k] + shift;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVShift_setProcMode(PVShift *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVShift_process_i;
            break;

        case 1:
            self->proc_func_ptr = PVShift_process_a;
            break;
    }
}

static void
PVShift_compute_next_data_frame(PVShift *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVShift_traverse(PVShift *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->shift);
    return 0;
}

static int
PVShift_clear(PVShift *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->shift);
    return 0;
}

static void
PVShift_dealloc(PVShift* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVShift_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVShift_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *shifttmp;
    PVShift *self;
    self = (PVShift *)type->tp_alloc(type, 0);

    self->shift = PyFloat_FromDouble(0);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVShift_compute_next_data_frame);
    self->mode_func_ptr = PVShift_setProcMode;

    static char *kwlist[] = {"input", "shift", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &inputtmp, &shifttmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVShift must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (shifttmp)
    {
        PyObject_CallMethod((PyObject *)self, "setShift", "O", shifttmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVShift_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVShift_getServer(PVShift* self) { GET_SERVER };
static PyObject * PVShift_getStream(PVShift* self) { GET_STREAM };
static PyObject * PVShift_getPVStream(PVShift* self) { GET_PV_STREAM };

static PyObject * PVShift_play(PVShift *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVShift_stop(PVShift *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVShift_setInput(PVShift *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVShift must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVShift_setShift(PVShift *self, PyObject *arg) { SET_PARAM(self->shift, self->shift_stream, 0); }

static PyMemberDef PVShift_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVShift, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVShift, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVShift, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVShift, input), 0, NULL},
    {"shift", T_OBJECT_EX, offsetof(PVShift, shift), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVShift_methods[] =
{
    {"getServer", (PyCFunction)PVShift_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVShift_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVShift_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVShift_setInput, METH_O, NULL},
    {"setShift", (PyCFunction)PVShift_setShift, METH_O, NULL},
    {"play", (PyCFunction)PVShift_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVShift_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVShiftType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVShift_base",                                   /*tp_name*/
    sizeof(PVShift),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVShift_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVShift_traverse,                  /* tp_traverse */
    (inquiry)PVShift_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVShift_methods,                                 /* tp_methods */
    PVShift_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVShift_new,                                     /* tp_new */
};

/*****************/
/** PVAmpMod **/
/*****************/
static void
PVMod_setTable(MYFLT *table, int shape)
{
    int i;

    if (shape < 0 || shape > 7)
        shape = 0;

    if (shape == 0)
    {
        for (i = 0; i < 8192; i++)
            table[i] = (MYFLT)(MYSIN(TWOPI * i / 8192.0) * 0.5 + 0.5);
    }
    else if (shape == 1)
    {
        for (i = 0; i < 8192; i++)
            table[i] = (MYFLT)(1.0 - (i / 8191.0));
    }
    else if (shape == 2)
    {
        for (i = 0; i < 8192; i++)
            table[i] = (MYFLT)(i / 8191.0);
    }
    else if (shape == 3)
    {
        for (i = 0; i < 4096; i++)
            table[i] = 1.0;

        for (i = 4096; i < 8192; i++)
            table[i] = 0.0;
    }
    else if (shape == 4)
    {
        for (i = 0; i < 2048; i++)
            table[i] = (MYFLT)(i / 4095.0) + 0.5;

        for (i = 2048; i < 6144; i++)
            table[i] = (MYFLT)(1.0 - ((i - 2048) / 4095.0));

        for (i = 6144; i < 8192; i++)
            table[i] = (MYFLT)((i - 6144) / 4095.0);
    }
    else if (shape == 5)
    {
        MYFLT val = RANDOM_UNIFORM;
        table[0] = val;

        for (i = 1; i < 8192; i++)
        {
            val += RANDOM_UNIFORM * 0.04 - 0.02;

            if (val < 0)
                val = -val;
            else if (val >= 1.0)
                val = 1.0 - (val - 1.0);

            table[i] = val;
        }
    }
    else if (shape == 6)
    {
        MYFLT val = RANDOM_UNIFORM;
        table[0] = val;

        for (i = 1; i < 8192; i++)
        {
            val += RANDOM_UNIFORM * 0.14 - 0.07;

            if (val < 0)
                val = -val;
            else if (val >= 1.0)
                val = 1.0 - (val - 1.0);

            table[i] = val;
        }
    }
    else if (shape == 7)
    {
        for (i = 0; i < 8192; i++)
        {
            table[i] = RANDOM_UNIFORM;
        }
    }

    table[8192] = table[0];
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *basefreq;
    Stream *basefreq_stream;
    PyObject *spread;
    Stream *spread_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT factor;
    MYFLT *table;
    MYFLT *pointers;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[2];
    int allocated;
    int last_olaps;
} PVAmpMod;

static void
PVAmpMod_realloc_memories(PVAmpMod *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;
    self->factor = 8192.0 / (self->sr / self->hopsize);

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->pointers = (MYFLT *)PyMem_RawRealloc(self->pointers, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVAmpMod_process_ii(PVAmpMod *self)
{
    int i, k;
    MYFLT bfreq, spread, pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    bfreq = PyFloat_AS_DOUBLE(self->basefreq);
    spread = PyFloat_AS_DOUBLE(self->spread);
    spread *= 0.001;
    spread += 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVAmpMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                self->magn[self->overcount][k] = magn[self->overcount][k] * self->table[(int)pos];
                self->freq[self->overcount][k] = freq[self->overcount][k];
                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAmpMod_process_ai(PVAmpMod *self)
{
    int i, k;
    MYFLT bfreq, spread, pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *bf = Stream_getData((Stream *)self->basefreq_stream);
    spread = PyFloat_AS_DOUBLE(self->spread);
    spread *= 0.001;
    spread += 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVAmpMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            bfreq = bf[i];

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                self->magn[self->overcount][k] = magn[self->overcount][k] * self->table[(int)pos];
                self->freq[self->overcount][k] = freq[self->overcount][k];
                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAmpMod_process_ia(PVAmpMod *self)
{
    int i, k;
    MYFLT bfreq, spread, pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    bfreq = PyFloat_AS_DOUBLE(self->basefreq);
    MYFLT *sp = Stream_getData((Stream *)self->spread_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVAmpMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            spread = sp[i];
            spread *= 0.001;
            spread += 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                self->magn[self->overcount][k] = magn[self->overcount][k] * self->table[(int)pos];
                self->freq[self->overcount][k] = freq[self->overcount][k];
                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAmpMod_process_aa(PVAmpMod *self)
{
    int i, k;
    MYFLT bfreq, spread, pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *bf = Stream_getData((Stream *)self->basefreq_stream);
    MYFLT *sp = Stream_getData((Stream *)self->spread_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVAmpMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            bfreq = bf[i];
            spread = sp[i];
            spread *= 0.001;
            spread += 1.0;

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                self->magn[self->overcount][k] = magn[self->overcount][k] * self->table[(int)pos];
                self->freq[self->overcount][k] = freq[self->overcount][k];
                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVAmpMod_setProcMode(PVAmpMod *self)
{
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVAmpMod_process_ii;
            break;

        case 1:
            self->proc_func_ptr = PVAmpMod_process_ai;
            break;

        case 10:
            self->proc_func_ptr = PVAmpMod_process_ia;
            break;

        case 11:
            self->proc_func_ptr = PVAmpMod_process_aa;
            break;
    }
}

static void
PVAmpMod_compute_next_data_frame(PVAmpMod *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVAmpMod_traverse(PVAmpMod *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->basefreq);
    Py_VISIT(self->spread);
    return 0;
}

static int
PVAmpMod_clear(PVAmpMod *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->basefreq);
    Py_CLEAR(self->spread);
    return 0;
}

static void
PVAmpMod_dealloc(PVAmpMod* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->table);
    PyMem_RawFree(self->pointers);
    PyMem_RawFree(self->count);
    PVAmpMod_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVAmpMod_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, shape = 0;
    PyObject *inputtmp, *input_streamtmp, *basefreqtmp = NULL, *spreadtmp = NULL;
    PVAmpMod *self;
    self = (PVAmpMod *)type->tp_alloc(type, 0);

    self->basefreq = PyFloat_FromDouble(1);
    self->spread = PyFloat_FromDouble(0);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVAmpMod_compute_next_data_frame);
    self->mode_func_ptr = PVAmpMod_setProcMode;

    static char *kwlist[] = {"input", "basefreq", "spread", "shape", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOi", kwlist, &inputtmp, &basefreqtmp, &spreadtmp, &shape))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVAmpMod must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (basefreqtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setBasefreq", "O", basefreqtmp);
    }

    if (spreadtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    self->table = (MYFLT *)PyMem_RawRealloc(self->table, 8193 * sizeof(MYFLT));
    PVMod_setTable(self->table, shape);

    PVAmpMod_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVAmpMod_getServer(PVAmpMod* self) { GET_SERVER };
static PyObject * PVAmpMod_getStream(PVAmpMod* self) { GET_STREAM };
static PyObject * PVAmpMod_getPVStream(PVAmpMod* self) { GET_PV_STREAM };

static PyObject * PVAmpMod_play(PVAmpMod *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVAmpMod_stop(PVAmpMod *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVAmpMod_setInput(PVAmpMod *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVAmpMod must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVAmpMod_setBasefreq(PVAmpMod *self, PyObject *arg) { SET_PARAM(self->basefreq, self->basefreq_stream, 0); }
static PyObject * PVAmpMod_setSpread(PVAmpMod *self, PyObject *arg) { SET_PARAM(self->spread, self->spread_stream, 1); }

static PyObject *
PVAmpMod_setShape(PVAmpMod *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        PVMod_setTable(self->table, PyLong_AsLong(arg));
    }

    Py_RETURN_NONE;
}

static PyObject *
PVAmpMod_reset(PVAmpMod *self)
{
    int i;

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    Py_RETURN_NONE;
}

static PyMemberDef PVAmpMod_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVAmpMod, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVAmpMod, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVAmpMod, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVAmpMod, input), 0, NULL},
    {"basefreq", T_OBJECT_EX, offsetof(PVAmpMod, basefreq), 0, NULL},
    {"spread", T_OBJECT_EX, offsetof(PVAmpMod, spread), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVAmpMod_methods[] =
{
    {"getServer", (PyCFunction)PVAmpMod_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVAmpMod_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVAmpMod_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVAmpMod_setInput, METH_O, NULL},
    {"setBasefreq", (PyCFunction)PVAmpMod_setBasefreq, METH_O, NULL},
    {"setSpread", (PyCFunction)PVAmpMod_setSpread, METH_O, NULL},
    {"setShape", (PyCFunction)PVAmpMod_setShape, METH_O, NULL},
    {"reset", (PyCFunction)PVAmpMod_reset, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVAmpMod_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVAmpMod_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVAmpModType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVAmpMod_base",                                   /*tp_name*/
    sizeof(PVAmpMod),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVAmpMod_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVAmpMod_traverse,                  /* tp_traverse */
    (inquiry)PVAmpMod_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVAmpMod_methods,                                 /* tp_methods */
    PVAmpMod_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVAmpMod_new,                                     /* tp_new */
};

/*****************/
/** PVFreqMod **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *basefreq;
    Stream *basefreq_stream;
    PyObject *spread;
    Stream *spread_stream;
    PyObject *depth;
    Stream *depth_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT factor;
    MYFLT *table;
    MYFLT *pointers;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int modebuffer[3];
    int allocated;
    int last_olaps;
} PVFreqMod;

static void
PVFreqMod_realloc_memories(PVFreqMod *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;
    self->factor = 8192.0 / (self->sr / self->hopsize);

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->pointers = (MYFLT *)PyMem_RawRealloc(self->pointers, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVFreqMod_process_ii(PVFreqMod *self)
{
    int i, k, nbin;
    MYFLT bfreq, spread, pos, nfreq, freqPerBin, depth;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    bfreq = PyFloat_AS_DOUBLE(self->basefreq);
    spread = PyFloat_AS_DOUBLE(self->spread);
    spread *= 0.001;
    spread += 1.0;

    if (self->modebuffer[2] == 0)
        depth = PyFloat_AS_DOUBLE(self->depth);
    else
        depth = Stream_getData((Stream *)self->depth_stream)[0];

    if (depth < 0)
        depth = 0.0;
    else if (depth > 1)
        depth = 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFreqMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            freqPerBin = self->sr / self->size;

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                nfreq = freq[self->overcount][k] * (self->table[(int)pos] * depth + 1.0);
                nbin = (int)(nfreq / freqPerBin);

                if (nbin > 0 && nbin < self->hsize)
                {
                    self->magn[self->overcount][nbin] += magn[self->overcount][k];
                    self->freq[self->overcount][nbin] = nfreq;
                }

                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFreqMod_process_ai(PVFreqMod *self)
{
    int i, k, nbin;
    MYFLT bfreq, spread, pos, nfreq, freqPerBin, depth;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *bf = Stream_getData((Stream *)self->basefreq_stream);
    spread = PyFloat_AS_DOUBLE(self->spread);
    spread *= 0.001;
    spread += 1.0;

    if (self->modebuffer[2] == 0)
        depth = PyFloat_AS_DOUBLE(self->depth);
    else
        depth = Stream_getData((Stream *)self->depth_stream)[0];

    if (depth < 0)
        depth = 0.0;
    else if (depth > 1)
        depth = 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFreqMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            bfreq = bf[i];
            freqPerBin = self->sr / self->size;

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                nfreq = freq[self->overcount][k] * (self->table[(int)pos] * depth + 1.0);
                nbin = (int)(nfreq / freqPerBin);

                if (nbin > 0 && nbin < self->hsize)
                {
                    self->magn[self->overcount][nbin] += magn[self->overcount][k];
                    self->freq[self->overcount][nbin] = nfreq;
                }

                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFreqMod_process_ia(PVFreqMod *self)
{
    int i, k, nbin;
    MYFLT bfreq, spread, pos, nfreq, freqPerBin, depth;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    bfreq = PyFloat_AS_DOUBLE(self->basefreq);
    MYFLT *sp = Stream_getData((Stream *)self->spread_stream);

    if (self->modebuffer[2] == 0)
        depth = PyFloat_AS_DOUBLE(self->depth);
    else
        depth = Stream_getData((Stream *)self->depth_stream)[0];

    if (depth < 0)
        depth = 0.0;
    else if (depth > 1)
        depth = 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFreqMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            spread = sp[i];
            spread *= 0.001;
            spread += 1.0;
            freqPerBin = self->sr / self->size;

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                nfreq = freq[self->overcount][k] * (self->table[(int)pos] * depth + 1.0);
                nbin = (int)(nfreq / freqPerBin);

                if (nbin > 0 && nbin < self->hsize)
                {
                    self->magn[self->overcount][nbin] += magn[self->overcount][k];
                    self->freq[self->overcount][nbin] = nfreq;
                }

                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFreqMod_process_aa(PVFreqMod *self)
{
    int i, k, nbin;
    MYFLT bfreq, spread, pos, nfreq, freqPerBin, depth;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);
    MYFLT *bf = Stream_getData((Stream *)self->basefreq_stream);
    MYFLT *sp = Stream_getData((Stream *)self->spread_stream);

    if (self->modebuffer[2] == 0)
        depth = PyFloat_AS_DOUBLE(self->depth);
    else
        depth = Stream_getData((Stream *)self->depth_stream)[0];

    if (depth < 0)
        depth = 0.0;
    else if (depth > 1)
        depth = 1.0;

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVFreqMod_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            bfreq = bf[i];
            spread = sp[i];
            spread *= 0.001;
            spread += 1.0;
            freqPerBin = self->sr / self->size;

            for (k = 0; k < self->hsize; k++)
            {
                self->magn[self->overcount][k] = self->freq[self->overcount][k] = 0.0;
            }

            for (k = 0; k < self->hsize; k++)
            {
                pos = self->pointers[k];
                nfreq = freq[self->overcount][k] * (self->table[(int)pos] * depth + 1.0);
                nbin = (int)(nfreq / freqPerBin);

                if (nbin > 0 && nbin < self->hsize)
                {
                    self->magn[self->overcount][nbin] += magn[self->overcount][k];
                    self->freq[self->overcount][nbin] = nfreq;
                }

                pos += bfreq * MYPOW(spread, k) * self->factor;

                while (pos >= 8192.0)
                    pos -= 8192.0;

                while (pos < 0.0)
                    pos += 8192.0;

                self->pointers[k] = pos;
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVFreqMod_setProcMode(PVFreqMod *self)
{
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PVFreqMod_process_ii;
            break;

        case 1:
            self->proc_func_ptr = PVFreqMod_process_ai;
            break;

        case 10:
            self->proc_func_ptr = PVFreqMod_process_ia;
            break;

        case 11:
            self->proc_func_ptr = PVFreqMod_process_aa;
            break;
    }
}

static void
PVFreqMod_compute_next_data_frame(PVFreqMod *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVFreqMod_traverse(PVFreqMod *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->basefreq);
    Py_VISIT(self->spread);
    Py_VISIT(self->depth);
    return 0;
}

static int
PVFreqMod_clear(PVFreqMod *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->basefreq);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->depth);
    return 0;
}

static void
PVFreqMod_dealloc(PVFreqMod* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->table);
    PyMem_RawFree(self->pointers);
    PyMem_RawFree(self->count);
    PVFreqMod_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVFreqMod_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, shape = 0;
    PyObject *inputtmp, *input_streamtmp, *basefreqtmp = NULL, *spreadtmp = NULL, *depthtmp = NULL;
    PVFreqMod *self;
    self = (PVFreqMod *)type->tp_alloc(type, 0);

    self->basefreq = PyFloat_FromDouble(1);
    self->spread = PyFloat_FromDouble(0);
    self->depth = PyFloat_FromDouble(0.1);
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVFreqMod_compute_next_data_frame);
    self->mode_func_ptr = PVFreqMod_setProcMode;

    static char *kwlist[] = {"input", "basefreq", "spread", "depth", "shape", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOi", kwlist, &inputtmp, &basefreqtmp, &spreadtmp, &depthtmp, &shape))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVFreqMod must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (basefreqtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setBasefreq", "O", basefreqtmp);
    }

    if (spreadtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    if (depthtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setDepth", "O", depthtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    self->table = (MYFLT *)PyMem_RawRealloc(self->table, 8193 * sizeof(MYFLT));
    PVMod_setTable(self->table, shape);

    PVFreqMod_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVFreqMod_getServer(PVFreqMod* self) { GET_SERVER };
static PyObject * PVFreqMod_getStream(PVFreqMod* self) { GET_STREAM };
static PyObject * PVFreqMod_getPVStream(PVFreqMod* self) { GET_PV_STREAM };

static PyObject * PVFreqMod_play(PVFreqMod *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVFreqMod_stop(PVFreqMod *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVFreqMod_setInput(PVFreqMod *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVFreqMod must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVFreqMod_setBasefreq(PVFreqMod *self, PyObject *arg) { SET_PARAM(self->basefreq, self->basefreq_stream, 0); }
static PyObject * PVFreqMod_setSpread(PVFreqMod *self, PyObject *arg) { SET_PARAM(self->spread, self->spread_stream, 1); }
static PyObject * PVFreqMod_setDepth(PVFreqMod *self, PyObject *arg) { SET_PARAM(self->depth, self->depth_stream, 2); }

static PyObject *
PVFreqMod_setShape(PVFreqMod *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        PVMod_setTable(self->table, PyLong_AsLong(arg));
    }

    Py_RETURN_NONE;
}

static PyObject *
PVFreqMod_reset(PVFreqMod *self)
{
    int i;

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    Py_RETURN_NONE;
}

static PyMemberDef PVFreqMod_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVFreqMod, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVFreqMod, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVFreqMod, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVFreqMod, input), 0, NULL},
    {"basefreq", T_OBJECT_EX, offsetof(PVFreqMod, basefreq), 0, NULL},
    {"spread", T_OBJECT_EX, offsetof(PVFreqMod, spread), 0, NULL},
    {"depth", T_OBJECT_EX, offsetof(PVFreqMod, depth), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVFreqMod_methods[] =
{
    {"getServer", (PyCFunction)PVFreqMod_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVFreqMod_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVFreqMod_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVFreqMod_setInput, METH_O, NULL},
    {"setBasefreq", (PyCFunction)PVFreqMod_setBasefreq, METH_O, NULL},
    {"setSpread", (PyCFunction)PVFreqMod_setSpread, METH_O, NULL},
    {"setDepth", (PyCFunction)PVFreqMod_setDepth, METH_O, NULL},
    {"setShape", (PyCFunction)PVFreqMod_setShape, METH_O, NULL},
    {"reset", (PyCFunction)PVFreqMod_reset, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVFreqMod_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVFreqMod_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVFreqModType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVFreqMod_base",                                   /*tp_name*/
    sizeof(PVFreqMod),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVFreqMod_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVFreqMod_traverse,                  /* tp_traverse */
    (inquiry)PVFreqMod_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVFreqMod_methods,                                 /* tp_methods */
    PVFreqMod_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVFreqMod_new,                                     /* tp_new */
};

/*****************/
/** PVBufLoops **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *low;
    Stream *low_stream;
    PyObject *high;
    Stream *high_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    int mode;
    MYFLT last_low;
    MYFLT last_high;
    int last_mode;
    MYFLT length;
    int numFrames;
    MYFLT OneOnNumFrames;
    int framecount;
    MYFLT *speeds;
    MYFLT *pointers;
    MYFLT **magn;
    MYFLT **freq;
    MYFLT **magn_buf;
    MYFLT **freq_buf;
    int *count;
    int modebuffer[2];
    int allocated;
    int last_olaps;
    int last_numFrames;
} PVBufLoops;

static void
PVBufLoops_realloc_memories(PVBufLoops *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->numFrames = (int)(self->length * self->sr / self->hopsize + 0.5);
    self->OneOnNumFrames = 1.0 / self->numFrames;
    self->overcount = 0;
    self->framecount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
        for (i = 0; i < self->last_numFrames; i++)
        {
            PyMem_RawFree(self->magn_buf[i]);
            PyMem_RawFree(self->freq_buf[i]);
        }
    }

    self->speeds = (MYFLT *)PyMem_RawRealloc(self->speeds, self->hsize * sizeof(MYFLT));
    self->pointers = (MYFLT *)PyMem_RawRealloc(self->pointers, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
    {
        self->speeds[i] = 1.0;
        self->pointers[i] = 0.0;
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    self->magn_buf = (MYFLT **)PyMem_RawRealloc(self->magn_buf, self->numFrames * sizeof(MYFLT *));
    self->freq_buf = (MYFLT **)PyMem_RawRealloc(self->freq_buf, self->numFrames * sizeof(MYFLT *));

    for (i = 0; i < self->numFrames; i++)
    {
        self->magn_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn_buf[i][j] = self->freq_buf[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->last_numFrames = self->numFrames;
    self->allocated = 1;
}

static void
PVBufLoops_setSpeeds(PVBufLoops *self, MYFLT low, MYFLT high)
{
    int i;
    MYFLT tmp;

    switch (self->mode)
    {
        case 0: /* linear */
            tmp = (high - low) / self->hsize;

            for (i = 0; i < self->hsize; i++)
                self->speeds[i] = low + i * tmp;

            break;

        case 1: /* exponential */
            tmp = high - low;

            for (i = 0; i < self->hsize; i++)
                self->speeds[i] = low + tmp * MYPOW((float)i / self->hsize, 3.0);

            break;

        case 2: /* logarithmic */
            tmp = high - low;

            for (i = 0; i < self->hsize; i++)
                self->speeds[i] = low + tmp * (1.0 - MYPOW(1.0 - (float)i / self->hsize, 3.0));

            break;

        case 3: /* random uniform */
            tmp = high - low;

            for (i = 0; i < self->hsize; i++)
                self->speeds[i] = RANDOM_UNIFORM * tmp + low;

            break;

        case 4: /* random exponential min */
            for (i = 0; i < self->hsize; i++)
            {
                tmp = -MYLOG(RANDOM_UNIFORM) * 0.05;
                tmp = tmp < 0 ? 0.0 : tmp;
                tmp = tmp > 1.0 ? 1.0 : tmp;
                self->speeds[i] = tmp * (high - low) + low;
            }

            break;

        case 5: /* random exponential max */
            for (i = 0; i < self->hsize; i++)
            {
                tmp = 1.0 - (-MYLOG(RANDOM_UNIFORM) * 0.05);
                tmp = tmp < 0 ? 0.0 : tmp;
                tmp = tmp > 1.0 ? 1.0 : tmp;
                self->speeds[i] = tmp * (high - low) + low;
            }

            break;

        case 6: /* random bi-exponential */
            for (i = 0; i < self->hsize; i++)
            {
                tmp = RANDOM_UNIFORM * 2.0;

                if (tmp > 1.0)
                    tmp = 0.5 * (-MYLOG(2.0 - tmp) * 0.05) + 0.5;
                else
                    tmp = 0.5 * (MYLOG(tmp) * 0.05) + 0.5;

                tmp = tmp < 0 ? 0.0 : tmp;
                tmp = tmp > 1.0 ? 1.0 : tmp;
                self->speeds[i] = tmp * (high - low) + low;
            }

            break;

        default: /* linear */
            tmp = (high - low) / self->hsize;

            for (i = 0; i < self->hsize; i++)
                self->speeds[i] = low + i * tmp;

            break;
    }
}

static void
PVBufLoops_process(PVBufLoops *self)
{
    int i, k, frame;
    MYFLT low, high, pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVBufLoops_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->framecount < self->numFrames)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k];
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k];
                    self->magn[self->overcount][k] = 0.0;
                    self->freq[self->overcount][k] = 0.0;
                }

                self->framecount++;
            }
            else
            {
                if (self->modebuffer[0] == 0)
                    low = PyFloat_AS_DOUBLE(self->low);
                else
                    low = Stream_getData((Stream *)self->low_stream)[i];

                if (self->modebuffer[1] == 0)
                    high = PyFloat_AS_DOUBLE(self->high);
                else
                    high = Stream_getData((Stream *)self->high_stream)[i];

                if (low != self->last_low || high != self->last_high || self->mode != self->last_mode)
                {
                    self->last_low = low;
                    self->last_high = high;
                    self->last_mode = self->mode;
                    PVBufLoops_setSpeeds(self, low, high);
                }

                for (k = 0; k < self->hsize; k++)
                {
                    pos = self->pointers[k];
                    frame = (int)(pos * (self->numFrames - 1));
                    self->magn[self->overcount][k] = self->magn_buf[frame][k];
                    self->freq[self->overcount][k] = self->freq_buf[frame][k];
                    pos += self->OneOnNumFrames * self->speeds[k];

                    if (pos < 0.0)
                        pos += 1.0;
                    else if (pos >= 1.0)
                        pos -= 1.0;

                    self->pointers[k] = pos;
                }

            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVBufLoops_setProcMode(PVBufLoops *self)
{
    self->proc_func_ptr = PVBufLoops_process;
}

static void
PVBufLoops_compute_next_data_frame(PVBufLoops *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVBufLoops_traverse(PVBufLoops *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->low);
    Py_VISIT(self->high);
    return 0;
}

static int
PVBufLoops_clear(PVBufLoops *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->low);
    Py_CLEAR(self->high);
    return 0;
}

static void
PVBufLoops_dealloc(PVBufLoops* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);

    for (i = 0; i < self->numFrames; i++)
    {
        PyMem_RawFree(self->magn_buf[i]);
        PyMem_RawFree(self->freq_buf[i]);
    }

    PyMem_RawFree(self->magn_buf);
    PyMem_RawFree(self->freq_buf);
    PyMem_RawFree(self->count);
    PyMem_RawFree(self->speeds);
    PyMem_RawFree(self->pointers);
    PVBufLoops_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVBufLoops_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *lowtmp = NULL, *hightmp = NULL;
    PVBufLoops *self;
    self = (PVBufLoops *)type->tp_alloc(type, 0);

    self->low = PyFloat_FromDouble(1.0);
    self->high = PyFloat_FromDouble(1.0);
    self->last_low = self->last_high = -1.0;
    self->mode = 0;
    self->last_mode = -1;
    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->numFrames = self->last_numFrames = 0;
    self->allocated = 0;
    self->length = 1.0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVBufLoops_compute_next_data_frame);
    self->mode_func_ptr = PVBufLoops_setProcMode;

    static char *kwlist[] = {"input", "low", "high", "mode", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOIF, kwlist, &inputtmp, &lowtmp, &hightmp, &self->mode, &self->length))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBufLoops must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    if (lowtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setLow", "O", lowtmp);
    }

    if (hightmp)
    {
        PyObject_CallMethod((PyObject *)self, "setHigh", "O", hightmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVBufLoops_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVBufLoops_getServer(PVBufLoops* self) { GET_SERVER };
static PyObject * PVBufLoops_getStream(PVBufLoops* self) { GET_STREAM };
static PyObject * PVBufLoops_getPVStream(PVBufLoops* self) { GET_PV_STREAM };

static PyObject * PVBufLoops_play(PVBufLoops *self, PyObject *args, PyObject *kwds)
{
    int k;

    for (k = 0; k < self->hsize; k++)
        self->pointers[k] = 0.0;

    self->framecount = 0;
    PLAY
};

static PyObject * PVBufLoops_stop(PVBufLoops *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVBufLoops_setInput(PVBufLoops *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBufLoops must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject * PVBufLoops_setLow(PVBufLoops *self, PyObject *arg) { SET_PARAM(self->low, self->low_stream, 0); }
static PyObject * PVBufLoops_setHigh(PVBufLoops *self, PyObject *arg) { SET_PARAM(self->high, self->high_stream, 1); }

static PyObject *
PVBufLoops_setMode(PVBufLoops *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp < 7)
            self->mode = tmp;
    }

    Py_RETURN_NONE;
}

static PyObject *
PVBufLoops_reset(PVBufLoops *self)
{
    int i;

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    Py_RETURN_NONE;
}

static PyMemberDef PVBufLoops_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVBufLoops, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVBufLoops, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVBufLoops, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVBufLoops, input), 0, NULL},
    {"low", T_OBJECT_EX, offsetof(PVBufLoops, low), 0, NULL},
    {"high", T_OBJECT_EX, offsetof(PVBufLoops, high), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVBufLoops_methods[] =
{
    {"getServer", (PyCFunction)PVBufLoops_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVBufLoops_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVBufLoops_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVBufLoops_setInput, METH_O, NULL},
    {"setLow", (PyCFunction)PVBufLoops_setLow, METH_O, NULL},
    {"setHigh", (PyCFunction)PVBufLoops_setHigh, METH_O, NULL},
    {"setMode", (PyCFunction)PVBufLoops_setMode, METH_O, NULL},
    {"reset", (PyCFunction)PVBufLoops_reset, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVBufLoops_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVBufLoops_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVBufLoopsType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVBufLoops_base",                                   /*tp_name*/
    sizeof(PVBufLoops),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVBufLoops_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVBufLoops_traverse,                  /* tp_traverse */
    (inquiry)PVBufLoops_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVBufLoops_methods,                                 /* tp_methods */
    PVBufLoops_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVBufLoops_new,                                     /* tp_new */
};

/*****************/
/** PVBufTabLoops **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PVStream *pv_stream;
    PyObject *speed;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT length;
    int numFrames;
    MYFLT OneOnNumFrames;
    int framecount;
    MYFLT *pointers;
    MYFLT **magn;
    MYFLT **freq;
    MYFLT **magn_buf;
    MYFLT **freq_buf;
    int *count;
    int allocated;
    int last_olaps;
    int last_numFrames;
} PVBufTabLoops;

static void
PVBufTabLoops_realloc_memories(PVBufTabLoops *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->numFrames = (int)(self->length * self->sr / self->hopsize + 0.5);
    self->OneOnNumFrames = 1.0 / self->numFrames;
    self->overcount = 0;
    self->framecount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
        for (i = 0; i < self->last_numFrames; i++)
        {
            PyMem_RawFree(self->magn_buf[i]);
            PyMem_RawFree(self->freq_buf[i]);
        }
    }

    self->pointers = (MYFLT *)PyMem_RawRealloc(self->pointers, self->hsize * sizeof(MYFLT));

    for (i = 0; i < self->hsize; i++)
    {
        self->pointers[i] = 0.0;
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    self->magn_buf = (MYFLT **)PyMem_RawRealloc(self->magn_buf, self->numFrames * sizeof(MYFLT *));
    self->freq_buf = (MYFLT **)PyMem_RawRealloc(self->freq_buf, self->numFrames * sizeof(MYFLT *));

    for (i = 0; i < self->numFrames; i++)
    {
        self->magn_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq_buf[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn_buf[i][j] = self->freq_buf[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->last_numFrames = self->numFrames;
    self->allocated = 1;
}

static void
PVBufTabLoops_process(PVBufTabLoops *self)
{
    int i, k, frame;
    MYFLT pos;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);

    MYFLT *spds = TableStream_getData((TableStream *)self->speed);
    int splen = TableStream_getSize((TableStream *)self->speed);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVBufTabLoops_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            if (self->framecount < self->numFrames)
            {
                for (k = 0; k < self->hsize; k++)
                {
                    self->magn_buf[self->framecount][k] = magn[self->overcount][k];
                    self->freq_buf[self->framecount][k] = freq[self->overcount][k];
                    self->magn[self->overcount][k] = 0.0;
                    self->freq[self->overcount][k] = 0.0;
                }

                self->framecount++;
            }
            else
            {
                for (k = 0; k < self->hsize; k++)
                {
                    pos = self->pointers[k];
                    frame = (int)(pos * (self->numFrames - 1));
                    self->magn[self->overcount][k] = self->magn_buf[frame][k];
                    self->freq[self->overcount][k] = self->freq_buf[frame][k];

                    if (k < splen)
                    {
                        pos += self->OneOnNumFrames * spds[k];

                        if (pos < 0.0)
                            pos += 1.0;
                        else if (pos >= 1.0)
                            pos -= 1.0;
                    }

                    self->pointers[k] = pos;
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVBufTabLoops_setProcMode(PVBufTabLoops *self)
{
    self->proc_func_ptr = PVBufTabLoops_process;
}

static void
PVBufTabLoops_compute_next_data_frame(PVBufTabLoops *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVBufTabLoops_traverse(PVBufTabLoops *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
PVBufTabLoops_clear(PVBufTabLoops *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
PVBufTabLoops_dealloc(PVBufTabLoops* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);

    for (i = 0; i < self->numFrames; i++)
    {
        PyMem_RawFree(self->magn_buf[i]);
        PyMem_RawFree(self->freq_buf[i]);
    }

    PyMem_RawFree(self->magn_buf);
    PyMem_RawFree(self->freq_buf);
    PyMem_RawFree(self->count);
    PyMem_RawFree(self->pointers);
    PVBufTabLoops_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVBufTabLoops_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *speedtmp;
    PVBufTabLoops *self;
    self = (PVBufTabLoops *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    self->numFrames = self->last_numFrames = 0;
    self->length = 1.0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVBufTabLoops_compute_next_data_frame);
    self->mode_func_ptr = PVBufTabLoops_setProcMode;

    static char *kwlist[] = {"input", "speed", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_F, kwlist, &inputtmp, &speedtmp, &self->length))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBufTabLoops must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    Py_XDECREF(self->speed);
    self->speed = PyObject_CallMethod((PyObject *)speedtmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVBufTabLoops_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVBufTabLoops_getServer(PVBufTabLoops* self) { GET_SERVER };
static PyObject * PVBufTabLoops_getStream(PVBufTabLoops* self) { GET_STREAM };
static PyObject * PVBufTabLoops_getPVStream(PVBufTabLoops* self) { GET_PV_STREAM };

static PyObject * PVBufTabLoops_play(PVBufTabLoops *self, PyObject *args, PyObject *kwds)
{
    int k;

    for (k = 0; k < self->hsize; k++)
        self->pointers[k] = 0.0;

    self->framecount = 0;
    PLAY
};

static PyObject * PVBufTabLoops_stop(PVBufTabLoops *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVBufTabLoops_setInput(PVBufTabLoops *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVBufTabLoops must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVBufTabLoops_setSpeed(PVBufTabLoops *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->speed);
    self->speed = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject *
PVBufTabLoops_reset(PVBufTabLoops *self)
{
    int i;

    for (i = 0; i < self->hsize; i++)
        self->pointers[i] = 0.0;

    Py_RETURN_NONE;
}

static PyMemberDef PVBufTabLoops_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVBufTabLoops, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVBufTabLoops, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVBufTabLoops, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVBufTabLoops, input), 0, NULL},
    {"speed", T_OBJECT_EX, offsetof(PVBufTabLoops, speed), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVBufTabLoops_methods[] =
{
    {"getServer", (PyCFunction)PVBufTabLoops_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVBufTabLoops_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVBufTabLoops_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVBufTabLoops_setInput, METH_O, NULL},
    {"setSpeed", (PyCFunction)PVBufTabLoops_setSpeed, METH_O, NULL},
    {"reset", (PyCFunction)PVBufTabLoops_reset, METH_NOARGS, NULL},
    {"play", (PyCFunction)PVBufTabLoops_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVBufTabLoops_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVBufTabLoopsType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVBufTabLoops_base",                                   /*tp_name*/
    sizeof(PVBufTabLoops),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVBufTabLoops_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVBufTabLoops_traverse,                  /* tp_traverse */
    (inquiry)PVBufTabLoops_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVBufTabLoops_methods,                                 /* tp_methods */
    PVBufTabLoops_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVBufTabLoops_new,                                     /* tp_new */
};

/*****************/
/** PVMix **/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PVStream *input_stream;
    PyObject *input2;
    PVStream *input2_stream;
    PVStream *pv_stream;
    int size;
    int olaps;
    int hsize;
    int hopsize;
    int overcount;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
    int allocated;
    int last_olaps;
} PVMix;

static void
PVMix_realloc_memories(PVMix *self)
{
    int i, j, inputLatency;
    self->hsize = self->size / 2;
    self->hopsize = self->size / self->olaps;
    inputLatency = self->size - self->hopsize;
    self->overcount = 0;

    if (self->allocated)
    {
        for (i = 0; i < self->last_olaps; i++)
        {
            PyMem_RawFree(self->magn[i]);
            PyMem_RawFree(self->freq[i]);
        }
    }

    self->magn = (MYFLT **)PyMem_RawRealloc(self->magn, self->olaps * sizeof(MYFLT *));
    self->freq = (MYFLT **)PyMem_RawRealloc(self->freq, self->olaps * sizeof(MYFLT *));

    for (i = 0; i < self->olaps; i++)
    {
        self->magn[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));
        self->freq[i] = (MYFLT *)PyMem_RawMalloc(self->hsize * sizeof(MYFLT));

        for (j = 0; j < self->hsize; j++)
            self->magn[i][j] = self->freq[i][j] = 0.0;
    }

    for (i = 0; i < self->bufsize; i++)
        self->count[i] = inputLatency;

    PVStream_setFFTsize(self->pv_stream, self->size);
    PVStream_setOlaps(self->pv_stream, self->olaps);
    PVStream_setMagn(self->pv_stream, self->magn);
    PVStream_setFreq(self->pv_stream, self->freq);
    PVStream_setCount(self->pv_stream, self->count);

    self->last_olaps = self->olaps;
    self->allocated = 1;
}

static void
PVMix_process_i(PVMix *self)
{
    int i, k;
    MYFLT **magn = PVStream_getMagn((PVStream *)self->input_stream);
    MYFLT **freq = PVStream_getFreq((PVStream *)self->input_stream);
    MYFLT **magn2 = PVStream_getMagn((PVStream *)self->input2_stream);
    MYFLT **freq2 = PVStream_getFreq((PVStream *)self->input2_stream);
    int *count = PVStream_getCount((PVStream *)self->input_stream);
    int size = PVStream_getFFTsize((PVStream *)self->input_stream);
    int olaps = PVStream_getOlaps((PVStream *)self->input_stream);

    if (self->size != size || self->olaps != olaps)
    {
        self->size = size;
        self->olaps = olaps;
        PVMix_realloc_memories(self);
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->count[i] = count[i];

        if (count[i] >= (self->size - 1))
        {
            for (k = 0; k < self->hsize; k++)
            {
                if (magn[self->overcount][k] > magn2[self->overcount][k])
                {
                    self->magn[self->overcount][k] = magn[self->overcount][k];
                    self->freq[self->overcount][k] = freq[self->overcount][k];
                }
                else
                {
                    self->magn[self->overcount][k] = magn2[self->overcount][k];
                    self->freq[self->overcount][k] = freq2[self->overcount][k];
                }
            }

            self->overcount++;

            if (self->overcount >= self->olaps)
                self->overcount = 0;
        }
    }
}

static void
PVMix_setProcMode(PVMix *self)
{
    self->proc_func_ptr = PVMix_process_i;
}

static void
PVMix_compute_next_data_frame(PVMix *self)
{
    (*self->proc_func_ptr)(self);
}

static int
PVMix_traverse(PVMix *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    return 0;
}

static int
PVMix_clear(PVMix *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    return 0;
}

static void
PVMix_dealloc(PVMix* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < self->olaps; i++)
    {
        PyMem_RawFree(self->magn[i]);
        PyMem_RawFree(self->freq[i]);
    }

    PyMem_RawFree(self->magn);
    PyMem_RawFree(self->freq);
    PyMem_RawFree(self->count);
    PVMix_clear(self);
    Py_TYPE(self->pv_stream)->tp_free((PyObject*)self->pv_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PVMix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp;
    PVMix *self;
    self = (PVMix *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->olaps = self->last_olaps = 4;
    self->allocated = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PVMix_compute_next_data_frame);
    self->mode_func_ptr = PVMix_setProcMode;

    static char *kwlist[] = {"input", "input2", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &input2tmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMix must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT_PV_STREAM

    if ( PyObject_HasAttrString((PyObject *)input2tmp, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMix must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    INIT_INPUT2_PV_STREAM

    self->size = PVStream_getFFTsize(self->input_stream);
    self->olaps = PVStream_getOlaps(self->input_stream);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    MAKE_NEW_PV_STREAM(self->pv_stream, &PVStreamType, NULL);

    self->count = (int *)PyMem_RawRealloc(self->count, self->bufsize * sizeof(int));

    PVMix_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PVMix_getServer(PVMix* self) { GET_SERVER };
static PyObject * PVMix_getStream(PVMix* self) { GET_STREAM };
static PyObject * PVMix_getPVStream(PVMix* self) { GET_PV_STREAM };

static PyObject * PVMix_play(PVMix *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PVMix_stop(PVMix *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
PVMix_setInput(PVMix *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument of PVMix must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input);

    self->input = arg;
    Py_INCREF(self->input);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getPVStream", NULL);
    self->input_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input_stream);

    Py_RETURN_NONE;
}

static PyObject *
PVMix_setInput2(PVMix *self, PyObject *arg)
{
    if ( PyObject_HasAttrString((PyObject *)arg, "pv_stream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input2\" argument of PVMix must be a PyoPVObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input2);

    self->input2 = arg;
    Py_INCREF(self->input2);
    PyObject *input_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getPVStream", NULL);
    self->input2_stream = (PVStream *)input_streamtmp;
    Py_INCREF(self->input2_stream);

    Py_RETURN_NONE;
}

static PyMemberDef PVMix_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PVMix, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(PVMix, stream), 0, NULL},
    {"pv_stream", T_OBJECT_EX, offsetof(PVMix, pv_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(PVMix, input), 0, NULL},
    {"input2", T_OBJECT_EX, offsetof(PVMix, input2), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PVMix_methods[] =
{
    {"getServer", (PyCFunction)PVMix_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)PVMix_getStream, METH_NOARGS, NULL},
    {"_getPVStream", (PyCFunction)PVMix_getPVStream, METH_NOARGS, NULL},
    {"setInput", (PyCFunction)PVMix_setInput, METH_O, NULL},
    {"setInput2", (PyCFunction)PVMix_setInput2, METH_O, NULL},
    {"play", (PyCFunction)PVMix_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)PVMix_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PVMixType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PVMix_base",                                   /*tp_name*/
    sizeof(PVMix),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)PVMix_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)PVMix_traverse,                  /* tp_traverse */
    (inquiry)PVMix_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    PVMix_methods,                                 /* tp_methods */
    PVMix_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    PVMix_new,                                     /* tp_new */
};