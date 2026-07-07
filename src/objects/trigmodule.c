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
#include "tablemodule.h"
#include "interpolation.h"

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *max;
    Stream *max_stream;
    MYFLT value;
    int modebuffer[3]; // need at least 2 slots for mul & add
} TrigRandInt;

static void
TrigRandInt_generate_i(TrigRandInt *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
            self->value = (MYFLT)((int)(RANDOM_UNIFORM * ma));

        self->data[i] = self->value;
    }
}

static void
TrigRandInt_generate_a(TrigRandInt *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
            self->value = (MYFLT)((int)(RANDOM_UNIFORM * ma[i]));

        self->data[i] = self->value;
    }
}

static void TrigRandInt_postprocessing_ii(TrigRandInt *self) { POST_PROCESSING_II };
static void TrigRandInt_postprocessing_ai(TrigRandInt *self) { POST_PROCESSING_AI };
static void TrigRandInt_postprocessing_ia(TrigRandInt *self) { POST_PROCESSING_IA };
static void TrigRandInt_postprocessing_aa(TrigRandInt *self) { POST_PROCESSING_AA };
static void TrigRandInt_postprocessing_ireva(TrigRandInt *self) { POST_PROCESSING_IREVA };
static void TrigRandInt_postprocessing_areva(TrigRandInt *self) { POST_PROCESSING_AREVA };
static void TrigRandInt_postprocessing_revai(TrigRandInt *self) { POST_PROCESSING_REVAI };
static void TrigRandInt_postprocessing_revaa(TrigRandInt *self) { POST_PROCESSING_REVAA };
static void TrigRandInt_postprocessing_revareva(TrigRandInt *self) { POST_PROCESSING_REVAREVA };

static void
TrigRandInt_setProcMode(TrigRandInt *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_generate_i);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_generate_a);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_postprocessing_revareva);
            break;
    }
}

static void
TrigRandInt_compute_next_data_frame(TrigRandInt *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigRandInt_traverse(TrigRandInt *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->max);
    return 0;
}

static int
TrigRandInt_clear(TrigRandInt *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->max);
    return 0;
}

static void
TrigRandInt_dealloc(TrigRandInt* self)
{
    pyo_DEALLOC
    TrigRandInt_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigRandInt_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT ma;
    PyObject *inputtmp, *input_streamtmp, *maxtmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigRandInt *self;
    self = (TrigRandInt *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->max = PyFloat_FromDouble(100.);
    self->value = 0.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigRandInt_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigRandInt_setProcMode);

    static char *kwlist[] = {"input", "max", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &maxtmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (maxtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMax", maxtmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Server_generateSeed((Server *)self->server, TRIGRANDINT_ID);

    if (self->modebuffer[2] == 0)
        ma = PyFloat_AsDouble(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];

    self->value = (MYFLT)((int)(RANDOM_UNIFORM * ma));

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigRandInt_getServer(TrigRandInt* self) { GET_SERVER };
static PyObject * TrigRandInt_getStream(TrigRandInt* self) { GET_STREAM };
static PyObject * TrigRandInt_setMul(TrigRandInt *self, PyObject *arg) { SET_MUL };
static PyObject * TrigRandInt_setAdd(TrigRandInt *self, PyObject *arg) { SET_ADD };
static PyObject * TrigRandInt_setSub(TrigRandInt *self, PyObject *arg) { SET_SUB };
static PyObject * TrigRandInt_setDiv(TrigRandInt *self, PyObject *arg) { SET_DIV };

static PyObject * TrigRandInt_play(TrigRandInt *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigRandInt_out(TrigRandInt *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigRandInt_stop(TrigRandInt *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigRandInt_multiply(TrigRandInt *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigRandInt_inplace_multiply(TrigRandInt *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigRandInt_add(TrigRandInt *self, PyObject *arg) { ADD };
static PyObject * TrigRandInt_inplace_add(TrigRandInt *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigRandInt_sub(TrigRandInt *self, PyObject *arg) { SUB };
static PyObject * TrigRandInt_inplace_sub(TrigRandInt *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigRandInt_div(TrigRandInt *self, PyObject *arg) { DIV };
static PyObject * TrigRandInt_inplace_div(TrigRandInt *self, PyObject *arg) { INPLACE_DIV };

static PyObject * TrigRandInt_setMax(TrigRandInt *self, PyObject *arg) { SET_PARAM(self->max, self->max_stream, 2); }

static PyMemberDef TrigRandInt_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigRandInt, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigRandInt, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigRandInt, input), 0, "Input sound object."},
    {"max", T_OBJECT_EX, offsetof(TrigRandInt, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(TrigRandInt, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigRandInt, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigRandInt_methods[] =
{
    {"getServer", (PyCFunction)TrigRandInt_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigRandInt_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigRandInt_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigRandInt_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigRandInt_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMax", (PyCFunction)TrigRandInt_setMax, METH_O, "Sets maximum possible value."},
    {"setMul", (PyCFunction)TrigRandInt_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigRandInt_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigRandInt_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigRandInt_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigRandIntType_slots[] =
{
    {Py_tp_dealloc, TrigRandInt_dealloc},
    {Py_tp_doc, "TrigRandInt objects. Generates a new random integer value on a trigger signal."},
    {Py_tp_traverse, TrigRandInt_traverse},
    {Py_tp_clear, TrigRandInt_clear},
    {Py_tp_methods, TrigRandInt_methods},
    {Py_tp_members, TrigRandInt_members},
    {Py_nb_add, TrigRandInt_add},
    {Py_nb_subtract, TrigRandInt_sub},
    {Py_nb_multiply, TrigRandInt_multiply},
    {Py_nb_true_divide, TrigRandInt_div},
    {Py_nb_inplace_add, TrigRandInt_inplace_add},
    {Py_nb_inplace_subtract, TrigRandInt_inplace_sub},
    {Py_nb_inplace_multiply, TrigRandInt_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigRandInt_inplace_div},
    {Py_tp_new, TrigRandInt_new},
    {0, NULL}
};

static PyType_Spec TrigRandIntType_spec =
{
    "_pyo.TrigRandInt_base",
    sizeof(TrigRandInt),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigRandIntType_slots
};

PyTypeObject *
PyoCreateTrigRandIntType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigRandIntType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    PyObject *max;
    Stream *min_stream;
    Stream *max_stream;
    MYFLT value;
    MYFLT currentValue;
    MYFLT time;
    int timeStep;
    MYFLT stepVal;
    int timeCount;
    int modebuffer[4]; // need at least 2 slots for mul & add
} TrigRand;

static void
TrigRand_generate_ii(TrigRand *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    MYFLT range = ma - mi;

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->timeCount = 0;
            self->value = range * RANDOM_UNIFORM + mi;

            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1))
        {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep)
        {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }

        self->data[i] = self->currentValue;
    }
}

static void
TrigRand_generate_ai(TrigRand *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);

    for (i = 0; i < self->bufsize; i++)
    {
        MYFLT range = ma - mi[i];

        if (in[i] == 1)
        {
            self->timeCount = 0;
            self->value = range * RANDOM_UNIFORM + mi[i];

            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1))
        {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep)
        {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }

        self->data[i] = self->currentValue;
    }
}

static void
TrigRand_generate_ia(TrigRand *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        MYFLT range = ma[i] - mi;

        if (in[i] == 1)
        {
            self->timeCount = 0;
            self->value = range * RANDOM_UNIFORM + mi;

            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1))
        {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep)
        {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }

        self->data[i] = self->currentValue;
    }
}

static void
TrigRand_generate_aa(TrigRand *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        MYFLT range = ma[i] - mi[i];

        if (in[i] == 1)
        {
            self->timeCount = 0;
            self->value = range * RANDOM_UNIFORM + mi[i];

            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1))
        {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep)
        {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }

        self->data[i] = self->currentValue;
    }
}

static void TrigRand_postprocessing_ii(TrigRand *self) { POST_PROCESSING_II };
static void TrigRand_postprocessing_ai(TrigRand *self) { POST_PROCESSING_AI };
static void TrigRand_postprocessing_ia(TrigRand *self) { POST_PROCESSING_IA };
static void TrigRand_postprocessing_aa(TrigRand *self) { POST_PROCESSING_AA };
static void TrigRand_postprocessing_ireva(TrigRand *self) { POST_PROCESSING_IREVA };
static void TrigRand_postprocessing_areva(TrigRand *self) { POST_PROCESSING_AREVA };
static void TrigRand_postprocessing_revai(TrigRand *self) { POST_PROCESSING_REVAI };
static void TrigRand_postprocessing_revaa(TrigRand *self) { POST_PROCESSING_REVAA };
static void TrigRand_postprocessing_revareva(TrigRand *self) { POST_PROCESSING_REVAREVA };

static void
TrigRand_setProcMode(TrigRand *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_generate_ii);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_generate_ai);
            break;

        case 10:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_generate_ia);
            break;

        case 11:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_generate_aa);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_postprocessing_revareva);
            break;
    }
}

static void
TrigRand_compute_next_data_frame(TrigRand *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigRand_traverse(TrigRand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->min);
    Py_VISIT(self->max);
    return 0;
}

static int
TrigRand_clear(TrigRand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->min);
    Py_CLEAR(self->max);
    return 0;
}

