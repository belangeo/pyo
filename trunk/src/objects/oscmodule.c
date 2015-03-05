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

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

/* main OSC receiver */
typedef struct {
    pyo_audio_HEAD
    lo_server osc_server;
    int port;
    PyObject *dict;
    PyObject *address_path;
} OscReceiver;

int OscReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        void *data, void *user_data)
{
    OscReceiver *self = user_data;
    PyDict_SetItem(self->dict, PyString_FromString(path), PyFloat_FromDouble(argv[0]->FLOAT_VALUE));
    return 0;
}

MYFLT OscReceiver_getValue(OscReceiver *self, PyObject *path)
{
    PyObject *tmp;
    tmp = PyDict_GetItem(self->dict, path);
    return PyFloat_AsDouble(tmp);
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
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp;
    OscReceiver *self;
    self = (OscReceiver *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscReceiver_compute_next_data_frame);

    static char *kwlist[] = {"port", "address", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO", kwlist, &self->port, &pathtmp))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->dict = PyDict_New();

    if (PyString_Check(pathtmp) || PyList_Check(pathtmp)) {
        Py_INCREF(pathtmp);
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a list of strings.");
        Py_RETURN_NONE;
    }

    if (PyString_Check(self->address_path)) {
        PyDict_SetItem(self->dict, self->address_path, PyFloat_FromDouble(0.));
    }
    else if (PyList_Check(self->address_path)) {
        int lsize = PyList_Size(self->address_path);
        for (i=0; i<lsize; i++) {
            PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), PyFloat_FromDouble(0.));
        }
    }

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
    if (PyString_Check(arg)) {
        PyDict_SetItem(self->dict, arg, PyFloat_FromDouble(0.));
    }
    else if (PyList_Check(arg)) {
        Py_ssize_t lsize = PyList_Size(arg);
        for (i=0; i<lsize; i++) {
            PyDict_SetItem(self->dict, PyList_GET_ITEM(arg, i), PyFloat_FromDouble(0.));
        }
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
OscReceiver_delAddress(OscReceiver *self, PyObject *arg)
{
    int i;
    if (PyString_Check(arg)) {
        PyDict_DelItem(self->dict, arg);
    }
    else if (PyList_Check(arg)) {
        Py_ssize_t lsize = PyList_Size(arg);
        for (i=0; i<lsize; i++) {
            PyDict_DelItem(self->dict, PyList_GET_ITEM(arg, i));
        }
    }
	Py_INCREF(Py_None);
	return Py_None;
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

static PyMemberDef OscReceiver_members[] = {
{"server", T_OBJECT_EX, offsetof(OscReceiver, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(OscReceiver, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef OscReceiver_methods[] = {
{"getServer", (PyCFunction)OscReceiver_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)OscReceiver_getStream, METH_NOARGS, "Returns stream object."},
{"addAddress", (PyCFunction)OscReceiver_addAddress, METH_O, "Add a new address to the dictionary."},
{"delAddress", (PyCFunction)OscReceiver_delAddress, METH_O, "Remove an address from the dictionary."},
{"setValue", (PyCFunction)OscReceiver_setValue, METH_VARARGS|METH_KEYWORDS, "Sets value for a specified address."},
{NULL}  /* Sentinel */
};

PyTypeObject OscReceiverType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.OscReceiver_base",         /*tp_name*/
sizeof(OscReceiver),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)OscReceiver_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
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
"OscReceiver objects. Receive values via Open Sound Control protocol.",           /* tp_doc */
(traverseproc)OscReceiver_traverse,   /* tp_traverse */
(inquiry)OscReceiver_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
OscReceiver_methods,             /* tp_methods */
OscReceiver_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
OscReceiver_new,                 /* tp_new */
};

/* OSC receiver stream object */
typedef struct {
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

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = OscReceive_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = OscReceive_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = OscReceive_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = OscReceive_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = OscReceive_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = OscReceive_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = OscReceive_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = OscReceive_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = OscReceive_postprocessing_revareva;
            break;
    }
}

static void
OscReceive_compute_next_data_frame(OscReceive *self)
{
    int i;
    MYFLT val = OscReceiver_getValue((OscReceiver *)self->input, self->address_path);

    if (self->interpolation == 1) {

        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->value = self->value + (val - self->value) * self->factor;
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
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
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp=NULL, *pathtmp=NULL, *multmp=NULL, *addtmp=NULL;;
    OscReceive *self;
    self = (OscReceive *)type->tp_alloc(type, 0);

    self->value = 0.;
    self->interpolation = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    self->factor = 1. / (0.01 * self->sr);

    Stream_setFunctionPtr(self->stream, OscReceive_compute_next_data_frame);
    self->mode_func_ptr = OscReceive_setProcMode;

    static char *kwlist[] = {"input", "address", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &pathtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->input);
    Py_INCREF(inputtmp);
    self->input = inputtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
        Py_RETURN_NONE;
    }

    Py_INCREF(pathtmp);
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
OscReceive_setInterpolation(OscReceive *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    self->interpolation = PyInt_AsLong(arg);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * OscReceive_getServer(OscReceive* self) { GET_SERVER };
static PyObject * OscReceive_getStream(OscReceive* self) { GET_STREAM };
static PyObject * OscReceive_setMul(OscReceive *self, PyObject *arg) { SET_MUL };
static PyObject * OscReceive_setAdd(OscReceive *self, PyObject *arg) { SET_ADD };
static PyObject * OscReceive_setSub(OscReceive *self, PyObject *arg) { SET_SUB };
static PyObject * OscReceive_setDiv(OscReceive *self, PyObject *arg) { SET_DIV };

static PyObject * OscReceive_play(OscReceive *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscReceive_stop(OscReceive *self) { STOP };

static PyObject * OscReceive_multiply(OscReceive *self, PyObject *arg) { MULTIPLY };
static PyObject * OscReceive_inplace_multiply(OscReceive *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscReceive_add(OscReceive *self, PyObject *arg) { ADD };
static PyObject * OscReceive_inplace_add(OscReceive *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscReceive_sub(OscReceive *self, PyObject *arg) { SUB };
static PyObject * OscReceive_inplace_sub(OscReceive *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscReceive_div(OscReceive *self, PyObject *arg) { DIV };
static PyObject * OscReceive_inplace_div(OscReceive *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef OscReceive_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscReceive, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(OscReceive, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscReceive, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscReceive_methods[] = {
    {"getServer", (PyCFunction)OscReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscReceive_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscReceive_stop, METH_NOARGS, "Stops computing."},
    {"setInterpolation", (PyCFunction)OscReceive_setInterpolation, METH_O, "Sets interpolation on or off."},
    {"setMul", (PyCFunction)OscReceive_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscReceive_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscReceive_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)OscReceive_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods OscReceive_as_number = {
    (binaryfunc)OscReceive_add,                      /*nb_add*/
    (binaryfunc)OscReceive_sub,                 /*nb_subtract*/
    (binaryfunc)OscReceive_multiply,                 /*nb_multiply*/
    (binaryfunc)OscReceive_div,                   /*nb_divide*/
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
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)OscReceive_inplace_add,              /*inplace_add*/
    (binaryfunc)OscReceive_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)OscReceive_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)OscReceive_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject OscReceiveType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscReceive_base",         /*tp_name*/
    sizeof(OscReceive),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscReceive_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &OscReceive_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscReceive objects. Receive values via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscReceive_traverse,   /* tp_traverse */
    (inquiry)OscReceive_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscReceive_methods,             /* tp_methods */
    OscReceive_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscReceive_new,                 /* tp_new */
};

/* OSC send object */
typedef struct {
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
    self->count++;
    if (self->count >= self->bufrate) {
        self->count = 0;
        MYFLT *in = Stream_getData((Stream *)self->input_stream);
        float value = (float)in[0];

        char *path  = PyString_AsString(self->address_path);

        if (lo_send(self->address, path, "f", value) == -1) {
            printf("OSC error %d: %s\n", lo_address_errno(self->address), lo_address_errstr(self->address));
        }
    }
}

static int
OscSend_traverse(OscSend *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->address_path);
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
OscSend_clear(OscSend *self)
{
    pyo_CLEAR
    Py_CLEAR(self->address_path);
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
OscSend_dealloc(OscSend* self)
{
    pyo_DEALLOC
    OscSend_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pathtmp;
    OscSend *self;
    self = (OscSend *)type->tp_alloc(type, 0);

    self->host = NULL;
    self->count = 0;
    self->bufrate = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscSend_compute_next_data_frame);

    static char *kwlist[] = {"input", "port", "address", "host", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OiO|s", kwlist, &inputtmp, &self->port, &pathtmp, &self->host))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
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
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    self->bufrate = PyInt_AsLong(arg);
    if (self->bufrate < 1)
        self->bufrate = 1;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * OscSend_getServer(OscSend* self) { GET_SERVER };
static PyObject * OscSend_getStream(OscSend* self) { GET_STREAM };

static PyObject * OscSend_play(OscSend *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscSend_stop(OscSend *self) { STOP };

static PyMemberDef OscSend_members[] = {
{"server", T_OBJECT_EX, offsetof(OscSend, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(OscSend, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(OscSend, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef OscSend_methods[] = {
{"getServer", (PyCFunction)OscSend_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)OscSend_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)OscSend_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)OscSend_stop, METH_NOARGS, "Stops computing."},
{"setBufferRate", (PyCFunction)OscSend_setBufferRate, METH_O, "Set how many buffers to wait before sending a new value."},
{NULL}  /* Sentinel */
};

PyTypeObject OscSendType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.OscSend_base",         /*tp_name*/
sizeof(OscSend),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)OscSend_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
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
"OscSend objects. Send values via Open Sound Control protocol.",           /* tp_doc */
(traverseproc)OscSend_traverse,   /* tp_traverse */
(inquiry)OscSend_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
OscSend_methods,             /* tp_methods */
OscSend_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
OscSend_new,                 /* tp_new */
};

/* OscDataSend object */
typedef struct {
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
    int i;
    lo_message *msg;
    char *path  = PyString_AsString(self->address_path);

    if (self->something_to_send == 1) {
        msg = lo_message_new();

        for (i=0; i<self->num_items; i++) {
            switch (self->types[i]) {
                case LO_INT32:
                    lo_message_add_int32(msg, PyInt_AsLong(PyList_GetItem(self->value, i)));
                    break;
                case LO_INT64:
                    lo_message_add_int64(msg, (long)PyLong_AsLong(PyList_GetItem(self->value, i)));
                    break;
                case LO_FLOAT:
                    lo_message_add_float(msg, PyFloat_AsDouble(PyList_GetItem(self->value, i)));
                    break;
                case LO_DOUBLE:
                    lo_message_add_double(msg, (double)PyFloat_AsDouble(PyList_GetItem(self->value, i)));
                    break;
                case LO_STRING:
                    lo_message_add_string(msg, PyString_AsString(PyList_GetItem(self->value, i)));
                    break;
                default:
                    break;
            }
        }
        if (lo_send_message(self->address, path, msg) == -1) {
            printf("OSC error %d: %s\n", lo_address_errno(self->address), lo_address_errstr(self->address));
        }
        self->something_to_send = 0;
        lo_message_free(msg);
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
    OscDataSend_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscDataSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp;
    OscDataSend *self;
    self = (OscDataSend *)type->tp_alloc(type, 0);

    self->host = NULL;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscDataSend_compute_next_data_frame);

    static char *kwlist[] = {"types", "port", "address", "host", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "siO|s", kwlist, &self->types, &self->port, &pathtmp, &self->host))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
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
static PyObject * OscDataSend_stop(OscDataSend *self) { STOP };

static PyObject *
OscDataSend_send(OscDataSend *self, PyObject *arg)
{
    PyObject *tmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    if (PyList_Check(arg)) {
        tmp = arg;
        Py_XDECREF(self->value);
        Py_INCREF(tmp);
        self->value = tmp;
        self->something_to_send = 1;
    }
    else
        printf("argument to send() method must be a tuple of values.\n");

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef OscDataSend_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscDataSend, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscDataSend, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscDataSend_methods[] = {
    {"getServer", (PyCFunction)OscDataSend_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscDataSend_getStream, METH_NOARGS, "Returns stream object."},
    {"send", (PyCFunction)OscDataSend_send, METH_O, "Sets values to be sent."},
    {"play", (PyCFunction)OscDataSend_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscDataSend_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject OscDataSendType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscDataSend_base",         /*tp_name*/
    sizeof(OscDataSend),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscDataSend_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
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
    "OscDataSend objects. Send data values via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscDataSend_traverse,   /* tp_traverse */
    (inquiry)OscDataSend_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscDataSend_methods,             /* tp_methods */
    OscDataSend_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscDataSend_new,                 /* tp_new */
};

/* main OscDataReceive */
typedef struct {
    pyo_audio_HEAD
    lo_server osc_server;
    PyObject *address_path;
    PyObject *callable;
    int port;
} OscDataReceive;

int OscDataReceive_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        void *data, void *user_data)
{
    OscDataReceive *self = user_data;
    PyObject *tup, *result=NULL;
    tup = PyTuple_New(argc+1);
    int i, ok = 0;

    Py_ssize_t lsize = PyList_Size(self->address_path);
    for (i=0; i<lsize; i++) {
        if (lo_pattern_match(path, PyString_AsString(PyList_GetItem(self->address_path, i)))) {
            ok = 1;
            break;
        }
    }

    if (ok) {
        PyTuple_SetItem(tup, 0, PyString_FromString(path));
        for (i=0; i<argc; i++) {
            switch (types[i]) {
                case LO_INT32:
                    PyTuple_SetItem(tup, i+1, PyInt_FromLong(argv[i]->i));
                    break;
                case LO_INT64:
                    PyTuple_SetItem(tup, i+1, PyLong_FromLong(argv[i]->h));
                    break;
                case LO_FLOAT:
                    PyTuple_SetItem(tup, i+1, PyFloat_FromDouble(argv[i]->f));
                    break;
                case LO_DOUBLE:
                    PyTuple_SetItem(tup, i+1, PyFloat_FromDouble(argv[i]->d));
                    break;
                case LO_STRING:
                    PyTuple_SetItem(tup, i+1, PyString_FromString(&argv[i]->s));
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
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscDataReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *pathtmp, *calltmp;
    OscDataReceive *self;
    self = (OscDataReceive *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscDataReceive_compute_next_data_frame);

    static char *kwlist[] = {"port", "address", "callable", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iOO", kwlist, &self->port, &pathtmp, &calltmp))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Py_XDECREF(self->callable);
    self->callable = calltmp;

    if (PyString_Check(pathtmp) || PyList_Check(pathtmp)) {
        Py_INCREF(pathtmp);
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a list of strings.");
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
static PyObject * OscDataReceive_stop(OscDataReceive *self) { STOP };

static PyObject *
OscDataReceive_addAddress(OscDataReceive *self, PyObject *arg) {
    int i;
    if (arg != NULL) {
        if (PyString_Check(arg))
            PyList_Append(self->address_path, arg);
        else if (PyList_Check(arg)) {
            Py_ssize_t len = PyList_Size(arg);
            for (i=0; i<len; i++) {
                PyList_Append(self->address_path, PyList_GET_ITEM(arg, i));
            }
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscDataReceive_delAddress(OscDataReceive *self, PyObject *arg) {
    if (arg != NULL) {
        if (PyInt_Check(arg)) {
            PySequence_DelItem(self->address_path, PyInt_AsLong(arg));
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef OscDataReceive_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscDataReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscDataReceive, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscDataReceive_methods[] = {
    {"getServer", (PyCFunction)OscDataReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscDataReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscDataReceive_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscDataReceive_stop, METH_NOARGS, "Stops computing."},
    {"addAddress", (PyCFunction)OscDataReceive_addAddress, METH_O, "Add new paths to the object."},
    {"delAddress", (PyCFunction)OscDataReceive_delAddress, METH_O, "Remove path from the object."},
    {NULL}  /* Sentinel */
};

PyTypeObject OscDataReceiveType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscDataReceive_base",         /*tp_name*/
    sizeof(OscDataReceive),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscDataReceive_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
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
    "OscDataReceive objects. Receive values via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscDataReceive_traverse,   /* tp_traverse */
    (inquiry)OscDataReceive_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscDataReceive_methods,             /* tp_methods */
    OscDataReceive_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscDataReceive_new,                 /* tp_new */
};

/* main OSC list receiver */
typedef struct {
    pyo_audio_HEAD
    lo_server osc_server;
    PyObject *dict;
    PyObject *address_path;
    int port;
    int num;
} OscListReceiver;

int OscListReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        void *data, void *user_data)
{
    OscListReceiver *self = user_data;

    int i;
    PyObject *flist;
    flist = PyList_New(self->num);

    for (i=0; i<self->num; i++) {
        PyList_SET_ITEM(flist, i, PyFloat_FromDouble(argv[i]->FLOAT_VALUE));
    }
    PyDict_SetItem(self->dict, PyString_FromString(path), flist);
    return 0;
}

PyObject *
OscListReceiver_getValue(OscListReceiver *self, PyObject *path)
{
    PyObject *tmp;
    tmp = PyDict_GetItem(self->dict, path);
    return tmp;
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
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscListReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *pathtmp, *flist;
    OscListReceiver *self;
    self = (OscListReceiver *)type->tp_alloc(type, 0);

    self->num = 8;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscListReceiver_compute_next_data_frame);

    static char *kwlist[] = {"port", "address", "num", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO|i", kwlist, &self->port, &pathtmp, &self->num))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->dict = PyDict_New();

    if (PyString_Check(pathtmp) || PyList_Check(pathtmp)) {
        Py_INCREF(pathtmp);
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a list of strings.");
        Py_RETURN_NONE;
    }

    if (PyString_Check(self->address_path)) {
        flist = PyList_New(self->num);
        for (j=0; j<self->num; j++) {
            PyList_SET_ITEM(flist, j, PyFloat_FromDouble(0.));
        }
        PyDict_SetItem(self->dict, self->address_path, flist);
    }
    else if (PyList_Check(self->address_path)) {
        int lsize = PyList_Size(self->address_path);
        for (i=0; i<lsize; i++) {
            flist = PyList_New(self->num);
            for (j=0; j<self->num; j++) {
                PyList_SET_ITEM(flist, j, PyFloat_FromDouble(0.));
            }
            PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), flist);
        }
    }

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

    if (PyString_Check(arg)) {
        flist = PyList_New(self->num);
        for (j=0; j<self->num; j++) {
            PyList_SET_ITEM(flist, j, PyFloat_FromDouble(0.));
        }
        PyDict_SetItem(self->dict, arg, flist);
    }
    else if (PyList_Check(arg)) {
        Py_ssize_t lsize = PyList_Size(arg);
        for (i=0; i<lsize; i++) {
            flist = PyList_New(self->num);
            for (j=0; j<self->num; j++) {
                PyList_SET_ITEM(flist, j, PyFloat_FromDouble(0.));
            }
            PyDict_SetItem(self->dict, PyList_GET_ITEM(arg, i), flist);
        }
    }
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
OscListReceiver_delAddress(OscListReceiver *self, PyObject *arg)
{
    int i;
    if (PyString_Check(arg)) {
        PyDict_DelItem(self->dict, arg);
    }
    else if (PyList_Check(arg)) {
        Py_ssize_t lsize = PyList_Size(arg);
        for (i=0; i<lsize; i++) {
            PyDict_DelItem(self->dict, PyList_GET_ITEM(arg, i));
        }
    }
	Py_INCREF(Py_None);
	return Py_None;
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

static PyMemberDef OscListReceiver_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscListReceiver, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscListReceiver, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscListReceiver_methods[] = {
    {"getServer", (PyCFunction)OscListReceiver_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscListReceiver_getStream, METH_NOARGS, "Returns stream object."},
    {"addAddress", (PyCFunction)OscListReceiver_addAddress, METH_O, "Add a new address to the dictionary."},
    {"delAddress", (PyCFunction)OscListReceiver_delAddress, METH_O, "Remove an address from the dictionary."},
    {"setValue", (PyCFunction)OscListReceiver_setValue, METH_VARARGS|METH_KEYWORDS, "Sets value for a specified address."},
    {NULL}  /* Sentinel */
};

PyTypeObject OscListReceiverType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscListReceiver_base",         /*tp_name*/
    sizeof(OscListReceiver),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscListReceiver_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
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
    "OscListReceiver objects. Receive list of values via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscListReceiver_traverse,   /* tp_traverse */
    (inquiry)OscListReceiver_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscListReceiver_methods,             /* tp_methods */
    OscListReceiver_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscListReceiver_new,                 /* tp_new */
};

/* OSC list receiver stream object */
typedef struct {
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

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = OscListReceive_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = OscListReceive_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = OscListReceive_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = OscListReceive_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = OscListReceive_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = OscListReceive_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = OscListReceive_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = OscListReceive_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = OscListReceive_postprocessing_revareva;
            break;
    }
}

static void
OscListReceive_compute_next_data_frame(OscListReceive *self)
{
    int i;
    PyObject *flist = OscListReceiver_getValue((OscListReceiver *)self->input, self->address_path);
    MYFLT val = PyFloat_AsDouble(PyList_GET_ITEM(flist, self->order));

    if (self->interpolation == 1) {

        for (i=0; i<self->bufsize; i++) {
            self->data[i] = self->value = self->value + (val - self->value) * self->factor;
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
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
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscListReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp=NULL, *pathtmp=NULL, *multmp=NULL, *addtmp=NULL;;
    OscListReceive *self;
    self = (OscListReceive *)type->tp_alloc(type, 0);

    self->order = 0;
    self->value = 0.;
    self->interpolation = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    self->factor = 1. / (0.01 * self->sr);

    Stream_setFunctionPtr(self->stream, OscListReceive_compute_next_data_frame);
    self->mode_func_ptr = OscListReceive_setProcMode;

    static char *kwlist[] = {"input", "address", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &pathtmp, &self->order, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->input);
    Py_INCREF(inputtmp);
    self->input = inputtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
        Py_RETURN_NONE;
    }

    Py_INCREF(pathtmp);
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject *
OscListReceive_setInterpolation(OscListReceive *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    self->interpolation = PyInt_AsLong(arg);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * OscListReceive_getServer(OscListReceive* self) { GET_SERVER };
static PyObject * OscListReceive_getStream(OscListReceive* self) { GET_STREAM };
static PyObject * OscListReceive_setMul(OscListReceive *self, PyObject *arg) { SET_MUL };
static PyObject * OscListReceive_setAdd(OscListReceive *self, PyObject *arg) { SET_ADD };
static PyObject * OscListReceive_setSub(OscListReceive *self, PyObject *arg) { SET_SUB };
static PyObject * OscListReceive_setDiv(OscListReceive *self, PyObject *arg) { SET_DIV };

static PyObject * OscListReceive_play(OscListReceive *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscListReceive_stop(OscListReceive *self) { STOP };

static PyObject * OscListReceive_multiply(OscListReceive *self, PyObject *arg) { MULTIPLY };
static PyObject * OscListReceive_inplace_multiply(OscListReceive *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscListReceive_add(OscListReceive *self, PyObject *arg) { ADD };
static PyObject * OscListReceive_inplace_add(OscListReceive *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscListReceive_sub(OscListReceive *self, PyObject *arg) { SUB };
static PyObject * OscListReceive_inplace_sub(OscListReceive *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscListReceive_div(OscListReceive *self, PyObject *arg) { DIV };
static PyObject * OscListReceive_inplace_div(OscListReceive *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef OscListReceive_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscListReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscListReceive, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(OscListReceive, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscListReceive, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscListReceive_methods[] = {
    {"getServer", (PyCFunction)OscListReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscListReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscListReceive_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscListReceive_stop, METH_NOARGS, "Stops computing."},
    {"setInterpolation", (PyCFunction)OscListReceive_setInterpolation, METH_O, "Sets interpolation on or off."},
    {"setMul", (PyCFunction)OscListReceive_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscListReceive_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscListReceive_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)OscListReceive_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods OscListReceive_as_number = {
    (binaryfunc)OscListReceive_add,                      /*nb_add*/
    (binaryfunc)OscListReceive_sub,                 /*nb_subtract*/
    (binaryfunc)OscListReceive_multiply,                 /*nb_multiply*/
    (binaryfunc)OscListReceive_div,                   /*nb_divide*/
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
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)OscListReceive_inplace_add,              /*inplace_add*/
    (binaryfunc)OscListReceive_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)OscListReceive_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)OscListReceive_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject OscListReceiveType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscListReceive_base",         /*tp_name*/
    sizeof(OscListReceive),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscListReceive_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &OscListReceive_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscListReceive objects. Receive one value from a list of floats via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscListReceive_traverse,   /* tp_traverse */
    (inquiry)OscListReceive_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscListReceive_methods,             /* tp_methods */
    OscListReceive_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscListReceive_new,                 /* tp_new */
};