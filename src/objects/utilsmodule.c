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
#include "interpolation.h"

/************/
/* Print */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    const char *message;
    int method; // 0 -> interval, 1 -> change
    MYFLT lastValue;
    MYFLT time;
    MYFLT currentTime;
    MYFLT sampleToSec;
} Print;

static void
Print_process_time(Print *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->currentTime >= self->time)
        {
            self->currentTime = 0.0;

            if (self->message == NULL || self->message[0] == '\0')
                PySys_WriteStdout("%f\n", in[i]);
            else
                PySys_WriteStdout("%s : %f\n", self->message, in[i]);
        }

        self->currentTime += self->sampleToSec;
    }
}

static void
Print_process_change(Print *self)
{
    int i;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        inval = in[i];

        if (inval < (self->lastValue - 0.00001) || inval > (self->lastValue + 0.00001))
        {
            if (self->message == NULL || self->message[0] == '\0')
                PySys_WriteStdout("%f\n", inval);
            else
                PySys_WriteStdout("%s : %f\n", self->message, inval);

            self->lastValue = inval;
        }
    }
}

static void
Print_setProcMode(Print *self)
{
    if (self->method < 0 || self->method > 1)
        self->method = 0;

    switch (self->method)
    {
        case 0:
            self->proc_func_ptr = Print_process_time;
            break;

        case 1:
            self->proc_func_ptr = Print_process_change;
            break;
    }
}

static void
Print_compute_next_data_frame(Print *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Print_traverse(Print *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Print_clear(Print *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Print_dealloc(Print* self)
{
    pyo_DEALLOC
    Print_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Print_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    Print *self;
    self = (Print *)type->tp_alloc(type, 0);

    self->method = 0;
    self->time = 0.25;
    self->lastValue = -99999.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Print_compute_next_data_frame);
    self->mode_func_ptr = Print_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;

    static char *kwlist[] = {"input", "method", "interval", "message", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IFS, kwlist, &inputtmp, &self->method, &self->time, &self->message))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Print_getServer(Print* self) { GET_SERVER };
static PyObject * Print_getStream(Print* self) { GET_STREAM };

static PyObject * Print_play(Print *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Print_stop(Print *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Print_setMethod(Print *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg) == 1)
    {
        self->method = PyLong_AsLong(arg);
        (*self->mode_func_ptr)(self);
    }

    Py_RETURN_NONE;
}

static PyObject *
Print_setInterval(Print *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg) == 1)
        self->time = PyFloat_AsDouble(arg);

    Py_RETURN_NONE;
}

static PyObject *
Print_setMessage(Print *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyUnicode_Check(arg))
        self->message = PyUnicode_AsUTF8(arg);

    Py_RETURN_NONE;
}

static PyMemberDef Print_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Print, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Print, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Print, input), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Print_methods[] =
{
    {"getServer", (PyCFunction)Print_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Print_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Print_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Print_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMethod", (PyCFunction)Print_setMethod, METH_O, NULL},
    {"setInterval", (PyCFunction)Print_setInterval, METH_O, NULL},
    {"setMessage", (PyCFunction)Print_setMessage, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PrintType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Print_base",                                   /*tp_name*/
    sizeof(Print),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Print_dealloc,                     /*tp_dealloc*/
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
    (traverseproc)Print_traverse,                  /* tp_traverse */
    (inquiry)Print_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Print_methods,                                 /* tp_methods */
    Print_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Print_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* Snap ********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int scale; // 0 = Midi, 1 = frequency, 2 = transpo
    int chSize;
    int highbound;
    MYFLT *choice;
    MYFLT value;
    MYFLT last_input;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Snap;

static MYFLT
Snap_convert(Snap *self)
{
    int midival;
    MYFLT val;

    midival = self->value;

    if (self->scale == 1)
        val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
    else if (self->scale == 2)
        val = MYPOW(1.0594630943593, midival - 60);
    else
        val = midival;

    return val;
}

static void
Snap_generate(Snap *self)
{
    int i, j, pos;
    MYFLT intmp, diff, difftmp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] < (self->last_input - 0.001) || in[i] > (self->last_input + 0.001))
        {
            int oct = 0;
            self->last_input = intmp = in[i];

            while (intmp >= self->highbound)
            {
                oct++;
                intmp -= self->highbound;
            }

            diff = MYFABS(self->choice[0] - intmp);
            pos = 0;

            for (j = 1; j < self->chSize; j++)
            {
                difftmp = MYFABS(self->choice[j] - intmp);

                if (difftmp < diff)
                {
                    diff = difftmp;
                    pos = j;
                }
            }

            self->value = self->choice[pos] + (self->highbound * oct);
            self->value = Snap_convert(self);
        }

        self->data[i] = self->value;
    }
}

static void Snap_postprocessing_ii(Snap *self) { POST_PROCESSING_II };
static void Snap_postprocessing_ai(Snap *self) { POST_PROCESSING_AI };
static void Snap_postprocessing_ia(Snap *self) { POST_PROCESSING_IA };
static void Snap_postprocessing_aa(Snap *self) { POST_PROCESSING_AA };
static void Snap_postprocessing_ireva(Snap *self) { POST_PROCESSING_IREVA };
static void Snap_postprocessing_areva(Snap *self) { POST_PROCESSING_AREVA };
static void Snap_postprocessing_revai(Snap *self) { POST_PROCESSING_REVAI };
static void Snap_postprocessing_revaa(Snap *self) { POST_PROCESSING_REVAA };
static void Snap_postprocessing_revareva(Snap *self) { POST_PROCESSING_REVAREVA };

static void
Snap_setProcMode(Snap *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Snap_generate;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Snap_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Snap_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Snap_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Snap_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Snap_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Snap_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Snap_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Snap_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Snap_postprocessing_revareva;
            break;
    }
}

