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

/* Compressor */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    PyObject *thresh;
    PyObject *ratio;
    Stream *risetime_stream;
    Stream *falltime_stream;
    Stream *thresh_stream;
    Stream *ratio_stream;
    int modebuffer[6]; // need at least 2 slots for mul & add
    int outputAmp;
    MYFLT follow;
    MYFLT knee;
    long lh_delay;
    long lh_size;
    long lh_in_count;
    MYFLT *lh_buffer;
} Compress;

static MYFLT
C_clip(MYFLT x)
{
    if (x < 1.0e-20)
        return 1.0e-20;
    else if (x > 1.0)
        return 1.0;
    else
        return x;
}

static void
Compress_compress_soft(Compress *self) {
    MYFLT samp, ampthresh, absin, indb, diff, outdb, outa;
    MYFLT kneethresh, kneescl, knee, kneeratio, invKneeRange;
    MYFLT risetime, falltime, thresh, ratio;
    int i;
    long ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        risetime = PyFloat_AS_DOUBLE(self->risetime);
    else
        risetime = Stream_getData((Stream *)self->risetime_stream)[0];
    if (risetime <= 0.0)
        risetime = 0.001;
    if (self->modebuffer[3] == 0)
        falltime = PyFloat_AS_DOUBLE(self->falltime);
    else
        falltime = Stream_getData((Stream *)self->falltime_stream)[0];
    if (falltime <= 0.0)
        falltime = 0.001;
    if (self->modebuffer[4] == 0)
        thresh = PyFloat_AS_DOUBLE(self->thresh);
    else
        thresh = Stream_getData((Stream *)self->thresh_stream)[0];
    if (self->modebuffer[5] == 0)
        ratio = PyFloat_AS_DOUBLE(self->ratio);
    else
        ratio = Stream_getData((Stream *)self->ratio_stream)[0];

    ratio = 1.0 / ratio;
    risetime = MYEXP(-1.0 / (self->sr * risetime));
    falltime = MYEXP(-1.0 / (self->sr * falltime));
    knee = self->knee * 0.999 + 0.001; /* 0 = hard knee, 1 = soft knee */
    thresh += 3.0 * self->knee;
    if (thresh > 0.0)
        thresh = 0.0;
    ampthresh = MYPOW(10.0, thresh * 0.05); /* up to 3 dB above threshold */
    kneethresh = MYPOW(10.0, (thresh - (self->knee * 8.5 + 0.5)) * 0.05); /* up to 6 dB under threshold */
    invKneeRange = 1.0 / (ampthresh - kneethresh);

    for (i=0; i<self->bufsize; i++) {
        /* Envelope follower */
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = absin + risetime * (self->follow - absin);
        else
            self->follow = absin + falltime * (self->follow - absin);

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Compress signal */
        outa = 1.0;
        if (self->follow > ampthresh) { /* Above threshold */
            indb = 20.0 * MYLOG10(self->follow + 1.0e-20);
            diff = indb - thresh;
            outdb = diff - diff * ratio;
            outa = MYPOW(10.0, -outdb * 0.05);
        }
        else if (self->follow > kneethresh) { /* Under the knee */
            kneescl = (self->follow - kneethresh) * invKneeRange;
            kneeratio = (((knee + 1.0) * kneescl) / (knee + kneescl)) * (ratio - 1.0) + 1.0;
            indb = 20.0 * MYLOG10(self->follow + 1.0e-20);
            diff = indb - thresh;
            outdb = diff - diff * kneeratio;
            outa = MYPOW(10.0, -outdb * 0.05);
        }
        if (self->outputAmp == 0)
            self->data[i] = samp * C_clip(outa);
        else
            self->data[i] = C_clip(outa);
    }
}

static void Compress_postprocessing_ii(Compress *self) { POST_PROCESSING_II };
static void Compress_postprocessing_ai(Compress *self) { POST_PROCESSING_AI };
static void Compress_postprocessing_ia(Compress *self) { POST_PROCESSING_IA };
static void Compress_postprocessing_aa(Compress *self) { POST_PROCESSING_AA };
static void Compress_postprocessing_ireva(Compress *self) { POST_PROCESSING_IREVA };
static void Compress_postprocessing_areva(Compress *self) { POST_PROCESSING_AREVA };
static void Compress_postprocessing_revai(Compress *self) { POST_PROCESSING_REVAI };
static void Compress_postprocessing_revaa(Compress *self) { POST_PROCESSING_REVAA };
static void Compress_postprocessing_revareva(Compress *self) { POST_PROCESSING_REVAREVA };

static void
Compress_setProcMode(Compress *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Compress_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Compress_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Compress_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Compress_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Compress_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Compress_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Compress_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Compress_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Compress_postprocessing_revareva;
            break;
    }
}

static void
Compress_compute_next_data_frame(Compress *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Compress_traverse(Compress *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);
    Py_VISIT(self->risetime_stream);
    Py_VISIT(self->falltime);
    Py_VISIT(self->falltime_stream);
    Py_VISIT(self->thresh);
    Py_VISIT(self->thresh_stream);
    Py_VISIT(self->ratio);
    Py_VISIT(self->ratio_stream);
    return 0;
}

