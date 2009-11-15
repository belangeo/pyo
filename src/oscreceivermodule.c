#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "oscreceivermodule.h"
#include "lo/lo.h"


void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

int OscReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                void *data, void *user_data)
{
    OscReceiver *self = user_data;
    PyDict_SetItem(self->dict, PyString_FromString(path), PyFloat_FromDouble(argv[0]->f));
    return 0;
}

float OscReceiver_getValue(OscReceiver *self, PyObject *path)
{
    PyObject *tmp;
    tmp = PyDict_GetItem(self->dict, path);
    //Py_INCREF(tmp);
    return PyFloat_AsDouble(tmp);
}

static void
_compute_next_data_frame(OscReceiver *self)
{
    lo_server_recv_noblock(self->osc_server, 0);
}

static int
OscReceiver_traverse(OscReceiver *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
OscReceiver_clear(OscReceiver *self)
{
    pyo_CLEAR
    return 0;
}

static void
OscReceiver_dealloc(OscReceiver* self)
{
    lo_server_free(self->osc_server);
    free(self->data);
    OscReceiver_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    OscReceiver *self;
    self = (OscReceiver *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON

    return (PyObject *)self;
}

static int
OscReceiver_init(OscReceiver *self, PyObject *args, PyObject *kwds)
{
    PyObject *pathtmp;
    Py_ssize_t i;
    
    static char *kwlist[] = {"port", "address", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO|OO", kwlist, &self->port, &pathtmp))
        return -1; 

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->dict = PyDict_New();

    if (PyString_Check(pathtmp) || PyList_Check(pathtmp)) {
        Py_INCREF(pathtmp);    
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a list of strings.");
        return -1;
    }    

    if (PyString_Check(self->address_path)) {
        PyDict_SetItem(self->dict, self->address_path, PyFloat_FromDouble(0.));
    }    
    else if (PyList_Check(self->address_path)) {
        Py_ssize_t lsize = PyList_Size(self->address_path);
        for (i=0; i<lsize; i++) {
            PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), PyFloat_FromDouble(0.));
        }    
    }
    
    char buf[20];
    sprintf(buf, "%i", self->port);
    self->osc_server = lo_server_new(buf, error);
    
    lo_server_add_method(self->osc_server, NULL, "f", OscReceiver_handler, self);

    _compute_next_data_frame((OscReceiver *)self);

    Py_INCREF(self);
    return 0;
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
    (initproc)OscReceiver_init,      /* tp_init */
    0,                         /* tp_alloc */
    OscReceiver_new,                 /* tp_new */
};

