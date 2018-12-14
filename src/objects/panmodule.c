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
    PyObject *pan;
    Stream *pan_stream;
    PyObject *spread;
    Stream *spread_stream;
    int chnls;
    int modebuffer[2];
    MYFLT *buffer_streams;
} Panner;

static MYFLT
P_clip(MYFLT p) {
    if (p < 0.0)
        return 0.0;
    else if (p > 1.0)
        return 1.0;
    else
        return p;
}

static void
Panner_splitter_thru(Panner *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i] = in[i];
    }
}

static void
Panner_splitter_st_i(Panner *self) {
    int i;
    MYFLT inval, pi_over_two = PI / 2.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT pan = PyFloat_AS_DOUBLE(self->pan);
    pan = P_clip(pan) * pi_over_two;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        self->buffer_streams[i] = inval * MYCOS(pan);
        self->buffer_streams[i+self->bufsize] = inval * MYSIN(pan);
    }
}

static void
Panner_splitter_st_a(Panner *self) {
    int i;
    MYFLT inval, panval, pi_over_two = PI / 2.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *pan = Stream_getData((Stream *)self->pan_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        panval = P_clip(pan[i]) * pi_over_two;
        self->buffer_streams[i] = inval * MYCOS(panval);
        self->buffer_streams[i+self->bufsize] = inval * MYSIN(panval);
    }
}

