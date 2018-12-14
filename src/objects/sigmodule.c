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
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *value;
    Stream *value_stream;
    int modebuffer[3];
} Sig;

static void Sig_postprocessing_ii(Sig *self) { POST_PROCESSING_II };
static void Sig_postprocessing_ai(Sig *self) { POST_PROCESSING_AI };
static void Sig_postprocessing_ia(Sig *self) { POST_PROCESSING_IA };
static void Sig_postprocessing_aa(Sig *self) { POST_PROCESSING_AA };
static void Sig_postprocessing_ireva(Sig *self) { POST_PROCESSING_IREVA };
static void Sig_postprocessing_areva(Sig *self) { POST_PROCESSING_AREVA };
static void Sig_postprocessing_revai(Sig *self) { POST_PROCESSING_REVAI };
static void Sig_postprocessing_revaa(Sig *self) { POST_PROCESSING_REVAA };
static void Sig_postprocessing_revareva(Sig *self) { POST_PROCESSING_REVAREVA };

static void
Sig_setProcMode(Sig *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Sig_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Sig_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Sig_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Sig_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Sig_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Sig_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Sig_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Sig_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Sig_postprocessing_revareva;
            break;
    }
}

static void
Sig_compute_next_data_frame(Sig *self)
{
    int i;
    if (self->modebuffer[2] == 0) {
        MYFLT val = PyFloat_AS_DOUBLE(self->value);
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = val;
        }
    }
    else {
        MYFLT *vals = Stream_getData((Stream *)self->value_stream);
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = vals[i];
        }
    }
    (*self->muladd_func_ptr)(self);
}

static int
Sig_traverse(Sig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->value);
    Py_VISIT(self->value_stream);
    return 0;
}

static int
Sig_clear(Sig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->value);
    Py_CLEAR(self->value_stream);
    return 0;
}

