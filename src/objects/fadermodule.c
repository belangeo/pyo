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
    int modebuffer[2];
    int fademode;
    int ended;
    MYFLT topValue;
    MYFLT attack;
    MYFLT release;
    MYFLT duration;
    MYFLT exp;
    MYFLT offset;
    MYFLT currentVal;
    double currentTime;
    MYFLT sampleToSec;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} Fader;

static void Fader_internal_stop(Fader *self) {
    int i;
    Stream_setStreamActive(self->stream, 0);
    Stream_setStreamChnl(self->stream, 0);
    Stream_setStreamToDac(self->stream, 0);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0;
        self->trigsBuffer[i] = 0.0;
    }
}

static void
Fader_generate_auto(Fader *self) {
    MYFLT val, iatt, irel;
    int i;
    
    iatt = 1.0 / self->attack;
    irel = 1.0 / self->release;

    if (self->ended == 1) {
        Fader_internal_stop((Fader *)self);
        return;
    }

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        if (self->currentTime <= self->attack)
            val = self->currentTime * iatt * (1.0 - self->offset) + self->offset;
        else if (self->currentTime > self->duration) {
            if (self->ended == 0) {
                self->trigsBuffer[i] = 1.0;
            }
            val = 0.;
            self->ended = 1;
        }
        else if (self->currentTime >= (self->duration - self->release))
            val = (self->duration - self->currentTime) * irel;
        else
            val = 1.;

        self->data[i] = self->currentVal = val;
        self->currentTime += self->sampleToSec;
    }

    if (self->exp != 1.0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYPOW(self->data[i], self->exp);
        }
    }
}

static void
Fader_generate_wait(Fader *self) {
    MYFLT val, iatt, irel;
    int i;

    iatt = 1.0 / self->attack;
    irel = 1.0 / self->release;

    if (self->fademode == 1 && self->ended == 1) {
        Fader_internal_stop((Fader *)self);
        return;
    }

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        if (self->fademode == 0) {
            if (self->currentTime <= self->attack)
                val = self->currentTime * iatt * (1.0 - self->offset) + self->offset;
            else
                val = 1.;
            self->topValue = val;
        }
        else {
            if (self->currentTime <= self->release)
                val = (1. - self->currentTime * irel) * self->topValue;
            else {
                if (self->ended == 0) {
                    self->trigsBuffer[i] = 1.0;
                }
                val = 0.;
                self->ended = 1;
            }
        }
        self->data[i] = self->currentVal = val;
        self->currentTime += self->sampleToSec;
    }
    
    if (self->exp != 1.0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYPOW(self->data[i], self->exp);
        }
    }
}

static void Fader_postprocessing_ii(Fader *self) { POST_PROCESSING_II };
static void Fader_postprocessing_ai(Fader *self) { POST_PROCESSING_AI };
static void Fader_postprocessing_ia(Fader *self) { POST_PROCESSING_IA };
static void Fader_postprocessing_aa(Fader *self) { POST_PROCESSING_AA };
static void Fader_postprocessing_ireva(Fader *self) { POST_PROCESSING_IREVA };
static void Fader_postprocessing_areva(Fader *self) { POST_PROCESSING_AREVA };
static void Fader_postprocessing_revai(Fader *self) { POST_PROCESSING_REVAI };
static void Fader_postprocessing_revaa(Fader *self) { POST_PROCESSING_REVAA };
static void Fader_postprocessing_revareva(Fader *self) { POST_PROCESSING_REVAREVA };

static void
Fader_setProcMode(Fader *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    if (self->duration == 0.0)
        self->proc_func_ptr = Fader_generate_wait;
    else
        self->proc_func_ptr = Fader_generate_auto;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Fader_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Fader_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Fader_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Fader_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Fader_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Fader_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Fader_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Fader_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Fader_postprocessing_revareva;
            break;
    }
}

