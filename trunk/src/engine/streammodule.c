#include <Python.h>
#include "structmember.h"

#define __STREAM_MODULE
#include "streammodule.h"
#undef __STREAM_MODULE

int stream_id = 1;

int 
Stream_getNewStreamId() 
{
    return stream_id++;
}

static void
Stream_dealloc(Stream* self)
{
    free(self->data);
    Py_XDECREF(self->streamobject);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Stream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Stream *self;
    MAKE_NEW_STREAM(self, type, NULL);
    return (PyObject *)self;
}

static int
Stream_init(Stream *self, PyObject *args, PyObject *kwds)
{
    PyObject *object=NULL, *tmp;
     
    static char *kwlist[] = {"streamobject", NULL};
 
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &object)){
        return -1;
    }
 
    if (object) {
        tmp = self->streamobject;
        Py_INCREF(object);
        self->streamobject = object;
        self->active = 0;
        self->chnl = 0;
        self->todac = 0;
        Py_DECREF(tmp);
    }
 
    return 0;
}

PyObject *
Stream_getStreamObject(Stream *self)
{
    Py_INCREF(self->streamobject);
    return self->streamobject;
}

int
Stream_getStreamId(Stream *self)
{
    return self->sid;
}

int
Stream_getStreamActive(Stream *self)
{
    return self->active;
}

int
Stream_getStreamChnl(Stream *self)
{
    return self->chnl;
}

int
Stream_getStreamToDac(Stream *self)
{
    return self->todac;
}

float *
Stream_getData(Stream *self)
{
    return (float *)self->data;
}    

void
Stream_setData(Stream *self, float *data)
{
    self->data = data;
}    

void Stream_setFunctionPtr(Stream *self, void *ptr)
{
    self->funcptr = ptr;
}

void Stream_callFunction(Stream *self)
{
    (*self->funcptr)(self->streamobject);
}    

PyTypeObject StreamType = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "pyo.Stream", /*tp_name*/
    sizeof(Stream), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)Stream_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    0, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_compare*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    0, /*tp_as_mapping*/
    0, /*tp_hash */
    0, /*tp_call*/
    0, /*tp_str*/
    0, /*tp_getattro*/
    0, /*tp_setattro*/
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Audio stream objects. For internal use only. Must never be instantiated by the user.", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    0, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc)Stream_init, /* tp_init */
    0, /* tp_alloc */
    Stream_new, /* tp_new */
};
