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

static void Dummy_postprocessing_ii(Dummy *self) { POST_PROCESSING_II };
static void Dummy_postprocessing_ai(Dummy *self) { POST_PROCESSING_AI };
static void Dummy_postprocessing_ia(Dummy *self) { POST_PROCESSING_IA };
static void Dummy_postprocessing_aa(Dummy *self) { POST_PROCESSING_AA };
static void Dummy_postprocessing_ireva(Dummy *self) { POST_PROCESSING_IREVA };
static void Dummy_postprocessing_areva(Dummy *self) { POST_PROCESSING_AREVA };
static void Dummy_postprocessing_revai(Dummy *self) { POST_PROCESSING_REVAI };
static void Dummy_postprocessing_revaa(Dummy *self) { POST_PROCESSING_REVAA };
static void Dummy_postprocessing_revareva(Dummy *self) { POST_PROCESSING_REVAREVA };

static void
Dummy_setProcMode(Dummy *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Dummy_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Dummy_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Dummy_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Dummy_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Dummy_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Dummy_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Dummy_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Dummy_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Dummy_postprocessing_revareva;
            break;
    }
}

static void
Dummy_compute_next_data_frame(Dummy *self)
{
    int i;
    if (self->modebuffer[2] == 0) {
        MYFLT inval = PyFloat_AS_DOUBLE(self->input);
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = inval;
        }
    } else {
        MYFLT *in = Stream_getData((Stream *)self->input_stream);
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i];
        }
    }
    (*self->muladd_func_ptr)(self);
}

