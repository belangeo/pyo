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
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

/****************/
/**** Metro *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *time;
    Stream *time_stream;
    int modebuffer[3];
    double sampleToSec;
    double currentTime;
    double offset;
    int flag;
} Metro;

static void
Metro_generate_i(Metro *self) {
    MYFLT val;
    double tm, off;
    int i;

    tm = PyFloat_AS_DOUBLE(self->time);
    off = tm * self->offset;

    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm) {
            val = 0;
            self->currentTime -= tm;
            self->flag = 1;
        }
        else if (self->currentTime >= off && self->flag == 1) {
            val = 1;
            self->flag = 0;
        }
        else
            val = 0;

        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}

static void
Metro_generate_a(Metro *self) {
    MYFLT val;
    double off, tmd;
    int i;

    MYFLT *tm = Stream_getData((Stream *)self->time_stream);

    for (i=0; i<self->bufsize; i++) {
        tmd = (double)tm[i];
        off = tmd * self->offset;
        if (self->currentTime >= tmd) {
            val = 0;
            self->currentTime -= tmd;
            self->flag = 1;
        }
        else if (self->currentTime >= off && self->flag == 1) {
            val = 1;
            self->flag = 0;
        }
        else
            val = 0;

        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}

static void Metro_postprocessing_ii(Metro *self) { POST_PROCESSING_II };
static void Metro_postprocessing_ai(Metro *self) { POST_PROCESSING_AI };
static void Metro_postprocessing_ia(Metro *self) { POST_PROCESSING_IA };
static void Metro_postprocessing_aa(Metro *self) { POST_PROCESSING_AA };
static void Metro_postprocessing_ireva(Metro *self) { POST_PROCESSING_IREVA };
static void Metro_postprocessing_areva(Metro *self) { POST_PROCESSING_AREVA };
static void Metro_postprocessing_revai(Metro *self) { POST_PROCESSING_REVAI };
static void Metro_postprocessing_revaa(Metro *self) { POST_PROCESSING_REVAA };
static void Metro_postprocessing_revareva(Metro *self) { POST_PROCESSING_REVAREVA };

static void
Metro_setProcMode(Metro *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode) {
        case 0:
            self->proc_func_ptr = Metro_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Metro_generate_a;
            break;
    }
    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Metro_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Metro_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Metro_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Metro_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Metro_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Metro_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Metro_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Metro_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Metro_postprocessing_revareva;
            break;
    }
}

static void
Metro_compute_next_data_frame(Metro *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Metro_traverse(Metro *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->time);
    Py_VISIT(self->time_stream);
    return 0;
}

static int
Metro_clear(Metro *self)
{
    pyo_CLEAR
    Py_CLEAR(self->time);
    Py_CLEAR(self->time_stream);
    return 0;
}

static void
Metro_dealloc(Metro* self)
{
    pyo_DEALLOC
    Metro_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Metro_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *timetmp=NULL;
    Metro *self;
    self = (Metro *)type->tp_alloc(type, 0);

    self->time = PyFloat_FromDouble(1.);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    self->flag = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Metro_compute_next_data_frame);
    self->mode_func_ptr = Metro_setProcMode;

    Stream_setStreamActive(self->stream, 0);

    self->sampleToSec = 1.0 / self->sr;
    self->currentTime = 0.;

    static char *kwlist[] = {"time", "offset", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Od", kwlist, &timetmp, &self->offset))
        Py_RETURN_NONE;

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Metro_getServer(Metro* self) { GET_SERVER };
static PyObject * Metro_getStream(Metro* self) { GET_STREAM };
static PyObject * Metro_setMul(Metro *self, PyObject *arg) { SET_MUL };
static PyObject * Metro_setAdd(Metro *self, PyObject *arg) { SET_ADD };
static PyObject * Metro_setSub(Metro *self, PyObject *arg) { SET_SUB };
static PyObject * Metro_setDiv(Metro *self, PyObject *arg) { SET_DIV };

static PyObject * Metro_play(Metro *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Metro_stop(Metro *self) { STOP };

static PyObject * Metro_multiply(Metro *self, PyObject *arg) { MULTIPLY };
static PyObject * Metro_inplace_multiply(Metro *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Metro_add(Metro *self, PyObject *arg) { ADD };
static PyObject * Metro_inplace_add(Metro *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Metro_sub(Metro *self, PyObject *arg) { SUB };
static PyObject * Metro_inplace_sub(Metro *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Metro_div(Metro *self, PyObject *arg) { DIV };
static PyObject * Metro_inplace_div(Metro *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Metro_setTime(Metro *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Metro_members[] = {
{"server", T_OBJECT_EX, offsetof(Metro, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Metro, stream), 0, "Stream object."},
{"time", T_OBJECT_EX, offsetof(Metro, time), 0, "Metro time factor."},
{"mul", T_OBJECT_EX, offsetof(Metro, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Metro, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Metro_methods[] = {
{"getServer", (PyCFunction)Metro_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Metro_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Metro_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Metro_stop, METH_NOARGS, "Stops computing."},
{"setTime", (PyCFunction)Metro_setTime, METH_O, "Sets time factor."},
{"setMul", (PyCFunction)Metro_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Metro_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Metro_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Metro_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Metro_as_number = {
    (binaryfunc)Metro_add,                         /*nb_add*/
    (binaryfunc)Metro_sub,                         /*nb_subtract*/
    (binaryfunc)Metro_multiply,                    /*nb_multiply*/
    (binaryfunc)Metro_div,                                              /*nb_divide*/
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
    (binaryfunc)Metro_inplace_add,                 /*inplace_add*/
    (binaryfunc)Metro_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Metro_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Metro_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject MetroType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Metro_base",         /*tp_name*/
sizeof(Metro),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Metro_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Metro_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Metro objects. Create a metronome.",           /* tp_doc */
(traverseproc)Metro_traverse,   /* tp_traverse */
(inquiry)Metro_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Metro_methods,             /* tp_methods */
Metro_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Metro_new,                 /* tp_new */
};

/****************/
/**** Seqer *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *time;
    Stream *time_stream;
    PyObject *tmp;
    int modebuffer[1];
    double sampleToSec;
    double currentTime;
    double duration;
    int *seq;
    int count;
    MYFLT *buffer_streams;
    int seqsize;
    int poly;
    int flag;
    int tap;
    int voiceCount;
    int newseq;
} Seqer;

static void
Seqer_reset(Seqer *self)
{
    int i;

    self->seqsize = PyList_Size(self->tmp);
    self->seq = (int *)realloc(self->seq, self->seqsize * sizeof(int));
    for (i=0; i<self->seqsize; i++) {
        self->seq[i] = PyInt_AS_LONG(PyNumber_Int(PyList_GET_ITEM(self->tmp, i)));
    }
    self->newseq = 0;
}

static void
Seqer_generate_i(Seqer *self) {
    double tm;
    int i;

    tm = PyFloat_AS_DOUBLE(self->time);

    if (self->currentTime == -1.0) {
        self->currentTime = tm;
    }

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        self->currentTime += self->sampleToSec;
        if (self->currentTime >= tm) {
            self->currentTime -= tm;
            self->count++;
            if (self->count >= self->seq[self->tap]) {
                self->count = 0;
                self->buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
                self->voiceCount++;
                if (self->voiceCount >= self->poly)
                    self->voiceCount = 0;
                self->tap++;
                if (self->tap >= self->seqsize) {
                    self->tap = 0;
                    if (self->newseq == 1)
                        Seqer_reset(self);
                }
            }
        }
    }
}

static void
Seqer_generate_a(Seqer *self) {
    double tm;
    int i;

    MYFLT *time = Stream_getData((Stream *)self->time_stream);

    if (self->currentTime == -1.0) {
        self->currentTime = time[0];
    }

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        tm = (double)time[i];
        self->currentTime += self->sampleToSec;
        if (self->currentTime >= tm) {
            self->currentTime -= tm;
            self->count++;
            if (self->count >= self->seq[self->tap]) {
                self->count = 0;
                self->buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
                self->voiceCount++;
                if (self->voiceCount >= self->poly)
                    self->voiceCount = 0;
                self->tap++;
                if (self->tap >= self->seqsize) {
                    self->tap = 0;
                    if (self->newseq == 1)
                        Seqer_reset(self);
                }
            }
        }
    }
}

MYFLT *
Seqer_getSamplesBuffer(Seqer *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Seqer_setProcMode(Seqer *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:
            self->proc_func_ptr = Seqer_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Seqer_generate_a;
            break;
    }
}

static void
Seqer_compute_next_data_frame(Seqer *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Seqer_traverse(Seqer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->time);
    Py_VISIT(self->time_stream);
    Py_VISIT(self->tmp);
    return 0;
}

static int
Seqer_clear(Seqer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->time);
    Py_CLEAR(self->time_stream);
    Py_CLEAR(self->tmp);
    return 0;
}

static void
Seqer_dealloc(Seqer* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    Seqer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Seqer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *timetmp=NULL, *seqtmp=NULL;
    Seqer *self;
    self = (Seqer *)type->tp_alloc(type, 0);

    self->time = PyFloat_FromDouble(1.);
    self->flag = 1;
    self->poly = 1;
    self->seqsize = 1;
    self->seq = (int *)realloc(self->seq, self->seqsize * sizeof(int));
    self->seq[0] = 1;
    self->newseq = 0;
    self->duration = -1.0;
    self->tap = 0;
    self->count = 0;
    self->voiceCount = 0;
	self->modebuffer[0] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Seqer_compute_next_data_frame);
    self->mode_func_ptr = Seqer_setProcMode;

    Stream_setStreamActive(self->stream, 0);

    self->sampleToSec = 1.0 / self->sr;
    self->currentTime = -1.0;

    static char *kwlist[] = {"time", "seq", "poly", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist, &timetmp, &seqtmp, &self->poly))
        Py_RETURN_NONE;

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    if (seqtmp) {
        PyObject_CallMethod((PyObject *)self, "setSeq", "O", seqtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Seqer_getServer(Seqer* self) { GET_SERVER };
static PyObject * Seqer_getStream(Seqer* self) { GET_STREAM };

static PyObject * Seqer_play(Seqer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Seqer_stop(Seqer *self) { STOP };

static PyObject *
Seqer_setTime(Seqer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Seqer_setSeq(Seqer *self, PyObject *arg)
{
	PyObject *tmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isList = PyList_Check(arg);

	if (isList == 1) {
        tmp = arg;
        Py_INCREF(tmp);
        Py_XDECREF(self->tmp);
        self->tmp = tmp;
        self->newseq = 1;
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Seqer_members[] = {
    {"server", T_OBJECT_EX, offsetof(Seqer, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Seqer, stream), 0, "Stream object."},
    {"time", T_OBJECT_EX, offsetof(Seqer, time), 0, "Seqer time factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Seqer_methods[] = {
    {"getServer", (PyCFunction)Seqer_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Seqer_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Seqer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Seqer_stop, METH_NOARGS, "Stops computing."},
    {"setTime", (PyCFunction)Seqer_setTime, METH_O, "Sets time factor."},
    {"setSeq", (PyCFunction)Seqer_setSeq, METH_O, "Sets duration sequence."},
    {NULL}  /* Sentinel */
};

PyTypeObject SeqerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Seqer_base",         /*tp_name*/
    sizeof(Seqer),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Seqer_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Seqer objects. Creates a metronome with a duration sequence.",           /* tp_doc */
    (traverseproc)Seqer_traverse,   /* tp_traverse */
    (inquiry)Seqer_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Seqer_methods,             /* tp_methods */
    Seqer_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Seqer_new,                 /* tp_new */
};

