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
#include <math.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *min;
    PyObject *max;
    PyObject *freq;
    Stream *min_stream;
    Stream *max_stream;
    Stream *freq_stream;
    MYFLT value;
    MYFLT oldValue;
    MYFLT diff;
    MYFLT time;
    int modebuffer[5]; // need at least 2 slots for mul & add
} Randi;

static void
Randi_generate_iii(Randi *self) {
    int i;
    MYFLT inc;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT range = ma - mi;
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aii(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iai(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aai(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iia(Randi *self) {
    int i;
    MYFLT inc;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT range = ma - mi;

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aia(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iaa(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aaa(Randi *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void Randi_postprocessing_ii(Randi *self) { POST_PROCESSING_II };
static void Randi_postprocessing_ai(Randi *self) { POST_PROCESSING_AI };
static void Randi_postprocessing_ia(Randi *self) { POST_PROCESSING_IA };
static void Randi_postprocessing_aa(Randi *self) { POST_PROCESSING_AA };
static void Randi_postprocessing_ireva(Randi *self) { POST_PROCESSING_IREVA };
static void Randi_postprocessing_areva(Randi *self) { POST_PROCESSING_AREVA };
static void Randi_postprocessing_revai(Randi *self) { POST_PROCESSING_REVAI };
static void Randi_postprocessing_revaa(Randi *self) { POST_PROCESSING_REVAA };
static void Randi_postprocessing_revareva(Randi *self) { POST_PROCESSING_REVAREVA };

static void
Randi_setProcMode(Randi *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Randi_generate_iii;
            break;
        case 1:
            self->proc_func_ptr = Randi_generate_aii;
            break;
        case 10:
            self->proc_func_ptr = Randi_generate_iai;
            break;
        case 11:
            self->proc_func_ptr = Randi_generate_aai;
            break;
        case 100:
            self->proc_func_ptr = Randi_generate_iia;
            break;
        case 101:
            self->proc_func_ptr = Randi_generate_aia;
            break;
        case 110:
            self->proc_func_ptr = Randi_generate_iaa;
            break;
        case 111:
            self->proc_func_ptr = Randi_generate_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Randi_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Randi_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Randi_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Randi_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Randi_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Randi_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Randi_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Randi_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Randi_postprocessing_revareva;
            break;
    }
}

static void
Randi_compute_next_data_frame(Randi *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Randi_traverse(Randi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->min);
    Py_VISIT(self->min_stream);
    Py_VISIT(self->max);
    Py_VISIT(self->max_stream);
    return 0;
}

static int
Randi_clear(Randi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->min);
    Py_CLEAR(self->min_stream);
    Py_CLEAR(self->max);
    Py_CLEAR(self->max_stream);
    return 0;
}

static void
Randi_dealloc(Randi* self)
{
    pyo_DEALLOC
    Randi_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Randi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT mi, ma;
    PyObject *mintmp=NULL, *maxtmp=NULL, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Randi *self;
    self = (Randi *)type->tp_alloc(type, 0);

    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->freq = PyFloat_FromDouble(1.);
    self->value = self->oldValue = self->diff = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Randi_compute_next_data_frame);
    self->mode_func_ptr = Randi_setProcMode;

    static char *kwlist[] = {"min", "max", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist, &mintmp, &maxtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }

    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }

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

    Server_generateSeed((Server *)self->server, RANDI_ID);

    if (self->modebuffer[2] == 0)
        mi = PyFloat_AS_DOUBLE(self->min);
    else
        mi = Stream_getData((Stream *)self->min_stream)[0];
    if (self->modebuffer[3] == 0)
        ma = PyFloat_AS_DOUBLE(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];

    self->value = self->oldValue = (mi + ma) * 0.5;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Randi_getServer(Randi* self) { GET_SERVER };
static PyObject * Randi_getStream(Randi* self) { GET_STREAM };
static PyObject * Randi_setMul(Randi *self, PyObject *arg) { SET_MUL };
static PyObject * Randi_setAdd(Randi *self, PyObject *arg) { SET_ADD };
static PyObject * Randi_setSub(Randi *self, PyObject *arg) { SET_SUB };
static PyObject * Randi_setDiv(Randi *self, PyObject *arg) { SET_DIV };

static PyObject * Randi_play(Randi *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Randi_out(Randi *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Randi_stop(Randi *self) { STOP };

static PyObject * Randi_multiply(Randi *self, PyObject *arg) { MULTIPLY };
static PyObject * Randi_inplace_multiply(Randi *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Randi_add(Randi *self, PyObject *arg) { ADD };
static PyObject * Randi_inplace_add(Randi *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Randi_sub(Randi *self, PyObject *arg) { SUB };
static PyObject * Randi_inplace_sub(Randi *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Randi_div(Randi *self, PyObject *arg) { DIV };
static PyObject * Randi_inplace_div(Randi *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Randi_setMin(Randi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Randi_setMax(Randi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Randi_setFreq(Randi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Randi_members[] = {
{"server", T_OBJECT_EX, offsetof(Randi, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Randi, stream), 0, "Stream object."},
{"min", T_OBJECT_EX, offsetof(Randi, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(Randi, max), 0, "Maximum possible value."},
{"freq", T_OBJECT_EX, offsetof(Randi, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(Randi, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Randi, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Randi_methods[] = {
{"getServer", (PyCFunction)Randi_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Randi_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Randi_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Randi_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Randi_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)Randi_setMin, METH_O, "Sets minimum possible value."},
{"setMax", (PyCFunction)Randi_setMax, METH_O, "Sets maximum possible value."},
{"setFreq", (PyCFunction)Randi_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)Randi_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Randi_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Randi_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Randi_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Randi_as_number = {
(binaryfunc)Randi_add,                         /*nb_add*/
(binaryfunc)Randi_sub,                         /*nb_subtract*/
(binaryfunc)Randi_multiply,                    /*nb_multiply*/
(binaryfunc)Randi_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Randi_inplace_add,                 /*inplace_add*/
(binaryfunc)Randi_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Randi_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Randi_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject RandiType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Randi_base",                                   /*tp_name*/
sizeof(Randi),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Randi_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Randi_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Randi objects. Periodically generates a new random value with interpolation.",           /* tp_doc */
(traverseproc)Randi_traverse,                  /* tp_traverse */
(inquiry)Randi_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Randi_methods,                                 /* tp_methods */
Randi_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Randi_new,                                     /* tp_new */
};

/****************/
/**** Randh *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *min;
    PyObject *max;
    PyObject *freq;
    Stream *min_stream;
    Stream *max_stream;
    Stream *freq_stream;
    MYFLT value;
    MYFLT time;
    int modebuffer[5]; // need at least 2 slots for mul & add
} Randh;

static void
Randh_generate_iii(Randh *self) {
    int i;
    MYFLT inc;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT range = ma - mi;
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_aii(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_iai(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_aai(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_iia(Randh *self) {
    int i;
    MYFLT inc;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT range = ma - mi;

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_aia(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_iaa(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
        }
        self->data[i] = self->value;
    }
}

static void
Randh_generate_aaa(Randh *self) {
    int i;
    MYFLT inc, range;
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi[i];
        }
        self->data[i] = self->value;
    }
}

static void Randh_postprocessing_ii(Randh *self) { POST_PROCESSING_II };
static void Randh_postprocessing_ai(Randh *self) { POST_PROCESSING_AI };
static void Randh_postprocessing_ia(Randh *self) { POST_PROCESSING_IA };
static void Randh_postprocessing_aa(Randh *self) { POST_PROCESSING_AA };
static void Randh_postprocessing_ireva(Randh *self) { POST_PROCESSING_IREVA };
static void Randh_postprocessing_areva(Randh *self) { POST_PROCESSING_AREVA };
static void Randh_postprocessing_revai(Randh *self) { POST_PROCESSING_REVAI };
static void Randh_postprocessing_revaa(Randh *self) { POST_PROCESSING_REVAA };
static void Randh_postprocessing_revareva(Randh *self) { POST_PROCESSING_REVAREVA };

static void
Randh_setProcMode(Randh *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Randh_generate_iii;
            break;
        case 1:
            self->proc_func_ptr = Randh_generate_aii;
            break;
        case 10:
            self->proc_func_ptr = Randh_generate_iai;
            break;
        case 11:
            self->proc_func_ptr = Randh_generate_aai;
            break;
        case 100:
            self->proc_func_ptr = Randh_generate_iia;
            break;
        case 101:
            self->proc_func_ptr = Randh_generate_aia;
            break;
        case 110:
            self->proc_func_ptr = Randh_generate_iaa;
            break;
        case 111:
            self->proc_func_ptr = Randh_generate_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Randh_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Randh_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Randh_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Randh_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Randh_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Randh_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Randh_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Randh_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Randh_postprocessing_revareva;
            break;
    }
}

static void
Randh_compute_next_data_frame(Randh *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Randh_traverse(Randh *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->min);
    Py_VISIT(self->min_stream);
    Py_VISIT(self->max);
    Py_VISIT(self->max_stream);
    return 0;
}

static int
Randh_clear(Randh *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->min);
    Py_CLEAR(self->min_stream);
    Py_CLEAR(self->max);
    Py_CLEAR(self->max_stream);
    return 0;
}

static void
Randh_dealloc(Randh* self)
{
    pyo_DEALLOC
    Randh_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Randh_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT mi, ma;
    PyObject *mintmp=NULL, *maxtmp=NULL, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Randh *self;
    self = (Randh *)type->tp_alloc(type, 0);

    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->freq = PyFloat_FromDouble(1.);
    self->value = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Randh_compute_next_data_frame);
    self->mode_func_ptr = Randh_setProcMode;

    static char *kwlist[] = {"min", "max", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist, &mintmp, &maxtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }

    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }

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

    Server_generateSeed((Server *)self->server, RANDH_ID);

    if (self->modebuffer[2] == 0)
        mi = PyFloat_AS_DOUBLE(self->min);
    else
        mi = Stream_getData((Stream *)self->min_stream)[0];
    if (self->modebuffer[3] == 0)
        ma = PyFloat_AS_DOUBLE(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];

    self->value = (mi + ma) * 0.5;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Randh_getServer(Randh* self) { GET_SERVER };
static PyObject * Randh_getStream(Randh* self) { GET_STREAM };
static PyObject * Randh_setMul(Randh *self, PyObject *arg) { SET_MUL };
static PyObject * Randh_setAdd(Randh *self, PyObject *arg) { SET_ADD };
static PyObject * Randh_setSub(Randh *self, PyObject *arg) { SET_SUB };
static PyObject * Randh_setDiv(Randh *self, PyObject *arg) { SET_DIV };

static PyObject * Randh_play(Randh *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Randh_out(Randh *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Randh_stop(Randh *self) { STOP };

static PyObject * Randh_multiply(Randh *self, PyObject *arg) { MULTIPLY };
static PyObject * Randh_inplace_multiply(Randh *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Randh_add(Randh *self, PyObject *arg) { ADD };
static PyObject * Randh_inplace_add(Randh *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Randh_sub(Randh *self, PyObject *arg) { SUB };
static PyObject * Randh_inplace_sub(Randh *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Randh_div(Randh *self, PyObject *arg) { DIV };
static PyObject * Randh_inplace_div(Randh *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Randh_setMin(Randh *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Randh_setMax(Randh *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Randh_setFreq(Randh *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Randh_members[] = {
{"server", T_OBJECT_EX, offsetof(Randh, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Randh, stream), 0, "Stream object."},
{"min", T_OBJECT_EX, offsetof(Randh, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(Randh, max), 0, "Maximum possible value."},
{"freq", T_OBJECT_EX, offsetof(Randh, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(Randh, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Randh, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Randh_methods[] = {
{"getServer", (PyCFunction)Randh_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Randh_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Randh_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Randh_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Randh_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)Randh_setMin, METH_O, "Sets minimum possible value."},
{"setMax", (PyCFunction)Randh_setMax, METH_O, "Sets maximum possible value."},
{"setFreq", (PyCFunction)Randh_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)Randh_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Randh_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Randh_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Randh_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Randh_as_number = {
(binaryfunc)Randh_add,                         /*nb_add*/
(binaryfunc)Randh_sub,                         /*nb_subtract*/
(binaryfunc)Randh_multiply,                    /*nb_multiply*/
(binaryfunc)Randh_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Randh_inplace_add,                 /*inplace_add*/
(binaryfunc)Randh_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Randh_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Randh_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject RandhType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Randh_base",                                   /*tp_name*/
sizeof(Randh),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Randh_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Randh_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Randh objects. Periodically generates a new random value.",           /* tp_doc */
(traverseproc)Randh_traverse,                  /* tp_traverse */
(inquiry)Randh_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Randh_methods,                                 /* tp_methods */
Randh_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Randh_new,                                     /* tp_new */
};

/****************/
/**** Choice *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    int chSize;
    MYFLT *choice;
    MYFLT value;
    MYFLT time;
    int modebuffer[3]; // need at least 2 slots for mul & add
} Choice;

static void
Choice_generate_i(Choice *self) {
    int i;
    MYFLT inc;
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = self->choice[(int)((rand()/((MYFLT)(RAND_MAX))) * self->chSize)];
        }
        self->data[i] = self->value;
    }
}

static void
Choice_generate_a(Choice *self) {
    int i;
    MYFLT inc;
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = self->choice[(int)((rand()/((MYFLT)(RAND_MAX))) * self->chSize)];
        }
        self->data[i] = self->value;
    }
}

static void Choice_postprocessing_ii(Choice *self) { POST_PROCESSING_II };
static void Choice_postprocessing_ai(Choice *self) { POST_PROCESSING_AI };
static void Choice_postprocessing_ia(Choice *self) { POST_PROCESSING_IA };
static void Choice_postprocessing_aa(Choice *self) { POST_PROCESSING_AA };
static void Choice_postprocessing_ireva(Choice *self) { POST_PROCESSING_IREVA };
static void Choice_postprocessing_areva(Choice *self) { POST_PROCESSING_AREVA };
static void Choice_postprocessing_revai(Choice *self) { POST_PROCESSING_REVAI };
static void Choice_postprocessing_revaa(Choice *self) { POST_PROCESSING_REVAA };
static void Choice_postprocessing_revareva(Choice *self) { POST_PROCESSING_REVAREVA };

static void
Choice_setProcMode(Choice *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Choice_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Choice_generate_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Choice_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Choice_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Choice_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Choice_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Choice_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Choice_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Choice_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Choice_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Choice_postprocessing_revareva;
            break;
    }
}

static void
Choice_compute_next_data_frame(Choice *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Choice_traverse(Choice *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
Choice_clear(Choice *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
Choice_dealloc(Choice* self)
{
    pyo_DEALLOC
    free(self->choice);
    Choice_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Choice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *choicetmp=NULL, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Choice *self;
    self = (Choice *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1.);
    self->value = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Choice_compute_next_data_frame);
    self->mode_func_ptr = Choice_setProcMode;

    static char *kwlist[] = {"choice", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &choicetmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (choicetmp) {
        PyObject_CallMethod((PyObject *)self, "setChoice", "O", choicetmp);
    }

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

    Server_generateSeed((Server *)self->server, CHOICE_ID);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Choice_getServer(Choice* self) { GET_SERVER };
static PyObject * Choice_getStream(Choice* self) { GET_STREAM };
static PyObject * Choice_setMul(Choice *self, PyObject *arg) { SET_MUL };
static PyObject * Choice_setAdd(Choice *self, PyObject *arg) { SET_ADD };
static PyObject * Choice_setSub(Choice *self, PyObject *arg) { SET_SUB };
static PyObject * Choice_setDiv(Choice *self, PyObject *arg) { SET_DIV };

static PyObject * Choice_play(Choice *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Choice_out(Choice *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Choice_stop(Choice *self) { STOP };

static PyObject * Choice_multiply(Choice *self, PyObject *arg) { MULTIPLY };
static PyObject * Choice_inplace_multiply(Choice *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Choice_add(Choice *self, PyObject *arg) { ADD };
static PyObject * Choice_inplace_add(Choice *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Choice_sub(Choice *self, PyObject *arg) { SUB };
static PyObject * Choice_inplace_sub(Choice *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Choice_div(Choice *self, PyObject *arg) { DIV };
static PyObject * Choice_inplace_div(Choice *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Choice_setChoice(Choice *self, PyObject *arg)
{
    int i;
	PyObject *tmp;

	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The choice attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    self->chSize = PyList_Size(tmp);
    self->choice = (MYFLT *)realloc(self->choice, self->chSize * sizeof(MYFLT));
    for (i=0; i<self->chSize; i++) {
        self->choice[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(tmp, i)));
    }

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Choice_setFreq(Choice *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

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

static PyMemberDef Choice_members[] = {
{"server", T_OBJECT_EX, offsetof(Choice, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Choice, stream), 0, "Stream object."},
{"freq", T_OBJECT_EX, offsetof(Choice, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(Choice, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Choice, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Choice_methods[] = {
{"getServer", (PyCFunction)Choice_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Choice_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Choice_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Choice_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Choice_stop, METH_NOARGS, "Stops computing."},
{"setChoice", (PyCFunction)Choice_setChoice, METH_O, "Sets list of possible floats."},
{"setFreq", (PyCFunction)Choice_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)Choice_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Choice_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Choice_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Choice_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Choice_as_number = {
(binaryfunc)Choice_add,                         /*nb_add*/
(binaryfunc)Choice_sub,                         /*nb_subtract*/
(binaryfunc)Choice_multiply,                    /*nb_multiply*/
(binaryfunc)Choice_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Choice_inplace_add,                 /*inplace_add*/
(binaryfunc)Choice_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Choice_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Choice_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ChoiceType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Choice_base",                                   /*tp_name*/
sizeof(Choice),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Choice_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Choice_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Choice objects. Periodically generates a new random value from a list.",           /* tp_doc */
(traverseproc)Choice_traverse,                  /* tp_traverse */
(inquiry)Choice_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Choice_methods,                                 /* tp_methods */
Choice_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Choice_new,                                     /* tp_new */
};

/****************/
/**** RandInt ***/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *max;
    PyObject *freq;
    Stream *max_stream;
    Stream *freq_stream;
    MYFLT value;
    MYFLT time;
    int modebuffer[4]; // need at least 2 slots for mul & add
} RandInt;

static void
RandInt_generate_ii(RandInt *self) {
    int i;
    MYFLT inc;
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)((int)(rand()/((MYFLT)(RAND_MAX)+1)*ma));
        }
        self->data[i] = self->value;
    }
}

static void
RandInt_generate_ai(RandInt *self) {
    int i;
    MYFLT inc;
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)((int)(rand()/((MYFLT)(RAND_MAX)+1)*ma[i]));
        }
        self->data[i] = self->value;
    }
}

static void
RandInt_generate_ia(RandInt *self) {
    int i;
    MYFLT inc;
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)((int)(rand()/((MYFLT)(RAND_MAX)+1)*ma));
        }
        self->data[i] = self->value;
    }
}

static void
RandInt_generate_aa(RandInt *self) {
    int i;
    MYFLT inc;
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)((int)(rand()/((MYFLT)(RAND_MAX)+1)*ma[i]));
        }
        self->data[i] = self->value;
    }
}

static void RandInt_postprocessing_ii(RandInt *self) { POST_PROCESSING_II };
static void RandInt_postprocessing_ai(RandInt *self) { POST_PROCESSING_AI };
static void RandInt_postprocessing_ia(RandInt *self) { POST_PROCESSING_IA };
static void RandInt_postprocessing_aa(RandInt *self) { POST_PROCESSING_AA };
static void RandInt_postprocessing_ireva(RandInt *self) { POST_PROCESSING_IREVA };
static void RandInt_postprocessing_areva(RandInt *self) { POST_PROCESSING_AREVA };
static void RandInt_postprocessing_revai(RandInt *self) { POST_PROCESSING_REVAI };
static void RandInt_postprocessing_revaa(RandInt *self) { POST_PROCESSING_REVAA };
static void RandInt_postprocessing_revareva(RandInt *self) { POST_PROCESSING_REVAREVA };

static void
RandInt_setProcMode(RandInt *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = RandInt_generate_ii;
            break;
        case 1:
            self->proc_func_ptr = RandInt_generate_ai;
            break;
        case 10:
            self->proc_func_ptr = RandInt_generate_ia;
            break;
        case 11:
            self->proc_func_ptr = RandInt_generate_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = RandInt_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = RandInt_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = RandInt_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = RandInt_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = RandInt_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = RandInt_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = RandInt_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = RandInt_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = RandInt_postprocessing_revareva;
            break;
    }
}

static void
RandInt_compute_next_data_frame(RandInt *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
RandInt_traverse(RandInt *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->max);
    Py_VISIT(self->max_stream);
    return 0;
}

static int
RandInt_clear(RandInt *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->max);
    Py_CLEAR(self->max_stream);
    return 0;
}

static void
RandInt_dealloc(RandInt* self)
{
    pyo_DEALLOC
    RandInt_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
RandInt_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maxtmp=NULL, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    RandInt *self;
    self = (RandInt *)type->tp_alloc(type, 0);

    self->max = PyFloat_FromDouble(100.);
    self->freq = PyFloat_FromDouble(1.);
    self->value = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, RandInt_compute_next_data_frame);
    self->mode_func_ptr = RandInt_setProcMode;

    static char *kwlist[] = {"max", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &maxtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }

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

    Server_generateSeed((Server *)self->server, RANDINT_ID);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * RandInt_getServer(RandInt* self) { GET_SERVER };
static PyObject * RandInt_getStream(RandInt* self) { GET_STREAM };
static PyObject * RandInt_setMul(RandInt *self, PyObject *arg) { SET_MUL };
static PyObject * RandInt_setAdd(RandInt *self, PyObject *arg) { SET_ADD };
static PyObject * RandInt_setSub(RandInt *self, PyObject *arg) { SET_SUB };
static PyObject * RandInt_setDiv(RandInt *self, PyObject *arg) { SET_DIV };

static PyObject * RandInt_play(RandInt *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * RandInt_out(RandInt *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * RandInt_stop(RandInt *self) { STOP };

static PyObject * RandInt_multiply(RandInt *self, PyObject *arg) { MULTIPLY };
static PyObject * RandInt_inplace_multiply(RandInt *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * RandInt_add(RandInt *self, PyObject *arg) { ADD };
static PyObject * RandInt_inplace_add(RandInt *self, PyObject *arg) { INPLACE_ADD };
static PyObject * RandInt_sub(RandInt *self, PyObject *arg) { SUB };
static PyObject * RandInt_inplace_sub(RandInt *self, PyObject *arg) { INPLACE_SUB };
static PyObject * RandInt_div(RandInt *self, PyObject *arg) { DIV };
static PyObject * RandInt_inplace_div(RandInt *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
RandInt_setMax(RandInt *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
RandInt_setFreq(RandInt *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef RandInt_members[] = {
{"server", T_OBJECT_EX, offsetof(RandInt, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(RandInt, stream), 0, "Stream object."},
{"max", T_OBJECT_EX, offsetof(RandInt, max), 0, "Maximum possible value."},
{"freq", T_OBJECT_EX, offsetof(RandInt, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(RandInt, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(RandInt, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef RandInt_methods[] = {
{"getServer", (PyCFunction)RandInt_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)RandInt_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)RandInt_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)RandInt_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)RandInt_stop, METH_NOARGS, "Stops computing."},
{"setMax", (PyCFunction)RandInt_setMax, METH_O, "Sets maximum possible value."},
{"setFreq", (PyCFunction)RandInt_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)RandInt_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)RandInt_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)RandInt_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)RandInt_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods RandInt_as_number = {
(binaryfunc)RandInt_add,                         /*nb_add*/
(binaryfunc)RandInt_sub,                         /*nb_subtract*/
(binaryfunc)RandInt_multiply,                    /*nb_multiply*/
(binaryfunc)RandInt_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)RandInt_inplace_add,                 /*inplace_add*/
(binaryfunc)RandInt_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)RandInt_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)RandInt_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject RandIntType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.RandInt_base",                                   /*tp_name*/
sizeof(RandInt),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)RandInt_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&RandInt_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"RandInt objects. Periodically generates a new integer random value.",           /* tp_doc */
(traverseproc)RandInt_traverse,                  /* tp_traverse */
(inquiry)RandInt_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
RandInt_methods,                                 /* tp_methods */
RandInt_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
RandInt_new,                                     /* tp_new */
};

/*****************/
/**** RandDur ****/
/*****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *min;
    PyObject *max;
    Stream *min_stream;
    Stream *max_stream;
    MYFLT value;
    MYFLT time;
    MYFLT inc;
    int modebuffer[4]; // need at least 2 slots for mul & add
} RandDur;

static void
RandDur_generate_ii(RandDur *self) {
    int i;
    MYFLT range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i=0; i<self->bufsize; i++) {
        self->time += self->inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            if (mi < 0.0)
                mi = 0.0;
            range = ma - mi;
            if (range < 0.0)
                range = 0.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->inc = (1.0 / self->value) / self->sr;
        }
        self->data[i] = self->value;
    }
}

static void
RandDur_generate_ai(RandDur *self) {
    int i;
    MYFLT range, mi;
    MYFLT *min = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i=0; i<self->bufsize; i++) {
        self->time += self->inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            mi = min[i];
            if (mi < 0.0)
                mi = 0.0;
            range = ma - mi;
            if (range < 0.0)
                range = 0.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->inc = (1.0 / self->value) / self->sr;
        }
        self->data[i] = self->value;
    }
}

static void
RandDur_generate_ia(RandDur *self) {
    int i;
    MYFLT range;
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i=0; i<self->bufsize; i++) {
        self->time += self->inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            if (mi < 0.0)
                mi = 0.0;
            range = ma[i] - mi;
            if (range < 0.0)
                range = 0.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->inc = (1.0 / self->value) / self->sr;
        }
        self->data[i] = self->value;
    }
}

static void
RandDur_generate_aa(RandDur *self) {
    int i;
    MYFLT range, mi;
    MYFLT *min = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i=0; i<self->bufsize; i++) {
        self->time += self->inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            mi = min[i];
            if (mi < 0.0)
                mi = 0.0;
            range = ma[i] - mi;
            if (range < 0.0)
                range = 0.0;
            self->value = range * (rand()/((MYFLT)(RAND_MAX)+1)) + mi;
            self->inc = (1.0 / self->value) / self->sr;
        }
        self->data[i] = self->value;
    }
}

static void RandDur_postprocessing_ii(RandDur *self) { POST_PROCESSING_II };
static void RandDur_postprocessing_ai(RandDur *self) { POST_PROCESSING_AI };
static void RandDur_postprocessing_ia(RandDur *self) { POST_PROCESSING_IA };
static void RandDur_postprocessing_aa(RandDur *self) { POST_PROCESSING_AA };
static void RandDur_postprocessing_ireva(RandDur *self) { POST_PROCESSING_IREVA };
static void RandDur_postprocessing_areva(RandDur *self) { POST_PROCESSING_AREVA };
static void RandDur_postprocessing_revai(RandDur *self) { POST_PROCESSING_REVAI };
static void RandDur_postprocessing_revaa(RandDur *self) { POST_PROCESSING_REVAA };
static void RandDur_postprocessing_revareva(RandDur *self) { POST_PROCESSING_REVAREVA };

static void
RandDur_setProcMode(RandDur *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = RandDur_generate_ii;
            break;
        case 1:
            self->proc_func_ptr = RandDur_generate_ai;
            break;
        case 10:
            self->proc_func_ptr = RandDur_generate_ia;
            break;
        case 11:
            self->proc_func_ptr = RandDur_generate_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = RandDur_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = RandDur_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = RandDur_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = RandDur_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = RandDur_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = RandDur_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = RandDur_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = RandDur_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = RandDur_postprocessing_revareva;
            break;
    }
}

static void
RandDur_compute_next_data_frame(RandDur *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
RandDur_traverse(RandDur *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->min);
    Py_VISIT(self->min_stream);
    Py_VISIT(self->max);
    Py_VISIT(self->max_stream);
    return 0;
}

static int
RandDur_clear(RandDur *self)
{
    pyo_CLEAR
    Py_CLEAR(self->min);
    Py_CLEAR(self->min_stream);
    Py_CLEAR(self->max);
    Py_CLEAR(self->max_stream);
    return 0;
}

static void
RandDur_dealloc(RandDur* self)
{
    pyo_DEALLOC
    RandDur_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
RandDur_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT mi, ma;
    PyObject *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    RandDur *self;
    self = (RandDur *)type->tp_alloc(type, 0);

    self->min = PyFloat_FromDouble(0.01);
    self->max = PyFloat_FromDouble(1.);
    self->value = self->inc = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, RandDur_compute_next_data_frame);
    self->mode_func_ptr = RandDur_setProcMode;

    static char *kwlist[] = {"min", "max", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &mintmp, &maxtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }

    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Server_generateSeed((Server *)self->server, RANDDUR_ID);

    if (self->modebuffer[2] == 0)
        mi = PyFloat_AS_DOUBLE(self->min);
    else
        mi = Stream_getData((Stream *)self->min_stream)[0];
    if (self->modebuffer[3] == 0)
        ma = PyFloat_AS_DOUBLE(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];

    self->value = (mi + ma) * 0.5;
    if (self->value == 0.0)
        self->inc = 0.0;
    else
        self->inc = (1.0 / self->value) / self->sr;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * RandDur_getServer(RandDur* self) { GET_SERVER };
static PyObject * RandDur_getStream(RandDur* self) { GET_STREAM };
static PyObject * RandDur_setMul(RandDur *self, PyObject *arg) { SET_MUL };
static PyObject * RandDur_setAdd(RandDur *self, PyObject *arg) { SET_ADD };
static PyObject * RandDur_setSub(RandDur *self, PyObject *arg) { SET_SUB };
static PyObject * RandDur_setDiv(RandDur *self, PyObject *arg) { SET_DIV };

static PyObject * RandDur_play(RandDur *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * RandDur_out(RandDur *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * RandDur_stop(RandDur *self) { STOP };

static PyObject * RandDur_multiply(RandDur *self, PyObject *arg) { MULTIPLY };
static PyObject * RandDur_inplace_multiply(RandDur *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * RandDur_add(RandDur *self, PyObject *arg) { ADD };
static PyObject * RandDur_inplace_add(RandDur *self, PyObject *arg) { INPLACE_ADD };
static PyObject * RandDur_sub(RandDur *self, PyObject *arg) { SUB };
static PyObject * RandDur_inplace_sub(RandDur *self, PyObject *arg) { INPLACE_SUB };
static PyObject * RandDur_div(RandDur *self, PyObject *arg) { DIV };
static PyObject * RandDur_inplace_div(RandDur *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
RandDur_setMin(RandDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
RandDur_setMax(RandDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef RandDur_members[] = {
    {"server", T_OBJECT_EX, offsetof(RandDur, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(RandDur, stream), 0, "Stream object."},
    {"min", T_OBJECT_EX, offsetof(RandDur, min), 0, "Minimum possible value."},
    {"max", T_OBJECT_EX, offsetof(RandDur, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(RandDur, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(RandDur, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef RandDur_methods[] = {
    {"getServer", (PyCFunction)RandDur_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)RandDur_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)RandDur_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)RandDur_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)RandDur_stop, METH_NOARGS, "Stops computing."},
    {"setMin", (PyCFunction)RandDur_setMin, METH_O, "Sets minimum possible value."},
    {"setMax", (PyCFunction)RandDur_setMax, METH_O, "Sets maximum possible value."},
    {"setMul", (PyCFunction)RandDur_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)RandDur_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)RandDur_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)RandDur_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods RandDur_as_number = {
    (binaryfunc)RandDur_add,                         /*nb_add*/
    (binaryfunc)RandDur_sub,                         /*nb_subtract*/
    (binaryfunc)RandDur_multiply,                    /*nb_multiply*/
    (binaryfunc)RandDur_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)RandDur_inplace_add,                 /*inplace_add*/
    (binaryfunc)RandDur_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)RandDur_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)RandDur_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject RandDurType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.RandDur_base",                                   /*tp_name*/
    sizeof(RandDur),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)RandDur_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &RandDur_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "RandDur objects. Recursive time varying generation of random values.",           /* tp_doc */
    (traverseproc)RandDur_traverse,                  /* tp_traverse */
    (inquiry)RandDur_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    RandDur_methods,                                 /* tp_methods */
    RandDur_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    RandDur_new,                                     /* tp_new */
};

