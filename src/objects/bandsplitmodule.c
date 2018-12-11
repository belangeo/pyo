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

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *q;
    Stream *q_stream;
    int bands;
    MYFLT min_freq;
    MYFLT max_freq;
    int init;
    MYFLT halfSr;
    MYFLT TwoPiOnSr;
    MYFLT *band_freqs;
    // sample memories
    MYFLT *x1;
    MYFLT *x2;
    MYFLT *y1;
    MYFLT *y2;
    // coefficients
    MYFLT *b0;
    MYFLT *b2;
    MYFLT *a0;
    MYFLT *a1;
    MYFLT *a2;
    MYFLT *buffer_streams;
    int modebuffer[1];
} BandSplitter;

static void
BandSplitter_compute_variables(BandSplitter *self, MYFLT q)
{
    int i;
    MYFLT freq;
    for (i=0; i<self->bands; i++) {
        freq = self->band_freqs[i];
        if (freq <= 1)
            freq = 1;
        else if (freq >= self->halfSr)
            freq = self->halfSr;

        MYFLT w0 = self->TwoPiOnSr * freq;
        MYFLT c = MYCOS(w0);
        MYFLT alpha = MYSIN(w0) / (2 * q);

        self->b0[i] = alpha;
        self->b2[i] = -alpha;
        self->a0[i] = 1.0 / (1 + alpha);
        self->a1[i] = -2 * c;
        self->a2[i] = 1 - alpha;
    }
}

static void
BandSplitter_setFrequencies(BandSplitter *self)
{
    int i;
    MYFLT frac = 1. / self->bands;
    for (i=0; i<self->bands; i++) {
        self->band_freqs[i] = MYPOW(MYPOW(self->max_freq/self->min_freq, frac), i) * self->min_freq;
    }
}

static void
BandSplitter_filters_i(BandSplitter *self) {
    MYFLT val;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (j=0; j<self->bands; j++) {
            self->x1[j] = self->x2[j] = self->y1[j] = self->y2[j] = in[0];
        }
        self->init = 0;
    }

    for (j=0; j<self->bands; j++) {
        for (i=0; i<self->bufsize; i++) {
            val = ( (self->b0[j] * in[i]) + (self->b2[j] * self->x2[j]) - (self->a1[j] * self->y1[j]) - (self->a2[j] * self->y2[j]) ) * self->a0[j];
            self->y2[j] = self->y1[j];
            self->buffer_streams[i + j * self->bufsize] = self->y1[j] = val;
            self->x2[j] = self->x1[j];
            self->x1[j] = in[i];
        }
    }
}

static void
BandSplitter_filters_a(BandSplitter *self) {
    MYFLT val;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        for (j=0; j<self->bands; j++) {
            self->x1[j] = self->x2[j] = self->y1[j] = self->y2[j] = in[0];
        }
        self->init = 0;
    }

    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        BandSplitter_compute_variables((BandSplitter *)self, q[i]);
        for (j=0; j<self->bands; j++) {
            val = ( (self->b0[j] * in[i]) + (self->b2[j] * self->x2[j]) - (self->a1[j] * self->y1[j]) - (self->a2[j] * self->y2[j]) ) * self->a0[j];
            self->y2[j] = self->y1[j];
            self->buffer_streams[i + j * self->bufsize] = self->y1[j] = val;
            self->x2[j] = self->x1[j];
            self->x1[j] = in[i];
        }
    }
}

MYFLT *
BandSplitter_getSamplesBuffer(BandSplitter *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
BandSplitter_setProcMode(BandSplitter *self)
{
    int procmode;
    procmode = self->modebuffer[0];

	switch (procmode) {
        case 0:
            self->proc_func_ptr = BandSplitter_filters_i;
            break;
        case 1:
            self->proc_func_ptr = BandSplitter_filters_a;
            break;
    }
}

static void
BandSplitter_compute_next_data_frame(BandSplitter *self)
{
    (*self->proc_func_ptr)(self);
}

static int
BandSplitter_traverse(BandSplitter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->q);
    Py_VISIT(self->q_stream);
    return 0;
}

static int
BandSplitter_clear(BandSplitter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->q);
    Py_CLEAR(self->q_stream);
    return 0;
}

