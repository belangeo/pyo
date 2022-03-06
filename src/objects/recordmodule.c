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
#include "interpolation.h"

/************/
/* ControlRec */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *tmp_list;
    MYFLT dur;
    int rate;
    int modulo;
    long count;
    long time;
    long size;
    MYFLT *buffer;
} ControlRec;

static void
ControlRec_process(ControlRec *self)
{
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->dur > 0.0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            if ((self->time % self->modulo) == 0 && self->count < self->size)
            {
                self->buffer[self->count] = in[i];
                self->count++;
            }

            self->time++;

            if (self->count >= self->size)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            if ((self->time % self->modulo) == 0)
            {
                PyList_Append(self->tmp_list, PyFloat_FromDouble(in[i]));
            }

            self->time++;
        }
    }
}

static void
ControlRec_setProcMode(ControlRec *self)
{
    self->proc_func_ptr = ControlRec_process;
}

static void
ControlRec_compute_next_data_frame(ControlRec *self)
{
    (*self->proc_func_ptr)(self);
}

static int
ControlRec_traverse(ControlRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->tmp_list);
    return 0;
}

static int
ControlRec_clear(ControlRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->tmp_list);
    return 0;
}

static void
ControlRec_dealloc(ControlRec* self)
{
    pyo_DEALLOC

    if (self->buffer != NULL)
        PyMem_RawFree(self->buffer);

    ControlRec_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ControlRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    long j;
    PyObject *inputtmp, *input_streamtmp;
    ControlRec *self;
    self = (ControlRec *)type->tp_alloc(type, 0);

    self->dur = 0.0;
    self->rate = 1000;
    self->tmp_list = PyList_New(0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ControlRec_compute_next_data_frame);
    self->mode_func_ptr = ControlRec_setProcMode;

    static char *kwlist[] = {"input", "rate", "dur", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IF, kwlist, &inputtmp, &self->rate, &self->dur))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->dur > 0.0)
    {
        self->size = (long)(self->dur * self->rate + 1);
        self->buffer = (MYFLT *)PyMem_RawRealloc(self->buffer, self->size * sizeof(MYFLT));

        for (j = 0; j < self->size; j++)
        {
            self->buffer[j] = 0.0;
        }
    }

    self->modulo = (int)(self->sr / self->rate);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * ControlRec_getServer(ControlRec* self) { GET_SERVER };
static PyObject * ControlRec_getStream(ControlRec* self) { GET_STREAM };

static PyObject * ControlRec_play(ControlRec *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    PLAY
};

static PyObject * ControlRec_stop(ControlRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
ControlRec_getData(ControlRec *self)
{
    int i;
    PyObject *data, *point;
    MYFLT time, timescl = 1.0 / self->rate;

    if (self->dur > 0.0)
    {
        data = PyList_New(self->size);

        for (i = 0; i < self->size; i++)
        {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyFloat_FromDouble(self->buffer[i]));
            PyList_SetItem(data, i, point);
        }
    }
    else
    {
        if (Stream_getStreamActive(self->stream))
        {
            PyObject_CallMethod((PyObject *)self, "stop", NULL);
        }
        Py_ssize_t size = PyList_Size(self->tmp_list);
        data = PyList_New(size);

        for (i = 0; i < size; i++)
        {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list, i));
            PyList_SetItem(data, i, point);
        }
    }

    return data;
}

static PyMemberDef ControlRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ControlRec, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(ControlRec, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(ControlRec, input), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRec_methods[] =
{
    {"getServer", (PyCFunction)ControlRec_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)ControlRec_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)ControlRec_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)ControlRec_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getData", (PyCFunction)ControlRec_getData, METH_NOARGS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject ControlRecType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.ControlRec_base",                                   /*tp_name*/
    sizeof(ControlRec),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)ControlRec_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)ControlRec_traverse,                  /* tp_traverse */
    (inquiry)ControlRec_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    ControlRec_methods,                                 /* tp_methods */
    ControlRec_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    ControlRec_new,                                     /* tp_new */
};

