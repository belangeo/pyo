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
    PyObject *input;
    Stream *input_stream;
    MYFLT coefs[12];
    // sample memories
    MYFLT x1[12];
    MYFLT y1[12];
    MYFLT *buffer_streams;
} HilbertMain;

/* 6th order allpass poles */
static const MYFLT poles[12] = {.3609, 2.7412, 11.1573, 44.7581, 179.6242, 798.4578,
                                1.2524, 5.5671, 22.3423, 89.6271, 364.7914, 2770.1114
                               };

static void
HilbertMain_compute_variables(HilbertMain *self)
{
    int i;
    MYFLT polefreq[12];
    MYFLT rc[12];
    MYFLT alpha[12];

    for (i = 0; i < 12; i++)
    {
        polefreq[i] = poles[i] * 15.0;
        rc[i] = 1.0 / (TWOPI * polefreq[i]);
        alpha[i] = 1.0 / rc[i];
        self->coefs[i] = - (1.0 - (alpha[i] / (2.0 * self->sr))) / (1.0 + (alpha[i] / (2.0 * self->sr)));
    }
}

static void
HilbertMain_filters(HilbertMain *self)
{
    MYFLT xn1, xn2, yn1, yn2;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        xn1 = in[i];

        for (j = 0; j < 6; j++)
        {
            yn1 = self->coefs[j] * (xn1 - self->y1[j]) + self->x1[j];
            self->x1[j] = xn1;
            self->y1[j] = yn1;
            xn1 = yn1;
        }

        xn2 = in[i];

        for (j = 6; j < 12; j++)
        {
            yn2 = self->coefs[j] * (xn2 - self->y1[j]) + self->x1[j];
            self->x1[j] = xn2;
            self->y1[j] = yn2;
            xn2 = yn2;
        }

        self->buffer_streams[i] = yn1;
        self->buffer_streams[i + self->bufsize] = yn2;

    }
}

