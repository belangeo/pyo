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
    PyObject *callable;
    PyObject *time;
    Stream *time_stream;
    PyObject *arg;
    int modebuffer[1];
    MYFLT sampleToSec;
    double currentTime;
    int init;
} Pattern;

static void
Pattern_generate_i(Pattern *self)
{
    int i;
    MYFLT tm;
    PyObject *tuple, *result;

    tm = PyFloat_AS_DOUBLE(self->time);

    if (self->init)
    {
        self->init = 0;
        self->currentTime = tm;
    }

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime >= tm && PyCallable_Check(self->callable))
        {
            self->currentTime = 0.0;

            if (self->arg == Py_None)
            {
                result = PyObject_Call((PyObject *)self->callable, PyTuple_New(0), NULL);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
            }
            else
            {
                tuple = PyTuple_New(1);
                Py_INCREF(self->arg);
                PyTuple_SET_ITEM(tuple, 0, self->arg);
                result = PyObject_Call((PyObject *)self->callable, tuple, NULL);
                Py_DECREF(tuple);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
            }
        }

        self->currentTime += self->sampleToSec;
    }
}

static void
Pattern_generate_a(Pattern *self)
{
    int i;
    PyObject *tuple, *result;

    MYFLT *tm = Stream_getData((Stream *)self->time_stream);

    if (self->init)
    {
        self->init = 0;
        self->currentTime = tm[0];
    }

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime >= tm[i] && PyCallable_Check(self->callable))
        {
            self->currentTime = 0.0;

            if (self->arg == Py_None)
            {
                result = PyObject_Call((PyObject *)self->callable, PyTuple_New(0), NULL);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
            }
            else
            {
                tuple = PyTuple_New(1);
                Py_INCREF(self->arg);
                PyTuple_SET_ITEM(tuple, 0, self->arg);
                result = PyObject_Call((PyObject *)self->callable, tuple, NULL);
                Py_DECREF(tuple);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
            }
        }

        self->currentTime += self->sampleToSec;
    }
}

static void
Pattern_setProcMode(Pattern *self)
{
    int procmode = self->modebuffer[0];

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = Pattern_generate_i;
            break;

        case 1:
            self->proc_func_ptr = Pattern_generate_a;
            break;
    }
}

static void
Pattern_compute_next_data_frame(Pattern *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Pattern_traverse(Pattern *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    Py_VISIT(self->time);
    Py_VISIT(self->arg);
    return 0;
}

static int
Pattern_clear(Pattern *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    Py_CLEAR(self->time);
    Py_CLEAR(self->arg);
    return 0;
}

static void
Pattern_dealloc(Pattern* self)
{
    pyo_DEALLOC
    Pattern_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Pattern_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *timetmp = NULL, *calltmp = NULL, *argtmp = NULL;
    Pattern *self;
    self = (Pattern *)type->tp_alloc(type, 0);

    self->time = PyFloat_FromDouble(1.);
    self->modebuffer[0] = 0;
    self->init = 1;
    self->arg = Py_None;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pattern_compute_next_data_frame);
    self->mode_func_ptr = Pattern_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;

    static char *kwlist[] = {"callable", "time", "arg", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &calltmp, &timetmp, &argtmp))
        Py_RETURN_NONE;

    if (calltmp)
    {
        PyObject_CallMethod((PyObject *)self, "setFunction", "O", calltmp);
    }

    if (timetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }

    if (argtmp)
    {
        if (PyTuple_Check(argtmp))
        {
            PyObject *argument = PyTuple_New(1);
            PyTuple_SetItem(argument, 0, argtmp);
            PyObject_CallMethod((PyObject *)self, "setArg", "O", argument);
        }
        else
        {
            PyObject_CallMethod((PyObject *)self, "setArg", "O", argtmp);
        }
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Pattern_getServer(Pattern* self) { GET_SERVER };
static PyObject * Pattern_getStream(Pattern* self) { GET_STREAM };

static PyObject *
Pattern_play(Pattern *self, PyObject *args, PyObject *kwds)
{
    self->init = 1;
    PLAY
};

static PyObject * Pattern_stop(Pattern *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Pattern_setFunction(Pattern *self, PyObject *arg)
{
    if (! PyCallable_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->callable);
    self->callable = arg;
    Py_INCREF(self->callable);

    Py_RETURN_NONE;
}

static PyObject * Pattern_setTime(Pattern *self, PyObject *arg) { SET_PARAM(self->time, self->time_stream, 0); }

static PyObject *
Pattern_setArg(Pattern *self, PyObject *arg)
{
    Py_XDECREF(self->arg);
    self->arg = arg;
    Py_INCREF(self->arg);

    Py_RETURN_NONE;
}

static PyMemberDef Pattern_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Pattern, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Pattern, stream), 0, NULL},
    {"time", T_OBJECT_EX, offsetof(Pattern, time), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Pattern_methods[] =
{
    {"getServer", (PyCFunction)Pattern_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Pattern_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Pattern_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Pattern_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTime", (PyCFunction)Pattern_setTime, METH_O, NULL},
    {"setFunction", (PyCFunction)Pattern_setFunction, METH_O, NULL},
    {"setArg", (PyCFunction)Pattern_setArg, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PatternType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Pattern_base",         /*tp_name*/
    sizeof(Pattern),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Pattern_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)Pattern_traverse,   /* tp_traverse */
    (inquiry)Pattern_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Pattern_methods,             /* tp_methods */
    Pattern_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Pattern_new,                 /* tp_new */
};

/***************/
/**** Score ****/
/***************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    char *fname;
    char curfname[100];
    int last_value;
} Score;

static void
Score_selector(Score *self)
{
    int i, inval;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        inval = (int)in[i];

        if (inval != self->last_value)
        {
            sprintf(self->curfname, "%s%i()\n", self->fname, inval);
            PyRun_SimpleString(self->curfname);
            self->last_value = inval;
        }
    }
}

static void
Score_setProcMode(Score *self)
{
    self->proc_func_ptr = Score_selector;
}

static void
Score_compute_next_data_frame(Score *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Score_traverse(Score *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Score_clear(Score *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Score_dealloc(Score* self)
{
    pyo_DEALLOC
    Score_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Score_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    Score *self;
    self = (Score *)type->tp_alloc(type, 0);

    self->last_value = -99;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Score_compute_next_data_frame);
    self->mode_func_ptr = Score_setProcMode;

    static char *kwlist[] = {"input", "fname", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|s", kwlist, &inputtmp, &self->fname))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Score_getServer(Score* self) { GET_SERVER };
static PyObject * Score_getStream(Score* self) { GET_STREAM };

static PyObject * Score_play(Score *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Score_stop(Score *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef Score_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Score, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Score, stream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Score_methods[] =
{
    {"getServer", (PyCFunction)Score_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Score_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Score_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Score_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject ScoreType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Score_base",         /*tp_name*/
    sizeof(Score),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Score_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)Score_traverse,   /* tp_traverse */
    (inquiry)Score_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Score_methods,             /* tp_methods */
    Score_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Score_new,                 /* tp_new */
};