/**************/
/* ControlRead object */
/**************/
typedef struct
{
    pyo_audio_HEAD
    MYFLT *values;
    int rate;
    int modulo;
    int loop;
    int go;
    int modebuffer[2];
    T_SIZE_T count;
    long time;
    T_SIZE_T size;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, T_SIZE_T, MYFLT, T_SIZE_T);
} ControlRead;

static void
ControlRead_readframes_i(ControlRead *self)
{
    MYFLT fpart;
    long i, mod;
    MYFLT invmodulo = 1.0 / self->modulo;

    if (self->go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (self->go == 1)
        {
            mod = self->time % self->modulo;
            fpart = mod * invmodulo;
            self->data[i] = (*self->interp_func_ptr)(self->values, self->count, fpart, self->size);
        }
        else
        {
            mod = -1;
            self->data[i] = 0.0;
        }

        if (mod == 0)
        {
            self->count++;

            if (self->count >= (self->size - 1))
            {
                self->trigsBuffer[i] = 1.0;

                if (self->loop == 1)
                    self->count = 0;
                else
                    self->go = 0;
            }
        }

        self->time++;
    }
}

static void ControlRead_postprocessing_ii(ControlRead *self) { POST_PROCESSING_II };
static void ControlRead_postprocessing_ai(ControlRead *self) { POST_PROCESSING_AI };
static void ControlRead_postprocessing_ia(ControlRead *self) { POST_PROCESSING_IA };
static void ControlRead_postprocessing_aa(ControlRead *self) { POST_PROCESSING_AA };
static void ControlRead_postprocessing_ireva(ControlRead *self) { POST_PROCESSING_IREVA };
static void ControlRead_postprocessing_areva(ControlRead *self) { POST_PROCESSING_AREVA };
static void ControlRead_postprocessing_revai(ControlRead *self) { POST_PROCESSING_REVAI };
static void ControlRead_postprocessing_revaa(ControlRead *self) { POST_PROCESSING_REVAA };
static void ControlRead_postprocessing_revareva(ControlRead *self) { POST_PROCESSING_REVAREVA };

static void
ControlRead_setProcMode(ControlRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = ControlRead_readframes_i;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = ControlRead_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = ControlRead_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = ControlRead_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = ControlRead_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = ControlRead_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = ControlRead_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = ControlRead_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = ControlRead_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = ControlRead_postprocessing_revareva;
            break;
    }
}

