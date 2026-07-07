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
#include "lo/lo.h"

static void error(int num, const char *msg, const char *path)
{
    PySys_WriteStdout("liblo server error %d in path %s: %s\n", num, path, msg);
}

/* main OSC receiver */
typedef struct
{
    pyo_audio_HEAD
    lo_server osc_server;
    int port;
    PyObject *dict;
    PyObject *address_path;
} OscReceiver;

/* lo_method_handler' (aka 'int (*)(const char *, const char *, lo_arg **, int, struct lo_message_ *, void *)') */
int OscReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        lo_message data, void *user_data)
{
    OscReceiver *self = user_data;
    PyObject *pathObj = PyUnicode_FromString(path);
    PyObject *valueObj = PyFloat_FromDouble(argv[0]->FLOAT_VALUE);
    PyDict_SetItem(self->dict, pathObj, valueObj);
    Py_DECREF(pathObj);
    Py_DECREF(valueObj);
    return 0;
}

MYFLT OscReceiver_getValue(OscReceiver *self, PyObject *path)
{
    return PyFloat_AsDouble(PyDict_GetItem(self->dict, path));
}

static void
OscReceiver_compute_next_data_frame(OscReceiver *self)
{
    while (lo_server_recv_noblock(self->osc_server, 0) != 0) {};
}

static int
OscReceiver_traverse(OscReceiver *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->dict);
    Py_VISIT(self->address_path);
    return 0;
}

static int
OscReceiver_clear(OscReceiver *self)
{
    pyo_CLEAR
    Py_CLEAR(self->dict);
    Py_CLEAR(self->address_path);
    return 0;
}

static void
OscReceiver_dealloc(OscReceiver* self)
{
    lo_server_free(self->osc_server);
    pyo_DEALLOC
    OscReceiver_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp;
    OscReceiver *self;
    self = (OscReceiver *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscReceiver_compute_next_data_frame));

    static char *kwlist[] = {"port", "address", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO", kwlist, &self->port, &pathtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    self->dict = PyDict_New();

    if (PyList_Check(pathtmp))
    {
        self->address_path = pathtmp;
        Py_INCREF(self->address_path);
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "The OscReceiver_base 'address' attributes must be a list of strings and/or unicodes.");
        Py_RETURN_NONE;
    }

    int lsize = PyList_Size(self->address_path);

    PyObject *zero = PyFloat_FromDouble(0.);
    for (i = 0; i < lsize; i++)
    {
        PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), zero);
    }
    Py_DECREF(zero);

    char buf[20];
    sprintf(buf, "%i", self->port);
    self->osc_server = lo_server_new(buf, error);

    lo_server_add_method(self->osc_server, NULL, TYPE_F, OscReceiver_handler, self);

    return (PyObject *)self;
}

static PyObject *
OscReceiver_addAddress(OscReceiver *self, PyObject *arg)
{
    int i;

    if (PyUnicode_Check(arg))
    {
        PyObject *zero = PyFloat_FromDouble(0.);
        PyDict_SetItem(self->dict, arg, zero);
        Py_DECREF(zero);
    }
    else if (PyList_Check(arg))
    {
        Py_ssize_t lsize = PyList_Size(arg);

        PyObject *zero = PyFloat_FromDouble(0.);
        for (i = 0; i < lsize; i++)
        {
            PyDict_SetItem(self->dict, PyList_GET_ITEM(arg, i), zero);
        }
        Py_DECREF(zero);
    }

    Py_RETURN_NONE;
}