/************************************************************************************************/
/* Seq streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Seqer *mainPlayer;
    int chnl;
    int modebuffer[2];
} Seq;

static void Seq_postprocessing_ii(Seq *self) { POST_PROCESSING_II };
static void Seq_postprocessing_ai(Seq *self) { POST_PROCESSING_AI };
static void Seq_postprocessing_ia(Seq *self) { POST_PROCESSING_IA };
static void Seq_postprocessing_aa(Seq *self) { POST_PROCESSING_AA };
static void Seq_postprocessing_ireva(Seq *self) { POST_PROCESSING_IREVA };
static void Seq_postprocessing_areva(Seq *self) { POST_PROCESSING_AREVA };
static void Seq_postprocessing_revai(Seq *self) { POST_PROCESSING_REVAI };
static void Seq_postprocessing_revaa(Seq *self) { POST_PROCESSING_REVAA };
static void Seq_postprocessing_revareva(Seq *self) { POST_PROCESSING_REVAREVA };

static void
Seq_setProcMode(Seq *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Seq_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Seq_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Seq_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Seq_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Seq_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Seq_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Seq_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Seq_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Seq_postprocessing_revareva;
            break;
    }
}

static void
Seq_compute_next_data_frame(Seq *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Seqer_getSamplesBuffer((Seqer *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Seq_traverse(Seq *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
Seq_clear(Seq *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
Seq_dealloc(Seq* self)
{
    pyo_DEALLOC
    Seq_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Seq_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    Seq *self;
    self = (Seq *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Seq_compute_next_data_frame);
    self->mode_func_ptr = Seq_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Seqer *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Seq_getServer(Seq* self) { GET_SERVER };
static PyObject * Seq_getStream(Seq* self) { GET_STREAM };
static PyObject * Seq_setMul(Seq *self, PyObject *arg) { SET_MUL };
static PyObject * Seq_setAdd(Seq *self, PyObject *arg) { SET_ADD };
static PyObject * Seq_setSub(Seq *self, PyObject *arg) { SET_SUB };
static PyObject * Seq_setDiv(Seq *self, PyObject *arg) { SET_DIV };

static PyObject * Seq_play(Seq *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Seq_out(Seq *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Seq_stop(Seq *self) { STOP };

static PyObject * Seq_multiply(Seq *self, PyObject *arg) { MULTIPLY };
static PyObject * Seq_inplace_multiply(Seq *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Seq_add(Seq *self, PyObject *arg) { ADD };
static PyObject * Seq_inplace_add(Seq *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Seq_sub(Seq *self, PyObject *arg) { SUB };
static PyObject * Seq_inplace_sub(Seq *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Seq_div(Seq *self, PyObject *arg) { DIV };
static PyObject * Seq_inplace_div(Seq *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Seq_members[] = {
    {"server", T_OBJECT_EX, offsetof(Seq, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Seq, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Seq, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Seq, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Seq_methods[] = {
    {"getServer", (PyCFunction)Seq_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Seq_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Seq_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Seq_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Seq_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)Seq_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Seq_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Seq_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Seq_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Seq_as_number = {
    (binaryfunc)Seq_add,                         /*nb_add*/
    (binaryfunc)Seq_sub,                         /*nb_subtract*/
    (binaryfunc)Seq_multiply,                    /*nb_multiply*/
    (binaryfunc)Seq_div,                                              /*nb_divide*/
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
    (binaryfunc)Seq_inplace_add,                 /*inplace_add*/
    (binaryfunc)Seq_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Seq_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Seq_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject SeqType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Seq_base",         /*tp_name*/
    sizeof(Seq),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Seq_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Seq_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "Seq objects. Reads a channel from a Seqer.",           /* tp_doc */
    (traverseproc)Seq_traverse,   /* tp_traverse */
    (inquiry)Seq_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Seq_methods,             /* tp_methods */
    Seq_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Seq_new,                 /* tp_new */
};

/****************/
/**** Clouder *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *density;
    Stream *density_stream;
    int modebuffer[1];
    int poly;
    int voiceCount;
    MYFLT *buffer_streams;
} Clouder;

static void
Clouder_generate_i(Clouder *self) {
    int i, rnd;

    MYFLT dens = PyFloat_AS_DOUBLE(self->density);
    if (dens <= 0.0)
        dens = 0.0;
    else if (dens > self->sr)
        dens = self->sr;

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    dens *= 0.5;
    for (i=0; i<self->bufsize; i++) {
        rnd = (int)(rand() / (MYFLT)RAND_MAX * self->sr);
        if (rnd < dens) {
            self->buffer_streams[i + self->voiceCount++ * self->bufsize] = 1.0;
            if (self->voiceCount == self->poly)
                self->voiceCount = 0;
        }
    }
}

static void
Clouder_generate_a(Clouder *self) {
    MYFLT dens;
    int i, rnd;

    MYFLT *density = Stream_getData((Stream *)self->density_stream);

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        dens = density[i];
        if (dens <= 0.0)
            dens = 0.0;
        else if (dens > self->sr)
            dens = self->sr;

        dens *= 0.5;
        rnd = (int)(rand() / (MYFLT)RAND_MAX * self->sr);
        if (rnd < dens) {
            self->buffer_streams[i + self->voiceCount++ * self->bufsize] = 1.0;
            if (self->voiceCount == self->poly)
                self->voiceCount = 0;
        }
    }
}

MYFLT *
Clouder_getSamplesBuffer(Clouder *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Clouder_setProcMode(Clouder *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:
            self->proc_func_ptr = Clouder_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Clouder_generate_a;
            break;
    }
}

static void
Clouder_compute_next_data_frame(Clouder *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Clouder_traverse(Clouder *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->density);
    Py_VISIT(self->density_stream);
    return 0;
}

static int
Clouder_clear(Clouder *self)
{
    pyo_CLEAR
    Py_CLEAR(self->density);
    Py_CLEAR(self->density_stream);
    return 0;
}

static void
Clouder_dealloc(Clouder* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    Clouder_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Clouder_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *densitytmp=NULL;
    Clouder *self;
    self = (Clouder *)type->tp_alloc(type, 0);

    self->density = PyFloat_FromDouble(10.0);
    self->poly = 1;
    self->voiceCount = 0;
	self->modebuffer[0] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Clouder_compute_next_data_frame);
    self->mode_func_ptr = Clouder_setProcMode;

    Stream_setStreamActive(self->stream, 0);

    static char *kwlist[] = {"density", "poly", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &densitytmp, &self->poly))
        Py_RETURN_NONE;

    if (densitytmp) {
        PyObject_CallMethod((PyObject *)self, "setDensity", "O", densitytmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    Server_generateSeed((Server *)self->server, CLOUD_ID);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));

    return (PyObject *)self;
}

static PyObject * Clouder_getServer(Clouder* self) { GET_SERVER };
static PyObject * Clouder_getStream(Clouder* self) { GET_STREAM };

static PyObject * Clouder_play(Clouder *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Clouder_stop(Clouder *self) { STOP };

static PyObject *
Clouder_setDensity(Clouder *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->density);
	if (isNumber == 1) {
		self->density = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->density = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->density, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->density_stream);
        self->density_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Clouder_members[] = {
{"server", T_OBJECT_EX, offsetof(Clouder, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Clouder, stream), 0, "Stream object."},
{"density", T_OBJECT_EX, offsetof(Clouder, density), 0, "Clouder density factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Clouder_methods[] = {
{"getServer", (PyCFunction)Clouder_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Clouder_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Clouder_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Clouder_stop, METH_NOARGS, "Stops computing."},
{"setDensity", (PyCFunction)Clouder_setDensity, METH_O, "Sets density factor."},
{NULL}  /* Sentinel */
};

PyTypeObject ClouderType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Clouder_base",         /*tp_name*/
sizeof(Clouder),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Clouder_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Clouder objects. Create a cloud of triggers.",           /* tp_doc */
(traverseproc)Clouder_traverse,   /* tp_traverse */
(inquiry)Clouder_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Clouder_methods,             /* tp_methods */
Clouder_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Clouder_new,                 /* tp_new */
};

/************************************************************************************************/
/* Cloud streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Clouder *mainPlayer;
    int chnl;
    int modebuffer[2];
} Cloud;

static void Cloud_postprocessing_ii(Cloud *self) { POST_PROCESSING_II };
static void Cloud_postprocessing_ai(Cloud *self) { POST_PROCESSING_AI };
static void Cloud_postprocessing_ia(Cloud *self) { POST_PROCESSING_IA };
static void Cloud_postprocessing_aa(Cloud *self) { POST_PROCESSING_AA };
static void Cloud_postprocessing_ireva(Cloud *self) { POST_PROCESSING_IREVA };
static void Cloud_postprocessing_areva(Cloud *self) { POST_PROCESSING_AREVA };
static void Cloud_postprocessing_revai(Cloud *self) { POST_PROCESSING_REVAI };
static void Cloud_postprocessing_revaa(Cloud *self) { POST_PROCESSING_REVAA };
static void Cloud_postprocessing_revareva(Cloud *self) { POST_PROCESSING_REVAREVA };

static void
Cloud_setProcMode(Cloud *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Cloud_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Cloud_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Cloud_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Cloud_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Cloud_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Cloud_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Cloud_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Cloud_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Cloud_postprocessing_revareva;
            break;
    }
}

static void
Cloud_compute_next_data_frame(Cloud *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Clouder_getSamplesBuffer((Clouder *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Cloud_traverse(Cloud *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
Cloud_clear(Cloud *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
Cloud_dealloc(Cloud* self)
{
    pyo_DEALLOC
    Cloud_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Cloud_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    Cloud *self;
    self = (Cloud *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Cloud_compute_next_data_frame);
    self->mode_func_ptr = Cloud_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Clouder *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Cloud_getServer(Cloud* self) { GET_SERVER };
static PyObject * Cloud_getStream(Cloud* self) { GET_STREAM };
static PyObject * Cloud_setMul(Cloud *self, PyObject *arg) { SET_MUL };
static PyObject * Cloud_setAdd(Cloud *self, PyObject *arg) { SET_ADD };
static PyObject * Cloud_setSub(Cloud *self, PyObject *arg) { SET_SUB };
static PyObject * Cloud_setDiv(Cloud *self, PyObject *arg) { SET_DIV };

static PyObject * Cloud_play(Cloud *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Cloud_out(Cloud *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Cloud_stop(Cloud *self) { STOP };

static PyObject * Cloud_multiply(Cloud *self, PyObject *arg) { MULTIPLY };
static PyObject * Cloud_inplace_multiply(Cloud *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Cloud_add(Cloud *self, PyObject *arg) { ADD };
static PyObject * Cloud_inplace_add(Cloud *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Cloud_sub(Cloud *self, PyObject *arg) { SUB };
static PyObject * Cloud_inplace_sub(Cloud *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Cloud_div(Cloud *self, PyObject *arg) { DIV };
static PyObject * Cloud_inplace_div(Cloud *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Cloud_members[] = {
{"server", T_OBJECT_EX, offsetof(Cloud, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Cloud, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Cloud, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Cloud, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Cloud_methods[] = {
{"getServer", (PyCFunction)Cloud_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Cloud_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Cloud_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Cloud_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Cloud_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Cloud_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Cloud_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Cloud_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Cloud_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Cloud_as_number = {
    (binaryfunc)Cloud_add,                         /*nb_add*/
    (binaryfunc)Cloud_sub,                         /*nb_subtract*/
    (binaryfunc)Cloud_multiply,                    /*nb_multiply*/
    (binaryfunc)Cloud_div,                                              /*nb_divide*/
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
    (binaryfunc)Cloud_inplace_add,                 /*inplace_add*/
    (binaryfunc)Cloud_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Cloud_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Cloud_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject CloudType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Cloud_base",         /*tp_name*/
sizeof(Cloud),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Cloud_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Cloud_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Cloud objects. Reads a channel from a Clouder.",           /* tp_doc */
(traverseproc)Cloud_traverse,   /* tp_traverse */
(inquiry)Cloud_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Cloud_methods,             /* tp_methods */
Cloud_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Cloud_new,                 /* tp_new */
};