static void
ControlRead_compute_next_data_frame(ControlRead *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
ControlRead_traverse(ControlRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
ControlRead_clear(ControlRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
ControlRead_dealloc(ControlRead* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->values);
    PyMem_RawFree(self->trigsBuffer);
    ControlRead_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ControlRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *multmp = NULL, *addtmp = NULL;
    ControlRead *self;
    self = (ControlRead *)type->tp_alloc(type, 0);

    self->loop = 0;
    self->rate = 1000;
    self->interp = 2;
    self->go = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ControlRead_compute_next_data_frame);
    self->mode_func_ptr = ControlRead_setProcMode;

    static char *kwlist[] = {"values", "rate", "loop", "interp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiOO", kwlist, &valuestmp, &self->rate, &self->loop, &self->interp, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (valuestmp)
    {
        PyObject_CallMethod((PyObject *)self, "setValues", "O", valuestmp);
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

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    self->modulo = (int)(self->sr / self->rate);

    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    return (PyObject *)self;
}

static PyObject * ControlRead_getServer(ControlRead* self) { GET_SERVER };
static PyObject * ControlRead_getStream(ControlRead* self) { GET_STREAM };
static PyObject * ControlRead_getTriggerStream(ControlRead* self) { GET_TRIGGER_STREAM };
static PyObject * ControlRead_setMul(ControlRead *self, PyObject *arg) { SET_MUL };
static PyObject * ControlRead_setAdd(ControlRead *self, PyObject *arg) { SET_ADD };
static PyObject * ControlRead_setSub(ControlRead *self, PyObject *arg) { SET_SUB };
static PyObject * ControlRead_setDiv(ControlRead *self, PyObject *arg) { SET_DIV };

static PyObject * ControlRead_play(ControlRead *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    self->go = 1;
    PLAY
};

static PyObject * ControlRead_stop(ControlRead *self, PyObject *args, PyObject *kwds)
{
    self->go = 0;
    STOP
};

static PyObject * ControlRead_multiply(ControlRead *self, PyObject *arg) { MULTIPLY };
static PyObject * ControlRead_inplace_multiply(ControlRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ControlRead_add(ControlRead *self, PyObject *arg) { ADD };
static PyObject * ControlRead_inplace_add(ControlRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ControlRead_sub(ControlRead *self, PyObject *arg) { SUB };
static PyObject * ControlRead_inplace_sub(ControlRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ControlRead_div(ControlRead *self, PyObject *arg) { DIV };
static PyObject * ControlRead_inplace_div(ControlRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ControlRead_setValues(ControlRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->values = (MYFLT *)PyMem_RawRealloc(self->values, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
    {
        self->values[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
    }

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setRate(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->rate = PyLong_AsLong(arg);
    self->modulo = (int)(self->sr / self->rate);

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setLoop(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject *
ControlRead_setInterp(ControlRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->interp = PyLong_AsLong(PyNumber_Long(arg));
    }

    SET_INTERP_POINTER

    Py_RETURN_NONE;
}

static PyMemberDef ControlRead_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ControlRead, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(ControlRead, stream), 0, NULL},
    {"trig_stream", T_OBJECT_EX, offsetof(ControlRead, trig_stream), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(ControlRead, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(ControlRead, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRead_methods[] =
{
    {"getServer", (PyCFunction)ControlRead_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)ControlRead_getStream, METH_NOARGS, NULL},
    {"_getTriggerStream", (PyCFunction)ControlRead_getTriggerStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)ControlRead_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)ControlRead_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setValues", (PyCFunction)ControlRead_setValues, METH_O, NULL},
    {"setRate", (PyCFunction)ControlRead_setRate, METH_O, NULL},
    {"setLoop", (PyCFunction)ControlRead_setLoop, METH_O, NULL},
    {"setInterp", (PyCFunction)ControlRead_setInterp, METH_O, NULL},
    {"setMul", (PyCFunction)ControlRead_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)ControlRead_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)ControlRead_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)ControlRead_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods ControlRead_as_number =
{
    (binaryfunc)ControlRead_add,                      /*nb_add*/
    (binaryfunc)ControlRead_sub,                 /*nb_subtract*/
    (binaryfunc)ControlRead_multiply,                 /*nb_multiply*/
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
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    (binaryfunc)ControlRead_inplace_add,              /*inplace_add*/
    (binaryfunc)ControlRead_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)ControlRead_inplace_multiply,         /*inplace_multiply*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)ControlRead_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)ControlRead_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject ControlReadType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.ControlRead_base",         /*tp_name*/
    sizeof(ControlRead),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ControlRead_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &ControlRead_as_number,             /*tp_as_number*/
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
    (traverseproc)ControlRead_traverse,   /* tp_traverse */
    (inquiry)ControlRead_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    ControlRead_methods,             /* tp_methods */
    ControlRead_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    ControlRead_new,                 /* tp_new */
};

/************/
/* NoteinRec */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *inputp;
    Stream *inputp_stream;
    PyObject *inputv;
    Stream *inputv_stream;
    PyObject *tmp_list_p;
    PyObject *tmp_list_v;
    PyObject *tmp_list_t;
    MYFLT last_pitch;
    MYFLT last_vel;
    long time;
} NoteinRec;

static void
NoteinRec_process(NoteinRec *self)
{
    int i;
    MYFLT pit, vel;

    MYFLT *inp = Stream_getData((Stream *)self->inputp_stream);
    MYFLT *inv = Stream_getData((Stream *)self->inputv_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        pit = inp[i];
        vel = inv[i];

        if (pit != self->last_pitch || vel != self->last_vel)
        {
            self->last_pitch = pit;
            self->last_vel = vel;
            PyList_Append(self->tmp_list_p, PyFloat_FromDouble(pit));
            PyList_Append(self->tmp_list_v, PyFloat_FromDouble(vel));
            PyList_Append(self->tmp_list_t, PyFloat_FromDouble( (float)self->time / self->sr) );
        }

        self->time++;
    }
}

static void
NoteinRec_setProcMode(NoteinRec *self)
{
    self->proc_func_ptr = NoteinRec_process;
}

static void
NoteinRec_compute_next_data_frame(NoteinRec *self)
{
    (*self->proc_func_ptr)(self);
}

static int
NoteinRec_traverse(NoteinRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inputp);
    Py_VISIT(self->inputp_stream);
    Py_VISIT(self->inputv);
    Py_VISIT(self->inputv_stream);
    Py_VISIT(self->tmp_list_p);
    Py_VISIT(self->tmp_list_v);
    Py_VISIT(self->tmp_list_t);
    return 0;
}

static int
NoteinRec_clear(NoteinRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inputp);
    Py_CLEAR(self->inputp_stream);
    Py_CLEAR(self->inputv);
    Py_CLEAR(self->inputv_stream);
    Py_CLEAR(self->tmp_list_p);
    Py_CLEAR(self->tmp_list_v);
    Py_CLEAR(self->tmp_list_t);
    return 0;
}

static void
NoteinRec_dealloc(NoteinRec* self)
{
    pyo_DEALLOC
    NoteinRec_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NoteinRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputptmp, *inputp_streamtmp, *inputvtmp, *inputv_streamtmp;
    NoteinRec *self;
    self = (NoteinRec *)type->tp_alloc(type, 0);

    self->tmp_list_p = PyList_New(0);
    self->tmp_list_v = PyList_New(0);
    self->tmp_list_t = PyList_New(0);
    self->last_pitch = self->last_vel = 0.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, NoteinRec_compute_next_data_frame);
    self->mode_func_ptr = NoteinRec_setProcMode;

    static char *kwlist[] = {"inputp", "inputv", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputptmp, &inputvtmp))
        Py_RETURN_NONE;

    self->inputp = inputptmp;
    Py_INCREF(self->inputp);
    inputp_streamtmp = PyObject_CallMethod((PyObject *)self->inputp, "_getStream", NULL);
    self->inputp_stream = (Stream *)inputp_streamtmp;
    Py_INCREF(self->inputp_stream);

    self->inputv = inputvtmp;
    Py_INCREF(self->inputv);
    inputv_streamtmp = PyObject_CallMethod((PyObject *)self->inputv, "_getStream", NULL);
    self->inputv_stream = (Stream *)inputv_streamtmp;
    Py_INCREF(self->inputv_stream);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NoteinRec_getServer(NoteinRec* self) { GET_SERVER };
static PyObject * NoteinRec_getStream(NoteinRec* self) { GET_STREAM };

static PyObject * NoteinRec_play(NoteinRec *self, PyObject *args, PyObject *kwds)
{
    self->time = 0;
    PLAY
};

static PyObject * NoteinRec_stop(NoteinRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
NoteinRec_getData(NoteinRec *self)
{
    int i;
    PyObject *data, *point;

    Py_ssize_t size = PyList_Size(self->tmp_list_p);
    data = PyList_New(size);

    for (i = 0; i < size; i++)
    {
        point = PyTuple_New(3);
        PyTuple_SET_ITEM(point, 0, PyList_GET_ITEM(self->tmp_list_t, i));
        PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list_p, i));
        PyTuple_SET_ITEM(point, 2, PyList_GET_ITEM(self->tmp_list_v, i));
        PyList_SetItem(data, i, point);
    }

    return data;
}

static PyMemberDef NoteinRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NoteinRec, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(NoteinRec, stream), 0, NULL},
    {"inputp", T_OBJECT_EX, offsetof(NoteinRec, inputp), 0, NULL},
    {"inputv", T_OBJECT_EX, offsetof(NoteinRec, inputv), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRec_methods[] =
{
    {"getServer", (PyCFunction)NoteinRec_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)NoteinRec_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)NoteinRec_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)NoteinRec_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getData", (PyCFunction)NoteinRec_getData, METH_NOARGS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject NoteinRecType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.NoteinRec_base",                                   /*tp_name*/
    sizeof(NoteinRec),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)NoteinRec_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    0,                                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)NoteinRec_traverse,                  /* tp_traverse */
    (inquiry)NoteinRec_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    NoteinRec_methods,                                 /* tp_methods */
    NoteinRec_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    NoteinRec_new,                                     /* tp_new */
};

