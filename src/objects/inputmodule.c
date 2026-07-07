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

typedef struct
{
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

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Input_postprocessing_revareva);
            break;
    }
}

static void
Input_compute_next_data_frame(Input *self)
{
    int i;
    MYFLT *tmp;
    tmp = Server_getInputBuffer((Server *)self->server);

    for (i = 0; i < self->bufsize * self->ichnls; i++)
    {
        if ((i % self->ichnls) == self->chnl)
            self->data[(int)(i / self->ichnls)] = tmp[i];
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
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Input_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp = NULL, *addtmp = NULL;
    Input *self;
    self = (Input *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Input_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Input_setProcMode);

    static char *kwlist[] = {"chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iOO", kwlist, &self->chnl, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if (multmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp)
    {
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

static PyMemberDef Input_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Input, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Input, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Input, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Input, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Input_methods[] =
{
    {"getServer", (PyCFunction)Input_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Input_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Input_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Input_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Input_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Input_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Input_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Input_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Input_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot InputType_slots[] = {
    {Py_tp_dealloc, Input_dealloc},
    {Py_tp_doc, "Input objects. Retreive audio from an input channel."},
    {Py_tp_traverse, Input_traverse},
    {Py_tp_clear, Input_clear},
    {Py_tp_methods, Input_methods},
    {Py_tp_members, Input_members},
    {Py_tp_new, Input_new},
    {Py_nb_add, Input_add},
    {Py_nb_subtract, Input_sub},
    {Py_nb_multiply, Input_multiply},
    {Py_nb_true_divide, Input_div},
    {Py_nb_inplace_add, Input_inplace_add},
    {Py_nb_inplace_subtract, Input_inplace_sub},
    {Py_nb_inplace_multiply, Input_inplace_multiply},
    {Py_nb_inplace_true_divide, Input_inplace_div},
    {0, NULL}
};

static PyType_Spec InputType_spec =
{
    "_pyo.Input_base",
    sizeof(Input),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    InputType_slots
};

PyTypeObject *
PyoCreateInputType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &InputType_spec, NULL);
}