static int
Compress_clear(Compress *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);
    Py_CLEAR(self->risetime_stream);
    Py_CLEAR(self->falltime);
    Py_CLEAR(self->falltime_stream);
    Py_CLEAR(self->thresh);
    Py_CLEAR(self->thresh_stream);
    Py_CLEAR(self->ratio);
    Py_CLEAR(self->ratio_stream);
    return 0;
}

static void
Compress_dealloc(Compress* self)
{
    pyo_DEALLOC
    free(self->lh_buffer);
    Compress_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Compress_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *threshtmp=NULL, *ratiotmp=NULL, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    PyObject *looktmp=NULL, *kneetmp=NULL;
    Compress *self;
    self = (Compress *)type->tp_alloc(type, 0);

    self->thresh = PyFloat_FromDouble(-20.0);
    self->ratio = PyFloat_FromDouble(2.0);
    self->risetime = PyFloat_FromDouble(0.01);
    self->falltime = PyFloat_FromDouble(0.1);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;
    self->outputAmp = 0;
    self->follow = 0.0;
    self->lh_delay = 0;
    self->lh_in_count = 0;
    self->knee = 0.;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, Compress_compute_next_data_frame);
    self->mode_func_ptr = Compress_setProcMode;

    static char *kwlist[] = {"input", "thresh", "ratio", "risetime", "falltime", "lookahead", "knee", "outputAmp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOOiOO", kwlist, &inputtmp, &threshtmp, &ratiotmp, &risetimetmp, &falltimetmp, &looktmp, &kneetmp, &self->outputAmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (threshtmp) {
        PyObject_CallMethod((PyObject *)self, "setThresh", "O", threshtmp);
    }

    if (ratiotmp) {
        PyObject_CallMethod((PyObject *)self, "setRatio", "O", ratiotmp);
    }

    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod((PyObject *)self, "setLookAhead", "O", looktmp);
    PyObject_CallMethod((PyObject *)self, "setKnee", "O", kneetmp);

    self->lh_size = (long)(0.025 * self->sr + 0.5);
    self->lh_buffer = (MYFLT *)realloc(self->lh_buffer, (self->lh_size+1) * sizeof(MYFLT));
    for (i=0; i<(self->lh_size+1); i++) {
        self->lh_buffer[i] = 0.;
    }

    self->proc_func_ptr = Compress_compress_soft;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Compress_getServer(Compress* self) { GET_SERVER };
static PyObject * Compress_getStream(Compress* self) { GET_STREAM };
static PyObject * Compress_setMul(Compress *self, PyObject *arg) { SET_MUL };
static PyObject * Compress_setAdd(Compress *self, PyObject *arg) { SET_ADD };
static PyObject * Compress_setSub(Compress *self, PyObject *arg) { SET_SUB };
static PyObject * Compress_setDiv(Compress *self, PyObject *arg) { SET_DIV };

static PyObject * Compress_play(Compress *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Compress_out(Compress *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Compress_stop(Compress *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Compress_multiply(Compress *self, PyObject *arg) { MULTIPLY };
static PyObject * Compress_inplace_multiply(Compress *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Compress_add(Compress *self, PyObject *arg) { ADD };
static PyObject * Compress_inplace_add(Compress *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Compress_sub(Compress *self, PyObject *arg) { SUB };
static PyObject * Compress_inplace_sub(Compress *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Compress_div(Compress *self, PyObject *arg) { DIV };
static PyObject * Compress_inplace_div(Compress *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Compress_setThresh(Compress *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->thresh);
	if (isNumber == 1) {
		self->thresh = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->thresh = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->thresh, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->thresh_stream);
        self->thresh_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compress_setRatio(Compress *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ratio);
	if (isNumber == 1) {
		self->ratio = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->ratio = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ratio, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ratio_stream);
        self->ratio_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compress_setRiseTime(Compress *self, PyObject *arg)
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

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compress_setFallTime(Compress *self, PyObject *arg)
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

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compress_setLookAhead(Compress *self, PyObject *arg)
{
    MYFLT tmp;

    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg)) {
		tmp = PyFloat_AsDouble(arg);
        if (tmp <= 25.0)
            self->lh_delay = (long)(tmp * 0.001 * self->sr);
        else
            PySys_WriteStdout("Compress: lookahead argument must be less than 25.0 ms.\n");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compress_setKnee(Compress *self, PyObject *arg)
{
    MYFLT tmp;

    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg)) {
		tmp = PyFloat_AsDouble(arg);
        if (tmp >= 0.0 && tmp <= 1.0)
            self->knee = tmp;
        else
            PySys_WriteStdout("Compress: knee argument must be in range 0 (hard) -> 1 (soft).\n");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Compress_members[] = {
{"server", T_OBJECT_EX, offsetof(Compress, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Compress, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Compress, input), 0, "Input sound object."},
{"thresh", T_OBJECT_EX, offsetof(Compress, thresh), 0, "Compressor threshold."},
{"ratio", T_OBJECT_EX, offsetof(Compress, ratio), 0, "Compressor ratio."},
{"risetime", T_OBJECT_EX, offsetof(Compress, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Compress, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Compress, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Compress, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Compress_methods[] = {
{"getServer", (PyCFunction)Compress_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Compress_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Compress_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Compress_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Compress_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setThresh", (PyCFunction)Compress_setThresh, METH_O, "Sets compressor threshold."},
{"setRatio", (PyCFunction)Compress_setRatio, METH_O, "Sets compressor ratio."},
{"setRiseTime", (PyCFunction)Compress_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Compress_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setLookAhead", (PyCFunction)Compress_setLookAhead, METH_O, "Sets look ahead time in ms."},
{"setKnee", (PyCFunction)Compress_setKnee, METH_O, "Sets the knee between 0 (hard) and 1 (soft)."},
{"setMul", (PyCFunction)Compress_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Compress_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Compress_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Compress_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Compress_as_number = {
(binaryfunc)Compress_add,                         /*nb_add*/
(binaryfunc)Compress_sub,                         /*nb_subtract*/
(binaryfunc)Compress_multiply,                    /*nb_multiply*/
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
(binaryfunc)Compress_inplace_add,                 /*inplace_add*/
(binaryfunc)Compress_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Compress_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Compress_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Compress_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject CompressType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Compress_base",                                   /*tp_name*/
sizeof(Compress),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Compress_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Compress_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Compress objects. Compress audio signal by a certain ratio above a threshold.",           /* tp_doc */
(traverseproc)Compress_traverse,                  /* tp_traverse */
(inquiry)Compress_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Compress_methods,                                 /* tp_methods */
Compress_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Compress_new,                                     /* tp_new */
};

const MYFLT GATE_MIN_RAMP_TIME = 0.0001;
/************/
/* Gate */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *thresh;
    Stream *thresh_stream;
    PyObject *risetime;
    Stream *risetime_stream;
    PyObject *falltime;
    Stream *falltime_stream;
    int modebuffer[5]; // need at least 2 slots for mul & add
    int outputAmp;
    MYFLT lpfollow;
    MYFLT lpfactor;
    MYFLT gate;
    MYFLT last_risetime;
    MYFLT last_falltime;
    MYFLT risefactor;
    MYFLT fallfactor;
    long lh_delay;
    long lh_size;
    long lh_in_count;
    MYFLT *lh_buffer;
} Gate;

static void
Gate_filters_iii(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    thresh = PyFloat_AS_DOUBLE(self->thresh);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = GATE_MIN_RAMP_TIME;
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = GATE_MIN_RAMP_TIME;

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }

    ampthresh = MYPOW(10.0, thresh * 0.05);
    for (i=0; i<self->bufsize; i++) {
        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_aii(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *tr = Stream_getData((Stream *)self->thresh_stream);

    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = GATE_MIN_RAMP_TIME;
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = GATE_MIN_RAMP_TIME;

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }

    for (i=0; i<self->bufsize; i++) {
        thresh = tr[i];
        ampthresh = MYPOW(10.0, thresh * 0.05);
        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_iai(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    thresh = PyFloat_AS_DOUBLE(self->thresh);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);

    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = GATE_MIN_RAMP_TIME;

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }

    ampthresh = MYPOW(10.0, thresh * 0.05);
    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = GATE_MIN_RAMP_TIME;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_aai(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *tr = Stream_getData((Stream *)self->thresh_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);

    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = GATE_MIN_RAMP_TIME;

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }

    for (i=0; i<self->bufsize; i++) {
        thresh = tr[i];
        ampthresh = MYPOW(10.0, thresh * 0.05);
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = GATE_MIN_RAMP_TIME;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_iia(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    thresh = PyFloat_AS_DOUBLE(self->thresh);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = GATE_MIN_RAMP_TIME;
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    ampthresh = MYPOW(10.0, thresh * 0.05);
    for (i=0; i<self->bufsize; i++) {
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = GATE_MIN_RAMP_TIME;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_aia(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *tr = Stream_getData((Stream *)self->thresh_stream);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = GATE_MIN_RAMP_TIME;
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    for (i=0; i<self->bufsize; i++) {
        thresh = tr[i];
        ampthresh = MYPOW(10.0, thresh * 0.05);
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = GATE_MIN_RAMP_TIME;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_iaa(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    thresh = PyFloat_AS_DOUBLE(self->thresh);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    ampthresh = MYPOW(10.0, thresh * 0.05);
    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = GATE_MIN_RAMP_TIME;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = GATE_MIN_RAMP_TIME;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void
Gate_filters_aaa(Gate *self) {
    MYFLT samp, absin, thresh, ampthresh, risetime, falltime;
    int i, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *tr = Stream_getData((Stream *)self->thresh_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    for (i=0; i<self->bufsize; i++) {
        thresh = tr[i];
        ampthresh = MYPOW(10.0, thresh * 0.05);
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = 0.001;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = 0.001;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }

        /* Follower */
        absin = in[i] * in[i];
        self->lpfollow = absin + self->lpfactor * (self->lpfollow - absin);

        /* Gate slope */
        if (self->lpfollow >= ampthresh)
            self->gate = 1.0 + self->risefactor * (self->gate - 1.0);
        else
            self->gate = self->fallfactor * self->gate;

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Gate the signal */
        if (self->outputAmp == 0)
            self->data[i] = samp * self->gate;
        else
            self->data[i] = self->gate;
    }
}

static void Gate_postprocessing_ii(Gate *self) { POST_PROCESSING_II };
static void Gate_postprocessing_ai(Gate *self) { POST_PROCESSING_AI };
static void Gate_postprocessing_ia(Gate *self) { POST_PROCESSING_IA };
static void Gate_postprocessing_aa(Gate *self) { POST_PROCESSING_AA };
static void Gate_postprocessing_ireva(Gate *self) { POST_PROCESSING_IREVA };
static void Gate_postprocessing_areva(Gate *self) { POST_PROCESSING_AREVA };
static void Gate_postprocessing_revai(Gate *self) { POST_PROCESSING_REVAI };
static void Gate_postprocessing_revaa(Gate *self) { POST_PROCESSING_REVAA };
static void Gate_postprocessing_revareva(Gate *self) { POST_PROCESSING_REVAREVA };

static void
Gate_setProcMode(Gate *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Gate_filters_iii;
            break;
        case 1:
            self->proc_func_ptr = Gate_filters_aii;
            break;
        case 10:
            self->proc_func_ptr = Gate_filters_iai;
            break;
        case 11:
            self->proc_func_ptr = Gate_filters_aai;
            break;
        case 100:
            self->proc_func_ptr = Gate_filters_iia;
            break;
        case 101:
            self->proc_func_ptr = Gate_filters_aia;
            break;
        case 110:
            self->proc_func_ptr = Gate_filters_iaa;
            break;
        case 111:
            self->proc_func_ptr = Gate_filters_aaa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Gate_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Gate_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Gate_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Gate_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Gate_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Gate_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Gate_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Gate_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Gate_postprocessing_revareva;
            break;
    }
}

static void
Gate_compute_next_data_frame(Gate *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Gate_traverse(Gate *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->thresh);
    Py_VISIT(self->thresh_stream);
    Py_VISIT(self->risetime);
    Py_VISIT(self->risetime_stream);
    Py_VISIT(self->falltime);
    Py_VISIT(self->falltime_stream);
    return 0;
}

static int
Gate_clear(Gate *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->thresh);
    Py_CLEAR(self->thresh_stream);
    Py_CLEAR(self->risetime);
    Py_CLEAR(self->risetime_stream);
    Py_CLEAR(self->falltime);
    Py_CLEAR(self->falltime_stream);
    return 0;
}

static void
Gate_dealloc(Gate* self)
{
    pyo_DEALLOC
    free(self->lh_buffer);
    Gate_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Gate_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *threshtmp=NULL, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    PyObject *looktmp=NULL;
    Gate *self;
    self = (Gate *)type->tp_alloc(type, 0);

    self->thresh = PyFloat_FromDouble(-70.0);
    self->risetime = PyFloat_FromDouble(0.01);
    self->falltime = PyFloat_FromDouble(0.05);
    self->gate = self->lpfollow = 0.0;
    self->last_risetime = -1.0;
    self->last_falltime = -1.0;
    self->risefactor = self->fallfactor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->lh_delay = 0;
    self->lh_in_count = 0;
    self->outputAmp = 0;

    INIT_OBJECT_COMMON

    self->lpfactor = MYEXP(-1.0 / (self->sr / 20.0));

    Stream_setFunctionPtr(self->stream, Gate_compute_next_data_frame);
    self->mode_func_ptr = Gate_setProcMode;

    static char *kwlist[] = {"input", "thresh", "risetime", "falltime", "lookahead", "outputAmp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOiOO", kwlist, &inputtmp, &threshtmp, &risetimetmp, &falltimetmp, &looktmp, &self->outputAmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (threshtmp) {
        PyObject_CallMethod((PyObject *)self, "setThresh", "O", threshtmp);
    }

    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod((PyObject *)self, "setLookAhead", "O", looktmp);

    self->lh_size = (long)(0.025 * self->sr + 0.5);
    self->lh_buffer = (MYFLT *)realloc(self->lh_buffer, (self->lh_size+1) * sizeof(MYFLT));
    for (i=0; i<(self->lh_size+1); i++) {
        self->lh_buffer[i] = 0.;
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Gate_getServer(Gate* self) { GET_SERVER };
static PyObject * Gate_getStream(Gate* self) { GET_STREAM };
static PyObject * Gate_setMul(Gate *self, PyObject *arg) { SET_MUL };
static PyObject * Gate_setAdd(Gate *self, PyObject *arg) { SET_ADD };
static PyObject * Gate_setSub(Gate *self, PyObject *arg) { SET_SUB };
static PyObject * Gate_setDiv(Gate *self, PyObject *arg) { SET_DIV };

static PyObject * Gate_play(Gate *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Gate_out(Gate *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Gate_stop(Gate *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Gate_multiply(Gate *self, PyObject *arg) { MULTIPLY };
static PyObject * Gate_inplace_multiply(Gate *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Gate_add(Gate *self, PyObject *arg) { ADD };
static PyObject * Gate_inplace_add(Gate *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Gate_sub(Gate *self, PyObject *arg) { SUB };
static PyObject * Gate_inplace_sub(Gate *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Gate_div(Gate *self, PyObject *arg) { DIV };
static PyObject * Gate_inplace_div(Gate *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Gate_setThresh(Gate *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->thresh);
	if (isNumber == 1) {
		self->thresh = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->thresh = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->thresh, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->thresh_stream);
        self->thresh_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Gate_setRiseTime(Gate *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->risetime);
	if (isNumber == 1) {
		self->risetime = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->risetime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->risetime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->risetime_stream);
        self->risetime_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Gate_setFallTime(Gate *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->falltime);
	if (isNumber == 1) {
		self->falltime = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->falltime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->falltime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->falltime_stream);
        self->falltime_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Gate_setLookAhead(Gate *self, PyObject *arg)
{
    MYFLT tmp;

    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg)) {
		tmp = PyFloat_AsDouble(arg);
        if (tmp <= 25.0)
            self->lh_delay = (long)(tmp * 0.001 * self->sr);
        else
            PySys_WriteStdout("Gate: lookahead argument must be less than 25.0 ms.\n");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Gate_members[] = {
    {"server", T_OBJECT_EX, offsetof(Gate, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Gate, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Gate, input), 0, "Input sound object."},
    {"thresh", T_OBJECT_EX, offsetof(Gate, thresh), 0, "Threshold in dB."},
    {"risetime", T_OBJECT_EX, offsetof(Gate, risetime), 0, "Risetime in second."},
    {"falltime", T_OBJECT_EX, offsetof(Gate, falltime), 0, "Falltime in second."},
    {"mul", T_OBJECT_EX, offsetof(Gate, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Gate, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Gate_methods[] = {
    {"getServer", (PyCFunction)Gate_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Gate_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Gate_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Gate_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Gate_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setThresh", (PyCFunction)Gate_setThresh, METH_O, "Sets the threshold in dB."},
    {"setRiseTime", (PyCFunction)Gate_setRiseTime, METH_O, "Sets filter risetime in second."},
    {"setFallTime", (PyCFunction)Gate_setFallTime, METH_O, "Sets filter falltime in second."},
    {"setLookAhead", (PyCFunction)Gate_setLookAhead, METH_O, "Sets look ahead time in ms."},
    {"setMul", (PyCFunction)Gate_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Gate_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Gate_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Gate_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Gate_as_number = {
    (binaryfunc)Gate_add,                         /*nb_add*/
    (binaryfunc)Gate_sub,                         /*nb_subtract*/
    (binaryfunc)Gate_multiply,                    /*nb_multiply*/
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
    (binaryfunc)Gate_inplace_add,                 /*inplace_add*/
    (binaryfunc)Gate_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Gate_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Gate_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Gate_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject GateType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Gate_base",                                   /*tp_name*/
    sizeof(Gate),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Gate_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Gate_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Gate objects. Noise gate dynamic processor.",           /* tp_doc */
    (traverseproc)Gate_traverse,                  /* tp_traverse */
    (inquiry)Gate_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Gate_methods,                                 /* tp_methods */
    Gate_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Gate_new,                                     /* tp_new */
};

/************/
/* Balance */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
    MYFLT follow1;
    MYFLT follow2;
    MYFLT last_freq;
    MYFLT factor;
} Balance;

static void
Balance_filters_i(Balance *self) {
    MYFLT absin, freq;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);

    if (freq < 0.1)
        freq = 0.1;

    if (freq != self->last_freq) {
        self->factor = MYEXP(-1.0 / (self->sr / freq));
        self->last_freq = freq;
    }

    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow1 = absin + self->factor * (self->follow1 - absin);
        if (self->follow1 < 0.001)
            self->follow1 = 0.001;
        absin = in2[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow2 = absin + self->factor * (self->follow2 - absin);
        self->data[i] = in[i] * (self->follow2 / self->follow1);
    }
}

static void
Balance_filters_a(Balance *self) {
    MYFLT absin, freq;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq < 0.1)
            freq = 0.1;

        if (freq != self->last_freq) {
            self->factor = MYEXP(-1.0 / (self->sr / freq));
            self->last_freq = freq;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow1 = absin + self->factor * (self->follow1 - absin);
        if (self->follow1 < 0.001)
            self->follow1 = 0.001;
        absin = in2[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow2 = absin + self->factor * (self->follow2 - absin);
        self->data[i] = in[i] * (self->follow2 / self->follow1);
    }
}

static void Balance_postprocessing_ii(Balance *self) { POST_PROCESSING_II };
static void Balance_postprocessing_ai(Balance *self) { POST_PROCESSING_AI };
static void Balance_postprocessing_ia(Balance *self) { POST_PROCESSING_IA };
static void Balance_postprocessing_aa(Balance *self) { POST_PROCESSING_AA };
static void Balance_postprocessing_ireva(Balance *self) { POST_PROCESSING_IREVA };
static void Balance_postprocessing_areva(Balance *self) { POST_PROCESSING_AREVA };
static void Balance_postprocessing_revai(Balance *self) { POST_PROCESSING_REVAI };
static void Balance_postprocessing_revaa(Balance *self) { POST_PROCESSING_REVAA };
static void Balance_postprocessing_revareva(Balance *self) { POST_PROCESSING_REVAREVA };

static void
Balance_setProcMode(Balance *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = Balance_filters_i;
            break;
        case 1:
            self->proc_func_ptr = Balance_filters_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Balance_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Balance_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Balance_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Balance_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Balance_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Balance_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Balance_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Balance_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Balance_postprocessing_revareva;
            break;
    }
}

static void
Balance_compute_next_data_frame(Balance *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Balance_traverse(Balance *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    return 0;
}

static int
Balance_clear(Balance *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    return 0;
}

static void
Balance_dealloc(Balance* self)
{
    pyo_DEALLOC
    Balance_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Balance_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Balance *self;
    self = (Balance *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(10);
    self->follow1 = self->follow2 = 0.0;
    self->last_freq = -1.0;
    self->factor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Balance_compute_next_data_frame);
    self->mode_func_ptr = Balance_setProcMode;

    static char *kwlist[] = {"input", "input2", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &input2tmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;

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

static PyObject * Balance_getServer(Balance* self) { GET_SERVER };
static PyObject * Balance_getStream(Balance* self) { GET_STREAM };
static PyObject * Balance_setMul(Balance *self, PyObject *arg) { SET_MUL };
static PyObject * Balance_setAdd(Balance *self, PyObject *arg) { SET_ADD };
static PyObject * Balance_setSub(Balance *self, PyObject *arg) { SET_SUB };
static PyObject * Balance_setDiv(Balance *self, PyObject *arg) { SET_DIV };

static PyObject * Balance_play(Balance *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Balance_out(Balance *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Balance_stop(Balance *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Balance_multiply(Balance *self, PyObject *arg) { MULTIPLY };
static PyObject * Balance_inplace_multiply(Balance *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Balance_add(Balance *self, PyObject *arg) { ADD };
static PyObject * Balance_inplace_add(Balance *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Balance_sub(Balance *self, PyObject *arg) { SUB };
static PyObject * Balance_inplace_sub(Balance *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Balance_div(Balance *self, PyObject *arg) { DIV };
static PyObject * Balance_inplace_div(Balance *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Balance_setFreq(Balance *self, PyObject *arg)
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

static PyMemberDef Balance_members[] = {
    {"server", T_OBJECT_EX, offsetof(Balance, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Balance, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Balance, input), 0, "Input sound object."},
    {"input2", T_OBJECT_EX, offsetof(Balance, input2), 0, "Comparator input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Balance, freq), 0, "Cutoff frequency in cycle per second."},
    {"mul", T_OBJECT_EX, offsetof(Balance, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Balance, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Balance_methods[] = {
    {"getServer", (PyCFunction)Balance_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Balance_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Balance_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Balance_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Balance_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setFreq", (PyCFunction)Balance_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setMul", (PyCFunction)Balance_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Balance_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Balance_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Balance_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Balance_as_number = {
    (binaryfunc)Balance_add,                         /*nb_add*/
    (binaryfunc)Balance_sub,                         /*nb_subtract*/
    (binaryfunc)Balance_multiply,                    /*nb_multiply*/
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
    (binaryfunc)Balance_inplace_add,                 /*inplace_add*/
    (binaryfunc)Balance_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Balance_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Balance_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Balance_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject BalanceType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Balance_base",                                   /*tp_name*/
    sizeof(Balance),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Balance_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Balance_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Balance objects. The rms power of a signal is adjusted to match that of a comparator signal.",           /* tp_doc */
    (traverseproc)Balance_traverse,                  /* tp_traverse */
    (inquiry)Balance_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Balance_methods,                                 /* tp_methods */
    Balance_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Balance_new,                                     /* tp_new */
};

/* Expander */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    PyObject *upthresh;
    PyObject *downthresh;
    PyObject *ratio;
    Stream *risetime_stream;
    Stream *falltime_stream;
    Stream *upthresh_stream;
    Stream *downthresh_stream;
    Stream *ratio_stream;
    int modebuffer[7]; // need at least 2 slots for mul & add
    int outputAmp;
    MYFLT follow;
    long lh_delay;
    long lh_size;
    long lh_in_count;
    MYFLT *lh_buffer;
} Expand;

static void
Expand_compress_soft(Expand *self) {
    MYFLT samp, absin, indb, diff, outa;
    MYFLT risetime, falltime, upthresh, downthresh, ratio;
    int i;
    long ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        risetime = PyFloat_AS_DOUBLE(self->risetime);
    else
        risetime = Stream_getData((Stream *)self->risetime_stream)[0];
    if (risetime <= 0.0)
        risetime = 0.001;
    if (self->modebuffer[3] == 0)
        falltime = PyFloat_AS_DOUBLE(self->falltime);
    else
        falltime = Stream_getData((Stream *)self->falltime_stream)[0];
    if (falltime <= 0.0)
        falltime = 0.001;
    if (self->modebuffer[4] == 0)
        upthresh = PyFloat_AS_DOUBLE(self->upthresh);
    else
        upthresh = Stream_getData((Stream *)self->upthresh_stream)[0];
    if (upthresh > 0.0)
        upthresh = 0.0;
    if (self->modebuffer[5] == 0)
        downthresh = PyFloat_AS_DOUBLE(self->downthresh);
    else
        downthresh = Stream_getData((Stream *)self->downthresh_stream)[0];
    if (downthresh < -120.0)
        downthresh = -120.0;
    else if (downthresh > upthresh)
        downthresh = upthresh;
    if (self->modebuffer[6] == 0)
        ratio = PyFloat_AS_DOUBLE(self->ratio);
    else
        ratio = Stream_getData((Stream *)self->ratio_stream)[0];

    ratio = 1.0 / ratio;
    risetime = MYEXP(-1.0 / (self->sr * risetime));
    falltime = MYEXP(-1.0 / (self->sr * falltime));

    for (i=0; i<self->bufsize; i++) {
        /* Envelope follower */
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = absin + risetime * (self->follow - absin);
        else
            self->follow = absin + falltime * (self->follow - absin);

        /* Look ahead */
        ind = self->lh_in_count - self->lh_delay;
        if (ind < 0)
            ind += self->lh_size;
        samp = self->lh_buffer[ind];

        self->lh_buffer[self->lh_in_count] = in[i];
        self->lh_in_count++;
        if (self->lh_in_count >= self->lh_size)
            self->lh_in_count = 0;

        /* Expand signal */
        outa = 1.0;
        indb = 20.0 * MYLOG10(C_clip(self->follow));
        if (indb > upthresh) { /* Above upper threshold */
            diff = indb - upthresh;
            outa = MYPOW(10.0, (diff * ratio - diff) * 0.05);
        } else if (indb < downthresh) { /* Below lower threshold */
            diff = downthresh - indb;
            outa = MYPOW(10.0, (diff - diff * ratio) * 0.05);
        }
        outa = 1.0 / outa;

        if (self->outputAmp == 0)
            self->data[i] = samp * outa;
        else
            self->data[i] = outa;
    }
}

static void Expand_postprocessing_ii(Expand *self) { POST_PROCESSING_II };
static void Expand_postprocessing_ai(Expand *self) { POST_PROCESSING_AI };
static void Expand_postprocessing_ia(Expand *self) { POST_PROCESSING_IA };
static void Expand_postprocessing_aa(Expand *self) { POST_PROCESSING_AA };
static void Expand_postprocessing_ireva(Expand *self) { POST_PROCESSING_IREVA };
static void Expand_postprocessing_areva(Expand *self) { POST_PROCESSING_AREVA };
static void Expand_postprocessing_revai(Expand *self) { POST_PROCESSING_REVAI };
static void Expand_postprocessing_revaa(Expand *self) { POST_PROCESSING_REVAA };
static void Expand_postprocessing_revareva(Expand *self) { POST_PROCESSING_REVAREVA };

static void
Expand_setProcMode(Expand *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Expand_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Expand_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Expand_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Expand_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Expand_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Expand_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Expand_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Expand_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Expand_postprocessing_revareva;
            break;
    }
}

static void
Expand_compute_next_data_frame(Expand *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Expand_traverse(Expand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);
    Py_VISIT(self->risetime_stream);
    Py_VISIT(self->falltime);
    Py_VISIT(self->falltime_stream);
    Py_VISIT(self->upthresh);
    Py_VISIT(self->upthresh_stream);
    Py_VISIT(self->downthresh);
    Py_VISIT(self->downthresh_stream);
    Py_VISIT(self->ratio);
    Py_VISIT(self->ratio_stream);
    return 0;
}

static int
Expand_clear(Expand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);
    Py_CLEAR(self->risetime_stream);
    Py_CLEAR(self->falltime);
    Py_CLEAR(self->falltime_stream);
    Py_CLEAR(self->upthresh);
    Py_CLEAR(self->upthresh_stream);
    Py_CLEAR(self->downthresh);
    Py_CLEAR(self->downthresh_stream);
    Py_CLEAR(self->ratio);
    Py_CLEAR(self->ratio_stream);
    return 0;
}

static void
Expand_dealloc(Expand* self)
{
    pyo_DEALLOC
    free(self->lh_buffer);
    Expand_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Expand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *upthreshtmp=NULL, *downthreshtmp=NULL, *ratiotmp=NULL, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    PyObject *looktmp=NULL;
    Expand *self;
    self = (Expand *)type->tp_alloc(type, 0);

    self->downthresh = PyFloat_FromDouble(-60.0);
    self->upthresh = PyFloat_FromDouble(-20.0);
    self->ratio = PyFloat_FromDouble(2.0);
    self->risetime = PyFloat_FromDouble(0.01);
    self->falltime = PyFloat_FromDouble(0.1);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;
	self->modebuffer[6] = 0;
    self->outputAmp = 0;
    self->follow = 0.0;
    self->lh_delay = 0;
    self->lh_in_count = 0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, Expand_compute_next_data_frame);
    self->mode_func_ptr = Expand_setProcMode;

    static char *kwlist[] = {"input", "downthresh", "upthresh", "ratio", "risetime", "falltime", "lookahead", "outputAmp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOOiOO", kwlist, &inputtmp, &downthreshtmp, &upthreshtmp, &ratiotmp, &risetimetmp, &falltimetmp, &looktmp, &self->outputAmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (downthreshtmp) {
        PyObject_CallMethod((PyObject *)self, "setDownThresh", "O", downthreshtmp);
    }

    if (upthreshtmp) {
        PyObject_CallMethod((PyObject *)self, "setUpThresh", "O", upthreshtmp);
    }

    if (ratiotmp) {
        PyObject_CallMethod((PyObject *)self, "setRatio", "O", ratiotmp);
    }

    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod((PyObject *)self, "setLookAhead", "O", looktmp);

    self->lh_size = (long)(0.025 * self->sr + 0.5);
    self->lh_buffer = (MYFLT *)realloc(self->lh_buffer, (self->lh_size+1) * sizeof(MYFLT));
    for (i=0; i<(self->lh_size+1); i++) {
        self->lh_buffer[i] = 0.;
    }

    self->proc_func_ptr = Expand_compress_soft;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Expand_getServer(Expand* self) { GET_SERVER };
static PyObject * Expand_getStream(Expand* self) { GET_STREAM };
static PyObject * Expand_setMul(Expand *self, PyObject *arg) { SET_MUL };
static PyObject * Expand_setAdd(Expand *self, PyObject *arg) { SET_ADD };
static PyObject * Expand_setSub(Expand *self, PyObject *arg) { SET_SUB };
static PyObject * Expand_setDiv(Expand *self, PyObject *arg) { SET_DIV };

static PyObject * Expand_play(Expand *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Expand_out(Expand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Expand_stop(Expand *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Expand_multiply(Expand *self, PyObject *arg) { MULTIPLY };
static PyObject * Expand_inplace_multiply(Expand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Expand_add(Expand *self, PyObject *arg) { ADD };
static PyObject * Expand_inplace_add(Expand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Expand_sub(Expand *self, PyObject *arg) { SUB };
static PyObject * Expand_inplace_sub(Expand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Expand_div(Expand *self, PyObject *arg) { DIV };
static PyObject * Expand_inplace_div(Expand *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Expand_setDownThresh(Expand *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->downthresh);
	if (isNumber == 1) {
		self->downthresh = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->downthresh = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->downthresh, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->downthresh_stream);
        self->downthresh_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Expand_setUpThresh(Expand *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->upthresh);
	if (isNumber == 1) {
		self->upthresh = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->upthresh = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->upthresh, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->upthresh_stream);
        self->upthresh_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Expand_setRatio(Expand *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ratio);
	if (isNumber == 1) {
		self->ratio = PyNumber_Float(tmp);
        self->modebuffer[6] = 0;
	}
	else {
		self->ratio = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ratio, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ratio_stream);
        self->ratio_stream = (Stream *)streamtmp;
		self->modebuffer[6] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Expand_setRiseTime(Expand *self, PyObject *arg)
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

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Expand_setFallTime(Expand *self, PyObject *arg)
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

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Expand_setLookAhead(Expand *self, PyObject *arg)
{
    MYFLT tmp;

    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg)) {
		tmp = PyFloat_AsDouble(arg);
        if (tmp <= 25.0)
            self->lh_delay = (long)(tmp * 0.001 * self->sr);
        else
            PySys_WriteStdout("Expand: lookahead argument must be less than 25.0 ms.\n");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Expand_members[] = {
{"server", T_OBJECT_EX, offsetof(Expand, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Expand, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Expand, input), 0, "Input sound object."},
{"downthresh", T_OBJECT_EX, offsetof(Expand, downthresh), 0, "Expander lower threshold."},
{"upthresh", T_OBJECT_EX, offsetof(Expand, upthresh), 0, "Expander upper threshold."},
{"ratio", T_OBJECT_EX, offsetof(Expand, ratio), 0, "Expandor ratio."},
{"risetime", T_OBJECT_EX, offsetof(Expand, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Expand, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Expand, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Expand, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Expand_methods[] = {
{"getServer", (PyCFunction)Expand_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Expand_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Expand_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Expand_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Expand_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setDownThresh", (PyCFunction)Expand_setDownThresh, METH_O, "Sets expander lower threshold."},
{"setUpThresh", (PyCFunction)Expand_setUpThresh, METH_O, "Sets expander upper threshold."},
{"setRatio", (PyCFunction)Expand_setRatio, METH_O, "Sets expander ratio."},
{"setRiseTime", (PyCFunction)Expand_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Expand_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setLookAhead", (PyCFunction)Expand_setLookAhead, METH_O, "Sets look ahead time in ms."},
{"setMul", (PyCFunction)Expand_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Expand_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Expand_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Expand_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Expand_as_number = {
(binaryfunc)Expand_add,                         /*nb_add*/
(binaryfunc)Expand_sub,                         /*nb_subtract*/
(binaryfunc)Expand_multiply,                    /*nb_multiply*/
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
(binaryfunc)Expand_inplace_add,                 /*inplace_add*/
(binaryfunc)Expand_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Expand_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Expand_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Expand_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ExpandType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Expand_base",                                   /*tp_name*/
sizeof(Expand),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Expand_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Expand_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Expand objects. Expand audio signal by a certain ratio when below or above certain thresholds.", /* tp_doc */
(traverseproc)Expand_traverse,                  /* tp_traverse */
(inquiry)Expand_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Expand_methods,                                 /* tp_methods */
Expand_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Expand_new,                                     /* tp_new */
};