/**************/
/* NoteinRead object */
/**************/
typedef struct
{
    pyo_audio_HEAD
    MYFLT *values;
    long *timestamps;
    MYFLT value;
    int loop;
    int go;
    int modebuffer[2];
    long count;
    long time;
    long size;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} NoteinRead;

static void
NoteinRead_readframes_i(NoteinRead *self)
{
    long i;

    if (self->go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (self->go == 1)
        {
            if (self->time >= self->timestamps[self->count])
            {
                self->value = self->values[self->count];
                self->data[i] = self->value;
                self->count++;
            }
            else
                self->data[i] = self->value;
        }
        else
            self->data[i] = 0.0;

        if (self->count >= self->size)
        {
            self->trigsBuffer[i] = 1.0;

            if (self->loop == 1)
                self->time = self->count = 0;
            else
                self->go = 0;
        }

        self->time++;
    }
}

static void NoteinRead_postprocessing_ii(NoteinRead *self) { POST_PROCESSING_II };
static void NoteinRead_postprocessing_ai(NoteinRead *self) { POST_PROCESSING_AI };
static void NoteinRead_postprocessing_ia(NoteinRead *self) { POST_PROCESSING_IA };
static void NoteinRead_postprocessing_aa(NoteinRead *self) { POST_PROCESSING_AA };
static void NoteinRead_postprocessing_ireva(NoteinRead *self) { POST_PROCESSING_IREVA };
static void NoteinRead_postprocessing_areva(NoteinRead *self) { POST_PROCESSING_AREVA };
static void NoteinRead_postprocessing_revai(NoteinRead *self) { POST_PROCESSING_REVAI };
static void NoteinRead_postprocessing_revaa(NoteinRead *self) { POST_PROCESSING_REVAA };
static void NoteinRead_postprocessing_revareva(NoteinRead *self) { POST_PROCESSING_REVAREVA };