static void
Panner_splitter_ii(Panner *self) {
    MYFLT val, inval, phase, sprd;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT pan = PyFloat_AS_DOUBLE(self->pan);
    MYFLT spd = PyFloat_AS_DOUBLE(self->spread);

    pan = P_clip(pan);
    spd = P_clip(spd);

    sprd = 20.0 - (MYSQRT(spd) * 20.0) + 0.1;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        for (j=0; j<self->chnls; j++) {
            phase = j / (MYFLT)self->chnls;
            val = inval * MYPOW(MYCOS((pan - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }
}

static void
Panner_splitter_ai(Panner *self) {
    MYFLT val, inval, phase, sprd;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *pan = Stream_getData((Stream *)self->pan_stream);
    MYFLT spd = PyFloat_AS_DOUBLE(self->spread);

    spd = P_clip(spd);

    sprd = 20.0 - (MYSQRT(spd) * 20.0) + 0.1;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        for (j=0; j<self->chnls; j++) {
            phase = j / (MYFLT)self->chnls;
            val = inval * MYPOW(MYCOS((P_clip(pan[i]) - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }
}

static void
Panner_splitter_ia(Panner *self) {
    MYFLT val, inval, phase, spdval, sprd;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT pan = PyFloat_AS_DOUBLE(self->pan);
    MYFLT *spd = Stream_getData((Stream *)self->spread_stream);

    pan = P_clip(pan);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        spdval = P_clip(spd[i]);
        sprd = 20.0 - (MYSQRT(spdval) * 20.0) + 0.1;
        for (j=0; j<self->chnls; j++) {
            phase = j / (MYFLT)self->chnls;
            val = inval * MYPOW(MYCOS((pan - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }
}

static void
Panner_splitter_aa(Panner *self) {
    MYFLT val, inval, phase, spdval, sprd;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *pan = Stream_getData((Stream *)self->pan_stream);
    MYFLT *spd = Stream_getData((Stream *)self->spread_stream);


    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        spdval = P_clip(spd[i]);
        sprd = 20.0 - (MYSQRT(spdval) * 20.0) + 0.1;
        for (j=0; j<self->chnls; j++) {
            phase = j / (MYFLT)self->chnls;
            val = inval * MYPOW(MYCOS((P_clip(pan[i]) - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }
}

MYFLT *
Panner_getSamplesBuffer(Panner *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Panner_setProcMode(Panner *self)
{
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    if (self->chnls > 2) {
        switch (procmode) {
            case 0:
                self->proc_func_ptr = Panner_splitter_ii;
                break;
            case 1:
                self->proc_func_ptr = Panner_splitter_ai;
                break;
            case 10:
                self->proc_func_ptr = Panner_splitter_ia;
                break;
            case 11:
                self->proc_func_ptr = Panner_splitter_aa;
                break;
        }
    }
    else if (self->chnls == 2) {
        switch (self->modebuffer[0]) {
            case 0:
                self->proc_func_ptr = Panner_splitter_st_i;
                break;
            case 1:
                self->proc_func_ptr = Panner_splitter_st_a;
                break;
        }
    }
    else if (self->chnls == 1) {
        self->proc_func_ptr = Panner_splitter_thru;
    }
}

static void
Panner_compute_next_data_frame(Panner *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Panner_traverse(Panner *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->pan);
    Py_VISIT(self->pan_stream);
    Py_VISIT(self->spread);
    Py_VISIT(self->spread_stream);
    return 0;
}

static int
Panner_clear(Panner *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->pan);
    Py_CLEAR(self->pan_stream);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->spread_stream);
    return 0;
}

static void
Panner_dealloc(Panner* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    Panner_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Panner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pantmp=NULL, *spreadtmp=NULL;
    Panner *self;
    self = (Panner *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Panner_compute_next_data_frame);
    self->mode_func_ptr = Panner_setProcMode;

    self->pan = PyFloat_FromDouble(0.5);
    self->spread = PyFloat_FromDouble(0.5);
    self->chnls = 2;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    static char *kwlist[] = {"input", "outs", "pan", "spread", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->chnls, &pantmp, &spreadtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (pantmp) {
        PyObject_CallMethod((PyObject *)self, "setPan", "O", pantmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->chnls < 1)
        self->chnls = 1;

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->chnls * self->bufsize * sizeof(MYFLT));

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Panner_getServer(Panner* self) { GET_SERVER };
static PyObject * Panner_getStream(Panner* self) { GET_STREAM };

static PyObject * Panner_play(Panner *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Panner_stop(Panner *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Panner_setPan(Panner *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pan);
	if (isNumber == 1) {
		self->pan = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->pan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pan_stream);
        self->pan_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Panner_setSpread(Panner *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->spread);
	if (isNumber == 1) {
		self->spread = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
	}
	else {
		self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Panner_members[] = {
{"server", T_OBJECT_EX, offsetof(Panner, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Panner, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Panner, input), 0, "Input sound object."},
{"pan", T_OBJECT_EX, offsetof(Panner, pan), 0, "Pan object."},
{"spread", T_OBJECT_EX, offsetof(Panner, spread), 0, "Spread object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Panner_methods[] = {
{"getServer", (PyCFunction)Panner_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Panner_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Panner_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Panner_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setPan", (PyCFunction)Panner_setPan, METH_O, "Sets panning value between 0 and 1."},
{"setSpread", (PyCFunction)Panner_setSpread, METH_O, "Sets spreading value between 0 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject PannerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Panner_base",                                   /*tp_name*/
sizeof(Panner),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Panner_dealloc,                     /*tp_dealloc*/
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
"Panner main objects.",           /* tp_doc */
(traverseproc)Panner_traverse,                  /* tp_traverse */
(inquiry)Panner_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Panner_methods,                                 /* tp_methods */
Panner_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Panner_new,                                     /* tp_new */
};

/************************************************************************************************/
/* Pan streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Panner *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} Pan;

static void Pan_postprocessing_ii(Pan *self) { POST_PROCESSING_II };
static void Pan_postprocessing_ai(Pan *self) { POST_PROCESSING_AI };
static void Pan_postprocessing_ia(Pan *self) { POST_PROCESSING_IA };
static void Pan_postprocessing_aa(Pan *self) { POST_PROCESSING_AA };
static void Pan_postprocessing_ireva(Pan *self) { POST_PROCESSING_IREVA };
static void Pan_postprocessing_areva(Pan *self) { POST_PROCESSING_AREVA };
static void Pan_postprocessing_revai(Pan *self) { POST_PROCESSING_REVAI };
static void Pan_postprocessing_revaa(Pan *self) { POST_PROCESSING_REVAA };
static void Pan_postprocessing_revareva(Pan *self) { POST_PROCESSING_REVAREVA };

static void
Pan_setProcMode(Pan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Pan_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Pan_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Pan_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Pan_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Pan_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Pan_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Pan_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Pan_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Pan_postprocessing_revareva;
            break;
    }
}

static void
Pan_compute_next_data_frame(Pan *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Panner_getSamplesBuffer((Panner *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Pan_traverse(Pan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
Pan_clear(Pan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
Pan_dealloc(Pan* self)
{
    pyo_DEALLOC
    Pan_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Pan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    Pan *self;
    self = (Pan *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pan_compute_next_data_frame);
    self->mode_func_ptr = Pan_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (Panner *)maintmp;

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

static PyObject * Pan_getServer(Pan* self) { GET_SERVER };
static PyObject * Pan_getStream(Pan* self) { GET_STREAM };
static PyObject * Pan_setMul(Pan *self, PyObject *arg) { SET_MUL };
static PyObject * Pan_setAdd(Pan *self, PyObject *arg) { SET_ADD };
static PyObject * Pan_setSub(Pan *self, PyObject *arg) { SET_SUB };
static PyObject * Pan_setDiv(Pan *self, PyObject *arg) { SET_DIV };

static PyObject * Pan_play(Pan *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Pan_out(Pan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pan_stop(Pan *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Pan_multiply(Pan *self, PyObject *arg) { MULTIPLY };
static PyObject * Pan_inplace_multiply(Pan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pan_add(Pan *self, PyObject *arg) { ADD };
static PyObject * Pan_inplace_add(Pan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pan_sub(Pan *self, PyObject *arg) { SUB };
static PyObject * Pan_inplace_sub(Pan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pan_div(Pan *self, PyObject *arg) { DIV };
static PyObject * Pan_inplace_div(Pan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Pan_members[] = {
{"server", T_OBJECT_EX, offsetof(Pan, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pan, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Pan, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pan, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pan_methods[] = {
{"getServer", (PyCFunction)Pan_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pan_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Pan_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pan_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pan_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)Pan_setMul, METH_O, "Sets Pan mul factor."},
{"setAdd", (PyCFunction)Pan_setAdd, METH_O, "Sets Pan add factor."},
{"setSub", (PyCFunction)Pan_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Pan_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pan_as_number = {
(binaryfunc)Pan_add,                      /*nb_add*/
(binaryfunc)Pan_sub,                 /*nb_subtract*/
(binaryfunc)Pan_multiply,                 /*nb_multiply*/
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
(binaryfunc)Pan_inplace_add,              /*inplace_add*/
(binaryfunc)Pan_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pan_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Pan_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Pan_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject PanType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Pan_base",         /*tp_name*/
sizeof(Pan),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pan_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Pan_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Pan objects. Reads one band from a Panner.",           /* tp_doc */
(traverseproc)Pan_traverse,   /* tp_traverse */
(inquiry)Pan_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pan_methods,             /* tp_methods */
Pan_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Pan_new,                 /* tp_new */
};

/*********************/
/*** Simple panner ***/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *pan;
    Stream *pan_stream;
    int chnls;
    int k1;
    int k2;
    int modebuffer[1];
    MYFLT *buffer_streams;
} SPanner;

static void
SPanner_splitter_thru(SPanner *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i] = in[i];
    }
}

static void
SPanner_splitter_st_i(SPanner *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT pan = P_clip(PyFloat_AS_DOUBLE(self->pan));

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        self->buffer_streams[i] = inval * MYSQRT(1.0 - pan);
        self->buffer_streams[i+self->bufsize] = inval * MYSQRT(pan);
    }
}

static void
SPanner_splitter_st_a(SPanner *self) {
    int i;
    MYFLT inval, panval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *pan = Stream_getData((Stream *)self->pan_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        panval = P_clip(pan[i]);
        self->buffer_streams[i] = inval * MYSQRT(1.0 - panval);
        self->buffer_streams[i+self->bufsize] = inval * MYSQRT(panval);
    }
}

static void
SPanner_splitter_i(SPanner *self) {
    MYFLT inval, min, pan1, pan2;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT pan = PyFloat_AS_DOUBLE(self->pan);
    MYFLT fchnls = (MYFLT)self->chnls;

    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i+self->k1] = 0.0;
        self->buffer_streams[i+self->k2] = 0.0;
    }

    min = 0;
    self->k1 = 0;
    self->k2 = self->bufsize;

    for (j=self->chnls; j>0; j--) {
        int j1 = j - 1;
        min = j1 / fchnls;
        if (pan > min) {
            self->k1 = j1 * self->bufsize;
            if (j == self->chnls)
                self->k2 = 0;
            else
                self->k2 = j * self->bufsize;
            break;
        }
    }

    pan = P_clip((pan - min) * self->chnls);
    pan1 = MYSQRT(1.0 - pan);
    pan2 = MYSQRT(pan);
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        self->buffer_streams[i+self->k1] = inval * pan1;
        self->buffer_streams[i+self->k2] = inval * pan2;
    }
}

static void
SPanner_splitter_a(SPanner *self) {
    MYFLT inval, min, pan;
    int i, j, j1, len;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *apan = Stream_getData((Stream *)self->pan_stream);
    MYFLT fchnls = (MYFLT)self->chnls;

    len = self->chnls * self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        pan = apan[i];
        inval = in[i];
        min = 0;
        self->k1 = 0;
        self->k2 = self->bufsize;

        for (j=self->chnls; j>0; j--) {
            j1 = j - 1;
            min = j1 / fchnls;
            if (pan > min) {
                self->k1 = j1 * self->bufsize;
                if (j == self->chnls)
                    self->k2 = 0;
                else
                    self->k2 = j * self->bufsize;
                break;
            }
        }

        pan = P_clip((pan - min) * self->chnls);
        self->buffer_streams[i+self->k1] = inval * MYSQRT(1.0 - pan);
        self->buffer_streams[i+self->k2] = inval * MYSQRT(pan);
    }
}

MYFLT *
SPanner_getSamplesBuffer(SPanner *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
SPanner_setProcMode(SPanner *self)
{
    int procmode;
    procmode = self->modebuffer[0];

    if (self->chnls > 2) {
        switch (procmode) {
            case 0:
                self->proc_func_ptr = SPanner_splitter_i;
                break;
            case 1:
                self->proc_func_ptr = SPanner_splitter_a;
                break;
        }
    }
    else if (self->chnls == 2) {
        switch (self->modebuffer[0]) {
            case 0:
                self->proc_func_ptr = SPanner_splitter_st_i;
                break;
            case 1:
                self->proc_func_ptr = SPanner_splitter_st_a;
                break;
        }
    }
    else if (self->chnls == 1) {
            self->proc_func_ptr = SPanner_splitter_thru;
    }
}

static void
SPanner_compute_next_data_frame(SPanner *self)
{
    (*self->proc_func_ptr)(self);
}

static int
SPanner_traverse(SPanner *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->pan);
    Py_VISIT(self->pan_stream);
    return 0;
}

static int
SPanner_clear(SPanner *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->pan);
    Py_CLEAR(self->pan_stream);
    return 0;
}

static void
SPanner_dealloc(SPanner* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    SPanner_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SPanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pantmp=NULL;
    SPanner *self;
    self = (SPanner *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SPanner_compute_next_data_frame);
    self->mode_func_ptr = SPanner_setProcMode;

    self->pan = PyFloat_FromDouble(0.5);
    self->chnls = 2;
    self->k1 = 0;
    self->k2 = self->bufsize;
    self->modebuffer[0] = 0;

    static char *kwlist[] = {"input", "outs", "pan", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iO", kwlist, &inputtmp, &self->chnls, &pantmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (pantmp) {
        PyObject_CallMethod((PyObject *)self, "setPan", "O", pantmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->chnls < 1)
        self->chnls = 1;

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->chnls * self->bufsize * sizeof(MYFLT));

    (*self->mode_func_ptr)(self);

    int len = self->chnls*self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }

    return (PyObject *)self;
}

static PyObject * SPanner_getServer(SPanner* self) { GET_SERVER };
static PyObject * SPanner_getStream(SPanner* self) { GET_STREAM };

static PyObject * SPanner_play(SPanner *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SPanner_stop(SPanner *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
SPanner_setPan(SPanner *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pan);
	if (isNumber == 1) {
		self->pan = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->pan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pan_stream);
        self->pan_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef SPanner_members[] = {
{"server", T_OBJECT_EX, offsetof(SPanner, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SPanner, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(SPanner, input), 0, "Input sound object."},
{"pan", T_OBJECT_EX, offsetof(SPanner, pan), 0, "Pan object."},
{NULL}  /* Sentinel */
};

static PyMethodDef SPanner_methods[] = {
{"getServer", (PyCFunction)SPanner_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SPanner_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)SPanner_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)SPanner_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setPan", (PyCFunction)SPanner_setPan, METH_O, "Sets panning value between 0 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject SPannerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SPanner_base",                                   /*tp_name*/
sizeof(SPanner),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)SPanner_dealloc,                     /*tp_dealloc*/
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
"SPanner main objects. Simple equal power panner",           /* tp_doc */
(traverseproc)SPanner_traverse,                  /* tp_traverse */
(inquiry)SPanner_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
SPanner_methods,                                 /* tp_methods */
SPanner_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
SPanner_new,                                     /* tp_new */
};

/************************************************************************************************/
/* SSPan streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    SPanner *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} SPan;

static void SPan_postprocessing_ii(SPan *self) { POST_PROCESSING_II };
static void SPan_postprocessing_ai(SPan *self) { POST_PROCESSING_AI };
static void SPan_postprocessing_ia(SPan *self) { POST_PROCESSING_IA };
static void SPan_postprocessing_aa(SPan *self) { POST_PROCESSING_AA };
static void SPan_postprocessing_ireva(SPan *self) { POST_PROCESSING_IREVA };
static void SPan_postprocessing_areva(SPan *self) { POST_PROCESSING_AREVA };
static void SPan_postprocessing_revai(SPan *self) { POST_PROCESSING_REVAI };
static void SPan_postprocessing_revaa(SPan *self) { POST_PROCESSING_REVAA };
static void SPan_postprocessing_revareva(SPan *self) { POST_PROCESSING_REVAREVA };

static void
SPan_setProcMode(SPan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SPan_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SPan_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SPan_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SPan_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SPan_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SPan_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SPan_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SPan_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SPan_postprocessing_revareva;
            break;
    }
}

static void
SPan_compute_next_data_frame(SPan *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SPanner_getSamplesBuffer((SPanner *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
SPan_traverse(SPan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
SPan_clear(SPan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
SPan_dealloc(SPan* self)
{
    pyo_DEALLOC
    SPan_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SPan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    SPan *self;
    self = (SPan *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SPan_compute_next_data_frame);
    self->mode_func_ptr = SPan_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (SPanner *)maintmp;

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

static PyObject * SPan_getServer(SPan* self) { GET_SERVER };
static PyObject * SPan_getStream(SPan* self) { GET_STREAM };
static PyObject * SPan_setMul(SPan *self, PyObject *arg) { SET_MUL };
static PyObject * SPan_setAdd(SPan *self, PyObject *arg) { SET_ADD };
static PyObject * SPan_setSub(SPan *self, PyObject *arg) { SET_SUB };
static PyObject * SPan_setDiv(SPan *self, PyObject *arg) { SET_DIV };

static PyObject * SPan_play(SPan *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SPan_out(SPan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SPan_stop(SPan *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SPan_multiply(SPan *self, PyObject *arg) { MULTIPLY };
static PyObject * SPan_inplace_multiply(SPan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SPan_add(SPan *self, PyObject *arg) { ADD };
static PyObject * SPan_inplace_add(SPan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SPan_sub(SPan *self, PyObject *arg) { SUB };
static PyObject * SPan_inplace_sub(SPan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SPan_div(SPan *self, PyObject *arg) { DIV };
static PyObject * SPan_inplace_div(SPan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SPan_members[] = {
{"server", T_OBJECT_EX, offsetof(SPan, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SPan, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(SPan, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SPan, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SPan_methods[] = {
{"getServer", (PyCFunction)SPan_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SPan_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)SPan_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SPan_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SPan_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)SPan_setMul, METH_O, "Sets SPan mul factor."},
{"setAdd", (PyCFunction)SPan_setAdd, METH_O, "Sets SPan add factor."},
{"setSub", (PyCFunction)SPan_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SPan_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SPan_as_number = {
(binaryfunc)SPan_add,                      /*nb_add*/
(binaryfunc)SPan_sub,                 /*nb_subtract*/
(binaryfunc)SPan_multiply,                 /*nb_multiply*/
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
(binaryfunc)SPan_inplace_add,              /*inplace_add*/
(binaryfunc)SPan_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SPan_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)SPan_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)SPan_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SPanType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SPan_base",         /*tp_name*/
sizeof(SPan),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SPan_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&SPan_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SPan objects. Reads one band from a SPanner.",           /* tp_doc */
(traverseproc)SPan_traverse,   /* tp_traverse */
(inquiry)SPan_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SPan_methods,             /* tp_methods */
SPan_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
SPan_new,                 /* tp_new */
};

/*********************/
/***** Switcher ******/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *voice;
    Stream *voice_stream;
    int chnls;
    int k1;
    int k2;
    int modebuffer[1];
    MYFLT *buffer_streams;
} Switcher;

static MYFLT
Switcher_clip_voice(Switcher *self, MYFLT v) {
    int chnls = self->chnls - 1;
    if (v < 0.0)
        return 0.0;
    else if (v > chnls)
        return chnls;
    else
        return v;
}

static void
Switcher_splitter_i(Switcher *self) {
    MYFLT inval, voice1, voice2;
    int j1, j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT voice = Switcher_clip_voice(self, PyFloat_AS_DOUBLE(self->voice));

    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i+self->k1] = 0.0;
        self->buffer_streams[i+self->k2] = 0.0;
    }

    j1 = (int)voice;
    j = j1 + 1;
    if (j1 >= (self->chnls-1)) {
        j1--; j--;
    }

    self->k1 = j1 * self->bufsize;
    self->k2 = j * self->bufsize;

    voice = P_clip(voice - j1);
    voice1 = MYSQRT(1.0 - voice);
    voice2 = MYSQRT(voice);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        self->buffer_streams[i+self->k1] = inval * voice1;
        self->buffer_streams[i+self->k2] = inval * voice2;
    }
}

static void
Switcher_splitter_a(Switcher *self) {
    MYFLT inval, voice;
    int i, j, j1, len;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *avoice = Stream_getData((Stream *)self->voice_stream);

    len = self->chnls * self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        voice = Switcher_clip_voice(self, avoice[i]);
        inval = in[i];

        j1 = (int)voice;
        j = j1 + 1;
        if (j1 >= (self->chnls-1)) {
            j1--; j--;
        }

        self->k1 = j1 * self->bufsize;
        self->k2 = j * self->bufsize;

        voice = P_clip(voice - j1);

        self->buffer_streams[i+self->k1] = inval * MYSQRT(1.0 - voice);
        self->buffer_streams[i+self->k2] = inval * MYSQRT(voice);
    }
}

MYFLT *
Switcher_getSamplesBuffer(Switcher *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Switcher_setProcMode(Switcher *self)
{
    switch (self->modebuffer[0]) {
        case 0:
            self->proc_func_ptr = Switcher_splitter_i;
            break;
        case 1:
            self->proc_func_ptr = Switcher_splitter_a;
            break;
    }
}

static void
Switcher_compute_next_data_frame(Switcher *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Switcher_traverse(Switcher *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->voice);
    Py_VISIT(self->voice_stream);
    return 0;
}

static int
Switcher_clear(Switcher *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->voice);
    Py_CLEAR(self->voice_stream);
    return 0;
}

static void
Switcher_dealloc(Switcher* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    Switcher_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Switcher_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *voicetmp=NULL;
    Switcher *self;
    self = (Switcher *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Switcher_compute_next_data_frame);
    self->mode_func_ptr = Switcher_setProcMode;

    self->voice = PyFloat_FromDouble(0.0);
    self->chnls = 2;
    self->k1 = 0;
    self->k2 = self->bufsize;
    self->modebuffer[0] = 0;

    static char *kwlist[] = {"input", "outs", "voice", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iO", kwlist, &inputtmp, &self->chnls, &voicetmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (voicetmp) {
        PyObject_CallMethod((PyObject *)self, "setVoice", "O", voicetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->chnls * self->bufsize * sizeof(MYFLT));

    (*self->mode_func_ptr)(self);

    int len = self->chnls*self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }

    return (PyObject *)self;
}

static PyObject * Switcher_getServer(Switcher* self) { GET_SERVER };
static PyObject * Switcher_getStream(Switcher* self) { GET_STREAM };

static PyObject * Switcher_play(Switcher *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Switcher_stop(Switcher *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Switcher_setVoice(Switcher *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->voice);
	if (isNumber == 1) {
		self->voice = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->voice = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->voice, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->voice_stream);
        self->voice_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Switcher_members[] = {
    {"server", T_OBJECT_EX, offsetof(Switcher, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Switcher, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Switcher, input), 0, "Input sound object."},
    {"voice", T_OBJECT_EX, offsetof(Switcher, voice), 0, "voice object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Switcher_methods[] = {
    {"getServer", (PyCFunction)Switcher_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Switcher_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Switcher_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Switcher_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setVoice", (PyCFunction)Switcher_setVoice, METH_O, "Sets voice value between 0 and outs-1."},
    {NULL}  /* Sentinel */
};

PyTypeObject SwitcherType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Switcher_base",                                   /*tp_name*/
    sizeof(Switcher),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Switcher_dealloc,                     /*tp_dealloc*/
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
    "Switcher main objects. Simple equal power voicener",           /* tp_doc */
    (traverseproc)Switcher_traverse,                  /* tp_traverse */
    (inquiry)Switcher_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Switcher_methods,                                 /* tp_methods */
    Switcher_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Switcher_new,                                     /* tp_new */
};

/************************************************************************************************/
/* SSwitch streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Switcher *mainSplitter;
    int modebuffer[2];
    int chnl; // switch order
} Switch;

static void Switch_postprocessing_ii(Switch *self) { POST_PROCESSING_II };
static void Switch_postprocessing_ai(Switch *self) { POST_PROCESSING_AI };
static void Switch_postprocessing_ia(Switch *self) { POST_PROCESSING_IA };
static void Switch_postprocessing_aa(Switch *self) { POST_PROCESSING_AA };
static void Switch_postprocessing_ireva(Switch *self) { POST_PROCESSING_IREVA };
static void Switch_postprocessing_areva(Switch *self) { POST_PROCESSING_AREVA };
static void Switch_postprocessing_revai(Switch *self) { POST_PROCESSING_REVAI };
static void Switch_postprocessing_revaa(Switch *self) { POST_PROCESSING_REVAA };
static void Switch_postprocessing_revareva(Switch *self) { POST_PROCESSING_REVAREVA };

static void
Switch_setProcMode(Switch *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Switch_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Switch_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Switch_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Switch_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Switch_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Switch_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Switch_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Switch_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Switch_postprocessing_revareva;
            break;
    }
}

static void
Switch_compute_next_data_frame(Switch *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Switcher_getSamplesBuffer((Switcher *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Switch_traverse(Switch *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
Switch_clear(Switch *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
Switch_dealloc(Switch* self)
{
    pyo_DEALLOC
    Switch_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Switch_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    Switch *self;
    self = (Switch *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Switch_compute_next_data_frame);
    self->mode_func_ptr = Switch_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (Switcher *)maintmp;

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

static PyObject * Switch_getServer(Switch* self) { GET_SERVER };
static PyObject * Switch_getStream(Switch* self) { GET_STREAM };
static PyObject * Switch_setMul(Switch *self, PyObject *arg) { SET_MUL };
static PyObject * Switch_setAdd(Switch *self, PyObject *arg) { SET_ADD };
static PyObject * Switch_setSub(Switch *self, PyObject *arg) { SET_SUB };
static PyObject * Switch_setDiv(Switch *self, PyObject *arg) { SET_DIV };

static PyObject * Switch_play(Switch *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Switch_out(Switch *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Switch_stop(Switch *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Switch_multiply(Switch *self, PyObject *arg) { MULTIPLY };
static PyObject * Switch_inplace_multiply(Switch *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Switch_add(Switch *self, PyObject *arg) { ADD };
static PyObject * Switch_inplace_add(Switch *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Switch_sub(Switch *self, PyObject *arg) { SUB };
static PyObject * Switch_inplace_sub(Switch *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Switch_div(Switch *self, PyObject *arg) { DIV };
static PyObject * Switch_inplace_div(Switch *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Switch_members[] = {
    {"server", T_OBJECT_EX, offsetof(Switch, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Switch, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Switch, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Switch, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Switch_methods[] = {
    {"getServer", (PyCFunction)Switch_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Switch_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Switch_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Switch_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Switch_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Switch_setMul, METH_O, "Sets Switch mul factor."},
    {"setAdd", (PyCFunction)Switch_setAdd, METH_O, "Sets Switch add factor."},
    {"setSub", (PyCFunction)Switch_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Switch_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Switch_as_number = {
    (binaryfunc)Switch_add,                      /*nb_add*/
    (binaryfunc)Switch_sub,                 /*nb_subtract*/
    (binaryfunc)Switch_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Switch_inplace_add,              /*inplace_add*/
    (binaryfunc)Switch_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Switch_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Switch_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Switch_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject SwitchType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Switch_base",         /*tp_name*/
    sizeof(Switch),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Switch_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Switch_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "Switch objects. Reads one band from a Switchner.",           /* tp_doc */
    (traverseproc)Switch_traverse,   /* tp_traverse */
    (inquiry)Switch_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Switch_methods,             /* tp_methods */
    Switch_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Switch_new,                 /* tp_new */
};

/****************/
/**** VoiceManager *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    Stream **trigger_streams;
    int maxVoices;
    int *voices;
    int modebuffer[2]; // need at least 2 slots for mul & add
} VoiceManager;

static void
VoiceManager_generate(VoiceManager *self) {
    int j, i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++)
        self->data[i] = -1.0;

    if (self->maxVoices > 0) {
        for (i=0; i<self->bufsize; i++) {
            for (j=0; j<self->maxVoices; j++) {
                if (Stream_getData(self->trigger_streams[j])[i] == 1.0)
                    self->voices[j] = 0;
            }
            if (in[i] == 1.0) {
                for (j=0; j<self->maxVoices; j++) {
                    if (self->voices[j] == 0) {
                        self->data[i] = (MYFLT)j;
                        self->voices[j] = 1;
                        break;
                    }
                }
            }
        }
    }
}

static void VoiceManager_postprocessing_ii(VoiceManager *self) { POST_PROCESSING_II };
static void VoiceManager_postprocessing_ai(VoiceManager *self) { POST_PROCESSING_AI };
static void VoiceManager_postprocessing_ia(VoiceManager *self) { POST_PROCESSING_IA };
static void VoiceManager_postprocessing_aa(VoiceManager *self) { POST_PROCESSING_AA };
static void VoiceManager_postprocessing_ireva(VoiceManager *self) { POST_PROCESSING_IREVA };
static void VoiceManager_postprocessing_areva(VoiceManager *self) { POST_PROCESSING_AREVA };
static void VoiceManager_postprocessing_revai(VoiceManager *self) { POST_PROCESSING_REVAI };
static void VoiceManager_postprocessing_revaa(VoiceManager *self) { POST_PROCESSING_REVAA };
static void VoiceManager_postprocessing_revareva(VoiceManager *self) { POST_PROCESSING_REVAREVA };

static void
VoiceManager_setProcMode(VoiceManager *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = VoiceManager_generate;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = VoiceManager_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = VoiceManager_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = VoiceManager_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = VoiceManager_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = VoiceManager_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = VoiceManager_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = VoiceManager_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = VoiceManager_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = VoiceManager_postprocessing_revareva;
            break;
    }
}

static void
VoiceManager_compute_next_data_frame(VoiceManager *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
VoiceManager_traverse(VoiceManager *self, visitproc visit, void *arg)
{
    int i;
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    for (i=0; i<self->maxVoices; i++) {
        Py_VISIT(self->trigger_streams[i]);
    }
    return 0;
}

static int
VoiceManager_clear(VoiceManager *self)
{
    int i;
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    for (i=0; i<self->maxVoices; i++) {
        Py_CLEAR(self->trigger_streams[i]);
    }
    return 0;
}

static void
VoiceManager_dealloc(VoiceManager* self)
{
    pyo_DEALLOC
    VoiceManager_clear(self);
    if (self->voices != NULL) {
        free(self->voices);
        free(self->trigger_streams);
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
VoiceManager_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *triggerstmp=NULL, *multmp=NULL, *addtmp=NULL;
    VoiceManager *self;
    self = (VoiceManager *)type->tp_alloc(type, 0);

    self->voices = NULL;
    self->maxVoices = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, VoiceManager_compute_next_data_frame);
    self->mode_func_ptr = VoiceManager_setProcMode;

    static char *kwlist[] = {"input", "triggers", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &triggerstmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (triggerstmp && triggerstmp != Py_None) {
        PyObject_CallMethod((PyObject *)self, "setTriggers", "O", triggerstmp);
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

static PyObject * VoiceManager_getServer(VoiceManager* self) { GET_SERVER };
static PyObject * VoiceManager_getStream(VoiceManager* self) { GET_STREAM };
static PyObject * VoiceManager_setMul(VoiceManager *self, PyObject *arg) { SET_MUL };
static PyObject * VoiceManager_setAdd(VoiceManager *self, PyObject *arg) { SET_ADD };
static PyObject * VoiceManager_setSub(VoiceManager *self, PyObject *arg) { SET_SUB };
static PyObject * VoiceManager_setDiv(VoiceManager *self, PyObject *arg) { SET_DIV };

static PyObject * VoiceManager_play(VoiceManager *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * VoiceManager_stop(VoiceManager *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * VoiceManager_multiply(VoiceManager *self, PyObject *arg) { MULTIPLY };
static PyObject * VoiceManager_inplace_multiply(VoiceManager *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * VoiceManager_add(VoiceManager *self, PyObject *arg) { ADD };
static PyObject * VoiceManager_inplace_add(VoiceManager *self, PyObject *arg) { INPLACE_ADD };
static PyObject * VoiceManager_sub(VoiceManager *self, PyObject *arg) { SUB };
static PyObject * VoiceManager_inplace_sub(VoiceManager *self, PyObject *arg) { INPLACE_SUB };
static PyObject * VoiceManager_div(VoiceManager *self, PyObject *arg) { DIV };
static PyObject * VoiceManager_inplace_div(VoiceManager *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
VoiceManager_setTriggers(VoiceManager *self, PyObject *arg)
{
    int i;

	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The triggers attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    self->maxVoices = PyList_Size(arg);
    self->trigger_streams = (Stream **)realloc(self->trigger_streams, self->maxVoices * sizeof(Stream *));
    self->voices = (int *)realloc(self->voices, self->maxVoices * sizeof(int));
    for (i=0; i<self->maxVoices; i++) {
        self->trigger_streams[i] = (Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(arg, i), "_getStream", NULL);
        self->voices[i] = 0;
    }

    Py_RETURN_NONE;
}

static PyMemberDef VoiceManager_members[] = {
{"server", T_OBJECT_EX, offsetof(VoiceManager, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(VoiceManager, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(VoiceManager, input), 0, "Input sound object."},
{"trigger_streams", T_OBJECT_EX, offsetof(VoiceManager, trigger_streams), 0, "Array of trigger streams."},
{"mul", T_OBJECT_EX, offsetof(VoiceManager, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(VoiceManager, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef VoiceManager_methods[] = {
{"getServer", (PyCFunction)VoiceManager_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)VoiceManager_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)VoiceManager_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)VoiceManager_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setTriggers", (PyCFunction)VoiceManager_setTriggers, METH_O, "Sets list of trigger streams."},
{"setMul", (PyCFunction)VoiceManager_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)VoiceManager_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)VoiceManager_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)VoiceManager_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods VoiceManager_as_number = {
(binaryfunc)VoiceManager_add,                         /*nb_add*/
(binaryfunc)VoiceManager_sub,                         /*nb_subtract*/
(binaryfunc)VoiceManager_multiply,                    /*nb_multiply*/
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
(binaryfunc)VoiceManager_inplace_add,                 /*inplace_add*/
(binaryfunc)VoiceManager_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)VoiceManager_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)VoiceManager_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)VoiceManager_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject VoiceManagerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.VoiceManager_base",                                   /*tp_name*/
sizeof(VoiceManager),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)VoiceManager_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&VoiceManager_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"VoiceManager objects. Return the first free voices when receiving a trigger signal.",           /* tp_doc */
(traverseproc)VoiceManager_traverse,                  /* tp_traverse */
(inquiry)VoiceManager_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
VoiceManager_methods,                                 /* tp_methods */
VoiceManager_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
VoiceManager_new,                                     /* tp_new */
};