/****************/
/**** Xnoise *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *x1;
    PyObject *x2;
    PyObject *freq;
    Stream *x1_stream;
    Stream *x2_stream;
    Stream *freq_stream;
    MYFLT (*type_func_ptr)();
    MYFLT xx1;
    MYFLT xx2;
    int type;
    MYFLT value;
    MYFLT time;
    MYFLT lastPoissonX1;
    int poisson_tab;
    MYFLT poisson_buffer[2000];
    MYFLT walkerValue;
    MYFLT loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[5]; // need at least 2 slots for mul & add
} Xnoise;

// no parameter
static MYFLT
Xnoise_uniform(Xnoise *self) {
    return RANDOM_UNIFORM;
}

static MYFLT
Xnoise_linear_min(Xnoise *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static MYFLT
Xnoise_linear_max(Xnoise *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static MYFLT
Xnoise_triangle(Xnoise *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
Xnoise_expon_min(Xnoise *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = -MYLOG(RANDOM_UNIFORM) / self->xx1;
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
Xnoise_expon_max(Xnoise *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = 1.0 - (-MYLOG(RANDOM_UNIFORM) / self->xx1);
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
Xnoise_biexpon(Xnoise *self) {
    MYFLT polar, val;
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG(sum) / self->xx1) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
Xnoise_cauchy(Xnoise *self) {
    MYFLT rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (rand() < (RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->xx1 * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static MYFLT
Xnoise_weibull(Xnoise *self) {
    MYFLT rnd, val;
    if (self->xx2 <= 0.0) self->xx2 = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * MYPOW(MYLOG(rnd), (1.0 / self->xx2));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static MYFLT
Xnoise_gaussian(Xnoise *self) {
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static MYFLT
Xnoise_poisson(Xnoise *self) {
    int i, j, factorial;
    long tot;
    MYFLT val;
    if (self->xx1 < 0.1) self->xx1 = 0.1;
    if (self->xx2 < 0.1) self->xx2 = 0.1;

    if (self->xx1 != self->lastPoissonX1) {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;
        for (i=1; i<12; i++) {
            factorial *= i;
            tot = (long)(1000.0 * (MYPOW(2.7182818, -self->xx1) * MYPOW(self->xx1, i) / factorial));
            for (j=0; j<tot; j++) {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }
    val = self->poisson_buffer[rand() % self->poisson_tab] / 12.0 * self->xx2;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static MYFLT
Xnoise_walker(Xnoise *self) {
    int modulo, dir;

    if (self->xx2 < 0.002) self->xx2 = 0.002;

    modulo = (int)(self->xx2 * 1000.0);
    dir = rand() % 2;

    if (dir == 0)
        self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
    else
        self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;

    return self->walkerValue;
}

// x1 = max value, x2 = max step
static MYFLT
Xnoise_loopseg(Xnoise *self) {
    int modulo, dir;

    if (self->loopChoice == 0) {

        self->loopCountPlay = self->loopTime = 0;

        if (self->xx2 < 0.002) self->xx2 = 0.002;

        modulo = (int)(self->xx2 * 1000.0);
        dir = rand() % 2;

        if (dir == 0)
            self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
        else
            self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;

        self->loop_buffer[self->loopCountRec++] = self->walkerValue;

        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else {
            self->loopChoice = 1;
            self->loopStop = (rand() % 4) + 1;
        }
    }
    else {
        self->loopCountRec = 0;

        self->walkerValue = self->loop_buffer[self->loopCountPlay++];

        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else {
            self->loopCountPlay = 0;
            self->loopTime++;
        }

        if (self->loopTime == self->loopStop) {
            self->loopChoice = 0;
            self->loopLen = (rand() % 10) + 3;
        }
    }

    return self->walkerValue;
}

static void
Xnoise_generate_iii(Xnoise *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_aii(Xnoise *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_iai(Xnoise *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_aai(Xnoise *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_iia(Xnoise *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_aia(Xnoise *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_iaa(Xnoise *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void
Xnoise_generate_aaa(Xnoise *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }
        self->data[i] = self->value;
    }
}

static void Xnoise_postprocessing_ii(Xnoise *self) { POST_PROCESSING_II };
static void Xnoise_postprocessing_ai(Xnoise *self) { POST_PROCESSING_AI };
static void Xnoise_postprocessing_ia(Xnoise *self) { POST_PROCESSING_IA };
static void Xnoise_postprocessing_aa(Xnoise *self) { POST_PROCESSING_AA };
static void Xnoise_postprocessing_ireva(Xnoise *self) { POST_PROCESSING_IREVA };
static void Xnoise_postprocessing_areva(Xnoise *self) { POST_PROCESSING_AREVA };
static void Xnoise_postprocessing_revai(Xnoise *self) { POST_PROCESSING_REVAI };
static void Xnoise_postprocessing_revaa(Xnoise *self) { POST_PROCESSING_REVAA };
static void Xnoise_postprocessing_revareva(Xnoise *self) { POST_PROCESSING_REVAREVA };

static void
Xnoise_setRandomType(Xnoise *self)
{

    switch (self->type) {
        case 0:
            self->type_func_ptr = Xnoise_uniform;
            break;
        case 1:
            self->type_func_ptr = Xnoise_linear_min;
            break;
        case 2:
            self->type_func_ptr = Xnoise_linear_max;
            break;
        case 3:
            self->type_func_ptr = Xnoise_triangle;
            break;
        case 4:
            self->type_func_ptr = Xnoise_expon_min;
            break;
        case 5:
            self->type_func_ptr = Xnoise_expon_max;
            break;
        case 6:
            self->type_func_ptr = Xnoise_biexpon;
            break;
        case 7:
            self->type_func_ptr = Xnoise_cauchy;
            break;
        case 8:
            self->type_func_ptr = Xnoise_weibull;
            break;
        case 9:
            self->type_func_ptr = Xnoise_gaussian;
            break;
        case 10:
            self->type_func_ptr = Xnoise_poisson;
            break;
        case 11:
            self->type_func_ptr = Xnoise_walker;
            break;
        case 12:
            self->type_func_ptr = Xnoise_loopseg;
            break;
    }
}

static void
Xnoise_setProcMode(Xnoise *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Xnoise_generate_iii;
            break;
        case 1:
            self->proc_func_ptr = Xnoise_generate_aii;
            break;
        case 10:
            self->proc_func_ptr = Xnoise_generate_iai;
            break;
        case 11:
            self->proc_func_ptr = Xnoise_generate_aai;
            break;
        case 100:
            self->proc_func_ptr = Xnoise_generate_iia;
            break;
        case 101:
            self->proc_func_ptr = Xnoise_generate_aia;
            break;
        case 110:
            self->proc_func_ptr = Xnoise_generate_iaa;
            break;
        case 111:
            self->proc_func_ptr = Xnoise_generate_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Xnoise_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Xnoise_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Xnoise_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Xnoise_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Xnoise_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Xnoise_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Xnoise_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Xnoise_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Xnoise_postprocessing_revareva;
            break;
    }
}

static void
Xnoise_compute_next_data_frame(Xnoise *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Xnoise_traverse(Xnoise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->x1);
    Py_VISIT(self->x1_stream);
    Py_VISIT(self->x2);
    Py_VISIT(self->x2_stream);
    return 0;
}

static int
Xnoise_clear(Xnoise *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->x1);
    Py_CLEAR(self->x1_stream);
    Py_CLEAR(self->x2);
    Py_CLEAR(self->x2_stream);
    return 0;
}

static void
Xnoise_dealloc(Xnoise* self)
{
    pyo_DEALLOC
    Xnoise_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Xnoise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp=NULL, *x1tmp=NULL, *x2tmp=NULL, *multmp=NULL, *addtmp=NULL;
    Xnoise *self;
    self = (Xnoise *)type->tp_alloc(type, 0);

    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->freq = PyFloat_FromDouble(1.);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON

    Server_generateSeed((Server *)self->server, XNOISE_ID);

    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;
    for (i=0; i<2000; i++) {
        self->poisson_buffer[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->loop_buffer[i] = 0.0;
    }
    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;
    self->loopLen = (rand() % 10) + 3;

    Stream_setFunctionPtr(self->stream, Xnoise_compute_next_data_frame);
    self->mode_func_ptr = Xnoise_setProcMode;

    static char *kwlist[] = {"type", "freq", "x1", "x2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOOOOO", kwlist, &self->type, &freqtmp, &x1tmp, &x2tmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (x1tmp) {
        PyObject_CallMethod((PyObject *)self, "setX1", "O", x1tmp);
    }

    if (x2tmp) {
        PyObject_CallMethod((PyObject *)self, "setX2", "O", x2tmp);
    }

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

    Xnoise_setRandomType(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Xnoise_getServer(Xnoise* self) { GET_SERVER };
static PyObject * Xnoise_getStream(Xnoise* self) { GET_STREAM };
static PyObject * Xnoise_setMul(Xnoise *self, PyObject *arg) { SET_MUL };
static PyObject * Xnoise_setAdd(Xnoise *self, PyObject *arg) { SET_ADD };
static PyObject * Xnoise_setSub(Xnoise *self, PyObject *arg) { SET_SUB };
static PyObject * Xnoise_setDiv(Xnoise *self, PyObject *arg) { SET_DIV };

static PyObject * Xnoise_play(Xnoise *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Xnoise_out(Xnoise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Xnoise_stop(Xnoise *self) { STOP };

static PyObject * Xnoise_multiply(Xnoise *self, PyObject *arg) { MULTIPLY };
static PyObject * Xnoise_inplace_multiply(Xnoise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Xnoise_add(Xnoise *self, PyObject *arg) { ADD };
static PyObject * Xnoise_inplace_add(Xnoise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Xnoise_sub(Xnoise *self, PyObject *arg) { SUB };
static PyObject * Xnoise_inplace_sub(Xnoise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Xnoise_div(Xnoise *self, PyObject *arg) { DIV };
static PyObject * Xnoise_inplace_div(Xnoise *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Xnoise_setType(Xnoise *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyInt_Check(arg);

	if (isNumber == 1) {
		self->type = PyInt_AsLong(arg);
        Xnoise_setRandomType(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Xnoise_setX1(Xnoise *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x1);
	if (isNumber == 1) {
		self->x1 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->x1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x1_stream);
        self->x1_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Xnoise_setX2(Xnoise *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x2);
	if (isNumber == 1) {
		self->x2 = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->x2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x2_stream);
        self->x2_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Xnoise_setFreq(Xnoise *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Xnoise_members[] = {
    {"server", T_OBJECT_EX, offsetof(Xnoise, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Xnoise, stream), 0, "Stream object."},
    {"x1", T_OBJECT_EX, offsetof(Xnoise, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(Xnoise, x2), 0, "second param."},
    {"freq", T_OBJECT_EX, offsetof(Xnoise, freq), 0, "Polling frequency."},
    {"mul", T_OBJECT_EX, offsetof(Xnoise, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Xnoise, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Xnoise_methods[] = {
    {"getServer", (PyCFunction)Xnoise_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Xnoise_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Xnoise_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Xnoise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Xnoise_stop, METH_NOARGS, "Stops computing."},
    {"setType", (PyCFunction)Xnoise_setType, METH_O, "Sets distribution type."},
    {"setX1", (PyCFunction)Xnoise_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)Xnoise_setX2, METH_O, "Sets second param."},
    {"setFreq", (PyCFunction)Xnoise_setFreq, METH_O, "Sets polling frequency."},
    {"setMul", (PyCFunction)Xnoise_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Xnoise_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Xnoise_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Xnoise_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Xnoise_as_number = {
    (binaryfunc)Xnoise_add,                         /*nb_add*/
    (binaryfunc)Xnoise_sub,                         /*nb_subtract*/
    (binaryfunc)Xnoise_multiply,                    /*nb_multiply*/
    (binaryfunc)Xnoise_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)Xnoise_inplace_add,                 /*inplace_add*/
    (binaryfunc)Xnoise_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Xnoise_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Xnoise_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject XnoiseType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Xnoise_base",                                   /*tp_name*/
    sizeof(Xnoise),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Xnoise_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Xnoise_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Xnoise objects. Periodically generates a new random value.",           /* tp_doc */
    (traverseproc)Xnoise_traverse,                  /* tp_traverse */
    (inquiry)Xnoise_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Xnoise_methods,                                 /* tp_methods */
    Xnoise_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Xnoise_new,                                     /* tp_new */
};

