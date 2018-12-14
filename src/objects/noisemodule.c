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
    int seed;
    int type;
} Noise;

static void
Noise_generate(Noise *self) {
    int i;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = RANDOM_UNIFORM * 1.98 - 0.99;
    }
}

static void
Noise_generate_cheap(Noise *self) {
    int i;

    for (i=0; i<self->bufsize; i++) {
        self->seed = (self->seed * 15625 + 1) & 0xFFFF;
        self->data[i] = (self->seed - 0x8000) * 3.0517578125e-05;
    }
}

static void Noise_postprocessing_ii(Noise *self) { POST_PROCESSING_II };
static void Noise_postprocessing_ai(Noise *self) { POST_PROCESSING_AI };
static void Noise_postprocessing_ia(Noise *self) { POST_PROCESSING_IA };
static void Noise_postprocessing_aa(Noise *self) { POST_PROCESSING_AA };
static void Noise_postprocessing_ireva(Noise *self) { POST_PROCESSING_IREVA };
static void Noise_postprocessing_areva(Noise *self) { POST_PROCESSING_AREVA };
static void Noise_postprocessing_revai(Noise *self) { POST_PROCESSING_REVAI };
static void Noise_postprocessing_revaa(Noise *self) { POST_PROCESSING_REVAA };
static void Noise_postprocessing_revareva(Noise *self) { POST_PROCESSING_REVAREVA };

static void
Noise_setProcMode(Noise *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->type) {
        case 0:
            self->proc_func_ptr = Noise_generate;
            break;
        case 1:
            self->proc_func_ptr = Noise_generate_cheap;
            break;
    }
    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Noise_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Noise_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Noise_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Noise_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Noise_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Noise_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Noise_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Noise_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Noise_postprocessing_revareva;
            break;
    }
}