static void
TrigRand_dealloc(TrigRand* self)
{
    pyo_DEALLOC
    TrigRand_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigRand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *mintmp = NULL, *maxtmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigRand *self;
    self = (TrigRand *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->value = self->currentValue = 0.;
    self->time = 0.0;
    self->timeCount = 0;
    self->stepVal = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigRand_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigRand_setProcMode);

    static char *kwlist[] = {"input", "min", "max", "port", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFFOO, kwlist, &inputtmp, &mintmp, &maxtmp, &self->time, &inittmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (mintmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMin", mintmp);
    }

    if (maxtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMax", maxtmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Server_generateSeed((Server *)self->server, TRIGRAND_ID);

    self->value = self->currentValue = inittmp;
    self->timeStep = (int)(self->time * self->sr);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigRand_getServer(TrigRand* self) { GET_SERVER };
static PyObject * TrigRand_getStream(TrigRand* self) { GET_STREAM };
static PyObject * TrigRand_setMul(TrigRand *self, PyObject *arg) { SET_MUL };
static PyObject * TrigRand_setAdd(TrigRand *self, PyObject *arg) { SET_ADD };
static PyObject * TrigRand_setSub(TrigRand *self, PyObject *arg) { SET_SUB };
static PyObject * TrigRand_setDiv(TrigRand *self, PyObject *arg) { SET_DIV };

static PyObject * TrigRand_play(TrigRand *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigRand_out(TrigRand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigRand_stop(TrigRand *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigRand_multiply(TrigRand *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigRand_inplace_multiply(TrigRand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigRand_add(TrigRand *self, PyObject *arg) { ADD };
static PyObject * TrigRand_inplace_add(TrigRand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigRand_sub(TrigRand *self, PyObject *arg) { SUB };
static PyObject * TrigRand_inplace_sub(TrigRand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigRand_div(TrigRand *self, PyObject *arg) { DIV };
static PyObject * TrigRand_inplace_div(TrigRand *self, PyObject *arg) { INPLACE_DIV };

static PyObject * TrigRand_setMin(TrigRand *self, PyObject *arg) { SET_PARAM(self->min, self->min_stream, 2); }
static PyObject * TrigRand_setMax(TrigRand *self, PyObject *arg) { SET_PARAM(self->max, self->max_stream, 3); }

static PyObject *
TrigRand_setPort(TrigRand *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->time = PyFloat_AsDouble(arg);
        self->timeStep = (int)(self->time * self->sr);
    }

    Py_RETURN_NONE;
}

static PyMemberDef TrigRand_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigRand, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigRand, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigRand, input), 0, "Input sound object."},
    {"min", T_OBJECT_EX, offsetof(TrigRand, min), 0, "Minimum possible value."},
    {"max", T_OBJECT_EX, offsetof(TrigRand, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(TrigRand, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigRand, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigRand_methods[] =
{
    {"getServer", (PyCFunction)TrigRand_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigRand_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigRand_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigRand_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigRand_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMin", (PyCFunction)TrigRand_setMin, METH_O, "Sets minimum possible value."},
    {"setMax", (PyCFunction)TrigRand_setMax, METH_O, "Sets maximum possible value."},
    {"setPort", (PyCFunction)TrigRand_setPort, METH_O, "Sets a new ramp time value."},
    {"setMul", (PyCFunction)TrigRand_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigRand_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigRand_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigRand_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigRandType_slots[] =
{
    {Py_tp_dealloc, TrigRand_dealloc},
    {Py_tp_doc, "TrigRand objects. Generates a new random value on a trigger signal."},
    {Py_tp_traverse, TrigRand_traverse},
    {Py_tp_clear, TrigRand_clear},
    {Py_tp_methods, TrigRand_methods},
    {Py_tp_members, TrigRand_members},
    {Py_nb_add, TrigRand_add},
    {Py_nb_subtract, TrigRand_sub},
    {Py_nb_multiply, TrigRand_multiply},
    {Py_nb_true_divide, TrigRand_div},
    {Py_nb_inplace_add, TrigRand_inplace_add},
    {Py_nb_inplace_subtract, TrigRand_inplace_sub},
    {Py_nb_inplace_multiply, TrigRand_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigRand_inplace_div},
    {Py_tp_new, TrigRand_new},
    {0, NULL}
};

static PyType_Spec TrigRandType_spec =
{
    "_pyo.TrigRand_base",
    sizeof(TrigRand),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigRandType_slots
};

PyTypeObject *
PyoCreateTrigRandType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigRandType_spec, NULL);
}

/*********************************************************************************************/
/* TrigChoice ********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int chSize;
    MYFLT *choice;
    MYFLT value;
    MYFLT currentValue;
    MYFLT time;
    int timeStep;
    MYFLT stepVal;
    int timeCount;
    int modebuffer[2]; // need at least 2 slots for mul & add
} TrigChoice;

static void
TrigChoice_generate(TrigChoice *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->timeCount = 0;
            self->value = self->choice[(int)(RANDOM_UNIFORM * self->chSize)];

            if (self->time <= 0.0)
                self->currentValue = self->value;
            else
                self->stepVal = (self->value - self->currentValue) / self->timeStep;
        }

        if (self->timeCount == (self->timeStep - 1))
        {
            self->currentValue = self->value;
            self->timeCount++;
        }
        else if (self->timeCount < self->timeStep)
        {
            self->currentValue += self->stepVal;
            self->timeCount++;
        }

        self->data[i] = self->currentValue;
    }
}

static void TrigChoice_postprocessing_ii(TrigChoice *self) { POST_PROCESSING_II };
static void TrigChoice_postprocessing_ai(TrigChoice *self) { POST_PROCESSING_AI };
static void TrigChoice_postprocessing_ia(TrigChoice *self) { POST_PROCESSING_IA };
static void TrigChoice_postprocessing_aa(TrigChoice *self) { POST_PROCESSING_AA };
static void TrigChoice_postprocessing_ireva(TrigChoice *self) { POST_PROCESSING_IREVA };
static void TrigChoice_postprocessing_areva(TrigChoice *self) { POST_PROCESSING_AREVA };
static void TrigChoice_postprocessing_revai(TrigChoice *self) { POST_PROCESSING_REVAI };
static void TrigChoice_postprocessing_revaa(TrigChoice *self) { POST_PROCESSING_REVAA };
static void TrigChoice_postprocessing_revareva(TrigChoice *self) { POST_PROCESSING_REVAREVA };

static void
TrigChoice_setProcMode(TrigChoice *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_generate);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_postprocessing_revareva);
            break;
    }
}

static void
TrigChoice_compute_next_data_frame(TrigChoice *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigChoice_traverse(TrigChoice *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
TrigChoice_clear(TrigChoice *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
TrigChoice_dealloc(TrigChoice* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->choice);
    TrigChoice_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigChoice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *choicetmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigChoice *self;
    self = (TrigChoice *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = self->currentValue = 0.;
    self->time = 0.0;
    self->timeCount = 0;
    self->stepVal = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigChoice_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigChoice_setProcMode);

    static char *kwlist[] = {"input", "choice", "port", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_FFOO, kwlist, &inputtmp, &choicetmp, &self->time, &inittmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (choicetmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setChoice", choicetmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Server_generateSeed((Server *)self->server, TRIGCHOICE_ID);

    self->value = self->currentValue = inittmp;
    self->timeStep = (int)(self->time * self->sr);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigChoice_getServer(TrigChoice* self) { GET_SERVER };
static PyObject * TrigChoice_getStream(TrigChoice* self) { GET_STREAM };
static PyObject * TrigChoice_setMul(TrigChoice *self, PyObject *arg) { SET_MUL };
static PyObject * TrigChoice_setAdd(TrigChoice *self, PyObject *arg) { SET_ADD };
static PyObject * TrigChoice_setSub(TrigChoice *self, PyObject *arg) { SET_SUB };
static PyObject * TrigChoice_setDiv(TrigChoice *self, PyObject *arg) { SET_DIV };

static PyObject * TrigChoice_play(TrigChoice *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigChoice_out(TrigChoice *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigChoice_stop(TrigChoice *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigChoice_multiply(TrigChoice *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigChoice_inplace_multiply(TrigChoice *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigChoice_add(TrigChoice *self, PyObject *arg) { ADD };
static PyObject * TrigChoice_inplace_add(TrigChoice *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigChoice_sub(TrigChoice *self, PyObject *arg) { SUB };
static PyObject * TrigChoice_inplace_sub(TrigChoice *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigChoice_div(TrigChoice *self, PyObject *arg) { DIV };
static PyObject * TrigChoice_inplace_div(TrigChoice *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigChoice_setChoice(TrigChoice *self, PyObject *arg)
{
    int i;

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

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
TrigChoice_setPort(TrigChoice *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->time = PyFloat_AsDouble(arg);
        self->timeStep = (int)(self->time * self->sr);
    }

    Py_RETURN_NONE;
}

static PyMemberDef TrigChoice_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigChoice, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigChoice, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigChoice, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(TrigChoice, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigChoice, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigChoice_methods[] =
{
    {"getServer", (PyCFunction)TrigChoice_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigChoice_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigChoice_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigChoice_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigChoice_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setChoice", (PyCFunction)TrigChoice_setChoice, METH_O, "Sets possible values."},
    {"setPort", (PyCFunction)TrigChoice_setPort, METH_O, "Sets new portamento time."},
    {"setMul", (PyCFunction)TrigChoice_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigChoice_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigChoice_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigChoice_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigChoiceType_slots[] =
{
    {Py_tp_dealloc, TrigChoice_dealloc},
    {Py_tp_doc, "TrigChoice objects. Generates a new random value pick in a user choice on a trigger signal."},
    {Py_tp_traverse, TrigChoice_traverse},
    {Py_tp_clear, TrigChoice_clear},
    {Py_tp_methods, TrigChoice_methods},
    {Py_tp_members, TrigChoice_members},
    {Py_nb_add, TrigChoice_add},
    {Py_nb_subtract, TrigChoice_sub},
    {Py_nb_multiply, TrigChoice_multiply},
    {Py_nb_true_divide, TrigChoice_div},
    {Py_nb_inplace_add, TrigChoice_inplace_add},
    {Py_nb_inplace_subtract, TrigChoice_inplace_sub},
    {Py_nb_inplace_multiply, TrigChoice_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigChoice_inplace_div},
    {Py_tp_new, TrigChoice_new},
    {0, NULL}
};

static PyType_Spec TrigChoiceType_spec =
{
    "_pyo.TrigChoice_base",
    sizeof(TrigChoice),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigChoiceType_slots
};

PyTypeObject *
PyoCreateTrigChoiceType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigChoiceType_spec, NULL);
}

/*********************************************************************************************/
/* TrigFunc ********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *arg;
    PyObject *func;
} TrigFunc;

static void
TrigFunc_generate(TrigFunc *self)
{
    int i;
    PyObject *tuple, *result;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            if (self->arg == Py_None)
            {
                tuple = PyTuple_New(0);
                result = PyObject_Call(self->func, tuple, NULL);
                Py_DECREF(tuple);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
                Py_DECREF(result);
            }
            else
            {
                tuple = PyTuple_New(1);
                Py_INCREF(self->arg);
                PyTuple_SET_ITEM(tuple, 0, self->arg);
                result = PyObject_Call(self->func, tuple, NULL);
                Py_DECREF(tuple);

                if (result == NULL)
                {
                    PyErr_Print();
                    return;
                }
                Py_DECREF(result);
            }
        }
    }
}

static void
TrigFunc_compute_next_data_frame(TrigFunc *self)
{
    TrigFunc_generate(self);
}

static int
TrigFunc_traverse(TrigFunc *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->func);
    Py_VISIT(self->arg);
    return 0;
}

static int
TrigFunc_clear(TrigFunc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->func);
    Py_CLEAR(self->arg);
    return 0;
}

static void
TrigFunc_dealloc(TrigFunc* self)
{
    pyo_DEALLOC
    TrigFunc_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigFunc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *functmp = NULL, *argtmp = NULL;
    TrigFunc *self;
    self = (TrigFunc *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->arg = Py_None;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigFunc_compute_next_data_frame));

    static char *kwlist[] = {"input", "function", "arg", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|O", kwlist, &inputtmp, &functmp, &argtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (functmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setFunction", functmp);
    }

    if (argtmp)
    {
        if (PyTuple_Check(argtmp))
        {
            PyObject *argument = PyTuple_New(1);
            PyTuple_SetItem(argument, 0, argtmp);
            PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setArg", argument);
        }
        else
        {
            PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setArg", argtmp);
        }
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    return (PyObject *)self;
}

static PyObject * TrigFunc_getServer(TrigFunc* self) { GET_SERVER };
static PyObject * TrigFunc_getStream(TrigFunc* self) { GET_STREAM };

static PyObject * TrigFunc_play(TrigFunc *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigFunc_stop(TrigFunc *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
TrigFunc_setFunction(TrigFunc *self, PyObject *arg)
{
    if (! PyCallable_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The function attribute must be callable.");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->func);
    self->func = arg;
    Py_INCREF(self->func);

    Py_RETURN_NONE;
}

static PyObject *
TrigFunc_setArg(TrigFunc *self, PyObject *arg)
{
    Py_XDECREF(self->arg);
    self->arg = arg;
    Py_INCREF(self->arg);

    Py_RETURN_NONE;
}

static PyMemberDef TrigFunc_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigFunc, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigFunc, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigFunc, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigFunc_methods[] =
{
    {"getServer", (PyCFunction)TrigFunc_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigFunc_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigFunc_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigFunc_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setFunction", (PyCFunction)TrigFunc_setFunction, METH_O, "Sets function to be called."},
    {"setArg", (PyCFunction)TrigFunc_setArg, METH_O, "Sets function's argument."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigFuncType_slots[] =
{
    {Py_tp_dealloc, TrigFunc_dealloc},
    {Py_tp_doc, "TrigFunc objects. Called a function on a trigger signal."},
    {Py_tp_traverse, TrigFunc_traverse},
    {Py_tp_clear, TrigFunc_clear},
    {Py_tp_methods, TrigFunc_methods},
    {Py_tp_members, TrigFunc_members},
    {Py_tp_new, TrigFunc_new},
    {0, NULL}
};

static PyType_Spec TrigFuncType_spec =
{
    "_pyo.TrigFunc_base",
    sizeof(TrigFunc),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigFuncType_slots
};

PyTypeObject *
PyoCreateTrigFuncType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigFuncType_spec, NULL);
}

/*********************************************************************************************/
/* TrigEnv *********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *table;
    PyObject *input;
    Stream *input_stream;
    PyObject *dur;
    Stream *dur_stream;
    int modebuffer[3];
    int active;
    MYFLT current_dur; // duration in samples
    MYFLT inc; // table size / current_dur
    double pointerPos; // reading position in sample
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, T_SIZE_T, MYFLT, T_SIZE_T);
} TrigEnv;

static void
TrigEnv_readframes_i(TrigEnv *self)
{
    MYFLT fpart;
    int i;
    T_SIZE_T ipart;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *tablelist = TableStream_getData((TableStream *)self->table);
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] == 1)
        {
            MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
            self->current_dur = self->sr * dur;

            if (self->current_dur <= 0.0)
            {
                self->current_dur = 0.0;
                self->inc = 0.0;
                self->active = 0;
            }
            else
            {
                self->inc = (MYFLT)size / self->current_dur;
                self->active = 1;
            }

            self->pointerPos = 0.;
        }

        if (self->active == 1)
        {
            ipart = (T_SIZE_T)self->pointerPos;
            fpart = self->pointerPos - ipart;
            self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;

        if (self->pointerPos > size && self->active == 1)
        {
            self->trigsBuffer[i] = 1.0;
            self->active = 0;
        }
    }
}

static void
TrigEnv_readframes_a(TrigEnv *self)
{
    MYFLT fpart, dur;
    int i;
    T_SIZE_T ipart;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *dur_st = Stream_getData((Stream *)self->dur_stream);
    MYFLT *tablelist = TableStream_getData((TableStream *)self->table);
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] == 1)
        {
            dur = dur_st[i];
            self->current_dur = self->sr * dur;

            if (self->current_dur <= 0.0)
            {
                self->current_dur = 0.0;
                self->inc = 0.0;
                self->active = 0;
            }
            else
            {
                self->inc = (MYFLT)size / self->current_dur;
                self->active = 1;
            }

            self->pointerPos = 0.;
        }

        if (self->active == 1)
        {
            ipart = (T_SIZE_T)self->pointerPos;
            fpart = self->pointerPos - ipart;
            self->data[i] = (*self->interp_func_ptr)(tablelist, ipart, fpart, size);
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;

        if (self->pointerPos > size && self->active == 1)
        {
            self->trigsBuffer[i] = 1.0;
            self->active = 0;
        }
    }
}

static void TrigEnv_postprocessing_ii(TrigEnv *self) { POST_PROCESSING_II };
static void TrigEnv_postprocessing_ai(TrigEnv *self) { POST_PROCESSING_AI };
static void TrigEnv_postprocessing_ia(TrigEnv *self) { POST_PROCESSING_IA };
static void TrigEnv_postprocessing_aa(TrigEnv *self) { POST_PROCESSING_AA };
static void TrigEnv_postprocessing_ireva(TrigEnv *self) { POST_PROCESSING_IREVA };
static void TrigEnv_postprocessing_areva(TrigEnv *self) { POST_PROCESSING_AREVA };
static void TrigEnv_postprocessing_revai(TrigEnv *self) { POST_PROCESSING_REVAI };
static void TrigEnv_postprocessing_revaa(TrigEnv *self) { POST_PROCESSING_REVAA };
static void TrigEnv_postprocessing_revareva(TrigEnv *self) { POST_PROCESSING_REVAREVA };

static void
TrigEnv_setProcMode(TrigEnv *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_readframes_i);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_readframes_a);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_postprocessing_revareva);
            break;
    }
}

static void
TrigEnv_compute_next_data_frame(TrigEnv *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigEnv_traverse(TrigEnv *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->dur);
    return 0;
}

static int
TrigEnv_clear(TrigEnv *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->dur);
    return 0;
}

static void
TrigEnv_dealloc(TrigEnv* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->trigsBuffer);
    TrigEnv_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigEnv_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *durtmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigEnv *self;
    self = (TrigEnv *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    self->pointerPos = 0.;
    self->active = 0;
    self->interp = 2;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigEnv_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigEnv_setProcMode);

    self->dur = PyFloat_FromDouble(1.);
    self->current_dur = self->sr;

    static char *kwlist[] = {"input", "table", "dur", "interp", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OiOO", kwlist, &inputtmp, &tabletmp, &durtmp, &self->interp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TrigEnv must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    self->table = PYO_CALL_METHOD_RET((PyObject *)tabletmp, "getTableStream", "");

    if (durtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setDur", durtmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    return (PyObject *)self;
}

static PyObject * TrigEnv_getServer(TrigEnv* self) { GET_SERVER };
static PyObject * TrigEnv_getStream(TrigEnv* self) { GET_STREAM };
static PyObject * TrigEnv_getTriggerStream(TrigEnv* self) { GET_TRIGGER_STREAM };
static PyObject * TrigEnv_setMul(TrigEnv *self, PyObject *arg) { SET_MUL };
static PyObject * TrigEnv_setAdd(TrigEnv *self, PyObject *arg) { SET_ADD };
static PyObject * TrigEnv_setSub(TrigEnv *self, PyObject *arg) { SET_SUB };
static PyObject * TrigEnv_setDiv(TrigEnv *self, PyObject *arg) { SET_DIV };

static PyObject * TrigEnv_play(TrigEnv *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigEnv_out(TrigEnv *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigEnv_stop(TrigEnv *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigEnv_multiply(TrigEnv *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigEnv_inplace_multiply(TrigEnv *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigEnv_add(TrigEnv *self, PyObject *arg) { ADD };
static PyObject * TrigEnv_inplace_add(TrigEnv *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigEnv_sub(TrigEnv *self, PyObject *arg) { SUB };
static PyObject * TrigEnv_inplace_sub(TrigEnv *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigEnv_div(TrigEnv *self, PyObject *arg) { DIV };
static PyObject * TrigEnv_inplace_div(TrigEnv *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigEnv_getTable(TrigEnv* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
TrigEnv_setTable(TrigEnv *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PYO_CALL_METHOD_RET((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject * TrigEnv_setDur(TrigEnv *self, PyObject *arg) { SET_PARAM(self->dur, self->dur_stream, 2); }

static PyObject *
TrigEnv_setInterp(TrigEnv *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyNumber_Check(arg))
    {
        self->interp = PyLong_AsLong(PyNumber_Long(arg));
    }

    SET_INTERP_POINTER

    Py_RETURN_NONE;
}

static PyMemberDef TrigEnv_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigEnv, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigEnv, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(TrigEnv, trig_stream), 0, "Trigger Stream object."},
    {"table", T_OBJECT_EX, offsetof(TrigEnv, table), 0, "Envelope table."},
    {"dur", T_OBJECT_EX, offsetof(TrigEnv, dur), 0, "Envelope duration in seconds."},
    {"mul", T_OBJECT_EX, offsetof(TrigEnv, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigEnv, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigEnv_methods[] =
{
    {"getTable", (PyCFunction)TrigEnv_getTable, METH_NOARGS, "Returns waveform table object."},
    {"getServer", (PyCFunction)TrigEnv_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigEnv_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)TrigEnv_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)TrigEnv_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigEnv_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigEnv_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setTable", (PyCFunction)TrigEnv_setTable, METH_O, "Sets envelope table."},
    {"setDur", (PyCFunction)TrigEnv_setDur, METH_O, "Sets envelope duration in second."},
    {"setInterp", (PyCFunction)TrigEnv_setInterp, METH_O, "Sets oscillator interpolation mode."},
    {"setMul", (PyCFunction)TrigEnv_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)TrigEnv_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)TrigEnv_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigEnv_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigEnvType_slots[] =
{
    {Py_tp_dealloc, TrigEnv_dealloc},
    {Py_tp_doc, "TrigEnv objects. Starts an envelope on a trigger signal."},
    {Py_tp_traverse, TrigEnv_traverse},
    {Py_tp_clear, TrigEnv_clear},
    {Py_tp_methods, TrigEnv_methods},
    {Py_tp_members, TrigEnv_members},
    {Py_nb_add, TrigEnv_add},
    {Py_nb_subtract, TrigEnv_sub},
    {Py_nb_multiply, TrigEnv_multiply},
    {Py_nb_true_divide, TrigEnv_div},
    {Py_nb_inplace_add, TrigEnv_inplace_add},
    {Py_nb_inplace_subtract, TrigEnv_inplace_sub},
    {Py_nb_inplace_multiply, TrigEnv_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigEnv_inplace_div},
    {Py_tp_new, TrigEnv_new},
    {0, NULL}
};

static PyType_Spec TrigEnvType_spec =
{
    "_pyo.TrigEnv_base",
    sizeof(TrigEnv),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigEnvType_slots
};

PyTypeObject *
PyoCreateTrigEnvType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigEnvType_spec, NULL);
}

/*********************************************************************************************/
/* TrigLinseg *********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *pointslist;
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    double currentTime;
    double currentValue;
    MYFLT sampleToSec;
    double increment;
    MYFLT *targets;
    MYFLT *times;
    int which;
    int flag;
    int newlist;
    int listsize;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} TrigLinseg;

static void
TrigLinseg_convert_pointslist(TrigLinseg *self)
{
    int i;
    PyObject *tup;

    self->listsize = PyList_Size(self->pointslist);
    self->targets = (MYFLT *)PyMem_RawRealloc(self->targets, self->listsize * sizeof(MYFLT));
    self->times = (MYFLT *)PyMem_RawRealloc(self->times, self->listsize * sizeof(MYFLT));

    for (i = 0; i < self->listsize; i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        self->times[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
        self->targets[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
    }
}

static void
TrigLinseg_reinit(TrigLinseg *self)
{
    if (self->newlist == 1)
    {
        TrigLinseg_convert_pointslist((TrigLinseg *)self);
        self->newlist = 0;
    }

    self->currentTime = 0.0;
    self->currentValue = self->targets[0];
    self->which = 0;
    self->flag = 1;
}

static void
TrigLinseg_generate(TrigLinseg *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] == 1)
            TrigLinseg_reinit((TrigLinseg *)self);

        if (self->flag == 1)
        {
            if (self->currentTime >= self->times[self->which])
            {
                self->which++;

                if (self->which == self->listsize)
                {
                    self->trigsBuffer[i] = 1.0;
                    self->flag = 0;
                    self->currentValue = self->targets[self->which - 1];
                }
                else if ((self->times[self->which] - self->times[self->which - 1]) <= 0)
                    self->increment = self->targets[self->which] - self->currentValue;
                else
                    self->increment = (self->targets[self->which] - self->targets[self->which - 1]) / ((self->times[self->which] - self->times[self->which - 1]) / self->sampleToSec);
            }

            if (self->currentTime <= self->times[self->listsize - 1])
                self->currentValue += self->increment;

            self->data[i] = (MYFLT)self->currentValue;
            self->currentTime += self->sampleToSec;
        }
        else
            self->data[i] = (MYFLT)self->currentValue;
    }
}

static void TrigLinseg_postprocessing_ii(TrigLinseg *self) { POST_PROCESSING_II };
static void TrigLinseg_postprocessing_ai(TrigLinseg *self) { POST_PROCESSING_AI };
static void TrigLinseg_postprocessing_ia(TrigLinseg *self) { POST_PROCESSING_IA };
static void TrigLinseg_postprocessing_aa(TrigLinseg *self) { POST_PROCESSING_AA };
static void TrigLinseg_postprocessing_ireva(TrigLinseg *self) { POST_PROCESSING_IREVA };
static void TrigLinseg_postprocessing_areva(TrigLinseg *self) { POST_PROCESSING_AREVA };
static void TrigLinseg_postprocessing_revai(TrigLinseg *self) { POST_PROCESSING_REVAI };
static void TrigLinseg_postprocessing_revaa(TrigLinseg *self) { POST_PROCESSING_REVAA };
static void TrigLinseg_postprocessing_revareva(TrigLinseg *self) { POST_PROCESSING_REVAREVA };

static void
TrigLinseg_setProcMode(TrigLinseg *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_generate);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_postprocessing_revareva);
            break;
    }
}

static void
TrigLinseg_compute_next_data_frame(TrigLinseg *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigLinseg_traverse(TrigLinseg *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->pointslist);
    Py_VISIT(self->input);
    return 0;
}

static int
TrigLinseg_clear(TrigLinseg *self)
{
    pyo_CLEAR
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->input);
    return 0;
}

static void
TrigLinseg_dealloc(TrigLinseg* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->targets);
    PyMem_RawFree(self->times);
    PyMem_RawFree(self->trigsBuffer);
    TrigLinseg_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigLinseg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pointslist = NULL, *multmp = NULL, *addtmp = NULL;
    TrigLinseg *self;
    self = (TrigLinseg *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->newlist = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigLinseg_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigLinseg_setProcMode);

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"input", "list", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &pointslist, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    self->pointslist = pointslist;
    Py_INCREF(self->pointslist);
    TrigLinseg_convert_pointslist((TrigLinseg *)self);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

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

static PyObject * TrigLinseg_getServer(TrigLinseg* self) { GET_SERVER };
static PyObject * TrigLinseg_getStream(TrigLinseg* self) { GET_STREAM };
static PyObject * TrigLinseg_getTriggerStream(TrigLinseg* self) { GET_TRIGGER_STREAM };
static PyObject * TrigLinseg_setMul(TrigLinseg *self, PyObject *arg) { SET_MUL };
static PyObject * TrigLinseg_setAdd(TrigLinseg *self, PyObject *arg) { SET_ADD };
static PyObject * TrigLinseg_setSub(TrigLinseg *self, PyObject *arg) { SET_SUB };
static PyObject * TrigLinseg_setDiv(TrigLinseg *self, PyObject *arg) { SET_DIV };

static PyObject * TrigLinseg_play(TrigLinseg *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigLinseg_stop(TrigLinseg *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigLinseg_multiply(TrigLinseg *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigLinseg_inplace_multiply(TrigLinseg *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigLinseg_add(TrigLinseg *self, PyObject *arg) { ADD };
static PyObject * TrigLinseg_inplace_add(TrigLinseg *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigLinseg_sub(TrigLinseg *self, PyObject *arg) { SUB };
static PyObject * TrigLinseg_inplace_sub(TrigLinseg *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigLinseg_div(TrigLinseg *self, PyObject *arg) { DIV };
static PyObject * TrigLinseg_inplace_div(TrigLinseg *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigLinseg_setList(TrigLinseg *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The points list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    self->newlist = 1;

    Py_RETURN_NONE;
}

static PyMemberDef TrigLinseg_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigLinseg, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigLinseg, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(TrigLinseg, trig_stream), 0, "Trigger Stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(TrigLinseg, pointslist), 0, "List of target points."},
    {"mul", T_OBJECT_EX, offsetof(TrigLinseg, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigLinseg, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigLinseg_methods[] =
{
    {"getServer", (PyCFunction)TrigLinseg_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigLinseg_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)TrigLinseg_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)TrigLinseg_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigLinseg_stop, METH_VARARGS | METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"setList", (PyCFunction)TrigLinseg_setList, METH_O, "Sets target points list."},
    {"setMul", (PyCFunction)TrigLinseg_setMul, METH_O, "Sets TrigLinseg mul factor."},
    {"setAdd", (PyCFunction)TrigLinseg_setAdd, METH_O, "Sets TrigLinseg add factor."},
    {"setSub", (PyCFunction)TrigLinseg_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigLinseg_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigLinsegType_slots[] =
{
    {Py_tp_dealloc, TrigLinseg_dealloc},
    {Py_tp_doc, "TrigLinseg objects. Generates a linear segments break-points line."},
    {Py_tp_traverse, TrigLinseg_traverse},
    {Py_tp_clear, TrigLinseg_clear},
    {Py_tp_methods, TrigLinseg_methods},
    {Py_tp_members, TrigLinseg_members},
    {Py_nb_add, TrigLinseg_add},
    {Py_nb_subtract, TrigLinseg_sub},
    {Py_nb_multiply, TrigLinseg_multiply},
    {Py_nb_true_divide, TrigLinseg_div},
    {Py_nb_inplace_add, TrigLinseg_inplace_add},
    {Py_nb_inplace_subtract, TrigLinseg_inplace_sub},
    {Py_nb_inplace_multiply, TrigLinseg_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigLinseg_inplace_div},
    {Py_tp_new, TrigLinseg_new},
    {0, NULL}
};

static PyType_Spec TrigLinsegType_spec =
{
    "_pyo.TrigLinseg_base",
    sizeof(TrigLinseg),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigLinsegType_slots
};

PyTypeObject *
PyoCreateTrigLinsegType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigLinsegType_spec, NULL);
}

/*********************************************************************************************/
/* TrigExpseg *********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *pointslist;
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    double currentTime;
    double currentValue;
    MYFLT sampleToSec;
    double inc;
    double pointer;
    MYFLT range;
    double steps;
    MYFLT *targets;
    MYFLT *times;
    int which;
    int flag;
    int newlist;
    int listsize;
    double exp;
    double exp_tmp;
    int inverse;
    int inverse_tmp;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} TrigExpseg;

static void
TrigExpseg_convert_pointslist(TrigExpseg *self)
{
    int i;
    PyObject *tup;

    self->listsize = PyList_Size(self->pointslist);
    self->targets = (MYFLT *)PyMem_RawRealloc(self->targets, self->listsize * sizeof(MYFLT));
    self->times = (MYFLT *)PyMem_RawRealloc(self->times, self->listsize * sizeof(MYFLT));

    for (i = 0; i < self->listsize; i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        self->times[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
        self->targets[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
    }
}

static void
TrigExpseg_reinit(TrigExpseg *self)
{
    if (self->newlist == 1)
    {
        TrigExpseg_convert_pointslist((TrigExpseg *)self);
        self->newlist = 0;
    }

    self->currentTime = 0.0;
    self->currentValue = self->targets[0];
    self->which = 0;
    self->flag = 1;
    self->exp = self->exp_tmp;
    self->inverse = self->inverse_tmp;
}

static void
TrigExpseg_generate(TrigExpseg *self)
{
    int i;
    MYFLT scl;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] == 1)
            TrigExpseg_reinit((TrigExpseg *)self);

        if (self->flag == 1)
        {
            if (self->currentTime >= self->times[self->which])
            {
                self->which++;

                if (self->which == self->listsize)
                {
                    self->trigsBuffer[i] = 1.0;
                    self->flag = 0;
                    self->currentValue = self->targets[self->which - 1];
                }
                else
                {
                    self->range = self->targets[self->which] - self->targets[self->which - 1];
                    self->steps = (self->times[self->which] - self->times[self->which - 1]) * self->sr;

                    if (self->steps <= 0)
                        self->inc = 1.0;
                    else
                        self->inc = 1.0 / self->steps;

                    self->pointer = 0.0;
                }
            }

            if (self->currentTime <= self->times[self->listsize - 1])
            {
                if (self->pointer >= 1.0)
                    self->pointer = 1.0;

                if (self->inverse == 1 && self->range < 0.0)
                    scl = 1.0 - MYPOW(1.0 - self->pointer, self->exp);
                else
                    scl = MYPOW(self->pointer, self->exp);

                self->currentValue = scl * self->range + self->targets[self->which - 1];
                self->pointer += self->inc;
            }

            self->data[i] = (MYFLT)self->currentValue;
            self->currentTime += self->sampleToSec;
        }
        else
            self->data[i] = (MYFLT)self->currentValue;
    }
}

static void TrigExpseg_postprocessing_ii(TrigExpseg *self) { POST_PROCESSING_II };
static void TrigExpseg_postprocessing_ai(TrigExpseg *self) { POST_PROCESSING_AI };
static void TrigExpseg_postprocessing_ia(TrigExpseg *self) { POST_PROCESSING_IA };
static void TrigExpseg_postprocessing_aa(TrigExpseg *self) { POST_PROCESSING_AA };
static void TrigExpseg_postprocessing_ireva(TrigExpseg *self) { POST_PROCESSING_IREVA };
static void TrigExpseg_postprocessing_areva(TrigExpseg *self) { POST_PROCESSING_AREVA };
static void TrigExpseg_postprocessing_revai(TrigExpseg *self) { POST_PROCESSING_REVAI };
static void TrigExpseg_postprocessing_revaa(TrigExpseg *self) { POST_PROCESSING_REVAA };
static void TrigExpseg_postprocessing_revareva(TrigExpseg *self) { POST_PROCESSING_REVAREVA };

static void
TrigExpseg_setProcMode(TrigExpseg *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_generate);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_postprocessing_revareva);
            break;
    }
}

static void
TrigExpseg_compute_next_data_frame(TrigExpseg *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigExpseg_traverse(TrigExpseg *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->pointslist);
    Py_VISIT(self->input);
    return 0;
}

static int
TrigExpseg_clear(TrigExpseg *self)
{
    pyo_CLEAR
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->input);
    return 0;
}

static void
TrigExpseg_dealloc(TrigExpseg* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->targets);
    PyMem_RawFree(self->times);
    PyMem_RawFree(self->trigsBuffer);
    TrigExpseg_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigExpseg_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pointslist = NULL, *multmp = NULL, *addtmp = NULL;
    TrigExpseg *self;
    self = (TrigExpseg *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->newlist = 1;
    self->exp = self->exp_tmp = 10;
    self->inverse = self->inverse_tmp = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigExpseg_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigExpseg_setProcMode);

    self->sampleToSec = 1. / self->sr;

    static char *kwlist[] = {"input", "list", "exp", "inverse", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|diOO", kwlist, &inputtmp, &pointslist, &self->exp_tmp, &self->inverse_tmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    self->pointslist = pointslist;
    Py_INCREF(self->pointslist);
    TrigExpseg_convert_pointslist((TrigExpseg *)self);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

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

static PyObject * TrigExpseg_getServer(TrigExpseg* self) { GET_SERVER };
static PyObject * TrigExpseg_getStream(TrigExpseg* self) { GET_STREAM };
static PyObject * TrigExpseg_getTriggerStream(TrigExpseg* self) { GET_TRIGGER_STREAM };
static PyObject * TrigExpseg_setMul(TrigExpseg *self, PyObject *arg) { SET_MUL };
static PyObject * TrigExpseg_setAdd(TrigExpseg *self, PyObject *arg) { SET_ADD };
static PyObject * TrigExpseg_setSub(TrigExpseg *self, PyObject *arg) { SET_SUB };
static PyObject * TrigExpseg_setDiv(TrigExpseg *self, PyObject *arg) { SET_DIV };

static PyObject * TrigExpseg_play(TrigExpseg *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigExpseg_stop(TrigExpseg *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigExpseg_multiply(TrigExpseg *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigExpseg_inplace_multiply(TrigExpseg *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigExpseg_add(TrigExpseg *self, PyObject *arg) { ADD };
static PyObject * TrigExpseg_inplace_add(TrigExpseg *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigExpseg_sub(TrigExpseg *self, PyObject *arg) { SUB };
static PyObject * TrigExpseg_inplace_sub(TrigExpseg *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigExpseg_div(TrigExpseg *self, PyObject *arg) { DIV };
static PyObject * TrigExpseg_inplace_div(TrigExpseg *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigExpseg_setList(TrigExpseg *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The points list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    self->newlist = 1;

    Py_RETURN_NONE;
}

static PyObject *
TrigExpseg_setExp(TrigExpseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    // TODO: PyNumber_Check()
    self->exp_tmp = PyFloat_AsDouble(arg);

    Py_RETURN_NONE;
}

static PyObject *
TrigExpseg_setInverse(TrigExpseg *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->inverse_tmp = PyLong_AsLong(PyNumber_Long(arg));

    Py_RETURN_NONE;
}

static PyMemberDef TrigExpseg_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigExpseg, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigExpseg, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(TrigExpseg, trig_stream), 0, "Trigger Stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(TrigExpseg, pointslist), 0, "List of target points."},
    {"mul", T_OBJECT_EX, offsetof(TrigExpseg, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigExpseg, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigExpseg_methods[] =
{
    {"getServer", (PyCFunction)TrigExpseg_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigExpseg_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)TrigExpseg_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)TrigExpseg_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigExpseg_stop, METH_VARARGS | METH_KEYWORDS, "Starts fadeout and stops computing."},
    {"setList", (PyCFunction)TrigExpseg_setList, METH_O, "Sets target points list."},
    {"setExp", (PyCFunction)TrigExpseg_setExp, METH_O, "Sets exponent factor."},
    {"setInverse", (PyCFunction)TrigExpseg_setInverse, METH_O, "Sets inverse factor."},
    {"setMul", (PyCFunction)TrigExpseg_setMul, METH_O, "Sets TrigExpseg mul factor."},
    {"setAdd", (PyCFunction)TrigExpseg_setAdd, METH_O, "Sets TrigExpseg add factor."},
    {"setSub", (PyCFunction)TrigExpseg_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigExpseg_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigExpsegType_slots[] =
{
    {Py_tp_dealloc, TrigExpseg_dealloc},
    {Py_tp_doc, "TrigExpseg objects. Generates a linear segments break-points line."},
    {Py_tp_traverse, TrigExpseg_traverse},
    {Py_tp_clear, TrigExpseg_clear},
    {Py_tp_methods, TrigExpseg_methods},
    {Py_tp_members, TrigExpseg_members},
    {Py_nb_add, TrigExpseg_add},
    {Py_nb_subtract, TrigExpseg_sub},
    {Py_nb_multiply, TrigExpseg_multiply},
    {Py_nb_true_divide, TrigExpseg_div},
    {Py_nb_inplace_add, TrigExpseg_inplace_add},
    {Py_nb_inplace_subtract, TrigExpseg_inplace_sub},
    {Py_nb_inplace_multiply, TrigExpseg_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigExpseg_inplace_div},
    {Py_tp_new, TrigExpseg_new},
    {0, NULL}
};

static PyType_Spec TrigExpsegType_spec =
{
    "_pyo.TrigExpseg_base",
    sizeof(TrigExpseg),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigExpsegType_slots
};

PyTypeObject *
PyoCreateTrigExpsegType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigExpsegType_spec, NULL);
}

/****************/
/**** TrigXnoise *****/
/****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *x1;
    PyObject *x2;
    Stream *x1_stream;
    Stream *x2_stream;
    MYFLT (*type_func_ptr)(void *);
    MYFLT xx1;
    MYFLT xx2;
    int type;
    MYFLT value;
    MYFLT lastPoissonX1;
    int poisson_tab;
    MYFLT poisson_buffer[2000];
    MYFLT walkerValue;
    MYFLT loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[4]; // need at least 2 slots for mul & add
} TrigXnoise;

// no parameter
static MYFLT
TrigXnoise_uniform(TrigXnoise *self)
{
    return RANDOM_UNIFORM;
}

static MYFLT
TrigXnoise_linear_min(TrigXnoise *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;

    if (a < b) return a;
    else return b;
}

static MYFLT
TrigXnoise_linear_max(TrigXnoise *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;

    if (a > b) return a;
    else return b;
}

static MYFLT
TrigXnoise_triangle(TrigXnoise *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
TrigXnoise_expon_min(TrigXnoise *self)
{
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT val = -MYLOG(RANDOM_UNIFORM) / self->xx1;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
TrigXnoise_expon_max(TrigXnoise *self)
{
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT val = 1.0 - (-MYLOG(RANDOM_UNIFORM) / self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
TrigXnoise_biexpon(TrigXnoise *self)
{
    MYFLT polar, val;

    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0)
    {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG(sum) / self->xx1) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
TrigXnoise_cauchy(TrigXnoise *self)
{
    MYFLT rnd, val, dir;

    do
    {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (pyorand() < (PYO_RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->xx1 * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static MYFLT
TrigXnoise_weibull(TrigXnoise *self)
{
    MYFLT rnd, val;

    if (self->xx2 <= 0.0) self->xx2 = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * MYPOW(MYLOG(rnd), (1.0 / self->xx2));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static MYFLT
TrigXnoise_gaussian(TrigXnoise *self)
{
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static MYFLT
TrigXnoise_poisson(TrigXnoise *self)
{
    int i, j, factorial;
    long tot;
    MYFLT val;

    if (self->xx1 < 0.1) self->xx1 = 0.1;

    if (self->xx2 < 0.1) self->xx2 = 0.1;

    if (self->xx1 != self->lastPoissonX1)
    {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;

        for (i = 1; i < 12; i++)
        {
            factorial *= i;
            tot = (long)(1000.0 * (MYPOW(2.7182818, -self->xx1) * MYPOW(self->xx1, i) / factorial));

            for (j = 0; j < tot; j++)
            {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }

    val = self->poisson_buffer[pyorand() % self->poisson_tab] / 12.0 * self->xx2;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static MYFLT
TrigXnoise_walker(TrigXnoise *self)
{
    unsigned int modulo, dir;

    if (self->xx2 < 0.002) self->xx2 = 0.002;

    modulo = (unsigned int)(self->xx2 * 1000.0);
    dir = pyorand() % 100;

    if (dir < 50)
        self->walkerValue += ((MYFLT)(pyorand() % modulo) * 0.001);
    else
        self->walkerValue -= ((MYFLT)(pyorand() % modulo) * 0.001);

    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    else if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;

    return self->walkerValue;
}

// x1 = max value, x2 = max step
static MYFLT
TrigXnoise_loopseg(TrigXnoise *self)
{
    unsigned int modulo, dir;

    if (self->loopChoice == 0)
    {

        self->loopCountPlay = self->loopTime = 0;

        if (self->xx2 < 0.002) self->xx2 = 0.002;

        modulo = (unsigned int)(self->xx2 * 1000.0);
        dir = pyorand() % 100;

        if (dir < 50)
            self->walkerValue += ((MYFLT)(pyorand() % modulo) * 0.001);
        else
            self->walkerValue -= ((MYFLT)(pyorand() % modulo) * 0.001);

        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        else if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;

        self->loop_buffer[self->loopCountRec++] = self->walkerValue;

        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else
        {
            self->loopChoice = 1;
            self->loopStop = (pyorand() % 4) + 1;
        }
    }
    else
    {
        self->loopCountRec = 0;

        self->walkerValue = self->loop_buffer[self->loopCountPlay++];

        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else
        {
            self->loopCountPlay = 0;
            self->loopTime++;
        }

        if (self->loopTime == self->loopStop)
        {
            self->loopChoice = 0;
            self->loopLen = (pyorand() % 10) + 3;
        }
    }

    return self->walkerValue;
}

static void
TrigXnoise_generate_ii(TrigXnoise *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
            self->value = (*self->type_func_ptr)(self);

        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_ai(TrigXnoise *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
        }

        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_ia(TrigXnoise *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }

        self->data[i] = self->value;
    }
}

static void
TrigXnoise_generate_aa(TrigXnoise *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
        }

        self->data[i] = self->value;
    }
}

static void TrigXnoise_postprocessing_ii(TrigXnoise *self) { POST_PROCESSING_II };
static void TrigXnoise_postprocessing_ai(TrigXnoise *self) { POST_PROCESSING_AI };
static void TrigXnoise_postprocessing_ia(TrigXnoise *self) { POST_PROCESSING_IA };
static void TrigXnoise_postprocessing_aa(TrigXnoise *self) { POST_PROCESSING_AA };
static void TrigXnoise_postprocessing_ireva(TrigXnoise *self) { POST_PROCESSING_IREVA };
static void TrigXnoise_postprocessing_areva(TrigXnoise *self) { POST_PROCESSING_AREVA };
static void TrigXnoise_postprocessing_revai(TrigXnoise *self) { POST_PROCESSING_REVAI };
static void TrigXnoise_postprocessing_revaa(TrigXnoise *self) { POST_PROCESSING_REVAA };
static void TrigXnoise_postprocessing_revareva(TrigXnoise *self) { POST_PROCESSING_REVAREVA };

static void
TrigXnoise_setRandomType(TrigXnoise *self)
{

    switch (self->type)
    {
        case 0:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_uniform);
            break;

        case 1:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_linear_min);
            break;

        case 2:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_linear_max);
            break;

        case 3:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_triangle);
            break;

        case 4:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_expon_min);
            break;

        case 5:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_expon_max);
            break;

        case 6:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_biexpon);
            break;

        case 7:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_cauchy);
            break;

        case 8:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_weibull);
            break;

        case 9:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_gaussian);
            break;

        case 10:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_poisson);
            break;

        case 11:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_walker);
            break;

        case 12:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoise_loopseg);
            break;
    }
}

static void
TrigXnoise_setProcMode(TrigXnoise *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_generate_ii);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_generate_ai);
            break;

        case 10:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_generate_ia);
            break;

        case 11:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_generate_aa);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_postprocessing_revareva);
            break;
    }
}

static void
TrigXnoise_compute_next_data_frame(TrigXnoise *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigXnoise_traverse(TrigXnoise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->x1);
    Py_VISIT(self->x2);
    return 0;
}

static int
TrigXnoise_clear(TrigXnoise *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->x1);
    Py_CLEAR(self->x2);
    return 0;
}

static void
TrigXnoise_dealloc(TrigXnoise* self)
{
    pyo_DEALLOC
    TrigXnoise_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigXnoise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *x1tmp = NULL, *x2tmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigXnoise *self;
    self = (TrigXnoise *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    Server_generateSeed((Server *)self->server, TRIGXNOISE_ID);

    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;

    for (i = 0; i < 2000; i++)
    {
        self->poisson_buffer[i] = 0.0;
    }

    for (i = 0; i < 15; i++)
    {
        self->loop_buffer[i] = 0.0;
    }

    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;
    self->loopLen = (pyorand() % 10) + 3;

    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigXnoise_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoise_setProcMode);

    static char *kwlist[] = {"input", "type", "x1", "x2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOOOO", kwlist, &inputtmp, &self->type, &x1tmp, &x2tmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (x1tmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setX1", x1tmp);
    }

    if (x2tmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setX2", x2tmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    TrigXnoise_setRandomType(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigXnoise_getServer(TrigXnoise* self) { GET_SERVER };
static PyObject * TrigXnoise_getStream(TrigXnoise* self) { GET_STREAM };
static PyObject * TrigXnoise_setMul(TrigXnoise *self, PyObject *arg) { SET_MUL };
static PyObject * TrigXnoise_setAdd(TrigXnoise *self, PyObject *arg) { SET_ADD };
static PyObject * TrigXnoise_setSub(TrigXnoise *self, PyObject *arg) { SET_SUB };
static PyObject * TrigXnoise_setDiv(TrigXnoise *self, PyObject *arg) { SET_DIV };

static PyObject * TrigXnoise_play(TrigXnoise *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigXnoise_out(TrigXnoise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigXnoise_stop(TrigXnoise *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigXnoise_multiply(TrigXnoise *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigXnoise_inplace_multiply(TrigXnoise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigXnoise_add(TrigXnoise *self, PyObject *arg) { ADD };
static PyObject * TrigXnoise_inplace_add(TrigXnoise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigXnoise_sub(TrigXnoise *self, PyObject *arg) { SUB };
static PyObject * TrigXnoise_inplace_sub(TrigXnoise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigXnoise_div(TrigXnoise *self, PyObject *arg) { DIV };
static PyObject * TrigXnoise_inplace_div(TrigXnoise *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigXnoise_setType(TrigXnoise *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->type = PyLong_AsLong(arg);
        TrigXnoise_setRandomType(self);
    }

    Py_RETURN_NONE;
}

static PyObject * TrigXnoise_setX1(TrigXnoise *self, PyObject *arg) { SET_PARAM(self->x1, self->x1_stream, 2); }
static PyObject * TrigXnoise_setX2(TrigXnoise *self, PyObject *arg) { SET_PARAM(self->x2, self->x2_stream, 3); }

static PyMemberDef TrigXnoise_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigXnoise, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigXnoise, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigXnoise, input), 0, "Trigger input."},
    {"x1", T_OBJECT_EX, offsetof(TrigXnoise, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(TrigXnoise, x2), 0, "second param."},
    {"mul", T_OBJECT_EX, offsetof(TrigXnoise, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigXnoise, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigXnoise_methods[] =
{
    {"getServer", (PyCFunction)TrigXnoise_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigXnoise_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigXnoise_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigXnoise_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigXnoise_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setType", (PyCFunction)TrigXnoise_setType, METH_O, "Sets distribution type."},
    {"setX1", (PyCFunction)TrigXnoise_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)TrigXnoise_setX2, METH_O, "Sets second param."},
    {"setMul", (PyCFunction)TrigXnoise_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigXnoise_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigXnoise_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigXnoise_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigXnoiseType_slots[] =
{
    {Py_tp_dealloc, TrigXnoise_dealloc},
    {Py_tp_doc, "TrigXnoise objects. Periodically generates a new random value."},
    {Py_tp_traverse, TrigXnoise_traverse},
    {Py_tp_clear, TrigXnoise_clear},
    {Py_tp_methods, TrigXnoise_methods},
    {Py_tp_members, TrigXnoise_members},
    {Py_nb_add, TrigXnoise_add},
    {Py_nb_subtract, TrigXnoise_sub},
    {Py_nb_multiply, TrigXnoise_multiply},
    {Py_nb_true_divide, TrigXnoise_div},
    {Py_nb_inplace_add, TrigXnoise_inplace_add},
    {Py_nb_inplace_subtract, TrigXnoise_inplace_sub},
    {Py_nb_inplace_multiply, TrigXnoise_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigXnoise_inplace_div},
    {Py_tp_new, TrigXnoise_new},
    {0, NULL}
};

static PyType_Spec TrigXnoiseType_spec =
{
    "_pyo.TrigXnoise_base",
    sizeof(TrigXnoise),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigXnoiseType_slots
};

PyTypeObject *
PyoCreateTrigXnoiseType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigXnoiseType_spec, NULL);
}

/****************/
/**** TrigXnoiseMidi *****/
/****************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *x1;
    PyObject *x2;
    Stream *x1_stream;
    Stream *x2_stream;
    MYFLT (*type_func_ptr)(void *);
    int scale; // 0 = Midi, 1 = frequency, 2 = transpo
    int range_min;
    int range_max;
    int centralkey;
    MYFLT xx1;
    MYFLT xx2;
    int type;
    MYFLT value;
    MYFLT lastPoissonX1;
    int poisson_tab;
    MYFLT poisson_buffer[2000];
    MYFLT walkerValue;
    MYFLT loop_buffer[15];
    int loopChoice;
    int loopCountPlay;
    int loopTime;
    int loopCountRec;
    int loopLen;
    int loopStop;
    int modebuffer[4]; // need at least 2 slots for mul & add
} TrigXnoiseMidi;

static MYFLT
TrigXnoiseMidi_convert(TrigXnoiseMidi *self)
{
    int midival;
    MYFLT val;

    midival = (int)((self->value * (self->range_max - self->range_min)) + self->range_min);

    if (midival < 0)
        midival = 0;
    else if (midival > 127)
        midival = 127;

    if (self->scale == 0)
        val = (MYFLT)midival;
    else if (self->scale == 1)
        val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
    else if (self->scale == 2)
        val = MYPOW(1.0594630943593, midival - self->centralkey);
    else
        val = midival;

    return val;
}


// no parameter
static MYFLT
TrigXnoiseMidi_uniform(TrigXnoiseMidi *self)
{
    return RANDOM_UNIFORM;
}

static MYFLT
TrigXnoiseMidi_linear_min(TrigXnoiseMidi *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;

    if (a < b) return a;
    else return b;
}

static MYFLT
TrigXnoiseMidi_linear_max(TrigXnoiseMidi *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;

    if (a > b) return a;
    else return b;
}

static MYFLT
TrigXnoiseMidi_triangle(TrigXnoiseMidi *self)
{
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
TrigXnoiseMidi_expon_min(TrigXnoiseMidi *self)
{
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT val = -MYLOG10(RANDOM_UNIFORM) / self->xx1;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
TrigXnoiseMidi_expon_max(TrigXnoiseMidi *self)
{
    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT val = 1.0 - (-MYLOG10(RANDOM_UNIFORM) / self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
TrigXnoiseMidi_biexpon(TrigXnoiseMidi *self)
{
    MYFLT polar, val;

    if (self->xx1 <= 0.0) self->xx1 = 0.00001;

    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0)
    {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG10(sum) / self->xx1) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
TrigXnoiseMidi_cauchy(TrigXnoiseMidi *self)
{
    MYFLT rnd, val, dir;

    do
    {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (pyorand() < (PYO_RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->xx1 * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = shape
static MYFLT
TrigXnoiseMidi_weibull(TrigXnoiseMidi *self)
{
    MYFLT rnd, val;

    if (self->xx2 <= 0.0) self->xx2 = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = self->xx1 * MYPOW(MYLOG(rnd), (1.0 / self->xx2));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = locator, x2 = bandwidth
static MYFLT
TrigXnoiseMidi_gaussian(TrigXnoiseMidi *self)
{
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->xx2 * (rnd - 3.0) * 0.33 + self->xx1);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = gravity center, x2 = compress/expand
static MYFLT
TrigXnoiseMidi_poisson(TrigXnoiseMidi *self)
{
    int i, j, factorial;
    long tot;
    MYFLT val;

    if (self->xx1 < 0.1) self->xx1 = 0.1;

    if (self->xx2 < 0.1) self->xx2 = 0.1;

    if (self->xx1 != self->lastPoissonX1)
    {
        self->lastPoissonX1 = self->xx1;
        self->poisson_tab = 0;
        factorial = 1;

        for (i = 1; i < 12; i++)
        {
            factorial *= i;
            tot = (long)(1000.0 * (MYPOW(2.7182818, -self->xx1) * MYPOW(self->xx1, i) / factorial));

            for (j = 0; j < tot; j++)
            {
                self->poisson_buffer[self->poisson_tab] = i;
                self->poisson_tab++;
            }
        }
    }

    val = self->poisson_buffer[pyorand() % self->poisson_tab] / 12.0 * self->xx2;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = max value, x2 = max step
static MYFLT
TrigXnoiseMidi_walker(TrigXnoiseMidi *self)
{
    unsigned int modulo, dir;

    if (self->xx2 < 0.002) self->xx2 = 0.002;

    modulo = (unsigned int)(self->xx2 * 1000.0);
    dir = pyorand() % 100;

    if (dir < 50)
        self->walkerValue += ((MYFLT)(pyorand() % modulo) * 0.001);
    else
        self->walkerValue -= ((MYFLT)(pyorand() % modulo) * 0.001);

    if (self->walkerValue > self->xx1)
        self->walkerValue = self->xx1;
    else if (self->walkerValue < 0.0)
        self->walkerValue = 0.0;

    return self->walkerValue;
}

// x1 = max value, x2 = max step
static MYFLT
TrigXnoiseMidi_loopseg(TrigXnoiseMidi *self)
{
    unsigned int modulo, dir;

    if (self->loopChoice == 0)
    {

        self->loopCountPlay = self->loopTime = 0;

        if (self->xx2 < 0.002) self->xx2 = 0.002;

        modulo = (unsigned int)(self->xx2 * 1000.0);
        dir = pyorand() % 100;

        if (dir < 50)
            self->walkerValue += ((MYFLT)(pyorand() % modulo) * 0.001);
        else
            self->walkerValue -= ((MYFLT)(pyorand() % modulo) * 0.001);

        if (self->walkerValue > self->xx1)
            self->walkerValue = self->xx1;
        else if (self->walkerValue < 0.0)
            self->walkerValue = 0.0;

        self->loop_buffer[self->loopCountRec++] = self->walkerValue;

        if (self->loopCountRec < self->loopLen)
            self->loopChoice = 0;
        else
        {
            self->loopChoice = 1;
            self->loopStop = (pyorand() % 4) + 1;
        }
    }
    else
    {
        self->loopCountRec = 0;

        self->walkerValue = self->loop_buffer[self->loopCountPlay++];

        if (self->loopCountPlay < self->loopLen)
            self->loopChoice = 1;
        else
        {
            self->loopCountPlay = 0;
            self->loopTime++;
        }

        if (self->loopTime == self->loopStop)
        {
            self->loopChoice = 0;
            self->loopLen = (pyorand() % 10) + 3;
        }
    }

    return self->walkerValue;
}

static void
TrigXnoiseMidi_generate_ii(TrigXnoiseMidi *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }

        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_ai(TrigXnoiseMidi *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    self->xx2 = PyFloat_AS_DOUBLE(self->x2);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx1 = x1[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }

        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_ia(TrigXnoiseMidi *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    self->xx1 = PyFloat_AS_DOUBLE(self->x1);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }

        self->data[i] = self->value;
    }
}

static void
TrigXnoiseMidi_generate_aa(TrigXnoiseMidi *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *x1 = Stream_getData((Stream *)self->x1_stream);
    MYFLT *x2 = Stream_getData((Stream *)self->x2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->xx1 = x1[i];
            self->xx2 = x2[i];
            self->value = (*self->type_func_ptr)(self);
            self->value = TrigXnoiseMidi_convert(self);
        }

        self->data[i] = self->value;
    }
}

static void TrigXnoiseMidi_postprocessing_ii(TrigXnoiseMidi *self) { POST_PROCESSING_II };
static void TrigXnoiseMidi_postprocessing_ai(TrigXnoiseMidi *self) { POST_PROCESSING_AI };
static void TrigXnoiseMidi_postprocessing_ia(TrigXnoiseMidi *self) { POST_PROCESSING_IA };
static void TrigXnoiseMidi_postprocessing_aa(TrigXnoiseMidi *self) { POST_PROCESSING_AA };
static void TrigXnoiseMidi_postprocessing_ireva(TrigXnoiseMidi *self) { POST_PROCESSING_IREVA };
static void TrigXnoiseMidi_postprocessing_areva(TrigXnoiseMidi *self) { POST_PROCESSING_AREVA };
static void TrigXnoiseMidi_postprocessing_revai(TrigXnoiseMidi *self) { POST_PROCESSING_REVAI };
static void TrigXnoiseMidi_postprocessing_revaa(TrigXnoiseMidi *self) { POST_PROCESSING_REVAA };
static void TrigXnoiseMidi_postprocessing_revareva(TrigXnoiseMidi *self) { POST_PROCESSING_REVAREVA };

static void
TrigXnoiseMidi_setRandomType(TrigXnoiseMidi *self)
{

    switch (self->type)
    {
        case 0:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_uniform);
            break;

        case 1:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_linear_min);
            break;

        case 2:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_linear_max);
            break;

        case 3:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_triangle);
            break;

        case 4:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_expon_min);
            break;

        case 5:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_expon_max);
            break;

        case 6:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_biexpon);
            break;

        case 7:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_cauchy);
            break;

        case 8:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_weibull);
            break;

        case 9:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_gaussian);
            break;

        case 10:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_poisson);
            break;

        case 11:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_walker);
            break;

        case 12:
            self->type_func_ptr = PYO_MYFLT_CALLBACK(TrigXnoiseMidi_loopseg);
            break;
    }
}

static void
TrigXnoiseMidi_setProcMode(TrigXnoiseMidi *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_generate_ii);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_generate_ai);
            break;

        case 10:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_generate_ia);
            break;

        case 11:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_generate_aa);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_postprocessing_revareva);
            break;
    }
}

static void
TrigXnoiseMidi_compute_next_data_frame(TrigXnoiseMidi *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigXnoiseMidi_traverse(TrigXnoiseMidi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->x1);
    Py_VISIT(self->x2);
    return 0;
}

static int
TrigXnoiseMidi_clear(TrigXnoiseMidi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->x1);
    Py_CLEAR(self->x2);
    return 0;
}

static void
TrigXnoiseMidi_dealloc(TrigXnoiseMidi* self)
{
    pyo_DEALLOC
    TrigXnoiseMidi_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigXnoiseMidi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *x1tmp = NULL, *x2tmp = NULL, *rangetmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigXnoiseMidi *self;
    self = (TrigXnoiseMidi *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->x1 = PyFloat_FromDouble(0.5);
    self->x2 = PyFloat_FromDouble(0.5);
    self->xx1 = self->xx2 = self->walkerValue = 0.5;
    self->value = 0.0;
    self->scale = 0;
    self->range_min = 0;
    self->range_max = 127;
    self->centralkey = 64;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON

    Server_generateSeed((Server *)self->server, TRIGXNOISEMIDI_ID);

    self->poisson_tab = 0;
    self->lastPoissonX1 = -99.0;

    for (i = 0; i < 2000; i++)
    {
        self->poisson_buffer[i] = 0.0;
    }

    for (i = 0; i < 15; i++)
    {
        self->loop_buffer[i] = 0.0;
    }

    self->loopChoice = self->loopCountPlay = self->loopTime = self->loopCountRec = self->loopStop = 0;
    self->loopLen = (pyorand() % 10) + 3;

    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigXnoiseMidi_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigXnoiseMidi_setProcMode);

    static char *kwlist[] = {"input", "type", "x1", "x2", "scale", "range", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOOiOOO", kwlist, &inputtmp, &self->type, &x1tmp, &x2tmp, &self->scale, &rangetmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (x1tmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setX1", x1tmp);
    }

    if (x2tmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setX2", x2tmp);
    }

    if (rangetmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setRange", rangetmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    TrigXnoiseMidi_setRandomType(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigXnoiseMidi_getServer(TrigXnoiseMidi* self) { GET_SERVER };
static PyObject * TrigXnoiseMidi_getStream(TrigXnoiseMidi* self) { GET_STREAM };
static PyObject * TrigXnoiseMidi_setMul(TrigXnoiseMidi *self, PyObject *arg) { SET_MUL };
static PyObject * TrigXnoiseMidi_setAdd(TrigXnoiseMidi *self, PyObject *arg) { SET_ADD };
static PyObject * TrigXnoiseMidi_setSub(TrigXnoiseMidi *self, PyObject *arg) { SET_SUB };
static PyObject * TrigXnoiseMidi_setDiv(TrigXnoiseMidi *self, PyObject *arg) { SET_DIV };

static PyObject * TrigXnoiseMidi_play(TrigXnoiseMidi *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigXnoiseMidi_out(TrigXnoiseMidi *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigXnoiseMidi_stop(TrigXnoiseMidi *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigXnoiseMidi_multiply(TrigXnoiseMidi *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigXnoiseMidi_inplace_multiply(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigXnoiseMidi_add(TrigXnoiseMidi *self, PyObject *arg) { ADD };
static PyObject * TrigXnoiseMidi_inplace_add(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigXnoiseMidi_sub(TrigXnoiseMidi *self, PyObject *arg) { SUB };
static PyObject * TrigXnoiseMidi_inplace_sub(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigXnoiseMidi_div(TrigXnoiseMidi *self, PyObject *arg) { DIV };
static PyObject * TrigXnoiseMidi_inplace_div(TrigXnoiseMidi *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigXnoiseMidi_setType(TrigXnoiseMidi *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->type = PyLong_AsLong(arg);
        TrigXnoiseMidi_setRandomType(self);
    }

    Py_RETURN_NONE;
}

static PyObject *
TrigXnoiseMidi_setScale(TrigXnoiseMidi *self, PyObject *arg)
{
    int tmp;

    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp >= 0 && tmp <= 2)
            self->scale = tmp;
        else
            PySys_WriteStdout("TrigXnoiseMidi: scale attribute must be an integer {0, 1, 2}\n");
    }

    Py_RETURN_NONE;
}

static PyObject *
TrigXnoiseMidi_setRange(TrigXnoiseMidi *self, PyObject *args)
{
    if (args == NULL)
    {
        Py_RETURN_NONE;
    }

    int isTuple = PyTuple_Check(args);

    if (isTuple == 1)
    {
        self->range_min = PyLong_AsLong(PyTuple_GET_ITEM(args, 0));
        self->range_max = PyLong_AsLong(PyTuple_GET_ITEM(args, 1));
        self->centralkey = (int)((self->range_max + self->range_min) / 2);
    }

    Py_RETURN_NONE;
}

static PyObject * TrigXnoiseMidi_setX1(TrigXnoiseMidi *self, PyObject *arg) { SET_PARAM(self->x1, self->x1_stream, 2); }
static PyObject * TrigXnoiseMidi_setX2(TrigXnoiseMidi *self, PyObject *arg) { SET_PARAM(self->x2, self->x2_stream, 3); }

static PyMemberDef TrigXnoiseMidi_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigXnoiseMidi, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigXnoiseMidi, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigXnoiseMidi, input), 0, "Trigger input."},
    {"x1", T_OBJECT_EX, offsetof(TrigXnoiseMidi, x1), 0, "first param."},
    {"x2", T_OBJECT_EX, offsetof(TrigXnoiseMidi, x2), 0, "second param."},
    {"mul", T_OBJECT_EX, offsetof(TrigXnoiseMidi, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigXnoiseMidi, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigXnoiseMidi_methods[] =
{
    {"getServer", (PyCFunction)TrigXnoiseMidi_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigXnoiseMidi_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigXnoiseMidi_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigXnoiseMidi_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigXnoiseMidi_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setType", (PyCFunction)TrigXnoiseMidi_setType, METH_O, "Sets distribution type."},
    {"setScale", (PyCFunction)TrigXnoiseMidi_setScale, METH_O, "Sets output scale."},
    {"setRange", (PyCFunction)TrigXnoiseMidi_setRange, METH_VARARGS, "Sets range in midi notes (min, max)."},
    {"setX1", (PyCFunction)TrigXnoiseMidi_setX1, METH_O, "Sets first param."},
    {"setX2", (PyCFunction)TrigXnoiseMidi_setX2, METH_O, "Sets second param."},
    {"setMul", (PyCFunction)TrigXnoiseMidi_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigXnoiseMidi_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigXnoiseMidi_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigXnoiseMidi_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigXnoiseMidiType_slots[] =
{
    {Py_tp_dealloc, TrigXnoiseMidi_dealloc},
    {Py_tp_doc, "TrigXnoiseMidi objects. Periodically generates a new random value."},
    {Py_tp_traverse, TrigXnoiseMidi_traverse},
    {Py_tp_clear, TrigXnoiseMidi_clear},
    {Py_tp_methods, TrigXnoiseMidi_methods},
    {Py_tp_members, TrigXnoiseMidi_members},
    {Py_nb_add, TrigXnoiseMidi_add},
    {Py_nb_subtract, TrigXnoiseMidi_sub},
    {Py_nb_multiply, TrigXnoiseMidi_multiply},
    {Py_nb_true_divide, TrigXnoiseMidi_div},
    {Py_nb_inplace_add, TrigXnoiseMidi_inplace_add},
    {Py_nb_inplace_subtract, TrigXnoiseMidi_inplace_sub},
    {Py_nb_inplace_multiply, TrigXnoiseMidi_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigXnoiseMidi_inplace_div},
    {Py_tp_new, TrigXnoiseMidi_new},
    {0, NULL}
};

static PyType_Spec TrigXnoiseMidiType_spec =
{
    "_pyo.TrigXnoiseMidi_base",
    sizeof(TrigXnoiseMidi),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigXnoiseMidiType_slots
};

PyTypeObject *
PyoCreateTrigXnoiseMidiType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigXnoiseMidiType_spec, NULL);
}

/***************************************************/
/******* Counter ***********/
/***************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    long tmp;
    long min;
    long max;
    int dir;
    int direction;
    MYFLT value;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Counter;

static void
Counter_generates(Counter *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->value = (MYFLT)self->tmp;

            if (self->dir == 0)
            {
                self->tmp++;

                if (self->tmp >= self->max)
                    self->tmp = self->min;
            }
            else if (self->dir == 1)
            {
                self->tmp--;

                if (self->tmp < self->min)
                    self->tmp = self->max - 1;
            }
            else if (self->dir == 2)
            {
                self->tmp = self->tmp + self->direction;

                if (self->tmp >= self->max)
                {
                    self->direction = -1;
                    self->tmp = self->max - 2;
                }

                if (self->tmp <= self->min)
                {
                    self->direction = 1;
                    self->tmp = self->min;
                }
            }
        }

        self->data[i] = self->value;
    }
}

static void Counter_postprocessing_ii(Counter *self) { POST_PROCESSING_II };
static void Counter_postprocessing_ai(Counter *self) { POST_PROCESSING_AI };
static void Counter_postprocessing_ia(Counter *self) { POST_PROCESSING_IA };
static void Counter_postprocessing_aa(Counter *self) { POST_PROCESSING_AA };
static void Counter_postprocessing_ireva(Counter *self) { POST_PROCESSING_IREVA };
static void Counter_postprocessing_areva(Counter *self) { POST_PROCESSING_AREVA };
static void Counter_postprocessing_revai(Counter *self) { POST_PROCESSING_REVAI };
static void Counter_postprocessing_revaa(Counter *self) { POST_PROCESSING_REVAA };
static void Counter_postprocessing_revareva(Counter *self) { POST_PROCESSING_REVAREVA };

static void
Counter_setProcMode(Counter *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Counter_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Counter_postprocessing_revareva);
            break;
    }
}

static void
Counter_compute_next_data_frame(Counter *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Counter_traverse(Counter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Counter_clear(Counter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Counter_dealloc(Counter* self)
{
    pyo_DEALLOC
    Counter_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Counter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Counter *self;
    self = (Counter *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->min = 0;
    self->max = 100;
    self->dir = 0;
    self->direction = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Counter_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Counter_setProcMode);

    static char *kwlist[] = {"input", "min", "max", "dir", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|lliOO", kwlist, &inputtmp, &self->min, &self->max, &self->dir, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (self->dir == 0 || self->dir == 2)
        self->tmp = self->min;
    else
        self->tmp = self->max - 1;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Counter_getServer(Counter* self) { GET_SERVER };
static PyObject * Counter_getStream(Counter* self) { GET_STREAM };
static PyObject * Counter_setMul(Counter *self, PyObject *arg) { SET_MUL };
static PyObject * Counter_setAdd(Counter *self, PyObject *arg) { SET_ADD };
static PyObject * Counter_setSub(Counter *self, PyObject *arg) { SET_SUB };
static PyObject * Counter_setDiv(Counter *self, PyObject *arg) { SET_DIV };

static PyObject * Counter_play(Counter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Counter_stop(Counter *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Counter_multiply(Counter *self, PyObject *arg) { MULTIPLY };
static PyObject * Counter_inplace_multiply(Counter *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Counter_add(Counter *self, PyObject *arg) { ADD };
static PyObject * Counter_inplace_add(Counter *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Counter_sub(Counter *self, PyObject *arg) { SUB };
static PyObject * Counter_inplace_sub(Counter *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Counter_div(Counter *self, PyObject *arg) { DIV };
static PyObject * Counter_inplace_div(Counter *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Counter_setMin(Counter *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->min = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Counter_setMax(Counter *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->max = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Counter_setDir(Counter *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->dir = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Counter_reset(Counter *self, PyObject *arg)
{
    int val;

    if (arg == Py_None)
    {
        if (self->dir == 0 || self->dir == 2)
            val = self->min;
        else
            val = self->max - 1;

        self->tmp = val;
    }

    else if (PyLong_Check(arg))
    {
        val = PyLong_AsLong(arg);
        self->tmp = val;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Counter_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Counter, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Counter, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Counter, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Counter, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Counter, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Counter_methods[] =
{
    {"getServer", (PyCFunction)Counter_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Counter_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Counter_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Counter_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMin", (PyCFunction)Counter_setMin, METH_O, "Sets minimum value."},
    {"setMax", (PyCFunction)Counter_setMax, METH_O, "Sets maximum value."},
    {"setDir", (PyCFunction)Counter_setDir, METH_O, "Sets direction. 0 = forward, 1 = backward, 2 = back and forth"},
    {"reset", (PyCFunction)Counter_reset, METH_O, "Resets the current count of the counter."},
    {"setMul", (PyCFunction)Counter_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Counter_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Counter_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Counter_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot CounterType_slots[] =
{
    {Py_tp_dealloc, Counter_dealloc},
    {Py_tp_doc, "Counter objects. Integer incrementor."},
    {Py_tp_traverse, Counter_traverse},
    {Py_tp_clear, Counter_clear},
    {Py_tp_methods, Counter_methods},
    {Py_tp_members, Counter_members},
    {Py_nb_add, Counter_add},
    {Py_nb_subtract, Counter_sub},
    {Py_nb_multiply, Counter_multiply},
    {Py_nb_true_divide, Counter_div},
    {Py_nb_inplace_add, Counter_inplace_add},
    {Py_nb_inplace_subtract, Counter_inplace_sub},
    {Py_nb_inplace_multiply, Counter_inplace_multiply},
    {Py_nb_inplace_true_divide, Counter_inplace_div},
    {Py_tp_new, Counter_new},
    {0, NULL}
};

static PyType_Spec CounterType_spec =
{
    "_pyo.Counter_base",
    sizeof(Counter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    CounterType_slots
};

PyTypeObject *
PyoCreateCounterType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &CounterType_spec, NULL);
}

/***************************************************/
/******* Thresh ***********/
/***************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *threshold;
    Stream *threshold_stream;
    int dir;
    int ready;
    int modebuffer[3];
} Thresh;

static void
Thresh_generates_i(Thresh *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT thresh = PyFloat_AS_DOUBLE(self->threshold);

    switch (self->dir)
    {
        case 0:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] > thresh && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh && self->ready == 0)
                    self->ready = 1;
            }

            break;

        case 1:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] < thresh && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] >= thresh && self->ready == 0)
                    self->ready = 1;
            }

            break;

        case 2:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] > thresh && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh && self->ready == 0)
                {
                    self->data[i] = 1.0;
                    self->ready = 1;
                }
            }

            break;
    }
}

static void
Thresh_generates_a(Thresh *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *thresh = Stream_getData((Stream *)self->threshold_stream);

    switch (self->dir)
    {
        case 0:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] > thresh[i] && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh[i] && self->ready == 0)
                    self->ready = 1;
            }

            break;

        case 1:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] < thresh[i] && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] >= thresh[i] && self->ready == 0)
                    self->ready = 1;
            }

            break;

        case 2:
            for (i = 0; i < self->bufsize; i++)
            {
                self->data[i] = 0.0;

                if (in[i] > thresh[i] && self->ready == 1)
                {
                    self->data[i] = 1.0;
                    self->ready = 0;
                }
                else if (in[i] <= thresh[i] && self->ready == 0)
                    self->data[i] = 1.0;

                self->ready = 1;
            }

            break;
    }
}

static void Thresh_postprocessing_ii(Thresh *self) { POST_PROCESSING_II };
static void Thresh_postprocessing_ai(Thresh *self) { POST_PROCESSING_AI };
static void Thresh_postprocessing_ia(Thresh *self) { POST_PROCESSING_IA };
static void Thresh_postprocessing_aa(Thresh *self) { POST_PROCESSING_AA };
static void Thresh_postprocessing_ireva(Thresh *self) { POST_PROCESSING_IREVA };
static void Thresh_postprocessing_areva(Thresh *self) { POST_PROCESSING_AREVA };
static void Thresh_postprocessing_revai(Thresh *self) { POST_PROCESSING_REVAI };
static void Thresh_postprocessing_revaa(Thresh *self) { POST_PROCESSING_REVAA };
static void Thresh_postprocessing_revareva(Thresh *self) { POST_PROCESSING_REVAREVA };

static void
Thresh_setProcMode(Thresh *self)
{
    int procmode = self->modebuffer[2];
    int muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Thresh_generates_i);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Thresh_generates_a);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Thresh_postprocessing_revareva);
            break;
    }
}

static void
Thresh_compute_next_data_frame(Thresh *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Thresh_traverse(Thresh *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->threshold);
    return 0;
}

static int
Thresh_clear(Thresh *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->threshold);
    return 0;
}

static void
Thresh_dealloc(Thresh* self)
{
    pyo_DEALLOC
    Thresh_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Thresh_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *thresholdtmp, *multmp = NULL, *addtmp = NULL;
    Thresh *self;
    self = (Thresh *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->threshold = PyFloat_FromDouble(0.);
    self->dir = 0;
    self->ready = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Thresh_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Thresh_setProcMode);

    static char *kwlist[] = {"input", "threshold", "dir", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OiOO", kwlist, &inputtmp, &thresholdtmp, &self->dir, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (thresholdtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setThreshold", thresholdtmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Thresh_getServer(Thresh* self) { GET_SERVER };
static PyObject * Thresh_getStream(Thresh* self) { GET_STREAM };
static PyObject * Thresh_setMul(Thresh *self, PyObject *arg) { SET_MUL };
static PyObject * Thresh_setAdd(Thresh *self, PyObject *arg) { SET_ADD };
static PyObject * Thresh_setSub(Thresh *self, PyObject *arg) { SET_SUB };
static PyObject * Thresh_setDiv(Thresh *self, PyObject *arg) { SET_DIV };

static PyObject * Thresh_play(Thresh *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Thresh_stop(Thresh *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Thresh_multiply(Thresh *self, PyObject *arg) { MULTIPLY };
static PyObject * Thresh_inplace_multiply(Thresh *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Thresh_add(Thresh *self, PyObject *arg) { ADD };
static PyObject * Thresh_inplace_add(Thresh *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Thresh_sub(Thresh *self, PyObject *arg) { SUB };
static PyObject * Thresh_inplace_sub(Thresh *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Thresh_div(Thresh *self, PyObject *arg) { DIV };
static PyObject * Thresh_inplace_div(Thresh *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Thresh_setThreshold(Thresh *self, PyObject *arg) { SET_PARAM(self->threshold, self->threshold_stream, 2); }

static PyObject *
Thresh_setDir(Thresh *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyLong_Check(arg))
    {
        self->dir = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyMemberDef Thresh_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Thresh, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Thresh, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Thresh, input), 0, "Input sound object."},
    {"threshold", T_OBJECT_EX, offsetof(Thresh, threshold), 0, "Threshold object."},
    {"mul", T_OBJECT_EX, offsetof(Thresh, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Thresh, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Thresh_methods[] =
{
    {"getServer", (PyCFunction)Thresh_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Thresh_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Thresh_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Thresh_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setThreshold", (PyCFunction)Thresh_setThreshold, METH_O, "Sets threshold value."},
    {"setDir", (PyCFunction)Thresh_setDir, METH_O, "Sets direction. 0 = upward, 1 = downward, 2 = up and down"},
    {"setMul", (PyCFunction)Thresh_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Thresh_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Thresh_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Thresh_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot ThreshType_slots[] =
{
    {Py_tp_dealloc, Thresh_dealloc},
    {Py_tp_doc, "Thresh objects. Threshold detector."},
    {Py_tp_traverse, Thresh_traverse},
    {Py_tp_clear, Thresh_clear},
    {Py_tp_methods, Thresh_methods},
    {Py_tp_members, Thresh_members},
    {Py_nb_add, Thresh_add},
    {Py_nb_subtract, Thresh_sub},
    {Py_nb_multiply, Thresh_multiply},
    {Py_nb_true_divide, Thresh_div},
    {Py_nb_inplace_add, Thresh_inplace_add},
    {Py_nb_inplace_subtract, Thresh_inplace_sub},
    {Py_nb_inplace_multiply, Thresh_inplace_multiply},
    {Py_nb_inplace_true_divide, Thresh_inplace_div},
    {Py_tp_new, Thresh_new},
    {0, NULL}
};

static PyType_Spec ThreshType_spec =
{
    "_pyo.Thresh_base",
    sizeof(Thresh),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    ThreshType_slots
};

PyTypeObject *
PyoCreateThreshType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &ThreshType_spec, NULL);
}

/***************************************************/
/******* Percent ***********/
/***************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *percent;
    Stream *percent_stream;
    int modebuffer[3];
} Percent;

static void
Percent_generates_i(Percent *self)
{
    int i;
    MYFLT guess;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT perc = PyFloat_AS_DOUBLE(self->percent);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = 0.0;

        if (in[i] == 1.0)
        {
            guess = RANDOM_UNIFORM * 100.0;

            if (guess <= perc)
                self->data[i] = 1.0;
        }
    }
}

static void
Percent_generates_a(Percent *self)
{
    int i;
    MYFLT guess;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *perc = Stream_getData((Stream *)self->percent_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = 0.0;

        if (in[i] == 1.0)
        {
            guess = RANDOM_UNIFORM * 100.0;

            if (guess <= perc[i])
                self->data[i] = 1.0;
        }
    }
}

static void Percent_postprocessing_ii(Percent *self) { POST_PROCESSING_II };
static void Percent_postprocessing_ai(Percent *self) { POST_PROCESSING_AI };
static void Percent_postprocessing_ia(Percent *self) { POST_PROCESSING_IA };
static void Percent_postprocessing_aa(Percent *self) { POST_PROCESSING_AA };
static void Percent_postprocessing_ireva(Percent *self) { POST_PROCESSING_IREVA };
static void Percent_postprocessing_areva(Percent *self) { POST_PROCESSING_AREVA };
static void Percent_postprocessing_revai(Percent *self) { POST_PROCESSING_REVAI };
static void Percent_postprocessing_revaa(Percent *self) { POST_PROCESSING_REVAA };
static void Percent_postprocessing_revareva(Percent *self) { POST_PROCESSING_REVAREVA };

static void
Percent_setProcMode(Percent *self)
{
    int muladdmode, procmode
    ;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Percent_generates_i);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Percent_generates_a);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Percent_postprocessing_revareva);
            break;
    }
}

static void
Percent_compute_next_data_frame(Percent *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Percent_traverse(Percent *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->percent);
    return 0;
}

static int
Percent_clear(Percent *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->percent);
    return 0;
}

static void
Percent_dealloc(Percent* self)
{
    pyo_DEALLOC
    Percent_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Percent_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *percenttmp, *multmp = NULL, *addtmp = NULL;
    Percent *self;
    self = (Percent *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->percent = PyFloat_FromDouble(50.);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Percent_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Percent_setProcMode);

    static char *kwlist[] = {"input", "percent", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &percenttmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (percenttmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setPercent", percenttmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Server_generateSeed((Server *)self->server, PERCENT_ID);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Percent_getServer(Percent* self) { GET_SERVER };
static PyObject * Percent_getStream(Percent* self) { GET_STREAM };
static PyObject * Percent_setMul(Percent *self, PyObject *arg) { SET_MUL };
static PyObject * Percent_setAdd(Percent *self, PyObject *arg) { SET_ADD };
static PyObject * Percent_setSub(Percent *self, PyObject *arg) { SET_SUB };
static PyObject * Percent_setDiv(Percent *self, PyObject *arg) { SET_DIV };

static PyObject * Percent_play(Percent *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Percent_stop(Percent *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Percent_multiply(Percent *self, PyObject *arg) { MULTIPLY };
static PyObject * Percent_inplace_multiply(Percent *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Percent_add(Percent *self, PyObject *arg) { ADD };
static PyObject * Percent_inplace_add(Percent *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Percent_sub(Percent *self, PyObject *arg) { SUB };
static PyObject * Percent_inplace_sub(Percent *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Percent_div(Percent *self, PyObject *arg) { DIV };
static PyObject * Percent_inplace_div(Percent *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Percent_setPercent(Percent *self, PyObject *arg) { SET_PARAM(self->percent, self->percent_stream, 2); }

static PyMemberDef Percent_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Percent, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Percent, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Percent, input), 0, "Input sound object."},
    {"percent", T_OBJECT_EX, offsetof(Percent, percent), 0, "percent attribute."},
    {"mul", T_OBJECT_EX, offsetof(Percent, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Percent, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Percent_methods[] =
{
    {"getServer", (PyCFunction)Percent_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Percent_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Percent_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Percent_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setPercent", (PyCFunction)Percent_setPercent, METH_O, "Sets percentange value."},
    {"setMul", (PyCFunction)Percent_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Percent_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Percent_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Percent_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot PercentType_slots[] =
{
    {Py_tp_dealloc, Percent_dealloc},
    {Py_tp_doc, "Percent objects. Looks for input triggers and sets how much percentage of it to let pass."},
    {Py_tp_traverse, Percent_traverse},
    {Py_tp_clear, Percent_clear},
    {Py_tp_methods, Percent_methods},
    {Py_tp_members, Percent_members},
    {Py_nb_add, Percent_add},
    {Py_nb_subtract, Percent_sub},
    {Py_nb_multiply, Percent_multiply},
    {Py_nb_true_divide, Percent_div},
    {Py_nb_inplace_add, Percent_inplace_add},
    {Py_nb_inplace_subtract, Percent_inplace_sub},
    {Py_nb_inplace_multiply, Percent_inplace_multiply},
    {Py_nb_inplace_true_divide, Percent_inplace_div},
    {Py_tp_new, Percent_new},
    {0, NULL}
};

static PyType_Spec PercentType_spec =
{
    "_pyo.Percent_base",
    sizeof(Percent),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    PercentType_slots
};

PyTypeObject *
PyoCreatePercentType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &PercentType_spec, NULL);
}

/*************************/
/******* Timer ***********/
/*************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    unsigned long count;
    MYFLT lasttime;
    int started;
    int modebuffer[2];
} Timer;

static void
Timer_generates(Timer *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->started == 1)
        {
            self->count++;

            if (in[i] == 1.0)
            {
                self->lasttime = self->count / self->sr;
                self->started = 0;
            }
        }

        if (in2[i] == 1 && self->started == 0)
        {
            self->count = 0;
            self->started = 1;
        }

        self->data[i] = self->lasttime;
    }
}

static void Timer_postprocessing_ii(Timer *self) { POST_PROCESSING_II };
static void Timer_postprocessing_ai(Timer *self) { POST_PROCESSING_AI };
static void Timer_postprocessing_ia(Timer *self) { POST_PROCESSING_IA };
static void Timer_postprocessing_aa(Timer *self) { POST_PROCESSING_AA };
static void Timer_postprocessing_ireva(Timer *self) { POST_PROCESSING_IREVA };
static void Timer_postprocessing_areva(Timer *self) { POST_PROCESSING_AREVA };
static void Timer_postprocessing_revai(Timer *self) { POST_PROCESSING_REVAI };
static void Timer_postprocessing_revaa(Timer *self) { POST_PROCESSING_REVAA };
static void Timer_postprocessing_revareva(Timer *self) { POST_PROCESSING_REVAREVA };

static void
Timer_setProcMode(Timer *self)
{
    int muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Timer_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Timer_postprocessing_revareva);
            break;
    }
}

static void
Timer_compute_next_data_frame(Timer *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Timer_traverse(Timer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    return 0;
}

static int
Timer_clear(Timer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    return 0;
}

static void
Timer_dealloc(Timer* self)
{
    pyo_DEALLOC
    Timer_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Timer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp = NULL, *addtmp = NULL;
    Timer *self;
    self = (Timer *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->count = 0;
    self->started = 0;
    self->lasttime = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Timer_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Timer_setProcMode);

    static char *kwlist[] = {"input", "input2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &input2tmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    self->input2 = input2tmp;
    Py_INCREF(self->input2);
    input2_streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->input2, "_getStream", NULL);
    self->input2_stream = (Stream *)input2_streamtmp;
    Py_INCREF(self->input2_stream);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Timer_getServer(Timer* self) { GET_SERVER };
static PyObject * Timer_getStream(Timer* self) { GET_STREAM };
static PyObject * Timer_setMul(Timer *self, PyObject *arg) { SET_MUL };
static PyObject * Timer_setAdd(Timer *self, PyObject *arg) { SET_ADD };
static PyObject * Timer_setSub(Timer *self, PyObject *arg) { SET_SUB };
static PyObject * Timer_setDiv(Timer *self, PyObject *arg) { SET_DIV };

static PyObject * Timer_play(Timer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Timer_stop(Timer *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Timer_multiply(Timer *self, PyObject *arg) { MULTIPLY };
static PyObject * Timer_inplace_multiply(Timer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Timer_add(Timer *self, PyObject *arg) { ADD };
static PyObject * Timer_inplace_add(Timer *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Timer_sub(Timer *self, PyObject *arg) { SUB };
static PyObject * Timer_inplace_sub(Timer *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Timer_div(Timer *self, PyObject *arg) { DIV };
static PyObject * Timer_inplace_div(Timer *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Timer_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Timer, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Timer, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Timer, input), 0, "Stops timer and output time elapsed."},
    {"input2", T_OBJECT_EX, offsetof(Timer, input2), 0, "Starts timer."},
    {"mul", T_OBJECT_EX, offsetof(Timer, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Timer, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Timer_methods[] =
{
    {"getServer", (PyCFunction)Timer_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Timer_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Timer_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Timer_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Timer_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Timer_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Timer_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Timer_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TimerType_slots[] =
{
    {Py_tp_dealloc, Timer_dealloc},
    {Py_tp_doc, "Timer objects. Returns elapsed time between two triggers."},
    {Py_tp_traverse, Timer_traverse},
    {Py_tp_clear, Timer_clear},
    {Py_tp_methods, Timer_methods},
    {Py_tp_members, Timer_members},
    {Py_nb_add, Timer_add},
    {Py_nb_subtract, Timer_sub},
    {Py_nb_multiply, Timer_multiply},
    {Py_nb_true_divide, Timer_div},
    {Py_nb_inplace_add, Timer_inplace_add},
    {Py_nb_inplace_subtract, Timer_inplace_sub},
    {Py_nb_inplace_multiply, Timer_inplace_multiply},
    {Py_nb_inplace_true_divide, Timer_inplace_div},
    {Py_tp_new, Timer_new},
    {0, NULL}
};

static PyType_Spec TimerType_spec =
{
    "_pyo.Timer_base",
    sizeof(Timer),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TimerType_slots
};

PyTypeObject *
PyoCreateTimerType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TimerType_spec, NULL);
}

/*********************************************************************************************/
/* Iter ********************************************************************************/
/*********************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *choice;
    Stream *audioval;
    int chSize;
    int chCount;
    int curIsAudio;
    MYFLT value;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Iter;

static void
Iter_get_stream(Iter *self, PyObject *obj)
{
    PyObject *streamtmp;
    streamtmp = PYO_CALL_METHOD_RET((PyObject *)obj, "_getStream", NULL);
    Py_XDECREF(self->audioval);
    self->audioval = (Stream *)streamtmp;
    Py_INCREF(self->audioval);
}

static void
Iter_generate(Iter *self)
{
    int i;
    PyObject *obj;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;

        if (in[i] == 1)
        {
            if (self->chCount >= self->chSize)
                self->chCount = 0;

            obj = PyList_GetItem(self->choice, self->chCount);

            if (PyNumber_Check(obj))
            {
                self->value = PyFloat_AsDouble(obj);
                self->curIsAudio = 0;
            }
            else
            {
                self->curIsAudio = 1;
                Iter_get_stream(self, obj);
            }

            self->chCount++;

            if (self->chCount == self->chSize)
                self->trigsBuffer[i] = 1.0;
        }

        if (self->curIsAudio)
        {
            MYFLT *val = Stream_getData((Stream *)self->audioval);
            self->data[i] = val[i];
        }
        else
        {
            self->data[i] = self->value;
        }
    }
}

static void Iter_postprocessing_ii(Iter *self) { POST_PROCESSING_II };
static void Iter_postprocessing_ai(Iter *self) { POST_PROCESSING_AI };
static void Iter_postprocessing_ia(Iter *self) { POST_PROCESSING_IA };
static void Iter_postprocessing_aa(Iter *self) { POST_PROCESSING_AA };
static void Iter_postprocessing_ireva(Iter *self) { POST_PROCESSING_IREVA };
static void Iter_postprocessing_areva(Iter *self) { POST_PROCESSING_AREVA };
static void Iter_postprocessing_revai(Iter *self) { POST_PROCESSING_REVAI };
static void Iter_postprocessing_revaa(Iter *self) { POST_PROCESSING_REVAA };
static void Iter_postprocessing_revareva(Iter *self) { POST_PROCESSING_REVAREVA };

static void
Iter_setProcMode(Iter *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Iter_generate);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Iter_postprocessing_revareva);
            break;
    }
}

static void
Iter_compute_next_data_frame(Iter *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Iter_traverse(Iter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->choice);
    Py_VISIT(self->audioval);
    return 0;
}

static int
Iter_clear(Iter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->choice);
    Py_CLEAR(self->audioval);
    return 0;
}

static void
Iter_dealloc(Iter* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->trigsBuffer);
    Iter_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Iter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT inittmp = 0.0;
    PyObject *inputtmp, *input_streamtmp, *choicetmp = NULL, *multmp = NULL, *addtmp = NULL;
    Iter *self;
    self = (Iter *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = 0.;
    self->chCount = 0;
    self->curIsAudio = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Iter_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Iter_setProcMode);

    static char *kwlist[] = {"input", "choice", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_FOO, kwlist, &inputtmp, &choicetmp, &inittmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (choicetmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setChoice", choicetmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->value = inittmp;

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

static PyObject * Iter_getServer(Iter* self) { GET_SERVER };
static PyObject * Iter_getStream(Iter* self) { GET_STREAM };
static PyObject * Iter_getTriggerStream(Iter* self) { GET_TRIGGER_STREAM };
static PyObject * Iter_setMul(Iter *self, PyObject *arg) { SET_MUL };
static PyObject * Iter_setAdd(Iter *self, PyObject *arg) { SET_ADD };
static PyObject * Iter_setSub(Iter *self, PyObject *arg) { SET_SUB };
static PyObject * Iter_setDiv(Iter *self, PyObject *arg) { SET_DIV };

static PyObject * Iter_play(Iter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Iter_out(Iter *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Iter_stop(Iter *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Iter_multiply(Iter *self, PyObject *arg) { MULTIPLY };
static PyObject * Iter_inplace_multiply(Iter *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Iter_add(Iter *self, PyObject *arg) { ADD };
static PyObject * Iter_inplace_add(Iter *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Iter_sub(Iter *self, PyObject *arg) { SUB };
static PyObject * Iter_inplace_sub(Iter *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Iter_div(Iter *self, PyObject *arg) { DIV };
static PyObject * Iter_inplace_div(Iter *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Iter_setChoice(Iter *self, PyObject *arg)
{
    if (! PyList_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The choice attribute must be a list.");
        Py_RETURN_NONE;
    }

    self->chSize = PyList_Size(arg);
    Py_XDECREF(self->choice);
    self->choice = arg;
    Py_INCREF(self->choice);

    Py_RETURN_NONE;
}

static PyObject *
Iter_reset(Iter *self, PyObject *arg)
{
    int tmp;

    if (PyLong_Check(arg))
    {
        tmp = PyLong_AsLong(arg);

        if (tmp < self->chSize)
            self->chCount = tmp;
        else
            self->chCount = 0;
    }

    Py_RETURN_NONE;
}

static PyMemberDef Iter_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Iter, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Iter, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(Iter, trig_stream), 0, "Trigger Stream object."},
    {"input", T_OBJECT_EX, offsetof(Iter, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Iter, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Iter, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Iter_methods[] =
{
    {"getServer", (PyCFunction)Iter_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Iter_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)Iter_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"play", (PyCFunction)Iter_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Iter_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Iter_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setChoice", (PyCFunction)Iter_setChoice, METH_O, "Sets possible values."},
    {"reset", (PyCFunction)Iter_reset, METH_O, "Resets count to 0."},
    {"setMul", (PyCFunction)Iter_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Iter_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Iter_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Iter_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot IterType_slots[] =
{
    {Py_tp_dealloc, Iter_dealloc},
    {Py_tp_doc, "Iter objects. Triggers iterate over a list of values."},
    {Py_tp_traverse, Iter_traverse},
    {Py_tp_clear, Iter_clear},
    {Py_tp_methods, Iter_methods},
    {Py_tp_members, Iter_members},
    {Py_nb_add, Iter_add},
    {Py_nb_subtract, Iter_sub},
    {Py_nb_multiply, Iter_multiply},
    {Py_nb_true_divide, Iter_div},
    {Py_nb_inplace_add, Iter_inplace_add},
    {Py_nb_inplace_subtract, Iter_inplace_sub},
    {Py_nb_inplace_multiply, Iter_inplace_multiply},
    {Py_nb_inplace_true_divide, Iter_inplace_div},
    {Py_tp_new, Iter_new},
    {0, NULL}
};

static PyType_Spec IterType_spec =
{
    "_pyo.Iter_base",
    sizeof(Iter),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    IterType_slots
};

PyTypeObject *
PyoCreateIterType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &IterType_spec, NULL);
}

/*************************/
/******* Count ***********/
/*************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    unsigned long count;
    unsigned long min;
    unsigned long max;
    int started;
    int modebuffer[2];
} Count;

static void
Count_generates(Count *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
        {
            self->count = self->min;
            self->started = 1;
        }

        if (self->started == 1)
        {
            self->data[i] = (MYFLT)self->count;

            if (self->count++ >= self->max && self->max != 0)
                self->count = self->min;
        }
        else
        {
            self->data[i] = self->min;
        }
    }
}

static void Count_postprocessing_ii(Count *self) { POST_PROCESSING_II };
static void Count_postprocessing_ai(Count *self) { POST_PROCESSING_AI };
static void Count_postprocessing_ia(Count *self) { POST_PROCESSING_IA };
static void Count_postprocessing_aa(Count *self) { POST_PROCESSING_AA };
static void Count_postprocessing_ireva(Count *self) { POST_PROCESSING_IREVA };
static void Count_postprocessing_areva(Count *self) { POST_PROCESSING_AREVA };
static void Count_postprocessing_revai(Count *self) { POST_PROCESSING_REVAI };
static void Count_postprocessing_revaa(Count *self) { POST_PROCESSING_REVAA };
static void Count_postprocessing_revareva(Count *self) { POST_PROCESSING_REVAREVA };

static void
Count_setProcMode(Count *self)
{
    int muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(Count_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Count_postprocessing_revareva);
            break;
    }
}

static void
Count_compute_next_data_frame(Count *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Count_traverse(Count *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
Count_clear(Count *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Count_dealloc(Count* self)
{
    pyo_DEALLOC
    Count_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Count_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp = NULL, *addtmp = NULL;
    Count *self;
    self = (Count *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->started = 0;
    self->count = 0;
    self->min = 0;
    self->max = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Count_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Count_setProcMode);

    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|kkOO", kwlist, &inputtmp, &self->min, &self->max, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Count_getServer(Count* self) { GET_SERVER };
static PyObject * Count_getStream(Count* self) { GET_STREAM };
static PyObject * Count_setMul(Count *self, PyObject *arg) { SET_MUL };
static PyObject * Count_setAdd(Count *self, PyObject *arg) { SET_ADD };
static PyObject * Count_setSub(Count *self, PyObject *arg) { SET_SUB };
static PyObject * Count_setDiv(Count *self, PyObject *arg) { SET_DIV };

static PyObject * Count_play(Count *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Count_stop(Count *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Count_multiply(Count *self, PyObject *arg) { MULTIPLY };
static PyObject * Count_inplace_multiply(Count *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Count_add(Count *self, PyObject *arg) { ADD };
static PyObject * Count_inplace_add(Count *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Count_sub(Count *self, PyObject *arg) { SUB };
static PyObject * Count_inplace_sub(Count *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Count_div(Count *self, PyObject *arg) { DIV };
static PyObject * Count_inplace_div(Count *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Count_setMin(Count *self, PyObject *arg)
{
    if (PyLong_Check(arg))
        self->min = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject *
Count_setMax(Count *self, PyObject *arg)
{
    if (arg == Py_None)
        self->max = 0;
    else if (PyLong_Check(arg))
        self->max = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyMemberDef Count_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Count, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Count, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Count, input), 0, "Starts the count."},
    {"mul", T_OBJECT_EX, offsetof(Count, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Count, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Count_methods[] =
{
    {"getServer", (PyCFunction)Count_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Count_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Count_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Count_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMin", (PyCFunction)Count_setMin, METH_O, "Sets the minimum value."},
    {"setMax", (PyCFunction)Count_setMax, METH_O, "Sets the maximum value."},
    {"setMul", (PyCFunction)Count_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)Count_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)Count_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Count_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot CountType_slots[] =
{
    {Py_tp_dealloc, Count_dealloc},
    {Py_tp_doc, "Count objects. Counts integer at audio rate."},
    {Py_tp_traverse, Count_traverse},
    {Py_tp_clear, Count_clear},
    {Py_tp_methods, Count_methods},
    {Py_tp_members, Count_members},
    {Py_nb_add, Count_add},
    {Py_nb_subtract, Count_sub},
    {Py_nb_multiply, Count_multiply},
    {Py_nb_true_divide, Count_div},
    {Py_nb_inplace_add, Count_inplace_add},
    {Py_nb_inplace_subtract, Count_inplace_sub},
    {Py_nb_inplace_multiply, Count_inplace_multiply},
    {Py_nb_inplace_true_divide, Count_inplace_div},
    {Py_tp_new, Count_new},
    {0, NULL}
};

static PyType_Spec CountType_spec =
{
    "_pyo.Count_base",
    sizeof(Count),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    CountType_slots
};

PyTypeObject *
PyoCreateCountType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &CountType_spec, NULL);
}

/*************************/
/******* NextTrig ********/
/*************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    int gate;
    int modebuffer[2];
} NextTrig;

static void
NextTrig_generates(NextTrig *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = 0.0;

        if (self->gate == 1 && in[i] == 1.0)
        {
            self->data[i] = 1.0;
            self->gate = 0;
        }

        if (in2[i] == 1 && self->gate == 0)
            self->gate = 1;
    }
}

static void NextTrig_postprocessing_ii(NextTrig *self) { POST_PROCESSING_II };
static void NextTrig_postprocessing_ai(NextTrig *self) { POST_PROCESSING_AI };
static void NextTrig_postprocessing_ia(NextTrig *self) { POST_PROCESSING_IA };
static void NextTrig_postprocessing_aa(NextTrig *self) { POST_PROCESSING_AA };
static void NextTrig_postprocessing_ireva(NextTrig *self) { POST_PROCESSING_IREVA };
static void NextTrig_postprocessing_areva(NextTrig *self) { POST_PROCESSING_AREVA };
static void NextTrig_postprocessing_revai(NextTrig *self) { POST_PROCESSING_REVAI };
static void NextTrig_postprocessing_revaa(NextTrig *self) { POST_PROCESSING_REVAA };
static void NextTrig_postprocessing_revareva(NextTrig *self) { POST_PROCESSING_REVAREVA };

static void
NextTrig_setProcMode(NextTrig *self)
{
    int muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_generates);

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_postprocessing_revareva);
            break;
    }
}

static void
NextTrig_compute_next_data_frame(NextTrig *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
NextTrig_traverse(NextTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input2);
    return 0;
}

static int
NextTrig_clear(NextTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input2);
    return 0;
}

static void
NextTrig_dealloc(NextTrig* self)
{
    pyo_DEALLOC
    NextTrig_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NextTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp = NULL, *addtmp = NULL;
    NextTrig *self;
    self = (NextTrig *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->gate = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(NextTrig_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(NextTrig_setProcMode);

    static char *kwlist[] = {"input", "input2", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &input2tmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    self->input2 = input2tmp;
    Py_INCREF(self->input2);
    input2_streamtmp = PYO_CALL_METHOD_RET((PyObject *)self->input2, "_getStream", NULL);
    self->input2_stream = (Stream *)input2_streamtmp;
    Py_INCREF(self->input2_stream);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * NextTrig_getServer(NextTrig* self) { GET_SERVER };
static PyObject * NextTrig_getStream(NextTrig* self) { GET_STREAM };
static PyObject * NextTrig_setMul(NextTrig *self, PyObject *arg) { SET_MUL };
static PyObject * NextTrig_setAdd(NextTrig *self, PyObject *arg) { SET_ADD };
static PyObject * NextTrig_setSub(NextTrig *self, PyObject *arg) { SET_SUB };
static PyObject * NextTrig_setDiv(NextTrig *self, PyObject *arg) { SET_DIV };

static PyObject * NextTrig_play(NextTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * NextTrig_stop(NextTrig *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * NextTrig_multiply(NextTrig *self, PyObject *arg) { MULTIPLY };
static PyObject * NextTrig_inplace_multiply(NextTrig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NextTrig_add(NextTrig *self, PyObject *arg) { ADD };
static PyObject * NextTrig_inplace_add(NextTrig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NextTrig_sub(NextTrig *self, PyObject *arg) { SUB };
static PyObject * NextTrig_inplace_sub(NextTrig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NextTrig_div(NextTrig *self, PyObject *arg) { DIV };
static PyObject * NextTrig_inplace_div(NextTrig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef NextTrig_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NextTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NextTrig, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(NextTrig, input), 0, "Stops NextTrig and output time elapsed."},
    {"input2", T_OBJECT_EX, offsetof(NextTrig, input2), 0, "Starts NextTrig."},
    {"mul", T_OBJECT_EX, offsetof(NextTrig, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(NextTrig, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NextTrig_methods[] =
{
    {"getServer", (PyCFunction)NextTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NextTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)NextTrig_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NextTrig_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)NextTrig_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)NextTrig_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)NextTrig_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)NextTrig_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot NextTrigType_slots[] =
{
    {Py_tp_dealloc, NextTrig_dealloc},
    {Py_tp_doc, "NextTrig objects. A trig opens a gate only for the next one."},
    {Py_tp_traverse, NextTrig_traverse},
    {Py_tp_clear, NextTrig_clear},
    {Py_tp_methods, NextTrig_methods},
    {Py_tp_members, NextTrig_members},
    {Py_nb_add, NextTrig_add},
    {Py_nb_subtract, NextTrig_sub},
    {Py_nb_multiply, NextTrig_multiply},
    {Py_nb_true_divide, NextTrig_div},
    {Py_nb_inplace_add, NextTrig_inplace_add},
    {Py_nb_inplace_subtract, NextTrig_inplace_sub},
    {Py_nb_inplace_multiply, NextTrig_inplace_multiply},
    {Py_nb_inplace_true_divide, NextTrig_inplace_div},
    {Py_tp_new, NextTrig_new},
    {0, NULL}
};

static PyType_Spec NextTrigType_spec =
{
    "_pyo.NextTrig_base",
    sizeof(NextTrig),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    NextTrigType_slots
};

PyTypeObject *
PyoCreateNextTrigType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &NextTrigType_spec, NULL);
}

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *value;
    Stream *value_stream;
    MYFLT current_value;
    int modebuffer[3]; // need at least 2 slots for mul & add
} TrigVal;

static void
TrigVal_generate_i(TrigVal *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT val = PyFloat_AS_DOUBLE(self->value);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
            self->current_value = val;

        self->data[i] = self->current_value;
    }
}

static void
TrigVal_generate_a(TrigVal *self)
{
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *val = Stream_getData((Stream *)self->value_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (in[i] == 1)
            self->current_value = val[i];

        self->data[i] = self->current_value;
    }
}

static void TrigVal_postprocessing_ii(TrigVal *self) { POST_PROCESSING_II };
static void TrigVal_postprocessing_ai(TrigVal *self) { POST_PROCESSING_AI };
static void TrigVal_postprocessing_ia(TrigVal *self) { POST_PROCESSING_IA };
static void TrigVal_postprocessing_aa(TrigVal *self) { POST_PROCESSING_AA };
static void TrigVal_postprocessing_ireva(TrigVal *self) { POST_PROCESSING_IREVA };
static void TrigVal_postprocessing_areva(TrigVal *self) { POST_PROCESSING_AREVA };
static void TrigVal_postprocessing_revai(TrigVal *self) { POST_PROCESSING_REVAI };
static void TrigVal_postprocessing_revaa(TrigVal *self) { POST_PROCESSING_REVAA };
static void TrigVal_postprocessing_revareva(TrigVal *self) { POST_PROCESSING_REVAREVA };

static void
TrigVal_setProcMode(TrigVal *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_generate_i);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_generate_a);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_postprocessing_revareva);
            break;
    }
}

static void
TrigVal_compute_next_data_frame(TrigVal *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
TrigVal_traverse(TrigVal *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->value);
    return 0;
}

static int
TrigVal_clear(TrigVal *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->value);
    return 0;
}

static void
TrigVal_dealloc(TrigVal* self)
{
    pyo_DEALLOC
    TrigVal_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigVal_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *valuetmp = NULL, *multmp = NULL, *addtmp = NULL;
    TrigVal *self;
    self = (TrigVal *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = PyFloat_FromDouble(0.);
    self->current_value = 0.;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(TrigVal_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(TrigVal_setProcMode);

    static char *kwlist[] = {"input", "value", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OFOO, kwlist, &inputtmp, &valuetmp, &self->current_value, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (valuetmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setValue", valuetmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigVal_getServer(TrigVal* self) { GET_SERVER };
static PyObject * TrigVal_getStream(TrigVal* self) { GET_STREAM };
static PyObject * TrigVal_setMul(TrigVal *self, PyObject *arg) { SET_MUL };
static PyObject * TrigVal_setAdd(TrigVal *self, PyObject *arg) { SET_ADD };
static PyObject * TrigVal_setSub(TrigVal *self, PyObject *arg) { SET_SUB };
static PyObject * TrigVal_setDiv(TrigVal *self, PyObject *arg) { SET_DIV };

static PyObject * TrigVal_play(TrigVal *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigVal_out(TrigVal *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigVal_stop(TrigVal *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigVal_multiply(TrigVal *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigVal_inplace_multiply(TrigVal *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigVal_add(TrigVal *self, PyObject *arg) { ADD };
static PyObject * TrigVal_inplace_add(TrigVal *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigVal_sub(TrigVal *self, PyObject *arg) { SUB };
static PyObject * TrigVal_inplace_sub(TrigVal *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigVal_div(TrigVal *self, PyObject *arg) { DIV };
static PyObject * TrigVal_inplace_div(TrigVal *self, PyObject *arg) { INPLACE_DIV };

static PyObject * TrigVal_setValue(TrigVal *self, PyObject *arg) { SET_PARAM(self->value, self->value_stream, 2); }

static PyMemberDef TrigVal_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigVal, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigVal, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigVal, input), 0, "Input sound object."},
    {"value", T_OBJECT_EX, offsetof(TrigVal, value), 0, "Next value."},
    {"mul", T_OBJECT_EX, offsetof(TrigVal, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigVal, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigVal_methods[] =
{
    {"getServer", (PyCFunction)TrigVal_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigVal_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigVal_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigVal_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigVal_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setValue", (PyCFunction)TrigVal_setValue, METH_O, "Sets the next value."},
    {"setMul", (PyCFunction)TrigVal_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigVal_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigVal_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigVal_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot TrigValType_slots[] =
{
    {Py_tp_dealloc, TrigVal_dealloc},
    {Py_tp_doc, "TrigVal objects. Outputs a previously defined value on a trigger signal."},
    {Py_tp_traverse, TrigVal_traverse},
    {Py_tp_clear, TrigVal_clear},
    {Py_tp_methods, TrigVal_methods},
    {Py_tp_members, TrigVal_members},
    {Py_nb_add, TrigVal_add},
    {Py_nb_subtract, TrigVal_sub},
    {Py_nb_multiply, TrigVal_multiply},
    {Py_nb_true_divide, TrigVal_div},
    {Py_nb_inplace_add, TrigVal_inplace_add},
    {Py_nb_inplace_subtract, TrigVal_inplace_sub},
    {Py_nb_inplace_multiply, TrigVal_inplace_multiply},
    {Py_nb_inplace_true_divide, TrigVal_inplace_div},
    {Py_tp_new, TrigVal_new},
    {0, NULL}
};

static PyType_Spec TrigValType_spec =
{
    "_pyo.TrigVal_base",
    sizeof(TrigVal),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    TrigValType_slots
};

PyTypeObject *
PyoCreateTrigValType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &TrigValType_spec, NULL);
}
