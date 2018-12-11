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
#include "interpolation.h"
#include "fft.h"
#include "wind.h"

/************/
/* Follower */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT follow;
    MYFLT last_freq;
    MYFLT factor;
} Follower;

static void
Follower_filters_i(Follower *self) {
    MYFLT absin, freq;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);

    if (freq != self->last_freq) {
        if (freq < 0)
            freq = 0.0;
        self->factor = MYEXP(-TWOPI * freq / self->sr);
        self->last_freq = freq;
    }

    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow = self->data[i] = absin + self->factor * (self->follow - absin);
    }
}

static void
Follower_filters_a(Follower *self) {
    MYFLT freq, absin;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->last_freq) {
            if (freq < 0)
                freq = 0.0;
            self->factor = MYEXP(-TWOPI * freq / self->sr);
            self->last_freq = freq;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow = self->data[i] = absin + self->factor * (self->follow - absin);
    }
}

static void Follower_postprocessing_ii(Follower *self) { POST_PROCESSING_II };
static void Follower_postprocessing_ai(Follower *self) { POST_PROCESSING_AI };
static void Follower_postprocessing_ia(Follower *self) { POST_PROCESSING_IA };
static void Follower_postprocessing_aa(Follower *self) { POST_PROCESSING_AA };
static void Follower_postprocessing_ireva(Follower *self) { POST_PROCESSING_IREVA };
static void Follower_postprocessing_areva(Follower *self) { POST_PROCESSING_AREVA };
static void Follower_postprocessing_revai(Follower *self) { POST_PROCESSING_REVAI };
static void Follower_postprocessing_revaa(Follower *self) { POST_PROCESSING_REVAA };
static void Follower_postprocessing_revareva(Follower *self) { POST_PROCESSING_REVAREVA };

static void
Follower_setProcMode(Follower *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Follower_filters_i;
            break;
        case 1:
            self->proc_func_ptr = Follower_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Follower_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Follower_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Follower_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Follower_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Follower_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Follower_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Follower_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Follower_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Follower_postprocessing_revareva;
            break;
    }
}

