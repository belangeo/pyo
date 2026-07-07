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

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Dummy_postprocessing_revareva);
            break;
    }
}

static void
Dummy_compute_next_data_frame(Dummy *self)
{
    int i;

    if (self->modebuffer[2] == 0)
    {
        MYFLT inval = PyFloat_AS_DOUBLE(self->input);

        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = inval;
        }
    }
    else
    {
        MYFLT *in = Stream_getData((Stream *)self->input_stream);

        for (i = 0; i < self->bufsize; i++)
        {
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
    return 0;
}

static int
Dummy_clear(Dummy *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Dummy_dealloc(Dummy* self)
{
    pyo_DEALLOC
    Dummy_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Dummy_decref(Dummy* self)
{
    Py_DECREF(self); 
    Py_RETURN_NONE;
};

PyObject *
Dummy_initialize(Dummy *self)
{
    int i;
    self->input = PyFloat_FromDouble(0);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Dummy_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Dummy_setProcMode);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Stream_setStreamActive(self->stream, 1);

    Py_RETURN_NONE;
}

static PyObject *
Dummy_setInput(Dummy *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->input);

    if (PyNumber_Check(arg))
    {
        self->input = PyNumber_Float(arg);
        self->modebuffer[2] = 0;
    }
    else
    {
        self->input = arg;
        Py_INCREF(self->input);
        PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
        self->input_stream = (Stream *)streamtmp;
        Py_INCREF(self->input_stream);
        self->modebuffer[2] = 1;
    }

    (*self->mode_func_ptr)(self);

    Dummy_compute_next_data_frame(self);

    Py_RETURN_NONE;
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

static PyMemberDef Dummy_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Dummy, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Dummy, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Dummy, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Dummy, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Dummy, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Dummy_methods[] =
{
    {"getServer", (PyCFunction)Dummy_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Dummy_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Dummy_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Dummy_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Dummy_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setInput", (PyCFunction)Dummy_setInput, METH_O, "Sets the input sound object."},
    {"setMul", (PyCFunction)Dummy_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Dummy_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Dummy_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Dummy_setDiv, METH_O, "Sets inverse mul factor."},
    {"decref", (PyCFunction)Dummy_decref, METH_NOARGS, "Decref self reference count."},
    {NULL}  /* Sentinel */
};

static PyType_Slot DummyType_slots[] =
{
    {Py_tp_dealloc, Dummy_dealloc},
    {Py_tp_doc, "Dummy objects."},
    {Py_tp_traverse, Dummy_traverse},
    {Py_tp_clear, Dummy_clear},
    {Py_tp_methods, Dummy_methods},
    {Py_tp_members, Dummy_members},
    {Py_nb_add, Dummy_add},
    {Py_nb_subtract, Dummy_sub},
    {Py_nb_multiply, Dummy_multiply},
    {Py_nb_true_divide, Dummy_div},
    {Py_nb_inplace_add, Dummy_inplace_add},
    {Py_nb_inplace_subtract, Dummy_inplace_sub},
    {Py_nb_inplace_multiply, Dummy_inplace_multiply},
    {Py_nb_inplace_true_divide, Dummy_inplace_div},
    {Py_tp_new, PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec DummyType_spec =
{
    "_pyo.Dummy_base",
    sizeof(Dummy),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    DummyType_slots
};

PyTypeObject *
PyoCreateDummyType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &DummyType_spec, NULL);
}

/************************************************************************************************/
/* TriggerDummy streamer */
/************************************************************************************************/
typedef struct
{
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
TriggerDummy_setProcMode(TriggerDummy *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_postprocessing_revareva);
            break;
    }
}

static void
TriggerDummy_compute_next_data_frame(TriggerDummy *self)
{
    int i;
    MYFLT *tmp = TriggerStream_getData((TriggerStream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = tmp[i];
    }

    (*self->muladd_func_ptr)(self);
}

static int
TriggerDummy_traverse(TriggerDummy *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
TriggerDummy_clear(TriggerDummy *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
TriggerDummy_dealloc(TriggerDummy* self)
{
    pyo_DEALLOC
    TriggerDummy_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TriggerDummy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    TriggerDummy *self;
    self = (TriggerDummy *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TriggerDummy_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TriggerDummy_setProcMode);

    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp)) {
        Py_DECREF(self);
        return NULL;
    }

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

static PyMemberDef TriggerDummy_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TriggerDummy, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TriggerDummy, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TriggerDummy, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TriggerDummy, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TriggerDummy_methods[] =
{
    {"getServer", (PyCFunction)TriggerDummy_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TriggerDummy_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TriggerDummy_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TriggerDummy_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)TriggerDummy_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TriggerDummy_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TriggerDummy_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TriggerDummy_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};
static PyType_Slot TriggerDummyType_slots[] =
{
    {Py_tp_dealloc, TriggerDummy_dealloc},
    {Py_tp_doc, "TriggerDummy objects. Sends trigger at the end of playback."},
    {Py_tp_traverse, TriggerDummy_traverse},
    {Py_tp_clear, TriggerDummy_clear},
    {Py_tp_methods, TriggerDummy_methods},
    {Py_tp_members, TriggerDummy_members},
    {Py_nb_add, TriggerDummy_add},
    {Py_nb_subtract, TriggerDummy_sub},
    {Py_nb_multiply, TriggerDummy_multiply},
    {Py_nb_true_divide, TriggerDummy_div},
    {Py_nb_inplace_add, TriggerDummy_inplace_add},
    {Py_nb_inplace_subtract, TriggerDummy_inplace_sub},
    {Py_nb_inplace_multiply, TriggerDummy_inplace_multiply},
    {Py_nb_inplace_true_divide, TriggerDummy_inplace_div},
    {Py_tp_new, TriggerDummy_new},
    {0, NULL}
};

static PyType_Spec TriggerDummyType_spec =
{
    "_pyo.TriggerDummy_base",
    sizeof(TriggerDummy),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TriggerDummyType_slots
};

PyTypeObject *
PyoCreateTriggerDummyType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TriggerDummyType_spec, NULL);
}