static void
Noise_compute_next_data_frame(Noise *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Noise_traverse(Noise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Noise_clear(Noise *self)
{
    pyo_CLEAR
    return 0;
}

static void
Noise_dealloc(Noise* self)
{
    pyo_DEALLOC
    Noise_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Noise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    Noise *self;
    self = (Noise *)type->tp_alloc(type, 0);

    self->type = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Noise_compute_next_data_frame);
    self->mode_func_ptr = Noise_setProcMode;

    static char *kwlist[] = {"mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Server_generateSeed((Server *)self->server, NOISE_ID);

    self->seed = pyorand();

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Noise_getServer(Noise* self) { GET_SERVER };
static PyObject * Noise_getStream(Noise* self) { GET_STREAM };
static PyObject * Noise_setMul(Noise *self, PyObject *arg) { SET_MUL };
static PyObject * Noise_setAdd(Noise *self, PyObject *arg) { SET_ADD };
static PyObject * Noise_setSub(Noise *self, PyObject *arg) { SET_SUB };
static PyObject * Noise_setDiv(Noise *self, PyObject *arg) { SET_DIV };

static PyObject * Noise_play(Noise *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Noise_out(Noise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Noise_stop(Noise *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Noise_multiply(Noise *self, PyObject *arg) { MULTIPLY };
static PyObject * Noise_inplace_multiply(Noise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Noise_add(Noise *self, PyObject *arg) { ADD };
static PyObject * Noise_inplace_add(Noise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Noise_sub(Noise *self, PyObject *arg) { SUB };
static PyObject * Noise_inplace_sub(Noise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Noise_div(Noise *self, PyObject *arg) { DIV };
static PyObject * Noise_inplace_div(Noise *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Noise_setType(Noise *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyInt_AS_LONG(arg) == 0)
        self->type = 0;
    else if (PyInt_AS_LONG(arg) == 1)
        self->type = 1;

    (*self->mode_func_ptr)(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Noise_members[] = {
{"server", T_OBJECT_EX, offsetof(Noise, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Noise, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Noise, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Noise, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Noise_methods[] = {
{"getServer", (PyCFunction)Noise_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Noise_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Noise_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Noise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Noise_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setType", (PyCFunction)Noise_setType, METH_O, "Sets Noise generation algorithm."},
{"setMul", (PyCFunction)Noise_setMul, METH_O, "Sets Noise mul factor."},
{"setAdd", (PyCFunction)Noise_setAdd, METH_O, "Sets Noise add factor."},
{"setSub", (PyCFunction)Noise_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Noise_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Noise_as_number = {
(binaryfunc)Noise_add,                      /*nb_add*/
(binaryfunc)Noise_sub,                 /*nb_subtract*/
(binaryfunc)Noise_multiply,                 /*nb_multiply*/
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
(binaryfunc)Noise_inplace_add,              /*inplace_add*/
(binaryfunc)Noise_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Noise_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)Noise_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)Noise_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject NoiseType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Noise_base",         /*tp_name*/
sizeof(Noise),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Noise_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Noise_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Noise objects. White noise generator.",           /* tp_doc */
(traverseproc)Noise_traverse,   /* tp_traverse */
(inquiry)Noise_clear,           /* tp_clear */
0,                         /* tp_richcompare */
0,                         /* tp_weaklistoffset */
0,                         /* tp_iter */
0,                         /* tp_iternext */
Noise_methods,             /* tp_methods */
Noise_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Noise_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    int modebuffer[2];
    MYFLT c0;
    MYFLT c1;
    MYFLT c2;
    MYFLT c3;
    MYFLT c4;
    MYFLT c5;
    MYFLT c6;
} PinkNoise;

static void
PinkNoise_generate(PinkNoise *self) {
    MYFLT in, val;
    int i;

    for (i=0; i<self->bufsize; i++) {
        in = RANDOM_UNIFORM * 1.98 - 0.99;
        self->c0 = self->c0 * 0.99886 + in * 0.0555179;
        self->c1 = self->c1 * 0.99332 + in * 0.0750759;
        self->c2 = self->c2 * 0.96900 + in * 0.1538520;
        self->c3 = self->c3 * 0.86650 + in * 0.3104856;
        self->c4 = self->c4 * 0.55000 + in * 0.5329522;
        self->c5 = self->c5 * -0.7616 - in * 0.0168980;
        val = self->c0 + self->c1 + self->c2 + self->c3 + self->c4 + self->c5 + self->c6 + in * 0.5362;
        self->data[i] = val * 0.2;
        self->c6 = in * 0.115926;
    }
}

static void PinkNoise_postprocessing_ii(PinkNoise *self) { POST_PROCESSING_II };
static void PinkNoise_postprocessing_ai(PinkNoise *self) { POST_PROCESSING_AI };
static void PinkNoise_postprocessing_ia(PinkNoise *self) { POST_PROCESSING_IA };
static void PinkNoise_postprocessing_aa(PinkNoise *self) { POST_PROCESSING_AA };
static void PinkNoise_postprocessing_ireva(PinkNoise *self) { POST_PROCESSING_IREVA };
static void PinkNoise_postprocessing_areva(PinkNoise *self) { POST_PROCESSING_AREVA };
static void PinkNoise_postprocessing_revai(PinkNoise *self) { POST_PROCESSING_REVAI };
static void PinkNoise_postprocessing_revaa(PinkNoise *self) { POST_PROCESSING_REVAA };
static void PinkNoise_postprocessing_revareva(PinkNoise *self) { POST_PROCESSING_REVAREVA };

static void
PinkNoise_setProcMode(PinkNoise *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = PinkNoise_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = PinkNoise_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = PinkNoise_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = PinkNoise_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = PinkNoise_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = PinkNoise_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = PinkNoise_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = PinkNoise_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = PinkNoise_postprocessing_revareva;
            break;
    }
}

static void
PinkNoise_compute_next_data_frame(PinkNoise *self)
{
    PinkNoise_generate(self);
    (*self->muladd_func_ptr)(self);
}

static int
PinkNoise_traverse(PinkNoise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
PinkNoise_clear(PinkNoise *self)
{
    pyo_CLEAR
    return 0;
}

static void
PinkNoise_dealloc(PinkNoise* self)
{
    pyo_DEALLOC
    PinkNoise_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PinkNoise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    PinkNoise *self;
    self = (PinkNoise *)type->tp_alloc(type, 0);

    self->c0 = self->c1 = self->c2 = self->c3 = self->c4 = self->c5 = self->c6 = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PinkNoise_compute_next_data_frame);
    self->mode_func_ptr = PinkNoise_setProcMode;

    static char *kwlist[] = {"mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    Server_generateSeed((Server *)self->server, PINKNOISE_ID);

    return (PyObject *)self;
}

static PyObject * PinkNoise_getServer(PinkNoise* self) { GET_SERVER };
static PyObject * PinkNoise_getStream(PinkNoise* self) { GET_STREAM };
static PyObject * PinkNoise_setMul(PinkNoise *self, PyObject *arg) { SET_MUL };
static PyObject * PinkNoise_setAdd(PinkNoise *self, PyObject *arg) { SET_ADD };
static PyObject * PinkNoise_setSub(PinkNoise *self, PyObject *arg) { SET_SUB };
static PyObject * PinkNoise_setDiv(PinkNoise *self, PyObject *arg) { SET_DIV };

static PyObject * PinkNoise_play(PinkNoise *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PinkNoise_out(PinkNoise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * PinkNoise_stop(PinkNoise *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * PinkNoise_multiply(PinkNoise *self, PyObject *arg) { MULTIPLY };
static PyObject * PinkNoise_inplace_multiply(PinkNoise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PinkNoise_add(PinkNoise *self, PyObject *arg) { ADD };
static PyObject * PinkNoise_inplace_add(PinkNoise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PinkNoise_sub(PinkNoise *self, PyObject *arg) { SUB };
static PyObject * PinkNoise_inplace_sub(PinkNoise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PinkNoise_div(PinkNoise *self, PyObject *arg) { DIV };
static PyObject * PinkNoise_inplace_div(PinkNoise *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef PinkNoise_members[] = {
    {"server", T_OBJECT_EX, offsetof(PinkNoise, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(PinkNoise, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(PinkNoise, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(PinkNoise, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef PinkNoise_methods[] = {
    {"getServer", (PyCFunction)PinkNoise_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)PinkNoise_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)PinkNoise_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)PinkNoise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)PinkNoise_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)PinkNoise_setMul, METH_O, "Sets PinkNoise mul factor."},
    {"setAdd", (PyCFunction)PinkNoise_setAdd, METH_O, "Sets PinkNoise add factor."},
    {"setSub", (PyCFunction)PinkNoise_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)PinkNoise_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PinkNoise_as_number = {
    (binaryfunc)PinkNoise_add,                      /*nb_add*/
    (binaryfunc)PinkNoise_sub,                 /*nb_subtract*/
    (binaryfunc)PinkNoise_multiply,                 /*nb_multiply*/
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
    (binaryfunc)PinkNoise_inplace_add,              /*inplace_add*/
    (binaryfunc)PinkNoise_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)PinkNoise_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)PinkNoise_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)PinkNoise_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject PinkNoiseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PinkNoise_base",         /*tp_name*/
    sizeof(PinkNoise),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PinkNoise_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &PinkNoise_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "PinkNoise objects. Pink noise generator.",           /* tp_doc */
    (traverseproc)PinkNoise_traverse,   /* tp_traverse */
    (inquiry)PinkNoise_clear,           /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    PinkNoise_methods,             /* tp_methods */
    PinkNoise_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    PinkNoise_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    int modebuffer[2];
    MYFLT y1;
    MYFLT c;
} BrownNoise;

static void
BrownNoise_generate(BrownNoise *self) {
    MYFLT rnd;
    int i;

    for (i=0; i<self->bufsize; i++) {
        rnd = RANDOM_UNIFORM * 1.98 - 0.99;
        self->y1 = rnd + (self->y1 - rnd) * self->c;
        self->data[i] = self->y1 * 20.0; /* gain compensation */
    }
}

static void BrownNoise_postprocessing_ii(BrownNoise *self) { POST_PROCESSING_II };
static void BrownNoise_postprocessing_ai(BrownNoise *self) { POST_PROCESSING_AI };
static void BrownNoise_postprocessing_ia(BrownNoise *self) { POST_PROCESSING_IA };
static void BrownNoise_postprocessing_aa(BrownNoise *self) { POST_PROCESSING_AA };
static void BrownNoise_postprocessing_ireva(BrownNoise *self) { POST_PROCESSING_IREVA };
static void BrownNoise_postprocessing_areva(BrownNoise *self) { POST_PROCESSING_AREVA };
static void BrownNoise_postprocessing_revai(BrownNoise *self) { POST_PROCESSING_REVAI };
static void BrownNoise_postprocessing_revaa(BrownNoise *self) { POST_PROCESSING_REVAA };
static void BrownNoise_postprocessing_revareva(BrownNoise *self) { POST_PROCESSING_REVAREVA };

static void
BrownNoise_setProcMode(BrownNoise *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = BrownNoise_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = BrownNoise_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = BrownNoise_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = BrownNoise_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = BrownNoise_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = BrownNoise_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = BrownNoise_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = BrownNoise_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = BrownNoise_postprocessing_revareva;
            break;
    }
}

static void
BrownNoise_compute_next_data_frame(BrownNoise *self)
{
    BrownNoise_generate(self);
    (*self->muladd_func_ptr)(self);
}

static int
BrownNoise_traverse(BrownNoise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
BrownNoise_clear(BrownNoise *self)
{
    pyo_CLEAR
    return 0;
}

static void
BrownNoise_dealloc(BrownNoise* self)
{
    pyo_DEALLOC
    BrownNoise_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
BrownNoise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT b;
    PyObject *multmp=NULL, *addtmp=NULL;
    BrownNoise *self;
    self = (BrownNoise *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->y1 = self->c = 0.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BrownNoise_compute_next_data_frame);
    self->mode_func_ptr = BrownNoise_setProcMode;

    static char *kwlist[] = {"mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    b = 2.0 - MYCOS(TWOPI * 20.0 / self->sr);
    self->c = (b - MYSQRT(b * b - 1.0));

    (*self->mode_func_ptr)(self);

    Server_generateSeed((Server *)self->server, BROWNNOISE_ID);

    return (PyObject *)self;
}

static PyObject * BrownNoise_getServer(BrownNoise* self) { GET_SERVER };
static PyObject * BrownNoise_getStream(BrownNoise* self) { GET_STREAM };
static PyObject * BrownNoise_setMul(BrownNoise *self, PyObject *arg) { SET_MUL };
static PyObject * BrownNoise_setAdd(BrownNoise *self, PyObject *arg) { SET_ADD };
static PyObject * BrownNoise_setSub(BrownNoise *self, PyObject *arg) { SET_SUB };
static PyObject * BrownNoise_setDiv(BrownNoise *self, PyObject *arg) { SET_DIV };

static PyObject * BrownNoise_play(BrownNoise *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BrownNoise_out(BrownNoise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BrownNoise_stop(BrownNoise *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * BrownNoise_multiply(BrownNoise *self, PyObject *arg) { MULTIPLY };
static PyObject * BrownNoise_inplace_multiply(BrownNoise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BrownNoise_add(BrownNoise *self, PyObject *arg) { ADD };
static PyObject * BrownNoise_inplace_add(BrownNoise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BrownNoise_sub(BrownNoise *self, PyObject *arg) { SUB };
static PyObject * BrownNoise_inplace_sub(BrownNoise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BrownNoise_div(BrownNoise *self, PyObject *arg) { DIV };
static PyObject * BrownNoise_inplace_div(BrownNoise *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BrownNoise_members[] = {
    {"server", T_OBJECT_EX, offsetof(BrownNoise, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(BrownNoise, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(BrownNoise, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(BrownNoise, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef BrownNoise_methods[] = {
    {"getServer", (PyCFunction)BrownNoise_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)BrownNoise_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)BrownNoise_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)BrownNoise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)BrownNoise_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)BrownNoise_setMul, METH_O, "Sets BrownNoise mul factor."},
    {"setAdd", (PyCFunction)BrownNoise_setAdd, METH_O, "Sets BrownNoise add factor."},
    {"setSub", (PyCFunction)BrownNoise_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)BrownNoise_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods BrownNoise_as_number = {
    (binaryfunc)BrownNoise_add,                      /*nb_add*/
    (binaryfunc)BrownNoise_sub,                 /*nb_subtract*/
    (binaryfunc)BrownNoise_multiply,                 /*nb_multiply*/
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
    (binaryfunc)BrownNoise_inplace_add,              /*inplace_add*/
    (binaryfunc)BrownNoise_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)BrownNoise_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)BrownNoise_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)BrownNoise_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject BrownNoiseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.BrownNoise_base",         /*tp_name*/
    sizeof(BrownNoise),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)BrownNoise_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &BrownNoise_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "BrownNoise objects. Brown noise generator (-6dB/octave rolloff).",           /* tp_doc */
    (traverseproc)BrownNoise_traverse,   /* tp_traverse */
    (inquiry)BrownNoise_clear,           /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    BrownNoise_methods,             /* tp_methods */
    BrownNoise_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    BrownNoise_new,                 /* tp_new */
};