static void
Sig_dealloc(Sig* self)
{
    pyo_DEALLOC
    Sig_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Sig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Sig *self;
    self = (Sig *)type->tp_alloc(type, 0);

    self->value = PyFloat_FromDouble(0.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Sig_compute_next_data_frame);
    self->mode_func_ptr = Sig_setProcMode;

    static char *kwlist[] = {"value", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &valuetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (valuetmp) {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    Sig_compute_next_data_frame((Sig *)self);

    return (PyObject *)self;
}

static PyObject *
Sig_setValue(Sig *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
    Py_DECREF(self->value);
	if (isNumber == 1) {
		self->value = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else {
		self->value = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->value, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->value_stream);
        self->value_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * Sig_getServer(Sig* self) { GET_SERVER };
static PyObject * Sig_getStream(Sig* self) { GET_STREAM };
static PyObject * Sig_setMul(Sig *self, PyObject *arg) { SET_MUL };
static PyObject * Sig_setAdd(Sig *self, PyObject *arg) { SET_ADD };
static PyObject * Sig_setSub(Sig *self, PyObject *arg) { SET_SUB };
static PyObject * Sig_setDiv(Sig *self, PyObject *arg) { SET_DIV };

static PyObject * Sig_play(Sig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Sig_out(Sig *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Sig_stop(Sig *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Sig_multiply(Sig *self, PyObject *arg) { MULTIPLY };
static PyObject * Sig_inplace_multiply(Sig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Sig_add(Sig *self, PyObject *arg) { ADD };
static PyObject * Sig_inplace_add(Sig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Sig_sub(Sig *self, PyObject *arg) { SUB };
static PyObject * Sig_inplace_sub(Sig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Sig_div(Sig *self, PyObject *arg) { DIV };
static PyObject * Sig_inplace_div(Sig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Sig_members[] = {
{"server", T_OBJECT_EX, offsetof(Sig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Sig, stream), 0, "Stream object."},
{"value", T_OBJECT_EX, offsetof(Sig, value), 0, "Target value."},
{"mul", T_OBJECT_EX, offsetof(Sig, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Sig, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Sig_methods[] = {
{"getServer", (PyCFunction)Sig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Sig_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Sig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Sig_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Sig_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setValue", (PyCFunction)Sig_setValue, METH_O, "Sets Sig value."},
{"setMul", (PyCFunction)Sig_setMul, METH_O, "Sets Sig mul factor."},
{"setAdd", (PyCFunction)Sig_setAdd, METH_O, "Sets Sig add factor."},
{"setSub", (PyCFunction)Sig_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Sig_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Sig_as_number = {
(binaryfunc)Sig_add,                      /*nb_add*/
(binaryfunc)Sig_sub,                 /*nb_subtract*/
(binaryfunc)Sig_multiply,                 /*nb_multiply*/
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
(binaryfunc)Sig_inplace_add,              /*inplace_add*/
(binaryfunc)Sig_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Sig_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Sig_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Sig_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SigType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Sig_base",         /*tp_name*/
sizeof(Sig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Sig_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Sig_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Sig objects. Converts number into a signal stream.",           /* tp_doc */
(traverseproc)Sig_traverse,   /* tp_traverse */
(inquiry)Sig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Sig_methods,             /* tp_methods */
Sig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Sig_new,                 /* tp_new */
};

/***************************/
/* SigTo - Sig + ramp time */
/***************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *value;
    Stream *value_stream;
    PyObject *time;
    Stream *time_stream;
    MYFLT lastValue;
    MYFLT currentValue;
    long timeStep;
    MYFLT stepVal;
    long timeCount;
    int modebuffer[4];
} SigTo;

static void
SigTo_generates_i(SigTo *self) {
    int i;
    MYFLT value, time;

    if (self->modebuffer[2] == 0) {
        value = PyFloat_AS_DOUBLE(self->value);
        if (value != self->lastValue) {
            if (self->modebuffer[3] == 0)
                time = PyFloat_AS_DOUBLE(self->time);
            else
                time = Stream_getData((Stream *)self->time_stream)[0];
            self->timeCount = 0;
            self->lastValue = value;
            self->timeStep = (long)(time * self->sr);
            if (self->timeStep > 0)
                self->stepVal = (value - self->currentValue) / self->timeStep;
        }
        if (self->timeStep <= 0) {
            for (i=0; i<self->bufsize; i++)
                self->data[i] = self->currentValue = self->lastValue = value;
        }
        else {
            for (i=0; i<self->bufsize; i++) {
                if (self->timeCount == (self->timeStep - 1)) {
                    self->currentValue = value;
                    self->timeCount++;
                }
                else if (self->timeCount < self->timeStep) {
                    self->currentValue += self->stepVal;
                    self->timeCount++;
                }
                self->data[i] = self->currentValue;
            }
        }
    }
    else { 
        MYFLT *vals = Stream_getData((Stream *)self->value_stream);
        for (i=0; i<self->bufsize; i++) {
            value = vals[i];
            if (value != self->lastValue) {
                if (self->modebuffer[3] == 0)
                    time = PyFloat_AS_DOUBLE(self->time);
                else
                    time = Stream_getData((Stream *)self->time_stream)[i];
                self->timeCount = 0;
                self->lastValue = value;
                self->timeStep = (long)(time * self->sr);
                if (self->timeStep > 0)
                    self->stepVal = (value - self->currentValue) / self->timeStep;
            }
            if (self->timeStep <= 0) {
                self->data[i] = self->currentValue = self->lastValue = value;
            } else {
                if (self->timeCount == (self->timeStep - 1)) {
                    self->currentValue = value;
                    self->timeCount++;
                }
                else if (self->timeCount < self->timeStep) {
                    self->currentValue += self->stepVal;
                    self->timeCount++;
                }
                self->data[i] = self->currentValue;
            }
        }
    }
}

static void SigTo_postprocessing_ii(SigTo *self) { POST_PROCESSING_II };
static void SigTo_postprocessing_ai(SigTo *self) { POST_PROCESSING_AI };
static void SigTo_postprocessing_ia(SigTo *self) { POST_PROCESSING_IA };
static void SigTo_postprocessing_aa(SigTo *self) { POST_PROCESSING_AA };
static void SigTo_postprocessing_ireva(SigTo *self) { POST_PROCESSING_IREVA };
static void SigTo_postprocessing_areva(SigTo *self) { POST_PROCESSING_AREVA };
static void SigTo_postprocessing_revai(SigTo *self) { POST_PROCESSING_REVAI };
static void SigTo_postprocessing_revaa(SigTo *self) { POST_PROCESSING_REVAA };
static void SigTo_postprocessing_revareva(SigTo *self) { POST_PROCESSING_REVAREVA };

static void
SigTo_setProcMode(SigTo *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = SigTo_generates_i;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SigTo_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SigTo_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SigTo_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SigTo_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SigTo_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SigTo_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SigTo_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SigTo_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SigTo_postprocessing_revareva;
            break;
    }
}

static void
SigTo_compute_next_data_frame(SigTo *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
SigTo_traverse(SigTo *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->value);
    Py_VISIT(self->value_stream);
    Py_VISIT(self->time);
    Py_VISIT(self->time_stream);
    return 0;
}

static int
SigTo_clear(SigTo *self)
{
    pyo_CLEAR
    Py_CLEAR(self->value);
    Py_CLEAR(self->value_stream);
    Py_CLEAR(self->time);
    Py_CLEAR(self->time_stream);
    return 0;
}

static void
SigTo_dealloc(SigTo* self)
{
    pyo_DEALLOC
    SigTo_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SigTo_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *valuetmp=NULL, *timetmp=NULL, *multmp=NULL, *addtmp=NULL;
    SigTo *self;
    self = (SigTo *)type->tp_alloc(type, 0);

    self->value = PyFloat_FromDouble(0.0);
    self->time = PyFloat_FromDouble(0.025);
    self->timeCount = 0;
    self->stepVal = 0.0;
    self->timeStep = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SigTo_compute_next_data_frame);
    self->mode_func_ptr = SigTo_setProcMode;

    static char *kwlist[] = {"value", "time", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OFOO, kwlist, &valuetmp, &timetmp, &inittmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (valuetmp) {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
    }

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->lastValue = self->currentValue = inittmp;

    (*self->mode_func_ptr)(self);

    for(i=0; i<self->bufsize; i++) {
        self->data[i] = self->currentValue;
    }

    return (PyObject *)self;
}

static PyObject *
SigTo_setValue(SigTo *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
    Py_DECREF(self->value);
	if (isNumber == 1) {
		self->value = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else {
		self->value = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->value, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->value_stream);
        self->value_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

	Py_RETURN_NONE;
}

static PyObject *
SigTo_setTime(SigTo *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
    Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
    }
    else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
        self->modebuffer[3] = 1;
    }

	Py_RETURN_NONE;
}

static PyObject * SigTo_getServer(SigTo* self) { GET_SERVER };
static PyObject * SigTo_getStream(SigTo* self) { GET_STREAM };
static PyObject * SigTo_setMul(SigTo *self, PyObject *arg) { SET_MUL };
static PyObject * SigTo_setAdd(SigTo *self, PyObject *arg) { SET_ADD };
static PyObject * SigTo_setSub(SigTo *self, PyObject *arg) { SET_SUB };
static PyObject * SigTo_setDiv(SigTo *self, PyObject *arg) { SET_DIV };

static PyObject * SigTo_play(SigTo *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SigTo_stop(SigTo *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SigTo_multiply(SigTo *self, PyObject *arg) { MULTIPLY };
static PyObject * SigTo_inplace_multiply(SigTo *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SigTo_add(SigTo *self, PyObject *arg) { ADD };
static PyObject * SigTo_inplace_add(SigTo *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SigTo_sub(SigTo *self, PyObject *arg) { SUB };
static PyObject * SigTo_inplace_sub(SigTo *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SigTo_div(SigTo *self, PyObject *arg) { DIV };
static PyObject * SigTo_inplace_div(SigTo *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SigTo_members[] = {
{"server", T_OBJECT_EX, offsetof(SigTo, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SigTo, stream), 0, "Stream object."},
{"value", T_OBJECT_EX, offsetof(SigTo, value), 0, "Target value."},
{"time", T_OBJECT_EX, offsetof(SigTo, time), 0, "Ramp time."},
{"mul", T_OBJECT_EX, offsetof(SigTo, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SigTo, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SigTo_methods[] = {
{"getServer", (PyCFunction)SigTo_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SigTo_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)SigTo_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)SigTo_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setValue", (PyCFunction)SigTo_setValue, METH_O, "Sets SigTo value."},
{"setTime", (PyCFunction)SigTo_setTime, METH_O, "Sets ramp time in seconds."},
{"setMul", (PyCFunction)SigTo_setMul, METH_O, "Sets SigTo mul factor."},
{"setAdd", (PyCFunction)SigTo_setAdd, METH_O, "Sets SigTo add factor."},
{"setSub", (PyCFunction)SigTo_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SigTo_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SigTo_as_number = {
(binaryfunc)SigTo_add,                      /*nb_add*/
(binaryfunc)SigTo_sub,                 /*nb_subtract*/
(binaryfunc)SigTo_multiply,                 /*nb_multiply*/
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
(binaryfunc)SigTo_inplace_add,              /*inplace_add*/
(binaryfunc)SigTo_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SigTo_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)SigTo_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)SigTo_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SigToType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SigTo_base",         /*tp_name*/
sizeof(SigTo),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SigTo_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&SigTo_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"SigTo objects. Converts number into a signal stream and apply a ramp from last value.",           /* tp_doc */
(traverseproc)SigTo_traverse,   /* tp_traverse */
(inquiry)SigTo_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SigTo_methods,             /* tp_methods */
SigTo_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
SigTo_new,                 /* tp_new */
};

/***************************/
/* VarPort - Sig + ramp time + portamento + callback */
/***************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *callable;
    PyObject *arg;
    MYFLT value;
    MYFLT time;
    MYFLT lastValue;
    MYFLT currentValue;
    long timeStep;
    long timeout;
    MYFLT stepVal;
    long timeCount;
    int modebuffer[2];
    int flag;
} VarPort;

static void
VarPort_generates_i(VarPort *self) {
    int i;
    PyObject *tuple, *result;

    if (self->value != self->lastValue) {
        self->flag = 1;
        self->timeCount = 0;
        self->stepVal = (self->value - self->currentValue) / (self->timeStep+1);
        self->lastValue = self->value;
    }

    if (self->flag == 1) {
        for (i=0; i<self->bufsize; i++) {
            if (self->timeCount >= self->timeStep)
                self->currentValue = self->value;
            else if (self->timeCount < self->timeStep) {
                self->currentValue += self->stepVal;
            }

            self->timeCount++;
            self->data[i] = self->currentValue;
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->currentValue;
        }
    }

    if (self->timeCount >= self->timeout && self->flag == 1) {
        self->flag = 0;
        if (self->callable != Py_None) {
            if (self->arg != Py_None) {
                tuple = PyTuple_New(1);
                PyTuple_SET_ITEM(tuple, 0, self->arg);
            }
            else {
                tuple = PyTuple_New(0);
            }

            result = PyObject_Call(self->callable, tuple, NULL);
            if (result == NULL) {
                PyErr_Print();
                return;
            }
        }
    }
}

static void VarPort_postprocessing_ii(VarPort *self) { POST_PROCESSING_II };
static void VarPort_postprocessing_ai(VarPort *self) { POST_PROCESSING_AI };
static void VarPort_postprocessing_ia(VarPort *self) { POST_PROCESSING_IA };
static void VarPort_postprocessing_aa(VarPort *self) { POST_PROCESSING_AA };
static void VarPort_postprocessing_ireva(VarPort *self) { POST_PROCESSING_IREVA };
static void VarPort_postprocessing_areva(VarPort *self) { POST_PROCESSING_AREVA };
static void VarPort_postprocessing_revai(VarPort *self) { POST_PROCESSING_REVAI };
static void VarPort_postprocessing_revaa(VarPort *self) { POST_PROCESSING_REVAA };
static void VarPort_postprocessing_revareva(VarPort *self) { POST_PROCESSING_REVAREVA };

static void
VarPort_setProcMode(VarPort *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = VarPort_generates_i;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = VarPort_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = VarPort_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = VarPort_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = VarPort_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = VarPort_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = VarPort_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = VarPort_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = VarPort_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = VarPort_postprocessing_revareva;
            break;
    }
}

static void
VarPort_compute_next_data_frame(VarPort *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
VarPort_traverse(VarPort *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    Py_VISIT(self->arg);
    return 0;
}

static int
VarPort_clear(VarPort *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    Py_CLEAR(self->arg);
    return 0;
}

static void
VarPort_dealloc(VarPort* self)
{
    pyo_DEALLOC
    VarPort_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
VarPort_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *valuetmp=NULL, *timetmp=NULL, *calltmp=NULL, *argtmp=NULL, *multmp=NULL, *addtmp=NULL;
    VarPort *self;
    self = (VarPort *)type->tp_alloc(type, 0);

    self->flag = 1;
    self->time = 0.025;
    self->timeStep = (long)(self->time * self->sr);
    self->timeout = (long)((self->time + 0.1) * self->sr);
    self->timeCount = 0;
    self->stepVal = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    self->callable = Py_None;
    self->arg = Py_None;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, VarPort_compute_next_data_frame);
    self->mode_func_ptr = VarPort_setProcMode;

    static char *kwlist[] = {"value", "time", "init", "callable", "arg", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OFOOOO, kwlist, &valuetmp, &timetmp, &inittmp, &calltmp, &argtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (valuetmp) {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
    }

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    if (calltmp) {
        Py_DECREF(self->callable);
        Py_INCREF(calltmp);
        self->callable = calltmp;
    }

    if (argtmp) {
        Py_DECREF(self->arg);
        Py_INCREF(argtmp);
        self->arg = argtmp;
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->lastValue = self->currentValue = inittmp;

    (*self->mode_func_ptr)(self);

    for(i=0; i<self->bufsize; i++) {
        self->data[i] = self->currentValue;
    }

    return (PyObject *)self;
}

static PyObject *
VarPort_setValue(VarPort *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1)
		self->value = PyFloat_AsDouble(tmp);
    else
        self->value = self->lastValue;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
VarPort_setTime(VarPort *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1) {
		self->time = PyFloat_AsDouble(tmp);
        self->timeStep = (long)(self->time * self->sr);
        self->timeout = (long)((self->time + 0.1) * self->sr);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
VarPort_setFunction(VarPort *self, PyObject *arg)
{
	PyObject *tmp;

	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The function attribute must be callable.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    Py_XDECREF(self->callable);
    Py_INCREF(tmp);
    self->callable = tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * VarPort_getServer(VarPort* self) { GET_SERVER };
static PyObject * VarPort_getStream(VarPort* self) { GET_STREAM };
static PyObject * VarPort_setMul(VarPort *self, PyObject *arg) { SET_MUL };
static PyObject * VarPort_setAdd(VarPort *self, PyObject *arg) { SET_ADD };
static PyObject * VarPort_setSub(VarPort *self, PyObject *arg) { SET_SUB };
static PyObject * VarPort_setDiv(VarPort *self, PyObject *arg) { SET_DIV };

static PyObject * VarPort_play(VarPort *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * VarPort_stop(VarPort *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * VarPort_multiply(VarPort *self, PyObject *arg) { MULTIPLY };
static PyObject * VarPort_inplace_multiply(VarPort *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * VarPort_add(VarPort *self, PyObject *arg) { ADD };
static PyObject * VarPort_inplace_add(VarPort *self, PyObject *arg) { INPLACE_ADD };
static PyObject * VarPort_sub(VarPort *self, PyObject *arg) { SUB };
static PyObject * VarPort_inplace_sub(VarPort *self, PyObject *arg) { INPLACE_SUB };
static PyObject * VarPort_div(VarPort *self, PyObject *arg) { DIV };
static PyObject * VarPort_inplace_div(VarPort *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef VarPort_members[] = {
    {"server", T_OBJECT_EX, offsetof(VarPort, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(VarPort, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(VarPort, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(VarPort, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef VarPort_methods[] = {
    {"getServer", (PyCFunction)VarPort_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)VarPort_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)VarPort_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)VarPort_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setValue", (PyCFunction)VarPort_setValue, METH_O, "Sets VarPort value."},
    {"setTime", (PyCFunction)VarPort_setTime, METH_O, "Sets ramp time in seconds."},
    {"setFunction", (PyCFunction)VarPort_setFunction, METH_O, "Sets function to be called."},
    {"setMul", (PyCFunction)VarPort_setMul, METH_O, "Sets VarPort mul factor."},
    {"setAdd", (PyCFunction)VarPort_setAdd, METH_O, "Sets VarPort add factor."},
    {"setSub", (PyCFunction)VarPort_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)VarPort_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods VarPort_as_number = {
    (binaryfunc)VarPort_add,                      /*nb_add*/
    (binaryfunc)VarPort_sub,                 /*nb_subtract*/
    (binaryfunc)VarPort_multiply,                 /*nb_multiply*/
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
    (binaryfunc)VarPort_inplace_add,              /*inplace_add*/
    (binaryfunc)VarPort_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)VarPort_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)VarPort_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)VarPort_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject VarPortType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.VarPort_base",         /*tp_name*/
    sizeof(VarPort),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)VarPort_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &VarPort_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "VarPort objects. Converts number into a signal stream and apply a ramp from last value.",           /* tp_doc */
    (traverseproc)VarPort_traverse,   /* tp_traverse */
    (inquiry)VarPort_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VarPort_methods,             /* tp_methods */
    VarPort_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    VarPort_new,                 /* tp_new */
};