static void
Snap_compute_next_data_frame(Snap *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Snap_traverse(Snap *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Snap_clear(Snap *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Snap_dealloc(Snap* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->choice);
    Snap_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Snap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *choicetmp = NULL, *multmp = NULL, *addtmp = NULL;
    Snap *self;
    self = (Snap *)type->tp_alloc(type, 0);

    self->value = self->last_input = 0.;
    self->scale = 0;
    self->highbound = 12;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Snap_compute_next_data_frame);
    self->mode_func_ptr = Snap_setProcMode;

    static char *kwlist[] = {"input", "choice", "scale", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &inputtmp, &choicetmp, &self->scale, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (choicetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setChoice", "O", choicetmp);
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

static PyObject * Snap_getServer(Snap* self) { GET_SERVER };
static PyObject * Snap_getStream(Snap* self) { GET_STREAM };
static PyObject * Snap_setMul(Snap *self, PyObject *arg) { SET_MUL };
static PyObject * Snap_setAdd(Snap *self, PyObject *arg) { SET_ADD };
static PyObject * Snap_setSub(Snap *self, PyObject *arg) { SET_SUB };
static PyObject * Snap_setDiv(Snap *self, PyObject *arg) { SET_DIV };

static PyObject * Snap_play(Snap *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Snap_out(Snap *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Snap_stop(Snap *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Snap_multiply(Snap *self, PyObject *arg) { MULTIPLY };
static PyObject * Snap_inplace_multiply(Snap *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Snap_add(Snap *self, PyObject *arg) { ADD };
static PyObject * Snap_inplace_add(Snap *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Snap_sub(Snap *self, PyObject *arg) { SUB };
static PyObject * Snap_inplace_sub(Snap *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Snap_div(Snap *self, PyObject *arg) { DIV };
static PyObject * Snap_inplace_div(Snap *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Snap_setChoice(Snap *self, PyObject *arg)
{
    int i, oct;
    MYFLT max;

    if (! PyList_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The choice attribute must be a list.");
        Py_RETURN_NONE;
    }

    self->chSize = PyList_Size(arg);
    self->choice = (MYFLT *)PyMem_RawRealloc(self->choice, self->chSize * sizeof(MYFLT));

    for (i = 0; i < self->chSize; i++)
    {
        self->choice[i] = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
    }

    max = self->choice[self->chSize - 1];

    oct = 12;

    while (max >= oct)
    {
        oct += 12;
    }

    self->highbound = oct;

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
Snap_setScale(Snap *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp <= 2)
            self->scale = tmp;
        else
            PySys_WriteStdout("Snap: scale attribute must be an integer {0, 1, 2}\n");
    }

    Py_RETURN_NONE;
}

static PyMemberDef Snap_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Snap, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Snap, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Snap, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Snap, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Snap, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Snap_methods[] =
{
    {"getServer", (PyCFunction)Snap_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Snap_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Snap_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Snap_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Snap_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setChoice", (PyCFunction)Snap_setChoice, METH_O, NULL},
    {"setScale", (PyCFunction)Snap_setScale, METH_O, NULL},
    {"setMul", (PyCFunction)Snap_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Snap_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Snap_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Snap_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Snap_as_number =
{
    (binaryfunc)Snap_add,                         /*nb_add*/
    (binaryfunc)Snap_sub,                         /*nb_subtract*/
    (binaryfunc)Snap_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Snap_inplace_add,                 /*inplace_add*/
    (binaryfunc)Snap_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Snap_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Snap_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Snap_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject SnapType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Snap_base",                                   /*tp_name*/
    sizeof(Snap),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Snap_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Snap_as_number,                              /*tp_as_number*/
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
    (traverseproc)Snap_traverse,                  /* tp_traverse */
    (inquiry)Snap_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Snap_methods,                                 /* tp_methods */
    Snap_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Snap_new,                                     /* tp_new */
};

/************/
/* Interp */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    PyObject *interp;
    Stream *interp_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
} Interp;

static void
Interp_filters_i(Interp *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    MYFLT inter = PyFloat_AS_DOUBLE(self->interp);

    if (inter < 0.0)
        inter = 0.0;
    else if (inter > 1.0)
        inter = 1.0;

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = in[i] + (in2[i] - in[i]) * inter;
    }
}

static void
Interp_filters_a(Interp *self)
{
    MYFLT amp1;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    MYFLT *inter = Stream_getData((Stream *)self->interp_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        amp1 = inter[i];

        if (amp1 < 0.0)
            amp1 = 0.0;
        else if (amp1 > 1.0)
            amp1 = 1.0;

        self->data[i] = in[i] + (in2[i] - in[i]) * amp1;
    }
}

static void Interp_postprocessing_ii(Interp *self) { POST_PROCESSING_II };
static void Interp_postprocessing_ai(Interp *self) { POST_PROCESSING_AI };
static void Interp_postprocessing_ia(Interp *self) { POST_PROCESSING_IA };
static void Interp_postprocessing_aa(Interp *self) { POST_PROCESSING_AA };
static void Interp_postprocessing_ireva(Interp *self) { POST_PROCESSING_IREVA };
static void Interp_postprocessing_areva(Interp *self) { POST_PROCESSING_AREVA };
static void Interp_postprocessing_revai(Interp *self) { POST_PROCESSING_REVAI };
static void Interp_postprocessing_revaa(Interp *self) { POST_PROCESSING_REVAA };
static void Interp_postprocessing_revareva(Interp *self) { POST_PROCESSING_REVAREVA };

static void
Interp_setProcMode(Interp *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = Interp_filters_i;
            break;

        case 1:
            self->proc_func_ptr = Interp_filters_a;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Interp_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Interp_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Interp_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Interp_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Interp_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Interp_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Interp_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Interp_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Interp_postprocessing_revareva;
            break;
    }
}

static void
Interp_compute_next_data_frame(Interp *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Interp_traverse(Interp *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    Py_VISIT(self->interp);
    return 0;
}

static int
Interp_clear(Interp *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->interp);
    return 0;
}

static void
Interp_dealloc(Interp* self)
{
    pyo_DEALLOC
    Interp_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Interp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *interptmp = NULL, *multmp = NULL, *addtmp = NULL;
    Interp *self;
    self = (Interp *)type->tp_alloc(type, 0);

    self->interp = PyFloat_FromDouble(.5);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Interp_compute_next_data_frame);
    self->mode_func_ptr = Interp_setProcMode;

    static char *kwlist[] = {"input", "input2", "interp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &input2tmp, &interptmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    self->input2 = input2tmp;
    Py_INCREF(self->input2);
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    self->input2_stream = (Stream *)input2_streamtmp;
    Py_INCREF(self->input2_stream);

    if (interptmp)
    {
        PyObject_CallMethod((PyObject *)self, "setInterp", "O", interptmp);
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

static PyObject * Interp_getServer(Interp* self) { GET_SERVER };
static PyObject * Interp_getStream(Interp* self) { GET_STREAM };
static PyObject * Interp_setMul(Interp *self, PyObject *arg) { SET_MUL };
static PyObject * Interp_setAdd(Interp *self, PyObject *arg) { SET_ADD };
static PyObject * Interp_setSub(Interp *self, PyObject *arg) { SET_SUB };
static PyObject * Interp_setDiv(Interp *self, PyObject *arg) { SET_DIV };

static PyObject * Interp_play(Interp *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Interp_out(Interp *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Interp_stop(Interp *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Interp_multiply(Interp *self, PyObject *arg) { MULTIPLY };
static PyObject * Interp_inplace_multiply(Interp *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Interp_add(Interp *self, PyObject *arg) { ADD };
static PyObject * Interp_inplace_add(Interp *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Interp_sub(Interp *self, PyObject *arg) { SUB };
static PyObject * Interp_inplace_sub(Interp *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Interp_div(Interp *self, PyObject *arg) { DIV };
static PyObject * Interp_inplace_div(Interp *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Interp_setInterp(Interp *self, PyObject *arg) { SET_PARAM(self->interp, self->interp_stream, 2); }

static PyMemberDef Interp_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Interp, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Interp, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Interp, input), 0, NULL},
    {"input2", T_OBJECT_EX, offsetof(Interp, input2), 0, NULL},
    {"interp", T_OBJECT_EX, offsetof(Interp, interp), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Interp, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Interp, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Interp_methods[] =
{
    {"getServer", (PyCFunction)Interp_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Interp_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Interp_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Interp_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Interp_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setInterp", (PyCFunction)Interp_setInterp, METH_O, NULL},
    {"setMul", (PyCFunction)Interp_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Interp_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Interp_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Interp_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Interp_as_number =
{
    (binaryfunc)Interp_add,                         /*nb_add*/
    (binaryfunc)Interp_sub,                         /*nb_subtract*/
    (binaryfunc)Interp_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Interp_inplace_add,                 /*inplace_add*/
    (binaryfunc)Interp_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Interp_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Interp_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Interp_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject InterpType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Interp_base",                                   /*tp_name*/
    sizeof(Interp),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Interp_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Interp_as_number,                              /*tp_as_number*/
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
    (traverseproc)Interp_traverse,                  /* tp_traverse */
    (inquiry)Interp_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Interp_methods,                                 /* tp_methods */
    Interp_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Interp_new,                                     /* tp_new */
};

/************/
/* SampHold */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *controlsig;
    Stream *controlsig_stream;
    PyObject *value;
    Stream *value_stream;
    MYFLT currentValue;
    int flag;
    int modebuffer[3]; // need at least 2 slots for mul & add
} SampHold;

static void
SampHold_filters_i(SampHold *self)
{
    MYFLT ctrl;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT val = PyFloat_AS_DOUBLE(self->value);

    for (i = 0; i < self->bufsize; i++)
    {
        ctrl = ctrlsig[i];

        if (ctrl > (val - 0.001) && ctrl < (val + 0.001))
        {
            if (self->flag == 1)
            {
                self->currentValue = in[i];
                self->flag = 0;
            }
        }
        else
            self->flag = 1;

        self->data[i] = self->currentValue;
    }
}

static void
SampHold_filters_a(SampHold *self)
{
    MYFLT ctrl, val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT *valsig = Stream_getData((Stream *)self->value_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        ctrl = ctrlsig[i];
        val = valsig[i];

        if (ctrl > (val - 0.001) && ctrl < (val + 0.001))
        {
            if (self->flag == 1)
            {
                self->currentValue = in[i];
                self->flag = 0;
            }
        }
        else
            self->flag = 1;

        self->data[i] = self->currentValue;
    }
}

static void SampHold_postprocessing_ii(SampHold *self) { POST_PROCESSING_II };
static void SampHold_postprocessing_ai(SampHold *self) { POST_PROCESSING_AI };
static void SampHold_postprocessing_ia(SampHold *self) { POST_PROCESSING_IA };
static void SampHold_postprocessing_aa(SampHold *self) { POST_PROCESSING_AA };
static void SampHold_postprocessing_ireva(SampHold *self) { POST_PROCESSING_IREVA };
static void SampHold_postprocessing_areva(SampHold *self) { POST_PROCESSING_AREVA };
static void SampHold_postprocessing_revai(SampHold *self) { POST_PROCESSING_REVAI };
static void SampHold_postprocessing_revaa(SampHold *self) { POST_PROCESSING_REVAA };
static void SampHold_postprocessing_revareva(SampHold *self) { POST_PROCESSING_REVAREVA };

static void
SampHold_setProcMode(SampHold *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = SampHold_filters_i;
            break;

        case 1:
            self->proc_func_ptr = SampHold_filters_a;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = SampHold_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = SampHold_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = SampHold_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = SampHold_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = SampHold_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = SampHold_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = SampHold_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = SampHold_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = SampHold_postprocessing_revareva;
            break;
    }
}

static void
SampHold_compute_next_data_frame(SampHold *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
SampHold_traverse(SampHold *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->controlsig);
    Py_VISIT(self->value);
    return 0;
}

static int
SampHold_clear(SampHold *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->controlsig);
    Py_CLEAR(self->value);
    return 0;
}

static void
SampHold_dealloc(SampHold* self)
{
    pyo_DEALLOC
    SampHold_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SampHold_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *controlsigtmp, *controlsig_streamtmp, *valuetmp = NULL, *multmp = NULL, *addtmp = NULL;
    SampHold *self;
    self = (SampHold *)type->tp_alloc(type, 0);

    self->value = PyFloat_FromDouble(0.0);
    self->currentValue = 0.0;
    self->flag = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SampHold_compute_next_data_frame);
    self->mode_func_ptr = SampHold_setProcMode;

    static char *kwlist[] = {"input", "controlsig", "value", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &controlsigtmp, &valuetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    self->controlsig = controlsigtmp;
    Py_INCREF(self->controlsig);
    controlsig_streamtmp = PyObject_CallMethod((PyObject *)self->controlsig, "_getStream", NULL);
    self->controlsig_stream = (Stream *)controlsig_streamtmp;
    Py_INCREF(self->controlsig_stream);

    if (valuetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
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

static PyObject * SampHold_getServer(SampHold* self) { GET_SERVER };
static PyObject * SampHold_getStream(SampHold* self) { GET_STREAM };
static PyObject * SampHold_setMul(SampHold *self, PyObject *arg) { SET_MUL };
static PyObject * SampHold_setAdd(SampHold *self, PyObject *arg) { SET_ADD };
static PyObject * SampHold_setSub(SampHold *self, PyObject *arg) { SET_SUB };
static PyObject * SampHold_setDiv(SampHold *self, PyObject *arg) { SET_DIV };

static PyObject * SampHold_play(SampHold *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SampHold_out(SampHold *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SampHold_stop(SampHold *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SampHold_multiply(SampHold *self, PyObject *arg) { MULTIPLY };
static PyObject * SampHold_inplace_multiply(SampHold *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SampHold_add(SampHold *self, PyObject *arg) { ADD };
static PyObject * SampHold_inplace_add(SampHold *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SampHold_sub(SampHold *self, PyObject *arg) { SUB };
static PyObject * SampHold_inplace_sub(SampHold *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SampHold_div(SampHold *self, PyObject *arg) { DIV };
static PyObject * SampHold_inplace_div(SampHold *self, PyObject *arg) { INPLACE_DIV };

static PyObject * SampHold_setValue(SampHold *self, PyObject *arg) { SET_PARAM(self->value, self->value_stream, 2); }

static PyMemberDef SampHold_members[] =
{
    {"server", T_OBJECT_EX, offsetof(SampHold, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(SampHold, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(SampHold, input), 0, NULL},
    {"controlsig", T_OBJECT_EX, offsetof(SampHold, controlsig), 0, NULL},
    {"value", T_OBJECT_EX, offsetof(SampHold, value), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(SampHold, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(SampHold, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef SampHold_methods[] =
{
    {"getServer", (PyCFunction)SampHold_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)SampHold_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)SampHold_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)SampHold_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)SampHold_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setValue", (PyCFunction)SampHold_setValue, METH_O, NULL},
    {"setMul", (PyCFunction)SampHold_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)SampHold_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)SampHold_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)SampHold_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SampHold_as_number =
{
    (binaryfunc)SampHold_add,                         /*nb_add*/
    (binaryfunc)SampHold_sub,                         /*nb_subtract*/
    (binaryfunc)SampHold_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)SampHold_inplace_add,                 /*inplace_add*/
    (binaryfunc)SampHold_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)SampHold_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)SampHold_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)SampHold_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject SampHoldType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SampHold_base",                                   /*tp_name*/
    sizeof(SampHold),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)SampHold_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &SampHold_as_number,                              /*tp_as_number*/
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
    (traverseproc)SampHold_traverse,                  /* tp_traverse */
    (inquiry)SampHold_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    SampHold_methods,                                 /* tp_methods */
    SampHold_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    SampHold_new,                                     /* tp_new */
};

/************/
/* TrackHold */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *controlsig;
    Stream *controlsig_stream;
    PyObject *value;
    Stream *value_stream;
    MYFLT currentValue;
    int flag;
    int modebuffer[3]; // need at least 2 slots for mul & add
} TrackHold;

static void
TrackHold_filters_i(TrackHold *self)
{
    MYFLT ctrl;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT val = PyFloat_AS_DOUBLE(self->value);

    for (i = 0; i < self->bufsize; i++)
    {
        ctrl = ctrlsig[i];

        if (ctrl > (val - 0.0001) && ctrl < (val + 0.0001))
        {
            if (self->flag == 1)
            {
                self->currentValue = in[i];
                self->flag = 0;
            }
        }
        else
        {
            self->currentValue = in[i];
            self->flag = 1;
        }

        self->data[i] = self->currentValue;
    }
}

static void
TrackHold_filters_a(TrackHold *self)
{
    MYFLT ctrl, val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT *valsig = Stream_getData((Stream *)self->value_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        ctrl = ctrlsig[i];
        val = valsig[i];

        if (ctrl > (val - 0.0001) && ctrl < (val + 0.0001))
        {
            if (self->flag == 1)
            {
                self->currentValue = in[i];
                self->flag = 0;
            }
        }
        else
        {
            self->currentValue = in[i];
            self->flag = 1;
        }

        self->data[i] = self->currentValue;
    }
}

static void TrackHold_postprocessing_ii(TrackHold *self) { POST_PROCESSING_II };
static void TrackHold_postprocessing_ai(TrackHold *self) { POST_PROCESSING_AI };
static void TrackHold_postprocessing_ia(TrackHold *self) { POST_PROCESSING_IA };
static void TrackHold_postprocessing_aa(TrackHold *self) { POST_PROCESSING_AA };
static void TrackHold_postprocessing_ireva(TrackHold *self) { POST_PROCESSING_IREVA };
static void TrackHold_postprocessing_areva(TrackHold *self) { POST_PROCESSING_AREVA };
static void TrackHold_postprocessing_revai(TrackHold *self) { POST_PROCESSING_REVAI };
static void TrackHold_postprocessing_revaa(TrackHold *self) { POST_PROCESSING_REVAA };
static void TrackHold_postprocessing_revareva(TrackHold *self) { POST_PROCESSING_REVAREVA };

static void
TrackHold_setProcMode(TrackHold *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = TrackHold_filters_i;
            break;

        case 1:
            self->proc_func_ptr = TrackHold_filters_a;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = TrackHold_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = TrackHold_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = TrackHold_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = TrackHold_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = TrackHold_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = TrackHold_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = TrackHold_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = TrackHold_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = TrackHold_postprocessing_revareva;
            break;
    }
}

static void
TrackHold_compute_next_data_frame(TrackHold *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrackHold_traverse(TrackHold *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->controlsig);
    Py_VISIT(self->value);
    return 0;
}

static int
TrackHold_clear(TrackHold *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->controlsig);
    Py_CLEAR(self->value);
    return 0;
}

static void
TrackHold_dealloc(TrackHold* self)
{
    pyo_DEALLOC
    TrackHold_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrackHold_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *controlsigtmp, *controlsig_streamtmp, *valuetmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrackHold *self;
    self = (TrackHold *)type->tp_alloc(type, 0);

    self->value = PyFloat_FromDouble(0.0);
    self->currentValue = 0.0;
    self->flag = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrackHold_compute_next_data_frame);
    self->mode_func_ptr = TrackHold_setProcMode;

    static char *kwlist[] = {"input", "controlsig", "value", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &controlsigtmp, &valuetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    self->controlsig = controlsigtmp;
    Py_INCREF(self->controlsig);
    controlsig_streamtmp = PyObject_CallMethod((PyObject *)self->controlsig, "_getStream", NULL);
    self->controlsig_stream = (Stream *)controlsig_streamtmp;
    Py_INCREF(self->controlsig_stream);

    if (valuetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
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

static PyObject * TrackHold_getServer(TrackHold* self) { GET_SERVER };
static PyObject * TrackHold_getStream(TrackHold* self) { GET_STREAM };
static PyObject * TrackHold_setMul(TrackHold *self, PyObject *arg) { SET_MUL };
static PyObject * TrackHold_setAdd(TrackHold *self, PyObject *arg) { SET_ADD };
static PyObject * TrackHold_setSub(TrackHold *self, PyObject *arg) { SET_SUB };
static PyObject * TrackHold_setDiv(TrackHold *self, PyObject *arg) { SET_DIV };

static PyObject * TrackHold_play(TrackHold *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrackHold_out(TrackHold *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrackHold_stop(TrackHold *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrackHold_multiply(TrackHold *self, PyObject *arg) { MULTIPLY };
static PyObject * TrackHold_inplace_multiply(TrackHold *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrackHold_add(TrackHold *self, PyObject *arg) { ADD };
static PyObject * TrackHold_inplace_add(TrackHold *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrackHold_sub(TrackHold *self, PyObject *arg) { SUB };
static PyObject * TrackHold_inplace_sub(TrackHold *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrackHold_div(TrackHold *self, PyObject *arg) { DIV };
static PyObject * TrackHold_inplace_div(TrackHold *self, PyObject *arg) { INPLACE_DIV };

static PyObject * TrackHold_setValue(TrackHold *self, PyObject *arg) { SET_PARAM(self->value, self->value_stream, 2); }

static PyMemberDef TrackHold_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrackHold, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TrackHold, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TrackHold, input), 0, NULL},
    {"controlsig", T_OBJECT_EX, offsetof(TrackHold, controlsig), 0, NULL},
    {"value", T_OBJECT_EX, offsetof(TrackHold, value), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(TrackHold, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(TrackHold, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrackHold_methods[] =
{
    {"getServer", (PyCFunction)TrackHold_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TrackHold_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)TrackHold_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)TrackHold_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TrackHold_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setValue", (PyCFunction)TrackHold_setValue, METH_O, NULL},
    {"setMul", (PyCFunction)TrackHold_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)TrackHold_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)TrackHold_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)TrackHold_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrackHold_as_number =
{
    (binaryfunc)TrackHold_add,                         /*nb_add*/
    (binaryfunc)TrackHold_sub,                         /*nb_subtract*/
    (binaryfunc)TrackHold_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)TrackHold_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrackHold_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrackHold_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)TrackHold_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)TrackHold_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TrackHoldType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TrackHold_base",                                   /*tp_name*/
    sizeof(TrackHold),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)TrackHold_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &TrackHold_as_number,                              /*tp_as_number*/
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
    (traverseproc)TrackHold_traverse,                  /* tp_traverse */
    (inquiry)TrackHold_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    TrackHold_methods,                                 /* tp_methods */
    TrackHold_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    TrackHold_new,                                     /* tp_new */
};

/************/
/* Compare */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *comp;
    Stream *comp_stream;
    MYFLT (*compare_func_ptr)(MYFLT, MYFLT); // true = 1.0, false = 0.0
    int modebuffer[3]; // need at least 2 slots for mul & add
} Compare;

static MYFLT
Compare_lt(MYFLT in, MYFLT comp)
{
    if (in < comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_elt(MYFLT in, MYFLT comp)
{
    if (in <= comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_gt(MYFLT in, MYFLT comp)
{
    if (in > comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_egt(MYFLT in, MYFLT comp)
{
    if (in >= comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_eq(MYFLT in, MYFLT comp)
{
    if (in >= (comp - 0.0001) && in <= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_neq(MYFLT in, MYFLT comp)
{
    if (in <= (comp - 0.0001) || in >= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static void
Compare_process_i(Compare *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT comp = PyFloat_AS_DOUBLE(self->comp);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (*self->compare_func_ptr)(in[i], comp);
    }
}

static void
Compare_process_a(Compare *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *comp = Stream_getData((Stream *)self->comp_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (*self->compare_func_ptr)(in[i], comp[i]);
    }
}

static void Compare_postprocessing_ii(Compare *self) { POST_PROCESSING_II };
static void Compare_postprocessing_ai(Compare *self) { POST_PROCESSING_AI };
static void Compare_postprocessing_ia(Compare *self) { POST_PROCESSING_IA };
static void Compare_postprocessing_aa(Compare *self) { POST_PROCESSING_AA };
static void Compare_postprocessing_ireva(Compare *self) { POST_PROCESSING_IREVA };
static void Compare_postprocessing_areva(Compare *self) { POST_PROCESSING_AREVA };
static void Compare_postprocessing_revai(Compare *self) { POST_PROCESSING_REVAI };
static void Compare_postprocessing_revaa(Compare *self) { POST_PROCESSING_REVAA };
static void Compare_postprocessing_revareva(Compare *self) { POST_PROCESSING_REVAREVA };

static void
Compare_setProcMode(Compare *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = Compare_process_i;
            break;

        case 1:
            self->proc_func_ptr = Compare_process_a;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Compare_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Compare_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Compare_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Compare_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Compare_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Compare_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Compare_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Compare_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Compare_postprocessing_revareva;
            break;
    }
}

static void
Compare_compute_next_data_frame(Compare *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Compare_traverse(Compare *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->comp);
    return 0;
}

static int
Compare_clear(Compare *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->comp);
    return 0;
}

static void
Compare_dealloc(Compare* self)
{
    pyo_DEALLOC
    Compare_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Compare_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *comptmp, *modetmp = NULL, *multmp = NULL, *addtmp = NULL;
    Compare *self;
    self = (Compare *)type->tp_alloc(type, 0);

    self->comp = PyFloat_FromDouble(0.0);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    self->compare_func_ptr = Compare_lt;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Compare_compute_next_data_frame);
    self->mode_func_ptr = Compare_setProcMode;

    static char *kwlist[] = {"input", "comp", "mode", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &comptmp, &modetmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (comptmp)
    {
        PyObject_CallMethod((PyObject *)self, "setComp", "O", comptmp);
    }

    if (modetmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMode", "O", modetmp);
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

static PyObject * Compare_getServer(Compare* self) { GET_SERVER };
static PyObject * Compare_getStream(Compare* self) { GET_STREAM };
static PyObject * Compare_setMul(Compare *self, PyObject *arg) { SET_MUL };
static PyObject * Compare_setAdd(Compare *self, PyObject *arg) { SET_ADD };
static PyObject * Compare_setSub(Compare *self, PyObject *arg) { SET_SUB };
static PyObject * Compare_setDiv(Compare *self, PyObject *arg) { SET_DIV };

static PyObject * Compare_play(Compare *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Compare_stop(Compare *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Compare_multiply(Compare *self, PyObject *arg) { MULTIPLY };
static PyObject * Compare_inplace_multiply(Compare *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Compare_add(Compare *self, PyObject *arg) { ADD };
static PyObject * Compare_inplace_add(Compare *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Compare_sub(Compare *self, PyObject *arg) { SUB };
static PyObject * Compare_inplace_sub(Compare *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Compare_div(Compare *self, PyObject *arg) { DIV };
static PyObject * Compare_inplace_div(Compare *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Compare_setComp(Compare *self, PyObject *arg) { SET_PARAM(self->comp, self->comp_stream, 2); }

static PyObject *
Compare_setMode(Compare *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (! PyLong_Check(arg))
    {
        PySys_WriteStdout("Compare: mode attribute should be a comparison operator as a string\n");
        Py_RETURN_NONE;
    }

    int tmp = PyLong_AsLong(arg);

    if (tmp == 0)
        self->compare_func_ptr = Compare_lt;
    else if (tmp == 1)
        self->compare_func_ptr = Compare_elt;
    else if (tmp == 2)
        self->compare_func_ptr = Compare_gt;
    else if (tmp == 3)
        self->compare_func_ptr = Compare_egt;
    else if (tmp == 4)
        self->compare_func_ptr = Compare_eq;
    else if (tmp == 5)
        self->compare_func_ptr = Compare_neq;

    Py_RETURN_NONE;
}

static PyMemberDef Compare_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Compare, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Compare, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Compare, input), 0, NULL},
    {"comp", T_OBJECT_EX, offsetof(Compare, comp), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Compare, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Compare, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Compare_methods[] =
{
    {"getServer", (PyCFunction)Compare_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Compare_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Compare_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Compare_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setComp", (PyCFunction)Compare_setComp, METH_O, NULL},
    {"setMode", (PyCFunction)Compare_setMode, METH_O, NULL},
    {"setMul", (PyCFunction)Compare_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Compare_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Compare_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Compare_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Compare_as_number =
{
    (binaryfunc)Compare_add,                         /*nb_add*/
    (binaryfunc)Compare_sub,                         /*nb_subtract*/
    (binaryfunc)Compare_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Compare_inplace_add,                 /*inplace_add*/
    (binaryfunc)Compare_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Compare_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Compare_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Compare_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject CompareType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Compare_base",                                   /*tp_name*/
    sizeof(Compare),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Compare_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Compare_as_number,                              /*tp_as_number*/
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
    (traverseproc)Compare_traverse,                  /* tp_traverse */
    (inquiry)Compare_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Compare_methods,                                 /* tp_methods */
    Compare_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Compare_new,                                     /* tp_new */
};

/*****************/
/** Between object **/
/*****************/

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    Stream *min_stream;
    PyObject *max;
    Stream *max_stream;
    int modebuffer[4];
} Between;

static void
Between_transform_ii(Between *self)
{
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i = 0; i < self->bufsize; i++)
    {
        val = in[i];

        if(val >= mi && val < ma)
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_ai(Between *self)
{
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i = 0; i < self->bufsize; i++)
    {
        val = in[i];

        if(val >= mi[i] && val < ma)
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_ia(Between *self)
{
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        val = in[i];

        if(val >= mi && val < ma[i])
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_aa(Between *self)
{
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        val = in[i];

        if(val >= mi[i] && val < ma[i])
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void Between_postprocessing_ii(Between *self) { POST_PROCESSING_II };
static void Between_postprocessing_ai(Between *self) { POST_PROCESSING_AI };
static void Between_postprocessing_ia(Between *self) { POST_PROCESSING_IA };
static void Between_postprocessing_aa(Between *self) { POST_PROCESSING_AA };
static void Between_postprocessing_ireva(Between *self) { POST_PROCESSING_IREVA };
static void Between_postprocessing_areva(Between *self) { POST_PROCESSING_AREVA };
static void Between_postprocessing_revai(Between *self) { POST_PROCESSING_REVAI };
static void Between_postprocessing_revaa(Between *self) { POST_PROCESSING_REVAA };
static void Between_postprocessing_revareva(Between *self) { POST_PROCESSING_REVAREVA };

static void
Between_setProcMode(Between *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = Between_transform_ii;
            break;

        case 1:
            self->proc_func_ptr = Between_transform_ai;
            break;

        case 10:
            self->proc_func_ptr = Between_transform_ia;
            break;

        case 11:
            self->proc_func_ptr = Between_transform_aa;
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Between_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Between_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Between_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Between_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Between_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Between_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Between_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Between_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Between_postprocessing_revareva;
            break;
    }
}

static void
Between_compute_next_data_frame(Between *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Between_traverse(Between *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->min);
    Py_VISIT(self->max);
    return 0;
}

static int
Between_clear(Between *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->min);
    Py_CLEAR(self->max);
    return 0;
}

static void
Between_dealloc(Between* self)
{
    pyo_DEALLOC
    Between_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Between_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *mintmp = NULL, *maxtmp = NULL, *multmp = NULL, *addtmp = NULL;
    Between *self;
    self = (Between *)type->tp_alloc(type, 0);

    self->min = PyFloat_FromDouble(0.0);
    self->max = PyFloat_FromDouble(1.0);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Between_compute_next_data_frame);
    self->mode_func_ptr = Between_setProcMode;

    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (mintmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }

    if (maxtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
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

static PyObject * Between_getServer(Between* self) { GET_SERVER };
static PyObject * Between_getStream(Between* self) { GET_STREAM };
static PyObject * Between_setMul(Between *self, PyObject *arg) { SET_MUL };
static PyObject * Between_setAdd(Between *self, PyObject *arg) { SET_ADD };
static PyObject * Between_setSub(Between *self, PyObject *arg) { SET_SUB };
static PyObject * Between_setDiv(Between *self, PyObject *arg) { SET_DIV };

static PyObject * Between_play(Between *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Between_out(Between *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Between_stop(Between *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Between_multiply(Between *self, PyObject *arg) { MULTIPLY };
static PyObject * Between_inplace_multiply(Between *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Between_add(Between *self, PyObject *arg) { ADD };
static PyObject * Between_inplace_add(Between *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Between_sub(Between *self, PyObject *arg) { SUB };
static PyObject * Between_inplace_sub(Between *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Between_div(Between *self, PyObject *arg) { DIV };
static PyObject * Between_inplace_div(Between *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Between_setMin(Between *self, PyObject *arg) { SET_PARAM(self->min, self->min_stream, 2); }
static PyObject * Between_setMax(Between *self, PyObject *arg) { SET_PARAM(self->max, self->max_stream, 3); }

static PyMemberDef Between_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Between, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Between, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Between, input), 0, NULL},
    {"min", T_OBJECT_EX, offsetof(Between, min), 0, NULL},
    {"max", T_OBJECT_EX, offsetof(Between, max), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Between, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Between, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Between_methods[] =
{
    {"getServer", (PyCFunction)Between_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Between_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Between_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Between_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Between_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMin", (PyCFunction)Between_setMin, METH_O, NULL},
    {"setMax", (PyCFunction)Between_setMax, METH_O, NULL},
    {"setMul", (PyCFunction)Between_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Between_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Between_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Between_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Between_as_number =
{
    (binaryfunc)Between_add,                      /*nb_add*/
    (binaryfunc)Between_sub,                 /*nb_subtract*/
    (binaryfunc)Between_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Between_inplace_add,              /*inplace_add*/
    (binaryfunc)Between_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Between_inplace_multiply,         /*inplace_multiply*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Between_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Between_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject BetweenType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Between_base",         /*tp_name*/
    sizeof(Between),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Between_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Between_as_number,             /*tp_as_number*/
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
    (traverseproc)Between_traverse,   /* tp_traverse */
    (inquiry)Between_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Between_methods,             /* tp_methods */
    Between_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Between_new,                 /* tp_new */
};


/************/
/* Denorm */
/************/
#ifndef USE_DOUBLE
#define DENORM_RAND  ((MYFLT) ((pyorand()/((MYFLT)(PYO_RAND_MAX)*0.5+1) - 1.0) * (MYFLT)(1.0e-24)))
#else
#define DENORM_RAND  ((MYFLT) ((pyorand()/((MYFLT)(PYO_RAND_MAX)*0.5+1) - 1.0) * (MYFLT)(1.0e-60)))
#endif

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Denorm;

static void
Denorm_filters(Denorm *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = in[i] + DENORM_RAND;
    }
}

static void Denorm_postprocessing_ii(Denorm *self) { POST_PROCESSING_II };
static void Denorm_postprocessing_ai(Denorm *self) { POST_PROCESSING_AI };
static void Denorm_postprocessing_ia(Denorm *self) { POST_PROCESSING_IA };
static void Denorm_postprocessing_aa(Denorm *self) { POST_PROCESSING_AA };
static void Denorm_postprocessing_ireva(Denorm *self) { POST_PROCESSING_IREVA };
static void Denorm_postprocessing_areva(Denorm *self) { POST_PROCESSING_AREVA };
static void Denorm_postprocessing_revai(Denorm *self) { POST_PROCESSING_REVAI };
static void Denorm_postprocessing_revaa(Denorm *self) { POST_PROCESSING_REVAA };
static void Denorm_postprocessing_revareva(Denorm *self) { POST_PROCESSING_REVAREVA };

static void
Denorm_setProcMode(Denorm *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Denorm_filters;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Denorm_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Denorm_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Denorm_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Denorm_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Denorm_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Denorm_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Denorm_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Denorm_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Denorm_postprocessing_revareva;
            break;
    }
}

static void
Denorm_compute_next_data_frame(Denorm *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Denorm_traverse(Denorm *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Denorm_clear(Denorm *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Denorm_dealloc(Denorm* self)
{
    pyo_DEALLOC
    Denorm_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Denorm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Denorm *self;
    self = (Denorm *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Denorm_compute_next_data_frame);
    self->mode_func_ptr = Denorm_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

    Server_generateSeed((Server *)self->server, DENORM_ID);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Denorm_getServer(Denorm* self) { GET_SERVER };
static PyObject * Denorm_getStream(Denorm* self) { GET_STREAM };
static PyObject * Denorm_setMul(Denorm *self, PyObject *arg) { SET_MUL };
static PyObject * Denorm_setAdd(Denorm *self, PyObject *arg) { SET_ADD };
static PyObject * Denorm_setSub(Denorm *self, PyObject *arg) { SET_SUB };
static PyObject * Denorm_setDiv(Denorm *self, PyObject *arg) { SET_DIV };

static PyObject * Denorm_play(Denorm *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Denorm_out(Denorm *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Denorm_stop(Denorm *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Denorm_multiply(Denorm *self, PyObject *arg) { MULTIPLY };
static PyObject * Denorm_inplace_multiply(Denorm *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Denorm_add(Denorm *self, PyObject *arg) { ADD };
static PyObject * Denorm_inplace_add(Denorm *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Denorm_sub(Denorm *self, PyObject *arg) { SUB };
static PyObject * Denorm_inplace_sub(Denorm *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Denorm_div(Denorm *self, PyObject *arg) { DIV };
static PyObject * Denorm_inplace_div(Denorm *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Denorm_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Denorm, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Denorm, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Denorm, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Denorm, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Denorm, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Denorm_methods[] =
{
    {"getServer", (PyCFunction)Denorm_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Denorm_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Denorm_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Denorm_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Denorm_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)Denorm_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Denorm_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Denorm_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Denorm_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Denorm_as_number =
{
    (binaryfunc)Denorm_add,                         /*nb_add*/
    (binaryfunc)Denorm_sub,                         /*nb_subtract*/
    (binaryfunc)Denorm_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Denorm_inplace_add,                 /*inplace_add*/
    (binaryfunc)Denorm_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Denorm_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Denorm_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Denorm_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject DenormType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Denorm_base",                                   /*tp_name*/
    sizeof(Denorm),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Denorm_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Denorm_as_number,                              /*tp_as_number*/
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
    (traverseproc)Denorm_traverse,                  /* tp_traverse */
    (inquiry)Denorm_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Denorm_methods,                                 /* tp_methods */
    Denorm_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Denorm_new,                                     /* tp_new */
};

/************/
/* DBToA */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastdb;
    MYFLT currentamp;
    int modebuffer[2]; // need at least 2 slots for mul & add
} DBToA;

static void
DBToA_process(DBToA *self)
{
    int i;
    MYFLT db;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        db = in[i];

        if (db <= -120.0)
        {
            self->data[i] = self->currentamp = 0.0;
            self->lastdb = -120.0;
        }
        else if (db != self->lastdb)
        {
            self->data[i] = self->currentamp = MYPOW(10.0, db * 0.05);
            self->lastdb = db;
        }
        else
            self->data[i] = self->currentamp;
    }
}

static void DBToA_postprocessing_ii(DBToA *self) { POST_PROCESSING_II };
static void DBToA_postprocessing_ai(DBToA *self) { POST_PROCESSING_AI };
static void DBToA_postprocessing_ia(DBToA *self) { POST_PROCESSING_IA };
static void DBToA_postprocessing_aa(DBToA *self) { POST_PROCESSING_AA };
static void DBToA_postprocessing_ireva(DBToA *self) { POST_PROCESSING_IREVA };
static void DBToA_postprocessing_areva(DBToA *self) { POST_PROCESSING_AREVA };
static void DBToA_postprocessing_revai(DBToA *self) { POST_PROCESSING_REVAI };
static void DBToA_postprocessing_revaa(DBToA *self) { POST_PROCESSING_REVAA };
static void DBToA_postprocessing_revareva(DBToA *self) { POST_PROCESSING_REVAREVA };

static void
DBToA_setProcMode(DBToA *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = DBToA_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = DBToA_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = DBToA_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = DBToA_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = DBToA_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = DBToA_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = DBToA_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = DBToA_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = DBToA_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = DBToA_postprocessing_revareva;
            break;
    }
}

static void
DBToA_compute_next_data_frame(DBToA *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
DBToA_traverse(DBToA *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
DBToA_clear(DBToA *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
DBToA_dealloc(DBToA* self)
{
    pyo_DEALLOC
    DBToA_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
DBToA_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    DBToA *self;
    self = (DBToA *)type->tp_alloc(type, 0);

    self->lastdb = -120.0;
    self->currentamp = MYPOW(10.0, self->lastdb * 0.05);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, DBToA_compute_next_data_frame);
    self->mode_func_ptr = DBToA_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * DBToA_getServer(DBToA* self) { GET_SERVER };
static PyObject * DBToA_getStream(DBToA* self) { GET_STREAM };
static PyObject * DBToA_setMul(DBToA *self, PyObject *arg) { SET_MUL };
static PyObject * DBToA_setAdd(DBToA *self, PyObject *arg) { SET_ADD };
static PyObject * DBToA_setSub(DBToA *self, PyObject *arg) { SET_SUB };
static PyObject * DBToA_setDiv(DBToA *self, PyObject *arg) { SET_DIV };

static PyObject * DBToA_play(DBToA *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * DBToA_out(DBToA *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * DBToA_stop(DBToA *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * DBToA_multiply(DBToA *self, PyObject *arg) { MULTIPLY };
static PyObject * DBToA_inplace_multiply(DBToA *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * DBToA_add(DBToA *self, PyObject *arg) { ADD };
static PyObject * DBToA_inplace_add(DBToA *self, PyObject *arg) { INPLACE_ADD };
static PyObject * DBToA_sub(DBToA *self, PyObject *arg) { SUB };
static PyObject * DBToA_inplace_sub(DBToA *self, PyObject *arg) { INPLACE_SUB };
static PyObject * DBToA_div(DBToA *self, PyObject *arg) { DIV };
static PyObject * DBToA_inplace_div(DBToA *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef DBToA_members[] =
{
    {"server", T_OBJECT_EX, offsetof(DBToA, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(DBToA, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(DBToA, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(DBToA, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(DBToA, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef DBToA_methods[] =
{
    {"getServer", (PyCFunction)DBToA_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)DBToA_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)DBToA_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)DBToA_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)DBToA_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)DBToA_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)DBToA_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)DBToA_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)DBToA_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods DBToA_as_number =
{
    (binaryfunc)DBToA_add,                         /*nb_add*/
    (binaryfunc)DBToA_sub,                         /*nb_subtract*/
    (binaryfunc)DBToA_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)DBToA_inplace_add,                 /*inplace_add*/
    (binaryfunc)DBToA_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)DBToA_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)DBToA_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)DBToA_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject DBToAType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.DBToA_base",                                   /*tp_name*/
    sizeof(DBToA),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)DBToA_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &DBToA_as_number,                              /*tp_as_number*/
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
    (traverseproc)DBToA_traverse,                  /* tp_traverse */
    (inquiry)DBToA_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    DBToA_methods,                                 /* tp_methods */
    DBToA_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    DBToA_new,                                     /* tp_new */
};

/************/
/* AToDB */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastamp;
    MYFLT currentdb;
    int modebuffer[2]; // need at least 2 slots for mul & add
} AToDB;

static void
AToDB_process(AToDB *self)
{
    int i;
    MYFLT amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        amp = in[i];

        if (amp <= 0.000001)
        {
            self->data[i] = self->currentdb = -120.0;
            self->lastamp = 0.000001;
        }
        else if (amp != self->lastamp)
        {
            self->data[i] = self->currentdb = 20.0 * MYLOG10(amp);
            self->lastamp = amp;
        }
        else
            self->data[i] = self->currentdb;
    }
}

static void AToDB_postprocessing_ii(AToDB *self) { POST_PROCESSING_II };
static void AToDB_postprocessing_ai(AToDB *self) { POST_PROCESSING_AI };
static void AToDB_postprocessing_ia(AToDB *self) { POST_PROCESSING_IA };
static void AToDB_postprocessing_aa(AToDB *self) { POST_PROCESSING_AA };
static void AToDB_postprocessing_ireva(AToDB *self) { POST_PROCESSING_IREVA };
static void AToDB_postprocessing_areva(AToDB *self) { POST_PROCESSING_AREVA };
static void AToDB_postprocessing_revai(AToDB *self) { POST_PROCESSING_REVAI };
static void AToDB_postprocessing_revaa(AToDB *self) { POST_PROCESSING_REVAA };
static void AToDB_postprocessing_revareva(AToDB *self) { POST_PROCESSING_REVAREVA };

static void
AToDB_setProcMode(AToDB *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = AToDB_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = AToDB_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = AToDB_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = AToDB_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = AToDB_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = AToDB_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = AToDB_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = AToDB_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = AToDB_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = AToDB_postprocessing_revareva;
            break;
    }
}

static void
AToDB_compute_next_data_frame(AToDB *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
AToDB_traverse(AToDB *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
AToDB_clear(AToDB *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
AToDB_dealloc(AToDB* self)
{
    pyo_DEALLOC
    AToDB_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
AToDB_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    AToDB *self;
    self = (AToDB *)type->tp_alloc(type, 0);

    self->lastamp = 0.000001;
    self->currentdb = 20.0 * MYLOG10(self->lastamp);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, AToDB_compute_next_data_frame);
    self->mode_func_ptr = AToDB_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * AToDB_getServer(AToDB* self) { GET_SERVER };
static PyObject * AToDB_getStream(AToDB* self) { GET_STREAM };
static PyObject * AToDB_setMul(AToDB *self, PyObject *arg) { SET_MUL };
static PyObject * AToDB_setAdd(AToDB *self, PyObject *arg) { SET_ADD };
static PyObject * AToDB_setSub(AToDB *self, PyObject *arg) { SET_SUB };
static PyObject * AToDB_setDiv(AToDB *self, PyObject *arg) { SET_DIV };

static PyObject * AToDB_play(AToDB *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * AToDB_out(AToDB *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * AToDB_stop(AToDB *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * AToDB_multiply(AToDB *self, PyObject *arg) { MULTIPLY };
static PyObject * AToDB_inplace_multiply(AToDB *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * AToDB_add(AToDB *self, PyObject *arg) { ADD };
static PyObject * AToDB_inplace_add(AToDB *self, PyObject *arg) { INPLACE_ADD };
static PyObject * AToDB_sub(AToDB *self, PyObject *arg) { SUB };
static PyObject * AToDB_inplace_sub(AToDB *self, PyObject *arg) { INPLACE_SUB };
static PyObject * AToDB_div(AToDB *self, PyObject *arg) { DIV };
static PyObject * AToDB_inplace_div(AToDB *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef AToDB_members[] =
{
    {"server", T_OBJECT_EX, offsetof(AToDB, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(AToDB, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(AToDB, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(AToDB, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(AToDB, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef AToDB_methods[] =
{
    {"getServer", (PyCFunction)AToDB_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)AToDB_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)AToDB_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)AToDB_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)AToDB_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)AToDB_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)AToDB_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)AToDB_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)AToDB_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods AToDB_as_number =
{
    (binaryfunc)AToDB_add,                         /*nb_add*/
    (binaryfunc)AToDB_sub,                         /*nb_subtract*/
    (binaryfunc)AToDB_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)AToDB_inplace_add,                 /*inplace_add*/
    (binaryfunc)AToDB_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)AToDB_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)AToDB_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)AToDB_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject AToDBType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.AToDB_base",                                   /*tp_name*/
    sizeof(AToDB),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)AToDB_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &AToDB_as_number,                              /*tp_as_number*/
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
    (traverseproc)AToDB_traverse,                  /* tp_traverse */
    (inquiry)AToDB_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    AToDB_methods,                                 /* tp_methods */
    AToDB_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    AToDB_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* Scale ********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *inmin;
    Stream *inmin_stream;
    PyObject *inmax;
    Stream *inmax_stream;
    PyObject *outmin;
    Stream *outmin_stream;
    PyObject *outmax;
    Stream *outmax_stream;
    PyObject *exp;
    Stream *exp_stream;
    int modebuffer[7]; // need at least 2 slots for mul & add
} Scale;

static MYFLT
_scale_clip(MYFLT x, MYFLT min, MYFLT max)
{
    if (x < min)
        return min;
    else if (x > max)
        return max;
    else
        return x;
}

static void
Scale_generate(Scale *self)
{
    int i, inrev, outrev;
    MYFLT tmp, inrange, outrange, normin;
    MYFLT inmin, inmax, outmin, outmax, exp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        inmin = PyFloat_AS_DOUBLE(self->inmin);
    else
        inmin = Stream_getData((Stream *)self->inmin_stream)[0];

    if (self->modebuffer[3] == 0)
        inmax = PyFloat_AS_DOUBLE(self->inmax);
    else
        inmax = Stream_getData((Stream *)self->inmax_stream)[0];

    if (inmin < inmax)
    {
        inrev = 0;
    }
    else
    {
        tmp = inmin;
        inmin = inmax;
        inmax = tmp;
        inrev = 1;
    }

    inrange = inmax - inmin;

    if (self->modebuffer[4] == 0)
        outmin = PyFloat_AS_DOUBLE(self->outmin);
    else
        outmin = Stream_getData((Stream *)self->outmin_stream)[0];

    if (self->modebuffer[5] == 0)
        outmax = PyFloat_AS_DOUBLE(self->outmax);
    else
        outmax = Stream_getData((Stream *)self->outmax_stream)[0];

    if (outmin < outmax)
    {
        outrev = 0;
    }
    else
    {
        tmp = outmin;
        outmin = outmax;
        outmax = tmp;
        outrev = 1;
    }

    outrange = outmax - outmin;

    if (self->modebuffer[6] == 0)
        exp = PyFloat_AS_DOUBLE(self->exp);
    else
        exp = Stream_getData((Stream *)self->exp_stream)[0];

    if (exp < 0.0)
        exp = 0.0;

    /* Handle case where input or output range equal 0 */
    if (inrange == 0.0 || outrange == 0.0)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = outmin;
        }
    }
    /* Linear scaling */
    else if (exp == 1.0)
    {
        if (inrev == 0 && outrev == 0)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = (_scale_clip(in[i], inmin, inmax) - inmin) / inrange;
                self->data[i] = normin * outrange + outmin;
            }
        }
        else if (inrev == 1 && outrev == 0)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = 1.0 - ((_scale_clip(in[i], inmin, inmax) - inmin) / inrange);
                self->data[i] = normin * outrange + outmin;
            }
        }
        else if (inrev == 0 && outrev == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = (_scale_clip(in[i], inmin, inmax) - inmin) / inrange;
                self->data[i] = outmax - (normin * outrange);
            }
        }
        else if (inrev == 1 && outrev == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = 1.0 - ((_scale_clip(in[i], inmin, inmax) - inmin) / inrange);
                self->data[i] = outmax - (normin * outrange);
            }
        }
    }
    /* Exponential scaling */
    else
    {
        if (inrev == 0 && outrev == 0)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = MYPOW((_scale_clip(in[i], inmin, inmax) - inmin) / inrange, exp);
                self->data[i] = normin * outrange + outmin;
            }
        }
        else if (inrev == 1 && outrev == 0)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = MYPOW(1.0 - ((_scale_clip(in[i], inmin, inmax) - inmin) / inrange), exp);
                self->data[i] = normin * outrange + outmin;
            }
        }
        else if (inrev == 0 && outrev == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = MYPOW((_scale_clip(in[i], inmin, inmax) - inmin) / inrange, exp);
                self->data[i] = outmax - (normin * outrange);
            }
        }
        else if (inrev == 1 && outrev == 1)
        {
            for (i = 0; i < self->bufsize; i++)
            {
                normin = MYPOW(1.0 - ((_scale_clip(in[i], inmin, inmax) - inmin) / inrange), exp);
                self->data[i] = outmax - (normin * outrange);
            }
        }
    }
}

static void Scale_postprocessing_ii(Scale *self) { POST_PROCESSING_II };
static void Scale_postprocessing_ai(Scale *self) { POST_PROCESSING_AI };
static void Scale_postprocessing_ia(Scale *self) { POST_PROCESSING_IA };
static void Scale_postprocessing_aa(Scale *self) { POST_PROCESSING_AA };
static void Scale_postprocessing_ireva(Scale *self) { POST_PROCESSING_IREVA };
static void Scale_postprocessing_areva(Scale *self) { POST_PROCESSING_AREVA };
static void Scale_postprocessing_revai(Scale *self) { POST_PROCESSING_REVAI };
static void Scale_postprocessing_revaa(Scale *self) { POST_PROCESSING_REVAA };
static void Scale_postprocessing_revareva(Scale *self) { POST_PROCESSING_REVAREVA };

static void
Scale_setProcMode(Scale *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Scale_generate;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Scale_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Scale_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Scale_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Scale_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Scale_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Scale_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Scale_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Scale_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Scale_postprocessing_revareva;
            break;
    }
}

static void
Scale_compute_next_data_frame(Scale *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Scale_traverse(Scale *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->inmin);
    Py_VISIT(self->inmax);
    Py_VISIT(self->outmin);
    Py_VISIT(self->outmax);
    Py_VISIT(self->exp);
    return 0;
}

static int
Scale_clear(Scale *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->inmin);
    Py_CLEAR(self->inmax);
    Py_CLEAR(self->outmin);
    Py_CLEAR(self->outmax);
    Py_CLEAR(self->exp);
    return 0;
}

static void
Scale_dealloc(Scale* self)
{
    pyo_DEALLOC
    Scale_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Scale_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *inmintmp = NULL, *inmaxtmp = NULL, *outmintmp = NULL, *outmaxtmp = NULL, *exptmp = NULL, *multmp = NULL, *addtmp = NULL;
    Scale *self;
    self = (Scale *)type->tp_alloc(type, 0);

    self->inmin = PyFloat_FromDouble(0.0);
    self->inmax = PyFloat_FromDouble(1.0);
    self->outmin = PyFloat_FromDouble(0.0);
    self->outmax = PyFloat_FromDouble(1.0);
    self->exp = PyFloat_FromDouble(1.0);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;
    self->modebuffer[4] = 0;
    self->modebuffer[5] = 0;
    self->modebuffer[6] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Scale_compute_next_data_frame);
    self->mode_func_ptr = Scale_setProcMode;

    static char *kwlist[] = {"input", "inmin", "inmax", "outmin", "outmax", "exp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOOO", kwlist, &inputtmp, &inmintmp, &inmaxtmp, &outmintmp, &outmaxtmp, &exptmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (inmintmp)
    {
        PyObject_CallMethod((PyObject *)self, "setInMin", "O", inmintmp);
    }

    if (inmaxtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setInMax", "O", inmaxtmp);
    }

    if (outmintmp)
    {
        PyObject_CallMethod((PyObject *)self, "setOutMin", "O", outmintmp);
    }

    if (outmaxtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setOutMax", "O", outmaxtmp);
    }

    if (exptmp)
    {
        PyObject_CallMethod((PyObject *)self, "setExp", "O", exptmp);
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

static PyObject * Scale_getServer(Scale* self) { GET_SERVER };
static PyObject * Scale_getStream(Scale* self) { GET_STREAM };
static PyObject * Scale_setMul(Scale *self, PyObject *arg) { SET_MUL };
static PyObject * Scale_setAdd(Scale *self, PyObject *arg) { SET_ADD };
static PyObject * Scale_setSub(Scale *self, PyObject *arg) { SET_SUB };
static PyObject * Scale_setDiv(Scale *self, PyObject *arg) { SET_DIV };

static PyObject * Scale_play(Scale *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Scale_out(Scale *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Scale_stop(Scale *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Scale_multiply(Scale *self, PyObject *arg) { MULTIPLY };
static PyObject * Scale_inplace_multiply(Scale *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Scale_add(Scale *self, PyObject *arg) { ADD };
static PyObject * Scale_inplace_add(Scale *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Scale_sub(Scale *self, PyObject *arg) { SUB };
static PyObject * Scale_inplace_sub(Scale *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Scale_div(Scale *self, PyObject *arg) { DIV };
static PyObject * Scale_inplace_div(Scale *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Scale_setInMin(Scale *self, PyObject *arg) { SET_PARAM(self->inmin, self->inmin_stream, 2); }
static PyObject * Scale_setInMax(Scale *self, PyObject *arg) { SET_PARAM(self->inmax, self->inmax_stream, 3); }
static PyObject * Scale_setOutMin(Scale *self, PyObject *arg) { SET_PARAM(self->outmin, self->outmin_stream, 4); }
static PyObject * Scale_setOutMax(Scale *self, PyObject *arg) { SET_PARAM(self->outmax, self->outmax_stream, 5); }
static PyObject * Scale_setExp(Scale *self, PyObject *arg) { SET_PARAM(self->exp, self->exp_stream, 6); }

static PyMemberDef Scale_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Scale, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Scale, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Scale, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Scale, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Scale, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Scale_methods[] =
{
    {"getServer", (PyCFunction)Scale_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Scale_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Scale_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Scale_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Scale_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setInMin", (PyCFunction)Scale_setInMin, METH_O, NULL},
    {"setInMax", (PyCFunction)Scale_setInMax, METH_O, NULL},
    {"setOutMin", (PyCFunction)Scale_setOutMin, METH_O, NULL},
    {"setOutMax", (PyCFunction)Scale_setOutMax, METH_O, NULL},
    {"setExp", (PyCFunction)Scale_setExp, METH_O, NULL},
    {"setMul", (PyCFunction)Scale_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Scale_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Scale_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Scale_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Scale_as_number =
{
    (binaryfunc)Scale_add,                         /*nb_add*/
    (binaryfunc)Scale_sub,                         /*nb_subtract*/
    (binaryfunc)Scale_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Scale_inplace_add,                 /*inplace_add*/
    (binaryfunc)Scale_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Scale_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Scale_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Scale_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ScaleType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Scale_base",                                   /*tp_name*/
    sizeof(Scale),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Scale_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Scale_as_number,                              /*tp_as_number*/
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
    (traverseproc)Scale_traverse,                  /* tp_traverse */
    (inquiry)Scale_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Scale_methods,                                 /* tp_methods */
    Scale_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Scale_new,                                     /* tp_new */
};

/************/
/* CentsToTranspo */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastcents;
    MYFLT curtranspo;
    int modebuffer[2]; // need at least 2 slots for mul & add
} CentsToTranspo;

static void
CentsToTranspo_process(CentsToTranspo *self)
{
    int i;
    MYFLT cents;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        cents = in[i];

        if (cents != self->lastcents)
        {
            self->data[i] = self->curtranspo = MYPOW(2.0, cents / 1200.);
            self->lastcents = cents;
        }
        else
            self->data[i] = self->curtranspo;
    }
}

static void CentsToTranspo_postprocessing_ii(CentsToTranspo *self) { POST_PROCESSING_II };
static void CentsToTranspo_postprocessing_ai(CentsToTranspo *self) { POST_PROCESSING_AI };
static void CentsToTranspo_postprocessing_ia(CentsToTranspo *self) { POST_PROCESSING_IA };
static void CentsToTranspo_postprocessing_aa(CentsToTranspo *self) { POST_PROCESSING_AA };
static void CentsToTranspo_postprocessing_ireva(CentsToTranspo *self) { POST_PROCESSING_IREVA };
static void CentsToTranspo_postprocessing_areva(CentsToTranspo *self) { POST_PROCESSING_AREVA };
static void CentsToTranspo_postprocessing_revai(CentsToTranspo *self) { POST_PROCESSING_REVAI };
static void CentsToTranspo_postprocessing_revaa(CentsToTranspo *self) { POST_PROCESSING_REVAA };
static void CentsToTranspo_postprocessing_revareva(CentsToTranspo *self) { POST_PROCESSING_REVAREVA };

static void
CentsToTranspo_setProcMode(CentsToTranspo *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = CentsToTranspo_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = CentsToTranspo_postprocessing_revareva;
            break;
    }
}

static void
CentsToTranspo_compute_next_data_frame(CentsToTranspo *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
CentsToTranspo_traverse(CentsToTranspo *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
CentsToTranspo_clear(CentsToTranspo *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
CentsToTranspo_dealloc(CentsToTranspo* self)
{
    pyo_DEALLOC
    CentsToTranspo_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CentsToTranspo_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    CentsToTranspo *self;
    self = (CentsToTranspo *)type->tp_alloc(type, 0);

    self->lastcents = 0.0;
    self->curtranspo = 1.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CentsToTranspo_compute_next_data_frame);
    self->mode_func_ptr = CentsToTranspo_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * CentsToTranspo_getServer(CentsToTranspo* self) { GET_SERVER };
static PyObject * CentsToTranspo_getStream(CentsToTranspo* self) { GET_STREAM };
static PyObject * CentsToTranspo_setMul(CentsToTranspo *self, PyObject *arg) { SET_MUL };
static PyObject * CentsToTranspo_setAdd(CentsToTranspo *self, PyObject *arg) { SET_ADD };
static PyObject * CentsToTranspo_setSub(CentsToTranspo *self, PyObject *arg) { SET_SUB };
static PyObject * CentsToTranspo_setDiv(CentsToTranspo *self, PyObject *arg) { SET_DIV };

static PyObject * CentsToTranspo_play(CentsToTranspo *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CentsToTranspo_out(CentsToTranspo *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * CentsToTranspo_stop(CentsToTranspo *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * CentsToTranspo_multiply(CentsToTranspo *self, PyObject *arg) { MULTIPLY };
static PyObject * CentsToTranspo_inplace_multiply(CentsToTranspo *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * CentsToTranspo_add(CentsToTranspo *self, PyObject *arg) { ADD };
static PyObject * CentsToTranspo_inplace_add(CentsToTranspo *self, PyObject *arg) { INPLACE_ADD };
static PyObject * CentsToTranspo_sub(CentsToTranspo *self, PyObject *arg) { SUB };
static PyObject * CentsToTranspo_inplace_sub(CentsToTranspo *self, PyObject *arg) { INPLACE_SUB };
static PyObject * CentsToTranspo_div(CentsToTranspo *self, PyObject *arg) { DIV };
static PyObject * CentsToTranspo_inplace_div(CentsToTranspo *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef CentsToTranspo_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CentsToTranspo, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(CentsToTranspo, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(CentsToTranspo, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(CentsToTranspo, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(CentsToTranspo, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef CentsToTranspo_methods[] =
{
    {"getServer", (PyCFunction)CentsToTranspo_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)CentsToTranspo_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)CentsToTranspo_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)CentsToTranspo_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)CentsToTranspo_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)CentsToTranspo_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)CentsToTranspo_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)CentsToTranspo_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)CentsToTranspo_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods CentsToTranspo_as_number =
{
    (binaryfunc)CentsToTranspo_add,                         /*nb_add*/
    (binaryfunc)CentsToTranspo_sub,                         /*nb_subtract*/
    (binaryfunc)CentsToTranspo_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)CentsToTranspo_inplace_add,                 /*inplace_add*/
    (binaryfunc)CentsToTranspo_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)CentsToTranspo_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)CentsToTranspo_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)CentsToTranspo_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject CentsToTranspoType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CentsToTranspo_base",                                   /*tp_name*/
    sizeof(CentsToTranspo),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)CentsToTranspo_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &CentsToTranspo_as_number,                              /*tp_as_number*/
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
    (traverseproc)CentsToTranspo_traverse,                  /* tp_traverse */
    (inquiry)CentsToTranspo_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    CentsToTranspo_methods,                                 /* tp_methods */
    CentsToTranspo_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    CentsToTranspo_new,                                     /* tp_new */
};

/************/
/* TranspoToCents */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lasttranspo;
    MYFLT curcents;
    int modebuffer[2]; // need at least 2 slots for mul & add
} TranspoToCents;

static void
TranspoToCents_process(TranspoToCents *self)
{
    int i;
    MYFLT transpo;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        transpo = in[i];

        if (transpo != self->lasttranspo)
        {
            self->data[i] = self->curcents = 1200.0 * MYLOG2(transpo);
            self->lasttranspo = transpo;
        }
        else
            self->data[i] = self->curcents;
    }
}

static void TranspoToCents_postprocessing_ii(TranspoToCents *self) { POST_PROCESSING_II };
static void TranspoToCents_postprocessing_ai(TranspoToCents *self) { POST_PROCESSING_AI };
static void TranspoToCents_postprocessing_ia(TranspoToCents *self) { POST_PROCESSING_IA };
static void TranspoToCents_postprocessing_aa(TranspoToCents *self) { POST_PROCESSING_AA };
static void TranspoToCents_postprocessing_ireva(TranspoToCents *self) { POST_PROCESSING_IREVA };
static void TranspoToCents_postprocessing_areva(TranspoToCents *self) { POST_PROCESSING_AREVA };
static void TranspoToCents_postprocessing_revai(TranspoToCents *self) { POST_PROCESSING_REVAI };
static void TranspoToCents_postprocessing_revaa(TranspoToCents *self) { POST_PROCESSING_REVAA };
static void TranspoToCents_postprocessing_revareva(TranspoToCents *self) { POST_PROCESSING_REVAREVA };

static void
TranspoToCents_setProcMode(TranspoToCents *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = TranspoToCents_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = TranspoToCents_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = TranspoToCents_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = TranspoToCents_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = TranspoToCents_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = TranspoToCents_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = TranspoToCents_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = TranspoToCents_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = TranspoToCents_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = TranspoToCents_postprocessing_revareva;
            break;
    }
}

static void
TranspoToCents_compute_next_data_frame(TranspoToCents *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TranspoToCents_traverse(TranspoToCents *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
TranspoToCents_clear(TranspoToCents *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
TranspoToCents_dealloc(TranspoToCents* self)
{
    pyo_DEALLOC
    TranspoToCents_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TranspoToCents_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    TranspoToCents *self;
    self = (TranspoToCents *)type->tp_alloc(type, 0);

    self->lasttranspo = 1.0;
    self->curcents = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TranspoToCents_compute_next_data_frame);
    self->mode_func_ptr = TranspoToCents_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * TranspoToCents_getServer(TranspoToCents* self) { GET_SERVER };
static PyObject * TranspoToCents_getStream(TranspoToCents* self) { GET_STREAM };
static PyObject * TranspoToCents_setMul(TranspoToCents *self, PyObject *arg) { SET_MUL };
static PyObject * TranspoToCents_setAdd(TranspoToCents *self, PyObject *arg) { SET_ADD };
static PyObject * TranspoToCents_setSub(TranspoToCents *self, PyObject *arg) { SET_SUB };
static PyObject * TranspoToCents_setDiv(TranspoToCents *self, PyObject *arg) { SET_DIV };

static PyObject * TranspoToCents_play(TranspoToCents *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TranspoToCents_out(TranspoToCents *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TranspoToCents_stop(TranspoToCents *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TranspoToCents_multiply(TranspoToCents *self, PyObject *arg) { MULTIPLY };
static PyObject * TranspoToCents_inplace_multiply(TranspoToCents *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TranspoToCents_add(TranspoToCents *self, PyObject *arg) { ADD };
static PyObject * TranspoToCents_inplace_add(TranspoToCents *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TranspoToCents_sub(TranspoToCents *self, PyObject *arg) { SUB };
static PyObject * TranspoToCents_inplace_sub(TranspoToCents *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TranspoToCents_div(TranspoToCents *self, PyObject *arg) { DIV };
static PyObject * TranspoToCents_inplace_div(TranspoToCents *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TranspoToCents_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TranspoToCents, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TranspoToCents, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TranspoToCents, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(TranspoToCents, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(TranspoToCents, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TranspoToCents_methods[] =
{
    {"getServer", (PyCFunction)TranspoToCents_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TranspoToCents_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)TranspoToCents_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TranspoToCents_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)TranspoToCents_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)TranspoToCents_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)TranspoToCents_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)TranspoToCents_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)TranspoToCents_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TranspoToCents_as_number =
{
    (binaryfunc)TranspoToCents_add,                         /*nb_add*/
    (binaryfunc)TranspoToCents_sub,                         /*nb_subtract*/
    (binaryfunc)TranspoToCents_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)TranspoToCents_inplace_add,                 /*inplace_add*/
    (binaryfunc)TranspoToCents_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TranspoToCents_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)TranspoToCents_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)TranspoToCents_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TranspoToCentsType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TranspoToCents_base",                                   /*tp_name*/
    sizeof(TranspoToCents),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)TranspoToCents_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &TranspoToCents_as_number,                              /*tp_as_number*/
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
    (traverseproc)TranspoToCents_traverse,                  /* tp_traverse */
    (inquiry)TranspoToCents_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    TranspoToCents_methods,                                 /* tp_methods */
    TranspoToCents_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    TranspoToCents_new,                                     /* tp_new */
};

/************/
/* MToF */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastmidi;
    MYFLT curfreq;
    int modebuffer[2]; // need at least 2 slots for mul & add
} MToF;

static void
MToF_process(MToF *self)
{
    int i;
    MYFLT midi;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        midi = in[i];

        if (midi != self->lastmidi)
        {
            self->data[i] = self->curfreq = 440.0 * MYPOW(2.0, (midi - 69) / 12.0);
            self->lastmidi = midi;
        }
        else
            self->data[i] = self->curfreq;
    }
}

static void MToF_postprocessing_ii(MToF *self) { POST_PROCESSING_II };
static void MToF_postprocessing_ai(MToF *self) { POST_PROCESSING_AI };
static void MToF_postprocessing_ia(MToF *self) { POST_PROCESSING_IA };
static void MToF_postprocessing_aa(MToF *self) { POST_PROCESSING_AA };
static void MToF_postprocessing_ireva(MToF *self) { POST_PROCESSING_IREVA };
static void MToF_postprocessing_areva(MToF *self) { POST_PROCESSING_AREVA };
static void MToF_postprocessing_revai(MToF *self) { POST_PROCESSING_REVAI };
static void MToF_postprocessing_revaa(MToF *self) { POST_PROCESSING_REVAA };
static void MToF_postprocessing_revareva(MToF *self) { POST_PROCESSING_REVAREVA };

static void
MToF_setProcMode(MToF *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = MToF_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = MToF_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = MToF_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = MToF_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = MToF_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = MToF_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = MToF_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = MToF_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = MToF_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = MToF_postprocessing_revareva;
            break;
    }
}

static void
MToF_compute_next_data_frame(MToF *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
MToF_traverse(MToF *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
MToF_clear(MToF *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
MToF_dealloc(MToF* self)
{
    pyo_DEALLOC
    MToF_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MToF_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    MToF *self;
    self = (MToF *)type->tp_alloc(type, 0);

    self->lastmidi = 0;
    self->curfreq = 8.1757989156437;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MToF_compute_next_data_frame);
    self->mode_func_ptr = MToF_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * MToF_getServer(MToF* self) { GET_SERVER };
static PyObject * MToF_getStream(MToF* self) { GET_STREAM };
static PyObject * MToF_setMul(MToF *self, PyObject *arg) { SET_MUL };
static PyObject * MToF_setAdd(MToF *self, PyObject *arg) { SET_ADD };
static PyObject * MToF_setSub(MToF *self, PyObject *arg) { SET_SUB };
static PyObject * MToF_setDiv(MToF *self, PyObject *arg) { SET_DIV };

static PyObject * MToF_play(MToF *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MToF_out(MToF *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MToF_stop(MToF *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MToF_multiply(MToF *self, PyObject *arg) { MULTIPLY };
static PyObject * MToF_inplace_multiply(MToF *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MToF_add(MToF *self, PyObject *arg) { ADD };
static PyObject * MToF_inplace_add(MToF *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MToF_sub(MToF *self, PyObject *arg) { SUB };
static PyObject * MToF_inplace_sub(MToF *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MToF_div(MToF *self, PyObject *arg) { DIV };
static PyObject * MToF_inplace_div(MToF *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MToF_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MToF, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(MToF, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(MToF, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(MToF, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(MToF, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef MToF_methods[] =
{
    {"getServer", (PyCFunction)MToF_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)MToF_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)MToF_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)MToF_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)MToF_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)MToF_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)MToF_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)MToF_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)MToF_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MToF_as_number =
{
    (binaryfunc)MToF_add,                         /*nb_add*/
    (binaryfunc)MToF_sub,                         /*nb_subtract*/
    (binaryfunc)MToF_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)MToF_inplace_add,                 /*inplace_add*/
    (binaryfunc)MToF_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MToF_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MToF_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MToF_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MToFType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MToF_base",                                   /*tp_name*/
    sizeof(MToF),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)MToF_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &MToF_as_number,                              /*tp_as_number*/
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
    (traverseproc)MToF_traverse,                  /* tp_traverse */
    (inquiry)MToF_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    MToF_methods,                                 /* tp_methods */
    MToF_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    MToF_new,                                     /* tp_new */
};

/************/
/* FToM */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastfreq;
    MYFLT curmidi;
    int modebuffer[2]; // need at least 2 slots for mul & add
} FToM;

static void
FToM_process(FToM *self)
{
    int i;
    MYFLT freq;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        freq = in[i];

        if (freq != self->lastfreq)
        {
            if (freq < 8.1758)
                freq = 8.1578;

            self->curmidi = 12.0 * MYLOG2(freq / 440.0) + 69;
            self->lastfreq = freq;
        }
        else
            self->data[i] = self->curmidi;
    }
}

static void FToM_postprocessing_ii(FToM *self) { POST_PROCESSING_II };
static void FToM_postprocessing_ai(FToM *self) { POST_PROCESSING_AI };
static void FToM_postprocessing_ia(FToM *self) { POST_PROCESSING_IA };
static void FToM_postprocessing_aa(FToM *self) { POST_PROCESSING_AA };
static void FToM_postprocessing_ireva(FToM *self) { POST_PROCESSING_IREVA };
static void FToM_postprocessing_areva(FToM *self) { POST_PROCESSING_AREVA };
static void FToM_postprocessing_revai(FToM *self) { POST_PROCESSING_REVAI };
static void FToM_postprocessing_revaa(FToM *self) { POST_PROCESSING_REVAA };
static void FToM_postprocessing_revareva(FToM *self) { POST_PROCESSING_REVAREVA };

static void
FToM_setProcMode(FToM *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = FToM_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = FToM_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = FToM_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = FToM_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = FToM_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = FToM_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = FToM_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = FToM_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = FToM_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = FToM_postprocessing_revareva;
            break;
    }
}

static void
FToM_compute_next_data_frame(FToM *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
FToM_traverse(FToM *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
FToM_clear(FToM *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
FToM_dealloc(FToM* self)
{
    pyo_DEALLOC
    FToM_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
FToM_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    FToM *self;
    self = (FToM *)type->tp_alloc(type, 0);

    self->lastfreq = 8.1758;
    self->curmidi = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FToM_compute_next_data_frame);
    self->mode_func_ptr = FToM_setProcMode;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject * FToM_getServer(FToM* self) { GET_SERVER };
static PyObject * FToM_getStream(FToM* self) { GET_STREAM };
static PyObject * FToM_setMul(FToM *self, PyObject *arg) { SET_MUL };
static PyObject * FToM_setAdd(FToM *self, PyObject *arg) { SET_ADD };
static PyObject * FToM_setSub(FToM *self, PyObject *arg) { SET_SUB };
static PyObject * FToM_setDiv(FToM *self, PyObject *arg) { SET_DIV };

static PyObject * FToM_play(FToM *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FToM_out(FToM *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FToM_stop(FToM *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * FToM_multiply(FToM *self, PyObject *arg) { MULTIPLY };
static PyObject * FToM_inplace_multiply(FToM *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FToM_add(FToM *self, PyObject *arg) { ADD };
static PyObject * FToM_inplace_add(FToM *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FToM_sub(FToM *self, PyObject *arg) { SUB };
static PyObject * FToM_inplace_sub(FToM *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FToM_div(FToM *self, PyObject *arg) { DIV };
static PyObject * FToM_inplace_div(FToM *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FToM_members[] =
{
    {"server", T_OBJECT_EX, offsetof(FToM, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(FToM, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(FToM, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(FToM, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(FToM, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef FToM_methods[] =
{
    {"getServer", (PyCFunction)FToM_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)FToM_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)FToM_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)FToM_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)FToM_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)FToM_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)FToM_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)FToM_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)FToM_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FToM_as_number =
{
    (binaryfunc)FToM_add,                         /*nb_add*/
    (binaryfunc)FToM_sub,                         /*nb_subtract*/
    (binaryfunc)FToM_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)FToM_inplace_add,                 /*inplace_add*/
    (binaryfunc)FToM_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)FToM_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)FToM_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)FToM_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject FToMType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.FToM_base",                                   /*tp_name*/
    sizeof(FToM),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)FToM_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &FToM_as_number,                              /*tp_as_number*/
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
    (traverseproc)FToM_traverse,                  /* tp_traverse */
    (inquiry)FToM_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    FToM_methods,                                 /* tp_methods */
    FToM_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    FToM_new,                                     /* tp_new */
};

/************/
/* MToT */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT centralkey;
    MYFLT lastmidi;
    MYFLT curfreq;
    int modebuffer[2]; // need at least 2 slots for mul & add
} MToT;

static void
MToT_process(MToT *self)
{
    int i;
    MYFLT midi;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        midi = in[i];

        if (midi != self->lastmidi)
        {
            self->data[i] = self->curfreq = MYPOW(1.0594630943593, midi - self->centralkey);
            self->lastmidi = midi;
        }
        else
            self->data[i] = self->curfreq;
    }
}

static void MToT_postprocessing_ii(MToT *self) { POST_PROCESSING_II };
static void MToT_postprocessing_ai(MToT *self) { POST_PROCESSING_AI };
static void MToT_postprocessing_ia(MToT *self) { POST_PROCESSING_IA };
static void MToT_postprocessing_aa(MToT *self) { POST_PROCESSING_AA };
static void MToT_postprocessing_ireva(MToT *self) { POST_PROCESSING_IREVA };
static void MToT_postprocessing_areva(MToT *self) { POST_PROCESSING_AREVA };
static void MToT_postprocessing_revai(MToT *self) { POST_PROCESSING_REVAI };
static void MToT_postprocessing_revaa(MToT *self) { POST_PROCESSING_REVAA };
static void MToT_postprocessing_revareva(MToT *self) { POST_PROCESSING_REVAREVA };

static void
MToT_setProcMode(MToT *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = MToT_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = MToT_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = MToT_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = MToT_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = MToT_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = MToT_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = MToT_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = MToT_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = MToT_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = MToT_postprocessing_revareva;
            break;
    }
}

static void
MToT_compute_next_data_frame(MToT *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
MToT_traverse(MToT *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
MToT_clear(MToT *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
MToT_dealloc(MToT* self)
{
    pyo_DEALLOC
    MToT_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MToT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    MToT *self;
    self = (MToT *)type->tp_alloc(type, 0);

    self->centralkey = 60.0;
    self->lastmidi = 0;
    self->curfreq = 8.1757989156437;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MToT_compute_next_data_frame);
    self->mode_func_ptr = MToT_setProcMode;

    static char *kwlist[] = {"input", "centralkey", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FOO, kwlist, &inputtmp, &self->centralkey, &multmp, &addtmp))
        Py_RETURN_NONE;

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

static PyObject *
MToT_setCentralKey(MToT *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->centralkey = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject * MToT_getServer(MToT* self) { GET_SERVER };
static PyObject * MToT_getStream(MToT* self) { GET_STREAM };
static PyObject * MToT_setMul(MToT *self, PyObject *arg) { SET_MUL };
static PyObject * MToT_setAdd(MToT *self, PyObject *arg) { SET_ADD };
static PyObject * MToT_setSub(MToT *self, PyObject *arg) { SET_SUB };
static PyObject * MToT_setDiv(MToT *self, PyObject *arg) { SET_DIV };

static PyObject * MToT_play(MToT *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MToT_out(MToT *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MToT_stop(MToT *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MToT_multiply(MToT *self, PyObject *arg) { MULTIPLY };
static PyObject * MToT_inplace_multiply(MToT *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MToT_add(MToT *self, PyObject *arg) { ADD };
static PyObject * MToT_inplace_add(MToT *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MToT_sub(MToT *self, PyObject *arg) { SUB };
static PyObject * MToT_inplace_sub(MToT *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MToT_div(MToT *self, PyObject *arg) { DIV };
static PyObject * MToT_inplace_div(MToT *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MToT_members[] =
{
    {"server", T_OBJECT_EX, offsetof(MToT, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(MToT, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(MToT, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(MToT, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(MToT, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef MToT_methods[] =
{
    {"getServer", (PyCFunction)MToT_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)MToT_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)MToT_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)MToT_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)MToT_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setCentralKey", (PyCFunction)MToT_setCentralKey, METH_O, NULL},
    {"setMul", (PyCFunction)MToT_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)MToT_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)MToT_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)MToT_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MToT_as_number =
{
    (binaryfunc)MToT_add,                         /*nb_add*/
    (binaryfunc)MToT_sub,                         /*nb_subtract*/
    (binaryfunc)MToT_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)MToT_inplace_add,                 /*inplace_add*/
    (binaryfunc)MToT_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MToT_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MToT_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MToT_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MToTType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MToT_base",                                   /*tp_name*/
    sizeof(MToT),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)MToT_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &MToT_as_number,                              /*tp_as_number*/
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
    (traverseproc)MToT_traverse,                  /* tp_traverse */
    (inquiry)MToT_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    MToT_methods,                                 /* tp_methods */
    MToT_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    MToT_new,                                     /* tp_new */
};

/************/
/* Resample */
/************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT **pimpulse;
    MYFLT **pinput;
    int factor;
    int last_factor;
    int count;
    int dir;    // Up = 1, DOWN = 0
    int size;   // UP: 0 = zero-padding, 1 = sample-and-hold, 2+ polyphase lowpass filtering
    // DOWN: <2 = discard extra samples, 2+ polyphase lowpass filtering
    int modebuffer[2]; // need at least 2 slots for mul & add
    int allocated;
} Resample;

static void
Resample_create_impulse(Resample *self)
{
    int i, half;
    MYFLT val, scl, sum, invSum, env, w;
    MYFLT impulse[self->size];

    half = self->size / 2;
    sum = 0.0;
    w = TWOPI * 0.49 / self->factor;

    for (i = 0; i < half; i++)
    {
        env = 0.5 * (1.0 - MYCOS(TWOPI * i / self->size));
        scl = i - half;
        val = (MYSIN(w * scl) / scl) * env;
        sum += val;
        impulse[i] = val;
    }

    sum *= 2.0;
    sum += w;
    invSum = 1.0 / sum;
    impulse[half] = w * invSum;

    for (i = 0; i < half; i++)
    {
        impulse[i] *= invSum;
    }

    for (i = half + 1; i < self->size; i++)
    {
        impulse[i] = impulse[self->size - i];
    }

    /* Create sub-filters. */
    for (i = 0; i < self->size; i++)
    {
        self->pimpulse[i % self->factor][(int)(i / self->factor)] = impulse[i];
    }
}

static void
Resample_update_mode(Resample *self)
{
    int i = 0, j = 0;

    if (self->size > self->factor)
    {
        if (self->allocated)
        {
            for (j = 0; j < self->last_factor; j++)
            {
                PyMem_RawFree(self->pimpulse[j]);
                PyMem_RawFree(self->pinput[j]);
            }
        }

        self->pimpulse = (MYFLT **)PyMem_RawRealloc(self->pimpulse, self->factor * sizeof(MYFLT *));
        self->pinput = (MYFLT **)PyMem_RawRealloc(self->pinput, self->factor * sizeof(MYFLT *));

        for (j = 0; j < self->factor; j++)
        {
            self->pimpulse[j] = (MYFLT *)PyMem_RawMalloc(self->size / self->factor * sizeof(MYFLT));
            self->pinput[j] = (MYFLT *)PyMem_RawMalloc(self->size / self->factor * sizeof(MYFLT));

            for (i = 0; i < self->size / self->factor; i++)
            {
                self->pinput[j][i] = 0.0;
            }
        }

        self->last_factor = self->factor;
        self->allocated = 1;

        Resample_create_impulse(self);
    }
}

static void
Resample_downsample(Resample *self)
{
    int i, j, k, tmp_count, len;
    MYFLT filtout;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    len = self->size / self->factor;

    if (self->size <= self->factor)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = in[i * self->factor];
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = 0.0;

            for (j = 0; j < self->factor; j++) // for each polyphase sub-filter...
            {
                filtout = 0.0;
                tmp_count = self->count;

                for (k = 0; k < len; k++) // ... apply filter...
                {
                    if (tmp_count < 0)
                        tmp_count += len;

                    filtout += self->pinput[j][tmp_count--] * self->pimpulse[j][k];
                }

                self->data[i] += filtout; // ... and sum.
            }

            // Input decomposition (input delay line in reverse order).
            self->count++;

            if (self->count == len)
                self->count = 0;

            for (j = 0; j < self->factor; j++)
            {
                self->pinput[self->factor - 1 - j][self->count] = in[i * self->factor + j];
            }
        }
    }
}

static void
Resample_upsample(Resample *self)
{
    int i, j, k, tmp_count, len;
    MYFLT filtout;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    len = self->size / self->factor;

    if (self->size == 0)
    {
        for (i = 0; i < (self->bufsize / self->factor); i++)
        {
            j = i * self->factor;
            self->data[j] = in[i];

            for (k = 1; k < self->factor; k++)
            {
                self->data[j + k] = 0.0;
            }
        }
    }
    else if (self->size == self->factor)
    {
        for (i = 0; i < (self->bufsize / self->factor); i++)
        {
            for (k = 0; k < self->factor; k++)
            {
                self->data[i * self->factor + k] = in[i];
            }
        }
    }
    else
    {
        for (i = 0; i < self->bufsize / self->factor; i++)
        {
            for (j = 0; j < self->factor; j++) // for each polyphase sub-filter...
            {
                filtout = 0.0;
                tmp_count = self->count;

                for (k = 0; k < len; k++) // ... apply filter...
                {
                    if (tmp_count < 0)
                        tmp_count += len;

                    filtout += self->pinput[j][tmp_count--] * self->pimpulse[j][k];
                }

                self->data[i * self->factor + j] = filtout;
            }

            self->count++;

            if (self->count == len)
                self->count = 0;

            for (j = 0; j < self->factor; j++)
            {
                self->pinput[self->factor - 1 - j][self->count] = in[i];
            }
        }
    }
}

static void
Resample_process(Resample *self)
{
    if (self->dir == 0)
        Resample_downsample(self);
    else
        Resample_upsample(self);
}

static void Resample_postprocessing_ii(Resample *self) { POST_PROCESSING_II };
static void Resample_postprocessing_ai(Resample *self) { POST_PROCESSING_AI };
static void Resample_postprocessing_ia(Resample *self) { POST_PROCESSING_IA };
static void Resample_postprocessing_aa(Resample *self) { POST_PROCESSING_AA };
static void Resample_postprocessing_ireva(Resample *self) { POST_PROCESSING_IREVA };
static void Resample_postprocessing_areva(Resample *self) { POST_PROCESSING_AREVA };
static void Resample_postprocessing_revai(Resample *self) { POST_PROCESSING_REVAI };
static void Resample_postprocessing_revaa(Resample *self) { POST_PROCESSING_REVAA };
static void Resample_postprocessing_revareva(Resample *self) { POST_PROCESSING_REVAREVA };

static void
Resample_setProcMode(Resample *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Resample_process;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Resample_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Resample_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Resample_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Resample_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Resample_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Resample_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Resample_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Resample_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Resample_postprocessing_revareva;
            break;
    }
}

static void
Resample_compute_next_data_frame(Resample *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Resample_traverse(Resample *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Resample_clear(Resample *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Resample_dealloc(Resample* self)
{
    int i;
    pyo_DEALLOC

    if (self->size > self->factor)
    {
        for (i = 0; i < self->factor; i++)
        {
            PyMem_RawFree(self->pimpulse[i]);
            PyMem_RawFree(self->pinput[i]);
        }

        PyMem_RawFree(self->pimpulse);
        PyMem_RawFree(self->pinput);
    }

    Resample_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Resample_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, lfac, cfac, mode;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Resample *self;
    self = (Resample *)type->tp_alloc(type, 0);

    self->last_factor = self->factor = 1;
    self->size = 0;
    self->count = 0;
    self->dir = 0;
    self->allocated = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Resample_compute_next_data_frame);
    self->mode_func_ptr = Resample_setProcMode;

    static char *kwlist[] = {"input", "mode", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &mode, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    lfac = Server_getLastResamplingFactor((Server *)self->server);
    cfac = Server_getCurrentResamplingFactor((Server *)self->server);

    if (lfac == 1)
    {
        if (cfac < 0)
        {
            self->dir = 0;
            cfac = -cfac;
        }
        else
            self->dir = 1;

        self->factor = cfac;
    }
    else if (cfac == 1)
    {
        if (lfac < 0)
        {
            self->dir = 1;
            lfac = -lfac;
        }
        else
            self->dir = 0;

        self->factor = lfac;
    }

    self->size = self->factor * mode;
    Resample_update_mode(self);

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

static PyObject *
Resample_setMode(Resample *self, PyObject *arg)
{
    int mode = -1;
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg) == 1)
    {
        mode = PyLong_AsLong(arg);
    }

    if (mode >= 0)
    {
        self->size = self->factor * mode;
        Resample_update_mode(self);
    }

    Py_RETURN_NONE;
}

static PyObject * Resample_getServer(Resample* self) { GET_SERVER };
static PyObject * Resample_getStream(Resample* self) { GET_STREAM };
static PyObject * Resample_setMul(Resample *self, PyObject *arg) { SET_MUL };
static PyObject * Resample_setAdd(Resample *self, PyObject *arg) { SET_ADD };
static PyObject * Resample_setSub(Resample *self, PyObject *arg) { SET_SUB };
static PyObject * Resample_setDiv(Resample *self, PyObject *arg) { SET_DIV };

static PyObject * Resample_play(Resample *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Resample_out(Resample *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Resample_stop(Resample *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Resample_multiply(Resample *self, PyObject *arg) { MULTIPLY };
static PyObject * Resample_inplace_multiply(Resample *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Resample_add(Resample *self, PyObject *arg) { ADD };
static PyObject * Resample_inplace_add(Resample *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Resample_sub(Resample *self, PyObject *arg) { SUB };
static PyObject * Resample_inplace_sub(Resample *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Resample_div(Resample *self, PyObject *arg) { DIV };
static PyObject * Resample_inplace_div(Resample *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Resample_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Resample, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(Resample, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(Resample, input), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(Resample, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(Resample, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef Resample_methods[] =
{
    {"getServer", (PyCFunction)Resample_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)Resample_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)Resample_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)Resample_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)Resample_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMode", (PyCFunction)Resample_setMode, METH_O, NULL},
    {"setMul", (PyCFunction)Resample_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)Resample_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)Resample_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)Resample_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Resample_as_number =
{
    (binaryfunc)Resample_add,                         /*nb_add*/
    (binaryfunc)Resample_sub,                         /*nb_subtract*/
    (binaryfunc)Resample_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)Resample_inplace_add,                 /*inplace_add*/
    (binaryfunc)Resample_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Resample_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)Resample_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)Resample_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ResampleType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Resample_base",                                   /*tp_name*/
    sizeof(Resample),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Resample_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &Resample_as_number,                              /*tp_as_number*/
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
    (traverseproc)Resample_traverse,                  /* tp_traverse */
    (inquiry)Resample_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Resample_methods,                                 /* tp_methods */
    Resample_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Resample_new,                                     /* tp_new */
};
