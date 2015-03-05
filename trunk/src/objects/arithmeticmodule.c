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
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

/************/
/* M_Sin */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Sin;

static void
M_Sin_process(M_Sin *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYSIN(in[i]);
    }
}

static void M_Sin_postprocessing_ii(M_Sin *self) { POST_PROCESSING_II };
static void M_Sin_postprocessing_ai(M_Sin *self) { POST_PROCESSING_AI };
static void M_Sin_postprocessing_ia(M_Sin *self) { POST_PROCESSING_IA };
static void M_Sin_postprocessing_aa(M_Sin *self) { POST_PROCESSING_AA };
static void M_Sin_postprocessing_ireva(M_Sin *self) { POST_PROCESSING_IREVA };
static void M_Sin_postprocessing_areva(M_Sin *self) { POST_PROCESSING_AREVA };
static void M_Sin_postprocessing_revai(M_Sin *self) { POST_PROCESSING_REVAI };
static void M_Sin_postprocessing_revaa(M_Sin *self) { POST_PROCESSING_REVAA };
static void M_Sin_postprocessing_revareva(M_Sin *self) { POST_PROCESSING_REVAREVA };

static void
M_Sin_setProcMode(M_Sin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Sin_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Sin_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Sin_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Sin_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Sin_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Sin_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Sin_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Sin_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Sin_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Sin_postprocessing_revareva;
            break;
    }
}