/*****************/
/*** CallAfter ***/
/*****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *callable;
    PyObject *arg;
    MYFLT time;
    MYFLT sampleToSec;
    double currentTime;
} CallAfter;

static void
CallAfter_generate(CallAfter *self)
{
    int i;
    PyObject *tuple, *result;

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime >= self->time)
        {
            if (self->stream != NULL)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);

            if (self->arg == Py_None)
            {
                result = PyObject_Call(self->callable, PyTuple_New(0), NULL);
            }
            else
            {
                tuple = PyTuple_New(1);
                Py_INCREF(self->arg);
                PyTuple_SET_ITEM(tuple, 0, self->arg);
                result = PyObject_Call(self->callable, tuple, NULL);
                Py_DECREF(tuple);

            }

            if (result == NULL)
                PyErr_Print();

            break;
        }

        self->currentTime += self->sampleToSec;
    }
}

static void
CallAfter_setProcMode(CallAfter *self)
{
    self->proc_func_ptr = CallAfter_generate;
}

static void
CallAfter_compute_next_data_frame(CallAfter *self)
{
    (*self->proc_func_ptr)(self);
}

static int
CallAfter_traverse(CallAfter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    Py_VISIT(self->arg);
    return 0;
}

static int
CallAfter_clear(CallAfter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    Py_CLEAR(self->arg);
    return 0;
}

static void
CallAfter_dealloc(CallAfter* self)
{
    pyo_DEALLOC
    CallAfter_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CallAfter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *calltmp = NULL, *argtmp = NULL;
    CallAfter *self;
    self = (CallAfter *)type->tp_alloc(type, 0);

    self->time = 1.;
    self->arg = Py_None;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CallAfter_compute_next_data_frame);
    self->mode_func_ptr = CallAfter_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;

    static char *kwlist[] = {"callable", "time", "arg", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FO, kwlist, &calltmp, &self->time, &argtmp))
        Py_RETURN_NONE;

    if (! PyCallable_Check(calltmp))
        Py_RETURN_NONE;

    if (argtmp)
    {
        Py_DECREF(self->arg);
        Py_INCREF(argtmp);
        self->arg = argtmp;
    }

    Py_INCREF(calltmp);
    Py_XDECREF(self->callable);
    self->callable = calltmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * CallAfter_getServer(CallAfter* self) { GET_SERVER };
static PyObject * CallAfter_getStream(CallAfter* self) { GET_STREAM };

static PyObject * CallAfter_play(CallAfter *self, PyObject *args, PyObject *kwds)
{
    self->currentTime = 0.;
    PLAY
};
static PyObject * CallAfter_stop(CallAfter *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
CallAfter_setTime(CallAfter *self, PyObject *arg)
{
    if (PyNumber_Check(arg))
    {
        self->time = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
CallAfter_setArg(CallAfter *self, PyObject *arg)
{
    Py_XDECREF(self->arg);
    self->arg = arg;
    Py_INCREF(self->arg);

    Py_RETURN_NONE;
}

static PyMemberDef CallAfter_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CallAfter, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(CallAfter, stream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef CallAfter_methods[] =
{
    {"getServer", (PyCFunction)CallAfter_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)CallAfter_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)CallAfter_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)CallAfter_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTime", (PyCFunction)CallAfter_setTime, METH_O, NULL},
    {"setArg", (PyCFunction)CallAfter_setArg, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject CallAfterType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CallAfter_base",         /*tp_name*/
    sizeof(CallAfter),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CallAfter_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)CallAfter_traverse,   /* tp_traverse */
    (inquiry)CallAfter_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    CallAfter_methods,             /* tp_methods */
    CallAfter_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    CallAfter_new,                 /* tp_new */
};