/****************/
/**** Mixer *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *inputs;
    PyObject *gains;
    PyObject *lastGains;
    PyObject *currentGains;
    PyObject *stepVals;
    PyObject *timeCounts;
    int num_outs;
    MYFLT time;
    long timeStep;
    MYFLT *buffer_streams;
} Mixer;

static void
Mixer_generate(Mixer *self) {
    int num, k, j, i, tmpCount;
    MYFLT amp, lastAmp, currentAmp, tmpStepVal;
    PyObject *keys;
    PyObject *key;
    PyObject *list_of_gains, *list_of_last_gains, *list_of_current_gains, *list_of_step_vals, *list_of_time_counts;

    for (i=0; i<(self->num_outs * self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    keys = PyDict_Keys(self->inputs);
    num = PyList_Size(keys);

    for (j=0; j<num; j++) {
        key = PyList_GetItem(keys, j);
        MYFLT *st = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyDict_GetItem(self->inputs, key), "_getStream", NULL));
        list_of_gains = PyDict_GetItem(self->gains, key);
        list_of_last_gains = PyDict_GetItem(self->lastGains, key);
        list_of_current_gains = PyDict_GetItem(self->currentGains, key);
        list_of_step_vals = PyDict_GetItem(self->stepVals, key);
        list_of_time_counts = PyDict_GetItem(self->timeCounts, key);
        for (k=0; k<self->num_outs; k++) {
            amp = (MYFLT)PyFloat_AS_DOUBLE(PyList_GetItem(list_of_gains, k));
            lastAmp = (MYFLT)PyFloat_AS_DOUBLE(PyList_GetItem(list_of_last_gains, k));
            currentAmp = (MYFLT)PyFloat_AS_DOUBLE(PyList_GetItem(list_of_current_gains, k));
            tmpStepVal = (MYFLT)PyFloat_AS_DOUBLE(PyList_GetItem(list_of_step_vals, k));
            tmpCount = (int)PyLong_AsLong(PyList_GetItem(list_of_time_counts, k));
            if (amp != lastAmp) {
                tmpCount = 0;
                tmpStepVal = (amp - currentAmp) / self->timeStep;
                PyList_SetItem(list_of_last_gains, k, PyFloat_FromDouble(amp));
            }
            for (i=0; i<self->bufsize; i++) {
                if (tmpCount == (self->timeStep - 1)) {
                    currentAmp = amp;
                    tmpCount++;
                }
                else if (tmpCount < self->timeStep) {
                    currentAmp += tmpStepVal;
                    tmpCount++;
                }
                self->buffer_streams[self->bufsize * k + i] += st[i] * currentAmp;
            }
            PyList_SetItem(list_of_current_gains, k, PyFloat_FromDouble(currentAmp));
            PyList_SetItem(list_of_step_vals, k, PyFloat_FromDouble(tmpStepVal));
            PyList_SetItem(list_of_time_counts, k, PyLong_FromLong(tmpCount));
        }
    }
    Py_XDECREF(keys);
}

MYFLT *
Mixer_getSamplesBuffer(Mixer *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Mixer_setProcMode(Mixer *self)
{
    self->proc_func_ptr = Mixer_generate;
}

static void
Mixer_compute_next_data_frame(Mixer *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Mixer_traverse(Mixer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inputs);
    Py_VISIT(self->gains);
    Py_VISIT(self->lastGains);
    Py_VISIT(self->currentGains);
    Py_VISIT(self->stepVals);
    Py_VISIT(self->timeCounts);
    return 0;
}

static int
Mixer_clear(Mixer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inputs);
    Py_CLEAR(self->gains);
    Py_CLEAR(self->lastGains);
    Py_CLEAR(self->currentGains);
    Py_CLEAR(self->stepVals);
    Py_CLEAR(self->timeCounts);
    return 0;
}

static void
Mixer_dealloc(Mixer* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    Mixer_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Mixer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *timetmp=NULL;
    Mixer *self;
    self = (Mixer *)type->tp_alloc(type, 0);

    self->inputs = PyDict_New();
    self->gains = PyDict_New();
    self->lastGains = PyDict_New();
    self->currentGains = PyDict_New();
    self->stepVals = PyDict_New();
    self->timeCounts = PyDict_New();
    self->num_outs = 2;
    self->time = 0.025;
    self->timeStep = (long)(self->time * self->sr);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Mixer_compute_next_data_frame);
    self->mode_func_ptr = Mixer_setProcMode;

    static char *kwlist[] = {"outs", "time", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iO", kwlist, &self->num_outs, &timetmp))
        Py_RETURN_NONE;

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->num_outs * self->bufsize * sizeof(MYFLT));

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Mixer_getServer(Mixer* self) { GET_SERVER };
static PyObject * Mixer_getStream(Mixer* self) { GET_STREAM };

static PyObject * Mixer_play(Mixer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Mixer_stop(Mixer *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Mixer_setTime(Mixer *self, PyObject *arg)
{
    int i, j, num;
	PyObject *tmp, *keys, *key, *list_of_time_counts;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1) {
		self->time = PyFloat_AsDouble(tmp);
        self->timeStep = (long)(self->time * self->sr);

        keys = PyDict_Keys(self->inputs);
        num = PyList_Size(keys);
        if (num != 0) {
            for (j=0; j<num; j++) {
                key = PyList_GET_ITEM(keys, j);
                list_of_time_counts = PyDict_GetItem(self->timeCounts, key);
                for (i=0; i<self->num_outs; i++) {
                    PyList_SET_ITEM(list_of_time_counts, i, PyLong_FromLong(self->timeStep-1));
                }
            }
        }
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Mixer_addInput(Mixer *self, PyObject *args, PyObject *kwds)
{
    int i;
	PyObject *tmp;
    PyObject *initGains;
    PyObject *initLastGains;
    PyObject *initCurrentGains;
    PyObject *initStepVals;
    PyObject *initTimeCounts;
    PyObject *voice;

    static char *kwlist[] = {"voice", "input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &voice, &tmp)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    PyDict_SetItem(self->inputs, voice, tmp);
    initGains = PyList_New(self->num_outs);
    initLastGains = PyList_New(self->num_outs);
    initCurrentGains = PyList_New(self->num_outs);
    initStepVals = PyList_New(self->num_outs);
    initTimeCounts = PyList_New(self->num_outs);
    for (i=0; i<self->num_outs; i++) {
        PyList_SET_ITEM(initGains, i, PyFloat_FromDouble(0.0));
        PyList_SET_ITEM(initLastGains, i, PyFloat_FromDouble(0.0));
        PyList_SET_ITEM(initCurrentGains, i, PyFloat_FromDouble(0.0));
        PyList_SET_ITEM(initStepVals, i, PyFloat_FromDouble(0.0));
        PyList_SET_ITEM(initTimeCounts, i, PyInt_FromLong(0));
    }
    PyDict_SetItem(self->gains, voice, initGains);
    PyDict_SetItem(self->lastGains, voice, initLastGains);
    PyDict_SetItem(self->currentGains, voice, initCurrentGains);
    PyDict_SetItem(self->stepVals, voice, initStepVals);
    PyDict_SetItem(self->timeCounts, voice, initTimeCounts);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Mixer_delInput(Mixer *self, PyObject *arg)
{
    int ret;

    PyObject *key = arg;
    ret = PyDict_DelItem(self->inputs, key);
    if (ret == 0) {
        PyDict_DelItem(self->gains, key);
        PyDict_DelItem(self->lastGains, key);
        PyDict_DelItem(self->currentGains, key);
        PyDict_DelItem(self->stepVals, key);
        PyDict_DelItem(self->timeCounts, key);
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Mixer_setAmp(Mixer *self, PyObject *args, PyObject *kwds)
{
    int tmpout;
    PyObject *tmpin, *amp;
    static char *kwlist[] = {"vin", "vout", "amp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OiO", kwlist, &tmpin, &tmpout, &amp)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (! PyNumber_Check(amp)) {
        PySys_WriteStdout("Mixer: amp argument must be a number!n");
        Py_INCREF(Py_None);
        return Py_None;
    }

    Py_INCREF(amp);
    PyList_SET_ITEM(PyDict_GetItem(self->gains, tmpin), tmpout, PyNumber_Float(amp));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Mixer_members[] = {
    {"server", T_OBJECT_EX, offsetof(Mixer, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Mixer, stream), 0, "Stream object."},
    {"inputs", T_OBJECT_EX, offsetof(Mixer, inputs), 0, "Dictionary of input streams."},
    {"gains", T_OBJECT_EX, offsetof(Mixer, gains), 0, "Dictionary of list of amplitudes."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Mixer_methods[] = {
    {"getServer", (PyCFunction)Mixer_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Mixer_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Mixer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Mixer_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setTime", (PyCFunction)Mixer_setTime, METH_O, "Sets ramp time in seconds."},
    {"addInput", (PyCFunction)Mixer_addInput, METH_VARARGS|METH_KEYWORDS, "Adds an input to the mixer."},
    {"delInput", (PyCFunction)Mixer_delInput, METH_O, "Removes an input from the mixer."},
    {"setAmp", (PyCFunction)Mixer_setAmp, METH_VARARGS|METH_KEYWORDS, "Sets the amplitude of a specific input toward a specific output."},
    {NULL}  /* Sentinel */
};

