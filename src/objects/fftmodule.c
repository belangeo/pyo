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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "fft.h"
#include "wind.h"
#include "sndfile.h"

static int
isPowerOfTwo(int x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int size;
    int hsize;
    int hopsize;
    int wintype;
    int incount;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *window;
    MYFLT **twiddle;
    MYFLT *twiddle2;
    MYFLT *buffer_streams;
} FFTMain;

static void
FFTMain_realloc_memories(FFTMain *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++)
        self->inframe[i] = self->outframe[i] = 0.0;
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 3 * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->bufsize*3); i++)
        self->buffer_streams[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->twiddle2 = (MYFLT *)realloc(self->twiddle2, self->size * sizeof(MYFLT));
    fft_compute_radix2_twiddle(self->twiddle2, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);
    self->incount = -self->hopsize;
}

static void
FFTMain_filters(FFTMain *self) {
    int i, incount;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    incount = self->incount;

    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            self->inframe[incount] = in[i] * self->window[incount];
            if (incount < self->hsize) {
                self->buffer_streams[i] = self->outframe[incount];
                if (incount)
                    self->buffer_streams[i+self->bufsize] = self->outframe[self->size - incount];
                else
                    self->buffer_streams[i+self->bufsize] = 0.0;
            }
            else if (incount == self->hsize)
                self->buffer_streams[i] = self->outframe[incount];
            else
                self->buffer_streams[i] = self->buffer_streams[i+self->bufsize] = 0.0;
            self->buffer_streams[i+self->bufsize*2] = (MYFLT)incount;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;
            realfft_split(self->inframe, self->outframe, self->size, self->twiddle);
        }
    }

    /*
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            self->inframe[incount] = in[i] * self->window[incount];
            if (incount < self->hsize) {
                self->buffer_streams[i] = self->outframe[incount*2];
                self->buffer_streams[i+self->bufsize] = self->outframe[incount*2+1];
            }
            else
                self->buffer_streams[i] = self->buffer_streams[i+self->bufsize] = 0.0;
            self->buffer_streams[i+self->bufsize*2] = (MYFLT)incount;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;
            realfft_packed(self->inframe, self->outframe, self->size, self->twiddle2);
        }
    }
    */
    self->incount = incount;
}