/****************/
/**** XnoiseMidi *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *x1;
    PyObject *x2;
    PyObject *freq;
    Stream *x1_stream;
    Stream *x2_stream;
    Stream *freq_stream;
    MYFLT (*type_func_ptr)();
    int scale; // 0 = Midi, 1 = frequency, 2 = transpo
    MYFLT xx1;
    MYFLT xx2;
    int range_min;
    int range_max;
    int centralkey;
    int type;
    MYFLT value;
    MYFLT time;
    MYFLT lastPoissonX1;
    int poisson_tab;
    MYFLT poisson_buffer[2000];
    MYFLT walkerValue;
    MYFLT loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[5]; // need at least 2 slots for mul & add
} XnoiseMidi;

static MYFLT
XnoiseMidi_convert(XnoiseMidi *self) {
    int midival;
    MYFLT val;

    midival = (int)((self->value * (self->range_max-self->range_min)) + self->range_min);

    if (midival < 0)
        midival = 0;
    else if (midival > 127)
        midival = 127;

    if (self->scale == 0)
        val = (MYFLT)midival;
    else if (self->scale == 1)
        val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
    else if (self->scale == 2)
        val = MYPOW(1.0594630943593, midival - self->centralkey);
    else
        val = midival;

    return val;
}

// no parameter
static MYFLT
XnoiseMidi_uniform(XnoiseMidi *self) {
    return RANDOM_UNIFORM;
}

static MYFLT
XnoiseMidi_linear_min(XnoiseMidi *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static MYFLT
XnoiseMidi_linear_max(XnoiseMidi *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static MYFLT
XnoiseMidi_triangle(XnoiseMidi *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
XnoiseMidi_expon_min(XnoiseMidi *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = -MYLOG(RANDOM_UNIFORM) / self->xx1;
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
XnoiseMidi_expon_max(XnoiseMidi *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = 1.0 - (-MYLOG(RANDOM_UNIFORM) / self->xx1);
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
XnoiseMidi_biexpon(XnoiseMidi *self) {
    MYFLT polar, val;
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG(sum) / self->xx1) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
XnoiseMidi_cauchy(XnoiseMidi *self) {
    MYFLT rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (rand() < (RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->xx1 * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static MYFLT
XnoiseMidi_weibull(XnoiseMidi *self) {
    MYFLT rnd, val;
    if (self->xx2 <= 0.0) self->xx2 = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * MYPOW(MYLOG(rnd), (1.0 / self->xx2));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static MYFLT
XnoiseMidi_gaussian(XnoiseMidi *self) {
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static MYFLT
XnoiseMidi_poisson(XnoiseMidi *self) {
    int i, j, factorial;
    long tot;
    MYFLT val;
    if (self->xx1 < 0.1) self->xx1 = 0.1;
    if (self->xx2 < 0.1) self->xx2 = 0.1;

    if (self->xx1 != self->lastPoissonX1) {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;
        for (i=1; i<12; i++) {
            factorial *= i;
            tot = (long)(1000.0 * (MYPOW(2.7182818, -self->xx1) * MYPOW(self->xx1, i) / factorial));
            for (j=0; j<tot; j++) {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }
    val = self->poisson_buffer[rand() % self->poisson_tab] / 12.0 * self->xx2;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static MYFLT
XnoiseMidi_walker(XnoiseMidi *self) {
    int modulo, dir;

    if (self->xx2 < 0.002) self->xx2 = 0.002;

    modulo = (int)(self->xx2 * 1000.0);
    dir = rand() % 2;

    if (dir == 0)
        self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
    else
        self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;

    return self->walkerValue;
}

// x1 = max value, x2 = max step
static MYFLT
XnoiseMidi_loopseg(XnoiseMidi *self) {
    int modulo, dir;

    if (self->loopChoice == 0) {

        self->loopCountPlay = self->loopTime = 0;

        if (self->xx2 < 0.002) self->xx2 = 0.002;

        modulo = (int)(self->xx2 * 1000.0);
        dir = rand() % 2;

        if (dir == 0)
            self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
        else
            self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;

        self->loop_buffer[self->loopCountRec++] = self->walkerValue;

        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else {
            self->loopChoice = 1;
            self->loopStop = (rand() % 4) + 1;
        }
    }
    else {
        self->loopCountRec = 0;

        self->walkerValue = self->loop_buffer[self->loopCountPlay++];

        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else {
            self->loopCountPlay = 0;
            self->loopTime++;
        }

        if (self->loopTime == self->loopStop) {
            self->loopChoice = 0;
            self->loopLen = (rand() % 10) + 3;
        }
    }

    return self->walkerValue;
}

static void
XnoiseMidi_generate_iii(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_aii(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_iai(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_aai(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_iia(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_aia(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_iaa(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void
XnoiseMidi_generate_aaa(XnoiseMidi *self) {
    int i;
    MYFLT inc;
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = XnoiseMidi_convert(self);
        }
        self->data[i] = self->value;
    }
}

static void XnoiseMidi_postprocessing_ii(XnoiseMidi *self) { POST_PROCESSING_II };
static void XnoiseMidi_postprocessing_ai(XnoiseMidi *self) { POST_PROCESSING_AI };
static void XnoiseMidi_postprocessing_ia(XnoiseMidi *self) { POST_PROCESSING_IA };
static void XnoiseMidi_postprocessing_aa(XnoiseMidi *self) { POST_PROCESSING_AA };
static void XnoiseMidi_postprocessing_ireva(XnoiseMidi *self) { POST_PROCESSING_IREVA };
static void XnoiseMidi_postprocessing_areva(XnoiseMidi *self) { POST_PROCESSING_AREVA };
static void XnoiseMidi_postprocessing_revai(XnoiseMidi *self) { POST_PROCESSING_REVAI };
static void XnoiseMidi_postprocessing_revaa(XnoiseMidi *self) { POST_PROCESSING_REVAA };
static void XnoiseMidi_postprocessing_revareva(XnoiseMidi *self) { POST_PROCESSING_REVAREVA };

static void
XnoiseMidi_setRandomType(XnoiseMidi *self)
{

    switch (self->type) {
        case 0:
            self->type_func_ptr = XnoiseMidi_uniform;
            break;
        case 1:
            self->type_func_ptr = XnoiseMidi_linear_min;
            break;
        case 2:
            self->type_func_ptr = XnoiseMidi_linear_max;
            break;
        case 3:
            self->type_func_ptr = XnoiseMidi_triangle;
            break;
        case 4:
            self->type_func_ptr = XnoiseMidi_expon_min;
            break;
        case 5:
            self->type_func_ptr = XnoiseMidi_expon_max;
            break;
        case 6:
            self->type_func_ptr = XnoiseMidi_biexpon;
            break;
        case 7:
            self->type_func_ptr = XnoiseMidi_cauchy;
            break;
        case 8:
            self->type_func_ptr = XnoiseMidi_weibull;
            break;
        case 9:
            self->type_func_ptr = XnoiseMidi_gaussian;
            break;
        case 10:
            self->type_func_ptr = XnoiseMidi_poisson;
            break;
        case 11:
            self->type_func_ptr = XnoiseMidi_walker;
            break;
        case 12:
            self->type_func_ptr = XnoiseMidi_loopseg;
            break;
    }
}

static void
XnoiseMidi_setProcMode(XnoiseMidi *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = XnoiseMidi_generate_iii;
            break;
        case 1:
            self->proc_func_ptr = XnoiseMidi_generate_aii;
            break;
        case 10:
            self->proc_func_ptr = XnoiseMidi_generate_iai;
            break;
        case 11:
            self->proc_func_ptr = XnoiseMidi_generate_aai;
            break;
        case 100:
            self->proc_func_ptr = XnoiseMidi_generate_iia;
            break;
        case 101:
            self->proc_func_ptr = XnoiseMidi_generate_aia;
            break;
        case 110:
            self->proc_func_ptr = XnoiseMidi_generate_iaa;
            break;
        case 111:
            self->proc_func_ptr = XnoiseMidi_generate_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = XnoiseMidi_postprocessing_revareva;
            break;
    }
}

static void
XnoiseMidi_compute_next_data_frame(XnoiseMidi *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
XnoiseMidi_traverse(XnoiseMidi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->x1);
    Py_VISIT(self->x1_stream);
    Py_VISIT(self->x2);
    Py_VISIT(self->x2_stream);
    return 0;
}

static int
XnoiseMidi_clear(XnoiseMidi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->x1);
    Py_CLEAR(self->x1_stream);
    Py_CLEAR(self->x2);
    Py_CLEAR(self->x2_stream);
    return 0;
}

static void
XnoiseMidi_dealloc(XnoiseMidi* self)
{
    pyo_DEALLOC
    XnoiseMidi_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
XnoiseMidi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp=NULL, *x1tmp=NULL, *x2tmp=NULL, *rangetmp=NULL, *multmp=NULL, *addtmp=NULL;
    XnoiseMidi *self;
    self = (XnoiseMidi *)type->tp_alloc(type, 0);

    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->freq = PyFloat_FromDouble(1.);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
    self->time = 1.0;
    self->scale = 0;
    self->range_min = 0;
    self->range_max = 127;
    self->centralkey = 64;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON

    Server_generateSeed((Server *)self->server, XNOISEMIDI_ID);

    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;
    for (i=0; i<2000; i++) {
        self->poisson_buffer[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->loop_buffer[i] = 0.0;
    }
    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;
    self->loopLen = (rand() % 10) + 3;

    Stream_setFunctionPtr(self->stream, XnoiseMidi_compute_next_data_frame);
    self->mode_func_ptr = XnoiseMidi_setProcMode;

    static char *kwlist[] = {"type", "freq", "x1", "x2", "scale", "range", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOOOiOOO", kwlist, &self->type, &freqtmp, &x1tmp, &x2tmp, &self->scale, &rangetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (x1tmp) {
        PyObject_CallMethod((PyObject *)self, "setX1", "O", x1tmp);
    }

    if (x2tmp) {
        PyObject_CallMethod((PyObject *)self, "setX2", "O", x2tmp);
    }

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (rangetmp) {
        PyObject_CallMethod((PyObject *)self, "setRange", "O", rangetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    XnoiseMidi_setRandomType(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * XnoiseMidi_getServer(XnoiseMidi* self) { GET_SERVER };
static PyObject * XnoiseMidi_getStream(XnoiseMidi* self) { GET_STREAM };
static PyObject * XnoiseMidi_setMul(XnoiseMidi *self, PyObject *arg) { SET_MUL };
static PyObject * XnoiseMidi_setAdd(XnoiseMidi *self, PyObject *arg) { SET_ADD };
static PyObject * XnoiseMidi_setSub(XnoiseMidi *self, PyObject *arg) { SET_SUB };
static PyObject * XnoiseMidi_setDiv(XnoiseMidi *self, PyObject *arg) { SET_DIV };

static PyObject * XnoiseMidi_play(XnoiseMidi *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * XnoiseMidi_out(XnoiseMidi *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * XnoiseMidi_stop(XnoiseMidi *self) { STOP };

static PyObject * XnoiseMidi_multiply(XnoiseMidi *self, PyObject *arg) { MULTIPLY };
static PyObject * XnoiseMidi_inplace_multiply(XnoiseMidi *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * XnoiseMidi_add(XnoiseMidi *self, PyObject *arg) { ADD };
static PyObject * XnoiseMidi_inplace_add(XnoiseMidi *self, PyObject *arg) { INPLACE_ADD };
static PyObject * XnoiseMidi_sub(XnoiseMidi *self, PyObject *arg) { SUB };
static PyObject * XnoiseMidi_inplace_sub(XnoiseMidi *self, PyObject *arg) { INPLACE_SUB };
static PyObject * XnoiseMidi_div(XnoiseMidi *self, PyObject *arg) { DIV };
static PyObject * XnoiseMidi_inplace_div(XnoiseMidi *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
XnoiseMidi_setType(XnoiseMidi *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyInt_Check(arg);

	if (isNumber == 1) {
		self->type = PyInt_AsLong(arg);
        XnoiseMidi_setRandomType(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseMidi_setScale(XnoiseMidi *self, PyObject *arg)
{
    int tmp;
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyInt_Check(arg);

	if (isNumber == 1) {
		tmp = PyInt_AsLong(arg);
        if (tmp >= 0 && tmp <= 2)
            self->scale = tmp;
        else
            printf("scale attribute must be an integer {0, 1, 2}\n");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseMidi_setRange(XnoiseMidi *self, PyObject *args)
{
	if (args == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isTuple = PyTuple_Check(args);

	if (isTuple == 1) {
        self->range_min = PyInt_AsLong(PyTuple_GET_ITEM(args, 0));
        self->range_max = PyInt_AsLong(PyTuple_GET_ITEM(args, 1));
        self->centralkey = (int)((self->range_max + self->range_min) / 2);
	}

    Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseMidi_setX1(XnoiseMidi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x1);
	if (isNumber == 1) {
		self->x1 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->x1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x1_stream);
        self->x1_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseMidi_setX2(XnoiseMidi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x2);
	if (isNumber == 1) {
		self->x2 = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->x2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x2_stream);
        self->x2_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseMidi_setFreq(XnoiseMidi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef XnoiseMidi_members[] = {
    {"server", T_OBJECT_EX, offsetof(XnoiseMidi, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(XnoiseMidi, stream), 0, "Stream object."},
    {"x1", T_OBJECT_EX, offsetof(XnoiseMidi, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(XnoiseMidi, x2), 0, "second param."},
    {"freq", T_OBJECT_EX, offsetof(XnoiseMidi, freq), 0, "Polling frequency."},
    {"mul", T_OBJECT_EX, offsetof(XnoiseMidi, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(XnoiseMidi, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef XnoiseMidi_methods[] = {
    {"getServer", (PyCFunction)XnoiseMidi_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)XnoiseMidi_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)XnoiseMidi_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)XnoiseMidi_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)XnoiseMidi_stop, METH_NOARGS, "Stops computing."},
    {"setType", (PyCFunction)XnoiseMidi_setType, METH_O, "Sets distribution type."},
    {"setScale", (PyCFunction)XnoiseMidi_setScale, METH_O, "Sets output scale."},
    {"setRange", (PyCFunction)XnoiseMidi_setRange, METH_VARARGS, "Sets range in midi notes (min, max)."},
    {"setX1", (PyCFunction)XnoiseMidi_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)XnoiseMidi_setX2, METH_O, "Sets second param."},
    {"setFreq", (PyCFunction)XnoiseMidi_setFreq, METH_O, "Sets polling frequency."},
    {"setMul", (PyCFunction)XnoiseMidi_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)XnoiseMidi_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)XnoiseMidi_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)XnoiseMidi_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods XnoiseMidi_as_number = {
    (binaryfunc)XnoiseMidi_add,                         /*nb_add*/
    (binaryfunc)XnoiseMidi_sub,                         /*nb_subtract*/
    (binaryfunc)XnoiseMidi_multiply,                    /*nb_multiply*/
    (binaryfunc)XnoiseMidi_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)XnoiseMidi_inplace_add,                 /*inplace_add*/
    (binaryfunc)XnoiseMidi_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)XnoiseMidi_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)XnoiseMidi_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject XnoiseMidiType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.XnoiseMidi_base",                                   /*tp_name*/
    sizeof(XnoiseMidi),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)XnoiseMidi_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &XnoiseMidi_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "XnoiseMidi objects. Periodically generates a new random value.",           /* tp_doc */
    (traverseproc)XnoiseMidi_traverse,                  /* tp_traverse */
    (inquiry)XnoiseMidi_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    XnoiseMidi_methods,                                 /* tp_methods */
    XnoiseMidi_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    XnoiseMidi_new,                                     /* tp_new */
};