static void
BandSplitter_dealloc(BandSplitter* self)
{
    pyo_DEALLOC
    free(self->band_freqs);
    free(self->x1);
    free(self->x2);
    free(self->y1);
    free(self->y2);
    free(self->b0);
    free(self->b2);
    free(self->a0);
    free(self->a1);
    free(self->a2);
    free(self->buffer_streams);
    BandSplitter_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
BandSplitter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *qtmp=NULL;
    BandSplitter *self;
    self = (BandSplitter *)type->tp_alloc(type, 0);


    self->bands = 4;
    self->q = PyFloat_FromDouble(1.);
    self->init = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BandSplitter_compute_next_data_frame);
    self->mode_func_ptr = BandSplitter_setProcMode;

    self->halfSr = self->sr / 2.01;
    self->TwoPiOnSr = TWOPI / self->sr;

    static char *kwlist[] = {"input", "bands", "min", "max", "q", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IFFO, kwlist, &inputtmp, &self->bands, &self->min_freq, &self->max_freq, &qtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->band_freqs = (MYFLT *)realloc(self->band_freqs, self->bands * sizeof(MYFLT));
    self->x1 = (MYFLT *)realloc(self->x1, self->bands * sizeof(MYFLT));
    self->x2 = (MYFLT *)realloc(self->x2, self->bands * sizeof(MYFLT));
    self->y1 = (MYFLT *)realloc(self->y1, self->bands * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->bands * sizeof(MYFLT));
    self->b0 = (MYFLT *)realloc(self->b0, self->bands * sizeof(MYFLT));
    self->b2 = (MYFLT *)realloc(self->b2, self->bands * sizeof(MYFLT));
    self->a0 = (MYFLT *)realloc(self->a0, self->bands * sizeof(MYFLT));
    self->a1 = (MYFLT *)realloc(self->a1, self->bands * sizeof(MYFLT));
    self->a2 = (MYFLT *)realloc(self->a2, self->bands * sizeof(MYFLT));
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->bands * self->bufsize * sizeof(MYFLT));

    BandSplitter_setFrequencies((BandSplitter *)self);

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }
    else {
        BandSplitter_compute_variables((BandSplitter *)self, PyFloat_AS_DOUBLE(self->q));
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
BandSplitter_setQ(BandSplitter *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
        BandSplitter_compute_variables((BandSplitter *)self, PyFloat_AS_DOUBLE(self->q));
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * BandSplitter_getServer(BandSplitter* self) { GET_SERVER };
static PyObject * BandSplitter_getStream(BandSplitter* self) { GET_STREAM };

static PyObject * BandSplitter_play(BandSplitter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BandSplitter_stop(BandSplitter *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef BandSplitter_members[] = {
{"server", T_OBJECT_EX, offsetof(BandSplitter, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(BandSplitter, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(BandSplitter, input), 0, "Input sound object."},
{"q", T_OBJECT_EX, offsetof(BandSplitter, q), 0, "Filters Q."},
{NULL}  /* Sentinel */
};

static PyMethodDef BandSplitter_methods[] = {
{"getServer", (PyCFunction)BandSplitter_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)BandSplitter_getStream, METH_NOARGS, "Returns stream object."},
{"setQ", (PyCFunction)BandSplitter_setQ, METH_O, "Sets the filters Q."},
{"play", (PyCFunction)BandSplitter_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)BandSplitter_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject BandSplitterType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.BandSplitter_base",                                   /*tp_name*/
sizeof(BandSplitter),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)BandSplitter_dealloc,                     /*tp_dealloc*/
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
"BandSplitter objects. Split audio stream in multiple frequency bands.",           /* tp_doc */
(traverseproc)BandSplitter_traverse,                  /* tp_traverse */
(inquiry)BandSplitter_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
BandSplitter_methods,                                 /* tp_methods */
BandSplitter_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
BandSplitter_new,                                     /* tp_new */
};

/************************************************************************************************/
/* BandSplit streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    BandSplitter *mainSplitter;
    int modebuffer[2];
    int chnl;
} BandSplit;

static void BandSplit_postprocessing_ii(BandSplit *self) { POST_PROCESSING_II };
static void BandSplit_postprocessing_ai(BandSplit *self) { POST_PROCESSING_AI };
static void BandSplit_postprocessing_ia(BandSplit *self) { POST_PROCESSING_IA };
static void BandSplit_postprocessing_aa(BandSplit *self) { POST_PROCESSING_AA };
static void BandSplit_postprocessing_ireva(BandSplit *self) { POST_PROCESSING_IREVA };
static void BandSplit_postprocessing_areva(BandSplit *self) { POST_PROCESSING_AREVA };
static void BandSplit_postprocessing_revai(BandSplit *self) { POST_PROCESSING_REVAI };
static void BandSplit_postprocessing_revaa(BandSplit *self) { POST_PROCESSING_REVAA };
static void BandSplit_postprocessing_revareva(BandSplit *self) { POST_PROCESSING_REVAREVA };

static void
BandSplit_setProcMode(BandSplit *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BandSplit_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BandSplit_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BandSplit_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BandSplit_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BandSplit_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BandSplit_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BandSplit_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BandSplit_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BandSplit_postprocessing_revareva;
            break;
    }
}

static void
BandSplit_compute_next_data_frame(BandSplit *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = BandSplitter_getSamplesBuffer((BandSplitter *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
BandSplit_traverse(BandSplit *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
BandSplit_clear(BandSplit *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
BandSplit_dealloc(BandSplit* self)
{
    pyo_DEALLOC
    BandSplit_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
BandSplit_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    BandSplit *self;
    self = (BandSplit *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BandSplit_compute_next_data_frame);
    self->mode_func_ptr = BandSplit_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (BandSplitter *)maintmp;

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

static PyObject * BandSplit_getServer(BandSplit* self) { GET_SERVER };
static PyObject * BandSplit_getStream(BandSplit* self) { GET_STREAM };
static PyObject * BandSplit_setMul(BandSplit *self, PyObject *arg) { SET_MUL };
static PyObject * BandSplit_setAdd(BandSplit *self, PyObject *arg) { SET_ADD };
static PyObject * BandSplit_setSub(BandSplit *self, PyObject *arg) { SET_SUB };
static PyObject * BandSplit_setDiv(BandSplit *self, PyObject *arg) { SET_DIV };

static PyObject * BandSplit_play(BandSplit *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BandSplit_out(BandSplit *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BandSplit_stop(BandSplit *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * BandSplit_multiply(BandSplit *self, PyObject *arg) { MULTIPLY };
static PyObject * BandSplit_inplace_multiply(BandSplit *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BandSplit_add(BandSplit *self, PyObject *arg) { ADD };
static PyObject * BandSplit_inplace_add(BandSplit *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BandSplit_sub(BandSplit *self, PyObject *arg) { SUB };
static PyObject * BandSplit_inplace_sub(BandSplit *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BandSplit_div(BandSplit *self, PyObject *arg) { DIV };
static PyObject * BandSplit_inplace_div(BandSplit *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BandSplit_members[] = {
{"server", T_OBJECT_EX, offsetof(BandSplit, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(BandSplit, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(BandSplit, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(BandSplit, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef BandSplit_methods[] = {
{"getServer", (PyCFunction)BandSplit_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)BandSplit_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)BandSplit_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)BandSplit_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)BandSplit_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)BandSplit_setMul, METH_O, "Sets BandSplit mul factor."},
{"setAdd", (PyCFunction)BandSplit_setAdd, METH_O, "Sets BandSplit add factor."},
{"setSub", (PyCFunction)BandSplit_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)BandSplit_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods BandSplit_as_number = {
(binaryfunc)BandSplit_add,                      /*nb_add*/
(binaryfunc)BandSplit_sub,                 /*nb_subtract*/
(binaryfunc)BandSplit_multiply,                 /*nb_multiply*/
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
(binaryfunc)BandSplit_inplace_add,              /*inplace_add*/
(binaryfunc)BandSplit_inplace_sub,         /*inplace_subtract*/
(binaryfunc)BandSplit_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)BandSplit_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)BandSplit_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject BandSplitType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.BandSplit_base",         /*tp_name*/
sizeof(BandSplit),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)BandSplit_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&BandSplit_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"BandSplit objects. Reads one band from a BandSplitter process.",           /* tp_doc */
(traverseproc)BandSplit_traverse,   /* tp_traverse */
(inquiry)BandSplit_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
BandSplit_methods,             /* tp_methods */
BandSplit_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
BandSplit_new,                 /* tp_new */
};