static PyObject *
OscReceiver_delAddress(OscReceiver *self, PyObject *arg)
{
    int i;

    if (PyUnicode_Check(arg))
    {
        PyDict_DelItem(self->dict, arg);
    }
    else if (PyList_Check(arg))
    {
        Py_ssize_t lsize = PyList_Size(arg);

        for (i = 0; i < lsize; i++)
        {
            PyDict_DelItem(self->dict, PyList_GET_ITEM(arg, i));
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
OscReceiver_setValue(OscReceiver *self, PyObject *args, PyObject *kwds)
{
    PyObject *address, *value;

    static char *kwlist[] = {"address", "value", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &address, &value))
        Py_RETURN_NONE;

    PyDict_SetItem(self->dict, address, value);
    Py_RETURN_NONE;
}

static PyObject * OscReceiver_getServer(OscReceiver* self) { GET_SERVER };
static PyObject * OscReceiver_getStream(OscReceiver* self) { GET_STREAM };

static PyObject * OscReceiver_play(OscReceiver *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscReceiver_stop(OscReceiver *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef OscReceiver_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscReceiver, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscReceiver, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscReceiver_methods[] =
{
    {"getServer", (PyCFunction)OscReceiver_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscReceiver_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscReceiver_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscReceiver_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"addAddress", (PyCFunction)OscReceiver_addAddress, METH_O, "Add a new address to the dictionary."},
    {"delAddress", (PyCFunction)OscReceiver_delAddress, METH_O, "Remove an address from the dictionary."},
    {"setValue", (PyCFunction)OscReceiver_setValue, METH_VARARGS | METH_KEYWORDS, "Sets value for a specified address."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscReceiverType_slots[] =
{
    {Py_tp_dealloc, OscReceiver_dealloc},
    {Py_tp_doc, "OscReceiver objects. Receive values via Open Sound Control protocol."},
    {Py_tp_traverse, OscReceiver_traverse},
    {Py_tp_clear, OscReceiver_clear},
    {Py_tp_methods, OscReceiver_methods},
    {Py_tp_members, OscReceiver_members},
    {Py_tp_new, OscReceiver_new},
    {0, NULL}
};

static PyType_Spec OscReceiverType_spec =
{
    "_pyo.OscReceiver_base",
    sizeof(OscReceiver),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscReceiverType_slots
};

PyTypeObject *
PyoCreateOscReceiverType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscReceiverType_spec, NULL);
}

/* OSC receiver stream object */
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PyObject *address_path;
    MYFLT value;
    MYFLT factor;
    int interpolation;
    int modebuffer[2];
} OscReceive;

static void OscReceive_postprocessing_ii(OscReceive *self) { POST_PROCESSING_II };
static void OscReceive_postprocessing_ai(OscReceive *self) { POST_PROCESSING_AI };
static void OscReceive_postprocessing_ia(OscReceive *self) { POST_PROCESSING_IA };
static void OscReceive_postprocessing_aa(OscReceive *self) { POST_PROCESSING_AA };
static void OscReceive_postprocessing_ireva(OscReceive *self) { POST_PROCESSING_IREVA };
static void OscReceive_postprocessing_areva(OscReceive *self) { POST_PROCESSING_AREVA };
static void OscReceive_postprocessing_revai(OscReceive *self) { POST_PROCESSING_REVAI };
static void OscReceive_postprocessing_revaa(OscReceive *self) { POST_PROCESSING_REVAA };
static void OscReceive_postprocessing_revareva(OscReceive *self) { POST_PROCESSING_REVAREVA };

static void
OscReceive_setProcMode(OscReceive *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_postprocessing_revareva);
            break;
    }
}

static void
OscReceive_compute_next_data_frame(OscReceive *self)
{
    int i;
    MYFLT val = OscReceiver_getValue((OscReceiver *)self->input, self->address_path);

    if (self->interpolation == 1)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value = self->value + (val - self->value) * self->factor;
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value = val;
        }
    }

    (*self->muladd_func_ptr)(self);
}

static int
OscReceive_traverse(OscReceive *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->address_path);
    return 0;
}

static int
OscReceive_clear(OscReceive *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->address_path);
    return 0;
}

