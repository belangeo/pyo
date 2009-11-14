#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "lo/lo.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    lo_address address;
    char *host;
    int port;
    PyObject *address_path;
} OscSend;

static void
_compute_next_data_frame(OscSend *self)
{
    float *in = Stream_getData((Stream *)self->input_stream);
    float value = in[0];

    char *path  = PyString_AsString(self->address_path);

    if (lo_send(self->address, path, "f", value) == -1) {
        printf("OSC error %d: %s\n", lo_address_errno(self->address), lo_address_errstr(self->address));
    }    
}

static int
OscSend_traverse(OscSend *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int 
OscSend_clear(OscSend *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
OscSend_dealloc(OscSend* self)
{
    free(self->data);
    OscSend_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    OscSend *self;
    self = (OscSend *)type->tp_alloc(type, 0);

    self->host = NULL;

    INIT_OBJECT_COMMON

    return (PyObject *)self;
}

static int
OscSend_init(OscSend *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *pathtmp;

    static char *kwlist[] = {"input", "port", "address", "host", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OiO|s", kwlist, &inputtmp, &self->port, &pathtmp, &self->host))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
        
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
        return -1;
    }    
        
    Py_INCREF(pathtmp);    
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;
    
    char buf[20];
    sprintf(buf, "%i", self->port);
    self->address = lo_address_new(self->host, buf);
    
    _compute_next_data_frame((OscSend *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * OscSend_getServer(OscSend* self) { GET_SERVER };
static PyObject * OscSend_getStream(OscSend* self) { GET_STREAM };

static PyObject * OscSend_play(OscSend *self) { PLAY };
static PyObject * OscSend_stop(OscSend *self) { STOP };

static PyObject * OscSend_multiply(OscSend *self, PyObject *arg) { MULTIPLY };
static PyObject * OscSend_inplace_multiply(OscSend *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscSend_add(OscSend *self, PyObject *arg) { ADD };
static PyObject * OscSend_inplace_add(OscSend *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef OscSend_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscSend, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscSend, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(OscSend, input), 0, "Input sound object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscSend_methods[] = {
    {"getServer", (PyCFunction)OscSend_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscSend_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscSend_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscSend_stop, METH_NOARGS, "Stops computing."},
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
    (initproc)OscSend_init,      /* tp_init */
    0,                         /* tp_alloc */
    OscSend_new,                 /* tp_new */
};