/*****************/
/* FourBand main */
/*****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq1;
    Stream *freq1_stream;
    PyObject *freq2;
    Stream *freq2_stream;
    PyObject *freq3;
    Stream *freq3_stream;
    double last_freq1;
    double last_freq2;
    double last_freq3;
    // sample memories
    double x1[12];
    double x2[12];
    double x3[12];
    double x4[12];
    double y1[12];
    double y2[12];
    double y3[12];
    double y4[12];
    // coefficients
    double b1[3];
    double b2[3];
    double b3[3];
    double b4[3];
    double la0[3];
    double la1[3];
    double la2[3];
    double ha0[3];
    double ha1[3];
    double ha2[3];
    MYFLT *buffer_streams;
    int modebuffer[3];
} FourBandMain;

static void
FourBandMain_compute_variables(FourBandMain *self, double freq, int bound)
{
    double wc = TWOPI * freq;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(PI * freq / self->sr);
    double k2 = k * k;
    double k3 = k2 * k;
    double k4 = k2 * k2;
    double sqrt2 = sqrt(2.0);
    double sq_tmp1 = sqrt2 * wc3 * k;
    double sq_tmp2 = sqrt2 * wc * k3;
    double a_tmp = 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + k4 + 2.0 * sq_tmp2 + wc4;
    double wc4_a_tmp = wc4 / a_tmp;
    double k4_a_tmp = k4 / a_tmp;

    /* common */
    self->b1[bound] = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    self->b2[bound] = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    self->b3[bound] = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    self->b4[bound] = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    /* lowpass */
    self->la0[bound] = wc4_a_tmp;
    self->la1[bound] = 4.0 * wc4_a_tmp;
    self->la2[bound] = 6.0 * wc4_a_tmp;

    /* highpass */
    self->ha0[bound] = k4_a_tmp;
    self->ha1[bound] = -4.0 * k4_a_tmp;
    self->ha2[bound] = 6.0 * k4_a_tmp;
}

static int
FourBandMain_splitter(FourBandMain *self, MYFLT *input, MYFLT *outlow, MYFLT *outhigh, int bound, int filtercount) {
    int i, indl = filtercount, indh = filtercount + 1;
    double val, inval;
    for (i=0; i<self->bufsize; i++) {
        inval = (double)input[i];
        /* lowpass */
        val = self->la0[bound] * inval + self->la1[bound] * self->x1[indl] + self->la2[bound] * self->x2[indl] + 
              self->la1[bound] * self->x3[indl] + self->la0[bound] * self->x4[indl] - self->b1[bound] * self->y1[indl] - 
              self->b2[bound] * self->y2[indl] - self->b3[bound] * self->y3[indl] - self->b4[bound] * self->y4[indl];
        self->y4[indl] = self->y3[indl];
        self->y3[indl] = self->y2[indl];
        self->y2[indl] = self->y1[indl];
        self->y1[indl] = val;
        self->x4[indl] = self->x3[indl];
        self->x3[indl] = self->x2[indl];
        self->x2[indl] = self->x1[indl];
        self->x1[indl] = inval;
        outlow[i] = (MYFLT)val;

        /* highpass */
        val = self->ha0[bound] * inval + self->ha1[bound] * self->x1[indh] + self->ha2[bound] * self->x2[indh] + 
              self->ha1[bound] * self->x3[indh] + self->ha0[bound] * self->x4[indh] - self->b1[bound] * self->y1[indh] - 
              self->b2[bound] * self->y2[indh] - self->b3[bound] * self->y3[indh] - self->b4[bound] * self->y4[indh];
        self->y4[indh] = self->y3[indh];
        self->y3[indh] = self->y2[indh];
        self->y2[indh] = self->y1[indh];
        self->y1[indh] = val;
        self->x4[indh] = self->x3[indh];
        self->x3[indh] = self->x2[indh];
        self->x2[indh] = self->x1[indh];
        self->x1[indh] = inval;
        outhigh[i] = (MYFLT)val;
    }
    return filtercount + 2;
}