static void
Fader_compute_next_data_frame(Fader *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Fader_traverse(Fader *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
Fader_clear(Fader *self)
{
    pyo_CLEAR
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
Fader_dealloc(Fader* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    Fader_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Fader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    Fader *self;
    self = (Fader *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    self->topValue = 0.0;
    self->fademode = 0;
    self->ended = 0;
    self->attack = 0.01;
    self->release = 0.1;
    self->duration = 0.0;
    self->exp = 1.0;
    self->currentTime = 0.0;
    self->offset = 0.0;
    self->currentVal = 0.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Fader_compute_next_data_frame);
    self->mode_func_ptr = Fader_setProcMode;

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"fadein", "fadeout", "dur", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFFOO, kwlist, &self->attack, &self->release, &self->duration, &multmp, &addtmp))
        Py_RETURN_NONE;

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

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Fader_getServer(Fader* self) { GET_SERVER };
static PyObject * Fader_getStream(Fader* self) { GET_STREAM };
static PyObject * Fader_getTriggerStream(Fader* self) { GET_TRIGGER_STREAM };
static PyObject * Fader_setMul(Fader *self, PyObject *arg) { SET_MUL };
static PyObject * Fader_setAdd(Fader *self, PyObject *arg) { SET_ADD };
static PyObject * Fader_setSub(Fader *self, PyObject *arg) { SET_SUB };
static PyObject * Fader_setDiv(Fader *self, PyObject *arg) { SET_DIV };

static PyObject * Fader_play(Fader *self, PyObject *args, PyObject *kwds)
{
    self->fademode = 0;
    self->ended = 0;
    self->currentTime = 0.0;
    self->offset = self->currentVal;
    (*self->mode_func_ptr)(self);
    PLAY
};

static PyObject *
Fader_stop(Fader *self, PyObject *args, PyObject *kwds)
{
    if (self->duration == 0.0) {
        self->fademode = 1;
        self->currentTime = 0.0;
    }
    else
        Fader_internal_stop((Fader *)self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * Fader_multiply(Fader *self, PyObject *arg) { MULTIPLY };
static PyObject * Fader_inplace_multiply(Fader *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Fader_add(Fader *self, PyObject *arg) { ADD };
static PyObject * Fader_inplace_add(Fader *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Fader_sub(Fader *self, PyObject *arg) { SUB };
static PyObject * Fader_inplace_sub(Fader *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Fader_div(Fader *self, PyObject *arg) { DIV };
static PyObject * Fader_inplace_div(Fader *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Fader_setFadein(Fader *self, PyObject *arg)
{
    self->attack = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Fader_setFadeout(Fader *self, PyObject *arg)
{
    self->release = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Fader_setDur(Fader *self, PyObject *arg)
{
    self->duration = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Fader_setExp(Fader *self, PyObject *arg)
{
    MYFLT tmp = PyFloat_AsDouble(arg);
    if (tmp > 0.0)
        self->exp = tmp;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Fader_members[] = {
{"server", T_OBJECT_EX, offsetof(Fader, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Fader, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(Fader, trig_stream), 0, "Trigger Stream object."},
{"mul", T_OBJECT_EX, offsetof(Fader, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Fader, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Fader_methods[] = {
{"getServer", (PyCFunction)Fader_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Fader_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)Fader_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
{"play", (PyCFunction)Fader_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Fader_stop, METH_VARARGS|METH_KEYWORDS, "Starts fadeout and stops computing."},
{"setMul", (PyCFunction)Fader_setMul, METH_O, "Sets Fader mul factor."},
{"setAdd", (PyCFunction)Fader_setAdd, METH_O, "Sets Fader add factor."},
{"setSub", (PyCFunction)Fader_setSub, METH_O, "Sets inverse add factor."},
{"setFadein", (PyCFunction)Fader_setFadein, METH_O, "Sets fadein time in seconds."},
{"setFadeout", (PyCFunction)Fader_setFadeout, METH_O, "Sets fadeout time in seconds."},
{"setDur", (PyCFunction)Fader_setDur, METH_O, "Sets duration in seconds (0 means wait for stop method to start fadeout)."},
{"setExp", (PyCFunction)Fader_setExp, METH_O, "Sets the exponent factor for exponential envelope."},
{"setDiv", (PyCFunction)Fader_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Fader_as_number = {
(binaryfunc)Fader_add,                      /*nb_add*/
(binaryfunc)Fader_sub,                 /*nb_subtract*/
(binaryfunc)Fader_multiply,                 /*nb_multiply*/
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
(binaryfunc)Fader_inplace_add,              /*inplace_add*/
(binaryfunc)Fader_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Fader_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Fader_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Fader_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject FaderType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Fader_base",         /*tp_name*/
sizeof(Fader),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Fader_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Fader_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Fader objects. Generates fadin and fadeout signal.",           /* tp_doc */
(traverseproc)Fader_traverse,   /* tp_traverse */
(inquiry)Fader_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Fader_methods,             /* tp_methods */
Fader_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Fader_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    int modebuffer[2];
    int fademode;
    MYFLT topValue;
    MYFLT attack;
    MYFLT decay;
    MYFLT sustain;
    MYFLT release;
    MYFLT duration;
    MYFLT exp;
    MYFLT offset;
    MYFLT currentVal;
    double currentTime;
    MYFLT sampleToSec;
    int ended;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} Adsr;

static void Adsr_internal_stop(Adsr *self) {
    int i;
    Stream_setStreamActive(self->stream, 0);
    Stream_setStreamChnl(self->stream, 0);
    Stream_setStreamToDac(self->stream, 0);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0;
        self->trigsBuffer[i] = 0.0;
    }
}

static void
Adsr_generate_auto(Adsr *self) {
    MYFLT val, invatt, invdec, invrel;
    int i;

    if (self->currentTime > self->duration)
        Adsr_internal_stop((Adsr *)self);

    invatt = 1.0 / self->attack;
    invdec = 1.0 / self->decay;
    invrel = 1.0 / self->release;

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        if (self->currentTime <= self->attack)
            val = self->currentTime * invatt * (1.0 - self->offset) + self->offset;
        else if (self->currentTime <= (self->attack + self->decay))
            val = (self->decay - (self->currentTime - self->attack)) * invdec * (1. - self->sustain) + self->sustain;
        else if (self->currentTime > self->duration) {
            if (self->ended == 0) {
                self->trigsBuffer[i] = 1.0;
            }
            self->ended = 1;
            val = 0.;
        }
        else if (self->currentTime >= (self->duration - self->release))
            val = (self->duration - self->currentTime) * invrel * self->sustain;
        else
            val = self->sustain;

        self->data[i] = self->currentVal = val;
        self->currentTime += self->sampleToSec;
    }

    if (self->exp != 1.0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYPOW(self->data[i], self->exp);
        }
    }
}

static void
Adsr_generate_wait(Adsr *self) {
    MYFLT val, invatt, invdec, invrel;
    int i;

    if (self->fademode == 1 && self->currentTime > self->release)
        Adsr_internal_stop((Adsr *)self);

    invatt = 1.0 / self->attack;
    invdec = 1.0 / self->decay;
    invrel = 1.0 / self->release;

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        if (self->fademode == 0) {
            if (self->currentTime <= self->attack)
                val = self->currentTime * invatt * (1.0 - self->offset) + self->offset;
            else if (self->currentTime <= (self->attack + self->decay))
                val = (self->decay - (self->currentTime - self->attack)) * invdec * (1. - self->sustain) + self->sustain;
            else
                val = self->sustain;
            self->topValue = val;
        }
        else {
            if (self->currentTime <= self->release) {
                val = self->topValue * (1. - self->currentTime * invrel);
            }
            else {
                if (self->ended == 0) {
                    self->trigsBuffer[i] = 1.0;
                }
                self->ended = 1;
                val = 0.;
            }
        }
        self->data[i] = self->currentVal = val;
        self->currentTime += self->sampleToSec;
    }

    if (self->exp != 1.0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYPOW(self->data[i], self->exp);
        }
    }
}

static void Adsr_postprocessing_ii(Adsr *self) { POST_PROCESSING_II };
static void Adsr_postprocessing_ai(Adsr *self) { POST_PROCESSING_AI };
static void Adsr_postprocessing_ia(Adsr *self) { POST_PROCESSING_IA };
static void Adsr_postprocessing_aa(Adsr *self) { POST_PROCESSING_AA };
static void Adsr_postprocessing_ireva(Adsr *self) { POST_PROCESSING_IREVA };
static void Adsr_postprocessing_areva(Adsr *self) { POST_PROCESSING_AREVA };
static void Adsr_postprocessing_revai(Adsr *self) { POST_PROCESSING_REVAI };
static void Adsr_postprocessing_revaa(Adsr *self) { POST_PROCESSING_REVAA };
static void Adsr_postprocessing_revareva(Adsr *self) { POST_PROCESSING_REVAREVA };

static void
Adsr_setProcMode(Adsr *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    if (self->duration == 0.0)
        self->proc_func_ptr = Adsr_generate_wait;
    else
        self->proc_func_ptr = Adsr_generate_auto;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Adsr_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Adsr_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Adsr_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Adsr_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Adsr_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Adsr_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Adsr_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Adsr_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Adsr_postprocessing_revareva;
            break;
    }
}

static void
Adsr_compute_next_data_frame(Adsr *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Adsr_traverse(Adsr *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
Adsr_clear(Adsr *self)
{
    pyo_CLEAR
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
Adsr_dealloc(Adsr* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    Adsr_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Adsr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    Adsr *self;
    self = (Adsr *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    self->topValue = 0.0;
    self->fademode = 0;
    self->attack = 0.01;
    self->decay = 0.05;
    self->sustain = 0.707;
    self->release = 0.1;
    self->duration = 0.0;
    self->exp = 1.0;
    self->currentTime = 0.0;
    self->offset = 0.0;
    self->currentVal = 0.0;
    self->ended = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Adsr_compute_next_data_frame);
    self->mode_func_ptr = Adsr_setProcMode;

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"attack", "decay", "sustain", "release", "dur", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFFFFOO, kwlist, &self->attack, &self->decay, &self->sustain, &self->release, &self->duration, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->attack < 0.000001)
        self->attack = 0.000001;
    if (self->decay < 0.000001)
        self->decay = 0.000001;
    if (self->release < 0.000001)
        self->release = 0.000001;
    if (self->sustain < 0.0)
        self->sustain = 0.0;
    else if (self->sustain > 1.0)
        self->sustain = 1.0;

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Adsr_getServer(Adsr* self) { GET_SERVER };
static PyObject * Adsr_getStream(Adsr* self) { GET_STREAM };
static PyObject * Adsr_getTriggerStream(Adsr* self) { GET_TRIGGER_STREAM };
static PyObject * Adsr_setMul(Adsr *self, PyObject *arg) { SET_MUL };
static PyObject * Adsr_setAdd(Adsr *self, PyObject *arg) { SET_ADD };
static PyObject * Adsr_setSub(Adsr *self, PyObject *arg) { SET_SUB };
static PyObject * Adsr_setDiv(Adsr *self, PyObject *arg) { SET_DIV };

static PyObject * Adsr_play(Adsr *self, PyObject *args, PyObject *kwds)
{
    self->ended = 0;
    self->fademode = 0;
    self->currentTime = 0.0;
    self->offset = self->currentVal;
    (*self->mode_func_ptr)(self);
    PLAY
};

static PyObject *
Adsr_stop(Adsr *self, PyObject *args, PyObject *kwds)
{
    if (self->duration == 0.0) {
        self->fademode = 1;
        self->currentTime = 0.0;
    }
    else
        Adsr_internal_stop((Adsr *)self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * Adsr_multiply(Adsr *self, PyObject *arg) { MULTIPLY };
static PyObject * Adsr_inplace_multiply(Adsr *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Adsr_add(Adsr *self, PyObject *arg) { ADD };
static PyObject * Adsr_inplace_add(Adsr *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Adsr_sub(Adsr *self, PyObject *arg) { SUB };
static PyObject * Adsr_inplace_sub(Adsr *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Adsr_div(Adsr *self, PyObject *arg) { DIV };
static PyObject * Adsr_inplace_div(Adsr *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Adsr_setAttack(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) {
        self->attack = PyFloat_AsDouble(arg);
        if (self->attack < 0.000001)
            self->attack = 0.000001;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Adsr_setDecay(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) {
        self->decay = PyFloat_AsDouble(arg);
        if (self->decay < 0.000001)
            self->decay = 0.000001;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Adsr_setSustain(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) {
        self->sustain = PyFloat_AsDouble(arg);
        if (self->sustain < 0.0)
            self->sustain = 0.0;
        else if (self->sustain > 1.0)
            self->sustain = 1.0;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Adsr_setRelease(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) {
        self->release = PyFloat_AsDouble(arg);
        if (self->release < 0.000001)
            self->release = 0.000001;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Adsr_setDur(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) 
        self->duration = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Adsr_setExp(Adsr *self, PyObject *arg)
{
	if (PyNumber_Check(arg)) {
        MYFLT tmp = PyFloat_AsDouble(arg);
        if (tmp > 0.0)
            self->exp = tmp;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Adsr_members[] = {
{"server", T_OBJECT_EX, offsetof(Adsr, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Adsr, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(Adsr, trig_stream), 0, "Trigger Stream object."},
{"mul", T_OBJECT_EX, offsetof(Adsr, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Adsr, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Adsr_methods[] = {
{"getServer", (PyCFunction)Adsr_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Adsr_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)Adsr_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
{"play", (PyCFunction)Adsr_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Adsr_stop, METH_VARARGS|METH_KEYWORDS, "Starts fadeout and stops computing."},
{"setMul", (PyCFunction)Adsr_setMul, METH_O, "Sets Adsr mul factor."},
{"setAdd", (PyCFunction)Adsr_setAdd, METH_O, "Sets Adsr add factor."},
{"setSub", (PyCFunction)Adsr_setSub, METH_O, "Sets inverse add factor."},
{"setAttack", (PyCFunction)Adsr_setAttack, METH_O, "Sets attack time in seconds."},
{"setDecay", (PyCFunction)Adsr_setDecay, METH_O, "Sets attack time in seconds."},
{"setSustain", (PyCFunction)Adsr_setSustain, METH_O, "Sets attack time in seconds."},
{"setRelease", (PyCFunction)Adsr_setRelease, METH_O, "Sets release time in seconds."},
{"setDur", (PyCFunction)Adsr_setDur, METH_O, "Sets duration in seconds (0 means wait for stop method to start fadeout)."},
{"setExp", (PyCFunction)Adsr_setExp, METH_O, "Sets the exponent factor for exponential envelope."},
{"setDiv", (PyCFunction)Adsr_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Adsr_as_number = {
(binaryfunc)Adsr_add,                      /*nb_add*/
(binaryfunc)Adsr_sub,                 /*nb_subtract*/
(binaryfunc)Adsr_multiply,                 /*nb_multiply*/
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
(binaryfunc)Adsr_inplace_add,              /*inplace_add*/
(binaryfunc)Adsr_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Adsr_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Adsr_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Adsr_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject AdsrType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Adsr_base",         /*tp_name*/
sizeof(Adsr),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Adsr_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Adsr_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Adsr objects. Generates Adsr envelope signal.",           /* tp_doc */
(traverseproc)Adsr_traverse,   /* tp_traverse */
(inquiry)Adsr_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Adsr_methods,             /* tp_methods */
Adsr_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Adsr_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *pointslist;
    int modebuffer[2];
    double currentTime;
    double currentValue;
    MYFLT sampleToSec;
    double increment;
    MYFLT *targets;
    MYFLT *times;
    int which;
    int flag;
    int newlist;
    int loop;
    int listsize;
    int okToPause;
} Linseg;

static void
Linseg_convert_pointslist(Linseg *self) {
    int i;
    PyObject *tup;

    self->listsize = PyList_Size(self->pointslist);
    self->targets = (MYFLT *)realloc(self->targets, self->listsize * sizeof(MYFLT));
    self->times = (MYFLT *)realloc(self->times, self->listsize * sizeof(MYFLT));
    for (i=0; i<self->listsize; i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        self->times[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
        self->targets[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
    }
}

static void
Linseg_reinit(Linseg *self) {
    if (self->newlist == 1) {
        Linseg_convert_pointslist((Linseg *)self);
        self->newlist = 0;
    }
    self->currentTime = 0.0;
    self->currentValue = self->targets[0];
    self->which = 0;
    self->flag = 1;
    self->okToPause = 1;
}

static void
Linseg_generate(Linseg *self) {
    int i;

    for (i=0; i<self->bufsize; i++) {
        if (self->flag == 1) {
            if (self->currentTime >= self->times[self->which]) {
                self->which++;
                if (self->which == self->listsize) {
                    if (self->loop == 1)
                        Linseg_reinit((Linseg *)self);
                    else {
                        self->flag = 0;
                        self->okToPause = 0;
                        self->currentValue = self->targets[self->which-1];
                    }
                }
                else {
                    if ((self->times[self->which] - self->times[self->which-1]) <= 0)
                        self->increment = self->targets[self->which] - self->currentValue;
                    else
                        self->increment = (self->targets[self->which] - self->targets[self->which-1]) / ((self->times[self->which] - self->times[self->which-1]) / self->sampleToSec);
                }
            }
            if (self->currentTime <= self->times[self->listsize-1])
                self->currentValue += self->increment;
            self->data[i] = (MYFLT)self->currentValue;
            self->currentTime += self->sampleToSec;
        }
        else
            self->data[i] = (MYFLT)self->currentValue;
    }
}

static void Linseg_postprocessing_ii(Linseg *self) { POST_PROCESSING_II };
static void Linseg_postprocessing_ai(Linseg *self) { POST_PROCESSING_AI };
static void Linseg_postprocessing_ia(Linseg *self) { POST_PROCESSING_IA };
static void Linseg_postprocessing_aa(Linseg *self) { POST_PROCESSING_AA };
static void Linseg_postprocessing_ireva(Linseg *self) { POST_PROCESSING_IREVA };
static void Linseg_postprocessing_areva(Linseg *self) { POST_PROCESSING_AREVA };
static void Linseg_postprocessing_revai(Linseg *self) { POST_PROCESSING_REVAI };
static void Linseg_postprocessing_revaa(Linseg *self) { POST_PROCESSING_REVAA };
static void Linseg_postprocessing_revareva(Linseg *self) { POST_PROCESSING_REVAREVA };

static void
Linseg_setProcMode(Linseg *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Linseg_generate;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Linseg_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Linseg_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Linseg_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Linseg_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Linseg_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Linseg_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Linseg_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Linseg_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Linseg_postprocessing_revareva;
            break;
    }
}

static void
Linseg_compute_next_data_frame(Linseg *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Linseg_traverse(Linseg *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
Linseg_clear(Linseg *self)
{
    pyo_CLEAR
    Py_CLEAR(self->pointslist);
    return 0;
}

static void
Linseg_dealloc(Linseg* self)
{
    pyo_DEALLOC
    free(self->targets);
    free(self->times);
    Linseg_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Linseg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, initToFirstVal = 0;
    PyObject *pointslist=NULL, *multmp=NULL, *addtmp=NULL;
    Linseg *self;
    self = (Linseg *)type->tp_alloc(type, 0);

    self->loop = 0;
    self->newlist = 1;
    self->okToPause = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Linseg_compute_next_data_frame);
    self->mode_func_ptr = Linseg_setProcMode;

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"list", "loop", "initToFirstVal", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiOO", kwlist, &pointslist, &self->loop, &initToFirstVal, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_INCREF(pointslist);
    Py_XDECREF(self->pointslist);
    self->pointslist = pointslist;
    Linseg_convert_pointslist((Linseg *)self);

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (initToFirstVal) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->targets[0];
        }
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Linseg_getServer(Linseg* self) { GET_SERVER };
static PyObject * Linseg_getStream(Linseg* self) { GET_STREAM };
static PyObject * Linseg_setMul(Linseg *self, PyObject *arg) { SET_MUL };
static PyObject * Linseg_setAdd(Linseg *self, PyObject *arg) { SET_ADD };
static PyObject * Linseg_setSub(Linseg *self, PyObject *arg) { SET_SUB };
static PyObject * Linseg_setDiv(Linseg *self, PyObject *arg) { SET_DIV };

static PyObject * Linseg_play(Linseg *self, PyObject *args, PyObject *kwds)
{
    Linseg_reinit((Linseg *)self);
    PLAY
};

static PyObject * Linseg_stop(Linseg *self, PyObject *args, PyObject *kwds) 
{ 
    self->okToPause = 0;
    STOP 
};

static PyObject * Linseg_pause(Linseg *self) 
{ 
    if (self->okToPause == 1)
        self->flag = 1 - self->flag;

    Py_INCREF(Py_None);
    return Py_None;
};

static PyObject * Linseg_multiply(Linseg *self, PyObject *arg) { MULTIPLY };
static PyObject * Linseg_inplace_multiply(Linseg *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Linseg_add(Linseg *self, PyObject *arg) { ADD };
static PyObject * Linseg_inplace_add(Linseg *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Linseg_sub(Linseg *self, PyObject *arg) { SUB };
static PyObject * Linseg_inplace_sub(Linseg *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Linseg_div(Linseg *self, PyObject *arg) { DIV };
static PyObject * Linseg_inplace_div(Linseg *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Linseg_setList(Linseg *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }

    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The points list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    self->newlist = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Linseg_setLoop(Linseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyInt_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Linseg_clear_data(Linseg *self) {
    int i;
    self->currentValue = 0.0;
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Linseg_members[] = {
{"server", T_OBJECT_EX, offsetof(Linseg, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Linseg, stream), 0, "Stream object."},
{"pointslist", T_OBJECT_EX, offsetof(Linseg, pointslist), 0, "List of target points."},
{"mul", T_OBJECT_EX, offsetof(Linseg, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Linseg, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Linseg_methods[] = {
{"getServer", (PyCFunction)Linseg_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Linseg_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Linseg_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Linseg_stop, METH_VARARGS|METH_KEYWORDS, "Starts fadeout and stops computing."},
{"clear", (PyCFunction)Linseg_clear_data, METH_NOARGS, "Resets the data buffer to 0."},
{"pause", (PyCFunction)Linseg_pause, METH_NOARGS, "Toggles between play and stop without reset."},
{"setList", (PyCFunction)Linseg_setList, METH_O, "Sets target points list."},
{"setLoop", (PyCFunction)Linseg_setLoop, METH_O, "Sets looping mode."},
{"setMul", (PyCFunction)Linseg_setMul, METH_O, "Sets Linseg mul factor."},
{"setAdd", (PyCFunction)Linseg_setAdd, METH_O, "Sets Linseg add factor."},
{"setSub", (PyCFunction)Linseg_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Linseg_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Linseg_as_number = {
(binaryfunc)Linseg_add,                      /*nb_add*/
(binaryfunc)Linseg_sub,                 /*nb_subtract*/
(binaryfunc)Linseg_multiply,                 /*nb_multiply*/
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
(binaryfunc)Linseg_inplace_add,              /*inplace_add*/
(binaryfunc)Linseg_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Linseg_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Linseg_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Linseg_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject LinsegType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Linseg_base",         /*tp_name*/
sizeof(Linseg),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Linseg_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Linseg_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Linseg objects. Generates a linear segments break-points line.",           /* tp_doc */
(traverseproc)Linseg_traverse,   /* tp_traverse */
(inquiry)Linseg_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Linseg_methods,             /* tp_methods */
Linseg_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Linseg_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *pointslist;
    int modebuffer[2];
    double currentTime;
    double currentValue;
    MYFLT sampleToSec;
    double inc;
    double pointer;
    MYFLT range;
    double steps;
    MYFLT *targets;
    MYFLT *times;
    int which;
    int flag;
    int newlist;
    int loop;
    int listsize;
    double exp;
    double exp_tmp;
    int inverse;
    int inverse_tmp;
    int okToPause;
} Expseg;

static void
Expseg_convert_pointslist(Expseg *self) {
    int i;
    PyObject *tup;

    self->listsize = PyList_Size(self->pointslist);
    self->targets = (MYFLT *)realloc(self->targets, self->listsize * sizeof(MYFLT));
    self->times = (MYFLT *)realloc(self->times, self->listsize * sizeof(MYFLT));
    for (i=0; i<self->listsize; i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        self->times[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
        self->targets[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
    }
}

static void
Expseg_reinit(Expseg *self) {
    if (self->newlist == 1) {
        Expseg_convert_pointslist((Expseg *)self);
        self->newlist = 0;
    }
    self->currentTime = 0.0;
    self->currentValue = self->targets[0];
    self->which = 0;
    self->flag = 1;
    self->exp = self->exp_tmp;
    self->inverse = self->inverse_tmp;
    self->okToPause = 1;
}

static void
Expseg_generate(Expseg *self) {
    int i;
    double scl;

    for (i=0; i<self->bufsize; i++) {
        if (self->flag == 1) {
            if (self->currentTime >= self->times[self->which]) {
                self->which++;
                if (self->which == self->listsize) {
                    if (self->loop == 1)
                        Expseg_reinit((Expseg *)self);
                    else {
                        self->flag = 0;
                        self->currentValue = self->targets[self->which-1];
                        self->okToPause = 0;
                    }
                }
                else {
                    self->range = self->targets[self->which] - self->targets[self->which-1];
                    self->steps = (self->times[self->which] - self->times[self->which-1]) * self->sr;
                    if (self->steps <= 0)
                        self->inc = 1.0;
                    else
                        self->inc = 1.0 / self->steps;
                }
                self->pointer = 0.0;
            }
            if (self->currentTime <= self->times[self->listsize-1]) {
                if (self->pointer >= 1.0)
                    self->pointer = 1.0;
                if (self->inverse == 1 && self->range < 0.0)
                    scl = 1.0 - pow(1.0 - self->pointer, self->exp);
                else
                    scl = pow(self->pointer, self->exp);

                self->currentValue = scl * self->range + self->targets[self->which-1];
                self->pointer += self->inc;
            }
            self->data[i] = (MYFLT)self->currentValue;
            self->currentTime += self->sampleToSec;
        }
        else
            self->data[i] = (MYFLT)self->currentValue;
    }
}

static void Expseg_postprocessing_ii(Expseg *self) { POST_PROCESSING_II };
static void Expseg_postprocessing_ai(Expseg *self) { POST_PROCESSING_AI };
static void Expseg_postprocessing_ia(Expseg *self) { POST_PROCESSING_IA };
static void Expseg_postprocessing_aa(Expseg *self) { POST_PROCESSING_AA };
static void Expseg_postprocessing_ireva(Expseg *self) { POST_PROCESSING_IREVA };
static void Expseg_postprocessing_areva(Expseg *self) { POST_PROCESSING_AREVA };
static void Expseg_postprocessing_revai(Expseg *self) { POST_PROCESSING_REVAI };
static void Expseg_postprocessing_revaa(Expseg *self) { POST_PROCESSING_REVAA };
static void Expseg_postprocessing_revareva(Expseg *self) { POST_PROCESSING_REVAREVA };

static void
Expseg_setProcMode(Expseg *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Expseg_generate;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Expseg_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Expseg_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Expseg_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Expseg_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Expseg_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Expseg_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Expseg_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Expseg_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Expseg_postprocessing_revareva;
            break;
    }
}

static void
Expseg_compute_next_data_frame(Expseg *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Expseg_traverse(Expseg *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
Expseg_clear(Expseg *self)
{
    pyo_CLEAR
    Py_CLEAR(self->pointslist);
    return 0;
}

static void
Expseg_dealloc(Expseg* self)
{
    pyo_DEALLOC
    free(self->targets);
    free(self->times);
    Expseg_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Expseg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, initToFirstVal = 0;
    PyObject *pointslist=NULL, *multmp=NULL, *addtmp=NULL;
    Expseg *self;
    self = (Expseg *)type->tp_alloc(type, 0);

    self->loop = 0;
    self->newlist = 1;
    self->exp = self->exp_tmp = 10;
    self->inverse = self->inverse_tmp = 1;
    self->okToPause = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Expseg_compute_next_data_frame);
    self->mode_func_ptr = Expseg_setProcMode;

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"list", "loop", "exp", "inverse", "initToFirstVal", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|idiiOO", kwlist, &pointslist, &self->loop, &self->exp_tmp, &self->inverse_tmp, &initToFirstVal, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_INCREF(pointslist);
    Py_XDECREF(self->pointslist);
    self->pointslist = pointslist;
    Expseg_convert_pointslist((Expseg *)self);

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (initToFirstVal) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->targets[0];
        }
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Expseg_getServer(Expseg* self) { GET_SERVER };
static PyObject * Expseg_getStream(Expseg* self) { GET_STREAM };
static PyObject * Expseg_setMul(Expseg *self, PyObject *arg) { SET_MUL };
static PyObject * Expseg_setAdd(Expseg *self, PyObject *arg) { SET_ADD };
static PyObject * Expseg_setSub(Expseg *self, PyObject *arg) { SET_SUB };
static PyObject * Expseg_setDiv(Expseg *self, PyObject *arg) { SET_DIV };

static PyObject * Expseg_play(Expseg *self, PyObject *args, PyObject *kwds)
{
    Expseg_reinit((Expseg *)self);
    PLAY
};

static PyObject * Expseg_stop(Expseg *self, PyObject *args, PyObject *kwds)
{ 
    self->okToPause = 0;
    STOP 
};

static PyObject * Expseg_pause(Expseg *self) 
{ 
    if (self->okToPause == 1)
        self->flag = 1 - self->flag;

    Py_INCREF(Py_None);
    return Py_None;
};

static PyObject * Expseg_multiply(Expseg *self, PyObject *arg) { MULTIPLY };
static PyObject * Expseg_inplace_multiply(Expseg *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Expseg_add(Expseg *self, PyObject *arg) { ADD };
static PyObject * Expseg_inplace_add(Expseg *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Expseg_sub(Expseg *self, PyObject *arg) { SUB };
static PyObject * Expseg_inplace_sub(Expseg *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Expseg_div(Expseg *self, PyObject *arg) { DIV };
static PyObject * Expseg_inplace_div(Expseg *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Expseg_setList(Expseg *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }

    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The points list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    self->newlist = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Expseg_setLoop(Expseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyInt_AsLong(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Expseg_setExp(Expseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg))
        self->exp_tmp = PyFloat_AsDouble(arg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Expseg_setInverse(Expseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	if (PyNumber_Check(arg))
        self->inverse_tmp = PyInt_AsLong(PyNumber_Int(arg));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Expseg_clear_data(Linseg *self) {
    int i;
    self->currentValue = 0.0;
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Expseg_members[] = {
    {"server", T_OBJECT_EX, offsetof(Expseg, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Expseg, stream), 0, "Stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(Expseg, pointslist), 0, "List of target points."},
    {"mul", T_OBJECT_EX, offsetof(Expseg, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Expseg, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Expseg_methods[] = {
    {"getServer", (PyCFunction)Expseg_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Expseg_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Expseg_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Expseg_stop, METH_VARARGS|METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"pause", (PyCFunction)Expseg_pause, METH_NOARGS, "Toggles between play and stop without reset."},
    {"setList", (PyCFunction)Expseg_setList, METH_O, "Sets target points list."},
    {"setLoop", (PyCFunction)Expseg_setLoop, METH_O, "Sets looping mode."},
    {"setExp", (PyCFunction)Expseg_setExp, METH_O, "Sets exponent factor."},
    {"setInverse", (PyCFunction)Expseg_setInverse, METH_O, "Sets inverse factor."},
    {"clear", (PyCFunction)Expseg_clear_data, METH_NOARGS, "Resets the data buffer to 0."},
    {"setMul", (PyCFunction)Expseg_setMul, METH_O, "Sets Expseg mul factor."},
    {"setAdd", (PyCFunction)Expseg_setAdd, METH_O, "Sets Expseg add factor."},
    {"setSub", (PyCFunction)Expseg_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Expseg_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Expseg_as_number = {
    (binaryfunc)Expseg_add,                      /*nb_add*/
    (binaryfunc)Expseg_sub,                 /*nb_subtract*/
    (binaryfunc)Expseg_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Expseg_inplace_add,              /*inplace_add*/
    (binaryfunc)Expseg_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Expseg_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Expseg_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Expseg_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject ExpsegType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Expseg_base",         /*tp_name*/
    sizeof(Expseg),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Expseg_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Expseg_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Expseg objects. Generates a linear segments break-points line.",           /* tp_doc */
    (traverseproc)Expseg_traverse,   /* tp_traverse */
    (inquiry)Expseg_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Expseg_methods,             /* tp_methods */
    Expseg_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Expseg_new,                 /* tp_new */
};
