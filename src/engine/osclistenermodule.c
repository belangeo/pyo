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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "lo/lo.h"

static void error(int num, const char *msg, const char *path)
{
    PySys_WriteStdout("liblo server error %d in path %s: %s\n", num, path, msg);
}

typedef struct {
    PyObject_HEAD
    PyObject *osccallable;
    lo_server osc_server;
    int oscport;
} OscListener;

static PyObject *
OscListener_get(OscListener *self)
{
    while (lo_server_recv_noblock(self->osc_server, 0) != 0) {};
	Py_INCREF(Py_None);
	return Py_None;
}

int process_osc(const char *path, const char *types, lo_arg **argv, int argc,
                void *data, void *user_data)
{
    OscListener *server = (OscListener *)user_data;
    PyObject *tup;
    lo_blob *blob = NULL;
    char *blobdata = NULL;
    uint32_t blobsize = 0;
    PyObject *charlist = NULL; 
    tup = PyTuple_New(argc+1);
    int i = 0;
    unsigned int j = 0;

    PyGILState_STATE s = PyGILState_Ensure();
    PyTuple_SET_ITEM(tup, 0, PyUnicode_FromString(path));
    for (i=0; i<argc; i++) {
        switch (types[i]) {
            case LO_INT32:
                PyTuple_SET_ITEM(tup, i+1, PyInt_FromLong(argv[i]->i));
                break;
            case LO_INT64:
                PyTuple_SET_ITEM(tup, i+1, PyLong_FromLong(argv[i]->h));
                break;
            case LO_FLOAT:
                PyTuple_SET_ITEM(tup, i+1, PyFloat_FromDouble(argv[i]->f));
                break;
            case LO_DOUBLE:
                PyTuple_SET_ITEM(tup, i+1, PyFloat_FromDouble(argv[i]->d));
                break;
            case LO_STRING:
                PyTuple_SET_ITEM(tup, i+1, PyUnicode_FromString(&argv[i]->s));
                break;
            case LO_CHAR:
                PyTuple_SET_ITEM(tup, i+1, PyUnicode_FromFormat("%c", argv[i]->c));
                break;
            case LO_BLOB:
                blob = (lo_blob)argv[i];
                blobsize = lo_blob_datasize(blob);
                blobdata = lo_blob_dataptr(blob);
                charlist = PyList_New(blobsize);
                for (j=0; j<blobsize; j++) {
                    PyList_SET_ITEM(charlist, j, PyUnicode_FromFormat("%c", blobdata[j]));
                }
                PyTuple_SET_ITEM(tup, i+1, charlist);
                break;
            case LO_MIDI:
                charlist = PyList_New(4);
                for (j=0; j<4; j++) {
                    PyList_SET_ITEM(charlist, j, PyInt_FromLong(argv[i]->m[j]));
                }
                PyTuple_SET_ITEM(tup, i+1, charlist);                    
                break;
            case LO_NIL:
                Py_INCREF(Py_None);
                PyTuple_SET_ITEM(tup, i+1, Py_None);
                break;
            case LO_TRUE:
                Py_INCREF(Py_True);
                PyTuple_SET_ITEM(tup, i+1, Py_True);
                break;
            case LO_FALSE:
                Py_INCREF(Py_False);
                PyTuple_SET_ITEM(tup, i+1, Py_False);
                break;
            default:
                break;
        }
    }
    PyObject_Call((PyObject *)server->osccallable, tup, NULL);
    PyGILState_Release(s);
    Py_XDECREF(tup);

    return 0;
}

static int
OscListener_traverse(OscListener *self, visitproc visit, void *arg)
{
    Py_VISIT(self->osccallable);
    return 0;
}

static int
OscListener_clear(OscListener *self)
{
    Py_CLEAR(self->osccallable);
    return 0;
}

static void
OscListener_dealloc(OscListener* self)
{
    lo_server_free(self->osc_server);
    OscListener_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscListener_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    char buf[20];
    PyObject *osccalltmp=NULL;
    OscListener *self;

    self = (OscListener *)type->tp_alloc(type, 0);

    static char *kwlist[] = {"osccallable", "port", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist, &osccalltmp, &self->oscport))
        Py_RETURN_NONE;

    if (osccalltmp) {
        PyObject_CallMethod((PyObject *)self, "setOscFunction", "O", osccalltmp);
    }

    sprintf(buf, "%i", self->oscport);
    self->osc_server = lo_server_new(buf, error);
    lo_server_add_method(self->osc_server, NULL, NULL, process_osc, (void *)self);

    return (PyObject *)self;
}

static PyObject *
OscListener_setOscFunction(OscListener *self, PyObject *arg)
{
	PyObject *tmp;

    if (arg == Py_None) {
        Py_INCREF(Py_None);
        return Py_None;
    }

	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    Py_XDECREF(self->osccallable);
    Py_INCREF(tmp);
    self->osccallable = tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef OscListener_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef OscListener_methods[] = {
    {"get", (PyCFunction)OscListener_get, METH_NOARGS, "Check for new osc messages."},
    {"setOscFunction", (PyCFunction)OscListener_setOscFunction, METH_O, "Sets the function to be called."},
    {NULL}  /* Sentinel */
};

PyTypeObject OscListenerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.OscListener_base",         /*tp_name*/
    sizeof(OscListener),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscListener_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscListener objects. Calls a function with OSC data as arguments.",           /* tp_doc */
    (traverseproc)OscListener_traverse,   /* tp_traverse */
    (inquiry)OscListener_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscListener_methods,             /* tp_methods */
    OscListener_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    OscListener_new,                 /* tp_new */
};