static int
FourBandMain_phase_align(FourBandMain *self, MYFLT *input, int bound, int filtercount) {
    int i, indl = filtercount, indh = filtercount + 1;
    double val, inval;
    MYFLT tmplow[self->bufsize], tmphigh[self->bufsize];

    for (i=0; i<self->bufsize; i++) {
        inval = (double)input[i];
        /* lowpass */
        val = self->la0[bound] * inval + self->la1[bound] * self->x1[indl] + self->la2[bound] * self->x2[indl] + 
              self->la1[bound] * self->x3[indl] + self->la0[bound] * self->x4[indl] - self->b1[bound] * self->y1[indl] - 
              self->b2[bound] * self->y2[indl] - self->b3[bound] * self->y3[indl] - self->b4[bound] * self->y4[indl];
        self->y4[indl] = self->y3[indl];
        self->y3[indl] = self->y2[indl];
        self->y2[indl] = self->y1[indl];
        self->y1[indl] = val;
        self->x4[indl] = self->x3[indl];
        self->x3[indl] = self->x2[indl];
        self->x2[indl] = self->x1[indl];
        self->x1[indl] = inval;
        tmplow[i] = (MYFLT)val;

        /* highpass */
        val = self->ha0[bound] * inval + self->ha1[bound] * self->x1[indh] + self->ha2[bound] * self->x2[indh] + 
              self->ha1[bound] * self->x3[indh] + self->ha0[bound] * self->x4[indh] - self->b1[bound] * self->y1[indh] - 
              self->b2[bound] * self->y2[indh] - self->b3[bound] * self->y3[indh] - self->b4[bound] * self->y4[indh];
        self->y4[indh] = self->y3[indh];
        self->y3[indh] = self->y2[indh];
        self->y2[indh] = self->y1[indh];
        self->y1[indh] = val;
        self->x4[indh] = self->x3[indh];
        self->x3[indh] = self->x2[indh];
        self->x2[indh] = self->x1[indh];
        self->x1[indh] = inval;
        tmphigh[i] = (MYFLT)val;
    }

    for (i=0; i<self->bufsize; i++) {
        input[i] = tmplow[i] + tmphigh[i];
    }

    return filtercount + 2;
}

static void
FourBandMain_filters(FourBandMain *self) {
    int i, bound, align, filtercount = 0;
    int bounds = 3;
    double f1, f2, f3;
    MYFLT *input;
    MYFLT outlow[self->bufsize], outhigh[self->bufsize];

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[0] == 0)
        f1 = PyFloat_AS_DOUBLE(self->freq1);
    else
        f1 = (double)Stream_getData((Stream *)self->freq1_stream)[0];
    if (self->modebuffer[1] == 0)
        f2 = PyFloat_AS_DOUBLE(self->freq2);
    else
        f2 = (double)Stream_getData((Stream *)self->freq2_stream)[0];
    if (self->modebuffer[2] == 0)
        f3 = PyFloat_AS_DOUBLE(self->freq3);
    else
        f3 = (double)Stream_getData((Stream *)self->freq3_stream)[0];

    if (f1 != self->last_freq1) {
        self->last_freq1 = f1;
        FourBandMain_compute_variables(self, f1, 0);
    }

    if (f2 != self->last_freq2) {
        self->last_freq2 = f2;
        FourBandMain_compute_variables(self, f2, 1);
    }

    if (f3 != self->last_freq3) {
        self->last_freq3 = f3;
        FourBandMain_compute_variables(self, f3, 2);
    }

    input = in;
    for (bound=0; bound<bounds; bound++) {
        filtercount = FourBandMain_splitter(self, input, outlow, outhigh, bound, filtercount);
        for (align=bound+1; align<bounds; align++) {
            filtercount = FourBandMain_phase_align(self, outlow, align, filtercount);
        }
        for (i=0; i<self->bufsize; i++) {
            self->buffer_streams[i + bound * self->bufsize] = outlow[i];
        }
        input = outhigh;
    }
    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i + bounds * self->bufsize] = outhigh[i];
    }
}

MYFLT *
FourBandMain_getSamplesBuffer(FourBandMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
FourBandMain_setProcMode(FourBandMain *self)
{
    self->proc_func_ptr = FourBandMain_filters;
}

static void
FourBandMain_compute_next_data_frame(FourBandMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
FourBandMain_traverse(FourBandMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq1);
    Py_VISIT(self->freq1_stream);
    Py_VISIT(self->freq2);
    Py_VISIT(self->freq2_stream);
    Py_VISIT(self->freq3);
    Py_VISIT(self->freq3_stream);
    return 0;
}

static int
FourBandMain_clear(FourBandMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq1);
    Py_CLEAR(self->freq1_stream);
    Py_CLEAR(self->freq2);
    Py_CLEAR(self->freq2_stream);
    Py_CLEAR(self->freq3);
    Py_CLEAR(self->freq3_stream);
    return 0;
}