PyTypeObject MixerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Mixer_base",                                   /*tp_name*/
    sizeof(Mixer),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Mixer_dealloc,                     /*tp_dealloc*/
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
    "Mixer objects. Mixes multiple inputs toward multiple outputs.",           /* tp_doc */
    (traverseproc)Mixer_traverse,                  /* tp_traverse */
    (inquiry)Mixer_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Mixer_methods,                                 /* tp_methods */
    Mixer_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Mixer_new,                                     /* tp_new */
};

/************************************************************************************************/
/* MixerVoice streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Mixer *mainMixer;
    int modebuffer[2];
    int chnl; // voice order
} MixerVoice;

static void MixerVoice_postprocessing_ii(MixerVoice *self) { POST_PROCESSING_II };
static void MixerVoice_postprocessing_ai(MixerVoice *self) { POST_PROCESSING_AI };
static void MixerVoice_postprocessing_ia(MixerVoice *self) { POST_PROCESSING_IA };
static void MixerVoice_postprocessing_aa(MixerVoice *self) { POST_PROCESSING_AA };
static void MixerVoice_postprocessing_ireva(MixerVoice *self) { POST_PROCESSING_IREVA };
static void MixerVoice_postprocessing_areva(MixerVoice *self) { POST_PROCESSING_AREVA };
static void MixerVoice_postprocessing_revai(MixerVoice *self) { POST_PROCESSING_REVAI };
static void MixerVoice_postprocessing_revaa(MixerVoice *self) { POST_PROCESSING_REVAA };
static void MixerVoice_postprocessing_revareva(MixerVoice *self) { POST_PROCESSING_REVAREVA };

static void
MixerVoice_setProcMode(MixerVoice *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MixerVoice_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MixerVoice_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MixerVoice_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MixerVoice_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MixerVoice_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MixerVoice_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MixerVoice_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MixerVoice_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MixerVoice_postprocessing_revareva;
            break;
    }
}

static void
MixerVoice_compute_next_data_frame(MixerVoice *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Mixer_getSamplesBuffer((Mixer *)self->mainMixer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MixerVoice_traverse(MixerVoice *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainMixer);
    return 0;
}

static int
MixerVoice_clear(MixerVoice *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainMixer);
    return 0;
}

static void
MixerVoice_dealloc(MixerVoice* self)
{
    pyo_DEALLOC
    MixerVoice_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MixerVoice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    MixerVoice *self;
    self = (MixerVoice *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MixerVoice_compute_next_data_frame);
    self->mode_func_ptr = MixerVoice_setProcMode;

    static char *kwlist[] = {"mainMixer", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainMixer);
    Py_INCREF(maintmp);
    self->mainMixer = (Mixer *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
}

static PyObject * MixerVoice_getServer(MixerVoice* self) { GET_SERVER };
static PyObject * MixerVoice_getStream(MixerVoice* self) { GET_STREAM };
static PyObject * MixerVoice_setMul(MixerVoice *self, PyObject *arg) { SET_MUL };
static PyObject * MixerVoice_setAdd(MixerVoice *self, PyObject *arg) { SET_ADD };
static PyObject * MixerVoice_setSub(MixerVoice *self, PyObject *arg) { SET_SUB };
static PyObject * MixerVoice_setDiv(MixerVoice *self, PyObject *arg) { SET_DIV };

static PyObject * MixerVoice_play(MixerVoice *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MixerVoice_out(MixerVoice *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MixerVoice_stop(MixerVoice *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MixerVoice_multiply(MixerVoice *self, PyObject *arg) { MULTIPLY };
static PyObject * MixerVoice_inplace_multiply(MixerVoice *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MixerVoice_add(MixerVoice *self, PyObject *arg) { ADD };
static PyObject * MixerVoice_inplace_add(MixerVoice *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MixerVoice_sub(MixerVoice *self, PyObject *arg) { SUB };
static PyObject * MixerVoice_inplace_sub(MixerVoice *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MixerVoice_div(MixerVoice *self, PyObject *arg) { DIV };
static PyObject * MixerVoice_inplace_div(MixerVoice *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MixerVoice_members[] = {
    {"server", T_OBJECT_EX, offsetof(MixerVoice, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MixerVoice, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MixerVoice, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MixerVoice, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MixerVoice_methods[] = {
    {"getServer", (PyCFunction)MixerVoice_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MixerVoice_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MixerVoice_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MixerVoice_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MixerVoice_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MixerVoice_setMul, METH_O, "Sets MixerVoice mul factor."},
    {"setAdd", (PyCFunction)MixerVoice_setAdd, METH_O, "Sets MixerVoice add factor."},
    {"setSub", (PyCFunction)MixerVoice_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MixerVoice_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MixerVoice_as_number = {
    (binaryfunc)MixerVoice_add,                      /*nb_add*/
    (binaryfunc)MixerVoice_sub,                 /*nb_subtract*/
    (binaryfunc)MixerVoice_multiply,                 /*nb_multiply*/
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
    (binaryfunc)MixerVoice_inplace_add,              /*inplace_add*/
    (binaryfunc)MixerVoice_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)MixerVoice_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)MixerVoice_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)MixerVoice_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject MixerVoiceType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MixerVoice_base",         /*tp_name*/
    sizeof(MixerVoice),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MixerVoice_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MixerVoice_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MixerVoice objects. Reads one band from a Mixer.",           /* tp_doc */
    (traverseproc)MixerVoice_traverse,   /* tp_traverse */
    (inquiry)MixerVoice_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MixerVoice_methods,             /* tp_methods */
    MixerVoice_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MixerVoice_new,                 /* tp_new */
};