static void
NoteinRead_setProcMode(NoteinRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = NoteinRead_readframes_i;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = NoteinRead_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = NoteinRead_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = NoteinRead_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = NoteinRead_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = NoteinRead_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = NoteinRead_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = NoteinRead_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = NoteinRead_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = NoteinRead_postprocessing_revareva;
            break;
    }
}

static void
NoteinRead_compute_next_data_frame(NoteinRead *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
NoteinRead_traverse(NoteinRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int
NoteinRead_clear(NoteinRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
NoteinRead_dealloc(NoteinRead* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->values);
    PyMem_RawFree(self->timestamps);
    PyMem_RawFree(self->trigsBuffer);
    NoteinRead_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NoteinRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *timestampstmp, *multmp = NULL, *addtmp = NULL;
    NoteinRead *self;
    self = (NoteinRead *)type->tp_alloc(type, 0);

    self->value = 0.0;
    self->loop = 0;
    self->go = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, NoteinRead_compute_next_data_frame);
    self->mode_func_ptr = NoteinRead_setProcMode;

    static char *kwlist[] = {"values", "timestamps", "loop", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &valuestmp, &timestampstmp, &self->loop, &multmp, &addtmp))
        Py_RETURN_NONE;

    if (valuestmp)
    {
        PyObject_CallMethod((PyObject *)self, "setValues", "O", valuestmp);
    }

    if (timestampstmp)
    {
        PyObject_CallMethod((PyObject *)self, "setTimestamps", "O", timestampstmp);
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

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NoteinRead_getServer(NoteinRead* self) { GET_SERVER };
static PyObject * NoteinRead_getStream(NoteinRead* self) { GET_STREAM };
static PyObject * NoteinRead_getTriggerStream(NoteinRead* self) { GET_TRIGGER_STREAM };
static PyObject * NoteinRead_setMul(NoteinRead *self, PyObject *arg) { SET_MUL };
static PyObject * NoteinRead_setAdd(NoteinRead *self, PyObject *arg) { SET_ADD };
static PyObject * NoteinRead_setSub(NoteinRead *self, PyObject *arg) { SET_SUB };
static PyObject * NoteinRead_setDiv(NoteinRead *self, PyObject *arg) { SET_DIV };

static PyObject * NoteinRead_play(NoteinRead *self, PyObject *args, PyObject *kwds)
{
    self->count = self->time = 0;
    self->go = 1;
    PLAY
};

static PyObject * NoteinRead_stop(NoteinRead *self, PyObject *args, PyObject *kwds)
{
    self->go = 0;
    STOP
};

static PyObject * NoteinRead_multiply(NoteinRead *self, PyObject *arg) { MULTIPLY };
static PyObject * NoteinRead_inplace_multiply(NoteinRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NoteinRead_add(NoteinRead *self, PyObject *arg) { ADD };
static PyObject * NoteinRead_inplace_add(NoteinRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NoteinRead_sub(NoteinRead *self, PyObject *arg) { SUB };
static PyObject * NoteinRead_inplace_sub(NoteinRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NoteinRead_div(NoteinRead *self, PyObject *arg) { DIV };
static PyObject * NoteinRead_inplace_div(NoteinRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
NoteinRead_setValues(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->values = (MYFLT *)PyMem_RawRealloc(self->values, self->size * sizeof(MYFLT));

    for (i = 0; i < self->size; i++)
    {
        self->values[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
    }

    Py_RETURN_NONE;
}

static PyObject *
NoteinRead_setTimestamps(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;

    ASSERT_ARG_NOT_NULL

    self->size = PyList_Size(arg);
    self->timestamps = (long *)PyMem_RawRealloc(self->timestamps, self->size * sizeof(long));

    for (i = 0; i < self->size; i++)
    {
        self->timestamps[i] = (long)(PyFloat_AsDouble(PyList_GET_ITEM(arg, i)) * self->sr);
    }

    Py_RETURN_NONE;
}

static PyObject *
NoteinRead_setLoop(NoteinRead *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyMemberDef NoteinRead_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NoteinRead, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(NoteinRead, stream), 0, NULL},
    {"trig_stream", T_OBJECT_EX, offsetof(NoteinRead, trig_stream), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(NoteinRead, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(NoteinRead, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRead_methods[] =
{
    {"getServer", (PyCFunction)NoteinRead_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)NoteinRead_getStream, METH_NOARGS, NULL},
    {"_getTriggerStream", (PyCFunction)NoteinRead_getTriggerStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)NoteinRead_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)NoteinRead_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setValues", (PyCFunction)NoteinRead_setValues, METH_O, NULL},
    {"setTimestamps", (PyCFunction)NoteinRead_setTimestamps, METH_O, NULL},
    {"setLoop", (PyCFunction)NoteinRead_setLoop, METH_O, NULL},
    {"setMul", (PyCFunction)NoteinRead_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)NoteinRead_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)NoteinRead_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)NoteinRead_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods NoteinRead_as_number =
{
    (binaryfunc)NoteinRead_add,                      /*nb_add*/
    (binaryfunc)NoteinRead_sub,                 /*nb_subtract*/
    (binaryfunc)NoteinRead_multiply,                 /*nb_multiply*/
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
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    (binaryfunc)NoteinRead_inplace_add,              /*inplace_add*/
    (binaryfunc)NoteinRead_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)NoteinRead_inplace_multiply,         /*inplace_multiply*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)NoteinRead_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)NoteinRead_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject NoteinReadType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.NoteinRead_base",         /*tp_name*/
    sizeof(NoteinRead),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NoteinRead_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &NoteinRead_as_number,             /*tp_as_number*/
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
    (traverseproc)NoteinRead_traverse,   /* tp_traverse */
    (inquiry)NoteinRead_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    NoteinRead_methods,             /* tp_methods */
    NoteinRead_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    NoteinRead_new,                 /* tp_new */
};