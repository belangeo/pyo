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
    PyObject *input;
    Stream *input_stream;
    long value;
    MYFLT last_value;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Select;

static void
Select_selector(Select *self) {
    MYFLT val, inval;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval == self->value && inval != self->last_value)
            val = 1;
        else
            val = 0;

        self->last_value = inval;
        self->data[i] = val;
    }
}

static void Select_postprocessing_ii(Select *self) { POST_PROCESSING_II };
static void Select_postprocessing_ai(Select *self) { POST_PROCESSING_AI };
static void Select_postprocessing_ia(Select *self) { POST_PROCESSING_IA };
static void Select_postprocessing_aa(Select *self) { POST_PROCESSING_AA };
static void Select_postprocessing_ireva(Select *self) { POST_PROCESSING_IREVA };
static void Select_postprocessing_areva(Select *self) { POST_PROCESSING_AREVA };
static void Select_postprocessing_revai(Select *self) { POST_PROCESSING_REVAI };
static void Select_postprocessing_revaa(Select *self) { POST_PROCESSING_REVAA };
static void Select_postprocessing_revareva(Select *self) { POST_PROCESSING_REVAREVA };

static void
Select_setProcMode(Select *self)
{
    int muladdmode;

    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Select_selector;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Select_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Select_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Select_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Select_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Select_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Select_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Select_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Select_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Select_postprocessing_revareva;
            break;
    }

}

