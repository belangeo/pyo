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
    PyObject *input;
    Stream *input_stream;
    long long value;
    MYFLT last_value;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Select;

static void
Select_selector(Select *self)
{
    MYFLT val, inval;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
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

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Select_selector);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Select_postprocessing_revareva);
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
    return 0;
}

static int
Select_clear(Select *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Select_dealloc(Select* self)
{
    pyo_DEALLOC
    Select_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Select_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Select *self;
    self = (Select *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = 0;
    self->last_value = -99.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Select_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Select_setProcMode);

    static char *kwlist[] = {"input", "value", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|LOO", kwlist, &inputtmp, &self->value, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

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

    if (PyLong_Check(arg))
    {
        self->value = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyMemberDef Select_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Select, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Select, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Select, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Select, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Select_methods[] =
{
    {"getServer", (PyCFunction)Select_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Select_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Select_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Select_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setValue", (PyCFunction)Select_setValue, METH_O, "Sets value to select."},
    {"setMul", (PyCFunction)Select_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Select_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Select_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Select_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot SelectType_slots[] = {
    {Py_tp_dealloc, Select_dealloc},
    {Py_tp_doc, "Select objects. Watch input and send a trig on a selected value."},
    {Py_tp_traverse, Select_traverse},
    {Py_tp_clear, Select_clear},
    {Py_tp_methods, Select_methods},
    {Py_tp_members, Select_members},
    {Py_tp_new, Select_new},
    {Py_nb_add, Select_add},
    {Py_nb_subtract, Select_sub},
    {Py_nb_multiply, Select_multiply},
    {Py_nb_true_divide, Select_div},
    {Py_nb_inplace_add, Select_inplace_add},
    {Py_nb_inplace_subtract, Select_inplace_sub},
    {Py_nb_inplace_multiply, Select_inplace_multiply},
    {Py_nb_inplace_true_divide, Select_inplace_div},
    {0, NULL}
};

static PyType_Spec SelectType_spec =
{
    "_pyo.Select_base",
    sizeof(Select),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    SelectType_slots
};

PyTypeObject *
PyoCreateSelectType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &SelectType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT last_value;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Change;

static void
Change_selector(Change *self)
{
    MYFLT val, inval;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        inval = in[i];

        if (inval < (self->last_value - 0.00001) || inval > (self->last_value + 0.00001))
        {
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

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Change_selector);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Change_postprocessing_revareva);
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
    return 0;
}

static int
Change_clear(Change *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Change_dealloc(Change* self)
{
    pyo_DEALLOC
    Change_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Change_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Change *self;
    self = (Change *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->last_value = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Change_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Change_setProcMode);

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

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

static PyMemberDef Change_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Change, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Change, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Change, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Change, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Change_methods[] =
{
    {"getServer", (PyCFunction)Change_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Change_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Change_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Change_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Change_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Change_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Change_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Change_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot ChangeType_slots[] = {
    {Py_tp_dealloc, Change_dealloc},
    {Py_tp_doc, "Change objects. Send a trig whenever input value changed."},
    {Py_tp_traverse, Change_traverse},
    {Py_tp_clear, Change_clear},
    {Py_tp_methods, Change_methods},
    {Py_tp_members, Change_members},
    {Py_tp_new, Change_new},
    {Py_nb_add, Change_add},
    {Py_nb_subtract, Change_sub},
    {Py_nb_multiply, Change_multiply},
    {Py_nb_true_divide, Change_div},
    {Py_nb_inplace_add, Change_inplace_add},
    {Py_nb_inplace_subtract, Change_inplace_sub},
    {Py_nb_inplace_multiply, Change_inplace_multiply},
    {Py_nb_inplace_true_divide, Change_inplace_div},
    {0, NULL}
};

static PyType_Spec ChangeType_spec =
{
    "_pyo.Change_base",
    sizeof(Change),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    ChangeType_slots
};

PyTypeObject *
PyoCreateChangeType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &ChangeType_spec, NULL);
}