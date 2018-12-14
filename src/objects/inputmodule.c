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
    int chnl;
    int modebuffer[2];
} Input;

static void Input_postprocessing_ii(Input *self) { POST_PROCESSING_II };
static void Input_postprocessing_ai(Input *self) { POST_PROCESSING_AI };
static void Input_postprocessing_ia(Input *self) { POST_PROCESSING_IA };
static void Input_postprocessing_aa(Input *self) { POST_PROCESSING_AA };
static void Input_postprocessing_ireva(Input *self) { POST_PROCESSING_IREVA };
static void Input_postprocessing_areva(Input *self) { POST_PROCESSING_AREVA };
static void Input_postprocessing_revai(Input *self) { POST_PROCESSING_REVAI };
static void Input_postprocessing_revaa(Input *self) { POST_PROCESSING_REVAA };
static void Input_postprocessing_revareva(Input *self) { POST_PROCESSING_REVAREVA };

static void
Input_setProcMode(Input *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Input_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Input_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Input_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Input_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Input_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Input_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Input_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Input_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Input_postprocessing_revareva;
            break;
    }
}

static void
Input_compute_next_data_frame(Input *self)
{
    int i;
    MYFLT *tmp;
    tmp = Server_getInputBuffer((Server *)self->server);
    for (i=0; i<self->bufsize*self->ichnls; i++) {
        if ((i % self->ichnls) == self->chnl)
            self->data[(int)(i/self->ichnls)] = tmp[i];
    }
    (*self->muladd_func_ptr)(self);
}

static int
Input_traverse(Input *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
Input_clear(Input *self)
{
    pyo_CLEAR
    return 0;
}

static void
Input_dealloc(Input* self)
{
    pyo_DEALLOC
    Input_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Input_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    Input *self;
    self = (Input *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Input_compute_next_data_frame);
    self->mode_func_ptr = Input_setProcMode;

    static char *kwlist[] = {"chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOO", kwlist, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * Input_getServer(Input* self) { GET_SERVER };
static PyObject * Input_getStream(Input* self) { GET_STREAM };
static PyObject * Input_setMul(Input *self, PyObject *arg) { SET_MUL };
static PyObject * Input_setAdd(Input *self, PyObject *arg) { SET_ADD };
static PyObject * Input_setSub(Input *self, PyObject *arg) { SET_SUB };
static PyObject * Input_setDiv(Input *self, PyObject *arg) { SET_DIV };

static PyObject * Input_play(Input *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Input_out(Input *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Input_stop(Input *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Input_multiply(Input *self, PyObject *arg) { MULTIPLY };
static PyObject * Input_inplace_multiply(Input *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Input_add(Input *self, PyObject *arg) { ADD };
static PyObject * Input_inplace_add(Input *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Input_sub(Input *self, PyObject *arg) { SUB };
static PyObject * Input_inplace_sub(Input *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Input_div(Input *self, PyObject *arg) { DIV };
static PyObject * Input_inplace_div(Input *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Input_members[] = {
    {"server", T_OBJECT_EX, offsetof(Input, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Input, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Input, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Input, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Input_methods[] = {
    {"getServer", (PyCFunction)Input_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Input_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Input_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Input_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Input_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setMul", (PyCFunction)Input_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Input_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Input_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Input_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Input_as_number = {
    (binaryfunc)Input_add,                      /*nb_add*/
    (binaryfunc)Input_sub,                 /*nb_subtract*/
    (binaryfunc)Input_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Input_inplace_add,              /*inplace_add*/
    (binaryfunc)Input_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Input_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Input_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Input_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject InputType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Input_base",         /*tp_name*/
    sizeof(Input),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Input_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Input_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Input objects. Retreive audio from an input channel.",           /* tp_doc */
    (traverseproc)Input_traverse,   /* tp_traverse */
    (inquiry)Input_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Input_methods,             /* tp_methods */
    Input_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Input_new,                 /* tp_new */
};