static void
FourBandMain_dealloc(FourBandMain* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    FourBandMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FourBandMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freq1tmp=NULL, *freq2tmp=NULL, *freq3tmp=NULL;
    FourBandMain *self;
    self = (FourBandMain *)type->tp_alloc(type, 0);

    self->freq1 = PyFloat_FromDouble(150);
    self->freq2 = PyFloat_FromDouble(500);
    self->freq3 = PyFloat_FromDouble(2000);
    self->last_freq1 = self->last_freq2 = self->last_freq3 = -1.0;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FourBandMain_compute_next_data_frame);
    self->mode_func_ptr = FourBandMain_setProcMode;

    static char *kwlist[] = {"input", "freq1", "freq2", "freq3", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freq1tmp, &freq2tmp, &freq3tmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    for (i=0; i<6; i++) {
        self->x1[i] = self->x2[i] = self->x3[i] = self->x4[i] = self->y1[i] = self->y2[i] = self->y3[i] = self->y4[i] = 0.0;
    }

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 4 * self->bufsize * sizeof(MYFLT));

    for (i=0; i<(4 * self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    if (freq1tmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq1", "O", freq1tmp);
    }

    if (freq2tmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq2", "O", freq2tmp);
    }

    if (freq3tmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq3", "O", freq3tmp);
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
FourBandMain_setFreq1(FourBandMain *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq1);
	if (isNumber == 1) {
		self->freq1 = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->freq1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq1_stream);
        self->freq1_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
FourBandMain_setFreq2(FourBandMain *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq2);
	if (isNumber == 1) {
		self->freq2 = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
	}
	else {
		self->freq2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq2_stream);
        self->freq2_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
FourBandMain_setFreq3(FourBandMain *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq3);
	if (isNumber == 1) {
		self->freq3 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq3 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq3, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq3_stream);
        self->freq3_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * FourBandMain_getServer(FourBandMain* self) { GET_SERVER };
static PyObject * FourBandMain_getStream(FourBandMain* self) { GET_STREAM };

static PyObject * FourBandMain_play(FourBandMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FourBandMain_stop(FourBandMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef FourBandMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(FourBandMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FourBandMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(FourBandMain, input), 0, "Input sound object."},
    {"freq1", T_OBJECT_EX, offsetof(FourBandMain, freq1), 0, "First cutoff frequency."},
    {"freq2", T_OBJECT_EX, offsetof(FourBandMain, freq2), 0, "Second cutoff frequency."},
    {"freq3", T_OBJECT_EX, offsetof(FourBandMain, freq3), 0, "Third cutoff frequency."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FourBandMain_methods[] = {
    {"getServer", (PyCFunction)FourBandMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FourBandMain_getStream, METH_NOARGS, "Returns stream object."},
    {"setFreq1", (PyCFunction)FourBandMain_setFreq1, METH_O, "Sets the first cutoff frequency."},
    {"setFreq2", (PyCFunction)FourBandMain_setFreq2, METH_O, "Sets the second cutoff frequency."},
    {"setFreq3", (PyCFunction)FourBandMain_setFreq3, METH_O, "Sets the third cutoff frequency."},
    {"play", (PyCFunction)FourBandMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)FourBandMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject FourBandMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FourBandMain_base",                                   /*tp_name*/
    sizeof(FourBandMain),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)FourBandMain_dealloc,                     /*tp_dealloc*/
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
    "FourBandMain objects. Split audio stream in four flat frequency and phase bands.",           /* tp_doc */
    (traverseproc)FourBandMain_traverse,                  /* tp_traverse */
    (inquiry)FourBandMain_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    FourBandMain_methods,                                 /* tp_methods */
    FourBandMain_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    FourBandMain_new,                                     /* tp_new */
};

/************************************************************************************************/
/* FourBand streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FourBandMain *mainSplitter;
    int modebuffer[2];
    int chnl;
} FourBand;

static void FourBand_postprocessing_ii(FourBand *self) { POST_PROCESSING_II };
static void FourBand_postprocessing_ai(FourBand *self) { POST_PROCESSING_AI };
static void FourBand_postprocessing_ia(FourBand *self) { POST_PROCESSING_IA };
static void FourBand_postprocessing_aa(FourBand *self) { POST_PROCESSING_AA };
static void FourBand_postprocessing_ireva(FourBand *self) { POST_PROCESSING_IREVA };
static void FourBand_postprocessing_areva(FourBand *self) { POST_PROCESSING_AREVA };
static void FourBand_postprocessing_revai(FourBand *self) { POST_PROCESSING_REVAI };
static void FourBand_postprocessing_revaa(FourBand *self) { POST_PROCESSING_REVAA };
static void FourBand_postprocessing_revareva(FourBand *self) { POST_PROCESSING_REVAREVA };

static void
FourBand_setProcMode(FourBand *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = FourBand_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = FourBand_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = FourBand_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = FourBand_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = FourBand_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = FourBand_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = FourBand_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = FourBand_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = FourBand_postprocessing_revareva;
            break;
    }
}

static void
FourBand_compute_next_data_frame(FourBand *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FourBandMain_getSamplesBuffer((FourBandMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
FourBand_traverse(FourBand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
FourBand_clear(FourBand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
FourBand_dealloc(FourBand* self)
{
    pyo_DEALLOC
    FourBand_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FourBand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    FourBand *self;
    self = (FourBand *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FourBand_compute_next_data_frame);
    self->mode_func_ptr = FourBand_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FourBandMain *)maintmp;

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

static PyObject * FourBand_getServer(FourBand* self) { GET_SERVER };
static PyObject * FourBand_getStream(FourBand* self) { GET_STREAM };
static PyObject * FourBand_setMul(FourBand *self, PyObject *arg) { SET_MUL };
static PyObject * FourBand_setAdd(FourBand *self, PyObject *arg) { SET_ADD };
static PyObject * FourBand_setSub(FourBand *self, PyObject *arg) { SET_SUB };
static PyObject * FourBand_setDiv(FourBand *self, PyObject *arg) { SET_DIV };

static PyObject * FourBand_play(FourBand *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FourBand_out(FourBand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FourBand_stop(FourBand *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * FourBand_multiply(FourBand *self, PyObject *arg) { MULTIPLY };
static PyObject * FourBand_inplace_multiply(FourBand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FourBand_add(FourBand *self, PyObject *arg) { ADD };
static PyObject * FourBand_inplace_add(FourBand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FourBand_sub(FourBand *self, PyObject *arg) { SUB };
static PyObject * FourBand_inplace_sub(FourBand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FourBand_div(FourBand *self, PyObject *arg) { DIV };
static PyObject * FourBand_inplace_div(FourBand *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FourBand_members[] = {
    {"server", T_OBJECT_EX, offsetof(FourBand, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FourBand, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(FourBand, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(FourBand, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FourBand_methods[] = {
    {"getServer", (PyCFunction)FourBand_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FourBand_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)FourBand_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)FourBand_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)FourBand_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)FourBand_setMul, METH_O, "Sets FourBand mul factor."},
    {"setAdd", (PyCFunction)FourBand_setAdd, METH_O, "Sets FourBand add factor."},
    {"setSub", (PyCFunction)FourBand_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)FourBand_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FourBand_as_number = {
    (binaryfunc)FourBand_add,                      /*nb_add*/
    (binaryfunc)FourBand_sub,                 /*nb_subtract*/
    (binaryfunc)FourBand_multiply,                 /*nb_multiply*/
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
    (binaryfunc)FourBand_inplace_add,              /*inplace_add*/
    (binaryfunc)FourBand_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)FourBand_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)FourBand_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)FourBand_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject FourBandType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FourBand_base",         /*tp_name*/
    sizeof(FourBand),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FourBand_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &FourBand_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FourBand objects. Reads one band from a FourBandMain process.",           /* tp_doc */
    (traverseproc)FourBand_traverse,   /* tp_traverse */
    (inquiry)FourBand_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FourBand_methods,             /* tp_methods */
    FourBand_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    FourBand_new,                 /* tp_new */
};

/*****************/
/* MultiBand main */
/*****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int nbands; // 2 -> 16
    // sample memories
    double x1[240];
    double x2[240];
    double x3[240];
    double x4[240];
    double y1[240];
    double y2[240];
    double y3[240];
    double y4[240];
    // coefficients
    double b1[15];
    double b2[15];
    double b3[15];
    double b4[15];
    double la0[15];
    double la1[15];
    double la2[15];
    double ha0[15];
    double ha1[15];
    double ha2[15];
    MYFLT *buffer_streams;
} MultiBandMain;

static void
MultiBandMain_compute_variables(MultiBandMain *self, double freq, int bound)
{
    double wc = TWOPI * freq;
    double wc2 = wc * wc;
    double wc3 = wc2 * wc;
    double wc4 = wc2 * wc2;
    double k = wc / tan(PI * freq / self->sr);
    double k2 = k * k;
    double k3 = k2 * k;
    double k4 = k2 * k2;
    double sqrt2 = sqrt(2.0);
    double sq_tmp1 = sqrt2 * wc3 * k;
    double sq_tmp2 = sqrt2 * wc * k3;
    double a_tmp = 4.0 * wc2 * k2 + 2.0 * sq_tmp1 + k4 + 2.0 * sq_tmp2 + wc4;
    double wc4_a_tmp = wc4 / a_tmp;
    double k4_a_tmp = k4 / a_tmp;

    /* common */
    self->b1[bound] = (4.0 * (wc4 + sq_tmp1 - k4 - sq_tmp2)) / a_tmp;
    self->b2[bound] = (6.0 * wc4 - 8.0 * wc2 * k2 + 6.0 * k4) / a_tmp;
    self->b3[bound] = (4.0 * (wc4 - sq_tmp1 + sq_tmp2 - k4)) / a_tmp;
    self->b4[bound] = (k4 - 2.0 * sq_tmp1 + wc4 - 2.0 * sq_tmp2 + 4.0 * wc2 * k2) / a_tmp;

    /* lowpass */
    self->la0[bound] = wc4_a_tmp;
    self->la1[bound] = 4.0 * wc4_a_tmp;
    self->la2[bound] = 6.0 * wc4_a_tmp;

    /* highpass */
    self->ha0[bound] = k4_a_tmp;
    self->ha1[bound] = -4.0 * k4_a_tmp;
    self->ha2[bound] = 6.0 * k4_a_tmp;
}