static void
OscReceive_dealloc(OscReceive* self)
{
    pyo_DEALLOC
    OscReceive_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp = NULL, *pathtmp = NULL, *multmp = NULL, *addtmp = NULL;;
    OscReceive *self;
    self = (OscReceive *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = 0.;
    self->interpolation = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    self->factor = 1. / (0.01 * self->sr);

    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscReceive_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(OscReceive_setProcMode);

    static char *kwlist[] = {"input", "address", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &pathtmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    self->input = inputtmp;
    Py_INCREF(self->input);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (!PyUnicode_Check(pathtmp))
    {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a unicode.");
        Py_RETURN_NONE;
    }

    self->address_path = pathtmp;
    Py_INCREF(self->address_path);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
OscReceive_setInterpolation(OscReceive *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->interpolation = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject * OscReceive_getServer(OscReceive* self) { GET_SERVER };
static PyObject * OscReceive_getStream(OscReceive* self) { GET_STREAM };
static PyObject * OscReceive_setMul(OscReceive *self, PyObject *arg) { SET_MUL };
static PyObject * OscReceive_setAdd(OscReceive *self, PyObject *arg) { SET_ADD };
static PyObject * OscReceive_setSub(OscReceive *self, PyObject *arg) { SET_SUB };
static PyObject * OscReceive_setDiv(OscReceive *self, PyObject *arg) { SET_DIV };

static PyObject * OscReceive_play(OscReceive *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscReceive_stop(OscReceive *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * OscReceive_multiply(OscReceive *self, PyObject *arg) { MULTIPLY };
static PyObject * OscReceive_inplace_multiply(OscReceive *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscReceive_add(OscReceive *self, PyObject *arg) { ADD };
static PyObject * OscReceive_inplace_add(OscReceive *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscReceive_sub(OscReceive *self, PyObject *arg) { SUB };
static PyObject * OscReceive_inplace_sub(OscReceive *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscReceive_div(OscReceive *self, PyObject *arg) { DIV };
static PyObject * OscReceive_inplace_div(OscReceive *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef OscReceive_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscReceive, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(OscReceive, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscReceive, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscReceive_methods[] =
{
    {"getServer", (PyCFunction)OscReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscReceive_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscReceive_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setInterpolation", (PyCFunction)OscReceive_setInterpolation, METH_O, "Sets interpolation on or off."},
    {"setMul", (PyCFunction)OscReceive_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscReceive_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscReceive_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)OscReceive_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscReceiveType_slots[] =
{
    {Py_tp_dealloc, OscReceive_dealloc},
    {Py_tp_doc, "OscReceive objects. Receive values via Open Sound Control protocol."},
    {Py_tp_traverse, OscReceive_traverse},
    {Py_tp_clear, OscReceive_clear},
    {Py_tp_methods, OscReceive_methods},
    {Py_tp_members, OscReceive_members},
    {Py_nb_add, OscReceive_add},
    {Py_nb_subtract, OscReceive_sub},
    {Py_nb_multiply, OscReceive_multiply},
    {Py_nb_true_divide, OscReceive_div},
    {Py_nb_inplace_add, OscReceive_inplace_add},
    {Py_nb_inplace_subtract, OscReceive_inplace_sub},
    {Py_nb_inplace_multiply, OscReceive_inplace_multiply},
    {Py_nb_inplace_true_divide, OscReceive_inplace_div},
    {Py_tp_new, OscReceive_new},
    {0, NULL}
};

static PyType_Spec OscReceiveType_spec =
{
    "_pyo.OscReceive_base",
    sizeof(OscReceive),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscReceiveType_slots
};

PyTypeObject *
PyoCreateOscReceiveType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscReceiveType_spec, NULL);
}

/* OSC send object */
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *address_path;
    lo_address address;
    char *host;
    int port;
    int count;
    int bufrate;
} OscSend;

static void
OscSend_compute_next_data_frame(OscSend *self)
{
    const char *path = NULL;
    self->count++;

    if (self->count >= self->bufrate)
    {
        self->count = 0;
        MYFLT *in = Stream_getData((Stream *)self->input_stream);
        float value = (float)in[0];

        if (PyBytes_Check(self->address_path))
            path = PyBytes_AsString(self->address_path);
        else
            path = PyUnicode_AsUTF8(self->address_path);

        if (lo_send(self->address, path, "f", value) == -1)
        {
            PySys_WriteStdout("OSC error %d: %s\n",
                              lo_address_errno(self->address),
                              lo_address_errstr(self->address));
        }
    }
}

static int
OscSend_traverse(OscSend *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->address_path);
    Py_VISIT(self->input);
    return 0;
}

static int
OscSend_clear(OscSend *self)
{
    pyo_CLEAR
    Py_CLEAR(self->address_path);
    Py_CLEAR(self->input);
    return 0;
}

static void
OscSend_dealloc(OscSend* self)
{
    pyo_DEALLOC
    lo_address_free(self->address);
    OscSend_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pathtmp;
    OscSend *self;
    self = (OscSend *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->host = NULL;
    self->count = 0;
    self->bufrate = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscSend_compute_next_data_frame));

    static char *kwlist[] = {"input", "port", "address", "host", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OiO|s", kwlist, &inputtmp, &self->port, &pathtmp, &self->host)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (!PyUnicode_Check(pathtmp))
    {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a unicode (bytes or string in Python 3).");
        Py_RETURN_NONE;
    }

    Py_INCREF(pathtmp);
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;

    char buf[20];
    sprintf(buf, "%i", self->port);
    self->address = lo_address_new(self->host, buf);

    return (PyObject *)self;
}

static PyObject *
OscSend_setBufferRate(OscSend *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->bufrate = PyLong_AsLong(arg);

    if (self->bufrate < 1)
        self->bufrate = 1;

    Py_RETURN_NONE;
}

static PyObject * OscSend_getServer(OscSend* self) { GET_SERVER };
static PyObject * OscSend_getStream(OscSend* self) { GET_STREAM };

static PyObject * OscSend_play(OscSend *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscSend_stop(OscSend *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef OscSend_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscSend, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscSend, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(OscSend, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscSend_methods[] =
{
    {"getServer", (PyCFunction)OscSend_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscSend_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscSend_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscSend_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setBufferRate", (PyCFunction)OscSend_setBufferRate, METH_O, "Set how many buffers to wait before sending a new value."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscSendType_slots[] =
{
    {Py_tp_dealloc, OscSend_dealloc},
    {Py_tp_doc, "OscSend objects. Send values via Open Sound Control protocol."},
    {Py_tp_traverse, OscSend_traverse},
    {Py_tp_clear, OscSend_clear},
    {Py_tp_methods, OscSend_methods},
    {Py_tp_members, OscSend_members},
    {Py_tp_new, OscSend_new},
    {0, NULL}
};

static PyType_Spec OscSendType_spec =
{
    "_pyo.OscSend_base",
    sizeof(OscSend),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscSendType_slots
};

PyTypeObject *
PyoCreateOscSendType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscSendType_spec, NULL);
}

/* OscDataSend object */
typedef struct
{
    pyo_audio_HEAD
    PyObject *value;
    PyObject *address_path;
    lo_address address;
    char *host;
    char *types;
    int port;
    int something_to_send;
    int num_items;
} OscDataSend;

static void
OscDataSend_compute_next_data_frame(OscDataSend *self)
{
    int i, j = 0;
    Py_ssize_t blobsize = 0;
    PyObject *inlist = NULL;
    PyObject *datalist = NULL;
    char *blobdata = NULL;
    uint8_t midi[4];
    lo_blob blob = NULL;
    const char *path = NULL;
    lo_message msg;

    while (self->something_to_send)
    {
        if (PyBytes_Check(self->address_path))
            path = PyBytes_AsString(self->address_path);
        else
            path = PyUnicode_AsUTF8(self->address_path);

        msg = lo_message_new();

        self->something_to_send--;
        inlist = PyList_GetItem(self->value, self->something_to_send);

        for (i = 0; i < self->num_items; i++)
        {
            switch (self->types[i])
            {
                case LO_INT32:
                    lo_message_add_int32(msg, PyLong_AsLong(PyList_GET_ITEM(inlist, i)));
                    break;

                case LO_INT64:
                    lo_message_add_int64(msg, (long)PyLong_AsLong(PyList_GET_ITEM(inlist, i)));
                    break;

                case LO_FLOAT:
                    lo_message_add_float(msg, (float)PyFloat_AsDouble(PyList_GET_ITEM(inlist, i)));
                    break;

                case LO_DOUBLE:
                    lo_message_add_double(msg, (double)PyFloat_AsDouble(PyList_GET_ITEM(inlist, i)));
                    break;

                case LO_STRING:
                    lo_message_add_string(msg, PyUnicode_AsUTF8(PyList_GET_ITEM(inlist, i)));
                    break;

                case LO_CHAR:
                    lo_message_add_char(msg, (char)PyUnicode_AsUTF8(PyList_GET_ITEM(inlist, i))[0]);
                    break;

                case LO_BLOB:
                    datalist = PyList_GET_ITEM(inlist, i);
                    blobsize = PyList_Size(datalist);
                    blobdata = (char *)PyMem_RawMalloc(blobsize * sizeof(char));

                    for (j = 0; j < blobsize; j++)
                    {
                        blobdata[j] = (char)PyUnicode_AsUTF8(PyList_GET_ITEM(datalist, j))[0];
                    }

                    blob = lo_blob_new(blobsize * sizeof(char), blobdata);
                    lo_message_add_blob(msg, blob);
                    break;

                case LO_MIDI:
                    datalist = PyList_GET_ITEM(inlist, i);

                    for (j = 0; j < 4; j++)
                    {
                        midi[j] = (uint8_t)PyLong_AsLong(PyList_GET_ITEM(datalist, j));
                    }

                    lo_message_add_midi(msg, midi);
                    break;

                case LO_NIL:
                    lo_message_add_nil(msg);
                    break;

                case LO_TRUE:
                    lo_message_add_true(msg);
                    break;

                case LO_FALSE:
                    lo_message_add_false(msg);
                    break;

                default:
                    break;
            }
        }

        if (lo_send_message(self->address, path, msg) == -1)
        {
            PySys_WriteStdout("OSC error %d: %s\n", lo_address_errno(self->address),
                              lo_address_errstr(self->address));
        }

        Py_DECREF(inlist);
        PySequence_DelItem(self->value, self->something_to_send);
        lo_message_free(msg);

        if (blob != NULL)
            lo_blob_free(blob);

        if (blobdata != NULL)
            PyMem_RawFree(blobdata);
    }
}

static int
OscDataSend_traverse(OscDataSend *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->value);
    Py_VISIT(self->address_path);
    return 0;
}

static int
OscDataSend_clear(OscDataSend *self)
{
    pyo_CLEAR
    Py_CLEAR(self->value);
    Py_CLEAR(self->address_path);
    return 0;
}

static void
OscDataSend_dealloc(OscDataSend* self)
{
    pyo_DEALLOC
    lo_address_free(self->address);
    OscDataSend_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscDataSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp;
    OscDataSend *self;
    self = (OscDataSend *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->value = PyList_New(0);
    self->something_to_send = 0;
    self->host = NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscDataSend_compute_next_data_frame));

    static char *kwlist[] = {"types", "port", "address", "host", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "siO|s", kwlist, &self->types, &self->port, &pathtmp, &self->host)) {
        Py_DECREF(self);
        return NULL;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (!PyUnicode_Check(pathtmp))
    {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be of type string or unicode (bytes or string in Python 3).");
        Py_RETURN_NONE;
    }

    self->num_items = strlen(self->types);

    Py_INCREF(pathtmp);
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;

    char buf[20];
    sprintf(buf, "%i", self->port);
    self->address = lo_address_new(self->host, buf);

    return (PyObject *)self;
}

static PyObject * OscDataSend_getServer(OscDataSend* self) { GET_SERVER };
static PyObject * OscDataSend_getStream(OscDataSend* self) { GET_STREAM };

static PyObject * OscDataSend_play(OscDataSend *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscDataSend_stop(OscDataSend *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
OscDataSend_send(OscDataSend *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyList_Check(arg))
    {
        Py_INCREF(arg);
        PyList_Append(self->value, arg);
        self->something_to_send++;
    }
    else
        PySys_WriteStdout("OscDataSend: argument to send() method must be a list of values.\n");

    Py_RETURN_NONE;
}

static PyMemberDef OscDataSend_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscDataSend, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscDataSend, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscDataSend_methods[] =
{
    {"getServer", (PyCFunction)OscDataSend_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscDataSend_getStream, METH_NOARGS, "Returns stream object."},
    {"send", (PyCFunction)OscDataSend_send, METH_O, "Sets values to be sent."},
    {"play", (PyCFunction)OscDataSend_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscDataSend_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscDataSendType_slots[] =
{
    {Py_tp_dealloc, OscDataSend_dealloc},
    {Py_tp_doc, "OscDataSend objects. Send data values via Open Sound Control protocol."},
    {Py_tp_traverse, OscDataSend_traverse},
    {Py_tp_clear, OscDataSend_clear},
    {Py_tp_methods, OscDataSend_methods},
    {Py_tp_members, OscDataSend_members},
    {Py_tp_new, OscDataSend_new},
    {0, NULL}
};

static PyType_Spec OscDataSendType_spec =
{
    "_pyo.OscDataSend_base",
    sizeof(OscDataSend),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscDataSendType_slots
};

PyTypeObject *
PyoCreateOscDataSendType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscDataSendType_spec, NULL);
}

/* main OscDataReceive */
typedef struct
{
    pyo_audio_HEAD
    lo_server osc_server;
    PyObject *address_path;
    PyObject *callable;
    int port;
} OscDataReceive;

/* lo_method_handler' (aka 'int (*)(const char *, const char *, lo_arg **, int, struct lo_message_ *, void *)') */
int OscDataReceive_handler(const char *path, const char *types, lo_arg **argv, int argc,
                           lo_message data, void *user_data)
{
    OscDataReceive *self = user_data;
    PyObject *tup, *result = NULL;
    lo_blob blob = NULL;
    char *blobdata = NULL;
    uint32_t blobsize = 0;
    PyObject *charlist = NULL;
    tup = PyTuple_New(argc + 1);
    int i, ok = 0;
    uint32_t j = 0;

    Py_ssize_t lsize = PyList_Size(self->address_path);

    for (i = 0; i < lsize; i++)
    {
        if (PyBytes_Check(PyList_GET_ITEM(self->address_path, i)))
        {
            if (lo_pattern_match(path, PyBytes_AsString(PyList_GET_ITEM(self->address_path, i))))
            {
                ok = 1;
                break;
            }
        }
        else
        {
            if (lo_pattern_match(path, PyUnicode_AsUTF8(PyList_GET_ITEM(self->address_path, i))))
            {
                ok = 1;
                break;
            }
        }
    }

    if (ok)
    {
        PyTuple_SET_ITEM(tup, 0, PyUnicode_FromString(path));

        for (i = 0; i < argc; i++)
        {
            switch (types[i])
            {
                case LO_INT32:
                    PyTuple_SET_ITEM(tup, i + 1, PyLong_FromLong(argv[i]->i));
                    break;

                case LO_INT64:
                    PyTuple_SET_ITEM(tup, i + 1, PyLong_FromLong(argv[i]->h));
                    break;

                case LO_FLOAT:
                    PyTuple_SET_ITEM(tup, i + 1, PyFloat_FromDouble(argv[i]->f));
                    break;

                case LO_DOUBLE:
                    PyTuple_SET_ITEM(tup, i + 1, PyFloat_FromDouble(argv[i]->d));
                    break;

                case LO_STRING:
                    PyTuple_SET_ITEM(tup, i + 1, PyUnicode_FromString(&argv[i]->s));
                    break;

                case LO_CHAR:
                    PyTuple_SET_ITEM(tup, i + 1, PyUnicode_FromFormat("%c", argv[i]->c));
                    break;

                case LO_BLOB:
                    blob = (lo_blob)argv[i];
                    blobsize = lo_blob_datasize(blob);
                    blobdata = lo_blob_dataptr(blob);
                    charlist = PyList_New(blobsize);

                    for (j = 0; j < blobsize; j++)
                    {
                        PyList_SET_ITEM(charlist, j, PyUnicode_FromFormat("%c", blobdata[j]));
                    }

                    PyTuple_SET_ITEM(tup, i + 1, charlist);
                    break;

                case LO_MIDI:
                    charlist = PyList_New(4);

                    for (j = 0; j < 4; j++)
                    {
                        PyList_SET_ITEM(charlist, j, PyLong_FromLong(argv[i]->m[j]));
                    }

                    PyTuple_SET_ITEM(tup, i + 1, charlist);
                    break;

                case LO_NIL:
                    Py_INCREF(Py_None);
                    PyTuple_SET_ITEM(tup, i + 1, Py_None);
                    break;

                case LO_TRUE:
                    Py_INCREF(Py_True);
                    PyTuple_SET_ITEM(tup, i + 1, Py_True);
                    break;

                case LO_FALSE:
                    Py_INCREF(Py_False);
                    PyTuple_SET_ITEM(tup, i + 1, Py_False);
                    break;

                default:
                    break;
            }
        }

        result = PyObject_Call(self->callable, tup, NULL);

        if (result == NULL)
            PyErr_Print();
    }

    Py_XDECREF(tup);
    Py_XDECREF(result);
    Py_XDECREF(charlist);
    return 0;
}

static void
OscDataReceive_compute_next_data_frame(OscDataReceive *self)
{
    while (lo_server_recv_noblock(self->osc_server, 0) != 0) {};
}

static int
OscDataReceive_traverse(OscDataReceive *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->address_path);
    Py_VISIT(self->callable);
    return 0;
}

static int
OscDataReceive_clear(OscDataReceive *self)
{
    pyo_CLEAR
    Py_CLEAR(self->address_path);
    Py_CLEAR(self->callable);
    return 0;
}

static void
OscDataReceive_dealloc(OscDataReceive* self)
{
    lo_server_free(self->osc_server);
    pyo_DEALLOC
    OscDataReceive_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscDataReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp, *calltmp;
    OscDataReceive *self;
    self = (OscDataReceive *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscDataReceive_compute_next_data_frame));

    static char *kwlist[] = {"port", "address", "callable", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iOO", kwlist, &self->port, &pathtmp, &calltmp)) {
        Py_DECREF(self);
        return NULL;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Py_XDECREF(self->callable);
    self->callable = calltmp;
    Py_INCREF(self->callable);

    if (PyList_Check(pathtmp))
    {
        Py_INCREF(pathtmp);
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a list of strings and/or unicodes.");
        Py_RETURN_NONE;
    }

    char buf[20];
    sprintf(buf, "%i", self->port);
    self->osc_server = lo_server_new(buf, error);

    lo_server_add_method(self->osc_server, NULL, NULL, OscDataReceive_handler, self);

    return (PyObject *)self;
}

static PyObject * OscDataReceive_getServer(OscDataReceive* self) { GET_SERVER };
static PyObject * OscDataReceive_getStream(OscDataReceive* self) { GET_STREAM };
static PyObject * OscDataReceive_play(OscDataReceive *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscDataReceive_stop(OscDataReceive *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
OscDataReceive_addAddress(OscDataReceive *self, PyObject *arg)
{
    int i;

    if (arg != NULL)
    {
        if (PyUnicode_Check(arg))
            PyList_Append(self->address_path, arg);
        else if (PyList_Check(arg))
        {
            Py_ssize_t len = PyList_Size(arg);

            for (i = 0; i < len; i++)
            {
                PyList_Append(self->address_path, PyList_GET_ITEM(arg, i));
            }
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
OscDataReceive_delAddress(OscDataReceive *self, PyObject *arg)
{
    if (arg != NULL)
    {
        if (PyLong_Check(arg))
        {
            PySequence_DelItem(self->address_path, PyLong_AsLong(arg));
        }
    }

    Py_RETURN_NONE;
}

static PyMemberDef OscDataReceive_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscDataReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscDataReceive, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscDataReceive_methods[] =
{
    {"getServer", (PyCFunction)OscDataReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscDataReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscDataReceive_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscDataReceive_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"addAddress", (PyCFunction)OscDataReceive_addAddress, METH_O, "Add new paths to the object."},
    {"delAddress", (PyCFunction)OscDataReceive_delAddress, METH_O, "Remove path from the object."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscDataReceiveType_slots[] =
{
    {Py_tp_dealloc, OscDataReceive_dealloc},
    {Py_tp_doc, "OscDataReceive objects. Receive values via Open Sound Control protocol."},
    {Py_tp_traverse, OscDataReceive_traverse},
    {Py_tp_clear, OscDataReceive_clear},
    {Py_tp_methods, OscDataReceive_methods},
    {Py_tp_members, OscDataReceive_members},
    {Py_tp_new, OscDataReceive_new},
    {0, NULL}
};

static PyType_Spec OscDataReceiveType_spec =
{
    "_pyo.OscDataReceive_base",
    sizeof(OscDataReceive),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscDataReceiveType_slots
};

PyTypeObject *
PyoCreateOscDataReceiveType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscDataReceiveType_spec, NULL);
}

/* main OSC list receiver */
typedef struct
{
    pyo_audio_HEAD
    lo_server osc_server;
    PyObject *dict;
    PyObject *address_path;
    int port;
    int num;
} OscListReceiver;

/* lo_method_handler' (aka 'int (*)(const char *, const char *, lo_arg **, int, struct lo_message_ *, void *)') */
int OscListReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                            lo_message data, void *user_data)
{
    OscListReceiver *self = user_data;

    int i;
    PyObject *flist;
    flist = PyList_New(self->num);

    for (i = 0; i < self->num; i++)
    {
        PyObject *valueObj = PyFloat_FromDouble(argv[i]->FLOAT_VALUE);
        PyList_SET_ITEM(flist, i, valueObj);
        Py_DECREF(valueObj);
    }

    PyObject *pathObj = PyUnicode_FromString(path);
    PyDict_SetItem(self->dict, pathObj, flist);
    Py_DECREF(pathObj);
    Py_DECREF(flist);
    return 0;
}

PyObject *
OscListReceiver_getValue(OscListReceiver *self, PyObject *path)
{
    PyObject *value = PyDict_GetItem(self->dict, path);
    Py_INCREF(value);
    return value;
}

static void
OscListReceiver_compute_next_data_frame(OscListReceiver *self)
{
    while (lo_server_recv_noblock(self->osc_server, 0) != 0) {};
}

static int
OscListReceiver_traverse(OscListReceiver *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->dict);
    Py_VISIT(self->address_path);
    return 0;
}

static int
OscListReceiver_clear(OscListReceiver *self)
{
    pyo_CLEAR
    Py_CLEAR(self->dict);
    Py_CLEAR(self->address_path);
    return 0;
}

static void
OscListReceiver_dealloc(OscListReceiver* self)
{
    lo_server_free(self->osc_server);
    pyo_DEALLOC
    OscListReceiver_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscListReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *pathtmp, *flist;
    OscListReceiver *self;
    self = (OscListReceiver *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->num = 8;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscListReceiver_compute_next_data_frame));

    static char *kwlist[] = {"port", "address", "num", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO|i", kwlist, &self->port, &pathtmp, &self->num)) {
        Py_DECREF(self);
        return NULL;
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    Py_XDECREF(self->dict);
    self->dict = PyDict_New();
    Py_INCREF(self->dict);

    if (PyList_Check(pathtmp))
    {
        Py_INCREF(pathtmp);
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else
    {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a list of strings and/or unicodes.");
        Py_RETURN_NONE;
    }

    int lsize = PyList_Size(self->address_path);

    PyObject *zero = PyFloat_FromDouble(0.);
    for (i = 0; i < lsize; i++)
    {
        flist = PyList_New(self->num);

        for (j = 0; j < self->num; j++)
        {
            PyList_SET_ITEM(flist, j, zero);
        }

        PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), flist);
        Py_DECREF(flist);
    }
    Py_DECREF(zero);

    char buf[20];
    sprintf(buf, "%i", self->port);
    self->osc_server = lo_server_new(buf, error);

    lo_server_add_method(self->osc_server, NULL, NULL, OscListReceiver_handler, self);

    return (PyObject *)self;
}

static PyObject *
OscListReceiver_addAddress(OscListReceiver *self, PyObject *arg)
{
    PyObject *flist;
    int i, j;

    if (PyUnicode_Check(arg))
    {
        flist = PyList_New(self->num);

        PyObject *zero = PyFloat_FromDouble(0.);
        for (j = 0; j < self->num; j++)
        {
            PyList_SET_ITEM(flist, j, zero);
        }
        Py_DECREF(zero);

        PyDict_SetItem(self->dict, arg, flist);
    }
    else if (PyList_Check(arg))
    {
        Py_ssize_t lsize = PyList_Size(arg);

        PyObject *zero = PyFloat_FromDouble(0.);
        for (i = 0; i < lsize; i++)
        {
            flist = PyList_New(self->num);

            for (j = 0; j < self->num; j++)
            {
                PyList_SET_ITEM(flist, j, zero);
            }

            PyDict_SetItem(self->dict, PyList_GET_ITEM(arg, i), flist);
        }
        Py_DECREF(zero);
    }

    Py_RETURN_NONE;
}

static PyObject *
OscListReceiver_delAddress(OscListReceiver *self, PyObject *arg)
{
    int i;

    if (PyUnicode_Check(arg))
    {
        PyDict_DelItem(self->dict, arg);
    }
    else if (PyList_Check(arg))
    {
        Py_ssize_t lsize = PyList_Size(arg);

        for (i = 0; i < lsize; i++)
        {
            if (PyDict_Contains(self->dict, PyList_GET_ITEM(arg, i)))
                PyDict_DelItem(self->dict, PyList_GET_ITEM(arg, i));
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
OscListReceiver_setValue(OscListReceiver *self, PyObject *args, PyObject *kwds)
{
    PyObject *address, *value;

    static char *kwlist[] = {"address", "value", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &address, &value))
        Py_RETURN_NONE;

    PyDict_SetItem(self->dict, address, value);
    Py_RETURN_NONE;
}

static PyObject * OscListReceiver_getServer(OscListReceiver* self) { GET_SERVER };
static PyObject * OscListReceiver_getStream(OscListReceiver* self) { GET_STREAM };

static PyObject * OscListReceiver_play(OscListReceiver *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscListReceiver_stop(OscListReceiver *self, PyObject *args, PyObject *kwds) { STOP };

static PyMemberDef OscListReceiver_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscListReceiver, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscListReceiver, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscListReceiver_methods[] =
{
    {"getServer", (PyCFunction)OscListReceiver_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscListReceiver_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscListReceiver_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscListReceiver_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"addAddress", (PyCFunction)OscListReceiver_addAddress, METH_O, "Add a new address to the dictionary."},
    {"delAddress", (PyCFunction)OscListReceiver_delAddress, METH_O, "Remove an address from the dictionary."},
    {"setValue", (PyCFunction)OscListReceiver_setValue, METH_VARARGS | METH_KEYWORDS, "Sets value for a specified address."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscListReceiverType_slots[] =
{
    {Py_tp_dealloc, OscListReceiver_dealloc},
    {Py_tp_doc, "OscListReceiver objects. Receive list of values via Open Sound Control protocol."},
    {Py_tp_traverse, OscListReceiver_traverse},
    {Py_tp_clear, OscListReceiver_clear},
    {Py_tp_methods, OscListReceiver_methods},
    {Py_tp_members, OscListReceiver_members},
    {Py_tp_new, OscListReceiver_new},
    {0, NULL}
};

static PyType_Spec OscListReceiverType_spec =
{
    "_pyo.OscListReceiver_base",
    sizeof(OscListReceiver),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscListReceiverType_slots
};

PyTypeObject *
PyoCreateOscListReceiverType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscListReceiverType_spec, NULL);
}

/* OSC list receiver stream object */
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    PyObject *address_path;
    MYFLT value;
    MYFLT factor;
    int order;
    int interpolation;
    int modebuffer[2];
} OscListReceive;

static void OscListReceive_postprocessing_ii(OscListReceive *self) { POST_PROCESSING_II };
static void OscListReceive_postprocessing_ai(OscListReceive *self) { POST_PROCESSING_AI };
static void OscListReceive_postprocessing_ia(OscListReceive *self) { POST_PROCESSING_IA };
static void OscListReceive_postprocessing_aa(OscListReceive *self) { POST_PROCESSING_AA };
static void OscListReceive_postprocessing_ireva(OscListReceive *self) { POST_PROCESSING_IREVA };
static void OscListReceive_postprocessing_areva(OscListReceive *self) { POST_PROCESSING_AREVA };
static void OscListReceive_postprocessing_revai(OscListReceive *self) { POST_PROCESSING_REVAI };
static void OscListReceive_postprocessing_revaa(OscListReceive *self) { POST_PROCESSING_REVAA };
static void OscListReceive_postprocessing_revareva(OscListReceive *self) { POST_PROCESSING_REVAREVA };

static void
OscListReceive_setProcMode(OscListReceive *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_postprocessing_revareva);
            break;
    }
}

static void
OscListReceive_compute_next_data_frame(OscListReceive *self)
{
    int i;
    PyObject *flist = OscListReceiver_getValue((OscListReceiver *)self->input, self->address_path);
    MYFLT val = PyFloat_AsDouble(PyList_GET_ITEM(flist, self->order));

    if (self->interpolation == 1)
    {

        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value = self->value + (val - self->value) * self->factor;
        }
    }
    else
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->data[i] = self->value = val;
        }
    }

    (*self->muladd_func_ptr)(self);
}

static int
OscListReceive_traverse(OscListReceive *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->address_path);
    return 0;
}

static int
OscListReceive_clear(OscListReceive *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->address_path);
    return 0;
}

static void
OscListReceive_dealloc(OscListReceive* self)
{
    pyo_DEALLOC
    OscListReceive_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscListReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp = NULL, *pathtmp = NULL, *multmp = NULL, *addtmp = NULL;;
    OscListReceive *self;
    self = (OscListReceive *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->order = 0;
    self->value = 0.;
    self->interpolation = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    self->factor = 1. / (0.01 * self->sr);

    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(OscListReceive_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(OscListReceive_setProcMode);

    static char *kwlist[] = {"input", "address", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &pathtmp, &self->order, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    self->input = inputtmp;
    Py_INCREF(self->input);

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    if (!PyUnicode_Check(pathtmp))
    {
        PyErr_SetString(PyExc_TypeError, "OscListReceive: the address attributes must be a string or a unicode.");
        Py_RETURN_NONE;
    }

    self->address_path = pathtmp;
    Py_INCREF(self->address_path);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
OscListReceive_setInterpolation(OscListReceive *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->interpolation = PyLong_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject * OscListReceive_getServer(OscListReceive* self) { GET_SERVER };
static PyObject * OscListReceive_getStream(OscListReceive* self) { GET_STREAM };
static PyObject * OscListReceive_setMul(OscListReceive *self, PyObject *arg) { SET_MUL };
static PyObject * OscListReceive_setAdd(OscListReceive *self, PyObject *arg) { SET_ADD };
static PyObject * OscListReceive_setSub(OscListReceive *self, PyObject *arg) { SET_SUB };
static PyObject * OscListReceive_setDiv(OscListReceive *self, PyObject *arg) { SET_DIV };

static PyObject * OscListReceive_play(OscListReceive *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscListReceive_stop(OscListReceive *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * OscListReceive_multiply(OscListReceive *self, PyObject *arg) { MULTIPLY };
static PyObject * OscListReceive_inplace_multiply(OscListReceive *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscListReceive_add(OscListReceive *self, PyObject *arg) { ADD };
static PyObject * OscListReceive_inplace_add(OscListReceive *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscListReceive_sub(OscListReceive *self, PyObject *arg) { SUB };
static PyObject * OscListReceive_inplace_sub(OscListReceive *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscListReceive_div(OscListReceive *self, PyObject *arg) { DIV };
static PyObject * OscListReceive_inplace_div(OscListReceive *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef OscListReceive_members[] =
{
    {"server", T_OBJECT_EX, offsetof(OscListReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscListReceive, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(OscListReceive, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscListReceive, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscListReceive_methods[] =
{
    {"getServer", (PyCFunction)OscListReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscListReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscListReceive_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscListReceive_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setInterpolation", (PyCFunction)OscListReceive_setInterpolation, METH_O, "Sets interpolation on or off."},
    {"setMul", (PyCFunction)OscListReceive_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscListReceive_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscListReceive_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)OscListReceive_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot OscListReceiveType_slots[] =
{
    {Py_tp_dealloc, OscListReceive_dealloc},
    {Py_tp_doc, "OscListReceive objects. Receive one value from a list of floats via Open Sound Control protocol."},
    {Py_tp_traverse, OscListReceive_traverse},
    {Py_tp_clear, OscListReceive_clear},
    {Py_tp_methods, OscListReceive_methods},
    {Py_tp_members, OscListReceive_members},
    {Py_nb_add, OscListReceive_add},
    {Py_nb_subtract, OscListReceive_sub},
    {Py_nb_multiply, OscListReceive_multiply},
    {Py_nb_true_divide, OscListReceive_div},
    {Py_nb_inplace_add, OscListReceive_inplace_add},
    {Py_nb_inplace_subtract, OscListReceive_inplace_sub},
    {Py_nb_inplace_multiply, OscListReceive_inplace_multiply},
    {Py_nb_inplace_true_divide, OscListReceive_inplace_div},
    {Py_tp_new, OscListReceive_new},
    {0, NULL}
};

static PyType_Spec OscListReceiveType_spec =
{
    "_pyo.OscListReceive_base",
    sizeof(OscListReceive),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    OscListReceiveType_slots
};

PyTypeObject *
PyoCreateOscListReceiveType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &OscListReceiveType_spec, NULL);
}