static void
M_Sin_compute_next_data_frame(M_Sin *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Sin_traverse(M_Sin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Sin_clear(M_Sin *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Sin_dealloc(M_Sin* self)
{
    pyo_DEALLOC
    M_Sin_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Sin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Sin *self;
    self = (M_Sin *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Sin_compute_next_data_frame);
    self->mode_func_ptr = M_Sin_setProcMode;

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

static PyObject * M_Sin_getServer(M_Sin* self) { GET_SERVER };
static PyObject * M_Sin_getStream(M_Sin* self) { GET_STREAM };
static PyObject * M_Sin_setMul(M_Sin *self, PyObject *arg) { SET_MUL };
static PyObject * M_Sin_setAdd(M_Sin *self, PyObject *arg) { SET_ADD };
static PyObject * M_Sin_setSub(M_Sin *self, PyObject *arg) { SET_SUB };
static PyObject * M_Sin_setDiv(M_Sin *self, PyObject *arg) { SET_DIV };

static PyObject * M_Sin_play(M_Sin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Sin_out(M_Sin *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Sin_stop(M_Sin *self) { STOP };

static PyObject * M_Sin_multiply(M_Sin *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Sin_inplace_multiply(M_Sin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Sin_add(M_Sin *self, PyObject *arg) { ADD };
static PyObject * M_Sin_inplace_add(M_Sin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Sin_sub(M_Sin *self, PyObject *arg) { SUB };
static PyObject * M_Sin_inplace_sub(M_Sin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Sin_div(M_Sin *self, PyObject *arg) { DIV };
static PyObject * M_Sin_inplace_div(M_Sin *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Sin_members[] = {
{"server", T_OBJECT_EX, offsetof(M_Sin, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(M_Sin, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(M_Sin, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(M_Sin, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(M_Sin, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef M_Sin_methods[] = {
{"getServer", (PyCFunction)M_Sin_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)M_Sin_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)M_Sin_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)M_Sin_stop, METH_NOARGS, "Stops computing."},
{"out", (PyCFunction)M_Sin_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"setMul", (PyCFunction)M_Sin_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)M_Sin_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)M_Sin_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)M_Sin_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods M_Sin_as_number = {
(binaryfunc)M_Sin_add,                         /*nb_add*/
(binaryfunc)M_Sin_sub,                         /*nb_subtract*/
(binaryfunc)M_Sin_multiply,                    /*nb_multiply*/
(binaryfunc)M_Sin_div,                                              /*nb_divide*/
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
(binaryfunc)M_Sin_inplace_add,                 /*inplace_add*/
(binaryfunc)M_Sin_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)M_Sin_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)M_Sin_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_SinType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.M_Sin_base",                                   /*tp_name*/
sizeof(M_Sin),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)M_Sin_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&M_Sin_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"M_Sin objects. Performs sin function on audio samples.",           /* tp_doc */
(traverseproc)M_Sin_traverse,                  /* tp_traverse */
(inquiry)M_Sin_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
M_Sin_methods,                                 /* tp_methods */
M_Sin_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
M_Sin_new,                                     /* tp_new */
};

/************/
/* M_Cos */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Cos;

static void
M_Cos_process(M_Cos *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYCOS(in[i]);
    }
}

static void M_Cos_postprocessing_ii(M_Cos *self) { POST_PROCESSING_II };
static void M_Cos_postprocessing_ai(M_Cos *self) { POST_PROCESSING_AI };
static void M_Cos_postprocessing_ia(M_Cos *self) { POST_PROCESSING_IA };
static void M_Cos_postprocessing_aa(M_Cos *self) { POST_PROCESSING_AA };
static void M_Cos_postprocessing_ireva(M_Cos *self) { POST_PROCESSING_IREVA };
static void M_Cos_postprocessing_areva(M_Cos *self) { POST_PROCESSING_AREVA };
static void M_Cos_postprocessing_revai(M_Cos *self) { POST_PROCESSING_REVAI };
static void M_Cos_postprocessing_revaa(M_Cos *self) { POST_PROCESSING_REVAA };
static void M_Cos_postprocessing_revareva(M_Cos *self) { POST_PROCESSING_REVAREVA };

static void
M_Cos_setProcMode(M_Cos *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Cos_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Cos_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Cos_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Cos_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Cos_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Cos_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Cos_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Cos_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Cos_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Cos_postprocessing_revareva;
            break;
    }
}

static void
M_Cos_compute_next_data_frame(M_Cos *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Cos_traverse(M_Cos *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Cos_clear(M_Cos *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Cos_dealloc(M_Cos* self)
{
    pyo_DEALLOC
    M_Cos_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Cos_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Cos *self;
    self = (M_Cos *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Cos_compute_next_data_frame);
    self->mode_func_ptr = M_Cos_setProcMode;

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

static PyObject * M_Cos_getServer(M_Cos* self) { GET_SERVER };
static PyObject * M_Cos_getStream(M_Cos* self) { GET_STREAM };
static PyObject * M_Cos_setMul(M_Cos *self, PyObject *arg) { SET_MUL };
static PyObject * M_Cos_setAdd(M_Cos *self, PyObject *arg) { SET_ADD };
static PyObject * M_Cos_setSub(M_Cos *self, PyObject *arg) { SET_SUB };
static PyObject * M_Cos_setDiv(M_Cos *self, PyObject *arg) { SET_DIV };

static PyObject * M_Cos_play(M_Cos *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Cos_out(M_Cos *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Cos_stop(M_Cos *self) { STOP };

static PyObject * M_Cos_multiply(M_Cos *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Cos_inplace_multiply(M_Cos *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Cos_add(M_Cos *self, PyObject *arg) { ADD };
static PyObject * M_Cos_inplace_add(M_Cos *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Cos_sub(M_Cos *self, PyObject *arg) { SUB };
static PyObject * M_Cos_inplace_sub(M_Cos *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Cos_div(M_Cos *self, PyObject *arg) { DIV };
static PyObject * M_Cos_inplace_div(M_Cos *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Cos_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Cos, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Cos, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Cos, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Cos, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Cos, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Cos_methods[] = {
    {"getServer", (PyCFunction)M_Cos_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Cos_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Cos_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Cos_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Cos_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Cos_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Cos_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Cos_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Cos_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Cos_as_number = {
    (binaryfunc)M_Cos_add,                         /*nb_add*/
    (binaryfunc)M_Cos_sub,                         /*nb_subtract*/
    (binaryfunc)M_Cos_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Cos_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Cos_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Cos_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Cos_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Cos_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_CosType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Cos_base",                                   /*tp_name*/
    sizeof(M_Cos),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Cos_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Cos_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Cos objects. Performs sin function on audio samples.",           /* tp_doc */
    (traverseproc)M_Cos_traverse,                  /* tp_traverse */
    (inquiry)M_Cos_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Cos_methods,                                 /* tp_methods */
    M_Cos_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Cos_new,                                     /* tp_new */
};

/************/
/* M_Tan */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Tan;

static void
M_Tan_process(M_Tan *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYTAN(in[i]);
    }
}

static void M_Tan_postprocessing_ii(M_Tan *self) { POST_PROCESSING_II };
static void M_Tan_postprocessing_ai(M_Tan *self) { POST_PROCESSING_AI };
static void M_Tan_postprocessing_ia(M_Tan *self) { POST_PROCESSING_IA };
static void M_Tan_postprocessing_aa(M_Tan *self) { POST_PROCESSING_AA };
static void M_Tan_postprocessing_ireva(M_Tan *self) { POST_PROCESSING_IREVA };
static void M_Tan_postprocessing_areva(M_Tan *self) { POST_PROCESSING_AREVA };
static void M_Tan_postprocessing_revai(M_Tan *self) { POST_PROCESSING_REVAI };
static void M_Tan_postprocessing_revaa(M_Tan *self) { POST_PROCESSING_REVAA };
static void M_Tan_postprocessing_revareva(M_Tan *self) { POST_PROCESSING_REVAREVA };

static void
M_Tan_setProcMode(M_Tan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Tan_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Tan_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Tan_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Tan_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Tan_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Tan_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Tan_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Tan_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Tan_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Tan_postprocessing_revareva;
            break;
    }
}

static void
M_Tan_compute_next_data_frame(M_Tan *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Tan_traverse(M_Tan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Tan_clear(M_Tan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Tan_dealloc(M_Tan* self)
{
    pyo_DEALLOC
    M_Tan_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Tan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Tan *self;
    self = (M_Tan *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Tan_compute_next_data_frame);
    self->mode_func_ptr = M_Tan_setProcMode;

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

static PyObject * M_Tan_getServer(M_Tan* self) { GET_SERVER };
static PyObject * M_Tan_getStream(M_Tan* self) { GET_STREAM };
static PyObject * M_Tan_setMul(M_Tan *self, PyObject *arg) { SET_MUL };
static PyObject * M_Tan_setAdd(M_Tan *self, PyObject *arg) { SET_ADD };
static PyObject * M_Tan_setSub(M_Tan *self, PyObject *arg) { SET_SUB };
static PyObject * M_Tan_setDiv(M_Tan *self, PyObject *arg) { SET_DIV };

static PyObject * M_Tan_play(M_Tan *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Tan_out(M_Tan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Tan_stop(M_Tan *self) { STOP };

static PyObject * M_Tan_multiply(M_Tan *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Tan_inplace_multiply(M_Tan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Tan_add(M_Tan *self, PyObject *arg) { ADD };
static PyObject * M_Tan_inplace_add(M_Tan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Tan_sub(M_Tan *self, PyObject *arg) { SUB };
static PyObject * M_Tan_inplace_sub(M_Tan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Tan_div(M_Tan *self, PyObject *arg) { DIV };
static PyObject * M_Tan_inplace_div(M_Tan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Tan_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Tan, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Tan, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Tan, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Tan, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Tan, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Tan_methods[] = {
    {"getServer", (PyCFunction)M_Tan_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Tan_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Tan_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Tan_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Tan_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Tan_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Tan_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Tan_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Tan_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Tan_as_number = {
    (binaryfunc)M_Tan_add,                         /*nb_add*/
    (binaryfunc)M_Tan_sub,                         /*nb_subtract*/
    (binaryfunc)M_Tan_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Tan_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Tan_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Tan_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Tan_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Tan_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_TanType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Tan_base",                                   /*tp_name*/
    sizeof(M_Tan),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Tan_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Tan_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Tan objects. Performs sin function on audio samples.",           /* tp_doc */
    (traverseproc)M_Tan_traverse,                  /* tp_traverse */
    (inquiry)M_Tan_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Tan_methods,                                 /* tp_methods */
    M_Tan_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Tan_new,                                     /* tp_new */
};

/************/
/* M_Abs */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Abs;

static void
M_Abs_process(M_Abs *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < 0.0)
            self->data[i] = -inval;
        else
            self->data[i] = inval;
    }
}

static void M_Abs_postprocessing_ii(M_Abs *self) { POST_PROCESSING_II };
static void M_Abs_postprocessing_ai(M_Abs *self) { POST_PROCESSING_AI };
static void M_Abs_postprocessing_ia(M_Abs *self) { POST_PROCESSING_IA };
static void M_Abs_postprocessing_aa(M_Abs *self) { POST_PROCESSING_AA };
static void M_Abs_postprocessing_ireva(M_Abs *self) { POST_PROCESSING_IREVA };
static void M_Abs_postprocessing_areva(M_Abs *self) { POST_PROCESSING_AREVA };
static void M_Abs_postprocessing_revai(M_Abs *self) { POST_PROCESSING_REVAI };
static void M_Abs_postprocessing_revaa(M_Abs *self) { POST_PROCESSING_REVAA };
static void M_Abs_postprocessing_revareva(M_Abs *self) { POST_PROCESSING_REVAREVA };

static void
M_Abs_setProcMode(M_Abs *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Abs_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Abs_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Abs_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Abs_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Abs_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Abs_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Abs_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Abs_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Abs_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Abs_postprocessing_revareva;
            break;
    }
}

static void
M_Abs_compute_next_data_frame(M_Abs *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Abs_traverse(M_Abs *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Abs_clear(M_Abs *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Abs_dealloc(M_Abs* self)
{
    pyo_DEALLOC
    M_Abs_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Abs_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Abs *self;
    self = (M_Abs *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Abs_compute_next_data_frame);
    self->mode_func_ptr = M_Abs_setProcMode;

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

static PyObject * M_Abs_getServer(M_Abs* self) { GET_SERVER };
static PyObject * M_Abs_getStream(M_Abs* self) { GET_STREAM };
static PyObject * M_Abs_setMul(M_Abs *self, PyObject *arg) { SET_MUL };
static PyObject * M_Abs_setAdd(M_Abs *self, PyObject *arg) { SET_ADD };
static PyObject * M_Abs_setSub(M_Abs *self, PyObject *arg) { SET_SUB };
static PyObject * M_Abs_setDiv(M_Abs *self, PyObject *arg) { SET_DIV };

static PyObject * M_Abs_play(M_Abs *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Abs_out(M_Abs *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Abs_stop(M_Abs *self) { STOP };

static PyObject * M_Abs_multiply(M_Abs *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Abs_inplace_multiply(M_Abs *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Abs_add(M_Abs *self, PyObject *arg) { ADD };
static PyObject * M_Abs_inplace_add(M_Abs *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Abs_sub(M_Abs *self, PyObject *arg) { SUB };
static PyObject * M_Abs_inplace_sub(M_Abs *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Abs_div(M_Abs *self, PyObject *arg) { DIV };
static PyObject * M_Abs_inplace_div(M_Abs *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Abs_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Abs, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Abs, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Abs, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Abs, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Abs, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Abs_methods[] = {
    {"getServer", (PyCFunction)M_Abs_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Abs_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Abs_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Abs_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Abs_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Abs_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Abs_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Abs_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Abs_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Abs_as_number = {
    (binaryfunc)M_Abs_add,                         /*nb_add*/
    (binaryfunc)M_Abs_sub,                         /*nb_subtract*/
    (binaryfunc)M_Abs_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Abs_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Abs_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Abs_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Abs_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Abs_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_AbsType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Abs_base",                                   /*tp_name*/
    sizeof(M_Abs),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Abs_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Abs_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Abs objects. Performs abs function on audio samples.",           /* tp_doc */
    (traverseproc)M_Abs_traverse,                  /* tp_traverse */
    (inquiry)M_Abs_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Abs_methods,                                 /* tp_methods */
    M_Abs_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Abs_new,                                     /* tp_new */
};

/************/
/* M_Sqrt */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Sqrt;

static void
M_Sqrt_process(M_Sqrt *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < 0.0)
            self->data[i] = 0.0;
        else
            self->data[i] = MYSQRT(inval);
    }
}

static void M_Sqrt_postprocessing_ii(M_Sqrt *self) { POST_PROCESSING_II };
static void M_Sqrt_postprocessing_ai(M_Sqrt *self) { POST_PROCESSING_AI };
static void M_Sqrt_postprocessing_ia(M_Sqrt *self) { POST_PROCESSING_IA };
static void M_Sqrt_postprocessing_aa(M_Sqrt *self) { POST_PROCESSING_AA };
static void M_Sqrt_postprocessing_ireva(M_Sqrt *self) { POST_PROCESSING_IREVA };
static void M_Sqrt_postprocessing_areva(M_Sqrt *self) { POST_PROCESSING_AREVA };
static void M_Sqrt_postprocessing_revai(M_Sqrt *self) { POST_PROCESSING_REVAI };
static void M_Sqrt_postprocessing_revaa(M_Sqrt *self) { POST_PROCESSING_REVAA };
static void M_Sqrt_postprocessing_revareva(M_Sqrt *self) { POST_PROCESSING_REVAREVA };

static void
M_Sqrt_setProcMode(M_Sqrt *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Sqrt_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Sqrt_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Sqrt_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Sqrt_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Sqrt_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Sqrt_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Sqrt_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Sqrt_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Sqrt_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Sqrt_postprocessing_revareva;
            break;
    }
}

static void
M_Sqrt_compute_next_data_frame(M_Sqrt *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Sqrt_traverse(M_Sqrt *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Sqrt_clear(M_Sqrt *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Sqrt_dealloc(M_Sqrt* self)
{
    pyo_DEALLOC
    M_Sqrt_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Sqrt_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Sqrt *self;
    self = (M_Sqrt *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Sqrt_compute_next_data_frame);
    self->mode_func_ptr = M_Sqrt_setProcMode;

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

static PyObject * M_Sqrt_getServer(M_Sqrt* self) { GET_SERVER };
static PyObject * M_Sqrt_getStream(M_Sqrt* self) { GET_STREAM };
static PyObject * M_Sqrt_setMul(M_Sqrt *self, PyObject *arg) { SET_MUL };
static PyObject * M_Sqrt_setAdd(M_Sqrt *self, PyObject *arg) { SET_ADD };
static PyObject * M_Sqrt_setSub(M_Sqrt *self, PyObject *arg) { SET_SUB };
static PyObject * M_Sqrt_setDiv(M_Sqrt *self, PyObject *arg) { SET_DIV };

static PyObject * M_Sqrt_play(M_Sqrt *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Sqrt_out(M_Sqrt *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Sqrt_stop(M_Sqrt *self) { STOP };

static PyObject * M_Sqrt_multiply(M_Sqrt *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Sqrt_inplace_multiply(M_Sqrt *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Sqrt_add(M_Sqrt *self, PyObject *arg) { ADD };
static PyObject * M_Sqrt_inplace_add(M_Sqrt *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Sqrt_sub(M_Sqrt *self, PyObject *arg) { SUB };
static PyObject * M_Sqrt_inplace_sub(M_Sqrt *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Sqrt_div(M_Sqrt *self, PyObject *arg) { DIV };
static PyObject * M_Sqrt_inplace_div(M_Sqrt *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Sqrt_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Sqrt, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Sqrt, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Sqrt, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Sqrt, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Sqrt, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Sqrt_methods[] = {
    {"getServer", (PyCFunction)M_Sqrt_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Sqrt_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Sqrt_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Sqrt_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Sqrt_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Sqrt_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Sqrt_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Sqrt_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Sqrt_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Sqrt_as_number = {
    (binaryfunc)M_Sqrt_add,                         /*nb_add*/
    (binaryfunc)M_Sqrt_sub,                         /*nb_subtract*/
    (binaryfunc)M_Sqrt_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Sqrt_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Sqrt_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Sqrt_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Sqrt_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Sqrt_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_SqrtType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Sqrt_base",                                   /*tp_name*/
    sizeof(M_Sqrt),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Sqrt_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Sqrt_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Sqrt objects. Performs sqrt function on audio samples.",           /* tp_doc */
    (traverseproc)M_Sqrt_traverse,                  /* tp_traverse */
    (inquiry)M_Sqrt_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Sqrt_methods,                                 /* tp_methods */
    M_Sqrt_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Sqrt_new,                                     /* tp_new */
};

/************/
/* M_Log */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log;

static void
M_Log_process(M_Log *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else
            self->data[i] = MYLOG(inval);
    }
}

static void M_Log_postprocessing_ii(M_Log *self) { POST_PROCESSING_II };
static void M_Log_postprocessing_ai(M_Log *self) { POST_PROCESSING_AI };
static void M_Log_postprocessing_ia(M_Log *self) { POST_PROCESSING_IA };
static void M_Log_postprocessing_aa(M_Log *self) { POST_PROCESSING_AA };
static void M_Log_postprocessing_ireva(M_Log *self) { POST_PROCESSING_IREVA };
static void M_Log_postprocessing_areva(M_Log *self) { POST_PROCESSING_AREVA };
static void M_Log_postprocessing_revai(M_Log *self) { POST_PROCESSING_REVAI };
static void M_Log_postprocessing_revaa(M_Log *self) { POST_PROCESSING_REVAA };
static void M_Log_postprocessing_revareva(M_Log *self) { POST_PROCESSING_REVAREVA };

static void
M_Log_setProcMode(M_Log *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Log_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Log_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Log_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Log_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Log_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Log_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Log_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Log_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Log_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Log_postprocessing_revareva;
            break;
    }
}

static void
M_Log_compute_next_data_frame(M_Log *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Log_traverse(M_Log *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Log_clear(M_Log *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log_dealloc(M_Log* self)
{
    pyo_DEALLOC
    M_Log_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Log_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Log *self;
    self = (M_Log *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log_compute_next_data_frame);
    self->mode_func_ptr = M_Log_setProcMode;

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

static PyObject * M_Log_getServer(M_Log* self) { GET_SERVER };
static PyObject * M_Log_getStream(M_Log* self) { GET_STREAM };
static PyObject * M_Log_setMul(M_Log *self, PyObject *arg) { SET_MUL };
static PyObject * M_Log_setAdd(M_Log *self, PyObject *arg) { SET_ADD };
static PyObject * M_Log_setSub(M_Log *self, PyObject *arg) { SET_SUB };
static PyObject * M_Log_setDiv(M_Log *self, PyObject *arg) { SET_DIV };

static PyObject * M_Log_play(M_Log *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Log_out(M_Log *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log_stop(M_Log *self) { STOP };

static PyObject * M_Log_multiply(M_Log *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log_inplace_multiply(M_Log *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log_add(M_Log *self, PyObject *arg) { ADD };
static PyObject * M_Log_inplace_add(M_Log *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log_sub(M_Log *self, PyObject *arg) { SUB };
static PyObject * M_Log_inplace_sub(M_Log *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log_div(M_Log *self, PyObject *arg) { DIV };
static PyObject * M_Log_inplace_div(M_Log *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log_methods[] = {
    {"getServer", (PyCFunction)M_Log_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Log_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log_as_number = {
    (binaryfunc)M_Log_add,                         /*nb_add*/
    (binaryfunc)M_Log_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_LogType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log_base",                                   /*tp_name*/
    sizeof(M_Log),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log objects. Performs natural log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log_traverse,                  /* tp_traverse */
    (inquiry)M_Log_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log_methods,                                 /* tp_methods */
    M_Log_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log_new,                                     /* tp_new */
};

/************/
/* M_Log10 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log10;

static void
M_Log10_process(M_Log10 *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else
            self->data[i] = MYLOG10(inval);
    }
}

static void M_Log10_postprocessing_ii(M_Log10 *self) { POST_PROCESSING_II };
static void M_Log10_postprocessing_ai(M_Log10 *self) { POST_PROCESSING_AI };
static void M_Log10_postprocessing_ia(M_Log10 *self) { POST_PROCESSING_IA };
static void M_Log10_postprocessing_aa(M_Log10 *self) { POST_PROCESSING_AA };
static void M_Log10_postprocessing_ireva(M_Log10 *self) { POST_PROCESSING_IREVA };
static void M_Log10_postprocessing_areva(M_Log10 *self) { POST_PROCESSING_AREVA };
static void M_Log10_postprocessing_revai(M_Log10 *self) { POST_PROCESSING_REVAI };
static void M_Log10_postprocessing_revaa(M_Log10 *self) { POST_PROCESSING_REVAA };
static void M_Log10_postprocessing_revareva(M_Log10 *self) { POST_PROCESSING_REVAREVA };

static void
M_Log10_setProcMode(M_Log10 *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Log10_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Log10_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Log10_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Log10_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Log10_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Log10_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Log10_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Log10_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Log10_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Log10_postprocessing_revareva;
            break;
    }
}

static void
M_Log10_compute_next_data_frame(M_Log10 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Log10_traverse(M_Log10 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Log10_clear(M_Log10 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log10_dealloc(M_Log10* self)
{
    pyo_DEALLOC
    M_Log10_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Log10_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Log10 *self;
    self = (M_Log10 *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log10_compute_next_data_frame);
    self->mode_func_ptr = M_Log10_setProcMode;

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

static PyObject * M_Log10_getServer(M_Log10* self) { GET_SERVER };
static PyObject * M_Log10_getStream(M_Log10* self) { GET_STREAM };
static PyObject * M_Log10_setMul(M_Log10 *self, PyObject *arg) { SET_MUL };
static PyObject * M_Log10_setAdd(M_Log10 *self, PyObject *arg) { SET_ADD };
static PyObject * M_Log10_setSub(M_Log10 *self, PyObject *arg) { SET_SUB };
static PyObject * M_Log10_setDiv(M_Log10 *self, PyObject *arg) { SET_DIV };

static PyObject * M_Log10_play(M_Log10 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Log10_out(M_Log10 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log10_stop(M_Log10 *self) { STOP };

static PyObject * M_Log10_multiply(M_Log10 *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log10_inplace_multiply(M_Log10 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log10_add(M_Log10 *self, PyObject *arg) { ADD };
static PyObject * M_Log10_inplace_add(M_Log10 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log10_sub(M_Log10 *self, PyObject *arg) { SUB };
static PyObject * M_Log10_inplace_sub(M_Log10 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log10_div(M_Log10 *self, PyObject *arg) { DIV };
static PyObject * M_Log10_inplace_div(M_Log10 *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log10_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log10, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log10, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log10, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log10, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log10, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log10_methods[] = {
    {"getServer", (PyCFunction)M_Log10_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log10_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Log10_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log10_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log10_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log10_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log10_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log10_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log10_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log10_as_number = {
    (binaryfunc)M_Log10_add,                         /*nb_add*/
    (binaryfunc)M_Log10_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log10_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log10_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log10_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log10_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log10_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log10_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_Log10Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log10_base",                                   /*tp_name*/
    sizeof(M_Log10),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log10_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log10_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log10 objects. Performs base 10 log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log10_traverse,                  /* tp_traverse */
    (inquiry)M_Log10_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log10_methods,                                 /* tp_methods */
    M_Log10_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log10_new,                                     /* tp_new */
};

/************/
/* M_Log2 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log2;

static void
M_Log2_process(M_Log2 *self) {
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else
            self->data[i] = MYLOG2(inval);
    }
}

static void M_Log2_postprocessing_ii(M_Log2 *self) { POST_PROCESSING_II };
static void M_Log2_postprocessing_ai(M_Log2 *self) { POST_PROCESSING_AI };
static void M_Log2_postprocessing_ia(M_Log2 *self) { POST_PROCESSING_IA };
static void M_Log2_postprocessing_aa(M_Log2 *self) { POST_PROCESSING_AA };
static void M_Log2_postprocessing_ireva(M_Log2 *self) { POST_PROCESSING_IREVA };
static void M_Log2_postprocessing_areva(M_Log2 *self) { POST_PROCESSING_AREVA };
static void M_Log2_postprocessing_revai(M_Log2 *self) { POST_PROCESSING_REVAI };
static void M_Log2_postprocessing_revaa(M_Log2 *self) { POST_PROCESSING_REVAA };
static void M_Log2_postprocessing_revareva(M_Log2 *self) { POST_PROCESSING_REVAREVA };

static void
M_Log2_setProcMode(M_Log2 *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Log2_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Log2_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Log2_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Log2_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Log2_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Log2_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Log2_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Log2_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Log2_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Log2_postprocessing_revareva;
            break;
    }
}

static void
M_Log2_compute_next_data_frame(M_Log2 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Log2_traverse(M_Log2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Log2_clear(M_Log2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log2_dealloc(M_Log2* self)
{
    pyo_DEALLOC
    M_Log2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Log2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Log2 *self;
    self = (M_Log2 *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log2_compute_next_data_frame);
    self->mode_func_ptr = M_Log2_setProcMode;

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

static PyObject * M_Log2_getServer(M_Log2* self) { GET_SERVER };
static PyObject * M_Log2_getStream(M_Log2* self) { GET_STREAM };
static PyObject * M_Log2_setMul(M_Log2 *self, PyObject *arg) { SET_MUL };
static PyObject * M_Log2_setAdd(M_Log2 *self, PyObject *arg) { SET_ADD };
static PyObject * M_Log2_setSub(M_Log2 *self, PyObject *arg) { SET_SUB };
static PyObject * M_Log2_setDiv(M_Log2 *self, PyObject *arg) { SET_DIV };

static PyObject * M_Log2_play(M_Log2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Log2_out(M_Log2 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log2_stop(M_Log2 *self) { STOP };

static PyObject * M_Log2_multiply(M_Log2 *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log2_inplace_multiply(M_Log2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log2_add(M_Log2 *self, PyObject *arg) { ADD };
static PyObject * M_Log2_inplace_add(M_Log2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log2_sub(M_Log2 *self, PyObject *arg) { SUB };
static PyObject * M_Log2_inplace_sub(M_Log2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log2_div(M_Log2 *self, PyObject *arg) { DIV };
static PyObject * M_Log2_inplace_div(M_Log2 *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log2_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log2, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log2, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log2, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log2, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log2_methods[] = {
    {"getServer", (PyCFunction)M_Log2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log2_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Log2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log2_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log2_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log2_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log2_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log2_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log2_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log2_as_number = {
    (binaryfunc)M_Log2_add,                         /*nb_add*/
    (binaryfunc)M_Log2_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log2_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log2_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log2_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log2_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log2_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log2_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_Log2Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log2_base",                                   /*tp_name*/
    sizeof(M_Log2),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log2_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log2_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log2 objects. Performs base 2 log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log2_traverse,                  /* tp_traverse */
    (inquiry)M_Log2_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log2_methods,                                 /* tp_methods */
    M_Log2_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log2_new,                                     /* tp_new */
};

/**************/
/* M_Pow object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *base;
    Stream *base_stream;
    PyObject *exponent;
    Stream *exponent_stream;
    int modebuffer[4];
} M_Pow;

static void
M_Pow_readframes_ii(M_Pow *self) {
    int i;

    MYFLT base = PyFloat_AS_DOUBLE(self->base);
    MYFLT exp = PyFloat_AS_DOUBLE(self->exponent);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYPOW(base, exp);
    }
}

static void
M_Pow_readframes_ai(M_Pow *self) {
    int i;

    MYFLT *base = Stream_getData((Stream *)self->base_stream);
    MYFLT exp = PyFloat_AS_DOUBLE(self->exponent);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYPOW(base[i], exp);
    }
}

static void
M_Pow_readframes_ia(M_Pow *self) {
    int i;

    MYFLT base = PyFloat_AS_DOUBLE(self->base);
    MYFLT *exp = Stream_getData((Stream *)self->exponent_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYPOW(base, exp[i]);
    }
}

static void
M_Pow_readframes_aa(M_Pow *self) {
    int i;

    MYFLT *base = Stream_getData((Stream *)self->base_stream);
    MYFLT *exp = Stream_getData((Stream *)self->exponent_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYPOW(base[i], exp[i]);
    }
}

static void M_Pow_postprocessing_ii(M_Pow *self) { POST_PROCESSING_II };
static void M_Pow_postprocessing_ai(M_Pow *self) { POST_PROCESSING_AI };
static void M_Pow_postprocessing_ia(M_Pow *self) { POST_PROCESSING_IA };
static void M_Pow_postprocessing_aa(M_Pow *self) { POST_PROCESSING_AA };
static void M_Pow_postprocessing_ireva(M_Pow *self) { POST_PROCESSING_IREVA };
static void M_Pow_postprocessing_areva(M_Pow *self) { POST_PROCESSING_AREVA };
static void M_Pow_postprocessing_revai(M_Pow *self) { POST_PROCESSING_REVAI };
static void M_Pow_postprocessing_revaa(M_Pow *self) { POST_PROCESSING_REVAA };
static void M_Pow_postprocessing_revareva(M_Pow *self) { POST_PROCESSING_REVAREVA };

static void
M_Pow_setProcMode(M_Pow *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = M_Pow_readframes_ii;
            break;
        case 1:
            self->proc_func_ptr = M_Pow_readframes_ai;
            break;
        case 10:
            self->proc_func_ptr = M_Pow_readframes_ia;
            break;
        case 11:
            self->proc_func_ptr = M_Pow_readframes_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Pow_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Pow_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Pow_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Pow_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Pow_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Pow_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Pow_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Pow_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Pow_postprocessing_revareva;
            break;
    }
}

static void
M_Pow_compute_next_data_frame(M_Pow *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Pow_traverse(M_Pow *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->exponent);
    Py_VISIT(self->exponent_stream);
    Py_VISIT(self->base);
    Py_VISIT(self->base_stream);
    return 0;
}

static int
M_Pow_clear(M_Pow *self)
{
    pyo_CLEAR
    Py_CLEAR(self->exponent);
    Py_CLEAR(self->exponent_stream);
    Py_CLEAR(self->base);
    Py_CLEAR(self->base_stream);
    return 0;
}

static void
M_Pow_dealloc(M_Pow* self)
{
    pyo_DEALLOC
    M_Pow_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Pow_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *basetmp=NULL, *exponenttmp=NULL, *multmp=NULL, *addtmp=NULL;
    M_Pow *self;
    self = (M_Pow *)type->tp_alloc(type, 0);

    self->base = PyFloat_FromDouble(10);
    self->exponent = PyFloat_FromDouble(1);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Pow_compute_next_data_frame);
    self->mode_func_ptr = M_Pow_setProcMode;

    static char *kwlist[] = {"base", "exponent", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &basetmp, &exponenttmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (basetmp) {
        PyObject_CallMethod((PyObject *)self, "setBase", "O", basetmp);
    }

    if (exponenttmp) {
        PyObject_CallMethod((PyObject *)self, "setExponent", "O", exponenttmp);
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

static PyObject * M_Pow_getServer(M_Pow* self) { GET_SERVER };
static PyObject * M_Pow_getStream(M_Pow* self) { GET_STREAM };
static PyObject * M_Pow_setMul(M_Pow *self, PyObject *arg) { SET_MUL };
static PyObject * M_Pow_setAdd(M_Pow *self, PyObject *arg) { SET_ADD };
static PyObject * M_Pow_setSub(M_Pow *self, PyObject *arg) { SET_SUB };
static PyObject * M_Pow_setDiv(M_Pow *self, PyObject *arg) { SET_DIV };

static PyObject * M_Pow_play(M_Pow *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Pow_out(M_Pow *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Pow_stop(M_Pow *self) { STOP };

static PyObject * M_Pow_multiply(M_Pow *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Pow_inplace_multiply(M_Pow *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Pow_add(M_Pow *self, PyObject *arg) { ADD };
static PyObject * M_Pow_inplace_add(M_Pow *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Pow_sub(M_Pow *self, PyObject *arg) { SUB };
static PyObject * M_Pow_inplace_sub(M_Pow *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Pow_div(M_Pow *self, PyObject *arg) { DIV };
static PyObject * M_Pow_inplace_div(M_Pow *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
M_Pow_setBase(M_Pow *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->base);
	if (isNumber == 1) {
		self->base = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->base = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->base, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->base_stream);
        self->base_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
M_Pow_setExponent(M_Pow *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->exponent);
	if (isNumber == 1) {
		self->exponent = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->exponent = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->exponent, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->exponent_stream);
        self->exponent_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef M_Pow_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Pow, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Pow, stream), 0, "Stream object."},
    {"base", T_OBJECT_EX, offsetof(M_Pow, base), 0, "base composant."},
    {"exponent", T_OBJECT_EX, offsetof(M_Pow, exponent), 0, "exponent composant."},
    {"mul", T_OBJECT_EX, offsetof(M_Pow, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Pow, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Pow_methods[] = {
    {"getServer", (PyCFunction)M_Pow_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Pow_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Pow_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)M_Pow_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)M_Pow_stop, METH_NOARGS, "Stops computing."},
    {"setBase", (PyCFunction)M_Pow_setBase, METH_O, "Sets base."},
    {"setExponent", (PyCFunction)M_Pow_setExponent, METH_O, "Sets exponent."},
    {"setMul", (PyCFunction)M_Pow_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)M_Pow_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)M_Pow_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Pow_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Pow_as_number = {
    (binaryfunc)M_Pow_add,                      /*nb_add*/
    (binaryfunc)M_Pow_sub,                 /*nb_subtract*/
    (binaryfunc)M_Pow_multiply,                 /*nb_multiply*/
    (binaryfunc)M_Pow_div,                   /*nb_divide*/
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
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)M_Pow_inplace_add,              /*inplace_add*/
    (binaryfunc)M_Pow_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)M_Pow_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)M_Pow_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject M_PowType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.M_Pow_base",         /*tp_name*/
    sizeof(M_Pow),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)M_Pow_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &M_Pow_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Pow objects. Power function.",           /* tp_doc */
    (traverseproc)M_Pow_traverse,   /* tp_traverse */
    (inquiry)M_Pow_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    M_Pow_methods,             /* tp_methods */
    M_Pow_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    M_Pow_new,                 /* tp_new */
};

/**************/
/* M_Atan2 object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *b;
    Stream *b_stream;
    PyObject *a;
    Stream *a_stream;
    int modebuffer[4];
} M_Atan2;

static void
M_Atan2_readframes_ii(M_Atan2 *self) {
    int i;

    MYFLT b = PyFloat_AS_DOUBLE(self->b);
    MYFLT a = PyFloat_AS_DOUBLE(self->a);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYATAN2(b, a);
    }
}

static void
M_Atan2_readframes_ai(M_Atan2 *self) {
    int i;

    MYFLT *b = Stream_getData((Stream *)self->b_stream);
    MYFLT a = PyFloat_AS_DOUBLE(self->a);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYATAN2(b[i], a);
    }
}

static void
M_Atan2_readframes_ia(M_Atan2 *self) {
    int i;

    MYFLT b = PyFloat_AS_DOUBLE(self->b);
    MYFLT *a = Stream_getData((Stream *)self->a_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYATAN2(b, a[i]);
    }
}

static void
M_Atan2_readframes_aa(M_Atan2 *self) {
    int i;

    MYFLT *b = Stream_getData((Stream *)self->b_stream);
    MYFLT *a = Stream_getData((Stream *)self->a_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYATAN2(b[i], a[i]);
    }
}

static void M_Atan2_postprocessing_ii(M_Atan2 *self) { POST_PROCESSING_II };
static void M_Atan2_postprocessing_ai(M_Atan2 *self) { POST_PROCESSING_AI };
static void M_Atan2_postprocessing_ia(M_Atan2 *self) { POST_PROCESSING_IA };
static void M_Atan2_postprocessing_aa(M_Atan2 *self) { POST_PROCESSING_AA };
static void M_Atan2_postprocessing_ireva(M_Atan2 *self) { POST_PROCESSING_IREVA };
static void M_Atan2_postprocessing_areva(M_Atan2 *self) { POST_PROCESSING_AREVA };
static void M_Atan2_postprocessing_revai(M_Atan2 *self) { POST_PROCESSING_REVAI };
static void M_Atan2_postprocessing_revaa(M_Atan2 *self) { POST_PROCESSING_REVAA };
static void M_Atan2_postprocessing_revareva(M_Atan2 *self) { POST_PROCESSING_REVAREVA };

static void
M_Atan2_setProcMode(M_Atan2 *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:
            self->proc_func_ptr = M_Atan2_readframes_ii;
            break;
        case 1:
            self->proc_func_ptr = M_Atan2_readframes_ai;
            break;
        case 10:
            self->proc_func_ptr = M_Atan2_readframes_ia;
            break;
        case 11:
            self->proc_func_ptr = M_Atan2_readframes_aa;
            break;
    }
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Atan2_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Atan2_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Atan2_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Atan2_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Atan2_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Atan2_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Atan2_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Atan2_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Atan2_postprocessing_revareva;
            break;
    }
}

static void
M_Atan2_compute_next_data_frame(M_Atan2 *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Atan2_traverse(M_Atan2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->b);
    Py_VISIT(self->b_stream);
    Py_VISIT(self->a);
    Py_VISIT(self->a_stream);
    return 0;
}

static int
M_Atan2_clear(M_Atan2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->b);
    Py_CLEAR(self->b_stream);
    Py_CLEAR(self->a);
    Py_CLEAR(self->a_stream);
    return 0;
}

static void
M_Atan2_dealloc(M_Atan2* self)
{
    pyo_DEALLOC
    M_Atan2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Atan2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *btmp=NULL, *atmp=NULL, *multmp=NULL, *addtmp=NULL;
    M_Atan2 *self;
    self = (M_Atan2 *)type->tp_alloc(type, 0);

    self->a = PyFloat_FromDouble(1);
    self->b = PyFloat_FromDouble(1);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Atan2_compute_next_data_frame);
    self->mode_func_ptr = M_Atan2_setProcMode;

    static char *kwlist[] = {"b", "a", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &btmp, &atmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (btmp) {
        PyObject_CallMethod((PyObject *)self, "setB", "O", btmp);
    }

    if (atmp) {
        PyObject_CallMethod((PyObject *)self, "setA", "O", atmp);
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

static PyObject * M_Atan2_getServer(M_Atan2* self) { GET_SERVER };
static PyObject * M_Atan2_getStream(M_Atan2* self) { GET_STREAM };
static PyObject * M_Atan2_setMul(M_Atan2 *self, PyObject *arg) { SET_MUL };
static PyObject * M_Atan2_setAdd(M_Atan2 *self, PyObject *arg) { SET_ADD };
static PyObject * M_Atan2_setSub(M_Atan2 *self, PyObject *arg) { SET_SUB };
static PyObject * M_Atan2_setDiv(M_Atan2 *self, PyObject *arg) { SET_DIV };

static PyObject * M_Atan2_play(M_Atan2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Atan2_out(M_Atan2 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Atan2_stop(M_Atan2 *self) { STOP };

static PyObject * M_Atan2_multiply(M_Atan2 *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Atan2_inplace_multiply(M_Atan2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Atan2_add(M_Atan2 *self, PyObject *arg) { ADD };
static PyObject * M_Atan2_inplace_add(M_Atan2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Atan2_sub(M_Atan2 *self, PyObject *arg) { SUB };
static PyObject * M_Atan2_inplace_sub(M_Atan2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Atan2_div(M_Atan2 *self, PyObject *arg) { DIV };
static PyObject * M_Atan2_inplace_div(M_Atan2 *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
M_Atan2_setB(M_Atan2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->b);
	if (isNumber == 1) {
		self->b = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->b = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->b, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->b_stream);
        self->b_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
M_Atan2_setA(M_Atan2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->a);
	if (isNumber == 1) {
		self->a = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->a = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->a, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->a_stream);
        self->a_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef M_Atan2_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Atan2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Atan2, stream), 0, "Stream object."},
    {"b", T_OBJECT_EX, offsetof(M_Atan2, b), 0, "b composant."},
    {"a", T_OBJECT_EX, offsetof(M_Atan2, a), 0, "a composant."},
    {"mul", T_OBJECT_EX, offsetof(M_Atan2, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Atan2, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Atan2_methods[] = {
    {"getServer", (PyCFunction)M_Atan2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Atan2_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Atan2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)M_Atan2_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)M_Atan2_stop, METH_NOARGS, "Stops computing."},
    {"setB", (PyCFunction)M_Atan2_setB, METH_O, "Sets b."},
    {"setA", (PyCFunction)M_Atan2_setA, METH_O, "Sets a."},
    {"setMul", (PyCFunction)M_Atan2_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)M_Atan2_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)M_Atan2_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Atan2_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Atan2_as_number = {
    (binaryfunc)M_Atan2_add,                      /*nb_add*/
    (binaryfunc)M_Atan2_sub,                 /*nb_subtract*/
    (binaryfunc)M_Atan2_multiply,                 /*nb_multiply*/
    (binaryfunc)M_Atan2_div,                   /*nb_divide*/
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
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)M_Atan2_inplace_add,              /*inplace_add*/
    (binaryfunc)M_Atan2_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)M_Atan2_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)M_Atan2_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject M_Atan2Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.M_Atan2_base",         /*tp_name*/
    sizeof(M_Atan2),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)M_Atan2_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &M_Atan2_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Atan2 objects. Atan2er function.",           /* tp_doc */
    (traverseproc)M_Atan2_traverse,   /* tp_traverse */
    (inquiry)M_Atan2_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    M_Atan2_methods,             /* tp_methods */
    M_Atan2_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    M_Atan2_new,                 /* tp_new */
};

/************/
/* M_Floor */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Floor;

static void
M_Floor_process(M_Floor *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYFLOOR(in[i]);
    }
}

static void M_Floor_postprocessing_ii(M_Floor *self) { POST_PROCESSING_II };
static void M_Floor_postprocessing_ai(M_Floor *self) { POST_PROCESSING_AI };
static void M_Floor_postprocessing_ia(M_Floor *self) { POST_PROCESSING_IA };
static void M_Floor_postprocessing_aa(M_Floor *self) { POST_PROCESSING_AA };
static void M_Floor_postprocessing_ireva(M_Floor *self) { POST_PROCESSING_IREVA };
static void M_Floor_postprocessing_areva(M_Floor *self) { POST_PROCESSING_AREVA };
static void M_Floor_postprocessing_revai(M_Floor *self) { POST_PROCESSING_REVAI };
static void M_Floor_postprocessing_revaa(M_Floor *self) { POST_PROCESSING_REVAA };
static void M_Floor_postprocessing_revareva(M_Floor *self) { POST_PROCESSING_REVAREVA };

static void
M_Floor_setProcMode(M_Floor *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Floor_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Floor_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Floor_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Floor_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Floor_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Floor_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Floor_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Floor_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Floor_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Floor_postprocessing_revareva;
            break;
    }
}

static void
M_Floor_compute_next_data_frame(M_Floor *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Floor_traverse(M_Floor *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Floor_clear(M_Floor *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Floor_dealloc(M_Floor* self)
{
    pyo_DEALLOC
    M_Floor_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Floor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Floor *self;
    self = (M_Floor *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Floor_compute_next_data_frame);
    self->mode_func_ptr = M_Floor_setProcMode;

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

static PyObject * M_Floor_getServer(M_Floor* self) { GET_SERVER };
static PyObject * M_Floor_getStream(M_Floor* self) { GET_STREAM };
static PyObject * M_Floor_setMul(M_Floor *self, PyObject *arg) { SET_MUL };
static PyObject * M_Floor_setAdd(M_Floor *self, PyObject *arg) { SET_ADD };
static PyObject * M_Floor_setSub(M_Floor *self, PyObject *arg) { SET_SUB };
static PyObject * M_Floor_setDiv(M_Floor *self, PyObject *arg) { SET_DIV };

static PyObject * M_Floor_play(M_Floor *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Floor_out(M_Floor *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Floor_stop(M_Floor *self) { STOP };

static PyObject * M_Floor_multiply(M_Floor *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Floor_inplace_multiply(M_Floor *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Floor_add(M_Floor *self, PyObject *arg) { ADD };
static PyObject * M_Floor_inplace_add(M_Floor *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Floor_sub(M_Floor *self, PyObject *arg) { SUB };
static PyObject * M_Floor_inplace_sub(M_Floor *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Floor_div(M_Floor *self, PyObject *arg) { DIV };
static PyObject * M_Floor_inplace_div(M_Floor *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Floor_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Floor, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Floor, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Floor, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Floor, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Floor, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Floor_methods[] = {
    {"getServer", (PyCFunction)M_Floor_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Floor_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Floor_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Floor_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Floor_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Floor_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Floor_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Floor_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Floor_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Floor_as_number = {
    (binaryfunc)M_Floor_add,                         /*nb_add*/
    (binaryfunc)M_Floor_sub,                         /*nb_subtract*/
    (binaryfunc)M_Floor_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Floor_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Floor_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Floor_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Floor_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Floor_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_FloorType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Floor_base",                                   /*tp_name*/
    sizeof(M_Floor),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Floor_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Floor_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Floor objects. Performs sqrt function on audio samples.",           /* tp_doc */
    (traverseproc)M_Floor_traverse,                  /* tp_traverse */
    (inquiry)M_Floor_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Floor_methods,                                 /* tp_methods */
    M_Floor_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Floor_new,                                     /* tp_new */
};

/************/
/* M_Ceil */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Ceil;

static void
M_Ceil_process(M_Ceil *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYCEIL(in[i]);
    }
}

static void M_Ceil_postprocessing_ii(M_Ceil *self) { POST_PROCESSING_II };
static void M_Ceil_postprocessing_ai(M_Ceil *self) { POST_PROCESSING_AI };
static void M_Ceil_postprocessing_ia(M_Ceil *self) { POST_PROCESSING_IA };
static void M_Ceil_postprocessing_aa(M_Ceil *self) { POST_PROCESSING_AA };
static void M_Ceil_postprocessing_ireva(M_Ceil *self) { POST_PROCESSING_IREVA };
static void M_Ceil_postprocessing_areva(M_Ceil *self) { POST_PROCESSING_AREVA };
static void M_Ceil_postprocessing_revai(M_Ceil *self) { POST_PROCESSING_REVAI };
static void M_Ceil_postprocessing_revaa(M_Ceil *self) { POST_PROCESSING_REVAA };
static void M_Ceil_postprocessing_revareva(M_Ceil *self) { POST_PROCESSING_REVAREVA };

static void
M_Ceil_setProcMode(M_Ceil *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Ceil_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Ceil_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Ceil_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Ceil_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Ceil_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Ceil_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Ceil_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Ceil_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Ceil_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Ceil_postprocessing_revareva;
            break;
    }
}

static void
M_Ceil_compute_next_data_frame(M_Ceil *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Ceil_traverse(M_Ceil *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Ceil_clear(M_Ceil *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Ceil_dealloc(M_Ceil* self)
{
    pyo_DEALLOC
    M_Ceil_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Ceil_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Ceil *self;
    self = (M_Ceil *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Ceil_compute_next_data_frame);
    self->mode_func_ptr = M_Ceil_setProcMode;

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

static PyObject * M_Ceil_getServer(M_Ceil* self) { GET_SERVER };
static PyObject * M_Ceil_getStream(M_Ceil* self) { GET_STREAM };
static PyObject * M_Ceil_setMul(M_Ceil *self, PyObject *arg) { SET_MUL };
static PyObject * M_Ceil_setAdd(M_Ceil *self, PyObject *arg) { SET_ADD };
static PyObject * M_Ceil_setSub(M_Ceil *self, PyObject *arg) { SET_SUB };
static PyObject * M_Ceil_setDiv(M_Ceil *self, PyObject *arg) { SET_DIV };

static PyObject * M_Ceil_play(M_Ceil *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Ceil_out(M_Ceil *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Ceil_stop(M_Ceil *self) { STOP };

static PyObject * M_Ceil_multiply(M_Ceil *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Ceil_inplace_multiply(M_Ceil *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Ceil_add(M_Ceil *self, PyObject *arg) { ADD };
static PyObject * M_Ceil_inplace_add(M_Ceil *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Ceil_sub(M_Ceil *self, PyObject *arg) { SUB };
static PyObject * M_Ceil_inplace_sub(M_Ceil *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Ceil_div(M_Ceil *self, PyObject *arg) { DIV };
static PyObject * M_Ceil_inplace_div(M_Ceil *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Ceil_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Ceil, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Ceil, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Ceil, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Ceil, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Ceil, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Ceil_methods[] = {
    {"getServer", (PyCFunction)M_Ceil_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Ceil_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Ceil_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Ceil_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Ceil_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Ceil_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Ceil_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Ceil_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Ceil_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Ceil_as_number = {
    (binaryfunc)M_Ceil_add,                         /*nb_add*/
    (binaryfunc)M_Ceil_sub,                         /*nb_subtract*/
    (binaryfunc)M_Ceil_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Ceil_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Ceil_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Ceil_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Ceil_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Ceil_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_CeilType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Ceil_base",                                   /*tp_name*/
    sizeof(M_Ceil),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Ceil_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Ceil_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Ceil objects. Performs ceil function on audio samples.",           /* tp_doc */
    (traverseproc)M_Ceil_traverse,                  /* tp_traverse */
    (inquiry)M_Ceil_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Ceil_methods,                                 /* tp_methods */
    M_Ceil_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Ceil_new,                                     /* tp_new */
};

/************/
/* M_Round */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Round;

static void
M_Round_process(M_Round *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYROUND(in[i]);
    }
}

static void M_Round_postprocessing_ii(M_Round *self) { POST_PROCESSING_II };
static void M_Round_postprocessing_ai(M_Round *self) { POST_PROCESSING_AI };
static void M_Round_postprocessing_ia(M_Round *self) { POST_PROCESSING_IA };
static void M_Round_postprocessing_aa(M_Round *self) { POST_PROCESSING_AA };
static void M_Round_postprocessing_ireva(M_Round *self) { POST_PROCESSING_IREVA };
static void M_Round_postprocessing_areva(M_Round *self) { POST_PROCESSING_AREVA };
static void M_Round_postprocessing_revai(M_Round *self) { POST_PROCESSING_REVAI };
static void M_Round_postprocessing_revaa(M_Round *self) { POST_PROCESSING_REVAA };
static void M_Round_postprocessing_revareva(M_Round *self) { POST_PROCESSING_REVAREVA };

static void
M_Round_setProcMode(M_Round *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Round_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Round_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Round_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Round_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Round_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Round_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Round_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Round_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Round_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Round_postprocessing_revareva;
            break;
    }
}

static void
M_Round_compute_next_data_frame(M_Round *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Round_traverse(M_Round *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Round_clear(M_Round *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Round_dealloc(M_Round* self)
{
    pyo_DEALLOC
    M_Round_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Round_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Round *self;
    self = (M_Round *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Round_compute_next_data_frame);
    self->mode_func_ptr = M_Round_setProcMode;

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

static PyObject * M_Round_getServer(M_Round* self) { GET_SERVER };
static PyObject * M_Round_getStream(M_Round* self) { GET_STREAM };
static PyObject * M_Round_setMul(M_Round *self, PyObject *arg) { SET_MUL };
static PyObject * M_Round_setAdd(M_Round *self, PyObject *arg) { SET_ADD };
static PyObject * M_Round_setSub(M_Round *self, PyObject *arg) { SET_SUB };
static PyObject * M_Round_setDiv(M_Round *self, PyObject *arg) { SET_DIV };

static PyObject * M_Round_play(M_Round *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Round_out(M_Round *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Round_stop(M_Round *self) { STOP };

static PyObject * M_Round_multiply(M_Round *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Round_inplace_multiply(M_Round *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Round_add(M_Round *self, PyObject *arg) { ADD };
static PyObject * M_Round_inplace_add(M_Round *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Round_sub(M_Round *self, PyObject *arg) { SUB };
static PyObject * M_Round_inplace_sub(M_Round *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Round_div(M_Round *self, PyObject *arg) { DIV };
static PyObject * M_Round_inplace_div(M_Round *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Round_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Round, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Round, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Round, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Round, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Round, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Round_methods[] = {
    {"getServer", (PyCFunction)M_Round_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Round_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)M_Round_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Round_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Round_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Round_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Round_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Round_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Round_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Round_as_number = {
    (binaryfunc)M_Round_add,                         /*nb_add*/
    (binaryfunc)M_Round_sub,                         /*nb_subtract*/
    (binaryfunc)M_Round_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Round_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Round_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Round_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Round_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Round_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_RoundType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Round_base",                                   /*tp_name*/
    sizeof(M_Round),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Round_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Round_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Round objects. Performs sqrt function on audio samples.",           /* tp_doc */
    (traverseproc)M_Round_traverse,                  /* tp_traverse */
    (inquiry)M_Round_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Round_methods,                                 /* tp_methods */
    M_Round_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Round_new,                                     /* tp_new */
};

/************/
/* M_Tanh */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Tanh;

static void
M_Tanh_process(M_Tanh *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = MYTANH(in[i]);
    }
}

static void M_Tanh_postprocessing_ii(M_Tanh *self) { POST_PROCESSING_II };
static void M_Tanh_postprocessing_ai(M_Tanh *self) { POST_PROCESSING_AI };
static void M_Tanh_postprocessing_ia(M_Tanh *self) { POST_PROCESSING_IA };
static void M_Tanh_postprocessing_aa(M_Tanh *self) { POST_PROCESSING_AA };
static void M_Tanh_postprocessing_ireva(M_Tanh *self) { POST_PROCESSING_IREVA };
static void M_Tanh_postprocessing_areva(M_Tanh *self) { POST_PROCESSING_AREVA };
static void M_Tanh_postprocessing_revai(M_Tanh *self) { POST_PROCESSING_REVAI };
static void M_Tanh_postprocessing_revaa(M_Tanh *self) { POST_PROCESSING_REVAA };
static void M_Tanh_postprocessing_revareva(M_Tanh *self) { POST_PROCESSING_REVAREVA };

static void
M_Tanh_setProcMode(M_Tanh *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = M_Tanh_process;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = M_Tanh_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = M_Tanh_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = M_Tanh_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = M_Tanh_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = M_Tanh_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = M_Tanh_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = M_Tanh_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = M_Tanh_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = M_Tanh_postprocessing_revareva;
            break;
    }
}

static void
M_Tanh_compute_next_data_frame(M_Tanh *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
M_Tanh_traverse(M_Tanh *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
M_Tanh_clear(M_Tanh *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Tanh_dealloc(M_Tanh* self)
{
    pyo_DEALLOC
    M_Tanh_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
M_Tanh_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    M_Tanh *self;
    self = (M_Tanh *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Tanh_compute_next_data_frame);
    self->mode_func_ptr = M_Tanh_setProcMode;

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

static PyObject * M_Tanh_getServer(M_Tanh* self) { GET_SERVER };
static PyObject * M_Tanh_getStream(M_Tanh* self) { GET_STREAM };
static PyObject * M_Tanh_setMul(M_Tanh *self, PyObject *arg) { SET_MUL };
static PyObject * M_Tanh_setAdd(M_Tanh *self, PyObject *arg) { SET_ADD };
static PyObject * M_Tanh_setSub(M_Tanh *self, PyObject *arg) { SET_SUB };
static PyObject * M_Tanh_setDiv(M_Tanh *self, PyObject *arg) { SET_DIV };

static PyObject * M_Tanh_play(M_Tanh *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * M_Tanh_out(M_Tanh *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Tanh_stop(M_Tanh *self) { STOP };

static PyObject * M_Tanh_multiply(M_Tanh *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Tanh_inplace_multiply(M_Tanh *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Tanh_add(M_Tanh *self, PyObject *arg) { ADD };
static PyObject * M_Tanh_inplace_add(M_Tanh *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Tanh_sub(M_Tanh *self, PyObject *arg) { SUB };
static PyObject * M_Tanh_inplace_sub(M_Tanh *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Tanh_div(M_Tanh *self, PyObject *arg) { DIV };
static PyObject * M_Tanh_inplace_div(M_Tanh *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Tanh_members[] = {
{"server", T_OBJECT_EX, offsetof(M_Tanh, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(M_Tanh, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(M_Tanh, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(M_Tanh, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(M_Tanh, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef M_Tanh_methods[] = {
{"getServer", (PyCFunction)M_Tanh_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)M_Tanh_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)M_Tanh_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)M_Tanh_stop, METH_NOARGS, "Stops computing."},
{"out", (PyCFunction)M_Tanh_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"setMul", (PyCFunction)M_Tanh_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)M_Tanh_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)M_Tanh_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)M_Tanh_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods M_Tanh_as_number = {
(binaryfunc)M_Tanh_add,                         /*nb_add*/
(binaryfunc)M_Tanh_sub,                         /*nb_subtract*/
(binaryfunc)M_Tanh_multiply,                    /*nb_multiply*/
(binaryfunc)M_Tanh_div,                                              /*nb_divide*/
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
(binaryfunc)M_Tanh_inplace_add,                 /*inplace_add*/
(binaryfunc)M_Tanh_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)M_Tanh_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)M_Tanh_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_TanhType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.M_Tanh_base",                                   /*tp_name*/
sizeof(M_Tanh),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)M_Tanh_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&M_Tanh_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"M_Tanh objects. Performs tanh function on audio samples.",           /* tp_doc */
(traverseproc)M_Tanh_traverse,                  /* tp_traverse */
(inquiry)M_Tanh_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
M_Tanh_methods,                                 /* tp_methods */
M_Tanh_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
M_Tanh_new,                                     /* tp_new */
};