static void
MultiBandMain_set_filter_frequencies(MultiBandMain *self)
{
    int i, nbounds = self->nbands - 1, minfreq = 50, maxfreq = 15000;
    int frange = maxfreq - minfreq;
    double norm, freq, step = 1.0 / self->nbands;
    for (i=0; i<nbounds; i++) {
        norm = pow((i + 1) * step, 3);
        freq = norm * frange + minfreq;
        MultiBandMain_compute_variables(self, freq, i);
    }
}

static int
MultiBandMain_splitter(MultiBandMain *self, MYFLT *input, MYFLT *outlow, MYFLT *outhigh, int bound, int filtercount) {
    int i, indl = filtercount, indh = filtercount + 1;
    double val, inval;
    for (i=0; i<self->bufsize; i++) {
        inval = (double)input[i];
        /* lowpass */
        val = self->la0[bound] * inval + self->la1[bound] * self->x1[indl] + self->la2[bound] * self->x2[indl] + 
              self->la1[bound] * self->x3[indl] + self->la0[bound] * self->x4[indl] - self->b1[bound] * self->y1[indl] - 
              self->b2[bound] * self->y2[indl] - self->b3[bound] * self->y3[indl] - self->b4[bound] * self->y4[indl];
        self->y4[indl] = self->y3[indl];
        self->y3[indl] = self->y2[indl];
        self->y2[indl] = self->y1[indl];
        self->y1[indl] = val;
        self->x4[indl] = self->x3[indl];
        self->x3[indl] = self->x2[indl];
        self->x2[indl] = self->x1[indl];
        self->x1[indl] = inval;
        outlow[i] = (MYFLT)val;

        /* highpass */
        val = self->ha0[bound] * inval + self->ha1[bound] * self->x1[indh] + self->ha2[bound] * self->x2[indh] + 
              self->ha1[bound] * self->x3[indh] + self->ha0[bound] * self->x4[indh] - self->b1[bound] * self->y1[indh] - 
              self->b2[bound] * self->y2[indh] - self->b3[bound] * self->y3[indh] - self->b4[bound] * self->y4[indh];
        self->y4[indh] = self->y3[indh];
        self->y3[indh] = self->y2[indh];
        self->y2[indh] = self->y1[indh];
        self->y1[indh] = val;
        self->x4[indh] = self->x3[indh];
        self->x3[indh] = self->x2[indh];
        self->x2[indh] = self->x1[indh];
        self->x1[indh] = inval;
        outhigh[i] = (MYFLT)val;
    }
    return filtercount + 2;
}