/****************/
/**** Trig *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    int flag;
    int modebuffer[2];
} Trig;

static void Trig_postprocessing_ii(Trig *self) { POST_PROCESSING_II };
static void Trig_postprocessing_ai(Trig *self) { POST_PROCESSING_AI };
static void Trig_postprocessing_ia(Trig *self) { POST_PROCESSING_IA };
static void Trig_postprocessing_aa(Trig *self) { POST_PROCESSING_AA };
static void Trig_postprocessing_ireva(Trig *self) { POST_PROCESSING_IREVA };
static void Trig_postprocessing_areva(Trig *self) { POST_PROCESSING_AREVA };
static void Trig_postprocessing_revai(Trig *self) { POST_PROCESSING_REVAI };
static void Trig_postprocessing_revaa(Trig *self) { POST_PROCESSING_REVAA };
static void Trig_postprocessing_revareva(Trig *self) { POST_PROCESSING_REVAREVA };

static void
Trig_setProcMode(Trig *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Trig_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Trig_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Trig_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Trig_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Trig_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Trig_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Trig_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Trig_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Trig_postprocessing_revareva;
            break;
    }
}

static void
Trig_compute_next_data_frame(Trig *self)
{
    if (self->flag == 1) {
        self->data[0] = 1.0;
        self->flag = 0;
    }
    else
        self->data[0] = 0.0;
    (*self->muladd_func_ptr)(self);
}

static int
Trig_traverse(Trig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Trig_clear(Trig *self)
{
    pyo_CLEAR
    return 0;
}

static void
Trig_dealloc(Trig* self)
{
    pyo_DEALLOC
    Trig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Trig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Trig *self;
    self = (Trig *)type->tp_alloc(type, 0);

    self->flag = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Trig_compute_next_data_frame);
    self->mode_func_ptr = Trig_setProcMode;

    static char *kwlist[] = {NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Trig_getServer(Trig* self) { GET_SERVER };
static PyObject * Trig_getStream(Trig* self) { GET_STREAM };
static PyObject * Trig_setMul(Trig *self, PyObject *arg) { SET_MUL };
static PyObject * Trig_setAdd(Trig *self, PyObject *arg) { SET_ADD };
static PyObject * Trig_setSub(Trig *self, PyObject *arg) { SET_SUB };
static PyObject * Trig_setDiv(Trig *self, PyObject *arg) { SET_DIV };

static PyObject * Trig_play(Trig *self, PyObject *args, PyObject *kwds)
{
    self->flag = 1;
    PLAY
};

static PyObject * Trig_stop(Trig *self) { STOP };

static PyObject * Trig_multiply(Trig *self, PyObject *arg) { MULTIPLY };
static PyObject * Trig_inplace_multiply(Trig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Trig_add(Trig *self, PyObject *arg) { ADD };
static PyObject * Trig_inplace_add(Trig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Trig_sub(Trig *self, PyObject *arg) { SUB };
static PyObject * Trig_inplace_sub(Trig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Trig_div(Trig *self, PyObject *arg) { DIV };
static PyObject * Trig_inplace_div(Trig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Trig_members[] = {
{"server", T_OBJECT_EX, offsetof(Trig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Trig, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Trig, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Trig, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Trig_methods[] = {
{"getServer", (PyCFunction)Trig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Trig_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Trig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Trig_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Trig_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Trig_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Trig_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Trig_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Trig_as_number = {
    (binaryfunc)Trig_add,                         /*nb_add*/
    (binaryfunc)Trig_sub,                         /*nb_subtract*/
    (binaryfunc)Trig_multiply,                    /*nb_multiply*/
    (binaryfunc)Trig_div,                                              /*nb_divide*/
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
    (binaryfunc)Trig_inplace_add,                 /*inplace_add*/
    (binaryfunc)Trig_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Trig_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Trig_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Trig_base",         /*tp_name*/
sizeof(Trig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Trig_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Trig_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Trig objects. Sends one trig.",           /* tp_doc */
(traverseproc)Trig_traverse,   /* tp_traverse */
(inquiry)Trig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Trig_methods,             /* tp_methods */
Trig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Trig_new,                 /* tp_new */
};

/****************/
/**** Beater *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *time;
    Stream *time_stream;
    int modebuffer[1];
    int poly;
    int voiceCount;
    int taps;
    int last_taps;
    int tapCount;
    int currentTap;
    int weight1;
    int last_weight1;
    int weight2;
    int last_weight2;
    int weight3;
    int last_weight3;
    int newFlag;
    int fillStart;
    int fillEnd;
    int sequence[64];
    int tmp_sequence[64];
    int tapList[64];
    int tapLength;
	int tapProb[64];
    int preset[32][65];
    int preCall;
	MYFLT durations[64];
	MYFLT tmp_durations[64];
	MYFLT accentTable[64];
	MYFLT tmp_accentTable[64];
    MYFLT tapDur;
    double sampleToSec;
    double currentTime;
    MYFLT *buffer_streams;
    MYFLT *tap_buffer_streams;
    MYFLT *amp_buffer_streams;
    MYFLT *dur_buffer_streams;
    MYFLT *end_buffer_streams;
    MYFLT *amplitudes;
} Beater;

static MYFLT
Beater_defineAccent(int n) {
	if (n == 1)
		return (MYFLT)((rand() % 15) + 112) / 127.; // 112 -> 127
	else if (n == 2)
		return (MYFLT)((rand() % 20) + 70) / 127.; // 70 -> 90
	else if (n == 3)
		return (MYFLT)((rand() % 20) + 40) / 127.; // 40 -> 60
    else
        return 0.5;
}

static void
Beater_restore(Beater *self) {
    int i;
    self->fillEnd = 0;
    for (i=0; i<self->taps; i++) {
        self->accentTable[i] = self->tmp_accentTable[i];
        self->sequence[i] = self->tmp_sequence[i];
        self->durations[i] = self->tmp_durations[i];
    }
}

static void
Beater_makeTable(Beater *self, int fill) {
	short i;
	short len;
    int w1, w2, w3;
    if (fill == 0) {
        self->last_taps = self->taps;
        w1 = self->last_weight1 = self->weight1;
        w2 = self->last_weight2 = self->weight2;
        w3 = self->last_weight3 = self->weight3;
        self->newFlag = 0;
    }
    else {
        w1 = self->weight1 + 20;
        w2 = self->weight2 + 20;
        w3 = self->weight3 + 20;
        self->fillStart = 0;
        self->fillEnd = 1;
        for (i=0; i<self->taps; i++) {
            self->tmp_accentTable[i] = self->accentTable[i];
            self->tmp_sequence[i] = self->sequence[i];
            self->tmp_durations[i] = self->durations[i];
        }
    }

	if ((self->taps % 7) == 0) {
		len = 7;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 4  || (i % len) == 2) {
                self->tapProb[i] = w2;
                self->accentTable[i] = Beater_defineAccent(2);
            }
            else if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
	else if ((self->taps % 6) == 0) {
		len = 6;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 3) {
                self->tapProb[i] = w2;
                self->accentTable[i] = Beater_defineAccent(2);
            }
            else if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
	else if ((self->taps % 5) == 0) {
		len = 5;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 3) {
                self->tapProb[i] = w2;
                self->accentTable[i] = Beater_defineAccent(2);
            }
            else if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
	else if ((self->taps % 4) == 0) {
		len = 4;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 2) {
                self->tapProb[i] = w2;
                self->accentTable[i] = Beater_defineAccent(2);
            }
            else if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
	else if ((self->taps % 3) == 0) {
		len = 3;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
	else if ((self->taps % 2) == 0) {
		len = 2;
		for (i=0; i < self->taps; i++) {
            if ((i % len) == 0) {
                self->tapProb[i] = w1;
                self->accentTable[i] = Beater_defineAccent(1);
            }
            else {
                self->tapProb[i] = w3;
                self->accentTable[i] = Beater_defineAccent(3);
            }
		}
	}
}

static void
Beater_makeSequence(Beater *self) {
	short i, j;

	j = 0;
	for (i=0; i < self->taps; i++) {
		if ((rand() % 100) < self->tapProb[i]) {
			self->sequence[i] = 1;
			self->tapList[j++] = i;
		}
		else
			self->sequence[i] = 0;
	}

    self->tapLength = j;
}

static void
Beater_calculateDurations(Beater *self) {
    int i;

    for (i=0; i < (self->tapLength-1); i++) {
		self->durations[self->tapList[i]] = (self->tapList[i+1] - self->tapList[i]) * self->tapDur - 0.005;
	}
	self->durations[self->tapList[self->tapLength-1]] = (self->last_taps - self->tapList[self->tapLength-1] + self->tapList[0]) * self->tapDur - 0.005;
}

static void
Beater_makePresetActive(Beater *self, int n)
{
    int i, j, len;

    self->preCall = -1;
    len = self->preset[n][0];
    if (len != self->taps) {
        self->taps = len;
        Beater_makeTable(self, 0);
    }

    j = 0;
    for (i=0; i<self->taps; i++) {
        self->sequence[i] = self->preset[n][i+1];
        if (self->sequence[i] == 1)
            self->tapList[j++] = i;
    }

    self->tapLength = j;
}

static void
Beater_generate_i(Beater *self) {
    int i, j;
    double tm;

    tm = PyFloat_AS_DOUBLE(self->time);

    self->tapDur = (MYFLT)tm;
    Beater_calculateDurations(self);

    if (self->currentTime == -1.0) {
        self->currentTime = tm;
    }

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        self->tap_buffer_streams[i + self->voiceCount * self->bufsize] = (MYFLT)self->currentTap;
        for (j=0; j<self->poly; j++) {
            self->amp_buffer_streams[i + j * self->bufsize] = self->amplitudes[j];
        }
        self->dur_buffer_streams[i + self->voiceCount * self->bufsize] = self->durations[self->tapCount];
        if (self->currentTime >= tm) {
            self->currentTime -= tm;
            if (self->tapCount == (self->last_taps-2))
                self->end_buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
            if (self->sequence[self->tapCount] == 1) {
                self->currentTap = self->tapCount;
                self->amplitudes[self->voiceCount] = self->accentTable[self->tapCount];
                self->buffer_streams[i + self->voiceCount++ * self->bufsize] = 1.0;
                if (self->voiceCount == self->poly)
                    self->voiceCount = 0;
            }
            self->tapCount++;
            if (self->tapCount >= self->last_taps) {
                self->tapCount = 0;
                if (self->fillStart == 1) {
                    Beater_makeTable(self, 1);
                    Beater_makeSequence(self);
                }
                else if (self->fillEnd == 1 && self->newFlag == 1) {
                    Beater_restore(self);
                    Beater_makeTable(self, 0);
                    Beater_makeSequence(self);
                }
                else if (self->fillEnd == 1 && self->preCall != -1) {
                    Beater_restore(self);
                    Beater_makePresetActive(self, self->preCall);
                }
                else if (self->fillEnd == 1) {
                    Beater_restore(self);
                }
                else if (self->preCall != -1) {
                    Beater_makePresetActive(self, self->preCall);
                }
                else if (self->last_taps != self->taps || self->last_weight1 != self->weight1 ||
                    self->last_weight2 != self->weight2 || self->last_weight3 != self->weight3 ||
                    self->newFlag == 1) {
                    Beater_makeTable(self, 0);
                    Beater_makeSequence(self);
                }
            }
        }
        self->currentTime += self->sampleToSec;
    }
}

static void
Beater_generate_a(Beater *self) {
    double tm;
    int i, j;

    MYFLT *time = Stream_getData((Stream *)self->time_stream);

    self->tapDur = time[0];
    Beater_calculateDurations(self);

    if (self->currentTime == -1.0) {
        self->currentTime = (double)time[0];
    }

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        tm = (double)time[i];
        self->tap_buffer_streams[i + self->voiceCount * self->bufsize] = (MYFLT)self->currentTap;
        for (j=0; j<self->poly; j++) {
            self->amp_buffer_streams[i + j * self->bufsize] = self->amplitudes[j];
        }
        self->dur_buffer_streams[i + self->voiceCount * self->bufsize] = self->durations[self->tapCount];
        if (self->currentTime >= tm) {
            self->currentTime -= tm;
            if (self->tapCount == (self->last_taps-2))
                self->end_buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
            if (self->sequence[self->tapCount] == 1) {
                self->currentTap = self->tapCount;
                self->amplitudes[self->voiceCount] = self->accentTable[self->tapCount];
                self->buffer_streams[i + self->voiceCount++ * self->bufsize] = 1.0;
                if (self->voiceCount == self->poly)
                    self->voiceCount = 0;
            }
            self->tapCount++;
            if (self->tapCount >= self->last_taps) {
                self->tapCount = 0;
                if (self->fillStart == 1) {
                    Beater_makeTable(self, 1);
                    Beater_makeSequence(self);
                }
                else if (self->fillEnd == 1 && self->newFlag == 1) {
                    Beater_restore(self);
                    Beater_makeTable(self, 0);
                    Beater_makeSequence(self);
                }
                else if (self->fillEnd == 1 && self->preCall != -1) {
                    Beater_restore(self);
                    Beater_makePresetActive(self, self->preCall);
                }
                else if (self->fillEnd == 1) {
                    Beater_restore(self);
                }
                else if (self->preCall != -1) {
                    Beater_makePresetActive(self, self->preCall);
                }
                else if (self->last_taps != self->taps || self->last_weight1 != self->weight1 ||
                    self->last_weight2 != self->weight2 || self->last_weight3 != self->weight3 ||
                    self->newFlag == 1) {
                    self->tapDur = time[i];
                    Beater_makeTable(self, 0);
                    Beater_makeSequence(self);
                }
            }
        }
        self->currentTime += self->sampleToSec;
    }
}

MYFLT *
Beater_getSamplesBuffer(Beater *self)
{
    return (MYFLT *)self->buffer_streams;
}

MYFLT *
Beater_getTapBuffer(Beater *self)
{
    return (MYFLT *)self->tap_buffer_streams;
}

MYFLT *
Beater_getAmpBuffer(Beater *self)
{
    return (MYFLT *)self->amp_buffer_streams;
}

MYFLT *
Beater_getDurBuffer(Beater *self)
{
    return (MYFLT *)self->dur_buffer_streams;
}

MYFLT *
Beater_getEndBuffer(Beater *self)
{
    return (MYFLT *)self->end_buffer_streams;
}

static void
Beater_setProcMode(Beater *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:
            self->proc_func_ptr = Beater_generate_i;
            break;
        case 1:
            self->proc_func_ptr = Beater_generate_a;
            break;
    }
}

static void
Beater_compute_next_data_frame(Beater *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Beater_traverse(Beater *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->time);
    Py_VISIT(self->time_stream);
    return 0;
}

static int
Beater_clear(Beater *self)
{
    pyo_CLEAR
    Py_CLEAR(self->time);
    Py_CLEAR(self->time_stream);
    return 0;
}

static void
Beater_dealloc(Beater* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    free(self->tap_buffer_streams);
    free(self->amp_buffer_streams);
    free(self->dur_buffer_streams);
    free(self->end_buffer_streams);
    free(self->amplitudes);
    Beater_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Beater_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *timetmp=NULL;
    Beater *self;
    self = (Beater *)type->tp_alloc(type, 0);

    for (i=0; i<32; i++) {
        for (j=0; j<64; j++) {
            self->preset[i][j] = 0;
        }
    }

    self->preCall = -1;
    self->time = PyFloat_FromDouble(0.125);
    self->tapDur = 0.125;
    self->poly = 1;
    self->voiceCount = 0;
	self->modebuffer[0] = 0;

    self->taps = 16;
    self->tapCount = 0;
    self->currentTap = 0;
    self->weight1 = 80;
    self->weight2 = 50;
    self->weight3 = 30;
    self->last_taps = self->last_weight1 = self->last_weight2 = self->last_weight3 = -1;
    self->newFlag = self->fillStart = self->fillEnd = 0;
    self->tapLength = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Beater_compute_next_data_frame);
    self->mode_func_ptr = Beater_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = -1.0;

    Stream_setStreamActive(self->stream, 0);

    static char *kwlist[] = {"time", "taps", "weight1", "weight2", "weight3", "poly", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oiiiii", kwlist, &timetmp, &self->taps, &self->weight1, &self->weight2, &self->weight3, &self->poly))
        Py_RETURN_NONE;

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    Server_generateSeed((Server *)self->server, BEATER_ID);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->tap_buffer_streams = (MYFLT *)realloc(self->tap_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->amp_buffer_streams = (MYFLT *)realloc(self->amp_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->dur_buffer_streams = (MYFLT *)realloc(self->dur_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->end_buffer_streams = (MYFLT *)realloc(self->end_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->tap_buffer_streams[i] = self->amp_buffer_streams[i] = self->dur_buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }
    self->amplitudes = (MYFLT *)realloc(self->amplitudes, self->poly * sizeof(MYFLT));
    for (i=0; i<self->poly; i++) {
        self->amplitudes[i] = 0.0;
    }

    Beater_makeTable(self, 0);
    Beater_makeSequence(self);

    return (PyObject *)self;
}

static PyObject * Beater_getServer(Beater* self) { GET_SERVER };
static PyObject * Beater_getStream(Beater* self) { GET_STREAM };

static PyObject * Beater_play(Beater *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Beater_stop(Beater *self) { STOP };

static PyObject *
Beater_setTime(Beater *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->tapDur = PyFloat_AS_DOUBLE(self->time);
        self->modebuffer[0] = 0;
	}
	else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
        MYFLT *time = Stream_getData((Stream *)self->time_stream);
        self->tapDur = time[0];
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_reset(Beater *self)
{
    self->voiceCount = 0;
    self->tapCount = 0;
    self->currentTap = 0;
    self->currentTime = -1.0;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_setTaps(Beater *self, PyObject *arg)
{
	if (PyInt_Check(arg))
        self->taps = PyInt_AS_LONG(arg);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_setWeights(Beater *self, PyObject *args, PyObject *kwds)
{
    PyObject *w1=NULL, *w2=NULL, *w3=NULL;

    static char *kwlist[] = {"weight1", "weight2", "weight3", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", kwlist, &w1, &w2, &w3)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

	if (PyInt_Check(w1))
        self->weight1 = PyInt_AS_LONG(w1);

    if (PyInt_Check(w2))
        self->weight2 = PyInt_AS_LONG(w2);

    if (PyInt_Check(w3))
        self->weight3 = PyInt_AS_LONG(w3);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_newPattern(Beater *self)
{
    self->newFlag = 1;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_fillPattern(Beater *self)
{
    self->fillStart = 1;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_storePreset(Beater *self, PyObject *arg)
{
    int i, n;

    if (PyInt_Check(arg)) {
        n = PyInt_AS_LONG(arg);
        self->preset[n][0] = self->taps;
        for (i=0; i<self->taps; i++) {
            self->preset[n][i+1] = self->sequence[i];
        }
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Beater_recallPreset(Beater *self, PyObject *arg)
{
    int x;
    if (PyInt_Check(arg)) {
        x = PyInt_AS_LONG(arg);
        if (x >= 0 && x < 32)
            self->preCall = x;
    }

    if (Stream_getStreamActive(self->stream) == 0)
        Beater_makePresetActive(self, self->preCall);

    Py_INCREF(Py_None);
    return Py_None;

}

static PyObject *
Beater_getPresets(Beater *self) {
    int i, j;
    PyObject *list, *tmp;

    list = PyList_New(0);
    for (i=0; i<32; i++) {
        if (self->preset[i][0] != 0) {
            tmp = PyList_New(0);
            PyList_Append(tmp, PyInt_FromLong(self->preset[i][0]));
            for (j=0; j<self->preset[i][0]; j++) {
                PyList_Append(tmp, PyInt_FromLong(self->preset[i][j+1]));
            }
            PyList_Append(list, tmp);
        }
    }

    return list;
}

static PyObject *
Beater_setPresets(Beater *self, PyObject *arg) {
    int i, j, len, len2;
    PyObject *tmp;

    if (PyList_Check(arg)) {
        len = PyList_Size(arg);
        for (i=0; i<len; i++) {
            tmp = PyList_GetItem(arg, i);
            if (PyList_Check(tmp)) {
                len2 = PyInt_AsLong(PyList_GetItem(tmp, 0));
                self->preset[i][0] = len2;
                for (j=0; j<len2; j++) {
                    self->preset[i][j+1] = PyInt_AsLong(PyList_GetItem(tmp, j+1));
                }
            }
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Beater_members[] = {
    {"server", T_OBJECT_EX, offsetof(Beater, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Beater, stream), 0, "Stream object."},
    {"time", T_OBJECT_EX, offsetof(Beater, time), 0, "Tap duration."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Beater_methods[] = {
    {"getServer", (PyCFunction)Beater_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Beater_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Beater_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Beater_stop, METH_NOARGS, "Stops computing."},
    {"setTime", (PyCFunction)Beater_setTime, METH_O, "Sets tap duration."},
    {"setTaps", (PyCFunction)Beater_setTaps, METH_O, "Sets number of taps in the pattern."},
    {"setWeights", (PyCFunction)Beater_setWeights, METH_VARARGS|METH_KEYWORDS, "Sets probabilities for time accents in the pattern."},
    {"new", (PyCFunction)Beater_newPattern, METH_NOARGS, "Generates a new pattern."},
    {"fill", (PyCFunction)Beater_fillPattern, METH_NOARGS, "Generates a fillin pattern and then restore the current pattern."},
    {"store", (PyCFunction)Beater_storePreset, METH_O, "Store the current pattern in a memory slot."},
    {"recall", (PyCFunction)Beater_recallPreset, METH_O, "Recall a pattern previously stored in a memory slot."},
    {"getPresets", (PyCFunction)Beater_getPresets, METH_NOARGS, "Returns the list of stored presets."},
    {"setPresets", (PyCFunction)Beater_setPresets, METH_O, "Store a list of presets."},
    {"reset", (PyCFunction)Beater_reset, METH_NOARGS, "Resets counters to 0."},
    {NULL}  /* Sentinel */
};