/****************/
/**** XnoiseDur *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *x1;
    PyObject *x2;
    PyObject *min;
    PyObject *max;
    Stream *x1_stream;
    Stream *x2_stream;
    Stream *min_stream;
    Stream *max_stream;
    MYFLT (*type_func_ptr)();
    MYFLT xx1;
    MYFLT xx2;
    int type;
    MYFLT value;
    MYFLT time;
    MYFLT inc;
    MYFLT lastPoissonX1;
    int poisson_tab;
    MYFLT poisson_buffer[2000];
    MYFLT walkerValue;
    MYFLT loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[6]; // need at least 2 slots for mul & add
} XnoiseDur;

// no parameter
static MYFLT
XnoiseDur_uniform(XnoiseDur *self) {
    return RANDOM_UNIFORM;
}

static MYFLT
XnoiseDur_linear_min(XnoiseDur *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static MYFLT
XnoiseDur_linear_max(XnoiseDur *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static MYFLT
XnoiseDur_triangle(XnoiseDur *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
XnoiseDur_expon_min(XnoiseDur *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = -MYLOG(RANDOM_UNIFORM) / self->xx1;
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
XnoiseDur_expon_max(XnoiseDur *self) {
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT val = 1.0 - (-MYLOG(RANDOM_UNIFORM) / self->xx1);
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
XnoiseDur_biexpon(XnoiseDur *self) {
    MYFLT polar, val;
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;
    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG(sum) / self->xx1) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
XnoiseDur_cauchy(XnoiseDur *self) {
    MYFLT rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (rand() < (RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->xx1 * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static MYFLT
XnoiseDur_weibull(XnoiseDur *self) {
    MYFLT rnd, val;
    if (self->xx2 <= 0.0) self->xx2 = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * MYPOW(MYLOG(rnd), (1.0 / self->xx2));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static MYFLT
XnoiseDur_gaussian(XnoiseDur *self) {
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static MYFLT
XnoiseDur_poisson(XnoiseDur *self) {
    int i, j, factorial;
    long tot;
    MYFLT val;
    if (self->xx1 < 0.1) self->xx1 = 0.1;
    if (self->xx2 < 0.1) self->xx2 = 0.1;

    if (self->xx1 != self->lastPoissonX1) {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;
        for (i=1; i<12; i++) {
            factorial *= i;
            tot = (long)(1000.0 * (MYPOW(2.7182818, -self->xx1) * MYPOW(self->xx1, i) / factorial));
            for (j=0; j<tot; j++) {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }
    val = self->poisson_buffer[rand() % self->poisson_tab] / 12.0 * self->xx2;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static MYFLT
XnoiseDur_walker(XnoiseDur *self) {
    int modulo, dir;

    if (self->xx2 < 0.002) self->xx2 = 0.002;

    modulo = (int)(self->xx2 * 1000.0);
    dir = rand() % 2;

    if (dir == 0)
        self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
    else
        self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;

    return self->walkerValue;
}

// x1 = max value, x2 = max step
static MYFLT
XnoiseDur_loopseg(XnoiseDur *self) {
    int modulo, dir;

    if (self->loopChoice == 0) {

        self->loopCountPlay = self->loopTime = 0;

        if (self->xx2 < 0.002) self->xx2 = 0.002;

        modulo = (int)(self->xx2 * 1000.0);
        dir = rand() % 2;

        if (dir == 0)
            self->walkerValue = self->walkerValue + (((rand() % modulo) - (modulo / 2)) * 0.001);
        else
            self->walkerValue = self->walkerValue - (((rand() % modulo) - (modulo / 2)) * 0.001);

        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;

        self->loop_buffer[self->loopCountRec++] = self->walkerValue;

        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else {
            self->loopChoice = 1;
            self->loopStop = (rand() % 4) + 1;
        }
    }
    else {
        self->loopCountRec = 0;

        self->walkerValue = self->loop_buffer[self->loopCountPlay++];

        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else {
            self->loopCountPlay = 0;
            self->loopTime++;
        }

        if (self->loopTime == self->loopStop) {
            self->loopChoice = 0;
            self->loopLen = (rand() % 10) + 3;
        }
    }

    return self->walkerValue;
}

static void
XnoiseDur_generate(XnoiseDur *self) {
    int i;
    MYFLT min, max;

    for (i=0; i<self->bufsize; i++) {
        self->time += self->inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            if (self->modebuffer[2] == 0)
                self->xx1 = PyFloat_AS_DOUBLE(self->x1);
            else
                self->xx1 = Stream_getData((Stream *)self->x1_stream)[i];
            if (self->modebuffer[3] == 0)
                self->xx2 = PyFloat_AS_DOUBLE(self->x2);
            else
                self->xx2 = Stream_getData((Stream *)self->x2_stream)[i];
            if (self->modebuffer[4] == 0)
                min = PyFloat_AS_DOUBLE(self->min);
            else
                min = Stream_getData((Stream *)self->min_stream)[i];
            if (self->modebuffer[5] == 0)
                max = PyFloat_AS_DOUBLE(self->max);
            else
                max = Stream_getData((Stream *)self->max_stream)[i];
            if (min > max)
                max = min;
            self->value = (*self->type_func_ptr)(self) * (max - min) + min;
            if (self->value == 0.0)
                self->inc = 0.0;
            else
                self->inc = (1.0 / self->value) / self->sr;
        }
        self->data[i] = self->value;
    }
}

static void XnoiseDur_postprocessing_ii(XnoiseDur *self) { POST_PROCESSING_II };
static void XnoiseDur_postprocessing_ai(XnoiseDur *self) { POST_PROCESSING_AI };
static void XnoiseDur_postprocessing_ia(XnoiseDur *self) { POST_PROCESSING_IA };
static void XnoiseDur_postprocessing_aa(XnoiseDur *self) { POST_PROCESSING_AA };
static void XnoiseDur_postprocessing_ireva(XnoiseDur *self) { POST_PROCESSING_IREVA };
static void XnoiseDur_postprocessing_areva(XnoiseDur *self) { POST_PROCESSING_AREVA };
static void XnoiseDur_postprocessing_revai(XnoiseDur *self) { POST_PROCESSING_REVAI };
static void XnoiseDur_postprocessing_revaa(XnoiseDur *self) { POST_PROCESSING_REVAA };
static void XnoiseDur_postprocessing_revareva(XnoiseDur *self) { POST_PROCESSING_REVAREVA };

static void
XnoiseDur_setRandomType(XnoiseDur *self)
{

    switch (self->type) {
        case 0:
            self->type_func_ptr = XnoiseDur_uniform;
            break;
        case 1:
            self->type_func_ptr = XnoiseDur_linear_min;
            break;
        case 2:
            self->type_func_ptr = XnoiseDur_linear_max;
            break;
        case 3:
            self->type_func_ptr = XnoiseDur_triangle;
            break;
        case 4:
            self->type_func_ptr = XnoiseDur_expon_min;
            break;
        case 5:
            self->type_func_ptr = XnoiseDur_expon_max;
            break;
        case 6:
            self->type_func_ptr = XnoiseDur_biexpon;
            break;
        case 7:
            self->type_func_ptr = XnoiseDur_cauchy;
            break;
        case 8:
            self->type_func_ptr = XnoiseDur_weibull;
            break;
        case 9:
            self->type_func_ptr = XnoiseDur_gaussian;
            break;
        case 10:
            self->type_func_ptr = XnoiseDur_poisson;
            break;
        case 11:
            self->type_func_ptr = XnoiseDur_walker;
            break;
        case 12:
            self->type_func_ptr = XnoiseDur_loopseg;
            break;
    }
}

static void
XnoiseDur_setProcMode(XnoiseDur *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = XnoiseDur_generate;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = XnoiseDur_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = XnoiseDur_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = XnoiseDur_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = XnoiseDur_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = XnoiseDur_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = XnoiseDur_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = XnoiseDur_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = XnoiseDur_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = XnoiseDur_postprocessing_revareva;
            break;
    }
}

static void
XnoiseDur_compute_next_data_frame(XnoiseDur *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
XnoiseDur_traverse(XnoiseDur *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->min);
    Py_VISIT(self->min_stream);
    Py_VISIT(self->max);
    Py_VISIT(self->max_stream);
    Py_VISIT(self->x1);
    Py_VISIT(self->x1_stream);
    Py_VISIT(self->x2);
    Py_VISIT(self->x2_stream);
    return 0;
}

static int
XnoiseDur_clear(XnoiseDur *self)
{
    pyo_CLEAR
    Py_CLEAR(self->min);
    Py_CLEAR(self->min_stream);
    Py_CLEAR(self->max);
    Py_CLEAR(self->max_stream);
    Py_CLEAR(self->x1);
    Py_CLEAR(self->x1_stream);
    Py_CLEAR(self->x2);
    Py_CLEAR(self->x2_stream);
    return 0;
}

static void
XnoiseDur_dealloc(XnoiseDur* self)
{
    pyo_DEALLOC
    XnoiseDur_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
XnoiseDur_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT mi, ma;
    PyObject *mintmp=NULL, *maxtmp=NULL, *x1tmp=NULL, *x2tmp=NULL, *multmp=NULL, *addtmp=NULL;
    XnoiseDur *self;
    self = (XnoiseDur *)type->tp_alloc(type, 0);

    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->min = PyFloat_FromDouble(0.0);
    self->max = PyFloat_FromDouble(1.0);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;

    INIT_OBJECT_COMMON

    Server_generateSeed((Server *)self->server, XNOISEDUR_ID);

    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;
    for (i=0; i<2000; i++) {
        self->poisson_buffer[i] = 0.0;
    }
    for (i=0; i<15; i++) {
        self->loop_buffer[i] = 0.0;
    }
    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;
    self->loopLen = (rand() % 10) + 3;

    Stream_setFunctionPtr(self->stream, XnoiseDur_compute_next_data_frame);
    self->mode_func_ptr = XnoiseDur_setProcMode;

    static char *kwlist[] = {"type", "min", "max", "x1", "x2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOOOOOO", kwlist, &self->type, &mintmp, &maxtmp, &x1tmp, &x2tmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (x1tmp) {
        PyObject_CallMethod((PyObject *)self, "setX1", "O", x1tmp);
    }

    if (x2tmp) {
        PyObject_CallMethod((PyObject *)self, "setX2", "O", x2tmp);
    }

    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }

    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->modebuffer[2] == 0)
        mi = PyFloat_AS_DOUBLE(self->min);
    else
        mi = Stream_getData((Stream *)self->min_stream)[0];
    if (self->modebuffer[3] == 0)
        ma = PyFloat_AS_DOUBLE(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];

    self->value = (mi + ma) * 0.5;
    if (self->value == 0.0)
        self->inc = 0.0;
    else
        self->inc = (1.0 / self->value) / self->sr;

    XnoiseDur_setRandomType(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * XnoiseDur_getServer(XnoiseDur* self) { GET_SERVER };
static PyObject * XnoiseDur_getStream(XnoiseDur* self) { GET_STREAM };
static PyObject * XnoiseDur_setMul(XnoiseDur *self, PyObject *arg) { SET_MUL };
static PyObject * XnoiseDur_setAdd(XnoiseDur *self, PyObject *arg) { SET_ADD };
static PyObject * XnoiseDur_setSub(XnoiseDur *self, PyObject *arg) { SET_SUB };
static PyObject * XnoiseDur_setDiv(XnoiseDur *self, PyObject *arg) { SET_DIV };

static PyObject * XnoiseDur_play(XnoiseDur *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * XnoiseDur_out(XnoiseDur *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * XnoiseDur_stop(XnoiseDur *self) { STOP };

static PyObject * XnoiseDur_multiply(XnoiseDur *self, PyObject *arg) { MULTIPLY };
static PyObject * XnoiseDur_inplace_multiply(XnoiseDur *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * XnoiseDur_add(XnoiseDur *self, PyObject *arg) { ADD };
static PyObject * XnoiseDur_inplace_add(XnoiseDur *self, PyObject *arg) { INPLACE_ADD };
static PyObject * XnoiseDur_sub(XnoiseDur *self, PyObject *arg) { SUB };
static PyObject * XnoiseDur_inplace_sub(XnoiseDur *self, PyObject *arg) { INPLACE_SUB };
static PyObject * XnoiseDur_div(XnoiseDur *self, PyObject *arg) { DIV };
static PyObject * XnoiseDur_inplace_div(XnoiseDur *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
XnoiseDur_setType(XnoiseDur *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyInt_Check(arg);

	if (isNumber == 1) {
		self->type = PyInt_AsLong(arg);
        XnoiseDur_setRandomType(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseDur_setX1(XnoiseDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x1);
	if (isNumber == 1) {
		self->x1 = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->x1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x1_stream);
        self->x1_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseDur_setX2(XnoiseDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->x2);
	if (isNumber == 1) {
		self->x2 = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->x2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->x2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->x2_stream);
        self->x2_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseDur_setMin(XnoiseDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
XnoiseDur_setMax(XnoiseDur *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef XnoiseDur_members[] = {
    {"server", T_OBJECT_EX, offsetof(XnoiseDur, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(XnoiseDur, stream), 0, "Stream object."},
    {"x1", T_OBJECT_EX, offsetof(XnoiseDur, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(XnoiseDur, x2), 0, "second param."},
    {"min", T_OBJECT_EX, offsetof(XnoiseDur, min), 0, "Minimum value."},
    {"max", T_OBJECT_EX, offsetof(XnoiseDur, max), 0, "Maximum value."},
    {"mul", T_OBJECT_EX, offsetof(XnoiseDur, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(XnoiseDur, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef XnoiseDur_methods[] = {
    {"getServer", (PyCFunction)XnoiseDur_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)XnoiseDur_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)XnoiseDur_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)XnoiseDur_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)XnoiseDur_stop, METH_NOARGS, "Stops computing."},
    {"setType", (PyCFunction)XnoiseDur_setType, METH_O, "Sets distribution type."},
    {"setX1", (PyCFunction)XnoiseDur_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)XnoiseDur_setX2, METH_O, "Sets second param."},
    {"setMin", (PyCFunction)XnoiseDur_setMin, METH_O, "Sets minimum value."},
    {"setMax", (PyCFunction)XnoiseDur_setMax, METH_O, "Sets maximum value."},
    {"setMul", (PyCFunction)XnoiseDur_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)XnoiseDur_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)XnoiseDur_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)XnoiseDur_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods XnoiseDur_as_number = {
    (binaryfunc)XnoiseDur_add,                         /*nb_add*/
    (binaryfunc)XnoiseDur_sub,                         /*nb_subtract*/
    (binaryfunc)XnoiseDur_multiply,                    /*nb_multiply*/
    (binaryfunc)XnoiseDur_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)XnoiseDur_inplace_add,                 /*inplace_add*/
    (binaryfunc)XnoiseDur_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)XnoiseDur_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)XnoiseDur_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject XnoiseDurType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.XnoiseDur_base",                                   /*tp_name*/
    sizeof(XnoiseDur),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)XnoiseDur_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &XnoiseDur_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "XnoiseDur objects. Generates a random value and uses this value as time base for the next generation.",           /* tp_doc */
    (traverseproc)XnoiseDur_traverse,                  /* tp_traverse */
    (inquiry)XnoiseDur_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    XnoiseDur_methods,                                 /* tp_methods */
    XnoiseDur_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    XnoiseDur_new,                                     /* tp_new */
};