static int
MultiBandMain_phase_align(MultiBandMain *self, MYFLT *input, int bound, int filtercount) {
    int i, indl = filtercount, indh = filtercount + 1;
    double val, inval;
    MYFLT tmplow[self->bufsize], tmphigh[self->bufsize];

    for (i=0; i<self->bufsize; i++) {
        inval = (double)input[i];
        /* lowpass */
        val = self->la0[bound] * inval + self->la1[bound] * self->x1[indl] + self->la2[bound] * self->x2[indl] + 
              self->la1[bound] * self->x3[indl] + self->la0[bound] * self->x4[indl] - self->b1[bound] * self->y1[indl] - 
              self->b2[bound] * self->y2[indl] - self->b3[bound] * self->y3[indl] - self->b4[bound] * self->y4[indl];
        self->y4[indl] = self->y3[indl];
        self->y3[indl] = self->y2[indl];
        self->y2[indl] = self->y1[indl];
        self->y1[indl] = val;
        self->x4[indl] = self->x3[indl];
        self->x3[indl] = self->x2[indl];
        self->x2[indl] = self->x1[indl];
        self->x1[indl] = inval;
        tmplow[i] = (MYFLT)val;

        /* highpass */
        val = self->ha0[bound] * inval + self->ha1[bound] * self->x1[indh] + self->ha2[bound] * self->x2[indh] + 
              self->ha1[bound] * self->x3[indh] + self->ha0[bound] * self->x4[indh] - self->b1[bound] * self->y1[indh] - 
              self->b2[bound] * self->y2[indh] - self->b3[bound] * self->y3[indh] - self->b4[bound] * self->y4[indh];
        self->y4[indh] = self->y3[indh];
        self->y3[indh] = self->y2[indh];
        self->y2[indh] = self->y1[indh];
        self->y1[indh] = val;
        self->x4[indh] = self->x3[indh];
        self->x3[indh] = self->x2[indh];
        self->x2[indh] = self->x1[indh];
        self->x1[indh] = inval;
        tmphigh[i] = (MYFLT)val;
    }

    for (i=0; i<self->bufsize; i++) {
        input[i] = tmplow[i] + tmphigh[i];
    }

    return filtercount + 2;
}

static void
MultiBandMain_filters(MultiBandMain *self) {
    int i, bound, align, filtercount = 0;
    int bounds = self->nbands - 1;
    MYFLT *input;
    MYFLT outlow[self->bufsize], outhigh[self->bufsize];

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    input = in;
    for (bound=0; bound<bounds; bound++) {
        filtercount = MultiBandMain_splitter(self, input, outlow, outhigh, bound, filtercount);
        for (align=bound+1; align<bounds; align++) {
            filtercount = MultiBandMain_phase_align(self, outlow, align, filtercount);
        }
        for (i=0; i<self->bufsize; i++) {
            self->buffer_streams[i + bound * self->bufsize] = outlow[i];
        }
        input = outhigh;
    }
    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i + bounds * self->bufsize] = outhigh[i];
    }
}