PyTypeObject BeaterType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Beater_base",         /*tp_name*/
    sizeof(Beater),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Beater_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Beater objects. Create an algorithmic beat sequence.",           /* tp_doc */
    (traverseproc)Beater_traverse,   /* tp_traverse */
    (inquiry)Beater_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Beater_methods,             /* tp_methods */
    Beater_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Beater_new,                 /* tp_new */
};

/************************************************************************************************/
/* Beat streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Beater *mainPlayer;
    int chnl;
    int modebuffer[2];
} Beat;

static void Beat_postprocessing_ii(Beat *self) { POST_PROCESSING_II };
static void Beat_postprocessing_ai(Beat *self) { POST_PROCESSING_AI };
static void Beat_postprocessing_ia(Beat *self) { POST_PROCESSING_IA };
static void Beat_postprocessing_aa(Beat *self) { POST_PROCESSING_AA };
static void Beat_postprocessing_ireva(Beat *self) { POST_PROCESSING_IREVA };
static void Beat_postprocessing_areva(Beat *self) { POST_PROCESSING_AREVA };
static void Beat_postprocessing_revai(Beat *self) { POST_PROCESSING_REVAI };
static void Beat_postprocessing_revaa(Beat *self) { POST_PROCESSING_REVAA };
static void Beat_postprocessing_revareva(Beat *self) { POST_PROCESSING_REVAREVA };

static void
Beat_setProcMode(Beat *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Beat_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Beat_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Beat_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Beat_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Beat_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Beat_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Beat_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Beat_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Beat_postprocessing_revareva;
            break;
    }
}

static void
Beat_compute_next_data_frame(Beat *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Beater_getSamplesBuffer((Beater *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Beat_traverse(Beat *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
Beat_clear(Beat *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
Beat_dealloc(Beat* self)
{
    pyo_DEALLOC
    Beat_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Beat_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    Beat *self;
    self = (Beat *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Beat_compute_next_data_frame);
    self->mode_func_ptr = Beat_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Beater *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Beat_getServer(Beat* self) { GET_SERVER };
static PyObject * Beat_getStream(Beat* self) { GET_STREAM };
static PyObject * Beat_setMul(Beat *self, PyObject *arg) { SET_MUL };
static PyObject * Beat_setAdd(Beat *self, PyObject *arg) { SET_ADD };
static PyObject * Beat_setSub(Beat *self, PyObject *arg) { SET_SUB };
static PyObject * Beat_setDiv(Beat *self, PyObject *arg) { SET_DIV };

static PyObject * Beat_play(Beat *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Beat_out(Beat *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Beat_stop(Beat *self) { STOP };

static PyObject * Beat_multiply(Beat *self, PyObject *arg) { MULTIPLY };
static PyObject * Beat_inplace_multiply(Beat *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Beat_add(Beat *self, PyObject *arg) { ADD };
static PyObject * Beat_inplace_add(Beat *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Beat_sub(Beat *self, PyObject *arg) { SUB };
static PyObject * Beat_inplace_sub(Beat *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Beat_div(Beat *self, PyObject *arg) { DIV };
static PyObject * Beat_inplace_div(Beat *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Beat_members[] = {
    {"server", T_OBJECT_EX, offsetof(Beat, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Beat, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Beat, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Beat, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Beat_methods[] = {
    {"getServer", (PyCFunction)Beat_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Beat_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Beat_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Beat_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Beat_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)Beat_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Beat_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Beat_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Beat_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Beat_as_number = {
    (binaryfunc)Beat_add,                         /*nb_add*/
    (binaryfunc)Beat_sub,                         /*nb_subtract*/
    (binaryfunc)Beat_multiply,                    /*nb_multiply*/
    (binaryfunc)Beat_div,                                              /*nb_divide*/
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
    (binaryfunc)Beat_inplace_add,                 /*inplace_add*/
    (binaryfunc)Beat_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Beat_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Beat_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BeatType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Beat_base",         /*tp_name*/
    sizeof(Beat),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Beat_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Beat_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "Beat objects. Reads a channel from a Beater.",           /* tp_doc */
    (traverseproc)Beat_traverse,   /* tp_traverse */
    (inquiry)Beat_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Beat_methods,             /* tp_methods */
    Beat_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Beat_new,                 /* tp_new */
};