static int
Dummy_traverse(Dummy *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Dummy_clear(Dummy *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Dummy_dealloc(Dummy* self)
{
    pyo_DEALLOC
    Dummy_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject *
Dummy_initialize(Dummy *self)
{
    int i;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Dummy_compute_next_data_frame);
    self->mode_func_ptr = Dummy_setProcMode;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Stream_setStreamActive(self->stream, 1);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Dummy_setInput(Dummy *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_XDECREF(self->input);

    if (isNumber == 1) {
        self->input = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    } else {
        self->input = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input_stream);
        self->input_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

    (*self->mode_func_ptr)(self);

    Dummy_compute_next_data_frame(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * Dummy_getServer(Dummy* self) { GET_SERVER };
static PyObject * Dummy_getStream(Dummy* self) { GET_STREAM };
static PyObject * Dummy_setMul(Dummy *self, PyObject *arg) { SET_MUL };
static PyObject * Dummy_setAdd(Dummy *self, PyObject *arg) { SET_ADD };
static PyObject * Dummy_setSub(Dummy *self, PyObject *arg) { SET_SUB };
static PyObject * Dummy_setDiv(Dummy *self, PyObject *arg) { SET_DIV };

static PyObject * Dummy_play(Dummy *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Dummy_out(Dummy *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Dummy_stop(Dummy *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Dummy_multiply(Dummy *self, PyObject *arg) { MULTIPLY };
static PyObject * Dummy_inplace_multiply(Dummy *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Dummy_add(Dummy *self, PyObject *arg) { ADD };
static PyObject * Dummy_inplace_add(Dummy *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Dummy_sub(Dummy *self, PyObject *arg) { SUB };
static PyObject * Dummy_inplace_sub(Dummy *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Dummy_div(Dummy *self, PyObject *arg) { DIV };
static PyObject * Dummy_inplace_div(Dummy *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Dummy_members[] = {
    {"server", T_OBJECT_EX, offsetof(Dummy, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Dummy, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Dummy, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Dummy, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Dummy, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Dummy_methods[] = {
    {"getServer", (PyCFunction)Dummy_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Dummy_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Dummy_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Dummy_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Dummy_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setInput", (PyCFunction)Dummy_setInput, METH_O, "Sets the input sound object."},
	{"setMul", (PyCFunction)Dummy_setMul, METH_O, "Sets mul factor."},
	{"setAdd", (PyCFunction)Dummy_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Dummy_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Dummy_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Dummy_as_number = {
    (binaryfunc)Dummy_add,                         /*nb_add*/
    (binaryfunc)Dummy_sub,                         /*nb_subtract*/
    (binaryfunc)Dummy_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO   /*nb_divide*/
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
    (binaryfunc)Dummy_inplace_add,                 /*inplace_add*/
    (binaryfunc)Dummy_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Dummy_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO               /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Dummy_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Dummy_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject DummyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Dummy_base",                                   /*tp_name*/
    sizeof(Dummy),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Dummy_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Dummy_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Dummy objects.",                               /* tp_doc */
    (traverseproc)Dummy_traverse,                  /* tp_traverse */
    (inquiry)Dummy_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Dummy_methods,                                 /* tp_methods */
    Dummy_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    0,                                     /* tp_new */
};

/************************************************************************************************/
/* TriggerDummy streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    TriggerStream *input_stream;
    int modebuffer[2];
} TriggerDummy;

static void TriggerDummy_postprocessing_ii(TriggerDummy *self) { POST_PROCESSING_II };
static void TriggerDummy_postprocessing_ai(TriggerDummy *self) { POST_PROCESSING_AI };
static void TriggerDummy_postprocessing_ia(TriggerDummy *self) { POST_PROCESSING_IA };
static void TriggerDummy_postprocessing_aa(TriggerDummy *self) { POST_PROCESSING_AA };
static void TriggerDummy_postprocessing_ireva(TriggerDummy *self) { POST_PROCESSING_IREVA };
static void TriggerDummy_postprocessing_areva(TriggerDummy *self) { POST_PROCESSING_AREVA };
static void TriggerDummy_postprocessing_revai(TriggerDummy *self) { POST_PROCESSING_REVAI };
static void TriggerDummy_postprocessing_revaa(TriggerDummy *self) { POST_PROCESSING_REVAA };
static void TriggerDummy_postprocessing_revareva(TriggerDummy *self) { POST_PROCESSING_REVAREVA };

static void
TriggerDummy_setProcMode(TriggerDummy *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TriggerDummy_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TriggerDummy_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TriggerDummy_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TriggerDummy_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TriggerDummy_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TriggerDummy_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TriggerDummy_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TriggerDummy_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TriggerDummy_postprocessing_revareva;
            break;
    }
}

static void
TriggerDummy_compute_next_data_frame(TriggerDummy *self)
{
    int i;
    MYFLT *tmp = TriggerStream_getData((TriggerStream *)self->input_stream);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TriggerDummy_traverse(TriggerDummy *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
TriggerDummy_clear(TriggerDummy *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
TriggerDummy_dealloc(TriggerDummy* self)
{
    pyo_DEALLOC
    TriggerDummy_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TriggerDummy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    TriggerDummy *self;
    self = (TriggerDummy *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TriggerDummy_compute_next_data_frame);
    self->mode_func_ptr = TriggerDummy_setProcMode;

    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp))
        Py_RETURN_NONE;

    INIT_INPUT_TRIGGER_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TriggerDummy_getServer(TriggerDummy* self) { GET_SERVER };
static PyObject * TriggerDummy_getStream(TriggerDummy* self) { GET_STREAM };
static PyObject * TriggerDummy_setMul(TriggerDummy *self, PyObject *arg) { SET_MUL };
static PyObject * TriggerDummy_setAdd(TriggerDummy *self, PyObject *arg) { SET_ADD };
static PyObject * TriggerDummy_setSub(TriggerDummy *self, PyObject *arg) { SET_SUB };
static PyObject * TriggerDummy_setDiv(TriggerDummy *self, PyObject *arg) { SET_DIV };

static PyObject * TriggerDummy_play(TriggerDummy *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TriggerDummy_stop(TriggerDummy *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TriggerDummy_multiply(TriggerDummy *self, PyObject *arg) { MULTIPLY };
static PyObject * TriggerDummy_inplace_multiply(TriggerDummy *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TriggerDummy_add(TriggerDummy *self, PyObject *arg) { ADD };
static PyObject * TriggerDummy_inplace_add(TriggerDummy *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TriggerDummy_sub(TriggerDummy *self, PyObject *arg) { SUB };
static PyObject * TriggerDummy_inplace_sub(TriggerDummy *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TriggerDummy_div(TriggerDummy *self, PyObject *arg) { DIV };
static PyObject * TriggerDummy_inplace_div(TriggerDummy *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TriggerDummy_members[] = {
    {"server", T_OBJECT_EX, offsetof(TriggerDummy, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TriggerDummy, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TriggerDummy, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TriggerDummy, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TriggerDummy_methods[] = {
    {"getServer", (PyCFunction)TriggerDummy_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TriggerDummy_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TriggerDummy_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TriggerDummy_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)TriggerDummy_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TriggerDummy_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TriggerDummy_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TriggerDummy_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};
static PyNumberMethods TriggerDummy_as_number = {
    (binaryfunc)TriggerDummy_add,                         /*nb_add*/
    (binaryfunc)TriggerDummy_sub,                         /*nb_subtract*/
    (binaryfunc)TriggerDummy_multiply,                    /*nb_multiply*/
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
    (binaryfunc)TriggerDummy_inplace_add,                 /*inplace_add*/
    (binaryfunc)TriggerDummy_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TriggerDummy_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)TriggerDummy_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)TriggerDummy_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TriggerDummyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TriggerDummy_base",         /*tp_name*/
    sizeof(TriggerDummy),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TriggerDummy_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &TriggerDummy_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TriggerDummy objects. Sends trigger at the end of playback.",           /* tp_doc */
    (traverseproc)TriggerDummy_traverse,   /* tp_traverse */
    (inquiry)TriggerDummy_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TriggerDummy_methods,             /* tp_methods */
    TriggerDummy_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TriggerDummy_new,                 /* tp_new */
};