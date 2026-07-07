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

typedef struct
{
    pyo_audio_HEAD
    PyObject *input1;
    PyObject *input2;
    Stream *input1_stream;
    Stream *input2_stream;
    MYFLT fadetime;
    int switcher;
    double currentTime;
    double sampleToSec;
} InputFader;

static void InputFader_setProcMode(InputFader *self) {};

static void InputFader_process_only_first(InputFader *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input1_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = in[i];
    }
}

static void InputFader_process_only_second(InputFader *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = in[i];
    }
}

static void InputFader_process_one(InputFader *self)
{
    int i;
    MYFLT sclfade, val;
    MYFLT *in1 = Stream_getData((Stream *)self->input1_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    val = 0.0;
    sclfade = 1. / self->fadetime;

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime < self->fadetime)
        {
            val = MYSQRT(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }
        else
            val = 1.;

        self->data[i] = in1[i] * val + in2[i] * (1 - val);
    }

    if (val == 1.)
        self->proc_func_ptr = PYO_AUDIO_CALLBACK(InputFader_process_only_first);

}

static void InputFader_process_two(InputFader *self)
{
    int i;
    MYFLT sclfade, val;
    MYFLT *in1 = Stream_getData((Stream *)self->input1_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    val = 0.0;
    sclfade = 1. / self->fadetime;

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime < self->fadetime)
        {
            val = MYSQRT(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }
        else
            val = 1.;

        self->data[i] = in2[i] * val + in1[i] * (1 - val);
    }

    if (val == 1.)
        self->proc_func_ptr = PYO_AUDIO_CALLBACK(InputFader_process_only_second);
}

static void
InputFader_compute_next_data_frame(InputFader *self)
{
    (*self->proc_func_ptr)(self);
}

static int
InputFader_traverse(InputFader *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input1);
    Py_VISIT(self->input2);
    return 0;
}

static int
InputFader_clear(InputFader *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input1);
    Py_CLEAR(self->input2);
    return 0;
}

static void
InputFader_dealloc(InputFader* self)
{
    pyo_DEALLOC
    InputFader_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
InputFader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp = NULL;
    InputFader *self;
    self = (InputFader *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->switcher = 0;
    self->fadetime = 0.05;
    self->currentTime = 0.0;
    self->input1 = PyFloat_FromDouble(0.0);
    self->input2 = PyFloat_FromDouble(0.0);

    INIT_OBJECT_COMMON

    self->sampleToSec = 1. / self->sr;

    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(InputFader_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(InputFader_setProcMode);
    self->proc_func_ptr = PYO_AUDIO_CALLBACK(InputFader_process_only_first);

    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "server") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument must be a PyoObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->input1);
    self->input1 = inputtmp;
    Py_INCREF(self->input1);
    PyObject *streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->input1, "_getStream", NULL);
    self->input1_stream = (Stream *)streamtmp;
    Py_INCREF(self->input1_stream);

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    return (PyObject *)self;
}

static PyObject *
InputFader_setInput(InputFader *self, PyObject *args, PyObject *kwds)
{
    PyObject *tmp, *streamtmp;

    static char *kwlist[] = {"input", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_F, kwlist, &tmp, &self->fadetime))
        Py_RETURN_NONE;

    self->switcher = (self->switcher + 1) % 2;
    self->currentTime = 0.0;

    if (self->fadetime == 0)
        self->fadetime = 0.0001;

    if (self->switcher == 0)
    {
        Py_DECREF(self->input1);
        self->input1 = tmp;
        Py_INCREF(self->input1);
        streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->input1, "_getStream", NULL);
        self->input1_stream = (Stream *)streamtmp;
        Py_INCREF(self->input1_stream);
        self->proc_func_ptr = PYO_AUDIO_CALLBACK(InputFader_process_one);
    }
    else
    {
        Py_DECREF(self->input2);
        self->input2 = tmp;
        Py_INCREF(self->input2);
        streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->input2, "_getStream", NULL);
        self->input2_stream = (Stream *)streamtmp;
        Py_INCREF(self->input2_stream);
        self->proc_func_ptr = PYO_AUDIO_CALLBACK(InputFader_process_two);
    }

    Py_RETURN_NONE;
}

static PyObject * InputFader_getServer(InputFader* self) { GET_SERVER };
static PyObject * InputFader_getStream(InputFader* self) { GET_STREAM };

static PyObject * InputFader_play(InputFader *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * InputFader_out(InputFader *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * InputFader_stop(InputFader *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef InputFader_members[] =
{
    {"server", T_OBJECT_EX, offsetof(InputFader, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(InputFader, stream), 0, "Stream object."},
    {"input1", T_OBJECT_EX, offsetof(InputFader, input1), 0, "First input."},
    {"input2", T_OBJECT_EX, offsetof(InputFader, input2), 0, "Second input."},
    {NULL}  /* Sentinel */
};

static PyMethodDef InputFader_methods[] =
{
    {"getServer", (PyCFunction)InputFader_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)InputFader_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)InputFader_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)InputFader_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setInput", (PyCFunction)InputFader_setInput, METH_VARARGS | METH_KEYWORDS, "Crossfade between current stream and given stream."},
    {"stop", (PyCFunction)InputFader_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

static PyType_Slot InputFaderType_slots[] =
{
    {Py_tp_dealloc, InputFader_dealloc},
    {Py_tp_doc, "InputFader objects. Generates a crossfade between current input sound stream and new input sound stream."},
    {Py_tp_traverse, InputFader_traverse},
    {Py_tp_clear, InputFader_clear},
    {Py_tp_methods, InputFader_methods},
    {Py_tp_members, InputFader_members},
    {Py_tp_new, InputFader_new},
    {0, NULL}
};

static PyType_Spec InputFaderType_spec =
{
    "_pyo.InputFader_base",
    sizeof(InputFader),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    InputFaderType_slots
};

PyTypeObject *
PyoCreateInputFaderType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &InputFaderType_spec, NULL);
}
