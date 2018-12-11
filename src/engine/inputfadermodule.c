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

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i];
    }
}

static void InputFader_process_only_second(InputFader *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input2_stream);

    for (i=0; i<self->bufsize; i++) {
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
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = MYSQRT(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }
        else
            val = 1.;

        self->data[i] = in1[i] * val + in2[i] * (1 - val);
    }
    if (val == 1.)
        self->proc_func_ptr = InputFader_process_only_first;

}

static void InputFader_process_two(InputFader *self)
{
    int i;
    MYFLT sclfade, val;
    MYFLT *in1 = Stream_getData((Stream *)self->input1_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    val = 0.0;
    sclfade = 1. / self->fadetime;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = MYSQRT(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }
        else
            val = 1.;

        self->data[i] = in2[i] * val + in1[i] * (1 - val);
    }
    if (val == 1.)
        self->proc_func_ptr = InputFader_process_only_second;
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
    Py_VISIT(self->input1_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int
InputFader_clear(InputFader *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input1);
    Py_CLEAR(self->input1_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
InputFader_dealloc(InputFader* self)
{
    pyo_DEALLOC
    InputFader_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
InputFader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp=NULL, *streamtmp;
    InputFader *self;
    self = (InputFader *)type->tp_alloc(type, 0);

    self->switcher = 0;
    self->fadetime = 0.05;
    self->currentTime = 0.0;

    INIT_OBJECT_COMMON

    self->sampleToSec = 1. / self->sr;

    Stream_setFunctionPtr(self->stream, InputFader_compute_next_data_frame);
    self->mode_func_ptr = InputFader_setProcMode;
    self->proc_func_ptr = InputFader_process_only_first;

    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)inputtmp, "server") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"input\" argument must be a PyoObject.\n");
        Py_RETURN_NONE;
    }
    Py_INCREF(inputtmp);
    Py_XDECREF(self->input1);
    self->input1 = inputtmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->input1_stream);
    self->input1_stream = (Stream *)streamtmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

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

    Py_INCREF(tmp);

    if (self->switcher == 0) {
        Py_DECREF(self->input1);
        self->input1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input1_stream);
        self->input1_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_one;
	}
    else {
        Py_XDECREF(self->input2);
        self->input2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input2_stream);
        self->input2_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_two;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * InputFader_getServer(InputFader* self) { GET_SERVER };
static PyObject * InputFader_getStream(InputFader* self) { GET_STREAM };

static PyObject * InputFader_play(InputFader *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * InputFader_out(InputFader *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * InputFader_stop(InputFader *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef InputFader_members[] = {
    {"server", T_OBJECT_EX, offsetof(InputFader, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(InputFader, stream), 0, "Stream object."},
    {"input1", T_OBJECT_EX, offsetof(InputFader, input1), 0, "First input."},
    {"input2", T_OBJECT_EX, offsetof(InputFader, input2), 0, "Second input."},
    {NULL}  /* Sentinel */
};

static PyMethodDef InputFader_methods[] = {
    {"getServer", (PyCFunction)InputFader_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)InputFader_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)InputFader_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)InputFader_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setInput", (PyCFunction)InputFader_setInput, METH_VARARGS|METH_KEYWORDS, "Crossfade between current stream and given stream."},
    {"stop", (PyCFunction)InputFader_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject InputFaderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.InputFader_base",         /*tp_name*/
    sizeof(InputFader),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)InputFader_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "InputFader objects. Generates a crossfade between current input sound stream and new input sound stream.",  /* tp_doc */
    (traverseproc)InputFader_traverse,   /* tp_traverse */
    (inquiry)InputFader_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    InputFader_methods,             /* tp_methods */
    InputFader_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    InputFader_new,                 /* tp_new */
};