MYFLT *
HilbertMain_getSamplesBuffer(HilbertMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
HilbertMain_setProcMode(HilbertMain *self)
{
    self->proc_func_ptr = PYO_AUDIO_CALLBACK(HilbertMain_filters);
}

static void
HilbertMain_compute_next_data_frame(HilbertMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
HilbertMain_traverse(HilbertMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
HilbertMain_clear(HilbertMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
HilbertMain_dealloc(HilbertMain* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->buffer_streams);
    HilbertMain_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
HilbertMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    HilbertMain *self;
    self = (HilbertMain *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(HilbertMain_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(HilbertMain_setProcMode);

    for (i = 0; i < 12; i++)
    {
        self->x1[i] = 0.0;
        self->y1[i] = 0.0;
    }

    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->buffer_streams = (MYFLT *)PyMem_RawRealloc(self->buffer_streams, 2 * self->bufsize * sizeof(MYFLT));

    HilbertMain_compute_variables((HilbertMain *)self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * HilbertMain_getServer(HilbertMain* self) { GET_SERVER };
static PyObject * HilbertMain_getStream(HilbertMain* self) { GET_STREAM };

static PyObject * HilbertMain_play(HilbertMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * HilbertMain_stop(HilbertMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef HilbertMain_members[] =
{
    {"server", T_OBJECT_EX, offsetof(HilbertMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(HilbertMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(HilbertMain, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef HilbertMain_methods[] =
{
    {"getServer", (PyCFunction)HilbertMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)HilbertMain_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)HilbertMain_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)HilbertMain_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

static PyType_Slot HilbertMainType_slots[] = {
    {Py_tp_dealloc, HilbertMain_dealloc},
    {Py_tp_doc, "HilbertMain objects. Hilbert transform. Created real and imaginary parts from an audio stream"},
    {Py_tp_traverse, HilbertMain_traverse},
    {Py_tp_clear, HilbertMain_clear},
    {Py_tp_methods, HilbertMain_methods},
    {Py_tp_members, HilbertMain_members},
    {Py_tp_new, HilbertMain_new},
    {0, NULL}
};

static PyType_Spec HilbertMainType_spec =
{
    "_pyo.HilbertMain_base",
    sizeof(HilbertMain),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    HilbertMainType_slots
};

PyTypeObject *
PyoCreateHilbertMainType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &HilbertMainType_spec, NULL);
}

/************************************************************************************************/
/* Hilbert streamer object */
/************************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    HilbertMain *mainSplitter;
    int modebuffer[2];
    int chnl; // 0 = real, 1 = imag
} Hilbert;

static void Hilbert_postprocessing_ii(Hilbert *self) { POST_PROCESSING_II };
static void Hilbert_postprocessing_ai(Hilbert *self) { POST_PROCESSING_AI };
static void Hilbert_postprocessing_ia(Hilbert *self) { POST_PROCESSING_IA };
static void Hilbert_postprocessing_aa(Hilbert *self) { POST_PROCESSING_AA };
static void Hilbert_postprocessing_ireva(Hilbert *self) { POST_PROCESSING_IREVA };
static void Hilbert_postprocessing_areva(Hilbert *self) { POST_PROCESSING_AREVA };
static void Hilbert_postprocessing_revai(Hilbert *self) { POST_PROCESSING_REVAI };
static void Hilbert_postprocessing_revaa(Hilbert *self) { POST_PROCESSING_REVAA };
static void Hilbert_postprocessing_revareva(Hilbert *self) { POST_PROCESSING_REVAREVA };

static void
Hilbert_setProcMode(Hilbert *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_postprocessing_revareva);
            break;
    }
}

static void
Hilbert_compute_next_data_frame(Hilbert *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = HilbertMain_getSamplesBuffer((HilbertMain *)self->mainSplitter);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = tmp[i + offset];
    }

    (*self->muladd_func_ptr)(self);
}

static int
Hilbert_traverse(Hilbert *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
Hilbert_clear(Hilbert *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
Hilbert_dealloc(Hilbert* self)
{
    pyo_DEALLOC
    Hilbert_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Hilbert_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp = NULL, *multmp = NULL, *addtmp = NULL;
    Hilbert *self;
    self = (Hilbert *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Hilbert_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Hilbert_setProcMode);

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    self->mainSplitter = (HilbertMain *)maintmp;
    Py_INCREF(self->mainSplitter);

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

static PyObject * Hilbert_getServer(Hilbert* self) { GET_SERVER };
static PyObject * Hilbert_getStream(Hilbert* self) { GET_STREAM };
static PyObject * Hilbert_setMul(Hilbert *self, PyObject *arg) { SET_MUL };
static PyObject * Hilbert_setAdd(Hilbert *self, PyObject *arg) { SET_ADD };
static PyObject * Hilbert_setSub(Hilbert *self, PyObject *arg) { SET_SUB };
static PyObject * Hilbert_setDiv(Hilbert *self, PyObject *arg) { SET_DIV };

static PyObject * Hilbert_play(Hilbert *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Hilbert_out(Hilbert *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Hilbert_stop(Hilbert *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Hilbert_multiply(Hilbert *self, PyObject *arg) { MULTIPLY };
static PyObject * Hilbert_inplace_multiply(Hilbert *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Hilbert_add(Hilbert *self, PyObject *arg) { ADD };
static PyObject * Hilbert_inplace_add(Hilbert *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Hilbert_sub(Hilbert *self, PyObject *arg) { SUB };
static PyObject * Hilbert_inplace_sub(Hilbert *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Hilbert_div(Hilbert *self, PyObject *arg) { DIV };
static PyObject * Hilbert_inplace_div(Hilbert *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Hilbert_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Hilbert, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Hilbert, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Hilbert, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Hilbert, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Hilbert_methods[] =
{
    {"getServer", (PyCFunction)Hilbert_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Hilbert_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Hilbert_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Hilbert_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Hilbert_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Hilbert_setMul, METH_O, "Sets Hilbert mul factor."},
    {"setAdd", (PyCFunction)Hilbert_setAdd, METH_O, "Sets Hilbert add factor."},
    {"setSub", (PyCFunction)Hilbert_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Hilbert_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot HilbertType_slots[] = {
    {Py_tp_dealloc, Hilbert_dealloc},
    {Py_tp_doc, "Hilbert objects. Reads one band from a Hilbert transform."},
    {Py_tp_traverse, Hilbert_traverse},
    {Py_tp_clear, Hilbert_clear},
    {Py_tp_methods, Hilbert_methods},
    {Py_tp_members, Hilbert_members},
    {Py_tp_new, Hilbert_new},
    {Py_nb_add, Hilbert_add},
    {Py_nb_subtract, Hilbert_sub},
    {Py_nb_multiply, Hilbert_multiply},
    {Py_nb_true_divide, Hilbert_div},
    {Py_nb_inplace_add, Hilbert_inplace_add},
    {Py_nb_inplace_subtract, Hilbert_inplace_sub},
    {Py_nb_inplace_multiply, Hilbert_inplace_multiply},
    {Py_nb_inplace_true_divide, Hilbert_inplace_div},
    {0, NULL}
};

static PyType_Spec HilbertType_spec =
{
    "_pyo.Hilbert_base",
    sizeof(Hilbert),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    HilbertType_slots
};

PyTypeObject *
PyoCreateHilbertType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &HilbertType_spec, NULL);
}