MYFLT *
FFTMain_getSamplesBuffer(FFTMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
FFTMain_setProcMode(FFTMain *self)
{
    self->proc_func_ptr = FFTMain_filters;
}

static void
FFTMain_compute_next_data_frame(FFTMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
FFTMain_traverse(FFTMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
FFTMain_clear(FFTMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
FFTMain_dealloc(FFTMain* self)
{
    int i;
    pyo_DEALLOC
    free(self->inframe);
    free(self->outframe);
    free(self->window);
    free(self->buffer_streams);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    free(self->twiddle2);
    FFTMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FFTMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    FFTMain *self;
    self = (FFTMain *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->wintype = 2;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FFTMain_compute_next_data_frame);
    self->mode_func_ptr = FFTMain_setProcMode;

    static char *kwlist[] = {"input", "size", "hopsize", "wintype", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iii", kwlist, &inputtmp, &self->size, &self->hopsize, &self->wintype))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    FFTMain_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FFTMain_getServer(FFTMain* self) { GET_SERVER };
static PyObject * FFTMain_getStream(FFTMain* self) { GET_STREAM };

static PyObject * FFTMain_play(FFTMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FFTMain_stop(FFTMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
FFTMain_setSize(FFTMain *self, PyObject *args, PyObject *kwds)
{
    int size, hopsize;

    static char *kwlist[] = {"size", "hopsize", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &size, &hopsize)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (isPowerOfTwo(size)) {
        self->size = size;
        self->hopsize = hopsize;
        FFTMain_realloc_memories(self);
    }
    else
        PySys_WriteStdout("FFT size must be a power of two!\n");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FFTMain_setWinType(FFTMain *self, PyObject *arg)
{
    if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef FFTMain_members[] = {
{"server", T_OBJECT_EX, offsetof(FFTMain, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(FFTMain, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(FFTMain, input), 0, "FFT sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef FFTMain_methods[] = {
{"getServer", (PyCFunction)FFTMain_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)FFTMain_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)FFTMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)FFTMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setSize", (PyCFunction)FFTMain_setSize, METH_VARARGS|METH_KEYWORDS, "Sets a new FFT size."},
{"setWinType", (PyCFunction)FFTMain_setWinType, METH_O, "Sets a new window."},
{NULL}  /* Sentinel */
};

PyTypeObject FFTMainType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.FFTMain_base",                                   /*tp_name*/
sizeof(FFTMain),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)FFTMain_dealloc,                     /*tp_dealloc*/
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
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"FFTMain objects. FFT transform.",           /* tp_doc */
(traverseproc)FFTMain_traverse,                  /* tp_traverse */
(inquiry)FFTMain_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
FFTMain_methods,                                 /* tp_methods */
FFTMain_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
FFTMain_new,                                     /* tp_new */
};

/************************************************************************************************/
/* FFT streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FFTMain *mainSplitter;
    int modebuffer[2];
    int chnl; // 0 = real, 1 = imag, 2 = bin
} FFT;

static void FFT_postprocessing_ii(FFT *self) { POST_PROCESSING_II };
static void FFT_postprocessing_ai(FFT *self) { POST_PROCESSING_AI };
static void FFT_postprocessing_ia(FFT *self) { POST_PROCESSING_IA };
static void FFT_postprocessing_aa(FFT *self) { POST_PROCESSING_AA };
static void FFT_postprocessing_ireva(FFT *self) { POST_PROCESSING_IREVA };
static void FFT_postprocessing_areva(FFT *self) { POST_PROCESSING_AREVA };
static void FFT_postprocessing_revai(FFT *self) { POST_PROCESSING_REVAI };
static void FFT_postprocessing_revaa(FFT *self) { POST_PROCESSING_REVAA };
static void FFT_postprocessing_revareva(FFT *self) { POST_PROCESSING_REVAREVA };

static void
FFT_setProcMode(FFT *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = FFT_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = FFT_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = FFT_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = FFT_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = FFT_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = FFT_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = FFT_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = FFT_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = FFT_postprocessing_revareva;
            break;
    }
}

static void
FFT_compute_next_data_frame(FFT *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FFTMain_getSamplesBuffer((FFTMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
FFT_traverse(FFT *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
FFT_clear(FFT *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
FFT_dealloc(FFT* self)
{
    pyo_DEALLOC
    FFT_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FFT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    FFT *self;
    self = (FFT *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FFT_compute_next_data_frame);
    self->mode_func_ptr = FFT_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FFTMain *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FFT_getServer(FFT* self) { GET_SERVER };
static PyObject * FFT_getStream(FFT* self) { GET_STREAM };
static PyObject * FFT_setMul(FFT *self, PyObject *arg) { SET_MUL };
static PyObject * FFT_setAdd(FFT *self, PyObject *arg) { SET_ADD };
static PyObject * FFT_setSub(FFT *self, PyObject *arg) { SET_SUB };
static PyObject * FFT_setDiv(FFT *self, PyObject *arg) { SET_DIV };

static PyObject * FFT_play(FFT *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FFT_stop(FFT *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * FFT_multiply(FFT *self, PyObject *arg) { MULTIPLY };
static PyObject * FFT_inplace_multiply(FFT *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FFT_add(FFT *self, PyObject *arg) { ADD };
static PyObject * FFT_inplace_add(FFT *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FFT_sub(FFT *self, PyObject *arg) { SUB };
static PyObject * FFT_inplace_sub(FFT *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FFT_div(FFT *self, PyObject *arg) { DIV };
static PyObject * FFT_inplace_div(FFT *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FFT_members[] = {
{"server", T_OBJECT_EX, offsetof(FFT, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(FFT, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(FFT, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(FFT, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef FFT_methods[] = {
{"getServer", (PyCFunction)FFT_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)FFT_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)FFT_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)FFT_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)FFT_setMul, METH_O, "Sets FFT mul factor."},
{"setAdd", (PyCFunction)FFT_setAdd, METH_O, "Sets FFT add factor."},
{"setSub", (PyCFunction)FFT_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)FFT_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods FFT_as_number = {
(binaryfunc)FFT_add,                      /*nb_add*/
(binaryfunc)FFT_sub,                 /*nb_subtract*/
(binaryfunc)FFT_multiply,                 /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
(binaryfunc)FFT_inplace_add,              /*inplace_add*/
(binaryfunc)FFT_inplace_sub,         /*inplace_subtract*/
(binaryfunc)FFT_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)FFT_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)FFT_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject FFTType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.FFT_base",         /*tp_name*/
sizeof(FFT),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)FFT_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&FFT_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"FFT objects. Reads one band (real, imag or bins) from a FFT transform.",           /* tp_doc */
(traverseproc)FFT_traverse,   /* tp_traverse */
(inquiry)FFT_clear,           /* tp_clear */
0,                       /* tp_richcompare */
0,                       /* tp_weaklistoffset */
0,                       /* tp_iter */
0,                       /* tp_iternext */
FFT_methods,             /* tp_methods */
FFT_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
FFT_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *inreal;
    Stream *inreal_stream;
    PyObject *inimag;
    Stream *inimag_stream;
    int size;
    int hsize;
    int hopsize;
    int wintype;
    int incount;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *window;
    MYFLT **twiddle;
    MYFLT *twiddle2;
    int modebuffer[2];
} IFFT;

static void
IFFT_realloc_memories(IFFT *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++)
        self->inframe[i] = self->outframe[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->twiddle2 = (MYFLT *)realloc(self->twiddle2, self->size * sizeof(MYFLT));
    fft_compute_radix2_twiddle(self->twiddle2, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);
    self->incount = -self->hopsize;
}

static void
IFFT_filters(IFFT *self) {
    int i, incount;
    MYFLT data;
    MYFLT *inreal = Stream_getData((Stream *)self->inreal_stream);
    MYFLT *inimag = Stream_getData((Stream *)self->inimag_stream);

    incount = self->incount;
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            if (incount < self->hsize) {
                self->inframe[incount] = inreal[i];
                if (incount)
                    self->inframe[self->size - incount] = inimag[i];
            }
            else if (incount == self->hsize)
                self->inframe[incount] = inreal[i];
            data = self->outframe[incount] * self->window[incount];
            self->data[i] = data;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;
            irealfft_split(self->inframe, self->outframe, self->size, self->twiddle);
        }
    }
    /*
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            if (incount < self->hsize) {
                self->inframe[incount*2] = inreal[i];
                self->inframe[incount*2+1] = inimag[i];
            }
            self->data[i] = self->outframe[incount] * self->window[incount];
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;
            irealfft_packed(self->inframe, self->outframe, self->size, self->twiddle2);
        }
    }
    */
    self->incount = incount;
}

static void IFFT_postprocessing_ii(IFFT *self) { POST_PROCESSING_II };
static void IFFT_postprocessing_ai(IFFT *self) { POST_PROCESSING_AI };
static void IFFT_postprocessing_ia(IFFT *self) { POST_PROCESSING_IA };
static void IFFT_postprocessing_aa(IFFT *self) { POST_PROCESSING_AA };
static void IFFT_postprocessing_ireva(IFFT *self) { POST_PROCESSING_IREVA };
static void IFFT_postprocessing_areva(IFFT *self) { POST_PROCESSING_AREVA };
static void IFFT_postprocessing_revai(IFFT *self) { POST_PROCESSING_REVAI };
static void IFFT_postprocessing_revaa(IFFT *self) { POST_PROCESSING_REVAA };
static void IFFT_postprocessing_revareva(IFFT *self) { POST_PROCESSING_REVAREVA };

static void
IFFT_setProcMode(IFFT *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IFFT_filters;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = IFFT_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = IFFT_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = IFFT_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = IFFT_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = IFFT_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = IFFT_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = IFFT_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = IFFT_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = IFFT_postprocessing_revareva;
            break;
    }
}

static void
IFFT_compute_next_data_frame(IFFT *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
IFFT_traverse(IFFT *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inreal);
    Py_VISIT(self->inreal_stream);
    Py_VISIT(self->inimag);
    Py_VISIT(self->inimag_stream);
    return 0;
}

static int
IFFT_clear(IFFT *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inreal);
    Py_CLEAR(self->inreal_stream);
    Py_CLEAR(self->inimag);
    Py_CLEAR(self->inimag_stream);
    return 0;
}

static void
IFFT_dealloc(IFFT* self)
{
    int i;
    pyo_DEALLOC
    free(self->inframe);
    free(self->outframe);
    free(self->window);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    free(self->twiddle2);
    IFFT_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
IFFT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inrealtmp, *inreal_streamtmp, *inimagtmp, *inimag_streamtmp, *multmp=NULL, *addtmp=NULL;
    IFFT *self;
    self = (IFFT *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->wintype = 2;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IFFT_compute_next_data_frame);
    self->mode_func_ptr = IFFT_setProcMode;

    static char *kwlist[] = {"inreal", "inimag", "size", "hopsize", "wintype", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iiiOO", kwlist, &inrealtmp, &inimagtmp, &self->size, &self->hopsize, &self->wintype, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->inimag);
    self->inimag = inimagtmp;
    inimag_streamtmp = PyObject_CallMethod((PyObject *)self->inimag, "_getStream", NULL);
    Py_INCREF(inimag_streamtmp);
    Py_XDECREF(self->inimag_stream);
    self->inimag_stream = (Stream *)inimag_streamtmp;

    Py_XDECREF(self->inreal);
    self->inreal = inrealtmp;
    inreal_streamtmp = PyObject_CallMethod((PyObject *)self->inreal, "_getStream", NULL);
    Py_INCREF(inreal_streamtmp);
    Py_XDECREF(self->inreal_stream);
    self->inreal_stream = (Stream *)inreal_streamtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IFFT_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * IFFT_getServer(IFFT* self) { GET_SERVER };
static PyObject * IFFT_getStream(IFFT* self) { GET_STREAM };
static PyObject * IFFT_setMul(IFFT *self, PyObject *arg) { SET_MUL };
static PyObject * IFFT_setAdd(IFFT *self, PyObject *arg) { SET_ADD };
static PyObject * IFFT_setSub(IFFT *self, PyObject *arg) { SET_SUB };
static PyObject * IFFT_setDiv(IFFT *self, PyObject *arg) { SET_DIV };

static PyObject * IFFT_play(IFFT *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IFFT_out(IFFT *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IFFT_stop(IFFT *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * IFFT_multiply(IFFT *self, PyObject *arg) { MULTIPLY };
static PyObject * IFFT_inplace_multiply(IFFT *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IFFT_add(IFFT *self, PyObject *arg) { ADD };
static PyObject * IFFT_inplace_add(IFFT *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IFFT_sub(IFFT *self, PyObject *arg) { SUB };
static PyObject * IFFT_inplace_sub(IFFT *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IFFT_div(IFFT *self, PyObject *arg) { DIV };
static PyObject * IFFT_inplace_div(IFFT *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
IFFT_setSize(IFFT *self, PyObject *args, PyObject *kwds)
{
    int size, hopsize;

    static char *kwlist[] = {"size", "hopsize", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &size, &hopsize)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (isPowerOfTwo(size)) {
        self->size = size;
        self->hopsize = hopsize;
        IFFT_realloc_memories(self);
    }
    else
        PySys_WriteStdout("IFFT size must be a power of two!\n");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
IFFT_setWinType(IFFT *self, PyObject *arg)
{
    if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef IFFT_members[] = {
    {"server", T_OBJECT_EX, offsetof(IFFT, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IFFT, stream), 0, "Stream object."},
    {"inreal", T_OBJECT_EX, offsetof(IFFT, inreal), 0, "Real input."},
    {"inimag", T_OBJECT_EX, offsetof(IFFT, inimag), 0, "Imaginary input."},
    {"mul", T_OBJECT_EX, offsetof(IFFT, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IFFT, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IFFT_methods[] = {
    {"getServer", (PyCFunction)IFFT_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IFFT_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)IFFT_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IFFT_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IFFT_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setSize", (PyCFunction)IFFT_setSize, METH_VARARGS|METH_KEYWORDS, "Sets a new IFFT size."},
    {"setWinType", (PyCFunction)IFFT_setWinType, METH_O, "Sets a new window."},
    {"setMul", (PyCFunction)IFFT_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)IFFT_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)IFFT_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IFFT_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IFFT_as_number = {
    (binaryfunc)IFFT_add,                      /*nb_add*/
    (binaryfunc)IFFT_sub,                 /*nb_subtract*/
    (binaryfunc)IFFT_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)IFFT_inplace_add,              /*inplace_add*/
    (binaryfunc)IFFT_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)IFFT_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)IFFT_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)IFFT_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject IFFTType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.IFFT_base",         /*tp_name*/
    sizeof(IFFT),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)IFFT_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &IFFT_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IFFT objects. Synthesize audio from an FFT real and imaginary parts.",           /* tp_doc */
    (traverseproc)IFFT_traverse,   /* tp_traverse */
    (inquiry)IFFT_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    IFFT_methods,             /* tp_methods */
    IFFT_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    IFFT_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; /* real input */
    Stream *input_stream;
    PyObject *input2; /* imag input */
    Stream *input2_stream;
    int modebuffer[2];
    int chnl; // 0 = mag, 1 = ang
} CarToPol;

static void
CarToPol_generate(CarToPol *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    if (self->chnl == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYSQRT(in[i]*in[i] + in2[i]*in2[i]);
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYATAN2(in2[i], in[i]);
        }
    }
}

static void CarToPol_postprocessing_ii(CarToPol *self) { POST_PROCESSING_II };
static void CarToPol_postprocessing_ai(CarToPol *self) { POST_PROCESSING_AI };
static void CarToPol_postprocessing_ia(CarToPol *self) { POST_PROCESSING_IA };
static void CarToPol_postprocessing_aa(CarToPol *self) { POST_PROCESSING_AA };
static void CarToPol_postprocessing_ireva(CarToPol *self) { POST_PROCESSING_IREVA };
static void CarToPol_postprocessing_areva(CarToPol *self) { POST_PROCESSING_AREVA };
static void CarToPol_postprocessing_revai(CarToPol *self) { POST_PROCESSING_REVAI };
static void CarToPol_postprocessing_revaa(CarToPol *self) { POST_PROCESSING_REVAA };
static void CarToPol_postprocessing_revareva(CarToPol *self) { POST_PROCESSING_REVAREVA };

static void
CarToPol_setProcMode(CarToPol *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = CarToPol_generate;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = CarToPol_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = CarToPol_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = CarToPol_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = CarToPol_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = CarToPol_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = CarToPol_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = CarToPol_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = CarToPol_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = CarToPol_postprocessing_revareva;
            break;
    }
}

static void
CarToPol_compute_next_data_frame(CarToPol *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
CarToPol_traverse(CarToPol *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int
CarToPol_clear(CarToPol *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
CarToPol_dealloc(CarToPol* self)
{
    pyo_DEALLOC
    CarToPol_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CarToPol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp=NULL, *addtmp=NULL;
    CarToPol *self;
    self = (CarToPol *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CarToPol_compute_next_data_frame);
    self->mode_func_ptr = CarToPol_setProcMode;

    static char *kwlist[] = {"input", "input2", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &input2tmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * CarToPol_getServer(CarToPol* self) { GET_SERVER };
static PyObject * CarToPol_getStream(CarToPol* self) { GET_STREAM };
static PyObject * CarToPol_setMul(CarToPol *self, PyObject *arg) { SET_MUL };
static PyObject * CarToPol_setAdd(CarToPol *self, PyObject *arg) { SET_ADD };
static PyObject * CarToPol_setSub(CarToPol *self, PyObject *arg) { SET_SUB };
static PyObject * CarToPol_setDiv(CarToPol *self, PyObject *arg) { SET_DIV };

static PyObject * CarToPol_play(CarToPol *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CarToPol_stop(CarToPol *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * CarToPol_multiply(CarToPol *self, PyObject *arg) { MULTIPLY };
static PyObject * CarToPol_inplace_multiply(CarToPol *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * CarToPol_add(CarToPol *self, PyObject *arg) { ADD };
static PyObject * CarToPol_inplace_add(CarToPol *self, PyObject *arg) { INPLACE_ADD };
static PyObject * CarToPol_sub(CarToPol *self, PyObject *arg) { SUB };
static PyObject * CarToPol_inplace_sub(CarToPol *self, PyObject *arg) { INPLACE_SUB };
static PyObject * CarToPol_div(CarToPol *self, PyObject *arg) { DIV };
static PyObject * CarToPol_inplace_div(CarToPol *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef CarToPol_members[] = {
    {"server", T_OBJECT_EX, offsetof(CarToPol, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(CarToPol, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(CarToPol, input), 0, "Real sound object."},
    {"input2", T_OBJECT_EX, offsetof(CarToPol, input2), 0, "Imaginary sound object."},
    {"mul", T_OBJECT_EX, offsetof(CarToPol, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(CarToPol, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CarToPol_methods[] = {
    {"getServer", (PyCFunction)CarToPol_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)CarToPol_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)CarToPol_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)CarToPol_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)CarToPol_setMul, METH_O, "Sets CarToPol mul factor."},
    {"setAdd", (PyCFunction)CarToPol_setAdd, METH_O, "Sets CarToPol add factor."},
    {"setSub", (PyCFunction)CarToPol_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)CarToPol_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods CarToPol_as_number = {
    (binaryfunc)CarToPol_add,                      /*nb_add*/
    (binaryfunc)CarToPol_sub,                 /*nb_subtract*/
    (binaryfunc)CarToPol_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)CarToPol_inplace_add,              /*inplace_add*/
    (binaryfunc)CarToPol_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)CarToPol_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)CarToPol_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)CarToPol_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject CarToPolType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CarToPol_base",         /*tp_name*/
    sizeof(CarToPol),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CarToPol_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &CarToPol_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "CarToPol objects. Cartesian to polar transform.",           /* tp_doc */
    (traverseproc)CarToPol_traverse,   /* tp_traverse */
    (inquiry)CarToPol_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    CarToPol_methods,             /* tp_methods */
    CarToPol_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    CarToPol_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; /* mag input */
    Stream *input_stream;
    PyObject *input2; /* ang input */
    Stream *input2_stream;
    int modebuffer[2];
    int chnl; // 0 = real, 1 = imag
} PolToCar;

static void
PolToCar_generate(PolToCar *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    if (self->chnl == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i] * MYCOS(in2[i]);
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i] * MYSIN(in2[i]);
        }
    }
}

static void PolToCar_postprocessing_ii(PolToCar *self) { POST_PROCESSING_II };
static void PolToCar_postprocessing_ai(PolToCar *self) { POST_PROCESSING_AI };
static void PolToCar_postprocessing_ia(PolToCar *self) { POST_PROCESSING_IA };
static void PolToCar_postprocessing_aa(PolToCar *self) { POST_PROCESSING_AA };
static void PolToCar_postprocessing_ireva(PolToCar *self) { POST_PROCESSING_IREVA };
static void PolToCar_postprocessing_areva(PolToCar *self) { POST_PROCESSING_AREVA };
static void PolToCar_postprocessing_revai(PolToCar *self) { POST_PROCESSING_REVAI };
static void PolToCar_postprocessing_revaa(PolToCar *self) { POST_PROCESSING_REVAA };
static void PolToCar_postprocessing_revareva(PolToCar *self) { POST_PROCESSING_REVAREVA };

static void
PolToCar_setProcMode(PolToCar *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PolToCar_generate;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = PolToCar_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = PolToCar_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = PolToCar_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = PolToCar_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = PolToCar_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = PolToCar_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = PolToCar_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = PolToCar_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = PolToCar_postprocessing_revareva;
            break;
    }
}

static void
PolToCar_compute_next_data_frame(PolToCar *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
PolToCar_traverse(PolToCar *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int
PolToCar_clear(PolToCar *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
PolToCar_dealloc(PolToCar* self)
{
    pyo_DEALLOC
    PolToCar_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PolToCar_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp=NULL, *addtmp=NULL;
    PolToCar *self;
    self = (PolToCar *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PolToCar_compute_next_data_frame);
    self->mode_func_ptr = PolToCar_setProcMode;

    static char *kwlist[] = {"input", "input2", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &input2tmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * PolToCar_getServer(PolToCar* self) { GET_SERVER };
static PyObject * PolToCar_getStream(PolToCar* self) { GET_STREAM };
static PyObject * PolToCar_setMul(PolToCar *self, PyObject *arg) { SET_MUL };
static PyObject * PolToCar_setAdd(PolToCar *self, PyObject *arg) { SET_ADD };
static PyObject * PolToCar_setSub(PolToCar *self, PyObject *arg) { SET_SUB };
static PyObject * PolToCar_setDiv(PolToCar *self, PyObject *arg) { SET_DIV };

static PyObject * PolToCar_play(PolToCar *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PolToCar_stop(PolToCar *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * PolToCar_multiply(PolToCar *self, PyObject *arg) { MULTIPLY };
static PyObject * PolToCar_inplace_multiply(PolToCar *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PolToCar_add(PolToCar *self, PyObject *arg) { ADD };
static PyObject * PolToCar_inplace_add(PolToCar *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PolToCar_sub(PolToCar *self, PyObject *arg) { SUB };
static PyObject * PolToCar_inplace_sub(PolToCar *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PolToCar_div(PolToCar *self, PyObject *arg) { DIV };
static PyObject * PolToCar_inplace_div(PolToCar *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef PolToCar_members[] = {
    {"server", T_OBJECT_EX, offsetof(PolToCar, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(PolToCar, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(PolToCar, input), 0, "Magnitude sound object."},
    {"input2", T_OBJECT_EX, offsetof(PolToCar, input2), 0, "Angle sound object."},
    {"mul", T_OBJECT_EX, offsetof(PolToCar, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(PolToCar, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef PolToCar_methods[] = {
    {"getServer", (PyCFunction)PolToCar_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)PolToCar_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)PolToCar_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)PolToCar_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)PolToCar_setMul, METH_O, "Sets PolToCar mul factor."},
    {"setAdd", (PyCFunction)PolToCar_setAdd, METH_O, "Sets PolToCar add factor."},
    {"setSub", (PyCFunction)PolToCar_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)PolToCar_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PolToCar_as_number = {
    (binaryfunc)PolToCar_add,                      /*nb_add*/
    (binaryfunc)PolToCar_sub,                 /*nb_subtract*/
    (binaryfunc)PolToCar_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)PolToCar_inplace_add,              /*inplace_add*/
    (binaryfunc)PolToCar_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)PolToCar_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)PolToCar_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)PolToCar_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject PolToCarType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PolToCar_base",         /*tp_name*/
    sizeof(PolToCar),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PolToCar_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &PolToCar_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "PolToCar objects. Polar to cartesian transform.",           /* tp_doc */
    (traverseproc)PolToCar_traverse,   /* tp_traverse */
    (inquiry)PolToCar_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    PolToCar_methods,             /* tp_methods */
    PolToCar_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    PolToCar_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    int inputSize;
    int modebuffer[2];
    int frameSize;
    int overlaps;
    int hopsize;
    int count;
    MYFLT **frameBuffer;
    MYFLT *buffer_streams;
} FrameDeltaMain;

static void
FrameDeltaMain_generate(FrameDeltaMain *self) {
    int i, j, which, where;
    MYFLT curPhase, lastPhase, diff;

    MYFLT ins[self->overlaps][self->bufsize];
    for (j=0; j<self->overlaps; j++) {
        MYFLT *in = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, j), "_getStream", NULL));
        for (i=0; i<self->bufsize; i++) {
            ins[j][i] = in[i];
        }
    }
    for (i=0; i<self->bufsize; i++) {
        for (j=0; j<self->overlaps; j++) {
            curPhase = ins[j][i];
            which = j - 1;
            if (which < 0)
                which = self->overlaps - 1;
            where = self->count - self->hopsize;
            if (where < 0)
                where += self->frameSize;
            lastPhase = self->frameBuffer[which][where];
            diff = curPhase - lastPhase;
            while (diff < -PI) {
                diff += TWOPI;
            }
            while (diff > PI) {
                diff -= TWOPI;
            }
            self->frameBuffer[j][self->count] = curPhase;
            self->buffer_streams[i+j*self->bufsize] = diff;
        }
        self->count++;
        if (self->count >= self->frameSize)
            self->count = 0;
    }
}

MYFLT *
FrameDeltaMain_getSamplesBuffer(FrameDeltaMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
FrameDeltaMain_setProcMode(FrameDeltaMain *self)
{
    self->proc_func_ptr = FrameDeltaMain_generate;
}

static void
FrameDeltaMain_compute_next_data_frame(FrameDeltaMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
FrameDeltaMain_traverse(FrameDeltaMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
FrameDeltaMain_clear(FrameDeltaMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
FrameDeltaMain_dealloc(FrameDeltaMain* self)
{
    int i;
    pyo_DEALLOC
    for (i=0; i<self->overlaps; i++) {
        free(self->frameBuffer[i]);
    }
    free(self->frameBuffer);
    free(self->buffer_streams);
    FrameDeltaMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FrameDeltaMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp;
    FrameDeltaMain *self;
    self = (FrameDeltaMain *)type->tp_alloc(type, 0);

    self->count = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameDeltaMain_compute_next_data_frame);
    self->mode_func_ptr = FrameDeltaMain_setProcMode;

    static char *kwlist[] = {"input", "frameSize", "overlaps", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii", kwlist, &inputtmp, &self->frameSize, &self->overlaps))
        Py_RETURN_NONE;

    if (inputtmp) {
        PyObject_CallMethod((PyObject *)self, "setInput", "O", inputtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->hopsize = self->frameSize / self->overlaps;
    self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
    for(i=0; i<self->overlaps; i++) {
        self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
        for (j=0; j<self->frameSize; j++) {
            self->frameBuffer[i][j] = 0.0;
        }
    }
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->overlaps * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->overlaps*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FrameDeltaMain_getServer(FrameDeltaMain* self) { GET_SERVER };
static PyObject * FrameDeltaMain_getStream(FrameDeltaMain* self) { GET_STREAM };

static PyObject * FrameDeltaMain_play(FrameDeltaMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameDeltaMain_stop(FrameDeltaMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
FrameDeltaMain_setInput(FrameDeltaMain *self, PyObject *arg)
{
    PyObject *tmp;

    if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
        Py_INCREF(Py_None);
        return Py_None;
    }

    tmp = arg;
    self->inputSize = PyList_Size(tmp);
    Py_INCREF(tmp);
    Py_XDECREF(self->input);
    self->input = tmp;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FrameDeltaMain_setFrameSize(FrameDeltaMain *self, PyObject *arg)
{
    int i, j, tmp;

    if (PyInt_Check(arg)) {
        tmp = PyLong_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->frameSize = tmp;
            self->hopsize = self->frameSize / self->overlaps;

            self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
            for(i=0; i<self->overlaps; i++) {
                self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
                for (j=0; j<self->frameSize; j++) {
                    self->frameBuffer[i][j] = 0.0;
                }
            }

            self->count = 0;
        }
    }
    else
        PySys_WriteStdout("frameSize must be a power of two!\n");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef FrameDeltaMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameDeltaMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameDeltaMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(FrameDeltaMain, input), 0, "Phase input object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameDeltaMain_methods[] = {
    {"getServer", (PyCFunction)FrameDeltaMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameDeltaMain_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)FrameDeltaMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)FrameDeltaMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setInput", (PyCFunction)FrameDeltaMain_setInput, METH_O, "Sets list of input streams."},
    {"setFrameSize", (PyCFunction)FrameDeltaMain_setFrameSize, METH_O, "Sets frame size."},
    {NULL}  /* Sentinel */
};

PyTypeObject FrameDeltaMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FrameDeltaMain_base",         /*tp_name*/
    sizeof(FrameDeltaMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameDeltaMain_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameDeltaMain objects. Compute the phase difference between successive frames.",           /* tp_doc */
    (traverseproc)FrameDeltaMain_traverse,   /* tp_traverse */
    (inquiry)FrameDeltaMain_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    FrameDeltaMain_methods,             /* tp_methods */
    FrameDeltaMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    FrameDeltaMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* FrameDelta streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FrameDeltaMain *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} FrameDelta;

static void FrameDelta_postprocessing_ii(FrameDelta *self) { POST_PROCESSING_II };
static void FrameDelta_postprocessing_ai(FrameDelta *self) { POST_PROCESSING_AI };
static void FrameDelta_postprocessing_ia(FrameDelta *self) { POST_PROCESSING_IA };
static void FrameDelta_postprocessing_aa(FrameDelta *self) { POST_PROCESSING_AA };
static void FrameDelta_postprocessing_ireva(FrameDelta *self) { POST_PROCESSING_IREVA };
static void FrameDelta_postprocessing_areva(FrameDelta *self) { POST_PROCESSING_AREVA };
static void FrameDelta_postprocessing_revai(FrameDelta *self) { POST_PROCESSING_REVAI };
static void FrameDelta_postprocessing_revaa(FrameDelta *self) { POST_PROCESSING_REVAA };
static void FrameDelta_postprocessing_revareva(FrameDelta *self) { POST_PROCESSING_REVAREVA };

static void
FrameDelta_setProcMode(FrameDelta *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = FrameDelta_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = FrameDelta_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = FrameDelta_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = FrameDelta_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = FrameDelta_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = FrameDelta_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = FrameDelta_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = FrameDelta_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = FrameDelta_postprocessing_revareva;
            break;
    }
}

static void
FrameDelta_compute_next_data_frame(FrameDelta *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FrameDeltaMain_getSamplesBuffer((FrameDeltaMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
FrameDelta_traverse(FrameDelta *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
FrameDelta_clear(FrameDelta *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
FrameDelta_dealloc(FrameDelta* self)
{
    pyo_DEALLOC
    FrameDelta_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FrameDelta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    FrameDelta *self;
    self = (FrameDelta *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameDelta_compute_next_data_frame);
    self->mode_func_ptr = FrameDelta_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FrameDeltaMain *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FrameDelta_getServer(FrameDelta* self) { GET_SERVER };
static PyObject * FrameDelta_getStream(FrameDelta* self) { GET_STREAM };
static PyObject * FrameDelta_setMul(FrameDelta *self, PyObject *arg) { SET_MUL };
static PyObject * FrameDelta_setAdd(FrameDelta *self, PyObject *arg) { SET_ADD };
static PyObject * FrameDelta_setSub(FrameDelta *self, PyObject *arg) { SET_SUB };
static PyObject * FrameDelta_setDiv(FrameDelta *self, PyObject *arg) { SET_DIV };

static PyObject * FrameDelta_play(FrameDelta *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameDelta_out(FrameDelta *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FrameDelta_stop(FrameDelta *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * FrameDelta_multiply(FrameDelta *self, PyObject *arg) { MULTIPLY };
static PyObject * FrameDelta_inplace_multiply(FrameDelta *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FrameDelta_add(FrameDelta *self, PyObject *arg) { ADD };
static PyObject * FrameDelta_inplace_add(FrameDelta *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FrameDelta_sub(FrameDelta *self, PyObject *arg) { SUB };
static PyObject * FrameDelta_inplace_sub(FrameDelta *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FrameDelta_div(FrameDelta *self, PyObject *arg) { DIV };
static PyObject * FrameDelta_inplace_div(FrameDelta *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FrameDelta_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameDelta, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameDelta, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(FrameDelta, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(FrameDelta, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameDelta_methods[] = {
    {"getServer", (PyCFunction)FrameDelta_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameDelta_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)FrameDelta_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)FrameDelta_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)FrameDelta_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)FrameDelta_setMul, METH_O, "Sets FrameDelta mul factor."},
    {"setAdd", (PyCFunction)FrameDelta_setAdd, METH_O, "Sets FrameDelta add factor."},
    {"setSub", (PyCFunction)FrameDelta_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)FrameDelta_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FrameDelta_as_number = {
    (binaryfunc)FrameDelta_add,                      /*nb_add*/
    (binaryfunc)FrameDelta_sub,                 /*nb_subtract*/
    (binaryfunc)FrameDelta_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)FrameDelta_inplace_add,              /*inplace_add*/
    (binaryfunc)FrameDelta_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)FrameDelta_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)FrameDelta_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)FrameDelta_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject FrameDeltaType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FrameDelta_base",         /*tp_name*/
    sizeof(FrameDelta),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameDelta_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &FrameDelta_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameDelta objects. Reads one band from a FrameDeltaMain object.",           /* tp_doc */
    (traverseproc)FrameDelta_traverse,   /* tp_traverse */
    (inquiry)FrameDelta_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    FrameDelta_methods,             /* tp_methods */
    FrameDelta_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    FrameDelta_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    int inputSize;
    int modebuffer[2];
    int frameSize;
    int overlaps;
    int hopsize;
    int count;
    MYFLT **frameBuffer;
    MYFLT *buffer_streams;
} FrameAccumMain;

static void
FrameAccumMain_generate(FrameAccumMain *self) {
    int i, j, which, where;
    MYFLT curPhase, lastPhase, diff;

    MYFLT ins[self->overlaps][self->bufsize];
    for (j=0; j<self->overlaps; j++) {
        MYFLT *in = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, j), "_getStream", NULL));
        for (i=0; i<self->bufsize; i++) {
            ins[j][i] = in[i];
        }
    }
    for (i=0; i<self->bufsize; i++) {
        for (j=0; j<self->overlaps; j++) {
            curPhase = ins[j][i];
            which = j - 1;
            if (which < 0)
                which = self->overlaps - 1;
            where = self->count - self->hopsize;
            if (where < 0)
                where += self->frameSize;
            lastPhase = self->frameBuffer[which][where];
            diff = curPhase + lastPhase;
            self->frameBuffer[j][self->count] = diff;
            self->buffer_streams[i+j*self->bufsize] = diff;
        }
        self->count++;
        if (self->count >= self->frameSize)
            self->count = 0;
    }
}

MYFLT *
FrameAccumMain_getSamplesBuffer(FrameAccumMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
FrameAccumMain_setProcMode(FrameAccumMain *self)
{
    self->proc_func_ptr = FrameAccumMain_generate;
}

static void
FrameAccumMain_compute_next_data_frame(FrameAccumMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
FrameAccumMain_traverse(FrameAccumMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
FrameAccumMain_clear(FrameAccumMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
FrameAccumMain_dealloc(FrameAccumMain* self)
{
    int i;
    pyo_DEALLOC
    for (i=0; i<self->overlaps; i++) {
        free(self->frameBuffer[i]);
    }
    free(self->frameBuffer);
    free(self->buffer_streams);
    FrameAccumMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FrameAccumMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp;
    FrameAccumMain *self;
    self = (FrameAccumMain *)type->tp_alloc(type, 0);

    self->count = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameAccumMain_compute_next_data_frame);
    self->mode_func_ptr = FrameAccumMain_setProcMode;

    static char *kwlist[] = {"input", "framesize", "overlaps", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii", kwlist, &inputtmp, &self->frameSize, &self->overlaps))
        Py_RETURN_NONE;

    if (inputtmp) {
        PyObject_CallMethod((PyObject *)self, "setInput", "O", inputtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->hopsize = self->frameSize / self->overlaps;
    self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
    for(i=0; i<self->overlaps; i++) {
        self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
        for (j=0; j<self->frameSize; j++) {
            self->frameBuffer[i][j] = 0.0;
        }
    }
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->overlaps * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->overlaps*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FrameAccumMain_getServer(FrameAccumMain* self) { GET_SERVER };
static PyObject * FrameAccumMain_getStream(FrameAccumMain* self) { GET_STREAM };

static PyObject * FrameAccumMain_play(FrameAccumMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameAccumMain_stop(FrameAccumMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
FrameAccumMain_setInput(FrameAccumMain *self, PyObject *arg)
{
    PyObject *tmp;

    if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
        Py_INCREF(Py_None);
        return Py_None;
    }

    tmp = arg;
    self->inputSize = PyList_Size(tmp);
    Py_INCREF(tmp);
    Py_XDECREF(self->input);
    self->input = tmp;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
FrameAccumMain_setFrameSize(FrameAccumMain *self, PyObject *arg)
{
    int i, j, tmp;

    if (PyInt_Check(arg)) {
        tmp = PyLong_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->frameSize = tmp;
            self->hopsize = self->frameSize / self->overlaps;

            self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
            for(i=0; i<self->overlaps; i++) {
                self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
                for (j=0; j<self->frameSize; j++) {
                    self->frameBuffer[i][j] = 0.0;
                }
            }

            self->count = 0;
        }
    }
    else
        PySys_WriteStdout("frameSize must be a power of two!\n");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef FrameAccumMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameAccumMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameAccumMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(FrameAccumMain, input), 0, "Phase input object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameAccumMain_methods[] = {
    {"getServer", (PyCFunction)FrameAccumMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameAccumMain_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)FrameAccumMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)FrameAccumMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setInput", (PyCFunction)FrameAccumMain_setInput, METH_O, "Sets list of input streams."},
    {"setFrameSize", (PyCFunction)FrameAccumMain_setFrameSize, METH_O, "Sets frame size."},
    {NULL}  /* Sentinel */
};

PyTypeObject FrameAccumMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FrameAccumMain_base",         /*tp_name*/
    sizeof(FrameAccumMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameAccumMain_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameAccumMain objects. Integrate the phase difference between successive frames.",           /* tp_doc */
    (traverseproc)FrameAccumMain_traverse,   /* tp_traverse */
    (inquiry)FrameAccumMain_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    FrameAccumMain_methods,             /* tp_methods */
    FrameAccumMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    FrameAccumMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* FrameAccum streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FrameAccumMain *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} FrameAccum;

static void FrameAccum_postprocessing_ii(FrameAccum *self) { POST_PROCESSING_II };
static void FrameAccum_postprocessing_ai(FrameAccum *self) { POST_PROCESSING_AI };
static void FrameAccum_postprocessing_ia(FrameAccum *self) { POST_PROCESSING_IA };
static void FrameAccum_postprocessing_aa(FrameAccum *self) { POST_PROCESSING_AA };
static void FrameAccum_postprocessing_ireva(FrameAccum *self) { POST_PROCESSING_IREVA };
static void FrameAccum_postprocessing_areva(FrameAccum *self) { POST_PROCESSING_AREVA };
static void FrameAccum_postprocessing_revai(FrameAccum *self) { POST_PROCESSING_REVAI };
static void FrameAccum_postprocessing_revaa(FrameAccum *self) { POST_PROCESSING_REVAA };
static void FrameAccum_postprocessing_revareva(FrameAccum *self) { POST_PROCESSING_REVAREVA };

static void
FrameAccum_setProcMode(FrameAccum *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = FrameAccum_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = FrameAccum_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = FrameAccum_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = FrameAccum_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = FrameAccum_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = FrameAccum_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = FrameAccum_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = FrameAccum_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = FrameAccum_postprocessing_revareva;
            break;
    }
}

static void
FrameAccum_compute_next_data_frame(FrameAccum *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FrameAccumMain_getSamplesBuffer((FrameAccumMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
FrameAccum_traverse(FrameAccum *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
FrameAccum_clear(FrameAccum *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
FrameAccum_dealloc(FrameAccum* self)
{
    pyo_DEALLOC
    FrameAccum_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FrameAccum_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    FrameAccum *self;
    self = (FrameAccum *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameAccum_compute_next_data_frame);
    self->mode_func_ptr = FrameAccum_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FrameAccumMain *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * FrameAccum_getServer(FrameAccum* self) { GET_SERVER };
static PyObject * FrameAccum_getStream(FrameAccum* self) { GET_STREAM };
static PyObject * FrameAccum_setMul(FrameAccum *self, PyObject *arg) { SET_MUL };
static PyObject * FrameAccum_setAdd(FrameAccum *self, PyObject *arg) { SET_ADD };
static PyObject * FrameAccum_setSub(FrameAccum *self, PyObject *arg) { SET_SUB };
static PyObject * FrameAccum_setDiv(FrameAccum *self, PyObject *arg) { SET_DIV };

static PyObject * FrameAccum_play(FrameAccum *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameAccum_out(FrameAccum *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FrameAccum_stop(FrameAccum *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * FrameAccum_multiply(FrameAccum *self, PyObject *arg) { MULTIPLY };
static PyObject * FrameAccum_inplace_multiply(FrameAccum *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FrameAccum_add(FrameAccum *self, PyObject *arg) { ADD };
static PyObject * FrameAccum_inplace_add(FrameAccum *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FrameAccum_sub(FrameAccum *self, PyObject *arg) { SUB };
static PyObject * FrameAccum_inplace_sub(FrameAccum *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FrameAccum_div(FrameAccum *self, PyObject *arg) { DIV };
static PyObject * FrameAccum_inplace_div(FrameAccum *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FrameAccum_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameAccum, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameAccum, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(FrameAccum, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(FrameAccum, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameAccum_methods[] = {
    {"getServer", (PyCFunction)FrameAccum_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameAccum_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)FrameAccum_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)FrameAccum_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)FrameAccum_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)FrameAccum_setMul, METH_O, "Sets FrameAccum mul factor."},
    {"setAdd", (PyCFunction)FrameAccum_setAdd, METH_O, "Sets FrameAccum add factor."},
    {"setSub", (PyCFunction)FrameAccum_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)FrameAccum_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FrameAccum_as_number = {
    (binaryfunc)FrameAccum_add,                      /*nb_add*/
    (binaryfunc)FrameAccum_sub,                 /*nb_subtract*/
    (binaryfunc)FrameAccum_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)FrameAccum_inplace_add,              /*inplace_add*/
    (binaryfunc)FrameAccum_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)FrameAccum_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)FrameAccum_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)FrameAccum_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject FrameAccumType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FrameAccum_base",         /*tp_name*/
    sizeof(FrameAccum),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameAccum_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &FrameAccum_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameAccum objects. Reads one band from a FrameAccumMain object.",           /* tp_doc */
    (traverseproc)FrameAccum_traverse,   /* tp_traverse */
    (inquiry)FrameAccum_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    FrameAccum_methods,             /* tp_methods */
    FrameAccum_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    FrameAccum_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    PyObject *up;
    Stream *up_stream;
    PyObject *down;
    Stream *down_stream;
    PyObject *damp;
    Stream *damp_stream;
    int inputSize;
    int modebuffer[5];
    int frameSize;
    int overlaps;
    int hopsize;
    int count;
    MYFLT **frameBuffer;
    MYFLT *buffer_streams;
} VectralMain;

static void
VectralMain_generate(VectralMain *self) {
    int i, j, which, where, bin, halfSize;
    MYFLT curMag, lastMag, diff, slope, up, down, damp;

    halfSize = self->frameSize / 2;

    if (self->modebuffer[2] == 0)
        up = PyFloat_AS_DOUBLE(self->up);
    else
        up = Stream_getData((Stream *)self->up_stream)[0];
    if (up < 0.001)
        up = 0.001;
    else if (up > 1.0)
        up = 1.0;
    up = MYPOW(up, 4.0);

    if (self->modebuffer[3] == 0)
        down = PyFloat_AS_DOUBLE(self->down);
    else
        down = Stream_getData((Stream *)self->down_stream)[0];
    if (down < 0.001)
        down = 0.001;
    else if (down > 1.0)
        down = 1.0;
    down = MYPOW(down, 4.0);

    if (self->modebuffer[4] == 0)
        damp = PyFloat_AS_DOUBLE(self->damp);
    else
        damp = Stream_getData((Stream *)self->damp_stream)[0];
    if (damp < 0.)
        damp = 0.;
    else if (damp > 1.0)
        damp = 1.0;
    damp = damp * 0.1 + 0.9;

    MYFLT ins[self->overlaps][self->bufsize];
    for (j=0; j<self->overlaps; j++) {
        MYFLT *in = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, j), "_getStream", NULL));
        for (i=0; i<self->bufsize; i++) {
            ins[j][i] = in[i];
        }
    }
    for (i=0; i<self->bufsize; i++) {
        for (j=0; j<self->overlaps; j++) {
            which = j - 1;
            if (which < 0)
                which = self->overlaps - 1;
            where = self->count - self->hopsize;
            if (where < 0)
                where += self->frameSize;
            bin = self->count - (self->hopsize * j);
            if (bin < 0)
                bin += self->frameSize;
            slope = MYPOW(damp, (MYFLT)(bin % halfSize));
            curMag = ins[j][i] * slope;
            lastMag = self->frameBuffer[which][where];
            diff = curMag - lastMag;
            if (diff < 0.0)
                curMag = curMag * down + lastMag * (1.0 - down);
            else if (diff >= 0.0)
                curMag = curMag * up + lastMag * (1.0 - up);
            self->frameBuffer[j][self->count] = curMag;
            self->buffer_streams[i+j*self->bufsize] = curMag;
        }
        self->count++;
        if (self->count >= self->frameSize)
            self->count = 0;
    }
}

MYFLT *
VectralMain_getSamplesBuffer(VectralMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
VectralMain_setProcMode(VectralMain *self)
{
    self->proc_func_ptr = VectralMain_generate;
}

static void
VectralMain_compute_next_data_frame(VectralMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
VectralMain_traverse(VectralMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->up);
    Py_VISIT(self->up_stream);
    Py_VISIT(self->down);
    Py_VISIT(self->down_stream);
    Py_VISIT(self->damp);
    Py_VISIT(self->damp_stream);
    return 0;
}

static int
VectralMain_clear(VectralMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->up);
    Py_CLEAR(self->up_stream);
    Py_CLEAR(self->down);
    Py_CLEAR(self->down_stream);
    Py_CLEAR(self->damp);
    Py_CLEAR(self->damp_stream);
    return 0;
}

static void
VectralMain_dealloc(VectralMain* self)
{
    int i;
    pyo_DEALLOC
    for (i=0; i<self->overlaps; i++) {
        free(self->frameBuffer[i]);
    }
    free(self->frameBuffer);
    free(self->buffer_streams);
    VectralMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
VectralMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp, *uptmp=NULL, *downtmp=NULL, *damptmp=NULL;
    VectralMain *self;
    self = (VectralMain *)type->tp_alloc(type, 0);

    self->up = PyFloat_FromDouble(1.0);
    self->down = PyFloat_FromDouble(0.7);
    self->damp = PyFloat_FromDouble(0.9);
    self->count = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;
    self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, VectralMain_compute_next_data_frame);
    self->mode_func_ptr = VectralMain_setProcMode;

    static char *kwlist[] = {"input", "frameSize", "overlaps", "up", "down", "damp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii|OOO", kwlist, &inputtmp, &self->frameSize, &self->overlaps, &uptmp, &downtmp, &damptmp))
        Py_RETURN_NONE;

    if (inputtmp) {
        PyObject_CallMethod((PyObject *)self, "setInput", "O", inputtmp);
    }

    if (uptmp) {
        PyObject_CallMethod((PyObject *)self, "setUp", "O", uptmp);
    }

    if (downtmp) {
        PyObject_CallMethod((PyObject *)self, "setDown", "O", downtmp);
    }

    if (damptmp) {
        PyObject_CallMethod((PyObject *)self, "setDamp", "O", damptmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->hopsize = self->frameSize / self->overlaps;
    self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
    for(i=0; i<self->overlaps; i++) {
        self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
        for (j=0; j<self->frameSize; j++) {
            self->frameBuffer[i][j] = 0.0;
        }
    }
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->overlaps * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->overlaps*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * VectralMain_getServer(VectralMain* self) { GET_SERVER };
static PyObject * VectralMain_getStream(VectralMain* self) { GET_STREAM };

static PyObject * VectralMain_play(VectralMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * VectralMain_stop(VectralMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
VectralMain_setInput(VectralMain *self, PyObject *arg)
{
    PyObject *tmp;

    if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
        Py_INCREF(Py_None);
        return Py_None;
    }

    tmp = arg;
    self->inputSize = PyList_Size(tmp);
    Py_INCREF(tmp);
    Py_XDECREF(self->input);
    self->input = tmp;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
VectralMain_setFrameSize(VectralMain *self, PyObject *arg)
{
    int i, j, tmp;

    if (PyInt_Check(arg)) {
        tmp = PyLong_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->frameSize = tmp;
            self->hopsize = self->frameSize / self->overlaps;

            self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT *));
            for(i=0; i<self->overlaps; i++) {
                self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
                for (j=0; j<self->frameSize; j++) {
                    self->frameBuffer[i][j] = 0.0;
                }
            }

            self->count = 0;
        }
    }
    else
        PySys_WriteStdout("frameSize must be a power of two!\n");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
VectralMain_setUp(VectralMain *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->up);
    if (isNumber == 1) {
        self->up = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else {
        self->up = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->up, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->up_stream);
        self->up_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
VectralMain_setDown(VectralMain *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->down);
    if (isNumber == 1) {
        self->down = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
    }
    else {
        self->down = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->down, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->down_stream);
        self->down_stream = (Stream *)streamtmp;
        self->modebuffer[3] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
VectralMain_setDamp(VectralMain *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->damp);
    if (isNumber == 1) {
        self->damp = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
    }
    else {
        self->damp = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->damp, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->damp_stream);
        self->damp_stream = (Stream *)streamtmp;
        self->modebuffer[4] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef VectralMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(VectralMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(VectralMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(VectralMain, input), 0, "Phase input object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef VectralMain_methods[] = {
    {"getServer", (PyCFunction)VectralMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)VectralMain_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)VectralMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)VectralMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setInput", (PyCFunction)VectralMain_setInput, METH_O, "Sets list of input streams."},
    {"setFrameSize", (PyCFunction)VectralMain_setFrameSize, METH_O, "Sets frame size."},
    {"setUp", (PyCFunction)VectralMain_setUp, METH_O, "Sets clipping upward factor."},
    {"setDown", (PyCFunction)VectralMain_setDown, METH_O, "Sets clipping downward factor."},
    {"setDamp", (PyCFunction)VectralMain_setDamp, METH_O, "Sets high frequencies damping factor."},
    {NULL}  /* Sentinel */
};

PyTypeObject VectralMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.VectralMain_base",         /*tp_name*/
    sizeof(VectralMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)VectralMain_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "VectralMain objects. Smoothing between successive magnitude FFT frames.",           /* tp_doc */
    (traverseproc)VectralMain_traverse,   /* tp_traverse */
    (inquiry)VectralMain_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    VectralMain_methods,             /* tp_methods */
    VectralMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    VectralMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* Vectral streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    VectralMain *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} Vectral;

static void Vectral_postprocessing_ii(Vectral *self) { POST_PROCESSING_II };
static void Vectral_postprocessing_ai(Vectral *self) { POST_PROCESSING_AI };
static void Vectral_postprocessing_ia(Vectral *self) { POST_PROCESSING_IA };
static void Vectral_postprocessing_aa(Vectral *self) { POST_PROCESSING_AA };
static void Vectral_postprocessing_ireva(Vectral *self) { POST_PROCESSING_IREVA };
static void Vectral_postprocessing_areva(Vectral *self) { POST_PROCESSING_AREVA };
static void Vectral_postprocessing_revai(Vectral *self) { POST_PROCESSING_REVAI };
static void Vectral_postprocessing_revaa(Vectral *self) { POST_PROCESSING_REVAA };
static void Vectral_postprocessing_revareva(Vectral *self) { POST_PROCESSING_REVAREVA };

static void
Vectral_setProcMode(Vectral *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Vectral_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Vectral_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Vectral_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Vectral_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Vectral_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Vectral_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Vectral_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Vectral_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Vectral_postprocessing_revareva;
            break;
    }
}

static void
Vectral_compute_next_data_frame(Vectral *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = VectralMain_getSamplesBuffer((VectralMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Vectral_traverse(Vectral *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
Vectral_clear(Vectral *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
Vectral_dealloc(Vectral* self)
{
    pyo_DEALLOC
    Vectral_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Vectral_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    Vectral *self;
    self = (Vectral *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Vectral_compute_next_data_frame);
    self->mode_func_ptr = Vectral_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (VectralMain *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Vectral_getServer(Vectral* self) { GET_SERVER };
static PyObject * Vectral_getStream(Vectral* self) { GET_STREAM };
static PyObject * Vectral_setMul(Vectral *self, PyObject *arg) { SET_MUL };
static PyObject * Vectral_setAdd(Vectral *self, PyObject *arg) { SET_ADD };
static PyObject * Vectral_setSub(Vectral *self, PyObject *arg) { SET_SUB };
static PyObject * Vectral_setDiv(Vectral *self, PyObject *arg) { SET_DIV };

static PyObject * Vectral_play(Vectral *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Vectral_out(Vectral *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Vectral_stop(Vectral *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Vectral_multiply(Vectral *self, PyObject *arg) { MULTIPLY };
static PyObject * Vectral_inplace_multiply(Vectral *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Vectral_add(Vectral *self, PyObject *arg) { ADD };
static PyObject * Vectral_inplace_add(Vectral *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Vectral_sub(Vectral *self, PyObject *arg) { SUB };
static PyObject * Vectral_inplace_sub(Vectral *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Vectral_div(Vectral *self, PyObject *arg) { DIV };
static PyObject * Vectral_inplace_div(Vectral *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Vectral_members[] = {
    {"server", T_OBJECT_EX, offsetof(Vectral, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Vectral, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Vectral, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Vectral, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Vectral_methods[] = {
    {"getServer", (PyCFunction)Vectral_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Vectral_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Vectral_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Vectral_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Vectral_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Vectral_setMul, METH_O, "Sets Vectral mul factor."},
    {"setAdd", (PyCFunction)Vectral_setAdd, METH_O, "Sets Vectral add factor."},
    {"setSub", (PyCFunction)Vectral_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Vectral_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Vectral_as_number = {
    (binaryfunc)Vectral_add,                      /*nb_add*/
    (binaryfunc)Vectral_sub,                 /*nb_subtract*/
    (binaryfunc)Vectral_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)Vectral_inplace_add,              /*inplace_add*/
    (binaryfunc)Vectral_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Vectral_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Vectral_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Vectral_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject VectralType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Vectral_base",         /*tp_name*/
    sizeof(Vectral),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Vectral_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Vectral_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "Vectral objects. Reads one band from a VectralMain object.",           /* tp_doc */
    (traverseproc)Vectral_traverse,   /* tp_traverse */
    (inquiry)Vectral_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Vectral_methods,             /* tp_methods */
    Vectral_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Vectral_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *bal;
    Stream *bal_stream;
    char *impulse_path;
    int chnl;
    int size;
    int size2;
    int hsize;
    int incount;
    int num_iter;
    int current_iter;
    int impulse_len;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *last_half_frame;
    MYFLT **twiddle;
    MYFLT *input_buffer;
    MYFLT *output_buffer;
    MYFLT **impulse_real;
    MYFLT **impulse_imag;
    MYFLT **accum_real;
    MYFLT **accum_imag;
    MYFLT *real;
    MYFLT *imag;
    int modebuffer[3];
} CvlVerb;

static void
CvlVerb_alloc_memories(CvlVerb *self) {
    int i, n8;
    self->hsize = self->size / 2;
    self->size2 = self->size * 2;
    n8 = self->size2 >> 3;
    self->real = (MYFLT *)realloc(self->real, self->size * sizeof(MYFLT));
    self->imag = (MYFLT *)realloc(self->imag, self->size * sizeof(MYFLT));
    self->inframe = (MYFLT *)realloc(self->inframe, self->size2 * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size2 * sizeof(MYFLT));
    self->last_half_frame = (MYFLT *)realloc(self->last_half_frame, self->size * sizeof(MYFLT));
    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->size * sizeof(MYFLT));
    self->output_buffer = (MYFLT *)realloc(self->output_buffer, self->size2 * sizeof(MYFLT));
    for (i=0; i<self->size2; i++)
        self->inframe[i] = self->outframe[i] = self->output_buffer[i] = 0.0;
    for (i=0; i<self->size; i++)
        self->last_half_frame[i] = self->input_buffer[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size2);
}

static void
CvlVerb_analyse_impulse(CvlVerb *self) {
    SNDFILE *sf;
    SF_INFO info;
    int i, j, snd_size, snd_sr, snd_chnls, num_items, num;
    MYFLT *tmp, *tmp2, *inframe, *outframe;

    info.format = 0;
    sf = sf_open(self->impulse_path, SFM_READ, &info);
    if (sf == NULL) {
        PySys_WriteStdout("CvlVerb failed to open the impulse file %s.\n", self->impulse_path);
        return;
    }
    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;

    if (snd_sr != self->sr) {
        PySys_WriteStdout("CvlVerb warning: Impulse sampling rate does't match the sampling rate of the server.\n");
    }

    self->num_iter = (int)MYCEIL((MYFLT)snd_size / self->size);
    self->impulse_len = self->num_iter * self->size;

    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    tmp2 = (MYFLT *)malloc(self->impulse_len * sizeof(MYFLT));

    sf_seek(sf, 0, SEEK_SET);
    num = SF_READ(sf, tmp, num_items);
    sf_close(sf);
    for (i=0; i<snd_size; i++) {
        tmp2[i] = tmp[i*snd_chnls+self->chnl];
    }
    for (i=snd_size; i<self->impulse_len; i++) {
        tmp2[i] = 0.0;
    }

    self->impulse_real = (MYFLT **)realloc(self->impulse_real, self->num_iter * sizeof(MYFLT *));
    self->impulse_imag = (MYFLT **)realloc(self->impulse_imag, self->num_iter * sizeof(MYFLT *));
    self->accum_real = (MYFLT **)realloc(self->accum_real, self->num_iter * sizeof(MYFLT *));
    self->accum_imag = (MYFLT **)realloc(self->accum_imag, self->num_iter * sizeof(MYFLT *));
    for(i=0; i<self->num_iter; i++) {
        self->impulse_real[i] = (MYFLT *)malloc(self->size * sizeof(MYFLT));
        self->impulse_imag[i] = (MYFLT *)malloc(self->size * sizeof(MYFLT));
        self->accum_real[i] = (MYFLT *)malloc(self->size * sizeof(MYFLT));
        self->accum_imag[i] = (MYFLT *)malloc(self->size * sizeof(MYFLT));
        for (j=0; j<self->size; j++) {
            self->accum_real[i][j] = 0.0;
            self->accum_imag[i][j] = 0.0;
        }
    }

    inframe = (MYFLT *)malloc(self->size2 * sizeof(MYFLT));
    outframe = (MYFLT *)malloc(self->size2 * sizeof(MYFLT));

    for (j=0; j<self->num_iter; j++) {
        num = j * self->size;
        for (i=0; i<self->size; i++) {
            inframe[i] = tmp2[num+i];
        }
        for (i=self->size; i<self->size2; i++) {
            inframe[i] = 0.0;
        }
        realfft_split(inframe, outframe, self->size2, self->twiddle);
        self->impulse_real[j][0] = outframe[0];
        self->impulse_imag[j][0] = 0.0;
        for (i=1; i<self->size; i++) {
            self->impulse_real[j][i] = outframe[i];
            self->impulse_imag[j][i] = outframe[self->size2 - i];
        }
    }

    free(tmp);
    free(tmp2);
    free(inframe);
    free(outframe);
}

static void
CvlVerb_process_i(CvlVerb *self) {
    int i, j, k;
    MYFLT gdry;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT bal = PyFloat_AS_DOUBLE(self->bal);
    if (bal < 0)
        bal = 0.0;
    else if (bal > 1)
        bal = 1.0;
    gdry = 1.0 - bal;

    for (i=0; i<self->bufsize; i++) {
        self->input_buffer[self->incount] = in[i];
        self->data[i] = (self->output_buffer[self->incount] * 100 * bal) + (in[i] * gdry);

        self->incount++;
        if (self->incount == self->size) {
            self->incount = 0;

            k = self->current_iter - 1;
            if (k < 0)
                k += self->num_iter;
            for (i=0; i<self->size; i++) {
                self->accum_real[k][i] = self->accum_imag[k][i] = 0.0;
                self->inframe[i] = self->last_half_frame[i];
                self->inframe[i+self->size] = self->last_half_frame[i] = self->input_buffer[i];
            }
            realfft_split(self->inframe, self->outframe, self->size2, self->twiddle);
            self->real[0] = self->outframe[0];
            self->imag[0] = 0.0;
            for (i=1; i<self->size; i++) {
                self->real[i] = self->outframe[i];
                self->imag[i] = self->outframe[self->size2 - i];
            }
            for (j=0; j<self->num_iter; j++) {
                k = self->current_iter + j;
                if (k >= self->num_iter)
                    k -= self->num_iter;
                for (i=0; i<self->size; i++) {
                    self->accum_real[k][i] += self->real[i] * self->impulse_real[j][i] - self->imag[i] * self->impulse_imag[j][i];
                    self->accum_imag[k][i] += self->real[i] * self->impulse_imag[j][i] + self->imag[i] * self->impulse_real[j][i];
                }
            }
            self->inframe[0] = self->accum_real[self->current_iter][0];
            self->inframe[self->size] = 0.0;
            for (i=1; i<self->size; i++) {
                self->inframe[i] = self->accum_real[self->current_iter][i];
                self->inframe[self->size2 - i] = self->accum_imag[self->current_iter][i];
            }
            irealfft_split(self->inframe, self->outframe, self->size2, self->twiddle);

            for (i=0; i<self->size; i++) {
                self->output_buffer[i] = self->outframe[i+self->size];
            }

            self->current_iter++;
            if (self->current_iter == self->num_iter)
                self->current_iter = 0;
        }
    }
}

static void
CvlVerb_process_a(CvlVerb *self) {
    int i, j, k;
    MYFLT gwet, gdry;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *bal = Stream_getData((Stream *)self->bal_stream);

    for (i=0; i<self->bufsize; i++) {
        gwet = bal[i];
        if (gwet < 0)
            gwet = 0.0;
        else if (gwet > 1)
            gwet = 1.0;
        gdry = 1.0 - gwet;
        self->input_buffer[self->incount] = in[i];
        self->data[i] = (self->output_buffer[self->incount] * 100 * gwet) + in[i] * gdry;

        self->incount++;
        if (self->incount == self->size) {
            self->incount = 0;

            k = self->current_iter - 1;
            if (k < 0)
                k += self->num_iter;
            for (i=0; i<self->size; i++) {
                self->accum_real[k][i] = self->accum_imag[k][i] = 0.0;
                self->inframe[i] = self->last_half_frame[i];
                self->inframe[i+self->size] = self->last_half_frame[i] = self->input_buffer[i];
            }
            realfft_split(self->inframe, self->outframe, self->size2, self->twiddle);
            self->real[0] = self->outframe[0];
            self->imag[0] = 0.0;
            for (i=1; i<self->size; i++) {
                self->real[i] = self->outframe[i];
                self->imag[i] = self->outframe[self->size2 - i];
            }
            for (j=0; j<self->num_iter; j++) {
                k = self->current_iter + j;
                if (k >= self->num_iter)
                    k -= self->num_iter;
                for (i=0; i<self->size; i++) {
                    self->accum_real[k][i] += self->real[i] * self->impulse_real[j][i] - self->imag[i] * self->impulse_imag[j][i];
                    self->accum_imag[k][i] += self->real[i] * self->impulse_imag[j][i] + self->imag[i] * self->impulse_real[j][i];
                }
            }
            self->inframe[0] = self->accum_real[self->current_iter][0];
            self->inframe[self->size] = 0.0;
            for (i=1; i<self->size; i++) {
                self->inframe[i] = self->accum_real[self->current_iter][i];
                self->inframe[self->size2 - i] = self->accum_imag[self->current_iter][i];
            }
            irealfft_split(self->inframe, self->outframe, self->size2, self->twiddle);

            for (i=0; i<self->size; i++) {
                self->output_buffer[i] = self->outframe[i+self->size];
            }

            self->current_iter++;
            if (self->current_iter == self->num_iter)
                self->current_iter = 0;
        }
    }
}

static void CvlVerb_postprocessing_ii(CvlVerb *self) { POST_PROCESSING_II };
static void CvlVerb_postprocessing_ai(CvlVerb *self) { POST_PROCESSING_AI };
static void CvlVerb_postprocessing_ia(CvlVerb *self) { POST_PROCESSING_IA };
static void CvlVerb_postprocessing_aa(CvlVerb *self) { POST_PROCESSING_AA };
static void CvlVerb_postprocessing_ireva(CvlVerb *self) { POST_PROCESSING_IREVA };
static void CvlVerb_postprocessing_areva(CvlVerb *self) { POST_PROCESSING_AREVA };
static void CvlVerb_postprocessing_revai(CvlVerb *self) { POST_PROCESSING_REVAI };
static void CvlVerb_postprocessing_revaa(CvlVerb *self) { POST_PROCESSING_REVAA };
static void CvlVerb_postprocessing_revareva(CvlVerb *self) { POST_PROCESSING_REVAREVA };

static void
CvlVerb_setProcMode(CvlVerb *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode) {
        case 0:
            self->proc_func_ptr = CvlVerb_process_i;
            break;
        case 1:
            self->proc_func_ptr = CvlVerb_process_a;
            break;
    }

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = CvlVerb_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = CvlVerb_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = CvlVerb_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = CvlVerb_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = CvlVerb_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = CvlVerb_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = CvlVerb_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = CvlVerb_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = CvlVerb_postprocessing_revareva;
            break;
    }
}

static void
CvlVerb_compute_next_data_frame(CvlVerb *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
CvlVerb_traverse(CvlVerb *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->bal);
    Py_VISIT(self->bal_stream);
    return 0;
}

static int
CvlVerb_clear(CvlVerb *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->bal);
    Py_CLEAR(self->bal_stream);
    return 0;
}

static void
CvlVerb_dealloc(CvlVerb* self)
{
    int i;
    pyo_DEALLOC
    free(self->inframe);
    free(self->outframe);
    free(self->input_buffer);
    free(self->output_buffer);
    free(self->last_half_frame);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    for(i=0; i<self->num_iter; i++) {
        free(self->impulse_real[i]);
        free(self->impulse_imag[i]);
        free(self->accum_real[i]);
        free(self->accum_imag[i]);
    }
    free(self->impulse_real);
    free(self->impulse_imag);
    free(self->accum_real);
    free(self->accum_imag);
    free(self->real);
    free(self->imag);
    CvlVerb_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CvlVerb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, k;
    Py_ssize_t psize;
    PyObject *inputtmp, *input_streamtmp, *baltmp=NULL, *multmp=NULL, *addtmp=NULL;
    CvlVerb *self;
    self = (CvlVerb *)type->tp_alloc(type, 0);

    self->bal = PyFloat_FromDouble(0.25);
    self->size = 1024;
    self->chnl = 0;
    self->incount = 0;
    self->current_iter = 0;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CvlVerb_compute_next_data_frame);
    self->mode_func_ptr = CvlVerb_setProcMode;

    static char *kwlist[] = {"input", "impulse", "bal", "size", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os#|OiiOO", kwlist, &inputtmp, &self->impulse_path, &psize, &baltmp, &self->size, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (self->size < self->bufsize) {
        PySys_WriteStdout("Warning: CvlVerb size less than buffer size!\nCvlVerb size set to buffersize: %d\n", self->bufsize);
        self->size = self->bufsize;
    }

    k = 1;
    while (k < self->size)
        k <<= 1;
    self->size = k;

    INIT_INPUT_STREAM

    if (baltmp) {
        PyObject_CallMethod((PyObject *)self, "setBal", "O", baltmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    CvlVerb_alloc_memories(self);
    CvlVerb_analyse_impulse(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
CvlVerb_setBal(CvlVerb *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->bal);
    if (isNumber == 1) {
        self->bal = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else {
        self->bal = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bal, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bal_stream);
        self->bal_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * CvlVerb_getServer(CvlVerb* self) { GET_SERVER };
static PyObject * CvlVerb_getStream(CvlVerb* self) { GET_STREAM };
static PyObject * CvlVerb_setMul(CvlVerb *self, PyObject *arg) { SET_MUL };
static PyObject * CvlVerb_setAdd(CvlVerb *self, PyObject *arg) { SET_ADD };
static PyObject * CvlVerb_setSub(CvlVerb *self, PyObject *arg) { SET_SUB };
static PyObject * CvlVerb_setDiv(CvlVerb *self, PyObject *arg) { SET_DIV };

static PyObject * CvlVerb_play(CvlVerb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CvlVerb_out(CvlVerb *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * CvlVerb_stop(CvlVerb *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * CvlVerb_multiply(CvlVerb *self, PyObject *arg) { MULTIPLY };
static PyObject * CvlVerb_inplace_multiply(CvlVerb *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * CvlVerb_add(CvlVerb *self, PyObject *arg) { ADD };
static PyObject * CvlVerb_inplace_add(CvlVerb *self, PyObject *arg) { INPLACE_ADD };
static PyObject * CvlVerb_sub(CvlVerb *self, PyObject *arg) { SUB };
static PyObject * CvlVerb_inplace_sub(CvlVerb *self, PyObject *arg) { INPLACE_SUB };
static PyObject * CvlVerb_div(CvlVerb *self, PyObject *arg) { DIV };
static PyObject * CvlVerb_inplace_div(CvlVerb *self, PyObject *arg) { INPLACE_DIV };


static PyMemberDef CvlVerb_members[] = {
{"server", T_OBJECT_EX, offsetof(CvlVerb, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(CvlVerb, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(CvlVerb, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(CvlVerb, add), 0, "Add factor."},
{"input", T_OBJECT_EX, offsetof(CvlVerb, input), 0, "Input sound object."},
{"bal", T_OBJECT_EX, offsetof(CvlVerb, bal), 0, "Wet/dry balance."},
{NULL}  /* Sentinel */
};

static PyMethodDef CvlVerb_methods[] = {
{"getServer", (PyCFunction)CvlVerb_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)CvlVerb_getStream, METH_NOARGS, "Returns stream object."},
{"out", (PyCFunction)CvlVerb_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"play", (PyCFunction)CvlVerb_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)CvlVerb_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setBal", (PyCFunction)CvlVerb_setBal, METH_O, "Sets wet/dry balance."},
{"setMul", (PyCFunction)CvlVerb_setMul, METH_O, "Sets CvlVerb mul factor."},
{"setAdd", (PyCFunction)CvlVerb_setAdd, METH_O, "Sets CvlVerb add factor."},
{"setSub", (PyCFunction)CvlVerb_setSub, METH_O, "Sets CvlVerb add factor."},
{"setDiv", (PyCFunction)CvlVerb_setDiv, METH_O, "Sets CvlVerb mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods CvlVerb_as_number = {
    (binaryfunc)CvlVerb_add,                      /*nb_add*/
    (binaryfunc)CvlVerb_sub,                 /*nb_subtract*/
    (binaryfunc)CvlVerb_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)CvlVerb_inplace_add,              /*inplace_add*/
    (binaryfunc)CvlVerb_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)CvlVerb_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)CvlVerb_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)CvlVerb_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject CvlVerbType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.CvlVerb_base",                                   /*tp_name*/
sizeof(CvlVerb),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)CvlVerb_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&CvlVerb_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"CvlVerb objects. FFT transform.",           /* tp_doc */
(traverseproc)CvlVerb_traverse,                  /* tp_traverse */
(inquiry)CvlVerb_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
CvlVerb_methods,                                 /* tp_methods */
CvlVerb_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
CvlVerb_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int size;
    int hsize;
    int wintype;
    int incount;
    int freqone;
    int freqtwo;
    int width;
    int height;
    int fscaling; /* frequency scaling : 0 = lin, 1 = log */
    int mscaling; /* magnitude scaling : 0 = lin, 1 = log */
    MYFLT gain;
    MYFLT oneOverSr;
    MYFLT freqPerBin;
    MYFLT *input_buffer;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT *magnitude;
    MYFLT *last_magnitude;
    MYFLT *tmpmag;
    MYFLT *window;
    MYFLT **twiddle;
} Spectrum;

static void
Spectrum_realloc_memories(Spectrum *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->size * sizeof(MYFLT));
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++)
        self->input_buffer[i] = self->inframe[i] = self->outframe[i] = 0.0;
    self->magnitude = (MYFLT *)realloc(self->magnitude, self->hsize * sizeof(MYFLT));
    self->last_magnitude = (MYFLT *)realloc(self->last_magnitude, self->hsize * sizeof(MYFLT));
    self->tmpmag = (MYFLT *)realloc(self->tmpmag, (self->hsize+6) * sizeof(MYFLT));
    for (i=0; i<self->hsize; i++)
        self->magnitude[i] = self->last_magnitude[i] = self->tmpmag[i+3] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);
    self->incount = self->hsize;
    self->freqPerBin = self->sr / self->size;
}

static PyObject *
Spectrum_display(Spectrum *self) {
    int i, p1, b1, b2, bins;
    MYFLT pos, step, frac, iw, mag, h4;
    MYFLT logmin, logrange;
    PyObject *points, *tuple;

    b1 = (int)(self->freqone / self->freqPerBin);
    b2 = (int)(self->freqtwo / self->freqPerBin);
    bins = b2 - b1;
    step = bins / (MYFLT)(self->width);
    iw = 1.0 / (MYFLT)(self->width);
    h4 = self->height * 0.75;

    points = PyList_New(self->width+2);

    tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(0));
    PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height));
    PyList_SET_ITEM(points, 0, tuple);
    tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(self->width));
    PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height));
    PyList_SET_ITEM(points, self->width+1, tuple);
    if (!self->fscaling && !self->mscaling) {
        for (i=0; i<self->width; i++) {
            pos = i * step + b1;
            p1 = (int)pos;
            frac = pos - p1;
            tuple = PyTuple_New(2);
            mag = ((self->magnitude[p1] + (self->magnitude[p1+1] - self->magnitude[p1]) * frac) * self->gain * 4 * h4);
            PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(i));
            PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height - (int)mag));
            PyList_SET_ITEM(points, i+1, tuple);
        }
    }
    else if (!self->fscaling && self->mscaling) {
        for (i=0; i<self->width; i++) {
            pos = i * step + b1;
            p1 = (int)pos;
            frac = pos - p1;
            tuple = PyTuple_New(2);
            mag = ((self->magnitude[p1] + (self->magnitude[p1+1] - self->magnitude[p1]) * frac) * 0.7 * self->gain);
            mag = mag > 0.001 ? mag : 0.001;
            mag = (60.0 + (20.0 * MYLOG10(mag))) * 0.01666 * h4;
            PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(i));
            PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height - (int)mag));
            PyList_SET_ITEM(points, i+1, tuple);
        }
    }
    else if (self->fscaling && !self->mscaling) {
        if (self->freqone <= 20.0)
            self->freqone = 20.0;
        logmin = MYLOG10(self->freqone);
        logrange = MYLOG10(self->freqtwo) - logmin;
        for (i=0; i<self->width; i++) {
            pos = MYPOW(10.0, i * iw * logrange + logmin) / self->freqPerBin;
            p1 = (int)pos;
            frac = pos - p1;
            tuple = PyTuple_New(2);
            mag = ((self->magnitude[p1] + (self->magnitude[p1+1] - self->magnitude[p1]) * frac) * self->gain * 4 * h4);
            PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(i));
            PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height - (int)mag));
            PyList_SET_ITEM(points, i+1, tuple);
        }
    }
    else {
        if (self->freqone <= 20.0)
            self->freqone = 20.0;
        logmin = MYLOG10(self->freqone);
        logrange = MYLOG10(self->freqtwo) - logmin;
        for (i=0; i<self->width; i++) {
            pos = MYPOW(10.0, i * iw * logrange + logmin) / self->freqPerBin;
            p1 = (int)pos;
            frac = pos - p1;
            tuple = PyTuple_New(2);
            mag = ((self->magnitude[p1] + (self->magnitude[p1+1] - self->magnitude[p1]) * frac) * 0.7 * self->gain);
            mag = mag > 0.001 ? mag : 0.001;
            mag = (60.0 + (20.0 * MYLOG10(mag))) * 0.01666 * self->height;
            PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(i));
            PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height - (int)mag));
            PyList_SET_ITEM(points, i+1, tuple);
        }
    }

    return points;
}

static void
Spectrum_filters(Spectrum *self) {
    int i, j = 0, impos = 0;
    MYFLT tmp = 0.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->input_buffer[self->incount] = in[i];
        self->incount++;
        if (self->incount == self->size) {
            for (j=0; j<self->size; j++) {
                self->inframe[j] = self->input_buffer[j] * self->window[j];
            }
            self->incount = self->hsize;
            realfft_split(self->inframe, self->outframe, self->size, self->twiddle);
            self->tmpmag[0] = self->tmpmag[1] = self->tmpmag[2] = 0.0;
            self->tmpmag[self->hsize] = self->tmpmag[self->hsize+1] = self->tmpmag[self->hsize+2] = 0.0;
            self->tmpmag[3] = MYSQRT(self->outframe[0]*self->outframe[0]);
            for (j=1; j<self->hsize; j++) {
                impos = self->size - j;
                tmp = MYSQRT(self->outframe[j]*self->outframe[j] + self->outframe[impos]*self->outframe[impos]) * 2;
                self->tmpmag[j+3] = self->last_magnitude[j] = tmp + self->last_magnitude[j] * 0.5;
            }
            for (j=0; j<self->hsize; j++) {
                tmp =   (self->tmpmag[j] + self->tmpmag[j+6]) * 0.05 +
                        (self->tmpmag[j+1] + self->tmpmag[j+5]) * 0.15 +
                        (self->tmpmag[j+2] + self->tmpmag[j+4])* 0.3 +
                        self->tmpmag[j+3] * 0.5;
                self->magnitude[j] = tmp;
                self->input_buffer[j] = self->input_buffer[j+self->hsize];
            }
        }
    }
}

static void
Spectrum_setProcMode(Spectrum *self)
{
    self->proc_func_ptr = Spectrum_filters;
}

static void
Spectrum_compute_next_data_frame(Spectrum *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Spectrum_traverse(Spectrum *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Spectrum_clear(Spectrum *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Spectrum_dealloc(Spectrum* self)
{
    int i;
    pyo_DEALLOC
    free(self->input_buffer);
    free(self->inframe);
    free(self->outframe);
    free(self->window);
    free(self->magnitude);
    free(self->last_magnitude);
    free(self->tmpmag);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    Spectrum_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Spectrum_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, k;
    PyObject *inputtmp, *input_streamtmp;
    Spectrum *self;
    self = (Spectrum *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->wintype = 2;

    INIT_OBJECT_COMMON

    self->gain = 1.0;
    self->oneOverSr = 1.0 / self->sr;
    self->freqone = 0.0;
    self->freqtwo = self->sr * 0.5;
    self->width = 500;
    self->height = 400;
    self->fscaling = 0;
    self->mscaling = 1;

    Stream_setFunctionPtr(self->stream, Spectrum_compute_next_data_frame);
    self->mode_func_ptr = Spectrum_setProcMode;

    static char *kwlist[] = {"input", "size", "wintype", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|ii", kwlist, &inputtmp, &self->size, &self->wintype))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (!isPowerOfTwo(self->size)) {
        k = 1;
        while (k < self->size)
            k *= 2;
        self->size = k;
        PySys_WriteStdout("Spectrum: size argument must be a power-of-2, using the next power-of-2 greater than size : %d\n", self->size);
    }

    Spectrum_realloc_memories(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Spectrum_getServer(Spectrum* self) { GET_SERVER };
static PyObject * Spectrum_getStream(Spectrum* self) { GET_STREAM };
static PyObject * Spectrum_play(Spectrum *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Spectrum_stop(Spectrum *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Spectrum_setSize(Spectrum *self, PyObject *arg)
{
    int tmp;
    if (PyLong_Check(arg) || PyInt_Check(arg)) {
        tmp = PyInt_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->size = tmp;
            Spectrum_realloc_memories(self);
        }
        else
            PySys_WriteStdout("FFT size must be a power of two!\n");
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setWinType(Spectrum *self, PyObject *arg)
{
    if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setLowbound(Spectrum *self, PyObject *arg)
{
    MYFLT tmp;
    if (PyNumber_Check(arg)) {
        tmp = PyFloat_AsDouble(arg);
        if (tmp >= 0.0 && tmp <= 0.5)
            self->freqone = tmp * self->sr;
        else
            self->freqone = 0.0;
    }
    else
        self->freqone = 0.0;

    return PyFloat_FromDouble(MYFLOOR(self->freqone / self->freqPerBin) * self->freqPerBin);
}

static PyObject *
Spectrum_getLowfreq(Spectrum *self)
{
    return PyFloat_FromDouble(self->freqone);
}

static PyObject *
Spectrum_setHighbound(Spectrum *self, PyObject *arg)
{
    MYFLT tmp;
    if (PyNumber_Check(arg)) {
        tmp = PyFloat_AsDouble(arg);
        if (tmp >= 0.0 && tmp <= 0.5)
            self->freqtwo = tmp * self->sr;
        else
            self->freqtwo = self->sr * 0.5;
    }
    else
        self->freqtwo = self->sr * 0.5;

    return PyFloat_FromDouble(MYFLOOR(self->freqtwo / self->freqPerBin) * self->freqPerBin);
}

static PyObject *
Spectrum_getHighfreq(Spectrum *self)
{
    return PyFloat_FromDouble(self->freqtwo);
}

static PyObject *
Spectrum_setWidth(Spectrum *self, PyObject *arg)
{
    if (PyInt_Check(arg) || PyLong_Check(arg))
        self->width = PyLong_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setHeight(Spectrum *self, PyObject *arg)
{
    if (PyInt_Check(arg) || PyLong_Check(arg))
        self->height = PyLong_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setFscaling(Spectrum *self, PyObject *arg)
{
    if (PyInt_Check(arg) || PyLong_Check(arg))
        self->fscaling = PyLong_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setMscaling(Spectrum *self, PyObject *arg)
{
    if (PyInt_Check(arg) || PyLong_Check(arg))
        self->mscaling = PyLong_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Spectrum_setGain(Spectrum *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
        self->gain = PyFloat_AsDouble(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Spectrum_members[] = {
{"server", T_OBJECT_EX, offsetof(Spectrum, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Spectrum, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Spectrum, input), 0, "FFT sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Spectrum_methods[] = {
{"getServer", (PyCFunction)Spectrum_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Spectrum_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Spectrum_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Spectrum_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setSize", (PyCFunction)Spectrum_setSize, METH_O, "Sets a new FFT size."},
{"setWinType", (PyCFunction)Spectrum_setWinType, METH_O, "Sets a new window."},
{"setLowbound", (PyCFunction)Spectrum_setLowbound, METH_O, "Sets the first frequency to display."},
{"setHighbound", (PyCFunction)Spectrum_setHighbound, METH_O, "Sets the last frequency to display."},
{"setWidth", (PyCFunction)Spectrum_setWidth, METH_O, "Sets the width of the display."},
{"setHeight", (PyCFunction)Spectrum_setHeight, METH_O, "Sets the height of the display."},
{"setFscaling", (PyCFunction)Spectrum_setFscaling, METH_O, "Sets the frequency scaling of the display."},
{"setMscaling", (PyCFunction)Spectrum_setMscaling, METH_O, "Sets the magnitude scaling of the display."},
{"setGain", (PyCFunction)Spectrum_setGain, METH_O, "Sets the magnitude gain of the display."},
{"display", (PyCFunction)Spectrum_display, METH_NOARGS, "Gets points to display."},
{"getLowfreq", (PyCFunction)Spectrum_getLowfreq, METH_NOARGS, "Returns the lowest frequency to display."},
{"getHighfreq", (PyCFunction)Spectrum_getHighfreq, METH_NOARGS, "Returns the highest frequency to display."},
{NULL}  /* Sentinel */
};

PyTypeObject SpectrumType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Spectrum_base",                                   /*tp_name*/
sizeof(Spectrum),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Spectrum_dealloc,                     /*tp_dealloc*/
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
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Spectrum objects. FFT spectrum analyser.",           /* tp_doc */
(traverseproc)Spectrum_traverse,                  /* tp_traverse */
(inquiry)Spectrum_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Spectrum_methods,                                 /* tp_methods */
Spectrum_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Spectrum_new,                                     /* tp_new */
};