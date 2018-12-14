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
    int modebuffer[2];
} Mix;

static void Mix_postprocessing_ii(Mix *self) { POST_PROCESSING_II };
static void Mix_postprocessing_ai(Mix *self) { POST_PROCESSING_AI };
static void Mix_postprocessing_ia(Mix *self) { POST_PROCESSING_IA };
static void Mix_postprocessing_aa(Mix *self) { POST_PROCESSING_AA };
static void Mix_postprocessing_ireva(Mix *self) { POST_PROCESSING_IREVA };
static void Mix_postprocessing_areva(Mix *self) { POST_PROCESSING_AREVA };
static void Mix_postprocessing_revai(Mix *self) { POST_PROCESSING_REVAI };
static void Mix_postprocessing_revaa(Mix *self) { POST_PROCESSING_REVAA };
static void Mix_postprocessing_revareva(Mix *self) { POST_PROCESSING_REVAREVA };

static void
Mix_setProcMode(Mix *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Mix_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Mix_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Mix_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Mix_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Mix_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Mix_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Mix_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Mix_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Mix_postprocessing_revareva;
            break;
    }
}

static void
Mix_compute_next_data_frame(Mix *self)
{
    int i, j;
    MYFLT old;
    PyObject *stream;
    Py_ssize_t lsize = PyList_Size(self->input);

    MYFLT buffer[self->bufsize];
    memset(&buffer, 0, sizeof(buffer));

    for (i=0; i<lsize; i++) {
        stream = PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, i), "_getStream", NULL);
        MYFLT *in = Stream_getData((Stream *)stream);
        for (j=0; j<self->bufsize; j++) {
            old = buffer[j];
            buffer[j] = in[j] + old;
        }
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = buffer[i];
    }

    (*self->muladd_func_ptr)(self);
}

static int
Mix_traverse(Mix *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Mix_clear(Mix *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Mix_dealloc(Mix* self)
{
    pyo_DEALLOC
    Mix_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Mix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Mix *self;
    self = (Mix *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Mix_compute_next_data_frame);
    self->mode_func_ptr = Mix_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_INCREF(inputtmp);
    Py_XDECREF(self->input);
    self->input = inputtmp;

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

static PyObject * Mix_getServer(Mix* self) { GET_SERVER };
static PyObject * Mix_getStream(Mix* self) { GET_STREAM };
static PyObject * Mix_setMul(Mix *self, PyObject *arg) { SET_MUL };
static PyObject * Mix_setAdd(Mix *self, PyObject *arg) { SET_ADD };
static PyObject * Mix_setSub(Mix *self, PyObject *arg) { SET_SUB };
static PyObject * Mix_setDiv(Mix *self, PyObject *arg) { SET_DIV };

static PyObject * Mix_play(Mix *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Mix_out(Mix *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Mix_stop(Mix *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Mix_multiply(Mix *self, PyObject *arg) { MULTIPLY };
static PyObject * Mix_inplace_multiply(Mix *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Mix_add(Mix *self, PyObject *arg) { ADD };
static PyObject * Mix_inplace_add(Mix *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Mix_sub(Mix *self, PyObject *arg) { SUB };
static PyObject * Mix_inplace_sub(Mix *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Mix_div(Mix *self, PyObject *arg) { DIV };
static PyObject * Mix_inplace_div(Mix *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Mix_members[] = {
    {"server", T_OBJECT_EX, offsetof(Mix, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Mix, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Mix, input), 0, "List of input signals to mix."},
    {"mul", T_OBJECT_EX, offsetof(Mix, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Mix, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Mix_methods[] = {
    {"getServer", (PyCFunction)Mix_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Mix_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Mix_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Mix_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Mix_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
	{"setMul", (PyCFunction)Mix_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Mix_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Mix_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Mix_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Mix_as_number = {
    (binaryfunc)Mix_add,                      /*nb_add*/
    (binaryfunc)Mix_sub,                 /*nb_subtract*/
    (binaryfunc)Mix_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Mix_inplace_add,              /*inplace_add*/
    (binaryfunc)Mix_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Mix_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Mix_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Mix_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject MixType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Mix_base",         /*tp_name*/
    sizeof(Mix),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Mix_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Mix_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Mix objects. Retreive audio from an input channel.",           /* tp_doc */
    (traverseproc)Mix_traverse,   /* tp_traverse */
    (inquiry)Mix_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Mix_methods,             /* tp_methods */
    Mix_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Mix_new,                 /* tp_new */
};