MYFLT *
MultiBandMain_getSamplesBuffer(MultiBandMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
MultiBandMain_setProcMode(MultiBandMain *self)
{
    self->proc_func_ptr = MultiBandMain_filters;
}

static void
MultiBandMain_compute_next_data_frame(MultiBandMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
MultiBandMain_traverse(MultiBandMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
MultiBandMain_clear(MultiBandMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
MultiBandMain_dealloc(MultiBandMain* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    MultiBandMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MultiBandMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    MultiBandMain *self;
    self = (MultiBandMain *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MultiBandMain_compute_next_data_frame);
    self->mode_func_ptr = MultiBandMain_setProcMode;

    static char *kwlist[] = {"input", "num", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist, &inputtmp, &self->nbands))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->nbands < 2)
        self->nbands = 2;
    else if (self->nbands > 16)
        self->nbands = 16;

    for (i=0; i<240; i++) {
        self->x1[i] = self->x2[i] = self->x3[i] = self->x4[i] = 0.0;
        self->y1[i] = self->y2[i] = self->y3[i] = self->y4[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->b1[i] = self->b2[i] = self->b3[i] = self->b4[i] = self->la0[i] = 0.0;
        self->la1[i] = self->la2[i] = self->ha0[i] = self->ha1[i] = self->ha2[i] = 0.0;
    }

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->nbands * self->bufsize * sizeof(MYFLT));

    for (i=0; i<(self->nbands * self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    MultiBandMain_set_filter_frequencies(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
MultiBandMain_setFrequencies(MultiBandMain* self, PyObject *arg) {
    int i, bounds = self->nbands - 1;
    if PyList_Check(arg) {
        if (PyList_Size(arg) == bounds) {
            for (i=0; i<bounds; i++) {
                MultiBandMain_compute_variables(self, PyFloat_AsDouble(PyList_GetItem(arg, i)), i);
            }
        }
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * MultiBandMain_getServer(MultiBandMain* self) { GET_SERVER };
static PyObject * MultiBandMain_getStream(MultiBandMain* self) { GET_STREAM };

static PyObject * MultiBandMain_play(MultiBandMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MultiBandMain_stop(MultiBandMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef MultiBandMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(MultiBandMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MultiBandMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MultiBandMain, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MultiBandMain_methods[] = {
    {"getServer", (PyCFunction)MultiBandMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MultiBandMain_getStream, METH_NOARGS, "Returns stream object."},
    {"setFrequencies", (PyCFunction)MultiBandMain_setFrequencies, METH_O, "Sets new filter cutoff frequencies."},
    {"play", (PyCFunction)MultiBandMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MultiBandMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MultiBandMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MultiBandMain_base",                                   /*tp_name*/
    sizeof(MultiBandMain),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)MultiBandMain_dealloc,                     /*tp_dealloc*/
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
    "MultiBandMain objects. Split audio stream in four flat frequency and phase bands.",           /* tp_doc */
    (traverseproc)MultiBandMain_traverse,                  /* tp_traverse */
    (inquiry)MultiBandMain_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    MultiBandMain_methods,                                 /* tp_methods */
    MultiBandMain_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    MultiBandMain_new,                                     /* tp_new */
};

/************************************************************************************************/
/* MultiBand streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MultiBandMain *mainSplitter;
    int modebuffer[2];
    int chnl;
} MultiBand;

static void MultiBand_postprocessing_ii(MultiBand *self) { POST_PROCESSING_II };
static void MultiBand_postprocessing_ai(MultiBand *self) { POST_PROCESSING_AI };
static void MultiBand_postprocessing_ia(MultiBand *self) { POST_PROCESSING_IA };
static void MultiBand_postprocessing_aa(MultiBand *self) { POST_PROCESSING_AA };
static void MultiBand_postprocessing_ireva(MultiBand *self) { POST_PROCESSING_IREVA };
static void MultiBand_postprocessing_areva(MultiBand *self) { POST_PROCESSING_AREVA };
static void MultiBand_postprocessing_revai(MultiBand *self) { POST_PROCESSING_REVAI };
static void MultiBand_postprocessing_revaa(MultiBand *self) { POST_PROCESSING_REVAA };
static void MultiBand_postprocessing_revareva(MultiBand *self) { POST_PROCESSING_REVAREVA };

static void
MultiBand_setProcMode(MultiBand *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MultiBand_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MultiBand_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MultiBand_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MultiBand_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MultiBand_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MultiBand_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MultiBand_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MultiBand_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MultiBand_postprocessing_revareva;
            break;
    }
}

static void
MultiBand_compute_next_data_frame(MultiBand *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MultiBandMain_getSamplesBuffer((MultiBandMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MultiBand_traverse(MultiBand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
MultiBand_clear(MultiBand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
MultiBand_dealloc(MultiBand* self)
{
    pyo_DEALLOC
    MultiBand_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MultiBand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    MultiBand *self;
    self = (MultiBand *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MultiBand_compute_next_data_frame);
    self->mode_func_ptr = MultiBand_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (MultiBandMain *)maintmp;

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

static PyObject * MultiBand_getServer(MultiBand* self) { GET_SERVER };
static PyObject * MultiBand_getStream(MultiBand* self) { GET_STREAM };
static PyObject * MultiBand_setMul(MultiBand *self, PyObject *arg) { SET_MUL };
static PyObject * MultiBand_setAdd(MultiBand *self, PyObject *arg) { SET_ADD };
static PyObject * MultiBand_setSub(MultiBand *self, PyObject *arg) { SET_SUB };
static PyObject * MultiBand_setDiv(MultiBand *self, PyObject *arg) { SET_DIV };

static PyObject * MultiBand_play(MultiBand *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MultiBand_out(MultiBand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MultiBand_stop(MultiBand *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MultiBand_multiply(MultiBand *self, PyObject *arg) { MULTIPLY };
static PyObject * MultiBand_inplace_multiply(MultiBand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MultiBand_add(MultiBand *self, PyObject *arg) { ADD };
static PyObject * MultiBand_inplace_add(MultiBand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MultiBand_sub(MultiBand *self, PyObject *arg) { SUB };
static PyObject * MultiBand_inplace_sub(MultiBand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MultiBand_div(MultiBand *self, PyObject *arg) { DIV };
static PyObject * MultiBand_inplace_div(MultiBand *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MultiBand_members[] = {
    {"server", T_OBJECT_EX, offsetof(MultiBand, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MultiBand, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MultiBand, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MultiBand, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MultiBand_methods[] = {
    {"getServer", (PyCFunction)MultiBand_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MultiBand_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MultiBand_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MultiBand_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MultiBand_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MultiBand_setMul, METH_O, "Sets MultiBand mul factor."},
    {"setAdd", (PyCFunction)MultiBand_setAdd, METH_O, "Sets MultiBand add factor."},
    {"setSub", (PyCFunction)MultiBand_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MultiBand_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MultiBand_as_number = {
    (binaryfunc)MultiBand_add,                      /*nb_add*/
    (binaryfunc)MultiBand_sub,                 /*nb_subtract*/
    (binaryfunc)MultiBand_multiply,                 /*nb_multiply*/
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
    (binaryfunc)MultiBand_inplace_add,              /*inplace_add*/
    (binaryfunc)MultiBand_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)MultiBand_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)MultiBand_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)MultiBand_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject MultiBandType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MultiBand_base",         /*tp_name*/
    sizeof(MultiBand),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MultiBand_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MultiBand_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MultiBand objects. Reads one band from a MultiBandMain process.",           /* tp_doc */
    (traverseproc)MultiBand_traverse,   /* tp_traverse */
    (inquiry)MultiBand_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MultiBand_methods,             /* tp_methods */
    MultiBand_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MultiBand_new,                 /* tp_new */
};