/****************/
/****** Urn *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    int *list;
    int max;
    int length;
    int lastvalue;
    MYFLT value;
    MYFLT time;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
} Urn;

static void
Urn_reset(Urn *self) {
    int i;
    self->lastvalue = (int)self->value;
    self->length = self->max;
    self->list = (int *)realloc(self->list, self->max * sizeof(int));
    for (i=0; i<self->max; i++) {
        self->list[i] = i;
    }
}

static int
Urn_choose(Urn *self) {
    int x = 0;
    int value = 0;
    int i, pick;

    pick = rand() % self->length;
    while (pick == self->lastvalue)
        pick = rand() % self->length;

    for (i=0; i<self->length; i++) {
        if (i != pick)
            self->list[x++] = self->list[i];
        else
            value = self->list[i];
    }

    self->length = x;
    self->lastvalue = -1;

    return value;
}

static void
Urn_generate_i(Urn *self) {
    int i;
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT inc = fr / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)Urn_choose(self);
            if (self->length == 0) {
                self->trigsBuffer[i] = 1.0;
                Urn_reset(self);
            }
        }
        self->data[i] = self->value;
    }
}

static void
Urn_generate_a(Urn *self) {
    int i;
    MYFLT inc;
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->value = (MYFLT)Urn_choose(self);
            if (self->length == 0) {
                self->trigsBuffer[i] = 1.0;
                Urn_reset(self);
            }
        }
        self->data[i] = self->value;
    }
}

static void Urn_postprocessing_ii(Urn *self) { POST_PROCESSING_II };
static void Urn_postprocessing_ai(Urn *self) { POST_PROCESSING_AI };
static void Urn_postprocessing_ia(Urn *self) { POST_PROCESSING_IA };
static void Urn_postprocessing_aa(Urn *self) { POST_PROCESSING_AA };
static void Urn_postprocessing_ireva(Urn *self) { POST_PROCESSING_IREVA };
static void Urn_postprocessing_areva(Urn *self) { POST_PROCESSING_AREVA };
static void Urn_postprocessing_revai(Urn *self) { POST_PROCESSING_REVAI };
static void Urn_postprocessing_revaa(Urn *self) { POST_PROCESSING_REVAA };
static void Urn_postprocessing_revareva(Urn *self) { POST_PROCESSING_REVAREVA };

static void
Urn_setProcMode(Urn *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Urn_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Urn_generate_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Urn_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Urn_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Urn_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Urn_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Urn_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Urn_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Urn_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Urn_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Urn_postprocessing_revareva;
            break;
    }
}

static void
Urn_compute_next_data_frame(Urn *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Urn_traverse(Urn *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
Urn_clear(Urn *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
Urn_dealloc(Urn* self)
{
    pyo_DEALLOC
    free(self->list);
    free(self->trigsBuffer);
    Urn_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Urn_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Urn *self;
    self = (Urn *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1.);
    self->max = 100;
    self->length = 0;
    self->lastvalue = -1;
    self->value = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Urn_compute_next_data_frame);
    self->mode_func_ptr = Urn_setProcMode;

    static char *kwlist[] = {"max", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOOO", kwlist, &self->max, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    Urn_reset(self);

    Server_generateSeed((Server *)self->server, URN_ID);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Urn_getServer(Urn* self) { GET_SERVER };
static PyObject * Urn_getStream(Urn* self) { GET_STREAM };
static PyObject * Urn_getTriggerStream(Urn* self) { GET_TRIGGER_STREAM };
static PyObject * Urn_setMul(Urn *self, PyObject *arg) { SET_MUL };
static PyObject * Urn_setAdd(Urn *self, PyObject *arg) { SET_ADD };
static PyObject * Urn_setSub(Urn *self, PyObject *arg) { SET_SUB };
static PyObject * Urn_setDiv(Urn *self, PyObject *arg) { SET_DIV };

static PyObject * Urn_play(Urn *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Urn_out(Urn *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Urn_stop(Urn *self) { STOP };

static PyObject * Urn_multiply(Urn *self, PyObject *arg) { MULTIPLY };
static PyObject * Urn_inplace_multiply(Urn *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Urn_add(Urn *self, PyObject *arg) { ADD };
static PyObject * Urn_inplace_add(Urn *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Urn_sub(Urn *self, PyObject *arg) { SUB };
static PyObject * Urn_inplace_sub(Urn *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Urn_div(Urn *self, PyObject *arg) { DIV };
static PyObject * Urn_inplace_div(Urn *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Urn_setMax(Urn *self, PyObject *arg)
{
	if (PyNumber_Check(arg) == 1)
		self->max = PyInt_AsLong(arg);

    Urn_reset(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Urn_setFreq(Urn *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

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

static PyMemberDef Urn_members[] = {
{"server", T_OBJECT_EX, offsetof(Urn, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Urn, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(Urn, trig_stream), 0, "Trigger Stream object."},
{"freq", T_OBJECT_EX, offsetof(Urn, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(Urn, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Urn, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Urn_methods[] = {
{"getServer", (PyCFunction)Urn_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Urn_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)Urn_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
{"play", (PyCFunction)Urn_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Urn_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Urn_stop, METH_NOARGS, "Stops computing."},
{"setMax", (PyCFunction)Urn_setMax, METH_O, "Sets maximum possible value."},
{"setFreq", (PyCFunction)Urn_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)Urn_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Urn_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Urn_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Urn_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Urn_as_number = {
(binaryfunc)Urn_add,                         /*nb_add*/
(binaryfunc)Urn_sub,                         /*nb_subtract*/
(binaryfunc)Urn_multiply,                    /*nb_multiply*/
(binaryfunc)Urn_div,                                              /*nb_divide*/
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
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Urn_inplace_add,                 /*inplace_add*/
(binaryfunc)Urn_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Urn_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Urn_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject UrnType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Urn_base",                                   /*tp_name*/
sizeof(Urn),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Urn_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Urn_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Urn objects. Periodically generates a new integer random value without repetition.",           /* tp_doc */
(traverseproc)Urn_traverse,                  /* tp_traverse */
(inquiry)Urn_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Urn_methods,                                 /* tp_methods */
Urn_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Urn_new,                                     /* tp_new */
};