static void
Select_compute_next_data_frame(Select *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Select_traverse(Select *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Select_clear(Select *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Select_dealloc(Select* self)
{
    pyo_DEALLOC
    Select_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Select_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Select *self;
    self = (Select *)type->tp_alloc(type, 0);

    self->value = 0;
    self->last_value = -99.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Select_compute_next_data_frame);
    self->mode_func_ptr = Select_setProcMode;

    static char *kwlist[] = {"input", "value", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->value, &multmp, &addtmp))
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

static PyObject * Select_getServer(Select* self) { GET_SERVER };
static PyObject * Select_getStream(Select* self) { GET_STREAM };
static PyObject * Select_setMul(Select *self, PyObject *arg) { SET_MUL };
static PyObject * Select_setAdd(Select *self, PyObject *arg) { SET_ADD };
static PyObject * Select_setSub(Select *self, PyObject *arg) { SET_SUB };
static PyObject * Select_setDiv(Select *self, PyObject *arg) { SET_DIV };

static PyObject * Select_play(Select *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Select_stop(Select *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Select_multiply(Select *self, PyObject *arg) { MULTIPLY };
static PyObject * Select_inplace_multiply(Select *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Select_add(Select *self, PyObject *arg) { ADD };
static PyObject * Select_inplace_add(Select *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Select_sub(Select *self, PyObject *arg) { SUB };
static PyObject * Select_inplace_sub(Select *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Select_div(Select *self, PyObject *arg) { DIV };
static PyObject * Select_inplace_div(Select *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Select_setValue(Select *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	if (PyLong_Check(arg) || PyInt_Check(arg)) {
		self->value = PyLong_AsLong(arg);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Select_members[] = {
{"server", T_OBJECT_EX, offsetof(Select, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Select, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Select, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Select, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Select_methods[] = {
{"getServer", (PyCFunction)Select_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Select_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Select_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Select_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setValue", (PyCFunction)Select_setValue, METH_O, "Sets value to select."},
{"setMul", (PyCFunction)Select_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Select_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Select_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Select_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Select_as_number = {
    (binaryfunc)Select_add,                         /*nb_add*/
    (binaryfunc)Select_sub,                         /*nb_subtract*/
    (binaryfunc)Select_multiply,                    /*nb_multiply*/
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
    (binaryfunc)Select_inplace_add,                 /*inplace_add*/
    (binaryfunc)Select_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Select_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Select_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Select_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject SelectType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Select_base",         /*tp_name*/
sizeof(Select),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Select_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Select_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Select objects. Watch input and send a trig on a selected value.",           /* tp_doc */
(traverseproc)Select_traverse,   /* tp_traverse */
(inquiry)Select_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Select_methods,             /* tp_methods */
Select_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Select_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT last_value;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Change;

static void
Change_selector(Change *self) {
    MYFLT val, inval;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < (self->last_value - 0.00001) || inval > (self->last_value + 0.00001)) {
            self->last_value = inval;
            val = 1;
        }
        else
            val = 0;

        self->data[i] = val;
    }
}

static void Change_postprocessing_ii(Change *self) { POST_PROCESSING_II };
static void Change_postprocessing_ai(Change *self) { POST_PROCESSING_AI };
static void Change_postprocessing_ia(Change *self) { POST_PROCESSING_IA };
static void Change_postprocessing_aa(Change *self) { POST_PROCESSING_AA };
static void Change_postprocessing_ireva(Change *self) { POST_PROCESSING_IREVA };
static void Change_postprocessing_areva(Change *self) { POST_PROCESSING_AREVA };
static void Change_postprocessing_revai(Change *self) { POST_PROCESSING_REVAI };
static void Change_postprocessing_revaa(Change *self) { POST_PROCESSING_REVAA };
static void Change_postprocessing_revareva(Change *self) { POST_PROCESSING_REVAREVA };

static void
Change_setProcMode(Change *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Change_selector;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Change_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Change_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Change_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Change_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Change_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Change_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Change_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Change_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Change_postprocessing_revareva;
            break;
    }
}

static void
Change_compute_next_data_frame(Change *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Change_traverse(Change *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
Change_clear(Change *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Change_dealloc(Change* self)
{
    pyo_DEALLOC
    Change_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Change_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Change *self;
    self = (Change *)type->tp_alloc(type, 0);

    self->last_value = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Change_compute_next_data_frame);
    self->mode_func_ptr = Change_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &multmp, &addtmp))
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

static PyObject * Change_getServer(Change* self) { GET_SERVER };
static PyObject * Change_getStream(Change* self) { GET_STREAM };
static PyObject * Change_setMul(Change *self, PyObject *arg) { SET_MUL };
static PyObject * Change_setAdd(Change *self, PyObject *arg) { SET_ADD };
static PyObject * Change_setSub(Change *self, PyObject *arg) { SET_SUB };
static PyObject * Change_setDiv(Change *self, PyObject *arg) { SET_DIV };

static PyObject * Change_play(Change *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Change_stop(Change *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Change_multiply(Change *self, PyObject *arg) { MULTIPLY };
static PyObject * Change_inplace_multiply(Change *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Change_add(Change *self, PyObject *arg) { ADD };
static PyObject * Change_inplace_add(Change *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Change_sub(Change *self, PyObject *arg) { SUB };
static PyObject * Change_inplace_sub(Change *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Change_div(Change *self, PyObject *arg) { DIV };
static PyObject * Change_inplace_div(Change *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Change_members[] = {
{"server", T_OBJECT_EX, offsetof(Change, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Change, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Change, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Change, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Change_methods[] = {
{"getServer", (PyCFunction)Change_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Change_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Change_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Change_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)Change_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Change_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Change_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Change_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Change_as_number = {
    (binaryfunc)Change_add,                         /*nb_add*/
    (binaryfunc)Change_sub,                         /*nb_subtract*/
    (binaryfunc)Change_multiply,                    /*nb_multiply*/
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
    (binaryfunc)Change_inplace_add,                 /*inplace_add*/
    (binaryfunc)Change_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Change_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Change_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Change_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ChangeType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Change_base",         /*tp_name*/
sizeof(Change),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Change_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&Change_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Change objects. Send a trig whenever input value changed.",           /* tp_doc */
(traverseproc)Change_traverse,   /* tp_traverse */
(inquiry)Change_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Change_methods,             /* tp_methods */
Change_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
Change_new,                 /* tp_new */
};