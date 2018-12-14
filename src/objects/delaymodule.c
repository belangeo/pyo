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
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT maxdelay;
    MYFLT oneOverSr;
    long size;
    long in_count;
    int modebuffer[4];
    MYFLT *buffer; // samples memory
} Delay;

static void
Delay_process_ii(Delay *self) {
    MYFLT val, xind, frac;
    int i;
    long ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (del < self->oneOverSr)
        del = self->oneOverSr;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    MYFLT sampdel = del * self->sr;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        self->data[i] = val;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ai(Delay *self) {
    MYFLT val, xind, frac, sampdel, del;
    int i;
    long ind;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < self->oneOverSr)
            del = self->oneOverSr;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        self->data[i] = val;

        self->buffer[self->in_count] = in[i]  + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ia(Delay *self) {
    MYFLT val, xind, frac, feed;
    int i;
    long ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);

    if (del < self->oneOverSr)
        del = self->oneOverSr;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    MYFLT sampdel = del * self->sr;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        self->data[i] = val;

        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_aa(Delay *self) {
    MYFLT val, xind, frac, sampdel, feed, del;
    int i;
    long ind;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < self->oneOverSr)
            del = self->oneOverSr;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        self->data[i] = val;

        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Delay_postprocessing_ii(Delay *self) { POST_PROCESSING_II };
static void Delay_postprocessing_ai(Delay *self) { POST_PROCESSING_AI };
static void Delay_postprocessing_ia(Delay *self) { POST_PROCESSING_IA };
static void Delay_postprocessing_aa(Delay *self) { POST_PROCESSING_AA };
static void Delay_postprocessing_ireva(Delay *self) { POST_PROCESSING_IREVA };
static void Delay_postprocessing_areva(Delay *self) { POST_PROCESSING_AREVA };
static void Delay_postprocessing_revai(Delay *self) { POST_PROCESSING_REVAI };
static void Delay_postprocessing_revaa(Delay *self) { POST_PROCESSING_REVAA };
static void Delay_postprocessing_revareva(Delay *self) { POST_PROCESSING_REVAREVA };

static void
Delay_setProcMode(Delay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Delay_process_ii;
            break;
        case 1:
            self->proc_func_ptr = Delay_process_ai;
            break;
        case 10:
            self->proc_func_ptr = Delay_process_ia;
            break;
        case 11:
            self->proc_func_ptr = Delay_process_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Delay_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Delay_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Delay_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Delay_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Delay_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Delay_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Delay_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Delay_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Delay_postprocessing_revareva;
            break;
    }
}