/************************************************************************************************/
/* BeatTapStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Beater *mainPlayer;
    int chnl;
    int modebuffer[2];
} BeatTapStream;

static void BeatTapStream_postprocessing_ii(BeatTapStream *self) { POST_PROCESSING_II };
static void BeatTapStream_postprocessing_ai(BeatTapStream *self) { POST_PROCESSING_AI };
static void BeatTapStream_postprocessing_ia(BeatTapStream *self) { POST_PROCESSING_IA };
static void BeatTapStream_postprocessing_aa(BeatTapStream *self) { POST_PROCESSING_AA };
static void BeatTapStream_postprocessing_ireva(BeatTapStream *self) { POST_PROCESSING_IREVA };
static void BeatTapStream_postprocessing_areva(BeatTapStream *self) { POST_PROCESSING_AREVA };
static void BeatTapStream_postprocessing_revai(BeatTapStream *self) { POST_PROCESSING_REVAI };
static void BeatTapStream_postprocessing_revaa(BeatTapStream *self) { POST_PROCESSING_REVAA };
static void BeatTapStream_postprocessing_revareva(BeatTapStream *self) { POST_PROCESSING_REVAREVA };

static void
BeatTapStream_setProcMode(BeatTapStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BeatTapStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BeatTapStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BeatTapStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BeatTapStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BeatTapStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BeatTapStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BeatTapStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BeatTapStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BeatTapStream_postprocessing_revareva;
            break;
    }
}

static void
BeatTapStream_compute_next_data_frame(BeatTapStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Beater_getTapBuffer((Beater *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
BeatTapStream_traverse(BeatTapStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
BeatTapStream_clear(BeatTapStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
BeatTapStream_dealloc(BeatTapStream* self)
{
    pyo_DEALLOC
    BeatTapStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BeatTapStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    BeatTapStream *self;
    self = (BeatTapStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BeatTapStream_compute_next_data_frame);
    self->mode_func_ptr = BeatTapStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Beater *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * BeatTapStream_getServer(BeatTapStream* self) { GET_SERVER };
static PyObject * BeatTapStream_getStream(BeatTapStream* self) { GET_STREAM };
static PyObject * BeatTapStream_setMul(BeatTapStream *self, PyObject *arg) { SET_MUL };
static PyObject * BeatTapStream_setAdd(BeatTapStream *self, PyObject *arg) { SET_ADD };
static PyObject * BeatTapStream_setSub(BeatTapStream *self, PyObject *arg) { SET_SUB };
static PyObject * BeatTapStream_setDiv(BeatTapStream *self, PyObject *arg) { SET_DIV };

static PyObject * BeatTapStream_play(BeatTapStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BeatTapStream_out(BeatTapStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BeatTapStream_stop(BeatTapStream *self) { STOP };

static PyObject * BeatTapStream_multiply(BeatTapStream *self, PyObject *arg) { MULTIPLY };
static PyObject * BeatTapStream_inplace_multiply(BeatTapStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BeatTapStream_add(BeatTapStream *self, PyObject *arg) { ADD };
static PyObject * BeatTapStream_inplace_add(BeatTapStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BeatTapStream_sub(BeatTapStream *self, PyObject *arg) { SUB };
static PyObject * BeatTapStream_inplace_sub(BeatTapStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BeatTapStream_div(BeatTapStream *self, PyObject *arg) { DIV };
static PyObject * BeatTapStream_inplace_div(BeatTapStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BeatTapStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(BeatTapStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(BeatTapStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(BeatTapStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(BeatTapStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef BeatTapStream_methods[] = {
    {"getServer", (PyCFunction)BeatTapStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)BeatTapStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)BeatTapStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)BeatTapStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)BeatTapStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)BeatTapStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)BeatTapStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)BeatTapStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)BeatTapStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods BeatTapStream_as_number = {
    (binaryfunc)BeatTapStream_add,                         /*nb_add*/
    (binaryfunc)BeatTapStream_sub,                         /*nb_subtract*/
    (binaryfunc)BeatTapStream_multiply,                    /*nb_multiply*/
    (binaryfunc)BeatTapStream_div,                                              /*nb_divide*/
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
    (binaryfunc)BeatTapStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)BeatTapStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)BeatTapStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)BeatTapStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BeatTapStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.BeatTapStream_base",         /*tp_name*/
    sizeof(BeatTapStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)BeatTapStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &BeatTapStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "BeatTapStream objects. Reads the current tap from a Beater object.",           /* tp_doc */
    (traverseproc)BeatTapStream_traverse,   /* tp_traverse */
    (inquiry)BeatTapStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    BeatTapStream_methods,             /* tp_methods */
    BeatTapStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    BeatTapStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* BeatAmpStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Beater *mainPlayer;
    int chnl;
    int modebuffer[2];
} BeatAmpStream;

static void BeatAmpStream_postprocessing_ii(BeatAmpStream *self) { POST_PROCESSING_II };
static void BeatAmpStream_postprocessing_ai(BeatAmpStream *self) { POST_PROCESSING_AI };
static void BeatAmpStream_postprocessing_ia(BeatAmpStream *self) { POST_PROCESSING_IA };
static void BeatAmpStream_postprocessing_aa(BeatAmpStream *self) { POST_PROCESSING_AA };
static void BeatAmpStream_postprocessing_ireva(BeatAmpStream *self) { POST_PROCESSING_IREVA };
static void BeatAmpStream_postprocessing_areva(BeatAmpStream *self) { POST_PROCESSING_AREVA };
static void BeatAmpStream_postprocessing_revai(BeatAmpStream *self) { POST_PROCESSING_REVAI };
static void BeatAmpStream_postprocessing_revaa(BeatAmpStream *self) { POST_PROCESSING_REVAA };
static void BeatAmpStream_postprocessing_revareva(BeatAmpStream *self) { POST_PROCESSING_REVAREVA };

static void
BeatAmpStream_setProcMode(BeatAmpStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BeatAmpStream_postprocessing_revareva;
            break;
    }
}