/****************/
/**** Selector *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *inputs;
    PyObject *voice;
    Stream *voice_stream;
    int chSize;
    int mode;
    int modebuffer[3]; // need at least 2 slots for mul & add
} Selector;

static MYFLT
Selector_clip_voice(Selector *self, MYFLT v) {
    int chSize = self->chSize - 1;
    if (v < 0.0)
        return 0.0;
    else if (v > chSize)
        return chSize;
    else
        return v;
}

static void
Selector_generate_i(Selector *self) {
    int j1, j, i;
    MYFLT  voice1, voice2;
    MYFLT voice = Selector_clip_voice(self, PyFloat_AS_DOUBLE(self->voice));

    j1 = (int)voice;
    j = j1 + 1;
    if (j1 >= (self->chSize-1)) {
        j1--; j--;
    }

    MYFLT *st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j1), "_getStream", NULL));
    MYFLT *st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j), "_getStream", NULL));

    voice = P_clip(voice - j1);
    voice1 = MYSQRT(1.0 - voice);
    voice2 = MYSQRT(voice);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = st1[i] * voice1 + st2[i] * voice2;
    }
}

static void
Selector_generate_lin_i(Selector *self) {
    int j1, j, i;
    MYFLT voice = Selector_clip_voice(self, PyFloat_AS_DOUBLE(self->voice));

    j1 = (int)voice;
    j = j1 + 1;
    if (j1 >= (self->chSize-1)) {
        j1--; j--;
    }

    MYFLT *st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j1), "_getStream", NULL));
    MYFLT *st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j), "_getStream", NULL));

    voice = P_clip(voice - j1);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = st1[i] * (1.0 - voice) + st2[i] * voice;
    }
}

static void
Selector_generate_a(Selector *self) {
    int old_j1, old_j, j1, j, i;
    MYFLT  voice;
    MYFLT *st1, *st2;
    MYFLT *vc = Stream_getData((Stream *)self->voice_stream);

    old_j1 = 0;
    old_j = 1;
    st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, old_j1), "_getStream", NULL));
    st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, old_j), "_getStream", NULL));

    for (i=0; i<self->bufsize; i++) {
        voice = Selector_clip_voice(self, vc[i]);

        j1 = (int)voice;
        j = j1 + 1;
        if (j1 >= (self->chSize-1)) {
            j1--; j--;
        }
        if (j1 != old_j1) {
            st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j1), "_getStream", NULL));
            old_j1 = j1;
        }
        if (j != old_j) {
            st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j), "_getStream", NULL));
            old_j = j;
        }

        voice = P_clip(voice - j1);

        self->data[i] = st1[i] * MYSQRT(1.0 - voice) + st2[i] * MYSQRT(voice);
    }
}

static void
Selector_generate_lin_a(Selector *self) {
    int old_j1, old_j, j1, j, i;
    MYFLT  voice;
    MYFLT *st1, *st2;
    MYFLT *vc = Stream_getData((Stream *)self->voice_stream);

    old_j1 = 0;
    old_j = 1;
    st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, old_j1), "_getStream", NULL));
    st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, old_j), "_getStream", NULL));

    for (i=0; i<self->bufsize; i++) {
        voice = Selector_clip_voice(self, vc[i]);

        j1 = (int)voice;
        j = j1 + 1;
        if (j1 >= (self->chSize-1)) {
            j1--; j--;
        }
        if (j1 != old_j1) {
            st1 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j1), "_getStream", NULL));
            old_j1 = j1;
        }
        if (j != old_j) {
            st2 = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->inputs, j), "_getStream", NULL));
            old_j = j;
        }

        voice = P_clip(voice - j1);

        self->data[i] = st1[i] * (1.0 - voice) + st2[i] * voice;
    }
}

static void Selector_postprocessing_ii(Selector *self) { POST_PROCESSING_II };
static void Selector_postprocessing_ai(Selector *self) { POST_PROCESSING_AI };
static void Selector_postprocessing_ia(Selector *self) { POST_PROCESSING_IA };
static void Selector_postprocessing_aa(Selector *self) { POST_PROCESSING_AA };
static void Selector_postprocessing_ireva(Selector *self) { POST_PROCESSING_IREVA };
static void Selector_postprocessing_areva(Selector *self) { POST_PROCESSING_AREVA };
static void Selector_postprocessing_revai(Selector *self) { POST_PROCESSING_REVAI };
static void Selector_postprocessing_revaa(Selector *self) { POST_PROCESSING_REVAA };
static void Selector_postprocessing_revareva(Selector *self) { POST_PROCESSING_REVAREVA };

static void
Selector_setProcMode(Selector *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            if (self->mode == 0)
                self->proc_func_ptr = Selector_generate_i;
            else
                self->proc_func_ptr = Selector_generate_lin_i;
            break;
        case 1:
            if (self->mode == 0)
                self->proc_func_ptr = Selector_generate_a;
            else
                self->proc_func_ptr = Selector_generate_lin_a;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Selector_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Selector_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Selector_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Selector_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Selector_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Selector_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Selector_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Selector_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Selector_postprocessing_revareva;
            break;
    }
}

static void
Selector_compute_next_data_frame(Selector *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Selector_traverse(Selector *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inputs);
    Py_VISIT(self->voice);
    Py_VISIT(self->voice_stream);
    return 0;
}

static int
Selector_clear(Selector *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inputs);
    Py_CLEAR(self->voice);
    Py_CLEAR(self->voice_stream);
    return 0;
}

static void
Selector_dealloc(Selector* self)
{
    pyo_DEALLOC
    Selector_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Selector_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputstmp=NULL, *voicetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Selector *self;
    self = (Selector *)type->tp_alloc(type, 0);

    self->voice = PyFloat_FromDouble(0.);
    self->mode = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Selector_compute_next_data_frame);
    self->mode_func_ptr = Selector_setProcMode;

    static char *kwlist[] = {"inputs", "voice", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputstmp, &voicetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (inputstmp) {
        PyObject_CallMethod((PyObject *)self, "setInputs", "O", inputstmp);
    }

    if (voicetmp) {
        PyObject_CallMethod((PyObject *)self, "setVoice", "O", voicetmp);
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

static PyObject * Selector_getServer(Selector* self) { GET_SERVER };
static PyObject * Selector_getStream(Selector* self) { GET_STREAM };
static PyObject * Selector_setMul(Selector *self, PyObject *arg) { SET_MUL };
static PyObject * Selector_setAdd(Selector *self, PyObject *arg) { SET_ADD };
static PyObject * Selector_setSub(Selector *self, PyObject *arg) { SET_SUB };
static PyObject * Selector_setDiv(Selector *self, PyObject *arg) { SET_DIV };

static PyObject * Selector_play(Selector *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Selector_out(Selector *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Selector_stop(Selector *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Selector_multiply(Selector *self, PyObject *arg) { MULTIPLY };
static PyObject * Selector_inplace_multiply(Selector *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Selector_add(Selector *self, PyObject *arg) { ADD };
static PyObject * Selector_inplace_add(Selector *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Selector_sub(Selector *self, PyObject *arg) { SUB };
static PyObject * Selector_inplace_sub(Selector *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Selector_div(Selector *self, PyObject *arg) { DIV };
static PyObject * Selector_inplace_div(Selector *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Selector_setInputs(Selector *self, PyObject *arg)
{
	PyObject *tmp;

	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    self->chSize = PyList_Size(tmp);
    Py_INCREF(tmp);
	Py_XDECREF(self->inputs);
    self->inputs = tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Selector_setVoice(Selector *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->voice);
	if (isNumber == 1) {
		self->voice = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->voice = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->voice, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->voice_stream);
        self->voice_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Selector_setMode(Selector *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	if (PyInt_Check(arg) == 1) {
		self->mode = PyLong_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Selector_members[] = {
{"server", T_OBJECT_EX, offsetof(Selector, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Selector, stream), 0, "Stream object."},
{"inputs", T_OBJECT_EX, offsetof(Selector, inputs), 0, "List of input streams."},
{"voice", T_OBJECT_EX, offsetof(Selector, voice), 0, "Voice position pointer."},
{"mul", T_OBJECT_EX, offsetof(Selector, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Selector, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Selector_methods[] = {
{"getServer", (PyCFunction)Selector_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Selector_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Selector_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Selector_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Selector_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setInputs", (PyCFunction)Selector_setInputs, METH_O, "Sets list of input streams."},
{"setVoice", (PyCFunction)Selector_setVoice, METH_O, "Sets voice position pointer."},
{"setMode", (PyCFunction)Selector_setMode, METH_O, "Sets interpolation algorithm."},
{"setMul", (PyCFunction)Selector_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Selector_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Selector_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Selector_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Selector_as_number = {
(binaryfunc)Selector_add,                         /*nb_add*/
(binaryfunc)Selector_sub,                         /*nb_subtract*/
(binaryfunc)Selector_multiply,                    /*nb_multiply*/
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
(binaryfunc)Selector_inplace_add,                 /*inplace_add*/
(binaryfunc)Selector_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Selector_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Selector_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Selector_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject SelectorType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Selector_base",                                   /*tp_name*/
sizeof(Selector),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Selector_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Selector_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Selector objects. Audio interpolation between multiple inputs.",           /* tp_doc */
(traverseproc)Selector_traverse,                  /* tp_traverse */
(inquiry)Selector_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Selector_methods,                                 /* tp_methods */
Selector_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Selector_new,                                     /* tp_new */
};