static void
Follower_compute_next_data_frame(Follower *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Follower_traverse(Follower *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
Follower_clear(Follower *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
Follower_dealloc(Follower* self)
{
    pyo_DEALLOC
    Follower_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Follower_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Follower *self;
    self = (Follower *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(20);
    self->follow = 0.0;
    self->last_freq = -1.0;
    self->factor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower_compute_next_data_frame);
    self->mode_func_ptr = Follower_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

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

static PyObject * Follower_getServer(Follower* self) { GET_SERVER };
static PyObject * Follower_getStream(Follower* self) { GET_STREAM };
static PyObject * Follower_setMul(Follower *self, PyObject *arg) { SET_MUL };
static PyObject * Follower_setAdd(Follower *self, PyObject *arg) { SET_ADD };
static PyObject * Follower_setSub(Follower *self, PyObject *arg) { SET_SUB };
static PyObject * Follower_setDiv(Follower *self, PyObject *arg) { SET_DIV };

static PyObject * Follower_play(Follower *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Follower_stop(Follower *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Follower_multiply(Follower *self, PyObject *arg) { MULTIPLY };
static PyObject * Follower_inplace_multiply(Follower *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Follower_add(Follower *self, PyObject *arg) { ADD };
static PyObject * Follower_inplace_add(Follower *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Follower_sub(Follower *self, PyObject *arg) { SUB };
static PyObject * Follower_inplace_sub(Follower *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Follower_div(Follower *self, PyObject *arg) { DIV };
static PyObject * Follower_inplace_div(Follower *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Follower_setFreq(Follower *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Follower_members[] = {
{"server", T_OBJECT_EX, offsetof(Follower, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Follower, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Follower, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Follower, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Follower, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Follower, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Follower_methods[] = {
{"getServer", (PyCFunction)Follower_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Follower_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Follower_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Follower_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setFreq", (PyCFunction)Follower_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Follower_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Follower_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Follower_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Follower_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Follower_as_number = {
(binaryfunc)Follower_add,                         /*nb_add*/
(binaryfunc)Follower_sub,                         /*nb_subtract*/
(binaryfunc)Follower_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)Follower_inplace_add,                 /*inplace_add*/
(binaryfunc)Follower_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Follower_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Follower_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Follower_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject FollowerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Follower_base",                                   /*tp_name*/
sizeof(Follower),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Follower_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Follower_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Follower objects. Envelope follower.",           /* tp_doc */
(traverseproc)Follower_traverse,                  /* tp_traverse */
(inquiry)Follower_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Follower_methods,                                 /* tp_methods */
Follower_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Follower_new,                                     /* tp_new */
};

/************/
/* Follower2 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    Stream *risetime_stream;
    PyObject *falltime;
    Stream *falltime_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT follow;
    MYFLT last_risetime;
    MYFLT last_falltime;
    MYFLT risefactor;
    MYFLT fallfactor;
    MYFLT mTwoPiOverSr;
} Follower2;

static void
Follower2_filters_ii(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = 0.000001;
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = 0.000001;

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(self->mTwoPiOverSr / risetime);
        self->last_risetime = risetime;
    }

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(self->mTwoPiOverSr / falltime);
        self->last_falltime = falltime;
    }

    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_ai(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = 0.000001;

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(self->mTwoPiOverSr / falltime);
        self->last_falltime = falltime;
    }

    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = 0.000001;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(self->mTwoPiOverSr / risetime);
            self->last_risetime = risetime;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_ia(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = 0.000001;
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(self->mTwoPiOverSr / risetime);
        self->last_risetime = risetime;
    }

    for (i=0; i<self->bufsize; i++) {
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = 0.000001;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(self->mTwoPiOverSr / falltime);
            self->last_falltime = falltime;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_aa(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = 0.000001;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(self->mTwoPiOverSr / risetime);
            self->last_risetime = risetime;
        }
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = 0.000001;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(self->mTwoPiOverSr / falltime);
            self->last_falltime = falltime;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void Follower2_postprocessing_ii(Follower2 *self) { POST_PROCESSING_II };
static void Follower2_postprocessing_ai(Follower2 *self) { POST_PROCESSING_AI };
static void Follower2_postprocessing_ia(Follower2 *self) { POST_PROCESSING_IA };
static void Follower2_postprocessing_aa(Follower2 *self) { POST_PROCESSING_AA };
static void Follower2_postprocessing_ireva(Follower2 *self) { POST_PROCESSING_IREVA };
static void Follower2_postprocessing_areva(Follower2 *self) { POST_PROCESSING_AREVA };
static void Follower2_postprocessing_revai(Follower2 *self) { POST_PROCESSING_REVAI };
static void Follower2_postprocessing_revaa(Follower2 *self) { POST_PROCESSING_REVAA };
static void Follower2_postprocessing_revareva(Follower2 *self) { POST_PROCESSING_REVAREVA };

static void
Follower2_setProcMode(Follower2 *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Follower2_filters_ii;
            break;
        case 1:
            self->proc_func_ptr = Follower2_filters_ai;
            break;
        case 10:
            self->proc_func_ptr = Follower2_filters_ia;
            break;
        case 11:
            self->proc_func_ptr = Follower2_filters_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Follower2_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Follower2_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Follower2_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Follower2_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Follower2_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Follower2_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Follower2_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Follower2_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Follower2_postprocessing_revareva;
            break;
    }
}

static void
Follower2_compute_next_data_frame(Follower2 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Follower2_traverse(Follower2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);
    Py_VISIT(self->risetime_stream);
    Py_VISIT(self->falltime);
    Py_VISIT(self->falltime_stream);
    return 0;
}

static int
Follower2_clear(Follower2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);
    Py_CLEAR(self->risetime_stream);
    Py_CLEAR(self->falltime);
    Py_CLEAR(self->falltime_stream);
    return 0;
}

static void
Follower2_dealloc(Follower2* self)
{
    pyo_DEALLOC
    Follower2_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Follower2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Follower2 *self;
    self = (Follower2 *)type->tp_alloc(type, 0);

    self->risetime = PyFloat_FromDouble(0.01);
    self->falltime = PyFloat_FromDouble(0.1);
    self->follow = 0.0;
    self->last_risetime = -1.0;
    self->last_falltime = -1.0;
    self->risefactor = self->fallfactor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower2_compute_next_data_frame);
    self->mode_func_ptr = Follower2_setProcMode;

    self->mTwoPiOverSr = -TWOPI / self->sr;

    static char *kwlist[] = {"input", "risetime", "falltime", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &risetimetmp, &falltimetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRisetime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFalltime", "O", falltimetmp);
    }

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

static PyObject * Follower2_getServer(Follower2* self) { GET_SERVER };
static PyObject * Follower2_getStream(Follower2* self) { GET_STREAM };
static PyObject * Follower2_setMul(Follower2 *self, PyObject *arg) { SET_MUL };
static PyObject * Follower2_setAdd(Follower2 *self, PyObject *arg) { SET_ADD };
static PyObject * Follower2_setSub(Follower2 *self, PyObject *arg) { SET_SUB };
static PyObject * Follower2_setDiv(Follower2 *self, PyObject *arg) { SET_DIV };

static PyObject * Follower2_play(Follower2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Follower2_stop(Follower2 *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Follower2_multiply(Follower2 *self, PyObject *arg) { MULTIPLY };
static PyObject * Follower2_inplace_multiply(Follower2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Follower2_add(Follower2 *self, PyObject *arg) { ADD };
static PyObject * Follower2_inplace_add(Follower2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Follower2_sub(Follower2 *self, PyObject *arg) { SUB };
static PyObject * Follower2_inplace_sub(Follower2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Follower2_div(Follower2 *self, PyObject *arg) { DIV };
static PyObject * Follower2_inplace_div(Follower2 *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Follower2_setRisetime(Follower2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->risetime);
	if (isNumber == 1) {
		self->risetime = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->risetime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->risetime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->risetime_stream);
        self->risetime_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Follower2_setFalltime(Follower2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->falltime);
	if (isNumber == 1) {
		self->falltime = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->falltime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->falltime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->falltime_stream);
        self->falltime_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Follower2_members[] = {
    {"server", T_OBJECT_EX, offsetof(Follower2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Follower2, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Follower2, input), 0, "Input sound object."},
    {"risetime", T_OBJECT_EX, offsetof(Follower2, risetime), 0, "Risetime in second."},
    {"falltime", T_OBJECT_EX, offsetof(Follower2, falltime), 0, "Falltime in second."},
    {"mul", T_OBJECT_EX, offsetof(Follower2, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Follower2, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Follower2_methods[] = {
    {"getServer", (PyCFunction)Follower2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Follower2_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Follower2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Follower2_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setRisetime", (PyCFunction)Follower2_setRisetime, METH_O, "Sets filter risetime in second."},
    {"setFalltime", (PyCFunction)Follower2_setFalltime, METH_O, "Sets filter falltime in second."},
    {"setMul", (PyCFunction)Follower2_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Follower2_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Follower2_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Follower2_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Follower2_as_number = {
    (binaryfunc)Follower2_add,                         /*nb_add*/
    (binaryfunc)Follower2_sub,                         /*nb_subtract*/
    (binaryfunc)Follower2_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)Follower2_inplace_add,                 /*inplace_add*/
    (binaryfunc)Follower2_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Follower2_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Follower2_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Follower2_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject Follower2Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Follower2_base",                                   /*tp_name*/
    sizeof(Follower2),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Follower2_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Follower2_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Follower2 objects. Envelope follower.",           /* tp_doc */
    (traverseproc)Follower2_traverse,                  /* tp_traverse */
    (inquiry)Follower2_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Follower2_methods,                                 /* tp_methods */
    Follower2_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Follower2_new,                                     /* tp_new */
};

/************/
/* ZCross */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT thresh;
    MYFLT lastValue;
    MYFLT lastSample;
    int modebuffer[2]; // need at least 2 slots for mul & add
} ZCross;

static void
ZCross_process(ZCross *self) {
    int i;
    int count = 0;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->lastValue;
        inval = in[i];
        if (self->lastSample >= 0.0) {
            if (inval < 0.0 && (self->lastSample-inval) > self->thresh)
                count++;
        }
        else {
            if (inval >= 0.0 && (inval-self->lastSample) > self->thresh)
                count++;
        }
        self->lastSample = inval;
    }
    self->lastValue = (MYFLT)count / self->bufsize;
}

static void ZCross_postprocessing_ii(ZCross *self) { POST_PROCESSING_II };
static void ZCross_postprocessing_ai(ZCross *self) { POST_PROCESSING_AI };
static void ZCross_postprocessing_ia(ZCross *self) { POST_PROCESSING_IA };
static void ZCross_postprocessing_aa(ZCross *self) { POST_PROCESSING_AA };
static void ZCross_postprocessing_ireva(ZCross *self) { POST_PROCESSING_IREVA };
static void ZCross_postprocessing_areva(ZCross *self) { POST_PROCESSING_AREVA };
static void ZCross_postprocessing_revai(ZCross *self) { POST_PROCESSING_REVAI };
static void ZCross_postprocessing_revaa(ZCross *self) { POST_PROCESSING_REVAA };
static void ZCross_postprocessing_revareva(ZCross *self) { POST_PROCESSING_REVAREVA };

static void
ZCross_setProcMode(ZCross *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = ZCross_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = ZCross_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = ZCross_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = ZCross_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = ZCross_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = ZCross_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = ZCross_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = ZCross_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = ZCross_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = ZCross_postprocessing_revareva;
            break;
    }
}

static void
ZCross_compute_next_data_frame(ZCross *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ZCross_traverse(ZCross *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
ZCross_clear(ZCross *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
ZCross_dealloc(ZCross* self)
{
    pyo_DEALLOC
    ZCross_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ZCross_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    ZCross *self;
    self = (ZCross *)type->tp_alloc(type, 0);

    self->thresh = 0.0;
    self->lastValue = self->lastSample = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ZCross_compute_next_data_frame);
    self->mode_func_ptr = ZCross_setProcMode;

    static char *kwlist[] = {"input", "thresh", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FOO, kwlist, &inputtmp, &self->thresh, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

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

static PyObject * ZCross_getServer(ZCross* self) { GET_SERVER };
static PyObject * ZCross_getStream(ZCross* self) { GET_STREAM };
static PyObject * ZCross_setMul(ZCross *self, PyObject *arg) { SET_MUL };
static PyObject * ZCross_setAdd(ZCross *self, PyObject *arg) { SET_ADD };
static PyObject * ZCross_setSub(ZCross *self, PyObject *arg) { SET_SUB };
static PyObject * ZCross_setDiv(ZCross *self, PyObject *arg) { SET_DIV };

static PyObject * ZCross_play(ZCross *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ZCross_stop(ZCross *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * ZCross_multiply(ZCross *self, PyObject *arg) { MULTIPLY };
static PyObject * ZCross_inplace_multiply(ZCross *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ZCross_add(ZCross *self, PyObject *arg) { ADD };
static PyObject * ZCross_inplace_add(ZCross *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ZCross_sub(ZCross *self, PyObject *arg) { SUB };
static PyObject * ZCross_inplace_sub(ZCross *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ZCross_div(ZCross *self, PyObject *arg) { DIV };
static PyObject * ZCross_inplace_div(ZCross *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ZCross_setThresh(ZCross *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->thresh = PyFloat_AsDouble(arg);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef ZCross_members[] = {
{"server", T_OBJECT_EX, offsetof(ZCross, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(ZCross, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(ZCross, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(ZCross, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(ZCross, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef ZCross_methods[] = {
{"getServer", (PyCFunction)ZCross_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)ZCross_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)ZCross_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)ZCross_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setThresh", (PyCFunction)ZCross_setThresh, METH_O, "Sets the threshold factor."},
{"setMul", (PyCFunction)ZCross_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)ZCross_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)ZCross_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)ZCross_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods ZCross_as_number = {
(binaryfunc)ZCross_add,                         /*nb_add*/
(binaryfunc)ZCross_sub,                         /*nb_subtract*/
(binaryfunc)ZCross_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)ZCross_inplace_add,                 /*inplace_add*/
(binaryfunc)ZCross_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)ZCross_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)ZCross_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)ZCross_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ZCrossType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.ZCross_base",                                   /*tp_name*/
sizeof(ZCross),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)ZCross_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&ZCross_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"ZCross objects. Returns number of zero crossing.",           /* tp_doc */
(traverseproc)ZCross_traverse,                  /* tp_traverse */
(inquiry)ZCross_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
ZCross_methods,                                 /* tp_methods */
ZCross_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
ZCross_new,                                     /* tp_new */
};

static int
min_elem_pos(MYFLT *buf, int size ) {
    int i;
    int pos = 0;
    for (i=1; i<size; i++) {
        if (buf[i] < buf[pos])
            pos = i;
    }
    return pos;
}

static MYFLT
quadraticInterpolation(MYFLT *buf, int period, int size) {
    int x0, x2;
    MYFLT pitch, s0, s1, s2;
    x0 = period < 1 ? period : period - 1;
    x2 = period + 1 < size ? period + 1 : period;
    if (x0 == period)
        pitch = buf[period] <= buf[x2] ? period : x2;
    else if (x2 == period)
        pitch = buf[period] <= buf[x0] ? period : x0;
    else {
        s0 = buf[x0];
        s1 = buf[period];
        s2 = buf[x2];
        pitch = period + 0.5 * (s2 - s0) / (s2 - 2.0 * s1 + s0);
    }
    return pitch;
}

/************/
/* Yin */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT *input_buffer;
    MYFLT *yin_buffer;
    int winsize;
    int halfsize;
    int input_count;
    MYFLT tolerance;
    MYFLT pitch;
    MYFLT minfreq;
    MYFLT maxfreq;
    MYFLT cutoff;
    MYFLT last_cutoff;
    MYFLT y1;
    MYFLT c2;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Yin;

static void
Yin_process(Yin *self) {
    int i, j, period, tau = 0;
    MYFLT candidate, tmp = 0.0, tmp2 = 0.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->cutoff != self->last_cutoff) {
        if (self->cutoff <= 1.0)
            self->cutoff = 1.0;
        else if (self->cutoff >= self->sr*0.5)
            self->cutoff = self->sr*0.5;
        self->last_cutoff = self->cutoff;
        self->c2 = MYEXP(-TWOPI * self->cutoff / self->sr);
    }

    for (i=0; i<self->bufsize; i++) {
        self->y1 = in[i] + (self->y1 - in[i]) * self->c2;
        self->input_buffer[self->input_count] = self->y1;
        if (self->input_count++ == self->winsize) {
            self->input_count = 0;

            self->yin_buffer[0] = 1.0;
            for (tau = 1; tau < self->halfsize; tau++) {
                self->yin_buffer[tau] = 0.0;
                for (j = 0; j < self->halfsize; j++) {
                    tmp = self->input_buffer[j] - self->input_buffer[j+tau];
                    self->yin_buffer[tau] += tmp*tmp;
                }
                tmp2 += self->yin_buffer[tau];
                self->yin_buffer[tau] *= tau / tmp2;
                period = tau - 3;
                if (tau > 4 && (self->yin_buffer[period] < self->tolerance) &&
                    (self->yin_buffer[period] < self->yin_buffer[period+1])) {
                    candidate = quadraticInterpolation(self->yin_buffer, period, self->halfsize);
                    goto founded;
                }
            }
            candidate = quadraticInterpolation(self->yin_buffer, min_elem_pos(self->yin_buffer, self->halfsize), self->halfsize);

        founded:

            candidate = self->sr / candidate;
            if (candidate > self->minfreq && candidate < self->maxfreq)
                self->pitch = candidate;

        }
        self->data[i] = self->pitch;
    }
}

static void Yin_postprocessing_ii(Yin *self) { POST_PROCESSING_II };
static void Yin_postprocessing_ai(Yin *self) { POST_PROCESSING_AI };
static void Yin_postprocessing_ia(Yin *self) { POST_PROCESSING_IA };
static void Yin_postprocessing_aa(Yin *self) { POST_PROCESSING_AA };
static void Yin_postprocessing_ireva(Yin *self) { POST_PROCESSING_IREVA };
static void Yin_postprocessing_areva(Yin *self) { POST_PROCESSING_AREVA };
static void Yin_postprocessing_revai(Yin *self) { POST_PROCESSING_REVAI };
static void Yin_postprocessing_revaa(Yin *self) { POST_PROCESSING_REVAA };
static void Yin_postprocessing_revareva(Yin *self) { POST_PROCESSING_REVAREVA };

static void
Yin_setProcMode(Yin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Yin_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Yin_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Yin_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Yin_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Yin_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Yin_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Yin_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Yin_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Yin_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Yin_postprocessing_revareva;
            break;
    }
}

static void
Yin_compute_next_data_frame(Yin *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Yin_traverse(Yin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Yin_clear(Yin *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Yin_dealloc(Yin* self)
{
    pyo_DEALLOC
    free(self->input_buffer);
    free(self->yin_buffer);
    Yin_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Yin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Yin *self;
    self = (Yin *)type->tp_alloc(type, 0);

    self->winsize = 1024;
    self->halfsize = 512;
    self->input_count = 0;
    self->pitch = 0.;
    self->tolerance = 0.15;
    self->minfreq = 40;
    self->maxfreq = 1000;
    self->cutoff = 1000;
    self->last_cutoff = -1.0;
    self->y1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Yin_compute_next_data_frame);
    self->mode_func_ptr = Yin_setProcMode;

    static char *kwlist[] = {"input", "tolerance", "minfreq", "maxfreq", "cutoff", "winsize", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFIOO, kwlist, &inputtmp, &self->tolerance, &self->minfreq, &self->maxfreq, &self->cutoff, &self->winsize, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->winsize % 2 == 1)
        self->winsize += 1;

    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->winsize * sizeof(MYFLT));
    for (i=0; i<self->winsize; i++)
        self->input_buffer[i] = 0.0;

    self->halfsize = self->winsize / 2;
    self->yin_buffer = (MYFLT *)realloc(self->yin_buffer, self->halfsize * sizeof(MYFLT));
    for (i=0; i<self->halfsize; i++)
        self->yin_buffer[i] = 0.0;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Yin_getServer(Yin* self) { GET_SERVER };
static PyObject * Yin_getStream(Yin* self) { GET_STREAM };
static PyObject * Yin_setMul(Yin *self, PyObject *arg) { SET_MUL };
static PyObject * Yin_setAdd(Yin *self, PyObject *arg) { SET_ADD };
static PyObject * Yin_setSub(Yin *self, PyObject *arg) { SET_SUB };
static PyObject * Yin_setDiv(Yin *self, PyObject *arg) { SET_DIV };

static PyObject * Yin_play(Yin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Yin_stop(Yin *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Yin_multiply(Yin *self, PyObject *arg) { MULTIPLY };
static PyObject * Yin_inplace_multiply(Yin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Yin_add(Yin *self, PyObject *arg) { ADD };
static PyObject * Yin_inplace_add(Yin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Yin_sub(Yin *self, PyObject *arg) { SUB };
static PyObject * Yin_inplace_sub(Yin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Yin_div(Yin *self, PyObject *arg) { DIV };
static PyObject * Yin_inplace_div(Yin *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Yin_setTolerance(Yin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->tolerance = PyFloat_AsDouble(arg);
	}

	Py_RETURN_NONE;
}

static PyObject *
Yin_setMinfreq(Yin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->minfreq = PyFloat_AsDouble(arg);
	}

	Py_RETURN_NONE;
}

static PyObject *
Yin_setMaxfreq(Yin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->maxfreq = PyFloat_AsDouble(arg);
	}

	Py_RETURN_NONE;
}

static PyObject *
Yin_setCutoff(Yin *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->cutoff = PyFloat_AsDouble(arg);
	}

	Py_RETURN_NONE;
}

static PyMemberDef Yin_members[] = {
{"server", T_OBJECT_EX, offsetof(Yin, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Yin, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Yin, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Yin, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Yin, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Yin_methods[] = {
{"getServer", (PyCFunction)Yin_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Yin_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Yin_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Yin_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setTolerance", (PyCFunction)Yin_setTolerance, METH_O, "Sets the tolerance factor."},
{"setMinfreq", (PyCFunction)Yin_setMinfreq, METH_O, "Sets the minimum frequency in output."},
{"setMaxfreq", (PyCFunction)Yin_setMaxfreq, METH_O, "Sets the maximum frequency in output."},
{"setCutoff", (PyCFunction)Yin_setCutoff, METH_O, "Sets the input lowpass filter cutoff frequency."},
{"setMul", (PyCFunction)Yin_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Yin_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Yin_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Yin_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Yin_as_number = {
(binaryfunc)Yin_add,                         /*nb_add*/
(binaryfunc)Yin_sub,                         /*nb_subtract*/
(binaryfunc)Yin_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)Yin_inplace_add,                 /*inplace_add*/
(binaryfunc)Yin_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Yin_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Yin_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Yin_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject YinType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Yin_base",                                   /*tp_name*/
sizeof(Yin),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Yin_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Yin_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Yin objects. Pitch tracker using the Yin algorithm.",           /* tp_doc */
(traverseproc)Yin_traverse,                  /* tp_traverse */
(inquiry)Yin_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Yin_methods,                                 /* tp_methods */
Yin_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Yin_new,                                     /* tp_new */
};

/********************/
/* Centroid */
/********************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int size;
    int hsize;
    int incount;
    MYFLT centroid;
    MYFLT *inframe;
    MYFLT *outframe;
    MYFLT **twiddle;
    MYFLT *input_buffer;
    MYFLT *window;
    int modebuffer[2];
} Centroid;

static void
Centroid_alloc_memories(Centroid *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));
    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++)
        self->inframe[i] = self->outframe[i] = self->input_buffer[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, 2);
}

static void
Centroid_process_i(Centroid *self) {
    int i;
    MYFLT re, im, tmp, sum1, sum2;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->input_buffer[self->incount] = in[i];
        self->data[i] = self->centroid;

        self->incount++;
        if (self->incount == self->size) {
            self->incount = self->hsize;

            for (i=0; i<self->size; i++) {
                self->inframe[i] = self->input_buffer[i] * self->window[i];
            }
            realfft_split(self->inframe, self->outframe, self->size, self->twiddle);
            sum1 = sum2 = 0.0;
            for (i=1; i<self->hsize; i++) {
                re = self->outframe[i];
                im = self->outframe[self->size - i];
                tmp = MYSQRT(re*re + im*im);
                sum1 += tmp * i;
                sum2 += tmp;
            }
            if (sum2 < 0.000000001)
                tmp = 0.0;
            else
                tmp = sum1 / sum2;
            self->centroid += tmp * self->sr / self->size;
            self->centroid *= 0.5;
            for (i=0; i<self->hsize; i++) {
                self->input_buffer[i] = self->input_buffer[i + self->hsize];
            }
        }
    }
}

static void Centroid_postprocessing_ii(Centroid *self) { POST_PROCESSING_II };
static void Centroid_postprocessing_ai(Centroid *self) { POST_PROCESSING_AI };
static void Centroid_postprocessing_ia(Centroid *self) { POST_PROCESSING_IA };
static void Centroid_postprocessing_aa(Centroid *self) { POST_PROCESSING_AA };
static void Centroid_postprocessing_ireva(Centroid *self) { POST_PROCESSING_IREVA };
static void Centroid_postprocessing_areva(Centroid *self) { POST_PROCESSING_AREVA };
static void Centroid_postprocessing_revai(Centroid *self) { POST_PROCESSING_REVAI };
static void Centroid_postprocessing_revaa(Centroid *self) { POST_PROCESSING_REVAA };
static void Centroid_postprocessing_revareva(Centroid *self) { POST_PROCESSING_REVAREVA };

static void
Centroid_setProcMode(Centroid *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Centroid_process_i;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Centroid_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Centroid_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Centroid_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Centroid_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Centroid_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Centroid_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Centroid_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Centroid_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Centroid_postprocessing_revareva;
            break;
    }
}

static void
Centroid_compute_next_data_frame(Centroid *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Centroid_traverse(Centroid *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Centroid_clear(Centroid *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Centroid_dealloc(Centroid* self)
{
    int i;
    pyo_DEALLOC
    free(self->inframe);
    free(self->outframe);
    free(self->input_buffer);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    free(self->window);
    Centroid_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Centroid_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, k;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Centroid *self;
    self = (Centroid *)type->tp_alloc(type, 0);

    self->centroid = 0;
    self->size = 1024;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Centroid_compute_next_data_frame);
    self->mode_func_ptr = Centroid_setProcMode;

    static char *kwlist[] = {"input", "size", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->size, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (self->size < self->bufsize) {
        PySys_WriteStdout("Warning : Centroid size less than buffer size!\nCentroid size set to buffersize: %d\n", self->bufsize);
        self->size = self->bufsize;
    }

    k = 1;
    while (k < self->size)
        k <<= 1;
    self->size = k;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Centroid_alloc_memories(self);

    self->incount = self->hsize;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Centroid_getServer(Centroid* self) { GET_SERVER };
static PyObject * Centroid_getStream(Centroid* self) { GET_STREAM };
static PyObject * Centroid_setMul(Centroid *self, PyObject *arg) { SET_MUL };
static PyObject * Centroid_setAdd(Centroid *self, PyObject *arg) { SET_ADD };
static PyObject * Centroid_setSub(Centroid *self, PyObject *arg) { SET_SUB };
static PyObject * Centroid_setDiv(Centroid *self, PyObject *arg) { SET_DIV };

static PyObject * Centroid_play(Centroid *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Centroid_stop(Centroid *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Centroid_multiply(Centroid *self, PyObject *arg) { MULTIPLY };
static PyObject * Centroid_inplace_multiply(Centroid *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Centroid_add(Centroid *self, PyObject *arg) { ADD };
static PyObject * Centroid_inplace_add(Centroid *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Centroid_sub(Centroid *self, PyObject *arg) { SUB };
static PyObject * Centroid_inplace_sub(Centroid *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Centroid_div(Centroid *self, PyObject *arg) { DIV };
static PyObject * Centroid_inplace_div(Centroid *self, PyObject *arg) { INPLACE_DIV };


static PyMemberDef Centroid_members[] = {
{"server", T_OBJECT_EX, offsetof(Centroid, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Centroid, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Centroid, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Centroid, add), 0, "Add factor."},
{"input", T_OBJECT_EX, offsetof(Centroid, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Centroid_methods[] = {
{"getServer", (PyCFunction)Centroid_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Centroid_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Centroid_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Centroid_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)Centroid_setMul, METH_O, "Sets Centroid mul factor."},
{"setAdd", (PyCFunction)Centroid_setAdd, METH_O, "Sets Centroid add factor."},
{"setSub", (PyCFunction)Centroid_setSub, METH_O, "Sets Centroid add factor."},
{"setDiv", (PyCFunction)Centroid_setDiv, METH_O, "Sets Centroid mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Centroid_as_number = {
    (binaryfunc)Centroid_add,                      /*nb_add*/
    (binaryfunc)Centroid_sub,                 /*nb_subtract*/
    (binaryfunc)Centroid_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Centroid_inplace_add,              /*inplace_add*/
    (binaryfunc)Centroid_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Centroid_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Centroid_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Centroid_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject CentroidType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Centroid_base",                                   /*tp_name*/
sizeof(Centroid),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Centroid_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Centroid_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Centroid objects. FFT transform.",           /* tp_doc */
(traverseproc)Centroid_traverse,                  /* tp_traverse */
(inquiry)Centroid_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Centroid_methods,                                 /* tp_methods */
Centroid_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Centroid_new,                                     /* tp_new */
};

/************/
/* AttackDetector */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT deltime;
    MYFLT cutoff;
    MYFLT maxthresh;
    MYFLT minthresh;
    MYFLT reltime;
    MYFLT folfactor;
    MYFLT follow;
    MYFLT followdb;
    MYFLT *buffer;
    MYFLT previous;
    int memsize;
    int sampdel;
    int incount;
    int overminok;
    int belowminok;
    long maxtime;
    long timer;
    int modebuffer[2]; // need at least 2 slots for mul & add
} AttackDetector;

static void
AttackDetector_process(AttackDetector *self) {
    int i, ind;
    MYFLT absin;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        absin = in[i];
        // envelope follower
        if (absin < 0.0)
            absin = -absin;
        self->follow = absin + self->folfactor * (self->follow - absin);
        // follower in dB
        if (self->follow <= 0.000001)
            self->followdb = -120.0;
        else
            self->followdb = 20.0 * MYLOG10(self->follow);
        // previous analysis
        ind = self->incount - self->sampdel;
        if (ind < 0)
            ind += self->memsize;
        self->previous = self->buffer[ind];
        self->buffer[self->incount] = self->followdb;
        self->incount++;
        if (self->incount >= self->memsize)
            self->incount = 0;
        // if release time has past
        if (self->timer >= self->maxtime) {
            // if rms is over min threshold
            if (self->overminok) {
                // if rms is greater than previous + maxthresh
                if (self->followdb > (self->previous + self->maxthresh)) {
                    self->data[i] = 1.0;
                    self->overminok = self->belowminok = 0;
                    self->timer = 0;
                }
            }
        }
        if (self->belowminok == 0 && self->followdb < self->minthresh)
            self->belowminok = 1;
        else if (self->belowminok == 1 && self->followdb > self->minthresh)
            self->overminok = 1;
        self->timer++;
    }
}

static void AttackDetector_postprocessing_ii(AttackDetector *self) { POST_PROCESSING_II };
static void AttackDetector_postprocessing_ai(AttackDetector *self) { POST_PROCESSING_AI };
static void AttackDetector_postprocessing_ia(AttackDetector *self) { POST_PROCESSING_IA };
static void AttackDetector_postprocessing_aa(AttackDetector *self) { POST_PROCESSING_AA };
static void AttackDetector_postprocessing_ireva(AttackDetector *self) { POST_PROCESSING_IREVA };
static void AttackDetector_postprocessing_areva(AttackDetector *self) { POST_PROCESSING_AREVA };
static void AttackDetector_postprocessing_revai(AttackDetector *self) { POST_PROCESSING_REVAI };
static void AttackDetector_postprocessing_revaa(AttackDetector *self) { POST_PROCESSING_REVAA };
static void AttackDetector_postprocessing_revareva(AttackDetector *self) { POST_PROCESSING_REVAREVA };

static void
AttackDetector_setProcMode(AttackDetector *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = AttackDetector_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = AttackDetector_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = AttackDetector_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = AttackDetector_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = AttackDetector_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = AttackDetector_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = AttackDetector_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = AttackDetector_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = AttackDetector_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = AttackDetector_postprocessing_revareva;
            break;
    }
}

static void
AttackDetector_compute_next_data_frame(AttackDetector *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
AttackDetector_traverse(AttackDetector *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
AttackDetector_clear(AttackDetector *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
AttackDetector_dealloc(AttackDetector* self)
{
    pyo_DEALLOC
    free(self->buffer);
    AttackDetector_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
AttackDetector_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    AttackDetector *self;
    self = (AttackDetector *)type->tp_alloc(type, 0);

    self->deltime = 0.005;
    self->cutoff = 10.0;
    self->maxthresh = 3.0;
    self->minthresh = -30.0;
    self->reltime = 0.1;
    self->follow = 0.0;
    self->followdb = -120.0;
    self->previous = 0.0;
    self->incount = 0;
    self->overminok = 0;
    self->belowminok = 0;
    self->timer = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, AttackDetector_compute_next_data_frame);
    self->mode_func_ptr = AttackDetector_setProcMode;

    static char *kwlist[] = {"input", "deltime", "cutoff", "maxthresh", "minthresh", "reltime", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFFOO, kwlist, &inputtmp, &self->deltime, &self->cutoff, &self->maxthresh, &self->minthresh, &self->reltime, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->memsize = (int)(0.055 * self->sr + 0.5);
    self->buffer = (MYFLT *)realloc(self->buffer, (self->memsize+1) * sizeof(MYFLT));
    for (i=0; i<(self->memsize+1); i++) {
        self->buffer[i] = 0.0;
    }

    if (self->deltime < 0.001) self->deltime = 0.001;
    else if (self->deltime > 0.05) self->deltime = 0.05;
    self->sampdel = (int)(self->deltime * self->sr);

    if (self->cutoff < 1.0) self->cutoff = 1.0;
    else if (self->cutoff > 1000.0) self->cutoff = 1000.0;
    self->folfactor = MYEXP(-TWOPI * self->cutoff / self->sr);

    if (self->cutoff < 1.0) self->cutoff = 1.0;
    else if (self->cutoff > 1000.0) self->cutoff = 1000.0;

    if (self->maxthresh < 0.0) self->maxthresh = 0.0;
    else if (self->maxthresh > 18.0) self->maxthresh = 18.0;

    if (self->minthresh < -90.0) self->minthresh = -90.0;
    else if (self->minthresh > 0.0) self->minthresh = 0.0;

    if (self->reltime < 0.001) self->reltime = 0.001;
    self->maxtime = (long)(self->reltime * self->sr + 0.5);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * AttackDetector_getServer(AttackDetector* self) { GET_SERVER };
static PyObject * AttackDetector_getStream(AttackDetector* self) { GET_STREAM };
static PyObject * AttackDetector_setMul(AttackDetector *self, PyObject *arg) { SET_MUL };
static PyObject * AttackDetector_setAdd(AttackDetector *self, PyObject *arg) { SET_ADD };
static PyObject * AttackDetector_setSub(AttackDetector *self, PyObject *arg) { SET_SUB };
static PyObject * AttackDetector_setDiv(AttackDetector *self, PyObject *arg) { SET_DIV };

static PyObject * AttackDetector_play(AttackDetector *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * AttackDetector_stop(AttackDetector *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * AttackDetector_multiply(AttackDetector *self, PyObject *arg) { MULTIPLY };
static PyObject * AttackDetector_inplace_multiply(AttackDetector *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * AttackDetector_add(AttackDetector *self, PyObject *arg) { ADD };
static PyObject * AttackDetector_inplace_add(AttackDetector *self, PyObject *arg) { INPLACE_ADD };
static PyObject * AttackDetector_sub(AttackDetector *self, PyObject *arg) { SUB };
static PyObject * AttackDetector_inplace_sub(AttackDetector *self, PyObject *arg) { INPLACE_SUB };
static PyObject * AttackDetector_div(AttackDetector *self, PyObject *arg) { DIV };
static PyObject * AttackDetector_inplace_div(AttackDetector *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
AttackDetector_setDeltime(AttackDetector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->deltime = PyFloat_AsDouble(arg);
        if (self->deltime < 0.001) self->deltime = 0.001;
        else if (self->deltime > 0.05) self->deltime = 0.05;
        self->sampdel = (int)(self->deltime * self->sr);
	}

	Py_RETURN_NONE;
}

static PyObject *
AttackDetector_setCutoff(AttackDetector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->cutoff = PyFloat_AsDouble(arg);
        if (self->cutoff < 1.0) self->cutoff = 1.0;
        else if (self->cutoff > 1000.0) self->cutoff = 1000.0;
        self->folfactor = MYEXP(-TWOPI * self->cutoff / self->sr);
	}

	Py_RETURN_NONE;
}

static PyObject *
AttackDetector_setMaxthresh(AttackDetector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->maxthresh = PyFloat_AsDouble(arg);
        if (self->maxthresh < 0.0) self->maxthresh = 0.0;
        else if (self->maxthresh > 18.0) self->maxthresh = 18.0;
	}

	Py_RETURN_NONE;
}

static PyObject *
AttackDetector_setMinthresh(AttackDetector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->minthresh = PyFloat_AsDouble(arg);
        if (self->minthresh < -90.0) self->minthresh = -90.0;
        else if (self->minthresh > 0.0) self->minthresh = 0.0;
	}

	Py_RETURN_NONE;
}

static PyObject *
AttackDetector_setReltime(AttackDetector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->reltime = PyFloat_AsDouble(arg);
        if (self->reltime < 0.001) self->reltime = 0.001;
        self->maxtime = (long)(self->reltime * self->sr + 0.5);
	}

	Py_RETURN_NONE;
}

static PyObject *
AttackDetector_readyToDetect(AttackDetector *self)
{
    self->overminok = 1;
    self->timer = self->maxtime;
	Py_RETURN_NONE;
}

static PyMemberDef AttackDetector_members[] = {
{"server", T_OBJECT_EX, offsetof(AttackDetector, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(AttackDetector, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(AttackDetector, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(AttackDetector, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(AttackDetector, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef AttackDetector_methods[] = {
{"getServer", (PyCFunction)AttackDetector_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)AttackDetector_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)AttackDetector_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)AttackDetector_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setDeltime", (PyCFunction)AttackDetector_setDeltime, METH_O, "Sets the delay time between current and previous analysis."},
{"setCutoff", (PyCFunction)AttackDetector_setCutoff, METH_O, "Sets the frequency of the internal lowpass filter."},
{"setMaxthresh", (PyCFunction)AttackDetector_setMaxthresh, METH_O, "Sets the higher threshold."},
{"setMinthresh", (PyCFunction)AttackDetector_setMinthresh, METH_O, "Sets the lower threshold."},
{"setReltime", (PyCFunction)AttackDetector_setReltime, METH_O, "Sets the release time (min time between two detected attacks)."},
{"readyToDetect", (PyCFunction)AttackDetector_readyToDetect, METH_NOARGS, "Initializes thresholds."},
{"setMul", (PyCFunction)AttackDetector_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)AttackDetector_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)AttackDetector_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)AttackDetector_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods AttackDetector_as_number = {
(binaryfunc)AttackDetector_add,                         /*nb_add*/
(binaryfunc)AttackDetector_sub,                         /*nb_subtract*/
(binaryfunc)AttackDetector_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)AttackDetector_inplace_add,                 /*inplace_add*/
(binaryfunc)AttackDetector_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)AttackDetector_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)AttackDetector_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)AttackDetector_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject AttackDetectorType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.AttackDetector_base",                                   /*tp_name*/
sizeof(AttackDetector),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)AttackDetector_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&AttackDetector_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"AttackDetector objects. Audio signal peak detection.",           /* tp_doc */
(traverseproc)AttackDetector_traverse,                  /* tp_traverse */
(inquiry)AttackDetector_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
AttackDetector_methods,                                 /* tp_methods */
AttackDetector_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
AttackDetector_new,                                     /* tp_new */
};

/**********************************/
/********* Scope ******************/
/**********************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *func;
    int size;
    int newsize;
    int width;
    int height;
    int pointer;
    int poll;
    MYFLT gain;
    MYFLT *buffer;
} Scope;

static void
Scope_generate(Scope *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    for (i=0; i<self->bufsize; i++) {
        if (self->pointer >= self->size) {
            if (self->func != Py_None && self->poll) {
                PyObject_Call(self->func, PyTuple_New(0), NULL);
            }
            self->pointer = 0;
            if (self->newsize != self->size)
                self->size = self->newsize;
        }
        self->buffer[self->pointer] = in[i];
        self->pointer++;
    }
}

static PyObject *
Scope_display(Scope *self) {
    int i, ipos;
    MYFLT pos, step, mag, h2;
    PyObject *points, *tuple;

    step = self->size / (MYFLT)(self->width);
    h2 = self->height * 0.5;

    points = PyList_New(self->width);

    for (i=0; i<self->width; i++) {
        pos = i * step;
        ipos = (int)pos;
        tuple = PyTuple_New(2);
        mag = ((self->buffer[ipos] + (self->buffer[ipos+1] - self->buffer[ipos]) * (pos - ipos)) * self->gain * h2 + h2);
        PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(i));
        PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(self->height - (int)mag));
        PyList_SET_ITEM(points, i, tuple);
    }
    return points;
}

static void
Scope_compute_next_data_frame(Scope *self)
{
    Scope_generate(self);
}

static int
Scope_traverse(Scope *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    if (self->func != Py_None) {
        Py_VISIT(self->func);
    }
    return 0;
}

static int
Scope_clear(Scope *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    if (self->func != Py_None) {
        Py_CLEAR(self->func);
    }
    return 0;
}

static void
Scope_dealloc(Scope* self)
{
    pyo_DEALLOC
    free(self->buffer);
    Scope_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Scope_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, maxsize, target;
    MYFLT length = 0.05;
    PyObject *inputtmp, *input_streamtmp;
    Scope *self;
    self = (Scope *)type->tp_alloc(type, 0);

    self->gain = 1.0;
    self->width = 500;
    self->height = 400;
    self->poll = 1;

    self->func = Py_None;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Scope_compute_next_data_frame);

    static char *kwlist[] = {"input", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_F, kwlist, &inputtmp, &length))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    maxsize = (int)(self->sr);
    self->buffer = (MYFLT *)realloc(self->buffer, maxsize * sizeof(MYFLT));

    self->size = 0;
    target = (int)(length * self->sr);
    while (self->size < target) {
        self->size += self->bufsize;
    }
    self->size -= self->bufsize;
    if (self->size < self->bufsize)
        self->size += self->bufsize;
    else if (self->size > maxsize)
        self->size = maxsize;
    self->newsize = self->size;
    self->pointer = 0;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
}

static PyObject * Scope_getServer(Scope* self) { GET_SERVER };
static PyObject * Scope_getStream(Scope* self) { GET_STREAM };

static PyObject * Scope_play(Scope *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Scope_stop(Scope *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Scope_setLength(Scope *self, PyObject *arg)
{
    int target;
    MYFLT length;
    int maxsize = (int)(self->sr);

    if (PyNumber_Check(arg)) {
        length = PyFloat_AsDouble(arg);
        self->newsize = 0;
        target = (int)(length * self->sr);
        while (self->newsize < target) {
            self->newsize += self->bufsize;
        }
        self->newsize -= self->bufsize;
    if (self->newsize < self->bufsize)
        self->newsize += self->bufsize;
    else if (self->newsize > maxsize)
        self->newsize = maxsize;
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Scope_setGain(Scope *self, PyObject *arg)
{
    if (PyNumber_Check(arg)) {
        self->gain = PyFloat_AsDouble(arg);
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Scope_setWidth(Scope *self, PyObject *arg)
{
    if (PyInt_Check(arg)) {
        self->width = PyInt_AsLong(arg);
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Scope_setHeight(Scope *self, PyObject *arg)
{
    if (PyInt_Check(arg)) {
        self->height = PyInt_AsLong(arg);
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Scope_setFunc(Scope *self, PyObject *arg)
{
	PyObject *tmp;

	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The function attribute must be callable.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    Py_XDECREF(self->func);
    Py_INCREF(tmp);
    self->func = tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Scope_setPoll(Scope *self, PyObject *arg)
{
    if (PyInt_Check(arg)) {
        self->poll = PyInt_AsLong(arg);
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Scope_members[] = {
{"server", T_OBJECT_EX, offsetof(Scope, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Scope, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Scope, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Scope_methods[] = {
{"getServer", (PyCFunction)Scope_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Scope_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Scope_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Scope_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"display", (PyCFunction)Scope_display, METH_NOARGS, "Computes the samples to draw."},
{"setLength", (PyCFunction)Scope_setLength, METH_O, "Sets function's argument."},
{"setGain", (PyCFunction)Scope_setGain, METH_O, "Sets gain compensation."},
{"setWidth", (PyCFunction)Scope_setWidth, METH_O, "Sets the width of the display."},
{"setHeight", (PyCFunction)Scope_setHeight, METH_O, "Sets the height of the display."},
{"setFunc", (PyCFunction)Scope_setFunc, METH_O, "Sets callback function."},
{"setPoll", (PyCFunction)Scope_setPoll, METH_O, "Activate/deactivate polling."},
{NULL}  /* Sentinel */
};

PyTypeObject ScopeType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Scope_base",                                   /*tp_name*/
sizeof(Scope),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Scope_dealloc,                     /*tp_dealloc*/
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
"Scope objects. Show the waveform of an input signal.",           /* tp_doc */
(traverseproc)Scope_traverse,                  /* tp_traverse */
(inquiry)Scope_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Scope_methods,                                 /* tp_methods */
Scope_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Scope_new,                                     /* tp_new */
};

/************/
/* PeakAmp */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    MYFLT follow;
} PeakAmp;

static void
PeakAmp_filters_i(PeakAmp *self) {
    MYFLT absin, peak;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    peak = 0.0;
    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (absin > peak)
            peak = absin;
        self->data[i] = self->follow;
    }
    self->follow = peak;
}

static void PeakAmp_postprocessing_ii(PeakAmp *self) { POST_PROCESSING_II };
static void PeakAmp_postprocessing_ai(PeakAmp *self) { POST_PROCESSING_AI };
static void PeakAmp_postprocessing_ia(PeakAmp *self) { POST_PROCESSING_IA };
static void PeakAmp_postprocessing_aa(PeakAmp *self) { POST_PROCESSING_AA };
static void PeakAmp_postprocessing_ireva(PeakAmp *self) { POST_PROCESSING_IREVA };
static void PeakAmp_postprocessing_areva(PeakAmp *self) { POST_PROCESSING_AREVA };
static void PeakAmp_postprocessing_revai(PeakAmp *self) { POST_PROCESSING_REVAI };
static void PeakAmp_postprocessing_revaa(PeakAmp *self) { POST_PROCESSING_REVAA };
static void PeakAmp_postprocessing_revareva(PeakAmp *self) { POST_PROCESSING_REVAREVA };

static void
PeakAmp_setProcMode(PeakAmp *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PeakAmp_filters_i;
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = PeakAmp_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = PeakAmp_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = PeakAmp_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = PeakAmp_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = PeakAmp_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = PeakAmp_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = PeakAmp_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = PeakAmp_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = PeakAmp_postprocessing_revareva;
            break;
    }
}

static void
PeakAmp_compute_next_data_frame(PeakAmp *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
PeakAmp_traverse(PeakAmp *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
PeakAmp_clear(PeakAmp *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
PeakAmp_dealloc(PeakAmp* self)
{
    pyo_DEALLOC
    PeakAmp_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PeakAmp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    PeakAmp *self;
    self = (PeakAmp *)type->tp_alloc(type, 0);

    self->follow = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PeakAmp_compute_next_data_frame);
    self->mode_func_ptr = PeakAmp_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

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

static PyObject * PeakAmp_getServer(PeakAmp* self) { GET_SERVER };
static PyObject * PeakAmp_getStream(PeakAmp* self) { GET_STREAM };
static PyObject * PeakAmp_setMul(PeakAmp *self, PyObject *arg) { SET_MUL };
static PyObject * PeakAmp_setAdd(PeakAmp *self, PyObject *arg) { SET_ADD };
static PyObject * PeakAmp_setSub(PeakAmp *self, PyObject *arg) { SET_SUB };
static PyObject * PeakAmp_setDiv(PeakAmp *self, PyObject *arg) { SET_DIV };

static PyObject * PeakAmp_play(PeakAmp *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PeakAmp_stop(PeakAmp *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * PeakAmp_multiply(PeakAmp *self, PyObject *arg) { MULTIPLY };
static PyObject * PeakAmp_inplace_multiply(PeakAmp *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PeakAmp_add(PeakAmp *self, PyObject *arg) { ADD };
static PyObject * PeakAmp_inplace_add(PeakAmp *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PeakAmp_sub(PeakAmp *self, PyObject *arg) { SUB };
static PyObject * PeakAmp_inplace_sub(PeakAmp *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PeakAmp_div(PeakAmp *self, PyObject *arg) { DIV };
static PyObject * PeakAmp_inplace_div(PeakAmp *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
PeakAmp_getValue(PeakAmp *self)
{
    return PyFloat_FromDouble(self->follow);
}

static PyMemberDef PeakAmp_members[] = {
{"server", T_OBJECT_EX, offsetof(PeakAmp, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(PeakAmp, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(PeakAmp, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(PeakAmp, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(PeakAmp, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef PeakAmp_methods[] = {
{"getServer", (PyCFunction)PeakAmp_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)PeakAmp_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)PeakAmp_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)PeakAmp_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"getValue", (PyCFunction)PeakAmp_getValue, METH_NOARGS, "Returns the current peaking value."},
{"setMul", (PyCFunction)PeakAmp_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)PeakAmp_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)PeakAmp_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)PeakAmp_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods PeakAmp_as_number = {
(binaryfunc)PeakAmp_add,                         /*nb_add*/
(binaryfunc)PeakAmp_sub,                         /*nb_subtract*/
(binaryfunc)PeakAmp_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)PeakAmp_inplace_add,                 /*inplace_add*/
(binaryfunc)PeakAmp_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)PeakAmp_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)PeakAmp_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)PeakAmp_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject PeakAmpType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.PeakAmp_base",                                   /*tp_name*/
sizeof(PeakAmp),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)PeakAmp_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&PeakAmp_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"PeakAmp objects. Envelope follower.",           /* tp_doc */
(traverseproc)PeakAmp_traverse,                  /* tp_traverse */
(inquiry)PeakAmp_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
PeakAmp_methods,                                 /* tp_methods */
PeakAmp_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
PeakAmp_new,                                     /* tp_new */
};

/************/
/* RMS */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    MYFLT follow;
} RMS;

static void
RMS_filters_i(RMS *self) {
    MYFLT sum = 0.0;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->follow;
        sum += in[i] * in[i];
    }
    self->follow = MYSQRT(sum / self->bufsize);
}

static void RMS_postprocessing_ii(RMS *self) { POST_PROCESSING_II };
static void RMS_postprocessing_ai(RMS *self) { POST_PROCESSING_AI };
static void RMS_postprocessing_ia(RMS *self) { POST_PROCESSING_IA };
static void RMS_postprocessing_aa(RMS *self) { POST_PROCESSING_AA };
static void RMS_postprocessing_ireva(RMS *self) { POST_PROCESSING_IREVA };
static void RMS_postprocessing_areva(RMS *self) { POST_PROCESSING_AREVA };
static void RMS_postprocessing_revai(RMS *self) { POST_PROCESSING_REVAI };
static void RMS_postprocessing_revaa(RMS *self) { POST_PROCESSING_REVAA };
static void RMS_postprocessing_revareva(RMS *self) { POST_PROCESSING_REVAREVA };

static void
RMS_setProcMode(RMS *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = RMS_filters_i;
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = RMS_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = RMS_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = RMS_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = RMS_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = RMS_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = RMS_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = RMS_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = RMS_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = RMS_postprocessing_revareva;
            break;
    }
}

static void
RMS_compute_next_data_frame(RMS *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
RMS_traverse(RMS *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
RMS_clear(RMS *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
RMS_dealloc(RMS* self)
{
    pyo_DEALLOC
    RMS_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
RMS_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    RMS *self;
    self = (RMS *)type->tp_alloc(type, 0);

    self->follow = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, RMS_compute_next_data_frame);
    self->mode_func_ptr = RMS_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

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

static PyObject * RMS_getServer(RMS* self) { GET_SERVER };
static PyObject * RMS_getStream(RMS* self) { GET_STREAM };
static PyObject * RMS_setMul(RMS *self, PyObject *arg) { SET_MUL };
static PyObject * RMS_setAdd(RMS *self, PyObject *arg) { SET_ADD };
static PyObject * RMS_setSub(RMS *self, PyObject *arg) { SET_SUB };
static PyObject * RMS_setDiv(RMS *self, PyObject *arg) { SET_DIV };

static PyObject * RMS_play(RMS *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * RMS_stop(RMS *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * RMS_multiply(RMS *self, PyObject *arg) { MULTIPLY };
static PyObject * RMS_inplace_multiply(RMS *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * RMS_add(RMS *self, PyObject *arg) { ADD };
static PyObject * RMS_inplace_add(RMS *self, PyObject *arg) { INPLACE_ADD };
static PyObject * RMS_sub(RMS *self, PyObject *arg) { SUB };
static PyObject * RMS_inplace_sub(RMS *self, PyObject *arg) { INPLACE_SUB };
static PyObject * RMS_div(RMS *self, PyObject *arg) { DIV };
static PyObject * RMS_inplace_div(RMS *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
RMS_getValue(RMS *self)
{
    return PyFloat_FromDouble(self->follow);
}

static PyMemberDef RMS_members[] = {
{"server", T_OBJECT_EX, offsetof(RMS, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(RMS, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(RMS, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(RMS, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(RMS, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef RMS_methods[] = {
{"getServer", (PyCFunction)RMS_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)RMS_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)RMS_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)RMS_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"getValue", (PyCFunction)RMS_getValue, METH_NOARGS, "Returns the current peaking value."},
{"setMul", (PyCFunction)RMS_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)RMS_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)RMS_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)RMS_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods RMS_as_number = {
(binaryfunc)RMS_add,                         /*nb_add*/
(binaryfunc)RMS_sub,                         /*nb_subtract*/
(binaryfunc)RMS_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)RMS_inplace_add,                 /*inplace_add*/
(binaryfunc)RMS_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)RMS_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)RMS_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)RMS_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject RMSType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.RMS_base",                                   /*tp_name*/
sizeof(RMS),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)RMS_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&RMS_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"RMS objects. Envelope follower.",           /* tp_doc */
(traverseproc)RMS_traverse,                  /* tp_traverse */
(inquiry)RMS_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
RMS_methods,                                 /* tp_methods */
RMS_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
RMS_new,                                     /* tp_new */
};