static void
BeatAmpStream_compute_next_data_frame(BeatAmpStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Beater_getAmpBuffer((Beater *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
BeatAmpStream_traverse(BeatAmpStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
BeatAmpStream_clear(BeatAmpStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
BeatAmpStream_dealloc(BeatAmpStream* self)
{
    pyo_DEALLOC
    BeatAmpStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BeatAmpStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    BeatAmpStream *self;
    self = (BeatAmpStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BeatAmpStream_compute_next_data_frame);
    self->mode_func_ptr = BeatAmpStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Beater *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * BeatAmpStream_getServer(BeatAmpStream* self) { GET_SERVER };
static PyObject * BeatAmpStream_getStream(BeatAmpStream* self) { GET_STREAM };
static PyObject * BeatAmpStream_setMul(BeatAmpStream *self, PyObject *arg) { SET_MUL };
static PyObject * BeatAmpStream_setAdd(BeatAmpStream *self, PyObject *arg) { SET_ADD };
static PyObject * BeatAmpStream_setSub(BeatAmpStream *self, PyObject *arg) { SET_SUB };
static PyObject * BeatAmpStream_setDiv(BeatAmpStream *self, PyObject *arg) { SET_DIV };

static PyObject * BeatAmpStream_play(BeatAmpStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BeatAmpStream_out(BeatAmpStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BeatAmpStream_stop(BeatAmpStream *self) { STOP };

static PyObject * BeatAmpStream_multiply(BeatAmpStream *self, PyObject *arg) { MULTIPLY };
static PyObject * BeatAmpStream_inplace_multiply(BeatAmpStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BeatAmpStream_add(BeatAmpStream *self, PyObject *arg) { ADD };
static PyObject * BeatAmpStream_inplace_add(BeatAmpStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BeatAmpStream_sub(BeatAmpStream *self, PyObject *arg) { SUB };
static PyObject * BeatAmpStream_inplace_sub(BeatAmpStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BeatAmpStream_div(BeatAmpStream *self, PyObject *arg) { DIV };
static PyObject * BeatAmpStream_inplace_div(BeatAmpStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BeatAmpStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(BeatAmpStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(BeatAmpStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(BeatAmpStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(BeatAmpStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef BeatAmpStream_methods[] = {
    {"getServer", (PyCFunction)BeatAmpStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)BeatAmpStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)BeatAmpStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)BeatAmpStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)BeatAmpStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)BeatAmpStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)BeatAmpStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)BeatAmpStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)BeatAmpStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods BeatAmpStream_as_number = {
    (binaryfunc)BeatAmpStream_add,                         /*nb_add*/
    (binaryfunc)BeatAmpStream_sub,                         /*nb_subtract*/
    (binaryfunc)BeatAmpStream_multiply,                    /*nb_multiply*/
    (binaryfunc)BeatAmpStream_div,                                              /*nb_divide*/
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
    (binaryfunc)BeatAmpStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)BeatAmpStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)BeatAmpStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)BeatAmpStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BeatAmpStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.BeatAmpStream_base",         /*tp_name*/
    sizeof(BeatAmpStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)BeatAmpStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &BeatAmpStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "BeatAmpStream objects. Reads a amplitude channel from a Beater object.",           /* tp_doc */
    (traverseproc)BeatAmpStream_traverse,   /* tp_traverse */
    (inquiry)BeatAmpStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    BeatAmpStream_methods,             /* tp_methods */
    BeatAmpStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    BeatAmpStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* BeatDurStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Beater *mainPlayer;
    int chnl;
    int modebuffer[2];
} BeatDurStream;

static void BeatDurStream_postprocessing_ii(BeatDurStream *self) { POST_PROCESSING_II };
static void BeatDurStream_postprocessing_ai(BeatDurStream *self) { POST_PROCESSING_AI };
static void BeatDurStream_postprocessing_ia(BeatDurStream *self) { POST_PROCESSING_IA };
static void BeatDurStream_postprocessing_aa(BeatDurStream *self) { POST_PROCESSING_AA };
static void BeatDurStream_postprocessing_ireva(BeatDurStream *self) { POST_PROCESSING_IREVA };
static void BeatDurStream_postprocessing_areva(BeatDurStream *self) { POST_PROCESSING_AREVA };
static void BeatDurStream_postprocessing_revai(BeatDurStream *self) { POST_PROCESSING_REVAI };
static void BeatDurStream_postprocessing_revaa(BeatDurStream *self) { POST_PROCESSING_REVAA };
static void BeatDurStream_postprocessing_revareva(BeatDurStream *self) { POST_PROCESSING_REVAREVA };

static void
BeatDurStream_setProcMode(BeatDurStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BeatDurStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BeatDurStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BeatDurStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BeatDurStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BeatDurStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BeatDurStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BeatDurStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BeatDurStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BeatDurStream_postprocessing_revareva;
            break;
    }
}

static void
BeatDurStream_compute_next_data_frame(BeatDurStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Beater_getDurBuffer((Beater *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
BeatDurStream_traverse(BeatDurStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
BeatDurStream_clear(BeatDurStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
BeatDurStream_dealloc(BeatDurStream* self)
{
    pyo_DEALLOC
    BeatDurStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BeatDurStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    BeatDurStream *self;
    self = (BeatDurStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BeatDurStream_compute_next_data_frame);
    self->mode_func_ptr = BeatDurStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Beater *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * BeatDurStream_getServer(BeatDurStream* self) { GET_SERVER };
static PyObject * BeatDurStream_getStream(BeatDurStream* self) { GET_STREAM };
static PyObject * BeatDurStream_setMul(BeatDurStream *self, PyObject *arg) { SET_MUL };
static PyObject * BeatDurStream_setAdd(BeatDurStream *self, PyObject *arg) { SET_ADD };
static PyObject * BeatDurStream_setSub(BeatDurStream *self, PyObject *arg) { SET_SUB };
static PyObject * BeatDurStream_setDiv(BeatDurStream *self, PyObject *arg) { SET_DIV };

static PyObject * BeatDurStream_play(BeatDurStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BeatDurStream_out(BeatDurStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BeatDurStream_stop(BeatDurStream *self) { STOP };

static PyObject * BeatDurStream_multiply(BeatDurStream *self, PyObject *arg) { MULTIPLY };
static PyObject * BeatDurStream_inplace_multiply(BeatDurStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BeatDurStream_add(BeatDurStream *self, PyObject *arg) { ADD };
static PyObject * BeatDurStream_inplace_add(BeatDurStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BeatDurStream_sub(BeatDurStream *self, PyObject *arg) { SUB };
static PyObject * BeatDurStream_inplace_sub(BeatDurStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BeatDurStream_div(BeatDurStream *self, PyObject *arg) { DIV };
static PyObject * BeatDurStream_inplace_div(BeatDurStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BeatDurStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(BeatDurStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(BeatDurStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(BeatDurStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(BeatDurStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef BeatDurStream_methods[] = {
    {"getServer", (PyCFunction)BeatDurStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)BeatDurStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)BeatDurStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)BeatDurStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)BeatDurStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)BeatDurStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)BeatDurStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)BeatDurStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)BeatDurStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods BeatDurStream_as_number = {
    (binaryfunc)BeatDurStream_add,                         /*nb_add*/
    (binaryfunc)BeatDurStream_sub,                         /*nb_subtract*/
    (binaryfunc)BeatDurStream_multiply,                    /*nb_multiply*/
    (binaryfunc)BeatDurStream_div,                                              /*nb_divide*/
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
    (binaryfunc)BeatDurStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)BeatDurStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)BeatDurStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)BeatDurStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BeatDurStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.BeatDurStream_base",         /*tp_name*/
    sizeof(BeatDurStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)BeatDurStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &BeatDurStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "BeatDurStream objects. Reads a duration channel from a Beater object.",           /* tp_doc */
    (traverseproc)BeatDurStream_traverse,   /* tp_traverse */
    (inquiry)BeatDurStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    BeatDurStream_methods,             /* tp_methods */
    BeatDurStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    BeatDurStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* BeatEndStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Beater *mainPlayer;
    int chnl;
    int modebuffer[2];
} BeatEndStream;

static void BeatEndStream_postprocessing_ii(BeatEndStream *self) { POST_PROCESSING_II };
static void BeatEndStream_postprocessing_ai(BeatEndStream *self) { POST_PROCESSING_AI };
static void BeatEndStream_postprocessing_ia(BeatEndStream *self) { POST_PROCESSING_IA };
static void BeatEndStream_postprocessing_aa(BeatEndStream *self) { POST_PROCESSING_AA };
static void BeatEndStream_postprocessing_ireva(BeatEndStream *self) { POST_PROCESSING_IREVA };
static void BeatEndStream_postprocessing_areva(BeatEndStream *self) { POST_PROCESSING_AREVA };
static void BeatEndStream_postprocessing_revai(BeatEndStream *self) { POST_PROCESSING_REVAI };
static void BeatEndStream_postprocessing_revaa(BeatEndStream *self) { POST_PROCESSING_REVAA };
static void BeatEndStream_postprocessing_revareva(BeatEndStream *self) { POST_PROCESSING_REVAREVA };

static void
BeatEndStream_setProcMode(BeatEndStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BeatEndStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BeatEndStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BeatEndStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BeatEndStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BeatEndStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BeatEndStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BeatEndStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BeatEndStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BeatEndStream_postprocessing_revareva;
            break;
    }
}

static void
BeatEndStream_compute_next_data_frame(BeatEndStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Beater_getEndBuffer((Beater *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
BeatEndStream_traverse(BeatEndStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
BeatEndStream_clear(BeatEndStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
BeatEndStream_dealloc(BeatEndStream* self)
{
    pyo_DEALLOC
    BeatEndStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
BeatEndStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    BeatEndStream *self;
    self = (BeatEndStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BeatEndStream_compute_next_data_frame);
    self->mode_func_ptr = BeatEndStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (Beater *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * BeatEndStream_getServer(BeatEndStream* self) { GET_SERVER };
static PyObject * BeatEndStream_getStream(BeatEndStream* self) { GET_STREAM };
static PyObject * BeatEndStream_setMul(BeatEndStream *self, PyObject *arg) { SET_MUL };
static PyObject * BeatEndStream_setAdd(BeatEndStream *self, PyObject *arg) { SET_ADD };
static PyObject * BeatEndStream_setSub(BeatEndStream *self, PyObject *arg) { SET_SUB };
static PyObject * BeatEndStream_setDiv(BeatEndStream *self, PyObject *arg) { SET_DIV };

static PyObject * BeatEndStream_play(BeatEndStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BeatEndStream_out(BeatEndStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BeatEndStream_stop(BeatEndStream *self) { STOP };

static PyObject * BeatEndStream_multiply(BeatEndStream *self, PyObject *arg) { MULTIPLY };
static PyObject * BeatEndStream_inplace_multiply(BeatEndStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BeatEndStream_add(BeatEndStream *self, PyObject *arg) { ADD };
static PyObject * BeatEndStream_inplace_add(BeatEndStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BeatEndStream_sub(BeatEndStream *self, PyObject *arg) { SUB };
static PyObject * BeatEndStream_inplace_sub(BeatEndStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BeatEndStream_div(BeatEndStream *self, PyObject *arg) { DIV };
static PyObject * BeatEndStream_inplace_div(BeatEndStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BeatEndStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(BeatEndStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(BeatEndStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(BeatEndStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(BeatEndStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef BeatEndStream_methods[] = {
    {"getServer", (PyCFunction)BeatEndStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)BeatEndStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)BeatEndStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)BeatEndStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)BeatEndStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)BeatEndStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)BeatEndStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)BeatEndStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)BeatEndStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods BeatEndStream_as_number = {
    (binaryfunc)BeatEndStream_add,                         /*nb_add*/
    (binaryfunc)BeatEndStream_sub,                         /*nb_subtract*/
    (binaryfunc)BeatEndStream_multiply,                    /*nb_multiply*/
    (binaryfunc)BeatEndStream_div,                                              /*nb_divide*/
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
    (binaryfunc)BeatEndStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)BeatEndStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)BeatEndStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)BeatEndStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BeatEndStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.BeatEndStream_base",         /*tp_name*/
    sizeof(BeatEndStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)BeatEndStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &BeatEndStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "BeatEndStream objects. Reads a duration channel from a Beater object.",           /* tp_doc */
    (traverseproc)BeatEndStream_traverse,   /* tp_traverse */
    (inquiry)BeatEndStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    BeatEndStream_methods,             /* tp_methods */
    BeatEndStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    BeatEndStream_new,                 /* tp_new */
};

/****************/
/**** TrigBurster *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[1];
    int poly;
    int voiceCount;
    MYFLT time;
    MYFLT a_time;
    int count;
    int a_count;
    MYFLT expand;
    MYFLT a_expand;
    MYFLT ampfade;
    MYFLT a_ampfade;
    int flag;
    double sampleToSec;
    double currentTime;
    double targetTime;
    int currentCount;
    int *currentTap;
    MYFLT *currentAmp;
    MYFLT *currentDur;
    MYFLT *buffer_streams;
    MYFLT *tap_buffer_streams;
    MYFLT *amp_buffer_streams;
    MYFLT *dur_buffer_streams;
    MYFLT *end_buffer_streams;
} TrigBurster;

static void
TrigBurster_generate_i(TrigBurster *self) {
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }

    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1.0) {
            self->flag = 1;
            self->currentCount = 0;
            self->a_time = self->targetTime = self->currentTime = self->time;
            self->a_count = self->count;
            self->a_expand = self->expand;
            self->a_ampfade = self->ampfade;
        }
        if (self->flag == 1) {
            if (self->currentTime >= self->targetTime) {
                self->currentTime -= self->targetTime;
                self->targetTime = self->a_time * MYPOW(self->a_expand, self->currentCount);
                self->currentTap[self->voiceCount] = self->currentCount;
                self->currentAmp[self->voiceCount] = MYPOW(self->a_ampfade, self->currentCount);
                self->currentDur[self->voiceCount] = self->targetTime;
                self->buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
                self->currentCount++;
                if (self->currentCount == (self->a_count - 1))
                    self->end_buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
                if (self->currentCount == self->a_count)
                    self->flag = 0;
                self->voiceCount++;
                if (self->voiceCount == self->poly)
                    self->voiceCount = 0;
            }
            self->currentTime += self->sampleToSec;
        }
        for (j=0; j<self->poly; j++) {
            self->tap_buffer_streams[i + j * self->bufsize] = (MYFLT)self->currentTap[j];
            self->amp_buffer_streams[i + j * self->bufsize] = self->currentAmp[j];
            self->dur_buffer_streams[i + j * self->bufsize] = self->currentDur[j];
        }
    }
}

MYFLT *
TrigBurster_getSamplesBuffer(TrigBurster *self)
{
    return (MYFLT *)self->buffer_streams;
}

MYFLT *
TrigBurster_getTapBuffer(TrigBurster *self)
{
    return (MYFLT *)self->tap_buffer_streams;
}

MYFLT *
TrigBurster_getAmpBuffer(TrigBurster *self)
{
    return (MYFLT *)self->amp_buffer_streams;
}

MYFLT *
TrigBurster_getDurBuffer(TrigBurster *self)
{
    return (MYFLT *)self->dur_buffer_streams;
}

MYFLT *
TrigBurster_getEndBuffer(TrigBurster *self)
{
    return (MYFLT *)self->end_buffer_streams;
}

static void
TrigBurster_setProcMode(TrigBurster *self)
{
    self->proc_func_ptr = TrigBurster_generate_i;
}

static void
TrigBurster_compute_next_data_frame(TrigBurster *self)
{
    (*self->proc_func_ptr)(self);
}

static int
TrigBurster_traverse(TrigBurster *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
TrigBurster_clear(TrigBurster *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
TrigBurster_dealloc(TrigBurster* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    free(self->tap_buffer_streams);
    free(self->amp_buffer_streams);
    free(self->dur_buffer_streams);
    free(self->end_buffer_streams);
    free(self->currentTap);
    free(self->currentAmp);
    free(self->currentDur);
    TrigBurster_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurster_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    TrigBurster *self;
    self = (TrigBurster *)type->tp_alloc(type, 0);


    self->poly = 1;
    self->voiceCount = 0;
    self->time = self->a_time = 0.25;
    self->count = self->a_count = 10;
    self->expand = self->a_expand = 1.0;
    self->ampfade = self->a_ampfade = 1.0;
    self->currentCount = 0;
    self->targetTime = 0.0;
    self->currentTime = -1.0;
    self->flag = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurster_compute_next_data_frame);
    self->mode_func_ptr = TrigBurster_setProcMode;

    self->sampleToSec = 1. / self->sr;

    Stream_setStreamActive(self->stream, 1);

    static char *kwlist[] = {"input", "time", "count", "expand", "ampfade", "poly", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FIFFI, kwlist, &inputtmp, &self->time, &self->count, &self->expand, &self->ampfade, &self->poly))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->tap_buffer_streams = (MYFLT *)realloc(self->tap_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->amp_buffer_streams = (MYFLT *)realloc(self->amp_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->dur_buffer_streams = (MYFLT *)realloc(self->dur_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->end_buffer_streams = (MYFLT *)realloc(self->end_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->tap_buffer_streams[i] = self->amp_buffer_streams[i] = self->dur_buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }

    self->currentTap = (int *)realloc(self->currentTap, self->poly * sizeof(int));
    self->currentAmp = (MYFLT *)realloc(self->currentAmp, self->poly * sizeof(MYFLT));
    self->currentDur = (MYFLT *)realloc(self->currentDur, self->poly * sizeof(MYFLT));
    for (i=0; i<(self->poly); i++) {
        self->currentTap[i] = 0;
        self->currentAmp[i] = self->currentDur[i] = 0.0;
    }

    return (PyObject *)self;
}

static PyObject * TrigBurster_getServer(TrigBurster* self) { GET_SERVER };
static PyObject * TrigBurster_getStream(TrigBurster* self) { GET_STREAM };

static PyObject * TrigBurster_play(TrigBurster *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurster_stop(TrigBurster *self) { STOP };

static PyObject *
TrigBurster_setTime(TrigBurster *self, PyObject *arg)
{
	if (PyNumber_Check(arg))
        self->time = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
        if (self->time <= 0.01)
            self->time = 0.01;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
TrigBurster_setCount(TrigBurster *self, PyObject *arg)
{
	if (PyInt_Check(arg))
        self->count = PyInt_AS_LONG(arg);
        if (self->count < 1)
            self->count = 1;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
TrigBurster_setExpand(TrigBurster *self, PyObject *arg)
{
	if (PyNumber_Check(arg))
        self->expand = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
        if (self->expand <= 0.1)
            self->expand = 0.1;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
TrigBurster_setAmpfade(TrigBurster *self, PyObject *arg)
{
	if (PyNumber_Check(arg))
        self->ampfade = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
        if (self->ampfade <= 0.1)
            self->ampfade = 0.1;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef TrigBurster_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurster, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurster, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigBurster, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurster_methods[] = {
    {"getServer", (PyCFunction)TrigBurster_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurster_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurster_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigBurster_stop, METH_NOARGS, "Stops computing."},
    {"setTime", (PyCFunction)TrigBurster_setTime, METH_O, "Sets the base time of the serie."},
    {"setCount", (PyCFunction)TrigBurster_setCount, METH_O, "Sets the number of trigs in the serie."},
    {"setExpand", (PyCFunction)TrigBurster_setExpand, METH_O, "Sets the time's expansion factor of the serie."},
    {"setAmpfade", (PyCFunction)TrigBurster_setAmpfade, METH_O, "Sets the amplitude's expansion factor of the serie."},
    {NULL}  /* Sentinel */
};

PyTypeObject TrigBursterType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurster_base",         /*tp_name*/
    sizeof(TrigBurster),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurster_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "TrigBurster objects. Create an algorithmic beat sequence.",           /* tp_doc */
    (traverseproc)TrigBurster_traverse,   /* tp_traverse */
    (inquiry)TrigBurster_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurster_methods,             /* tp_methods */
    TrigBurster_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurster_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigBurst streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigBurster *mainPlayer;
    int chnl;
    int modebuffer[2];
} TrigBurst;

static void TrigBurst_postprocessing_ii(TrigBurst *self) { POST_PROCESSING_II };
static void TrigBurst_postprocessing_ai(TrigBurst *self) { POST_PROCESSING_AI };
static void TrigBurst_postprocessing_ia(TrigBurst *self) { POST_PROCESSING_IA };
static void TrigBurst_postprocessing_aa(TrigBurst *self) { POST_PROCESSING_AA };
static void TrigBurst_postprocessing_ireva(TrigBurst *self) { POST_PROCESSING_IREVA };
static void TrigBurst_postprocessing_areva(TrigBurst *self) { POST_PROCESSING_AREVA };
static void TrigBurst_postprocessing_revai(TrigBurst *self) { POST_PROCESSING_REVAI };
static void TrigBurst_postprocessing_revaa(TrigBurst *self) { POST_PROCESSING_REVAA };
static void TrigBurst_postprocessing_revareva(TrigBurst *self) { POST_PROCESSING_REVAREVA };

static void
TrigBurst_setProcMode(TrigBurst *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigBurst_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigBurst_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigBurst_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigBurst_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigBurst_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigBurst_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigBurst_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigBurst_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigBurst_postprocessing_revareva;
            break;
    }
}

static void
TrigBurst_compute_next_data_frame(TrigBurst *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = TrigBurster_getSamplesBuffer((TrigBurster *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigBurst_traverse(TrigBurst *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigBurst_clear(TrigBurst *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigBurst_dealloc(TrigBurst* self)
{
    pyo_DEALLOC
    TrigBurst_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurst_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigBurst *self;
    self = (TrigBurst *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurst_compute_next_data_frame);
    self->mode_func_ptr = TrigBurst_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigBurster *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigBurst_getServer(TrigBurst* self) { GET_SERVER };
static PyObject * TrigBurst_getStream(TrigBurst* self) { GET_STREAM };
static PyObject * TrigBurst_setMul(TrigBurst *self, PyObject *arg) { SET_MUL };
static PyObject * TrigBurst_setAdd(TrigBurst *self, PyObject *arg) { SET_ADD };
static PyObject * TrigBurst_setSub(TrigBurst *self, PyObject *arg) { SET_SUB };
static PyObject * TrigBurst_setDiv(TrigBurst *self, PyObject *arg) { SET_DIV };

static PyObject * TrigBurst_play(TrigBurst *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurst_out(TrigBurst *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigBurst_stop(TrigBurst *self) { STOP };

static PyObject * TrigBurst_multiply(TrigBurst *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigBurst_inplace_multiply(TrigBurst *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigBurst_add(TrigBurst *self, PyObject *arg) { ADD };
static PyObject * TrigBurst_inplace_add(TrigBurst *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigBurst_sub(TrigBurst *self, PyObject *arg) { SUB };
static PyObject * TrigBurst_inplace_sub(TrigBurst *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigBurst_div(TrigBurst *self, PyObject *arg) { DIV };
static PyObject * TrigBurst_inplace_div(TrigBurst *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigBurst_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurst, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurst, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigBurst, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigBurst, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurst_methods[] = {
    {"getServer", (PyCFunction)TrigBurst_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurst_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurst_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigBurst_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigBurst_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigBurst_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigBurst_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigBurst_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigBurst_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigBurst_as_number = {
    (binaryfunc)TrigBurst_add,                         /*nb_add*/
    (binaryfunc)TrigBurst_sub,                         /*nb_subtract*/
    (binaryfunc)TrigBurst_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigBurst_div,                                              /*nb_divide*/
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
    (binaryfunc)TrigBurst_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigBurst_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigBurst_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigBurst_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigBurstType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurst_base",         /*tp_name*/
    sizeof(TrigBurst),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurst_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigBurst_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigBurst objects. Reads a channel from a TrigBurster.",           /* tp_doc */
    (traverseproc)TrigBurst_traverse,   /* tp_traverse */
    (inquiry)TrigBurst_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurst_methods,             /* tp_methods */
    TrigBurst_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurst_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigBurstTapStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigBurster *mainPlayer;
    int chnl;
    int modebuffer[2];
} TrigBurstTapStream;

static void TrigBurstTapStream_postprocessing_ii(TrigBurstTapStream *self) { POST_PROCESSING_II };
static void TrigBurstTapStream_postprocessing_ai(TrigBurstTapStream *self) { POST_PROCESSING_AI };
static void TrigBurstTapStream_postprocessing_ia(TrigBurstTapStream *self) { POST_PROCESSING_IA };
static void TrigBurstTapStream_postprocessing_aa(TrigBurstTapStream *self) { POST_PROCESSING_AA };
static void TrigBurstTapStream_postprocessing_ireva(TrigBurstTapStream *self) { POST_PROCESSING_IREVA };
static void TrigBurstTapStream_postprocessing_areva(TrigBurstTapStream *self) { POST_PROCESSING_AREVA };
static void TrigBurstTapStream_postprocessing_revai(TrigBurstTapStream *self) { POST_PROCESSING_REVAI };
static void TrigBurstTapStream_postprocessing_revaa(TrigBurstTapStream *self) { POST_PROCESSING_REVAA };
static void TrigBurstTapStream_postprocessing_revareva(TrigBurstTapStream *self) { POST_PROCESSING_REVAREVA };

static void
TrigBurstTapStream_setProcMode(TrigBurstTapStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigBurstTapStream_postprocessing_revareva;
            break;
    }
}

static void
TrigBurstTapStream_compute_next_data_frame(TrigBurstTapStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = TrigBurster_getTapBuffer((TrigBurster *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigBurstTapStream_traverse(TrigBurstTapStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigBurstTapStream_clear(TrigBurstTapStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigBurstTapStream_dealloc(TrigBurstTapStream* self)
{
    pyo_DEALLOC
    TrigBurstTapStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurstTapStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigBurstTapStream *self;
    self = (TrigBurstTapStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurstTapStream_compute_next_data_frame);
    self->mode_func_ptr = TrigBurstTapStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigBurster *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigBurstTapStream_getServer(TrigBurstTapStream* self) { GET_SERVER };
static PyObject * TrigBurstTapStream_getStream(TrigBurstTapStream* self) { GET_STREAM };
static PyObject * TrigBurstTapStream_setMul(TrigBurstTapStream *self, PyObject *arg) { SET_MUL };
static PyObject * TrigBurstTapStream_setAdd(TrigBurstTapStream *self, PyObject *arg) { SET_ADD };
static PyObject * TrigBurstTapStream_setSub(TrigBurstTapStream *self, PyObject *arg) { SET_SUB };
static PyObject * TrigBurstTapStream_setDiv(TrigBurstTapStream *self, PyObject *arg) { SET_DIV };

static PyObject * TrigBurstTapStream_play(TrigBurstTapStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurstTapStream_out(TrigBurstTapStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigBurstTapStream_stop(TrigBurstTapStream *self) { STOP };

static PyObject * TrigBurstTapStream_multiply(TrigBurstTapStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigBurstTapStream_inplace_multiply(TrigBurstTapStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigBurstTapStream_add(TrigBurstTapStream *self, PyObject *arg) { ADD };
static PyObject * TrigBurstTapStream_inplace_add(TrigBurstTapStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigBurstTapStream_sub(TrigBurstTapStream *self, PyObject *arg) { SUB };
static PyObject * TrigBurstTapStream_inplace_sub(TrigBurstTapStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigBurstTapStream_div(TrigBurstTapStream *self, PyObject *arg) { DIV };
static PyObject * TrigBurstTapStream_inplace_div(TrigBurstTapStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigBurstTapStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurstTapStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurstTapStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigBurstTapStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigBurstTapStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurstTapStream_methods[] = {
    {"getServer", (PyCFunction)TrigBurstTapStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurstTapStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurstTapStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigBurstTapStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigBurstTapStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigBurstTapStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigBurstTapStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigBurstTapStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigBurstTapStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigBurstTapStream_as_number = {
    (binaryfunc)TrigBurstTapStream_add,                         /*nb_add*/
    (binaryfunc)TrigBurstTapStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigBurstTapStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigBurstTapStream_div,                                              /*nb_divide*/
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
    (binaryfunc)TrigBurstTapStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigBurstTapStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigBurstTapStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigBurstTapStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigBurstTapStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurstTapStream_base",         /*tp_name*/
    sizeof(TrigBurstTapStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurstTapStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigBurstTapStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigBurstTapStream objects. Reads the current tap from a TrigBurster object.",           /* tp_doc */
    (traverseproc)TrigBurstTapStream_traverse,   /* tp_traverse */
    (inquiry)TrigBurstTapStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurstTapStream_methods,             /* tp_methods */
    TrigBurstTapStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurstTapStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigBurstAmpStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigBurster *mainPlayer;
    int chnl;
    int modebuffer[2];
} TrigBurstAmpStream;

static void TrigBurstAmpStream_postprocessing_ii(TrigBurstAmpStream *self) { POST_PROCESSING_II };
static void TrigBurstAmpStream_postprocessing_ai(TrigBurstAmpStream *self) { POST_PROCESSING_AI };
static void TrigBurstAmpStream_postprocessing_ia(TrigBurstAmpStream *self) { POST_PROCESSING_IA };
static void TrigBurstAmpStream_postprocessing_aa(TrigBurstAmpStream *self) { POST_PROCESSING_AA };
static void TrigBurstAmpStream_postprocessing_ireva(TrigBurstAmpStream *self) { POST_PROCESSING_IREVA };
static void TrigBurstAmpStream_postprocessing_areva(TrigBurstAmpStream *self) { POST_PROCESSING_AREVA };
static void TrigBurstAmpStream_postprocessing_revai(TrigBurstAmpStream *self) { POST_PROCESSING_REVAI };
static void TrigBurstAmpStream_postprocessing_revaa(TrigBurstAmpStream *self) { POST_PROCESSING_REVAA };
static void TrigBurstAmpStream_postprocessing_revareva(TrigBurstAmpStream *self) { POST_PROCESSING_REVAREVA };

static void
TrigBurstAmpStream_setProcMode(TrigBurstAmpStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigBurstAmpStream_postprocessing_revareva;
            break;
    }
}

static void
TrigBurstAmpStream_compute_next_data_frame(TrigBurstAmpStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = TrigBurster_getAmpBuffer((TrigBurster *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigBurstAmpStream_traverse(TrigBurstAmpStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigBurstAmpStream_clear(TrigBurstAmpStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigBurstAmpStream_dealloc(TrigBurstAmpStream* self)
{
    pyo_DEALLOC
    TrigBurstAmpStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurstAmpStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigBurstAmpStream *self;
    self = (TrigBurstAmpStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurstAmpStream_compute_next_data_frame);
    self->mode_func_ptr = TrigBurstAmpStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigBurster *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigBurstAmpStream_getServer(TrigBurstAmpStream* self) { GET_SERVER };
static PyObject * TrigBurstAmpStream_getStream(TrigBurstAmpStream* self) { GET_STREAM };
static PyObject * TrigBurstAmpStream_setMul(TrigBurstAmpStream *self, PyObject *arg) { SET_MUL };
static PyObject * TrigBurstAmpStream_setAdd(TrigBurstAmpStream *self, PyObject *arg) { SET_ADD };
static PyObject * TrigBurstAmpStream_setSub(TrigBurstAmpStream *self, PyObject *arg) { SET_SUB };
static PyObject * TrigBurstAmpStream_setDiv(TrigBurstAmpStream *self, PyObject *arg) { SET_DIV };

static PyObject * TrigBurstAmpStream_play(TrigBurstAmpStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurstAmpStream_out(TrigBurstAmpStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigBurstAmpStream_stop(TrigBurstAmpStream *self) { STOP };

static PyObject * TrigBurstAmpStream_multiply(TrigBurstAmpStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigBurstAmpStream_inplace_multiply(TrigBurstAmpStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigBurstAmpStream_add(TrigBurstAmpStream *self, PyObject *arg) { ADD };
static PyObject * TrigBurstAmpStream_inplace_add(TrigBurstAmpStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigBurstAmpStream_sub(TrigBurstAmpStream *self, PyObject *arg) { SUB };
static PyObject * TrigBurstAmpStream_inplace_sub(TrigBurstAmpStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigBurstAmpStream_div(TrigBurstAmpStream *self, PyObject *arg) { DIV };
static PyObject * TrigBurstAmpStream_inplace_div(TrigBurstAmpStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigBurstAmpStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurstAmpStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurstAmpStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigBurstAmpStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigBurstAmpStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurstAmpStream_methods[] = {
    {"getServer", (PyCFunction)TrigBurstAmpStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurstAmpStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurstAmpStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigBurstAmpStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigBurstAmpStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigBurstAmpStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigBurstAmpStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigBurstAmpStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigBurstAmpStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigBurstAmpStream_as_number = {
    (binaryfunc)TrigBurstAmpStream_add,                         /*nb_add*/
    (binaryfunc)TrigBurstAmpStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigBurstAmpStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigBurstAmpStream_div,                                              /*nb_divide*/
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
    (binaryfunc)TrigBurstAmpStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigBurstAmpStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigBurstAmpStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigBurstAmpStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigBurstAmpStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurstAmpStream_base",         /*tp_name*/
    sizeof(TrigBurstAmpStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurstAmpStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigBurstAmpStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigBurstAmpStream objects. Reads a amplitude channel from a TrigBurster object.",           /* tp_doc */
    (traverseproc)TrigBurstAmpStream_traverse,   /* tp_traverse */
    (inquiry)TrigBurstAmpStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurstAmpStream_methods,             /* tp_methods */
    TrigBurstAmpStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurstAmpStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigBurstDurStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigBurster *mainPlayer;
    int chnl;
    int modebuffer[2];
} TrigBurstDurStream;

static void TrigBurstDurStream_postprocessing_ii(TrigBurstDurStream *self) { POST_PROCESSING_II };
static void TrigBurstDurStream_postprocessing_ai(TrigBurstDurStream *self) { POST_PROCESSING_AI };
static void TrigBurstDurStream_postprocessing_ia(TrigBurstDurStream *self) { POST_PROCESSING_IA };
static void TrigBurstDurStream_postprocessing_aa(TrigBurstDurStream *self) { POST_PROCESSING_AA };
static void TrigBurstDurStream_postprocessing_ireva(TrigBurstDurStream *self) { POST_PROCESSING_IREVA };
static void TrigBurstDurStream_postprocessing_areva(TrigBurstDurStream *self) { POST_PROCESSING_AREVA };
static void TrigBurstDurStream_postprocessing_revai(TrigBurstDurStream *self) { POST_PROCESSING_REVAI };
static void TrigBurstDurStream_postprocessing_revaa(TrigBurstDurStream *self) { POST_PROCESSING_REVAA };
static void TrigBurstDurStream_postprocessing_revareva(TrigBurstDurStream *self) { POST_PROCESSING_REVAREVA };

static void
TrigBurstDurStream_setProcMode(TrigBurstDurStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigBurstDurStream_postprocessing_revareva;
            break;
    }
}

static void
TrigBurstDurStream_compute_next_data_frame(TrigBurstDurStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = TrigBurster_getDurBuffer((TrigBurster *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigBurstDurStream_traverse(TrigBurstDurStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigBurstDurStream_clear(TrigBurstDurStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigBurstDurStream_dealloc(TrigBurstDurStream* self)
{
    pyo_DEALLOC
    TrigBurstDurStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurstDurStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigBurstDurStream *self;
    self = (TrigBurstDurStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurstDurStream_compute_next_data_frame);
    self->mode_func_ptr = TrigBurstDurStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigBurster *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigBurstDurStream_getServer(TrigBurstDurStream* self) { GET_SERVER };
static PyObject * TrigBurstDurStream_getStream(TrigBurstDurStream* self) { GET_STREAM };
static PyObject * TrigBurstDurStream_setMul(TrigBurstDurStream *self, PyObject *arg) { SET_MUL };
static PyObject * TrigBurstDurStream_setAdd(TrigBurstDurStream *self, PyObject *arg) { SET_ADD };
static PyObject * TrigBurstDurStream_setSub(TrigBurstDurStream *self, PyObject *arg) { SET_SUB };
static PyObject * TrigBurstDurStream_setDiv(TrigBurstDurStream *self, PyObject *arg) { SET_DIV };

static PyObject * TrigBurstDurStream_play(TrigBurstDurStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurstDurStream_out(TrigBurstDurStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigBurstDurStream_stop(TrigBurstDurStream *self) { STOP };

static PyObject * TrigBurstDurStream_multiply(TrigBurstDurStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigBurstDurStream_inplace_multiply(TrigBurstDurStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigBurstDurStream_add(TrigBurstDurStream *self, PyObject *arg) { ADD };
static PyObject * TrigBurstDurStream_inplace_add(TrigBurstDurStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigBurstDurStream_sub(TrigBurstDurStream *self, PyObject *arg) { SUB };
static PyObject * TrigBurstDurStream_inplace_sub(TrigBurstDurStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigBurstDurStream_div(TrigBurstDurStream *self, PyObject *arg) { DIV };
static PyObject * TrigBurstDurStream_inplace_div(TrigBurstDurStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigBurstDurStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurstDurStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurstDurStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigBurstDurStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigBurstDurStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurstDurStream_methods[] = {
    {"getServer", (PyCFunction)TrigBurstDurStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurstDurStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurstDurStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigBurstDurStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigBurstDurStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigBurstDurStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigBurstDurStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigBurstDurStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigBurstDurStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigBurstDurStream_as_number = {
    (binaryfunc)TrigBurstDurStream_add,                         /*nb_add*/
    (binaryfunc)TrigBurstDurStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigBurstDurStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigBurstDurStream_div,                                              /*nb_divide*/
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
    (binaryfunc)TrigBurstDurStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigBurstDurStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigBurstDurStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigBurstDurStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigBurstDurStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurstDurStream_base",         /*tp_name*/
    sizeof(TrigBurstDurStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurstDurStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigBurstDurStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigBurstDurStream objects. Reads a duration channel from a TrigBurster object.",           /* tp_doc */
    (traverseproc)TrigBurstDurStream_traverse,   /* tp_traverse */
    (inquiry)TrigBurstDurStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurstDurStream_methods,             /* tp_methods */
    TrigBurstDurStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurstDurStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigBurstEndStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigBurster *mainPlayer;
    int chnl;
    int modebuffer[2];
} TrigBurstEndStream;

static void TrigBurstEndStream_postprocessing_ii(TrigBurstEndStream *self) { POST_PROCESSING_II };
static void TrigBurstEndStream_postprocessing_ai(TrigBurstEndStream *self) { POST_PROCESSING_AI };
static void TrigBurstEndStream_postprocessing_ia(TrigBurstEndStream *self) { POST_PROCESSING_IA };
static void TrigBurstEndStream_postprocessing_aa(TrigBurstEndStream *self) { POST_PROCESSING_AA };
static void TrigBurstEndStream_postprocessing_ireva(TrigBurstEndStream *self) { POST_PROCESSING_IREVA };
static void TrigBurstEndStream_postprocessing_areva(TrigBurstEndStream *self) { POST_PROCESSING_AREVA };
static void TrigBurstEndStream_postprocessing_revai(TrigBurstEndStream *self) { POST_PROCESSING_REVAI };
static void TrigBurstEndStream_postprocessing_revaa(TrigBurstEndStream *self) { POST_PROCESSING_REVAA };
static void TrigBurstEndStream_postprocessing_revareva(TrigBurstEndStream *self) { POST_PROCESSING_REVAREVA };

static void
TrigBurstEndStream_setProcMode(TrigBurstEndStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigBurstEndStream_postprocessing_revareva;
            break;
    }
}

static void
TrigBurstEndStream_compute_next_data_frame(TrigBurstEndStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = TrigBurster_getEndBuffer((TrigBurster *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigBurstEndStream_traverse(TrigBurstEndStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigBurstEndStream_clear(TrigBurstEndStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigBurstEndStream_dealloc(TrigBurstEndStream* self)
{
    pyo_DEALLOC
    TrigBurstEndStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigBurstEndStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigBurstEndStream *self;
    self = (TrigBurstEndStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigBurstEndStream_compute_next_data_frame);
    self->mode_func_ptr = TrigBurstEndStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigBurster *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigBurstEndStream_getServer(TrigBurstEndStream* self) { GET_SERVER };
static PyObject * TrigBurstEndStream_getStream(TrigBurstEndStream* self) { GET_STREAM };
static PyObject * TrigBurstEndStream_setMul(TrigBurstEndStream *self, PyObject *arg) { SET_MUL };
static PyObject * TrigBurstEndStream_setAdd(TrigBurstEndStream *self, PyObject *arg) { SET_ADD };
static PyObject * TrigBurstEndStream_setSub(TrigBurstEndStream *self, PyObject *arg) { SET_SUB };
static PyObject * TrigBurstEndStream_setDiv(TrigBurstEndStream *self, PyObject *arg) { SET_DIV };

static PyObject * TrigBurstEndStream_play(TrigBurstEndStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigBurstEndStream_out(TrigBurstEndStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigBurstEndStream_stop(TrigBurstEndStream *self) { STOP };

static PyObject * TrigBurstEndStream_multiply(TrigBurstEndStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigBurstEndStream_inplace_multiply(TrigBurstEndStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigBurstEndStream_add(TrigBurstEndStream *self, PyObject *arg) { ADD };
static PyObject * TrigBurstEndStream_inplace_add(TrigBurstEndStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigBurstEndStream_sub(TrigBurstEndStream *self, PyObject *arg) { SUB };
static PyObject * TrigBurstEndStream_inplace_sub(TrigBurstEndStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigBurstEndStream_div(TrigBurstEndStream *self, PyObject *arg) { DIV };
static PyObject * TrigBurstEndStream_inplace_div(TrigBurstEndStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigBurstEndStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigBurstEndStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigBurstEndStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigBurstEndStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigBurstEndStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigBurstEndStream_methods[] = {
    {"getServer", (PyCFunction)TrigBurstEndStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigBurstEndStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigBurstEndStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigBurstEndStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigBurstEndStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigBurstEndStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigBurstEndStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigBurstEndStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigBurstEndStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigBurstEndStream_as_number = {
    (binaryfunc)TrigBurstEndStream_add,                         /*nb_add*/
    (binaryfunc)TrigBurstEndStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigBurstEndStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigBurstEndStream_div,                                              /*nb_divide*/
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
    (binaryfunc)TrigBurstEndStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigBurstEndStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigBurstEndStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigBurstEndStream_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigBurstEndStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigBurstEndStream_base",         /*tp_name*/
    sizeof(TrigBurstEndStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigBurstEndStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigBurstEndStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigBurstEndStream objects. Reads a duration channel from a TrigBurster object.",           /* tp_doc */
    (traverseproc)TrigBurstEndStream_traverse,   /* tp_traverse */
    (inquiry)TrigBurstEndStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigBurstEndStream_methods,             /* tp_methods */
    TrigBurstEndStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigBurstEndStream_new,                 /* tp_new */
};