static void
Delay_compute_next_data_frame(Delay *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Delay_traverse(Delay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->delay);
    Py_VISIT(self->delay_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    return 0;
}

static int
Delay_clear(Delay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->delay);
    Py_CLEAR(self->delay_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    return 0;
}

static void
Delay_dealloc(Delay* self)
{
    pyo_DEALLOC
    free(self->buffer);
    Delay_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Delay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    Delay *self;
    self = (Delay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0.25);
    self->feedback = PyFloat_FromDouble(0);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->oneOverSr = 1.0 / self->sr;

    Stream_setFunctionPtr(self->stream, Delay_compute_next_data_frame);
    self->mode_func_ptr = Delay_setProcMode;

    static char *kwlist[] = {"input", "delay", "feedback", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxdelay, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(self->maxdelay * self->sr + 0.5);

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Delay_getServer(Delay* self) { GET_SERVER };
static PyObject * Delay_getStream(Delay* self) { GET_STREAM };
static PyObject * Delay_setMul(Delay *self, PyObject *arg) { SET_MUL };
static PyObject * Delay_setAdd(Delay *self, PyObject *arg) { SET_ADD };
static PyObject * Delay_setSub(Delay *self, PyObject *arg) { SET_SUB };
static PyObject * Delay_setDiv(Delay *self, PyObject *arg) { SET_DIV };

static PyObject * Delay_play(Delay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Delay_out(Delay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Delay_stop(Delay *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Delay_multiply(Delay *self, PyObject *arg) { MULTIPLY };
static PyObject * Delay_inplace_multiply(Delay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Delay_add(Delay *self, PyObject *arg) { ADD };
static PyObject * Delay_inplace_add(Delay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Delay_sub(Delay *self, PyObject *arg) { SUB };
static PyObject * Delay_inplace_sub(Delay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Delay_div(Delay *self, PyObject *arg) { DIV };
static PyObject * Delay_inplace_div(Delay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Delay_setDelay(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Delay_setFeedback(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Delay_reset(Delay *self)
{
    int i;
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Delay_members[] = {
    {"server", T_OBJECT_EX, offsetof(Delay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Delay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Delay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Delay, delay), 0, "Delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Delay, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Delay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Delay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Delay_methods[] = {
    {"getServer", (PyCFunction)Delay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Delay_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Delay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Delay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Delay_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setDelay", (PyCFunction)Delay_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Delay_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
    {"reset", (PyCFunction)Delay_reset, METH_NOARGS, "Resets the memory buffer to zeros."},
	{"setMul", (PyCFunction)Delay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Delay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Delay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Delay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Delay_as_number = {
    (binaryfunc)Delay_add,                      /*nb_add*/
    (binaryfunc)Delay_sub,                 /*nb_subtract*/
    (binaryfunc)Delay_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Delay_inplace_add,              /*inplace_add*/
    (binaryfunc)Delay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Delay_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Delay_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Delay_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject DelayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Delay_base",         /*tp_name*/
    sizeof(Delay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Delay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Delay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Delay objects. Delay signal by x samples.",           /* tp_doc */
    (traverseproc)Delay_traverse,   /* tp_traverse */
    (inquiry)Delay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Delay_methods,             /* tp_methods */
    Delay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Delay_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    MYFLT maxdelay;
    long size;
    long in_count;
    int modebuffer[3];
    MYFLT *buffer; // samples memory
} SDelay;

static void
SDelay_process_i(SDelay *self) {
    int i;
    long ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);

    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    long sampdel = (long)(del * self->sr);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (sampdel == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->buffer[self->in_count] = in[i];
            self->in_count++;
            if (self->in_count >= self->size)
                self->in_count = 0;
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            ind = self->in_count - sampdel;
            if (ind < 0)
                ind += self->size;
            self->data[i] = self->buffer[ind];

            self->buffer[self->in_count] = in[i];
            self->in_count++;
            if (self->in_count >= self->size)
                self->in_count = 0;
        }
    }
}

static void
SDelay_process_a(SDelay *self) {
    MYFLT del;
    int i;
    long ind, sampdel;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = (long)(del * self->sr);
        if (sampdel == 0) {
            self->data[i] = self->buffer[self->in_count] = in[i];
        }
        else {
            ind = self->in_count - sampdel;
            if (ind < 0)
                ind += self->size;
            self->data[i] = self->buffer[ind];
        }
        self->buffer[self->in_count++] = in[i];
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void SDelay_postprocessing_ii(SDelay *self) { POST_PROCESSING_II };
static void SDelay_postprocessing_ai(SDelay *self) { POST_PROCESSING_AI };
static void SDelay_postprocessing_ia(SDelay *self) { POST_PROCESSING_IA };
static void SDelay_postprocessing_aa(SDelay *self) { POST_PROCESSING_AA };
static void SDelay_postprocessing_ireva(SDelay *self) { POST_PROCESSING_IREVA };
static void SDelay_postprocessing_areva(SDelay *self) { POST_PROCESSING_AREVA };
static void SDelay_postprocessing_revai(SDelay *self) { POST_PROCESSING_REVAI };
static void SDelay_postprocessing_revaa(SDelay *self) { POST_PROCESSING_REVAA };
static void SDelay_postprocessing_revareva(SDelay *self) { POST_PROCESSING_REVAREVA };

static void
SDelay_setProcMode(SDelay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = SDelay_process_i;
            break;
        case 1:
            self->proc_func_ptr = SDelay_process_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SDelay_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SDelay_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SDelay_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SDelay_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SDelay_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SDelay_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SDelay_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SDelay_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SDelay_postprocessing_revareva;
            break;
    }
}

static void
SDelay_compute_next_data_frame(SDelay *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
SDelay_traverse(SDelay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->delay);
    Py_VISIT(self->delay_stream);
    return 0;
}

static int
SDelay_clear(SDelay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->delay);
    Py_CLEAR(self->delay_stream);
    return 0;
}

static void
SDelay_dealloc(SDelay* self)
{
    pyo_DEALLOC
    free(self->buffer);
    SDelay_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SDelay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *multmp=NULL, *addtmp=NULL;
    SDelay *self;
    self = (SDelay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0.25);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SDelay_compute_next_data_frame);
    self->mode_func_ptr = SDelay_setProcMode;

    static char *kwlist[] = {"input", "delay", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OFOO, kwlist, &inputtmp, &delaytmp, &self->maxdelay, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(self->maxdelay * self->sr + 0.5);

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SDelay_getServer(SDelay* self) { GET_SERVER };
static PyObject * SDelay_getStream(SDelay* self) { GET_STREAM };
static PyObject * SDelay_setMul(SDelay *self, PyObject *arg) { SET_MUL };
static PyObject * SDelay_setAdd(SDelay *self, PyObject *arg) { SET_ADD };
static PyObject * SDelay_setSub(SDelay *self, PyObject *arg) { SET_SUB };
static PyObject * SDelay_setDiv(SDelay *self, PyObject *arg) { SET_DIV };

static PyObject * SDelay_play(SDelay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SDelay_out(SDelay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SDelay_stop(SDelay *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SDelay_multiply(SDelay *self, PyObject *arg) { MULTIPLY };
static PyObject * SDelay_inplace_multiply(SDelay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SDelay_add(SDelay *self, PyObject *arg) { ADD };
static PyObject * SDelay_inplace_add(SDelay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SDelay_sub(SDelay *self, PyObject *arg) { SUB };
static PyObject * SDelay_inplace_sub(SDelay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SDelay_div(SDelay *self, PyObject *arg) { DIV };
static PyObject * SDelay_inplace_div(SDelay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SDelay_setDelay(SDelay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SDelay_reset(SDelay *self)
{
    int i;
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef SDelay_members[] = {
    {"server", T_OBJECT_EX, offsetof(SDelay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SDelay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(SDelay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(SDelay, delay), 0, "Delay time in seconds."},
    {"mul", T_OBJECT_EX, offsetof(SDelay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(SDelay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SDelay_methods[] = {
    {"getServer", (PyCFunction)SDelay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SDelay_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)SDelay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SDelay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SDelay_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setDelay", (PyCFunction)SDelay_setDelay, METH_O, "Sets delay time in seconds."},
	{"reset", (PyCFunction)SDelay_reset, METH_NOARGS, "Resets the memory buffer to zeros."},
	{"setMul", (PyCFunction)SDelay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)SDelay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)SDelay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)SDelay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SDelay_as_number = {
    (binaryfunc)SDelay_add,                      /*nb_add*/
    (binaryfunc)SDelay_sub,                 /*nb_subtract*/
    (binaryfunc)SDelay_multiply,                 /*nb_multiply*/
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
    (binaryfunc)SDelay_inplace_add,              /*inplace_add*/
    (binaryfunc)SDelay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)SDelay_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)SDelay_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)SDelay_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject SDelayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SDelay_base",         /*tp_name*/
    sizeof(SDelay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SDelay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &SDelay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "SDelay objects. Simple Delay with no interpolation and no feedback.",           /* tp_doc */
    (traverseproc)SDelay_traverse,   /* tp_traverse */
    (inquiry)SDelay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SDelay_methods,             /* tp_methods */
    SDelay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SDelay_new,                 /* tp_new */
};

/*********************/
/***** Waveguide *****/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *dur;
    Stream *dur_stream;
    MYFLT minfreq;
    MYFLT lastFreq;
    MYFLT lastSampDel;
    MYFLT lastDur;
    MYFLT lastFeed;
    long size;
    int in_count;
    MYFLT nyquist;
    int modebuffer[4];
    MYFLT lpsamp; // lowpass sample memory
    MYFLT coeffs[5]; // lagrange coefficients
    MYFLT lagrange[4]; // lagrange samples memories
    MYFLT xn1; // dc block input delay
    MYFLT yn1; // dc block output delay
    MYFLT *buffer; // samples memory
} Waveguide;

static void
Waveguide_process_ii(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, tmp;
    int i, ind, isamp;

    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;

    if (dur <= 0)
        dur = 0.1;


    sampdel = self->lastSampDel;
    feed = self->lastFeed;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = self->sr / fr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;

        self->lastDur = dur;
        feed = MYPOW(100, -1.0/(fr*dur));
        self->lastFeed = feed;
    }
    else if (dur != self->lastDur) {
        self->lastDur = dur;
        feed = MYPOW(100, -1.0/(fr*dur));
        self->lastFeed = feed;
    }

    /* pick a new value in th delay line */
    isamp = (int)sampdel;
    for (i=0; i<self->bufsize; i++) {
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];

        /* simple lowpass filtering */
        tmp = val;
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = tmp;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Waveguide_process_ai(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, freq, tmp;
    int i, ind, isamp;

    MYFLT *fr =Stream_getData((Stream *)self->freq_stream);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    /* Check dur boundary */
    if (dur <= 0)
        dur = 0.1;

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        /* Check frequency boundary */
        if (freq < self->minfreq)
            freq = self->minfreq;
        else if (freq >= self->nyquist)
            freq = self->nyquist;

        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = self->sr / freq - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;

            self->lastDur = dur;
            feed = MYPOW(100, -1.0/(freq*dur));
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -1.0/(freq*dur));
            self->lastFeed = feed;
        }

        /* pick a new value in th delay line */
        isamp = (int)sampdel;

        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];

        /* simple lowpass filtering */
        tmp = val;
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = tmp;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_ia(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, dur, tmp;
    int i, ind, isamp;

    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *du = Stream_getData((Stream *)self->dur_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;

    sampdel = self->lastSampDel;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = self->sr / fr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
    }

    /* pick a new value in th delay line */
    isamp = (int)sampdel;
    for (i=0; i<self->bufsize; i++) {
        feed = self->lastFeed;
        dur = du[i];
        if (dur <= 0)
            dur = 0.1;
        if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -1.0/(fr*dur));
            self->lastFeed = feed;
        }
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];

        /* simple lowpass filtering */
        tmp = val;
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = tmp;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_aa(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, freq, dur, tmp;
    int i, ind, isamp;

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *du = Stream_getData((Stream *)self->dur_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        dur = du[i];
        /* Check boundaries */
        if (freq < self->minfreq)
            freq = self->minfreq;
        else if (freq >= self->nyquist)
            freq = self->nyquist;

        if (dur <= 0)
            dur = 0.1;

        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = self->sr / freq - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;

            self->lastDur = dur;
            feed = MYPOW(100, -1.0/(freq*dur));
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -1.0/(freq*dur));
            self->lastFeed = feed;
        }

        /* pick a new value in th delay line */
        isamp = (int)sampdel;

        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];

        /* simple lowpass filtering */
        tmp = val;
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = tmp;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Waveguide_postprocessing_ii(Waveguide *self) { POST_PROCESSING_II };
static void Waveguide_postprocessing_ai(Waveguide *self) { POST_PROCESSING_AI };
static void Waveguide_postprocessing_ia(Waveguide *self) { POST_PROCESSING_IA };
static void Waveguide_postprocessing_aa(Waveguide *self) { POST_PROCESSING_AA };
static void Waveguide_postprocessing_ireva(Waveguide *self) { POST_PROCESSING_IREVA };
static void Waveguide_postprocessing_areva(Waveguide *self) { POST_PROCESSING_AREVA };
static void Waveguide_postprocessing_revai(Waveguide *self) { POST_PROCESSING_REVAI };
static void Waveguide_postprocessing_revaa(Waveguide *self) { POST_PROCESSING_REVAA };
static void Waveguide_postprocessing_revareva(Waveguide *self) { POST_PROCESSING_REVAREVA };

static void
Waveguide_setProcMode(Waveguide *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Waveguide_process_ii;
            break;
        case 1:
            self->proc_func_ptr = Waveguide_process_ai;
            break;
        case 10:
            self->proc_func_ptr = Waveguide_process_ia;
            break;
        case 11:
            self->proc_func_ptr = Waveguide_process_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Waveguide_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Waveguide_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Waveguide_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Waveguide_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Waveguide_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Waveguide_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Waveguide_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Waveguide_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Waveguide_postprocessing_revareva;
            break;
    }
}

static void
Waveguide_compute_next_data_frame(Waveguide *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Waveguide_traverse(Waveguide *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->dur);
    Py_VISIT(self->dur_stream);
    return 0;
}

static int
Waveguide_clear(Waveguide *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->dur);
    Py_CLEAR(self->dur_stream);
    return 0;
}

static void
Waveguide_dealloc(Waveguide* self)
{
    pyo_DEALLOC
    free(self->buffer);
    Waveguide_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Waveguide_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Waveguide *self;
    self = (Waveguide *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(100);
    self->dur = PyFloat_FromDouble(0.99);
    self->minfreq = 20;
    self->lastFreq = -1.0;
    self->lastSampDel = -1.0;
    self->lastDur = -1.0;
    self->lastFeed = 0.0;
    self->in_count = 0;
    self->lpsamp = 0.0;
    for(i=0; i<4; i++) {
        self->lagrange[i] = 0.0;
    }
    self->xn1 = 0.0;
    self->yn1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.45;

    Stream_setFunctionPtr(self->stream, Waveguide_compute_next_data_frame);
    self->mode_func_ptr = Waveguide_setProcMode;

    static char *kwlist[] = {"input", "freq", "dur", "minfreq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &freqtmp, &durtmp, &self->minfreq, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(1.0 / self->minfreq * self->sr + 0.5);

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Waveguide_getServer(Waveguide* self) { GET_SERVER };
static PyObject * Waveguide_getStream(Waveguide* self) { GET_STREAM };
static PyObject * Waveguide_setMul(Waveguide *self, PyObject *arg) { SET_MUL };
static PyObject * Waveguide_setAdd(Waveguide *self, PyObject *arg) { SET_ADD };
static PyObject * Waveguide_setSub(Waveguide *self, PyObject *arg) { SET_SUB };
static PyObject * Waveguide_setDiv(Waveguide *self, PyObject *arg) { SET_DIV };

static PyObject * Waveguide_play(Waveguide *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Waveguide_out(Waveguide *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Waveguide_stop(Waveguide *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Waveguide_multiply(Waveguide *self, PyObject *arg) { MULTIPLY };
static PyObject * Waveguide_inplace_multiply(Waveguide *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Waveguide_add(Waveguide *self, PyObject *arg) { ADD };
static PyObject * Waveguide_inplace_add(Waveguide *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Waveguide_sub(Waveguide *self, PyObject *arg) { SUB };
static PyObject * Waveguide_inplace_sub(Waveguide *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Waveguide_div(Waveguide *self, PyObject *arg) { DIV };
static PyObject * Waveguide_inplace_div(Waveguide *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Waveguide_reset(Waveguide *self)
{
    int i;

    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }
    for(i=0; i<4; i++) {
        self->lagrange[i] = 0.0;
    }
    self->lpsamp = 0.0;
    self->xn1 = 0.0;
    self->yn1 = 0.0;

	Py_RETURN_NONE;
}

static PyObject *
Waveguide_setFreq(Waveguide *self, PyObject *arg)
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

static PyObject *
Waveguide_setDur(Waveguide *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Waveguide_members[] = {
{"server", T_OBJECT_EX, offsetof(Waveguide, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Waveguide, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Waveguide, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Waveguide, freq), 0, "Waveguide time in seconds."},
{"dur", T_OBJECT_EX, offsetof(Waveguide, dur), 0, "Feedback value."},
{"mul", T_OBJECT_EX, offsetof(Waveguide, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Waveguide, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Waveguide_methods[] = {
{"getServer", (PyCFunction)Waveguide_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Waveguide_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Waveguide_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Waveguide_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Waveguide_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"reset", (PyCFunction)Waveguide_reset, METH_NOARGS, "Reset the delay line."},
{"setFreq", (PyCFunction)Waveguide_setFreq, METH_O, "Sets freq time in seconds."},
{"setDur", (PyCFunction)Waveguide_setDur, METH_O, "Sets dur value between 0 -> 1."},
{"setMul", (PyCFunction)Waveguide_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Waveguide_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Waveguide_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Waveguide_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Waveguide_as_number = {
(binaryfunc)Waveguide_add,                      /*nb_add*/
(binaryfunc)Waveguide_sub,                 /*nb_subtract*/
(binaryfunc)Waveguide_multiply,                 /*nb_multiply*/
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
(binaryfunc)Waveguide_inplace_add,              /*inplace_add*/
(binaryfunc)Waveguide_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Waveguide_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Waveguide_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Waveguide_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject WaveguideType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Waveguide_base",         /*tp_name*/
sizeof(Waveguide),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Waveguide_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Waveguide_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Waveguide objects. Waveguide signal by x samples.",           /* tp_doc */
(traverseproc)Waveguide_traverse,   /* tp_traverse */
(inquiry)Waveguide_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Waveguide_methods,             /* tp_methods */
Waveguide_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Waveguide_new,                 /* tp_new */
};

/*********************/
/***** AllpassWG *****/
/*********************/
static const MYFLT alp_chorus_factor[3] = {1.0, 0.9981, 0.9957};
static const MYFLT alp_feedback = 0.3;

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *feed;
    Stream *feed_stream;
    PyObject *detune;
    Stream *detune_stream;
    MYFLT minfreq;
    MYFLT nyquist;
    long size;
    int alpsize;
    int in_count;
    int alp_in_count[3];
    int modebuffer[5];
    MYFLT *alpbuffer[3]; // allpass samples memories
    MYFLT xn1; // dc block input delay
    MYFLT yn1; // dc block output delay
    MYFLT *buffer; // samples memory
} AllpassWG;

static void
AllpassWG_process_iii(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed);
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;

    sampdel = self->sr / (fr * freqshift);
    alpdetune = detune * self->alpsize;

    for (i=0; i<self->bufsize; i++) {
        /* pick a new value in the delay line */
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_aii(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed);
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);

    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;

    alpdetune = detune * self->alpsize;
    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        else if (fr >= self->nyquist)
            fr = self->nyquist;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_iai(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, feed, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream);
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;

    sampdel = self->sr / (fr * freqshift);
    alpdetune = detune * self->alpsize;

    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        /* pick a new value in the delay line */
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_aai(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, feed, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream);
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);

    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;

    alpdetune = detune * self->alpsize;
    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_iia(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed);
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;

    for (i=0; i<self->bufsize; i++) {
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_aia(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed);
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);

    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_iaa(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, feed, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream);
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    else if (fr >= self->nyquist)
        fr = self->nyquist;

    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
AllpassWG_process_aaa(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, feed, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream);
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        else if (fr >= self->nyquist)
            fr = self->nyquist;
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;

        /* pick a new value in the delay line */
        sampdel = self->sr / (fr * freqshift);
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;

        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void AllpassWG_postprocessing_ii(AllpassWG *self) { POST_PROCESSING_II };
static void AllpassWG_postprocessing_ai(AllpassWG *self) { POST_PROCESSING_AI };
static void AllpassWG_postprocessing_ia(AllpassWG *self) { POST_PROCESSING_IA };
static void AllpassWG_postprocessing_aa(AllpassWG *self) { POST_PROCESSING_AA };
static void AllpassWG_postprocessing_ireva(AllpassWG *self) { POST_PROCESSING_IREVA };
static void AllpassWG_postprocessing_areva(AllpassWG *self) { POST_PROCESSING_AREVA };
static void AllpassWG_postprocessing_revai(AllpassWG *self) { POST_PROCESSING_REVAI };
static void AllpassWG_postprocessing_revaa(AllpassWG *self) { POST_PROCESSING_REVAA };
static void AllpassWG_postprocessing_revareva(AllpassWG *self) { POST_PROCESSING_REVAREVA };

static void
AllpassWG_setProcMode(AllpassWG *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = AllpassWG_process_iii;
            break;
        case 1:
            self->proc_func_ptr = AllpassWG_process_aii;
            break;
        case 10:
            self->proc_func_ptr = AllpassWG_process_iai;
            break;
        case 11:
            self->proc_func_ptr = AllpassWG_process_aai;
            break;
        case 100:
            self->proc_func_ptr = AllpassWG_process_iia;
            break;
        case 101:
            self->proc_func_ptr = AllpassWG_process_aia;
            break;
        case 110:
            self->proc_func_ptr = AllpassWG_process_iaa;
            break;
        case 111:
            self->proc_func_ptr = AllpassWG_process_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = AllpassWG_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = AllpassWG_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = AllpassWG_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = AllpassWG_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = AllpassWG_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = AllpassWG_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = AllpassWG_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = AllpassWG_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = AllpassWG_postprocessing_revareva;
            break;
    }
}

static void
AllpassWG_compute_next_data_frame(AllpassWG *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
AllpassWG_traverse(AllpassWG *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->feed);
    Py_VISIT(self->feed_stream);
    Py_VISIT(self->detune);
    Py_VISIT(self->detune_stream);
    return 0;
}

static int
AllpassWG_clear(AllpassWG *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->feed);
    Py_CLEAR(self->feed_stream);
    Py_CLEAR(self->detune);
    Py_CLEAR(self->detune_stream);
    return 0;
}

static void
AllpassWG_dealloc(AllpassWG* self)
{
    int i;
    pyo_DEALLOC
    free(self->buffer);
    for(i=0; i<3; i++) {
        free(self->alpbuffer[i]);
    }
    AllpassWG_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
AllpassWG_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *feedtmp=NULL, *detunetmp=NULL, *multmp=NULL, *addtmp=NULL;
    AllpassWG *self;
    self = (AllpassWG *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(100);
    self->feed = PyFloat_FromDouble(0.);
    self->detune = PyFloat_FromDouble(0.5);
    self->minfreq = 20;
    self->in_count = self->alp_in_count[0] = self->alp_in_count[1] = self->alp_in_count[2] = 0;
    self->xn1 = 0.0;
    self->yn1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = (MYFLT)self->sr * 0.45;

    Stream_setFunctionPtr(self->stream, AllpassWG_compute_next_data_frame);
    self->mode_func_ptr = AllpassWG_setProcMode;

    static char *kwlist[] = {"input", "freq", "feed", "detune", "minfreq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOOFOO, kwlist, &inputtmp, &freqtmp, &feedtmp, &detunetmp, &self->minfreq, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (feedtmp) {
        PyObject_CallMethod((PyObject *)self, "setFeed", "O", feedtmp);
    }
    if (detunetmp) {
        PyObject_CallMethod((PyObject *)self, "setDetune", "O", detunetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(1.0 / self->minfreq * self->sr + 0.5);
    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    self->alpsize = (int)(self->sr * 0.0025);
    for (i=0; i<3; i++) {
        self->alpbuffer[i] = (MYFLT *)realloc(self->alpbuffer[i], (self->alpsize+1) * sizeof(MYFLT));
        for (j=0; j<(self->alpsize+1); j++) {
            self->alpbuffer[i][j] = 0.;
        }
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * AllpassWG_getServer(AllpassWG* self) { GET_SERVER };
static PyObject * AllpassWG_getStream(AllpassWG* self) { GET_STREAM };
static PyObject * AllpassWG_setMul(AllpassWG *self, PyObject *arg) { SET_MUL };
static PyObject * AllpassWG_setAdd(AllpassWG *self, PyObject *arg) { SET_ADD };
static PyObject * AllpassWG_setSub(AllpassWG *self, PyObject *arg) { SET_SUB };
static PyObject * AllpassWG_setDiv(AllpassWG *self, PyObject *arg) { SET_DIV };

static PyObject * AllpassWG_play(AllpassWG *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * AllpassWG_out(AllpassWG *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * AllpassWG_stop(AllpassWG *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * AllpassWG_multiply(AllpassWG *self, PyObject *arg) { MULTIPLY };
static PyObject * AllpassWG_inplace_multiply(AllpassWG *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * AllpassWG_add(AllpassWG *self, PyObject *arg) { ADD };
static PyObject * AllpassWG_inplace_add(AllpassWG *self, PyObject *arg) { INPLACE_ADD };
static PyObject * AllpassWG_sub(AllpassWG *self, PyObject *arg) { SUB };
static PyObject * AllpassWG_inplace_sub(AllpassWG *self, PyObject *arg) { INPLACE_SUB };
static PyObject * AllpassWG_div(AllpassWG *self, PyObject *arg) { DIV };
static PyObject * AllpassWG_inplace_div(AllpassWG *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
AllpassWG_reset(AllpassWG *self)
{
    int i, j;
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }
    for (i=0; i<3; i++) {
        for (j=0; j<(self->alpsize+1); j++) {
            self->alpbuffer[i][j] = 0.;
        }
    }
    self->in_count = self->alp_in_count[0] = self->alp_in_count[1] = self->alp_in_count[2] = 0;
    self->xn1 = self->yn1 = 0.0;

	Py_RETURN_NONE;
}

static PyObject *
AllpassWG_setFreq(AllpassWG *self, PyObject *arg)
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

static PyObject *
AllpassWG_setFeed(AllpassWG *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feed);
	if (isNumber == 1) {
		self->feed = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feed_stream);
        self->feed_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
AllpassWG_setDetune(AllpassWG *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->detune);
	if (isNumber == 1) {
		self->detune = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->detune = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->detune, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->detune_stream);
        self->detune_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef AllpassWG_members[] = {
    {"server", T_OBJECT_EX, offsetof(AllpassWG, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(AllpassWG, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(AllpassWG, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(AllpassWG, freq), 0, "AllpassWG time in seconds."},
    {"feed", T_OBJECT_EX, offsetof(AllpassWG, feed), 0, "Feedback value."},
    {"detune", T_OBJECT_EX, offsetof(AllpassWG, detune), 0, "Detune value between 0 and 1."},
    {"mul", T_OBJECT_EX, offsetof(AllpassWG, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(AllpassWG, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef AllpassWG_methods[] = {
    {"getServer", (PyCFunction)AllpassWG_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)AllpassWG_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)AllpassWG_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)AllpassWG_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)AllpassWG_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"reset", (PyCFunction)AllpassWG_reset, METH_NOARGS, "Reset the delay line."},
    {"setFreq", (PyCFunction)AllpassWG_setFreq, METH_O, "Sets freq time in seconds."},
    {"setFeed", (PyCFunction)AllpassWG_setFeed, METH_O, "Sets feed value between 0 -> 1."},
    {"setDetune", (PyCFunction)AllpassWG_setDetune, METH_O, "Sets detune value between 0 -> 1."},
    {"setMul", (PyCFunction)AllpassWG_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)AllpassWG_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)AllpassWG_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)AllpassWG_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods AllpassWG_as_number = {
    (binaryfunc)AllpassWG_add,                      /*nb_add*/
    (binaryfunc)AllpassWG_sub,                 /*nb_subtract*/
    (binaryfunc)AllpassWG_multiply,                 /*nb_multiply*/
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
    (binaryfunc)AllpassWG_inplace_add,              /*inplace_add*/
    (binaryfunc)AllpassWG_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)AllpassWG_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)AllpassWG_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)AllpassWG_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject AllpassWGType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.AllpassWG_base",         /*tp_name*/
    sizeof(AllpassWG),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)AllpassWG_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &AllpassWG_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "AllpassWG objects. Waveguide model with builtin allpass circuit to detune resonance frequencies.", /* tp_doc */
    (traverseproc)AllpassWG_traverse,   /* tp_traverse */
    (inquiry)AllpassWG_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    AllpassWG_methods,             /* tp_methods */
    AllpassWG_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    AllpassWG_new,                 /* tp_new */
};

/************/
/* Delay1 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    MYFLT x1;
} Delay1;

static void
Delay1_filters(Delay1 *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->x1;
        self->x1 = in[i];
    }
}

static void Delay1_postprocessing_ii(Delay1 *self) { POST_PROCESSING_II };
static void Delay1_postprocessing_ai(Delay1 *self) { POST_PROCESSING_AI };
static void Delay1_postprocessing_ia(Delay1 *self) { POST_PROCESSING_IA };
static void Delay1_postprocessing_aa(Delay1 *self) { POST_PROCESSING_AA };
static void Delay1_postprocessing_ireva(Delay1 *self) { POST_PROCESSING_IREVA };
static void Delay1_postprocessing_areva(Delay1 *self) { POST_PROCESSING_AREVA };
static void Delay1_postprocessing_revai(Delay1 *self) { POST_PROCESSING_REVAI };
static void Delay1_postprocessing_revaa(Delay1 *self) { POST_PROCESSING_REVAA };
static void Delay1_postprocessing_revareva(Delay1 *self) { POST_PROCESSING_REVAREVA };

static void
Delay1_setProcMode(Delay1 *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Delay1_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Delay1_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Delay1_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Delay1_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Delay1_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Delay1_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Delay1_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Delay1_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Delay1_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Delay1_postprocessing_revareva;
            break;
    }
}

static void
Delay1_compute_next_data_frame(Delay1 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Delay1_traverse(Delay1 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Delay1_clear(Delay1 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Delay1_dealloc(Delay1* self)
{
    pyo_DEALLOC
    Delay1_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Delay1_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Delay1 *self;
    self = (Delay1 *)type->tp_alloc(type, 0);

    self->x1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Delay1_compute_next_data_frame);
    self->mode_func_ptr = Delay1_setProcMode;

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

static PyObject * Delay1_getServer(Delay1* self) { GET_SERVER };
static PyObject * Delay1_getStream(Delay1* self) { GET_STREAM };
static PyObject * Delay1_setMul(Delay1 *self, PyObject *arg) { SET_MUL };
static PyObject * Delay1_setAdd(Delay1 *self, PyObject *arg) { SET_ADD };
static PyObject * Delay1_setSub(Delay1 *self, PyObject *arg) { SET_SUB };
static PyObject * Delay1_setDiv(Delay1 *self, PyObject *arg) { SET_DIV };

static PyObject * Delay1_play(Delay1 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Delay1_out(Delay1 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Delay1_stop(Delay1 *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Delay1_multiply(Delay1 *self, PyObject *arg) { MULTIPLY };
static PyObject * Delay1_inplace_multiply(Delay1 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Delay1_add(Delay1 *self, PyObject *arg) { ADD };
static PyObject * Delay1_inplace_add(Delay1 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Delay1_sub(Delay1 *self, PyObject *arg) { SUB };
static PyObject * Delay1_inplace_sub(Delay1 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Delay1_div(Delay1 *self, PyObject *arg) { DIV };
static PyObject * Delay1_inplace_div(Delay1 *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Delay1_members[] = {
{"server", T_OBJECT_EX, offsetof(Delay1, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Delay1, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Delay1, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Delay1, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Delay1, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Delay1_methods[] = {
{"getServer", (PyCFunction)Delay1_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Delay1_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Delay1_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Delay1_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Delay1_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)Delay1_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Delay1_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Delay1_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Delay1_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Delay1_as_number = {
(binaryfunc)Delay1_add,                         /*nb_add*/
(binaryfunc)Delay1_sub,                         /*nb_subtract*/
(binaryfunc)Delay1_multiply,                    /*nb_multiply*/
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
(binaryfunc)Delay1_inplace_add,                 /*inplace_add*/
(binaryfunc)Delay1_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Delay1_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Delay1_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Delay1_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject Delay1Type = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Delay1_base",                                   /*tp_name*/
sizeof(Delay1),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Delay1_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Delay1_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Delay1 objects. Delays a signal by one sample.",           /* tp_doc */
(traverseproc)Delay1_traverse,                  /* tp_traverse */
(inquiry)Delay1_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Delay1_methods,                                 /* tp_methods */
Delay1_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Delay1_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT crossfade;
    MYFLT maxdelay;
    MYFLT oneOverSr;
    MYFLT amp1;
    MYFLT amp2;
    MYFLT inc1;
    MYFLT inc2;
    int current;
    long timer;
    long size;
    long in_count;
    long sampdel;
    MYFLT sampdel1;
    MYFLT sampdel2;
    int modebuffer[4];
    MYFLT *buffer; // samples memory
} SmoothDelay;

static void
SmoothDelay_process_ii(SmoothDelay *self) {
    MYFLT val, xind, frac, sum;
    int i;
    long ind, xsamps = 0;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (del < self->oneOverSr) del = self->oneOverSr;
    else if (del > self->maxdelay) del = self->maxdelay;

    if (feed < 0) feed = 0.0;
    else if (feed > 1) feed = 1.0;

    for (i=0; i<self->bufsize; i++) {
        if (self->timer == 0) {
            self->current = (self->current + 1) % 2;
            self->sampdel = (long)(del * self->sr + 0.5);
            xsamps = (long)(self->crossfade * self->sr + 0.5);
            if (xsamps > self->sampdel) xsamps = self->sampdel;
            if (xsamps <= 0) xsamps = 1;
            if (self->current == 0) {
                self->sampdel1 = del * self->sr;
                self->inc1 = 1.0 / xsamps;
                self->inc2 = -self->inc1;
            }
            else {
                self->sampdel2 = del * self->sr;
                self->inc2 = 1.0 / xsamps;
                self->inc1 = -self->inc2;
            }
        }

        xind = self->in_count - self->sampdel1;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum = val * self->amp1;
        self->amp1 += self->inc1;
        if (self->amp1 < 0) self->amp1 = 0.0;
        else if (self->amp1 > 1) self->amp1 = 1.0;

        xind = self->in_count - self->sampdel2;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum += val * self->amp2;
        self->amp2 += self->inc2;
        if (self->amp2 < 0) self->amp2 = 0.0;
        else if (self->amp2 > 1) self->amp2 = 1.0;

        self->data[i] = sum;

        self->buffer[self->in_count] = in[i] + (sum * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;

        self->timer++;
        if (self->timer == self->sampdel)
            self->timer = 0;
    }
}

static void
SmoothDelay_process_ai(SmoothDelay *self) {
    MYFLT val, xind, frac, sum, del;
    int i;
    long ind, xsamps = 0;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *dl = Stream_getData((Stream *)self->delay_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0) feed = 0.0;
    else if (feed > 1) feed = 1.0;

    for (i=0; i<self->bufsize; i++) {
        if (self->timer == 0) {
            del = dl[i];
            if (del < self->oneOverSr) del = self->oneOverSr;
            else if (del > self->maxdelay) del = self->maxdelay;
            self->current = (self->current + 1) % 2;
            self->sampdel = (long)(del * self->sr + 0.5);
            xsamps = (long)(self->crossfade * self->sr + 0.5);
            if (xsamps > self->sampdel) xsamps = self->sampdel;
            if (xsamps <= 0) xsamps = 1;
            if (self->current == 0) {
                self->sampdel1 = del * self->sr;
                self->inc1 = 1.0 / xsamps;
                self->inc2 = -self->inc1;
            }
            else {
                self->sampdel2 = del * self->sr;
                self->inc2 = 1.0 / xsamps;
                self->inc1 = -self->inc2;
            }
        }

        xind = self->in_count - self->sampdel1;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum = val * self->amp1;
        self->amp1 += self->inc1;
        if (self->amp1 < 0) self->amp1 = 0.0;
        else if (self->amp1 > 1) self->amp1 = 1.0;

        xind = self->in_count - self->sampdel2;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum += val * self->amp2;
        self->amp2 += self->inc2;
        if (self->amp2 < 0) self->amp2 = 0.0;
        else if (self->amp2 > 1) self->amp2 = 1.0;

        self->data[i] = sum;

        self->buffer[self->in_count] = in[i] + (sum * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;

        self->timer++;
        if (self->timer == self->sampdel)
            self->timer = 0;
    }
}

static void
SmoothDelay_process_ia(SmoothDelay *self) {
    MYFLT val, xind, frac, sum, feed;
    int i;
    long ind, xsamps = 0;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);

    if (del < self->oneOverSr) del = self->oneOverSr;
    else if (del > self->maxdelay) del = self->maxdelay;

    for (i=0; i<self->bufsize; i++) {
        feed = fd[i];
        if (feed < 0) feed = 0.0;
        else if (feed > 1) feed = 1.0;
        if (self->timer == 0) {
            self->current = (self->current + 1) % 2;
            self->sampdel = (long)(del * self->sr + 0.5);
            xsamps = (long)(self->crossfade * self->sr + 0.5);
            if (xsamps > self->sampdel) xsamps = self->sampdel;
            if (xsamps <= 0) xsamps = 1;
            if (self->current == 0) {
                self->sampdel1 = del * self->sr;
                self->inc1 = 1.0 / xsamps;
                self->inc2 = -self->inc1;
            }
            else {
                self->sampdel2 = del * self->sr;
                self->inc2 = 1.0 / xsamps;
                self->inc1 = -self->inc2;
            }
        }

        xind = self->in_count - self->sampdel1;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum = val * self->amp1;
        self->amp1 += self->inc1;
        if (self->amp1 < 0) self->amp1 = 0.0;
        else if (self->amp1 > 1) self->amp1 = 1.0;

        xind = self->in_count - self->sampdel2;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum += val * self->amp2;
        self->amp2 += self->inc2;
        if (self->amp2 < 0) self->amp2 = 0.0;
        else if (self->amp2 > 1) self->amp2 = 1.0;

        self->data[i] = sum;

        self->buffer[self->in_count] = in[i] + (sum * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;

        self->timer++;
        if (self->timer == self->sampdel)
            self->timer = 0;
    }
}

static void
SmoothDelay_process_aa(SmoothDelay *self) {
    MYFLT val, xind, frac, sum, del, feed;
    int i;
    long ind, xsamps = 0;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *dl = Stream_getData((Stream *)self->delay_stream);
    MYFLT *fd = Stream_getData((Stream *)self->feedback_stream);

    for (i=0; i<self->bufsize; i++) {
        feed = fd[i];
        if (feed < 0) feed = 0.0;
        else if (feed > 1) feed = 1.0;
        if (self->timer == 0) {
            del = dl[i];
            if (del < self->oneOverSr) del = self->oneOverSr;
            else if (del > self->maxdelay) del = self->maxdelay;
            self->current = (self->current + 1) % 2;
            self->sampdel = (long)(del * self->sr + 0.5);
            xsamps = (long)(self->crossfade * self->sr + 0.5);
            if (xsamps > self->sampdel) xsamps = self->sampdel;
            if (xsamps <= 0) xsamps = 1;
            if (self->current == 0) {
                self->sampdel1 = del * self->sr;
                self->inc1 = 1.0 / xsamps;
                self->inc2 = -self->inc1;
            }
            else {
                self->sampdel2 = del * self->sr;
                self->inc2 = 1.0 / xsamps;
                self->inc1 = -self->inc2;
            }
        }

        xind = self->in_count - self->sampdel1;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum = val * self->amp1;
        self->amp1 += self->inc1;
        if (self->amp1 < 0) self->amp1 = 0.0;
        else if (self->amp1 > 1) self->amp1 = 1.0;

        xind = self->in_count - self->sampdel2;
        while (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        sum += val * self->amp2;
        self->amp2 += self->inc2;
        if (self->amp2 < 0) self->amp2 = 0.0;
        else if (self->amp2 > 1) self->amp2 = 1.0;

        self->data[i] = sum;

        self->buffer[self->in_count] = in[i] + (sum * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;

        self->timer++;
        if (self->timer == self->sampdel)
            self->timer = 0;
    }
}

static void SmoothDelay_postprocessing_ii(SmoothDelay *self) { POST_PROCESSING_II };
static void SmoothDelay_postprocessing_ai(SmoothDelay *self) { POST_PROCESSING_AI };
static void SmoothDelay_postprocessing_ia(SmoothDelay *self) { POST_PROCESSING_IA };
static void SmoothDelay_postprocessing_aa(SmoothDelay *self) { POST_PROCESSING_AA };
static void SmoothDelay_postprocessing_ireva(SmoothDelay *self) { POST_PROCESSING_IREVA };
static void SmoothDelay_postprocessing_areva(SmoothDelay *self) { POST_PROCESSING_AREVA };
static void SmoothDelay_postprocessing_revai(SmoothDelay *self) { POST_PROCESSING_REVAI };
static void SmoothDelay_postprocessing_revaa(SmoothDelay *self) { POST_PROCESSING_REVAA };
static void SmoothDelay_postprocessing_revareva(SmoothDelay *self) { POST_PROCESSING_REVAREVA };

static void
SmoothDelay_setProcMode(SmoothDelay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = SmoothDelay_process_ii;
            break;
        case 1:
            self->proc_func_ptr = SmoothDelay_process_ai;
            break;
        case 10:
            self->proc_func_ptr = SmoothDelay_process_ia;
            break;
        case 11:
            self->proc_func_ptr = SmoothDelay_process_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SmoothDelay_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SmoothDelay_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SmoothDelay_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SmoothDelay_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SmoothDelay_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SmoothDelay_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SmoothDelay_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SmoothDelay_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SmoothDelay_postprocessing_revareva;
            break;
    }
}

static void
SmoothDelay_compute_next_data_frame(SmoothDelay *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
SmoothDelay_traverse(SmoothDelay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->delay);
    Py_VISIT(self->delay_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    return 0;
}

static int
SmoothDelay_clear(SmoothDelay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->delay);
    Py_CLEAR(self->delay_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    return 0;
}

static void
SmoothDelay_dealloc(SmoothDelay* self)
{
    pyo_DEALLOC
    free(self->buffer);
    SmoothDelay_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SmoothDelay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    SmoothDelay *self;
    self = (SmoothDelay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0.25);
    self->feedback = PyFloat_FromDouble(0);
    self->crossfade = 0.05;
    self->maxdelay = 1;
    self->in_count = 0;
    self->current = 1;
    self->timer = 0;
    self->amp1 = 0.0;
    self->amp2 = 1.0;
    self->inc1 = self->inc2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    self->oneOverSr = self->sampdel1 = self->sampdel2 = 1.0 / self->sr;

    Stream_setFunctionPtr(self->stream, SmoothDelay_compute_next_data_frame);
    self->mode_func_ptr = SmoothDelay_setProcMode;

    static char *kwlist[] = {"input", "delay", "feedback", "crossfade", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFFOO, kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->crossfade, &self->maxdelay, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(self->maxdelay * self->sr + 0.5);

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SmoothDelay_getServer(SmoothDelay* self) { GET_SERVER };
static PyObject * SmoothDelay_getStream(SmoothDelay* self) { GET_STREAM };
static PyObject * SmoothDelay_setMul(SmoothDelay *self, PyObject *arg) { SET_MUL };
static PyObject * SmoothDelay_setAdd(SmoothDelay *self, PyObject *arg) { SET_ADD };
static PyObject * SmoothDelay_setSub(SmoothDelay *self, PyObject *arg) { SET_SUB };
static PyObject * SmoothDelay_setDiv(SmoothDelay *self, PyObject *arg) { SET_DIV };

static PyObject * SmoothDelay_play(SmoothDelay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SmoothDelay_out(SmoothDelay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SmoothDelay_stop(SmoothDelay *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SmoothDelay_multiply(SmoothDelay *self, PyObject *arg) { MULTIPLY };
static PyObject * SmoothDelay_inplace_multiply(SmoothDelay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SmoothDelay_add(SmoothDelay *self, PyObject *arg) { ADD };
static PyObject * SmoothDelay_inplace_add(SmoothDelay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SmoothDelay_sub(SmoothDelay *self, PyObject *arg) { SUB };
static PyObject * SmoothDelay_inplace_sub(SmoothDelay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SmoothDelay_div(SmoothDelay *self, PyObject *arg) { DIV };
static PyObject * SmoothDelay_inplace_div(SmoothDelay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SmoothDelay_setDelay(SmoothDelay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SmoothDelay_setFeedback(SmoothDelay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SmoothDelay_setCrossfade(SmoothDelay *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->crossfade = PyFloat_AsDouble(arg);
	}

	Py_RETURN_NONE;
}

static PyObject *
SmoothDelay_reset(SmoothDelay *self)
{
    int i;
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }
	Py_RETURN_NONE;
}

static PyMemberDef SmoothDelay_members[] = {
    {"server", T_OBJECT_EX, offsetof(SmoothDelay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SmoothDelay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(SmoothDelay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(SmoothDelay, delay), 0, "SmoothDelay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(SmoothDelay, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(SmoothDelay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(SmoothDelay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SmoothDelay_methods[] = {
    {"getServer", (PyCFunction)SmoothDelay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SmoothDelay_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)SmoothDelay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SmoothDelay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SmoothDelay_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setDelay", (PyCFunction)SmoothDelay_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)SmoothDelay_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
    {"setCrossfade", (PyCFunction)SmoothDelay_setCrossfade, METH_O, "Sets crossfade time."},
    {"reset", (PyCFunction)SmoothDelay_reset, METH_NOARGS, "Resets the memory buffer to zeros."},
	{"setMul", (PyCFunction)SmoothDelay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)SmoothDelay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)SmoothDelay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)SmoothDelay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SmoothDelay_as_number = {
    (binaryfunc)SmoothDelay_add,                      /*nb_add*/
    (binaryfunc)SmoothDelay_sub,                 /*nb_subtract*/
    (binaryfunc)SmoothDelay_multiply,                 /*nb_multiply*/
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
    (binaryfunc)SmoothDelay_inplace_add,              /*inplace_add*/
    (binaryfunc)SmoothDelay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)SmoothDelay_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)SmoothDelay_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)SmoothDelay_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject SmoothDelayType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SmoothDelay_base",         /*tp_name*/
    sizeof(SmoothDelay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SmoothDelay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &SmoothDelay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "SmoothDelay objects. Delay signal by x samples.",           /* tp_doc */
    (traverseproc)SmoothDelay_traverse,   /* tp_traverse */
    (inquiry)SmoothDelay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SmoothDelay_methods,             /* tp_methods */
    SmoothDelay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SmoothDelay_new,                 /* tp_new */
};