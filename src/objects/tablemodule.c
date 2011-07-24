/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"

#define __TABLE_MODULE
#include "tablemodule.h"
#undef __TABLE_MODULE

/*************************/
/* TableStream structure */
/*************************/
static void
TableStream_dealloc(TableStream* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TableStream *self;
    MAKE_NEW_TABLESTREAM(self, type, NULL);
    return (PyObject *)self;
}

MYFLT *
TableStream_getData(TableStream *self)
{
    return (MYFLT *)self->data;
}    

void
TableStream_setData(TableStream *self, MYFLT *data)
{
    self->data = data;
}    

int
TableStream_getSize(TableStream *self)
{
    return self->size;
}

void
TableStream_setSize(TableStream *self, int size)
{
    self->size = size;
}    

double
TableStream_getSamplingRate(TableStream *self)
{
    return self->samplingRate;
}

void
TableStream_setSamplingRate(TableStream *self, double sr)
{
    self->samplingRate = sr;
}    

PyTypeObject TableStreamType = {
PyObject_HEAD_INIT(NULL)
0, /*ob_size*/
"_pyo.TableStream", /*tp_name*/
sizeof(TableStream), /*tp_basicsize*/
0, /*tp_itemsize*/
(destructor)TableStream_dealloc, /*tp_dealloc*/
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
"TableStream objects. For internal use only. Must never be instantiated by the user.", /* tp_doc */
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
0, /* tp_init */
0, /* tp_alloc */
TableStream_new, /* tp_new */
};



/***********************/
/* HarmTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *amplist;
} HarmTable;

static void
HarmTable_generate(HarmTable *self) {
    int i, j, ampsize;
    MYFLT factor, amplitude, val;
    
    ampsize = PyList_Size(self->amplist);
    MYFLT array[ampsize];
    for(j=0; j<ampsize; j++) {
        array[j] =  PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(self->amplist, j)));
    }    
    
    factor = 1. / (self->size * 0.5) * PI;
    
    for(i=0; i<self->size; i++) {
        val = 0;
        for(j=0; j<ampsize; j++) {
            amplitude = array[j];
            if (amplitude != 0.0) {
                val += MYSIN((j+1) * i * factor) * amplitude;
            }
        }
        self->data[i] = val;
    }

    val = self->data[0];
    self->data[self->size] = val;  
}

static int
HarmTable_traverse(HarmTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->amplist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
HarmTable_clear(HarmTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->amplist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
HarmTable_dealloc(HarmTable* self)
{
    free(self->data);
    HarmTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
HarmTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HarmTable *self;
    
    self = (HarmTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->amplist = PyList_New(0);
    PyList_Append(self->amplist, PyFloat_FromDouble(1.));
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    return (PyObject *)self;
}

static int
HarmTable_init(HarmTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *amplist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &amplist, &self->size))
        return -1; 
    
    if (amplist) {
        Py_INCREF(amplist);
        Py_DECREF(self->amplist);
        self->amplist = amplist;
    }

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    HarmTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * HarmTable_getServer(HarmTable* self) { GET_SERVER };
static PyObject * HarmTable_getTableStream(HarmTable* self) { GET_TABLE_STREAM };
static PyObject * HarmTable_setData(HarmTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HarmTable_normalize(HarmTable *self) { NORMALIZE };
static PyObject * HarmTable_getTable(HarmTable *self) { GET_TABLE };
static PyObject * HarmTable_getViewTable(HarmTable *self) { GET_VIEW_TABLE };
static PyObject * HarmTable_put(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HarmTable_get(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
HarmTable_setSize(HarmTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    HarmTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
HarmTable_getSize(HarmTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
HarmTable_replace(HarmTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->amplist);
    self->amplist = value; 
    
    HarmTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef HarmTable_members[] = {
{"server", T_OBJECT_EX, offsetof(HarmTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(HarmTable, tablestream), 0, "Table stream object."},
{"amplist", T_OBJECT_EX, offsetof(HarmTable, amplist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef HarmTable_methods[] = {
{"getServer", (PyCFunction)HarmTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)HarmTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)HarmTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)HarmTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"normalize", (PyCFunction)HarmTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"setData", (PyCFunction)HarmTable_setData, METH_O, "Sets the table from samples in a text file."},
{"setSize", (PyCFunction)HarmTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HarmTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)HarmTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)HarmTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"replace", (PyCFunction)HarmTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject HarmTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.HarmTable_base",         /*tp_name*/
sizeof(HarmTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HarmTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"HarmTable objects. Generates a table filled with a waveform whose harmonic content correspond to a given amplitude list values.",  /* tp_doc */
(traverseproc)HarmTable_traverse,   /* tp_traverse */
(inquiry)HarmTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HarmTable_methods,             /* tp_methods */
HarmTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)HarmTable_init,      /* tp_init */
0,                         /* tp_alloc */
HarmTable_new,                 /* tp_new */
};

/***********************/
/* ChebyTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *amplist;
} ChebyTable;

static void
ChebyTable_generate(ChebyTable *self) {
    int i, j, ampsize, halfsize;
    MYFLT factor, amplitude, val, ihalfsize, index, x;
    
    ampsize = PyList_Size(self->amplist);
    if (ampsize > 12)
        ampsize = 12;
    MYFLT array[ampsize];
    for(j=0; j<ampsize; j++) {
        array[j] =  PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(self->amplist, j)));
    }    
    
    halfsize = self->size / 2;
    ihalfsize = 1.0 / halfsize;
    factor = 1. / (self->size * 0.5) * PI;
    
    x = 0.0;
    for(i=0; i<self->size; i++) {
        val = 0;
        index = (i - halfsize) * ihalfsize;
        for(j=0; j<ampsize; j++) {
            amplitude = array[j];
            switch (j) {
                case 0:
                    x = index;
                    break;
                case 1:
                    x = 2 * MYPOW(index, 2) - 1;
                    break;
                case 2:
                    x = 4 * MYPOW(index, 3) - 3 * index;
                    break;
                case 3:
                    x = 8 * MYPOW(index, 4) - 8 * MYPOW(index, 2) + 1;
                    break;
                case 4:
                    x = 16 * MYPOW(index, 5) - 20 * MYPOW(index, 3) + 5 * index;
                    break;
                case 5:
                    x = 32 * MYPOW(index, 6) - 48 * MYPOW(index, 4) + 18 * MYPOW(index, 2) - 1;
                    break;
                case 6:
                    x = 64 * MYPOW(index, 7) - 112 * MYPOW(index, 5) + 56 * MYPOW(index, 3) - 7 * index;
                    break;
                case 7:
                    x = 128 * MYPOW(index, 8) - 256 * MYPOW(index, 6) + 160 * MYPOW(index, 4) - 32 * MYPOW(index, 2) + 1;
                    break;
                case 8:
                    x = 256 * MYPOW(index, 9) - 576 * MYPOW(index, 7) + 432 * MYPOW(index, 5) - 120 * MYPOW(index, 3) + 9 * index;
                    break;
                case 9:
                    x = 512 * MYPOW(index, 10) - 1280 * MYPOW(index, 8) + 1120 * MYPOW(index, 6) - 400 * MYPOW(index, 4) + 50 * MYPOW(index, 2) - 1;
                    break;
                case 10:
                    x = 1024 * MYPOW(index, 11) - 2816 * MYPOW(index, 9) + 2816 * MYPOW(index, 7) - 1232 * MYPOW(index, 5) + 220 * MYPOW(index, 3) - 11 * index;
                    break;
                case 11:
                    x = 2048 * MYPOW(index, 12) - 6144 * MYPOW(index, 10) + 6912 * MYPOW(index, 8) - 3584 * MYPOW(index, 6) + 840 * MYPOW(index, 4) - 72 * MYPOW(index, 2) + 1;
                    break;
            }
            val += x * amplitude;
        }
        self->data[i] = val;
    }
    
    val = self->data[self->size-1];
    self->data[self->size] = val;  
}

static int
ChebyTable_traverse(ChebyTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->amplist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
ChebyTable_clear(ChebyTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->amplist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
ChebyTable_dealloc(ChebyTable* self)
{
    free(self->data);
    ChebyTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ChebyTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ChebyTable *self;
    
    self = (ChebyTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->amplist = PyList_New(0);
    PyList_Append(self->amplist, PyFloat_FromDouble(1.));
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
ChebyTable_init(ChebyTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *amplist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &amplist, &self->size))
        return -1; 
    
    if (amplist) {
        Py_INCREF(amplist);
        Py_DECREF(self->amplist);
        self->amplist = amplist;
    }
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    ChebyTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * ChebyTable_getServer(ChebyTable* self) { GET_SERVER };
static PyObject * ChebyTable_getTableStream(ChebyTable* self) { GET_TABLE_STREAM };
static PyObject * ChebyTable_setData(ChebyTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ChebyTable_normalize(ChebyTable *self) { NORMALIZE };
static PyObject * ChebyTable_getTable(ChebyTable *self) { GET_TABLE };
static PyObject * ChebyTable_getViewTable(ChebyTable *self) { GET_VIEW_TABLE };
static PyObject * ChebyTable_put(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ChebyTable_get(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
ChebyTable_setSize(ChebyTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    ChebyTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ChebyTable_getSize(ChebyTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
ChebyTable_replace(ChebyTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->amplist);
    self->amplist = value; 
    
    ChebyTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef ChebyTable_members[] = {
{"server", T_OBJECT_EX, offsetof(ChebyTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(ChebyTable, tablestream), 0, "Table stream object."},
{"amplist", T_OBJECT_EX, offsetof(ChebyTable, amplist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef ChebyTable_methods[] = {
{"getServer", (PyCFunction)ChebyTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)ChebyTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)ChebyTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)ChebyTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)ChebyTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)ChebyTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"setSize", (PyCFunction)ChebyTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)ChebyTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)ChebyTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)ChebyTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"replace", (PyCFunction)ChebyTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject ChebyTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.ChebyTable_base",         /*tp_name*/
sizeof(ChebyTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)ChebyTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"ChebyTable objects. Generates a table filled with a waveform whose harmonic content correspond to a given amplitude list values.",  /* tp_doc */
(traverseproc)ChebyTable_traverse,   /* tp_traverse */
(inquiry)ChebyTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
ChebyTable_methods,             /* tp_methods */
ChebyTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)ChebyTable_init,      /* tp_init */
0,                         /* tp_alloc */
ChebyTable_new,                 /* tp_new */
};

/***********************/
/* HannTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
} HannTable;

static void
HannTable_generate(HannTable *self) {
    int i, halfSize;
    MYFLT val;
    
    halfSize = self->size / 2 - 1;
    
    for(i=0; i<self->size; i++) {
        val = 0.5 + (MYCOS(TWOPI * (i - halfSize) / self->size) * 0.5);
        self->data[i] = val;
    }
    val = self->data[0];
    self->data[self->size] = val;  
}

static int
HannTable_traverse(HannTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
HannTable_clear(HannTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
HannTable_dealloc(HannTable* self)
{
    free(self->data);
    HannTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
HannTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HannTable *self;
    
    self = (HannTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
HannTable_init(HannTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->size))
        return -1; 
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    HannTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * HannTable_getServer(HannTable* self) { GET_SERVER };
static PyObject * HannTable_getTableStream(HannTable* self) { GET_TABLE_STREAM };
static PyObject * HannTable_setData(HannTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HannTable_normalize(HannTable *self) { NORMALIZE };
static PyObject * HannTable_getTable(HannTable *self) { GET_TABLE };
static PyObject * HannTable_getViewTable(HannTable *self) { GET_VIEW_TABLE };
static PyObject * HannTable_put(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HannTable_get(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
HannTable_setSize(HannTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    HannTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
HannTable_getSize(HannTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyMemberDef HannTable_members[] = {
{"server", T_OBJECT_EX, offsetof(HannTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(HannTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef HannTable_methods[] = {
{"getServer", (PyCFunction)HannTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)HannTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)HannTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)HannTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)HannTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)HannTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"setSize", (PyCFunction)HannTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HannTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)HannTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)HannTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{NULL}  /* Sentinel */
};

PyTypeObject HannTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.HannTable_base",         /*tp_name*/
sizeof(HannTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HannTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"HannTable objects. Generates a table filled with a hanning function.",  /* tp_doc */
(traverseproc)HannTable_traverse,   /* tp_traverse */
(inquiry)HannTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HannTable_methods,             /* tp_methods */
HannTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)HannTable_init,      /* tp_init */
0,                         /* tp_alloc */
HannTable_new,                 /* tp_new */
};

/***********************/
/* ParaTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
} ParaTable;

static void
ParaTable_generate(ParaTable *self) {
    int i, sizeMinusOne;
    MYFLT rdur, rdur2, level, slope, curve;
    
    sizeMinusOne = self->size - 1;
    rdur = 1.0 / sizeMinusOne;
    rdur2 = rdur * rdur;
    level = 0.0;
    slope = 4.0 * (rdur - rdur2);
    curve = -8.0 * rdur2;

    for(i=0; i<sizeMinusOne; i++) {
        self->data[i] = level;
        level += slope;
        slope += curve;
    }

    self->data[sizeMinusOne] = self->data[0];  
    self->data[self->size] = self->data[0];  
}

static int
ParaTable_traverse(ParaTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
ParaTable_clear(ParaTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
ParaTable_dealloc(ParaTable* self)
{
    free(self->data);
    ParaTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ParaTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ParaTable *self;
    
    self = (ParaTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
ParaTable_init(ParaTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->size))
        return -1; 
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    ParaTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * ParaTable_getServer(ParaTable* self) { GET_SERVER };
static PyObject * ParaTable_getTableStream(ParaTable* self) { GET_TABLE_STREAM };
static PyObject * ParaTable_setData(ParaTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ParaTable_normalize(ParaTable *self) { NORMALIZE };
static PyObject * ParaTable_getTable(ParaTable *self) { GET_TABLE };
static PyObject * ParaTable_getViewTable(ParaTable *self) { GET_VIEW_TABLE };
static PyObject * ParaTable_put(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ParaTable_get(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
ParaTable_setSize(ParaTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    ParaTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ParaTable_getSize(ParaTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyMemberDef ParaTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(ParaTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(ParaTable, tablestream), 0, "Table stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ParaTable_methods[] = {
    {"getServer", (PyCFunction)ParaTable_getServer, METH_NOARGS, "Returns server object."},
    {"getTable", (PyCFunction)ParaTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)ParaTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)ParaTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)ParaTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)ParaTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"setSize", (PyCFunction)ParaTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)ParaTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"put", (PyCFunction)ParaTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)ParaTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {NULL}  /* Sentinel */
};

PyTypeObject ParaTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.ParaTable_base",         /*tp_name*/
    sizeof(ParaTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ParaTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    "ParaTable objects. Generates a parabola table.",  /* tp_doc */
    (traverseproc)ParaTable_traverse,   /* tp_traverse */
    (inquiry)ParaTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    ParaTable_methods,             /* tp_methods */
    ParaTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ParaTable_init,      /* tp_init */
    0,                         /* tp_alloc */
    ParaTable_new,                 /* tp_new */
};

/***********************/
/* LinTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
} LinTable;

static void
LinTable_generate(LinTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup, *tup2;
    int x1, y1;
    MYFLT x2, y2, diff;
    
    y1 = 0;
    y2 = 0.0;

    listsize = PyList_Size(self->pointslist);
    
    for(i=0; i<(listsize-1); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));
        tup2 = PyList_GET_ITEM(self->pointslist, i+1);
        y1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup2, 0)));
        y2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup2, 1)));
        steps = y1 - x1;
        if (steps <= 0)
            continue;
        diff = (y2 - x2) / steps;
        for(j=0; j<steps; j++) {
            self->data[x1+j] = x2 + diff * j;
        }
    }
    if (y1 < (self->size-1)) {
        self->data[y1] = y2;
        for (i=y1; i<self->size; i++) {
            self->data[i+1] = 0.0;
        }
        self->data[self->size] = 0.0;
    }
    else {
        self->data[self->size-1] = y2;
        self->data[self->size] = y2;
    }    
}

static int
LinTable_traverse(LinTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->pointslist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
LinTable_clear(LinTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
LinTable_dealloc(LinTable* self)
{
    free(self->data);
    LinTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
LinTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LinTable *self;
    
    self = (LinTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointslist = PyList_New(0);
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
LinTable_init(LinTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        return -1; 
    
    if (pointslist) {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(self->size), PyFloat_FromDouble(1.)));
    }
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    LinTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * LinTable_getServer(LinTable* self) { GET_SERVER };
static PyObject * LinTable_getTableStream(LinTable* self) { GET_TABLE_STREAM };
static PyObject * LinTable_setData(LinTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * LinTable_normalize(LinTable *self) { NORMALIZE };
static PyObject * LinTable_getTable(LinTable *self) { GET_TABLE };
static PyObject * LinTable_getViewTable(LinTable *self) { GET_VIEW_TABLE };
static PyObject * LinTable_put(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * LinTable_get(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
LinTable_setSize(LinTable *self, PyObject *value)
{
    Py_ssize_t i;
    PyObject *tup, *x2;
    int old_size, x1;
    MYFLT factor;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }

    old_size = self->size;
    self->size = PyInt_AsLong(value); 
    
    factor = (MYFLT)(self->size) / old_size;
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);

    Py_ssize_t listsize = PyList_Size(self->pointslist);

    PyObject *listtemp = PyList_New(0);
    
    for(i=0; i<(listsize); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyNumber_Float(PyTuple_GET_ITEM(tup, 1));
        PyList_Append(listtemp, PyTuple_Pack(2, PyInt_FromLong((int)(x1*factor)), x2));
    }
    
    Py_INCREF(listtemp);
    Py_DECREF(self->pointslist);
    self->pointslist = listtemp;
    
    LinTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
LinTable_getSize(LinTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
LinTable_getPoints(LinTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
LinTable_replace(LinTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value; 
    
    LinTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef LinTable_members[] = {
{"server", T_OBJECT_EX, offsetof(LinTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(LinTable, tablestream), 0, "Table stream object."},
{"pointslist", T_OBJECT_EX, offsetof(LinTable, pointslist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef LinTable_methods[] = {
{"getServer", (PyCFunction)LinTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)LinTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)LinTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)LinTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)LinTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)LinTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"setSize", (PyCFunction)LinTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)LinTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)LinTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)LinTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)LinTable_getPoints, METH_NOARGS, "Return the list of points."},
{"replace", (PyCFunction)LinTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject LinTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.LinTable_base",         /*tp_name*/
sizeof(LinTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)LinTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"LinTable objects. Generates a table filled with one or more straight lines.",  /* tp_doc */
(traverseproc)LinTable_traverse,   /* tp_traverse */
(inquiry)LinTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
LinTable_methods,             /* tp_methods */
LinTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)LinTable_init,      /* tp_init */
0,                         /* tp_alloc */
LinTable_new,                 /* tp_new */
};

/***********************/
/* CosTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
} CosTable;

static void
CosTable_generate(CosTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup, *tup2;
    int x1, y1;
    MYFLT x2, y2, mu, mu2;
        
    y1 = 0;
    y2 = 0.0;
    
    listsize = PyList_Size(self->pointslist);
    
    for(i=0; i<(listsize-1); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));        
        tup2 = PyList_GET_ITEM(self->pointslist, i+1);
        y1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup2, 0)));
        y2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup2, 1)));
        
        steps = y1 - x1;
        if (steps <= 0)
            continue;
        for(j=0; j<steps; j++) {
            mu = (MYFLT)j / steps;
            mu2 = (1.0-MYCOS(mu*PI))/2.0;
            self->data[x1+j] = x2 *(1.0-mu2) + y2*mu2;
        }
    }
    if (y1 < (self->size-1)) {
        self->data[y1] = y2;
        for (i=y1; i<self->size; i++) {
            self->data[i+1] = 0.0;
        }
        self->data[self->size] = 0.0;
    }
    else {
        self->data[self->size-1] = y2;
        self->data[self->size] = y2;
    }    
}

static int
CosTable_traverse(CosTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->pointslist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
CosTable_clear(CosTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
CosTable_dealloc(CosTable* self)
{
    free(self->data);
    CosTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
CosTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CosTable *self;
    
    self = (CosTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointslist = PyList_New(0);
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
CosTable_init(CosTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        return -1; 
    
    if (pointslist) {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(self->size), PyFloat_FromDouble(1.)));
    }
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    CosTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * CosTable_getServer(CosTable* self) { GET_SERVER };
static PyObject * CosTable_getTableStream(CosTable* self) { GET_TABLE_STREAM };
static PyObject * CosTable_setData(CosTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CosTable_normalize(CosTable *self) { NORMALIZE };
static PyObject * CosTable_getTable(CosTable *self) { GET_TABLE };
static PyObject * CosTable_getViewTable(CosTable *self) { GET_VIEW_TABLE };
static PyObject * CosTable_put(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CosTable_get(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
CosTable_setSize(CosTable *self, PyObject *value)
{
    Py_ssize_t i;
    PyObject *tup, *x2;
    int old_size, x1;
    MYFLT factor;
    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    old_size = self->size;
    self->size = PyInt_AsLong(value); 
    
    factor = (MYFLT)(self->size) / old_size;
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    Py_ssize_t listsize = PyList_Size(self->pointslist);
    
    PyObject *listtemp = PyList_New(0);
    
    for(i=0; i<(listsize); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyNumber_Float(PyTuple_GET_ITEM(tup, 1));
        PyList_Append(listtemp, PyTuple_Pack(2, PyInt_FromLong((int)(x1*factor)), x2));
    }
    
    Py_INCREF(listtemp);
    Py_DECREF(self->pointslist);
    self->pointslist = listtemp;
    
    CosTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CosTable_getSize(CosTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
CosTable_getPoints(CosTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
CosTable_replace(CosTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value; 
    
    CosTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef CosTable_members[] = {
{"server", T_OBJECT_EX, offsetof(CosTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(CosTable, tablestream), 0, "Table stream object."},
{"pointslist", T_OBJECT_EX, offsetof(CosTable, pointslist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef CosTable_methods[] = {
{"getServer", (PyCFunction)CosTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)CosTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)CosTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)CosTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)CosTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)CosTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"setSize", (PyCFunction)CosTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)CosTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)CosTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)CosTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)CosTable_getPoints, METH_NOARGS, "Return the list of points."},
{"replace", (PyCFunction)CosTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject CosTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.CosTable_base",         /*tp_name*/
sizeof(CosTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)CosTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"CosTable objects. Generates a table filled with one or more straight lines.",  /* tp_doc */
(traverseproc)CosTable_traverse,   /* tp_traverse */
(inquiry)CosTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
CosTable_methods,             /* tp_methods */
CosTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)CosTable_init,      /* tp_init */
0,                         /* tp_alloc */
CosTable_new,                 /* tp_new */
};

/***********************/
/* CurveTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
    MYFLT tension;
    MYFLT bias;
} CurveTable;

static void
CurveTable_generate(CurveTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup;
    int x1, x2;
    MYFLT y0, y1, y2, y3; 
    MYFLT m0, m1, mu, mu2, mu3;
    MYFLT a0, a1, a2, a3;

    for (i=0; i<self->size; i++) {
        self->data[i] = 0.0;
    }
    
    listsize = PyList_Size(self->pointslist);
    int times[listsize+2];
    MYFLT values[listsize+2];
    
    for (i=0; i<listsize; i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        times[i+1] = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        values[i+1] = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));        
    }
    
    // sets imaginary points
    times[0] = times[1] - times[2];
    if (values[1] < values[2])
        values[0] = values[1] - values[2];
    else
        values[0] = values[1] + values[2];

    int endP = listsize+1;
    times[endP] = times[endP-2] - times[endP-1];
    if (values[endP-2] < values[endP-1])
        values[endP] = values[endP-1] + values[endP-2];
    else
        values[endP] = values[endP-1] - values[endP-2];
    
    for(i=1; i<listsize; i++) {
        x1 = times[i];
        x2 = times[i+1];   
        y0 = values[i-1]; y1 = values[i]; y2 = values[i+1]; y3 = values[i+2];
        
        steps = x2 - x1;
        if (steps <= 0)
            continue;
        for(j=0; j<steps; j++) {
            mu = (MYFLT)j / steps;
            mu2 = mu * mu;
            mu3 = mu2 * mu;
            m0 = (y1-y0)*(1.0+self->bias)*(1.0-self->tension)/2.0;
            m0 += (y2-y1)*(1.0-self->bias)*(1.0-self->tension)/2.0;
            m1 = (y2-y1)*(1.0+self->bias)*(1.0-self->tension)/2.0;
            m1 += (y3-y2)*(1.0-self->bias)*(1.0-self->tension)/2.0;
            a0 = 2.0*mu3 - 3.0*mu2 + 1.0;
            a1 = mu3 - 2.0*mu2 + mu;
            a2 = mu3 - mu2;
            a3 = -2.0*mu3 + 3.0*mu2;
            
            self->data[x1+j] = (a0*y1 + a1*m0 + a2*m1 + a3*y2);
        }
    }
    
    self->data[self->size] = self->data[self->size-1];
}

static int
CurveTable_traverse(CurveTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->pointslist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
CurveTable_clear(CurveTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
CurveTable_dealloc(CurveTable* self)
{
    free(self->data);
    CurveTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
CurveTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CurveTable *self;
    
    self = (CurveTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->tension = 0.0;
    self->bias = 0.0;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
CurveTable_init(CurveTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    
    static char *kwlist[] = {"list", "tension", "bias", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFFI, kwlist, &pointslist, &self->tension, &self->bias, &self->size))
        return -1; 
    
    if (pointslist) {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(self->size), PyFloat_FromDouble(1.)));
    }
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    CurveTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * CurveTable_getServer(CurveTable* self) { GET_SERVER };
static PyObject * CurveTable_getTableStream(CurveTable* self) { GET_TABLE_STREAM };
static PyObject * CurveTable_setData(CurveTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CurveTable_normalize(CurveTable * self) { NORMALIZE };
static PyObject * CurveTable_getTable(CurveTable *self) { GET_TABLE };
static PyObject * CurveTable_getViewTable(CurveTable *self) { GET_VIEW_TABLE };
static PyObject * CurveTable_put(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CurveTable_get(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
CurveTable_setTension(CurveTable *self, PyObject *value)
{    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the tension attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyNumber_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The tension attribute value must be a float.");
        return PyInt_FromLong(-1);
    }
    
    self->tension = PyFloat_AsDouble(PyNumber_Float(value)); 

    CurveTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CurveTable_setBias(CurveTable *self, PyObject *value)
{    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the bias attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyNumber_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The bias attribute value must be a float.");
        return PyInt_FromLong(-1);
    }
    
    self->bias = PyFloat_AsDouble(PyNumber_Float(value)); 
    
    CurveTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CurveTable_setSize(CurveTable *self, PyObject *value)
{
    Py_ssize_t i;
    PyObject *tup, *x2;
    int old_size, x1;
    MYFLT factor;
    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    old_size = self->size;
    self->size = PyInt_AsLong(value); 
    
    factor = (MYFLT)(self->size) / old_size;
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    Py_ssize_t listsize = PyList_Size(self->pointslist);
    
    PyObject *listtemp = PyList_New(0);
    
    for(i=0; i<(listsize); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyNumber_Float(PyTuple_GET_ITEM(tup, 1));
        PyList_Append(listtemp, PyTuple_Pack(2, PyInt_FromLong((int)(x1*factor)), x2));
    }
    
    Py_INCREF(listtemp);
    Py_DECREF(self->pointslist);
    self->pointslist = listtemp;
    
    CurveTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CurveTable_getSize(CurveTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
CurveTable_getPoints(CurveTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
CurveTable_replace(CurveTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value; 
    
    CurveTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef CurveTable_members[] = {
{"server", T_OBJECT_EX, offsetof(CurveTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(CurveTable, tablestream), 0, "Table stream object."},
{"pointslist", T_OBJECT_EX, offsetof(CurveTable, pointslist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef CurveTable_methods[] = {
{"getServer", (PyCFunction)CurveTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)CurveTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)CurveTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)CurveTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)CurveTable_setData, METH_O, "Sets the table from samples in a text file."},
{"setSize", (PyCFunction)CurveTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)CurveTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)CurveTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)CurveTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)CurveTable_getPoints, METH_NOARGS, "Return the list of points."},
{"setTension", (PyCFunction)CurveTable_setTension, METH_O, "Sets the curvature tension."},
{"setBias", (PyCFunction)CurveTable_setBias, METH_O, "Sets the curve bias."},
{"replace", (PyCFunction)CurveTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"normalize", (PyCFunction)CurveTable_normalize, METH_NOARGS, "Normalize table between -1 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject CurveTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.CurveTable_base",         /*tp_name*/
sizeof(CurveTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)CurveTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"CurveTable objects. Generates a table filled with one or more straight lines.",  /* tp_doc */
(traverseproc)CurveTable_traverse,   /* tp_traverse */
(inquiry)CurveTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
CurveTable_methods,             /* tp_methods */
CurveTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)CurveTable_init,      /* tp_init */
0,                         /* tp_alloc */
CurveTable_new,                 /* tp_new */
};

/***********************/
/* ExpTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
    MYFLT exp;
    int inverse;
} ExpTable;

static void
ExpTable_generate(ExpTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup;
    int x1, x2;
    MYFLT y1, y2, range, inc, pointer, scl; 
    
    for (i=0; i<self->size; i++) {
        self->data[i] = 0.0;
    }
    
    listsize = PyList_Size(self->pointslist);
    int times[listsize];
    MYFLT values[listsize];
    
    for (i=0; i<listsize; i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        times[i] = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        values[i] = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));        
    }

    y1 = y2 = 0.0;
    for(i=0; i<(listsize-1); i++) {
        x1 = times[i];
        x2 = times[i+1];   
        y1 = values[i]; 
        y2 = values[i+1];
        
        range = y2 - y1;
        steps = x2 - x1;
        if (steps <= 0)
            continue;
        inc = 1.0 / steps;
        pointer = 0.0;
        if (self->inverse == 1) {
            if (range >= 0) {
                for(j=0; j<steps; j++) {
                    scl = MYPOW(pointer, self->exp);
                    self->data[x1+j] = scl * range + y1;
                    pointer += inc;
                }
            }
            else {
                for(j=0; j<steps; j++) {
                    scl = 1.0 - MYPOW(1.0 - pointer, self->exp);
                    self->data[x1+j] = scl * range + y1;
                    pointer += inc;
                }
            }
        }    
        else {
            for(j=0; j<steps; j++) {
                scl = MYPOW(pointer, self->exp);
                self->data[x1+j] = scl * range + y1;
                pointer += inc;
            }
        }    
    }
    
    self->data[self->size] = y2;
}

static int
ExpTable_traverse(ExpTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->pointslist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
ExpTable_clear(ExpTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
ExpTable_dealloc(ExpTable* self)
{
    free(self->data);
    ExpTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ExpTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ExpTable *self;
    
    self = (ExpTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->exp = 10.0;
    self->inverse = 1;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
ExpTable_init(ExpTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    
    static char *kwlist[] = {"list", "exp", "inverse", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFII, kwlist, &pointslist, &self->exp, &self->inverse, &self->size))
        return -1; 
    
    if (pointslist) {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(self->size), PyFloat_FromDouble(1.)));
    }
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    ExpTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * ExpTable_getServer(ExpTable* self) { GET_SERVER };
static PyObject * ExpTable_getTableStream(ExpTable* self) { GET_TABLE_STREAM };
static PyObject * ExpTable_setData(ExpTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ExpTable_normalize(ExpTable * self) { NORMALIZE };
static PyObject * ExpTable_getTable(ExpTable *self) { GET_TABLE };
static PyObject * ExpTable_getViewTable(ExpTable *self) { GET_VIEW_TABLE };
static PyObject * ExpTable_put(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ExpTable_get(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
ExpTable_setExp(ExpTable *self, PyObject *value)
{    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the exp attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyNumber_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The exp attribute value must be a float.");
        return PyInt_FromLong(-1);
    }
    
    self->exp = PyFloat_AsDouble(PyNumber_Float(value)); 
    
    ExpTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ExpTable_setInverse(ExpTable *self, PyObject *value)
{    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the inverse attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The inverse attribute value must be a boolean (True or False or 0 or 1).");
        return PyInt_FromLong(-1);
    }
    
    self->inverse = PyInt_AsLong(value); 
    
    ExpTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ExpTable_setSize(ExpTable *self, PyObject *value)
{
    Py_ssize_t i;
    PyObject *tup, *x2;
    int old_size, x1;
    MYFLT factor;
    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    old_size = self->size;
    self->size = PyInt_AsLong(value); 
    
    factor = (MYFLT)(self->size) / old_size;
    
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    
    Py_ssize_t listsize = PyList_Size(self->pointslist);
    
    PyObject *listtemp = PyList_New(0);
    
    for(i=0; i<(listsize); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyNumber_Float(PyTuple_GET_ITEM(tup, 1));
        PyList_Append(listtemp, PyTuple_Pack(2, PyInt_FromLong((int)(x1*factor)), x2));
    }
    
    Py_INCREF(listtemp);
    Py_DECREF(self->pointslist);
    self->pointslist = listtemp;
    
    ExpTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ExpTable_getSize(ExpTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
ExpTable_getPoints(ExpTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
ExpTable_replace(ExpTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value; 
    
    ExpTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef ExpTable_members[] = {
{"server", T_OBJECT_EX, offsetof(ExpTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(ExpTable, tablestream), 0, "Table stream object."},
{"pointslist", T_OBJECT_EX, offsetof(ExpTable, pointslist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef ExpTable_methods[] = {
{"getServer", (PyCFunction)ExpTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)ExpTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)ExpTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)ExpTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)ExpTable_setData, METH_O, "Sets the table from samples in a text file."},
{"setSize", (PyCFunction)ExpTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)ExpTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)ExpTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)ExpTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)ExpTable_getPoints, METH_NOARGS, "Return the list of points."},
{"setExp", (PyCFunction)ExpTable_setExp, METH_O, "Sets the exponent factor."},
{"setInverse", (PyCFunction)ExpTable_setInverse, METH_O, "Sets the inverse factor."},
{"replace", (PyCFunction)ExpTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"normalize", (PyCFunction)ExpTable_normalize, METH_NOARGS, "Normalize table between -1 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject ExpTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.ExpTable_base",         /*tp_name*/
sizeof(ExpTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)ExpTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"ExpTable objects. Generates a table filled with one or more straight lines.",  /* tp_doc */
(traverseproc)ExpTable_traverse,   /* tp_traverse */
(inquiry)ExpTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
ExpTable_methods,             /* tp_methods */
ExpTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)ExpTable_init,      /* tp_init */
0,                         /* tp_alloc */
ExpTable_new,                 /* tp_new */
};

/***********************/
/* SndTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    char *path;
    int sndSr;
    int chnl;
} SndTable;

static void
SndTable_loadSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num, num_items, num_chnls;
    MYFLT val;
    MYFLT *tmp;
        
    /* Open the WAV file. */
    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("Failed to open the file.\n");
    }
    /* Print some of the info, and figure out how much data to read. */
    self->size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;
    /*
    printf("samples = %d\n", self->size);
    printf("samplingrate = %d\n", self->sndSr);
    printf("channels = %d\n", num_chnls);
    */
    num_items = self->size * num_chnls;
    /* Allocate space for the data to be read, then read it. */
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    num = SF_READ(sf, tmp, num_items);
    sf_close(sf);
    for (i=0; i<num_items; i++) {
        if ((i % num_chnls) == self->chnl) {
            self->data[(int)(i/num_chnls)] = tmp[i];
        }    
    }
    val = self->data[0];
    self->data[self->size] = val;  

    free(tmp);
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setSamplingRate(self->tablestream, self->sndSr);
    TableStream_setData(self->tablestream, self->data);
}

static int
SndTable_traverse(SndTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
SndTable_clear(SndTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
SndTable_dealloc(SndTable* self)
{
    free(self->data);
    SndTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
SndTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SndTable *self;
    
    self = (SndTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->chnl = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
SndTable_init(SndTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"path", "chnl", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, &self->path, &self->chnl))
        return -1; 
    
    SndTable_loadSound(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * SndTable_getServer(SndTable* self) { GET_SERVER };
static PyObject * SndTable_getTableStream(SndTable* self) { GET_TABLE_STREAM };
static PyObject * SndTable_setData(SndTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * SndTable_normalize(SndTable *self) { NORMALIZE };
static PyObject * SndTable_getTable(SndTable *self) { GET_TABLE };
static PyObject * SndTable_put(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * SndTable_get(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject * 
SndTable_getViewTable(SndTable *self) { 
    int i, j, y;
    int w = 500;
    int h = 200;
    int h2 = h/2;
    int amp = h2;
    int count = 0;
    MYFLT absin;
    int step = (int)(self->size / (MYFLT)(w - 1));
    PyObject *samples;

    samples = PyList_New(w*4);
    for(i=0; i<w; i++) {
        absin = 0.0;
        /*for (j=0; j<step; j++) {
            absin += self->data[count++];
        }
        y = (int)(MYFABS(absin / step) * amp); */
        for (j=0; j<step; j++) {
            if (MYFABS(self->data[count++]) > absin)
                absin = self->data[count];
        }
        y = (int)(absin * amp);
        PyList_SetItem(samples, i*4, PyInt_FromLong(i));
        PyList_SetItem(samples, i*4+1, PyInt_FromLong(h2-y));
        PyList_SetItem(samples, i*4+2, PyInt_FromLong(i));
        PyList_SetItem(samples, i*4+3, PyInt_FromLong(h2+y));
    }

    return samples;
};

static PyObject *
SndTable_setSound(SndTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"path", "chnl", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "si", kwlist, &self->path, &self->chnl)) {
        Py_INCREF(Py_None);
        return Py_None;
    }    
    
    SndTable_loadSound(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SndTable_getSize(SndTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
SndTable_getRate(SndTable *self)
{
    MYFLT sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr * (self->sndSr/sr) / self->size);
};

static PyMemberDef SndTable_members[] = {
{"server", T_OBJECT_EX, offsetof(SndTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(SndTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef SndTable_methods[] = {
{"getServer", (PyCFunction)SndTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)SndTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)SndTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)SndTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)SndTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)SndTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"put", (PyCFunction)SndTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)SndTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"setSound", (PyCFunction)SndTable_setSound, METH_VARARGS|METH_KEYWORDS, "Load a new sound in the table."},
{"getSize", (PyCFunction)SndTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getRate", (PyCFunction)SndTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject SndTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SndTable_base",         /*tp_name*/
sizeof(SndTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SndTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"SndTable objects. Generates a table filled with a soundfile.",  /* tp_doc */
(traverseproc)SndTable_traverse,   /* tp_traverse */
(inquiry)SndTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SndTable_methods,             /* tp_methods */
SndTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SndTable_init,      /* tp_init */
0,                         /* tp_alloc */
SndTable_new,                 /* tp_new */
};

/***********************/
/* NewTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    MYFLT length;
    MYFLT feedback;
    int pointer;
} NewTable;

static PyObject *
NewTable_recordChunk(NewTable *self, MYFLT *data, int datasize)
{
    int i;

    if (self->feedback == 0.0) {
        for (i=0; i<datasize; i++) {
            self->data[self->pointer++] = data[i];
            if (self->pointer == self->size)
                self->pointer = 0;
        }
    }
    else {
        for (i=0; i<datasize; i++) {
            self->data[self->pointer] = data[i] + self->data[self->pointer] * self->feedback;
            self->pointer++;
            if (self->pointer == self->size)
                self->pointer = 0;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
NewTable_traverse(NewTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
NewTable_clear(NewTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
NewTable_dealloc(NewTable* self)
{
    free(self->data);
    NewTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
NewTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NewTable *self;
    
    self = (NewTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointer = 0;
    self->feedback = 0.0;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
NewTable_init(NewTable *self, PyObject *args, PyObject *kwds)
{    
    int i;
    PyObject *inittmp=NULL;
    static char *kwlist[] = {"length", "init", "feedback", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_OF, kwlist, &self->length, &inittmp, &self->feedback))
        return -1; 

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    self->size = (int)(self->length * sr + 0.5);
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    for (i=0; i<(self->size+1); i++) {
        self->data[i] = 0.;
    }
    
    TableStream_setSize(self->tablestream, self->size);

    if (inittmp) {
        PyObject_CallMethod((PyObject *)self, "setTable", "O", inittmp);
    }
    
    TableStream_setData(self->tablestream, self->data);
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * NewTable_getServer(NewTable* self) { GET_SERVER };
static PyObject * NewTable_getTableStream(NewTable* self) { GET_TABLE_STREAM };
static PyObject * NewTable_setData(NewTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * NewTable_normalize(NewTable *self) { NORMALIZE };
static PyObject * NewTable_getTable(NewTable *self) { GET_TABLE };
static PyObject * NewTable_getViewTable(NewTable *self) { GET_VIEW_TABLE };
static PyObject * NewTable_put(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * NewTable_get(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
NewTable_getSize(NewTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
NewTable_getLength(NewTable *self)
{
    return PyFloat_FromDouble(self->length);
};

static PyObject *
NewTable_getRate(NewTable *self)
{
    MYFLT sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr / self->size);
};

static PyObject *
NewTable_setTable(NewTable *self, PyObject *value)
{
    int i;
    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "Arg must be a list.");
        return PyInt_FromLong(-1);
    }
    
    int size = PyList_Size(value);
    if (size != self->size) {
        PyErr_SetString(PyExc_TypeError, "New table must be of the same size as actual table.");
        return PyInt_FromLong(-1);
    }
    
    for(i=0; i<self->size; i++) {
        self->data[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(value, i)));
    }
    
    Py_RETURN_NONE;    
}

static PyObject *
NewTable_setFeedback(NewTable *self, PyObject *value)
{
    MYFLT feed;

    if (PyNumber_Check(value)) {
        feed = PyFloat_AsDouble(PyNumber_Float(value));
        if (feed < -1.0)
            feed = -1.0;
        else if (feed > 1.0)
            feed = 1.0;
        self->feedback = feed;
    }
        
    Py_RETURN_NONE;    
}

static PyMemberDef NewTable_members[] = {
{"server", T_OBJECT_EX, offsetof(NewTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(NewTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef NewTable_methods[] = {
{"getServer", (PyCFunction)NewTable_getServer, METH_NOARGS, "Returns server object."},
{"getTable", (PyCFunction)NewTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"setTable", (PyCFunction)NewTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getViewTable", (PyCFunction)NewTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)NewTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setFeedback", (PyCFunction)NewTable_setFeedback, METH_O, "Feedback sets the amount of old data to mix with a new recording."},
{"setData", (PyCFunction)NewTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)NewTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"put", (PyCFunction)NewTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)NewTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getSize", (PyCFunction)NewTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getLength", (PyCFunction)NewTable_getLength, METH_NOARGS, "Return the length of the table in seconds."},
{"getRate", (PyCFunction)NewTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject NewTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.NewTable_base",         /*tp_name*/
sizeof(NewTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)NewTable_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"NewTable objects. Generates an empty table.",  /* tp_doc */
(traverseproc)NewTable_traverse,   /* tp_traverse */
(inquiry)NewTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
NewTable_methods,             /* tp_methods */
NewTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)NewTable_init,      /* tp_init */
0,                         /* tp_alloc */
NewTable_new,                 /* tp_new */
};

/***********************/
/* DataTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    int pointer;
} DataTable;

static int
DataTable_traverse(DataTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
DataTable_clear(DataTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
DataTable_dealloc(DataTable* self)
{
    free(self->data);
    DataTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
DataTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    DataTable *self;
    
    self = (DataTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointer = 0;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
DataTable_init(DataTable *self, PyObject *args, PyObject *kwds)
{    
    int i;
    PyObject *inittmp=NULL;
    static char *kwlist[] = {"size", "init", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i|O", kwlist, &self->size, &inittmp))
        return -1; 
    
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));
    
    for (i=0; i<(self->size+1); i++) {
        self->data[i] = 0.;
    }
    
    TableStream_setSize(self->tablestream, self->size);
    
    if (inittmp) {
        PyObject_CallMethod((PyObject *)self, "setTable", "O", inittmp);
    }
    
    TableStream_setData(self->tablestream, self->data);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    Py_INCREF(self);
    return 0;
}

static PyObject * DataTable_getServer(DataTable* self) { GET_SERVER };
static PyObject * DataTable_getTableStream(DataTable* self) { GET_TABLE_STREAM };
static PyObject * DataTable_setData(DataTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * DataTable_normalize(DataTable *self) { NORMALIZE };
static PyObject * DataTable_getTable(DataTable *self) { GET_TABLE };
static PyObject * DataTable_getViewTable(DataTable *self) { GET_VIEW_TABLE };
static PyObject * DataTable_put(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * DataTable_get(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };

static PyObject *
DataTable_getSize(DataTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
DataTable_getRate(DataTable *self)
{
    MYFLT sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr / self->size);
};

static PyObject *
DataTable_setTable(DataTable *self, PyObject *value)
{
    int i;
    
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "Arg must be a list.");
        return PyInt_FromLong(-1);
    }
    
    int size = PyList_Size(value);
    if (size != self->size) {
        PyErr_SetString(PyExc_TypeError, "New table must be of the same size as actual table.");
        return PyInt_FromLong(-1);
    }
    
    for(i=0; i<self->size; i++) {
        self->data[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(value, i)));
    }
    
    Py_RETURN_NONE;    
}

static PyMemberDef DataTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(DataTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(DataTable, tablestream), 0, "Table stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef DataTable_methods[] = {
    {"getServer", (PyCFunction)DataTable_getServer, METH_NOARGS, "Returns server object."},
    {"getTable", (PyCFunction)DataTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"setTable", (PyCFunction)DataTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getViewTable", (PyCFunction)DataTable_getViewTable, METH_NOARGS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)DataTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)DataTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)DataTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"put", (PyCFunction)DataTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)DataTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"getSize", (PyCFunction)DataTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
    {"getRate", (PyCFunction)DataTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
    {NULL}  /* Sentinel */
};

PyTypeObject DataTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.DataTable_base",         /*tp_name*/
    sizeof(DataTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DataTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    "DataTable objects. Generates an empty table.",  /* tp_doc */
    (traverseproc)DataTable_traverse,   /* tp_traverse */
    (inquiry)DataTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    DataTable_methods,             /* tp_methods */
    DataTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)DataTable_init,      /* tp_init */
    0,                         /* tp_alloc */
    DataTable_new,                 /* tp_new */
};

/******************************/
/* TableRec object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    NewTable *table;
    int pointer;
    int active;
    MYFLT fadetime;
    MYFLT fadeInSample;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
} TableRec;

static void
TableRec_compute_next_data_frame(TableRec *self)
{
    int i, num, upBound;
    MYFLT sclfade, val;
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    
    if ((size - self->pointer) >= self->bufsize)
        num = self->bufsize;
    else {
        num = size - self->pointer;
        if (self->active == 1) {
            if (num <= 0)
                self->trigsBuffer[0] = 1.0;
            else
                self->trigsBuffer[num-1] = 1.0;
            self->active = 0;
        }    
    }
    
    if (self->pointer < size) {   
        sclfade = 1. / self->fadetime;
        upBound = size - self->fadeInSample;
        
        MYFLT buffer[num];
        memset(&buffer, 0, sizeof(buffer));
        MYFLT *in = Stream_getData((Stream *)self->input_stream);
        
        for (i=0; i<num; i++) {
            if (self->pointer < self->fadeInSample)
                val = self->pointer / self->fadeInSample;
            else if (self->pointer > upBound)
                val = (size - self->pointer) / self->fadeInSample;
            else
                val = 1.;
            buffer[i] = in[i] * val;
            self->pointer++;
        }
        NewTable_recordChunk((NewTable *)self->table, buffer, num);
    }    
}

static int
TableRec_traverse(TableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    return 0;
}

static int 
TableRec_clear(TableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    return 0;
}

static void
TableRec_dealloc(TableRec* self)
{
    free(self->data);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    TableRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableRec_deleteStream(TableRec *self) { DELETE_STREAM };

static PyObject *
TableRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TableRec *self;
    self = (TableRec *)type->tp_alloc(type, 0);
    
    self->pointer = 0;
    self->active = 1;
    self->fadetime = 0.;
    
    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableRec_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 0);

    return (PyObject *)self;
}

static int
TableRec_init(TableRec *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp;
    
    static char *kwlist[] = {"input", "table", "fadetime", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_F, kwlist, &inputtmp, &tabletmp, &self->fadetime))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = (NewTable *)tabletmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    if ((self->fadetime * self->sr) >= (size * 0.5))
        self->fadetime = size * 0.499 / self->sr;
    if (self->fadetime == 0.0)
        self->fadeInSample = 0.0;
    else
        self->fadeInSample = MYROUND(self->fadetime * self->sr + 0.5);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TableRec_getServer(TableRec* self) { GET_SERVER };
static PyObject * TableRec_getStream(TableRec* self) { GET_STREAM };

static PyObject * TableRec_play(TableRec *self, PyObject *args, PyObject *kwds) 
{ 
    self->pointer = 0;
    self->active = 1;
    PLAY 
};

static PyObject * TableRec_stop(TableRec *self) { STOP };

static PyObject *
TableRec_setTable(TableRec *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->table);
    self->table = (NewTable *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

MYFLT *
TableRec_getTrigsBuffer(TableRec *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    


static PyMemberDef TableRec_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRec, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRec, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TableRec, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TableRec, table), 0, "Table to record in."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRec_methods[] = {
{"getServer", (PyCFunction)TableRec_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRec_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"setTable", (PyCFunction)TableRec_setTable, METH_O, "Sets a new table."},
{"play", (PyCFunction)TableRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableRec_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableRecType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableRec_base",         /*tp_name*/
sizeof(TableRec),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableRec_dealloc, /*tp_dealloc*/
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
"TableRec objects. Record audio input in a table object.",           /* tp_doc */
(traverseproc)TableRec_traverse,   /* tp_traverse */
(inquiry)TableRec_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableRec_methods,             /* tp_methods */
TableRec_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableRec_init,      /* tp_init */
0,                         /* tp_alloc */
TableRec_new,                 /* tp_new */
};

/************************************************************************************************/
/* TableRecTrig trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TableRec *mainReader;
} TableRecTrig;

static void
TableRecTrig_compute_next_data_frame(TableRecTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = TableRec_getTrigsBuffer((TableRec *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
}

static int
TableRecTrig_traverse(TableRecTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
TableRecTrig_clear(TableRecTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
TableRecTrig_dealloc(TableRecTrig* self)
{
    free(self->data);
    TableRecTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableRecTrig_deleteStream(TableRecTrig *self) { DELETE_STREAM };

static PyObject *
TableRecTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TableRecTrig *self;
    self = (TableRecTrig *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TableRecTrig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TableRecTrig_init(TableRecTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (TableRec *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * TableRecTrig_getServer(TableRecTrig* self) { GET_SERVER };
static PyObject * TableRecTrig_getStream(TableRecTrig* self) { GET_STREAM };

static PyObject * TableRecTrig_play(TableRecTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableRecTrig_stop(TableRecTrig *self) { STOP };

static PyMemberDef TableRecTrig_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRecTrig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRecTrig, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRecTrig_methods[] = {
{"getServer", (PyCFunction)TableRecTrig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRecTrig_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableRecTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TableRecTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableRecTrig_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableRecTrigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableRecTrig_base",         /*tp_name*/
sizeof(TableRecTrig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableRecTrig_dealloc, /*tp_dealloc*/
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
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"TableRecTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
(traverseproc)TableRecTrig_traverse,   /* tp_traverse */
(inquiry)TableRecTrig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableRecTrig_methods,             /* tp_methods */
TableRecTrig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableRecTrig_init,      /* tp_init */
0,                         /* tp_alloc */
TableRecTrig_new,                 /* tp_new */
};

/******************************/
/* TableMorph object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *table;
    PyObject *sources;
} TableMorph;

static MYFLT
TableMorph_clip(MYFLT x) {
    if (x < 0.0)
        return 0.0;
    else if (x >= 0.999999)
        return 0.999999;
    else
        return x;
}

static void
TableMorph_compute_next_data_frame(TableMorph *self)
{
    int i, x, y;
    MYFLT input, interp, interp1, interp2;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    int len = PyList_Size(self->sources);

    input = TableMorph_clip(in[0]);

    interp = input * (len - 1);
    x = (int)(interp);   
    y = x + 1;
            
    MYFLT *tab1 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, x), "getTableStream", ""));
    MYFLT *tab2 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, y), "getTableStream", ""));
        
    interp = MYFMOD(interp, 1.0);
    interp1 = 1. - interp;
    interp2 = interp;
    
    MYFLT buffer[size];
    for (i=0; i<size; i++) {
        buffer[i] = tab1[i] * interp1 + tab2[i] * interp2;
    }    
    
    NewTable_recordChunk((NewTable *)self->table, buffer, size);
}

static int
TableMorph_traverse(TableMorph *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->sources);
    return 0;
}

static int 
TableMorph_clear(TableMorph *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->sources);
    return 0;
}

static void
TableMorph_dealloc(TableMorph* self)
{
    free(self->data);
    TableMorph_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableMorph_deleteStream(TableMorph *self) { DELETE_STREAM };

static PyObject *
TableMorph_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TableMorph *self;
    self = (TableMorph *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    
    Stream_setFunctionPtr(self->stream, TableMorph_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TableMorph_init(TableMorph *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *sourcestmp;
    
    static char *kwlist[] = {"input", "table", "sources", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &tabletmp, &sourcestmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = (PyObject *)tabletmp;
    
    Py_XDECREF(self->sources);
    self->sources = (PyObject *)sourcestmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TableMorph_getServer(TableMorph* self) { GET_SERVER };
static PyObject * TableMorph_getStream(TableMorph* self) { GET_STREAM };

static PyObject * TableMorph_play(TableMorph *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableMorph_stop(TableMorph *self) { STOP };

static PyObject *
TableMorph_setTable(TableMorph *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->table);
    self->table = (PyObject *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
TableMorph_setSources(TableMorph *self, PyObject *arg)
{	
    if (arg == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(arg);
    Py_DECREF(self->sources);
    self->sources = arg;
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef TableMorph_members[] = {
{"server", T_OBJECT_EX, offsetof(TableMorph, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableMorph, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TableMorph, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TableMorph, table), 0, "Table to record in."},
{"sources", T_OBJECT_EX, offsetof(TableMorph, sources), 0, "list of tables to interpolate from."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableMorph_methods[] = {
{"getServer", (PyCFunction)TableMorph_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableMorph_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableMorph_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"setTable", (PyCFunction)TableMorph_setTable, METH_O, "Sets a new table."},
{"setSources", (PyCFunction)TableMorph_setSources, METH_O, "Changes the sources tables."},
{"play", (PyCFunction)TableMorph_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableMorph_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableMorphType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableMorph_base",         /*tp_name*/
sizeof(TableMorph),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableMorph_dealloc, /*tp_dealloc*/
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
"TableMorph objects. Interpolation contents of different table objects.",           /* tp_doc */
(traverseproc)TableMorph_traverse,   /* tp_traverse */
(inquiry)TableMorph_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableMorph_methods,             /* tp_methods */
TableMorph_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableMorph_init,      /* tp_init */
0,                         /* tp_alloc */
TableMorph_new,                 /* tp_new */
};

/******************************/
/* TrigTableRec object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *trig;
    Stream *trig_stream;
    NewTable *table;
    int pointer;
    int active;
    MYFLT fadetime;
    MYFLT fadeInSample;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
} TrigTableRec;

static void
TrigTableRec_compute_next_data_frame(TrigTableRec *self)
{
    int i, j, num, upBound;
    MYFLT val;
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trig = Stream_getData((Stream *)self->trig_stream);

    if (self->active == 1) {
        if ((size - self->pointer) >= self->bufsize)
            num = self->bufsize;
        else {
            num = size - self->pointer;
            if (self->active == 1) {
                if (num <= 0)
                    self->trigsBuffer[0] = 1.0;
                else
                    self->trigsBuffer[num-1] = 1.0;
                self->active = 0;
            }    
        }
    
        if (self->pointer < size) {   
            upBound = size - self->fadeInSample;
        
            MYFLT buffer[num];
            memset(&buffer, 0, sizeof(buffer));
        
            for (i=0; i<num; i++) {
                if (self->pointer < self->fadeInSample)
                    val = self->pointer / self->fadeInSample;
                else if (self->pointer > upBound)
                    val = (size - self->pointer) / self->fadeInSample;
                else
                    val = 1.;
                buffer[i] = in[i] * val;
                self->pointer++;
            }
            NewTable_recordChunk((NewTable *)self->table, buffer, num);
        }
    }
    else {
        for (j=0; j<self->bufsize; j++) {
            if (trig[j] == 1.0) {
                self->active = 1;
                self->pointer = 0;
                if (size >= self->bufsize)
                    num = self->bufsize - j;
                else {
                    num = size < (self->bufsize - j) ? size : (self->bufsize - j);
                    if (self->active == 1) {
                        if (num <= 0)
                            self->trigsBuffer[0] = 1.0;
                        else
                            self->trigsBuffer[num-1] = 1.0;
                        self->active = 0;
                    }    
                }
                
                upBound = size - self->fadeInSample;
                    
                MYFLT buffer[num];
                memset(&buffer, 0, sizeof(buffer));
                
                for (i=0; i<num; i++) {
                    if (self->pointer < self->fadeInSample) {
                        val = self->pointer / self->fadeInSample;
                    }
                    else if (self->pointer > upBound)
                        val = (size - self->pointer) / self->fadeInSample;
                    else
                        val = 1.;
                    buffer[i] = in[i+j] * val;
                    self->pointer++;
                }
                NewTable_recordChunk((NewTable *)self->table, buffer, num);
                break;
            }
        }
    }
}

static int
TrigTableRec_traverse(TrigTableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->trig);
    Py_VISIT(self->trig_stream);
    Py_VISIT(self->table);
    return 0;
}

static int 
TrigTableRec_clear(TrigTableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->trig);
    Py_CLEAR(self->trig_stream);
    Py_CLEAR(self->table);
    return 0;
}

static void
TrigTableRec_dealloc(TrigTableRec* self)
{
    free(self->data);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    TrigTableRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigTableRec_deleteStream(TrigTableRec *self) { DELETE_STREAM };

static PyObject *
TrigTableRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TrigTableRec *self;
    self = (TrigTableRec *)type->tp_alloc(type, 0);
    
    self->pointer = 0;
    self->active = 0;
    self->fadetime = 0.;
    
    INIT_OBJECT_COMMON
    
    Stream_setFunctionPtr(self->stream, TrigTableRec_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TrigTableRec_init(TrigTableRec *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *trigtmp, *trig_streamtmp, *tabletmp;
    
    static char *kwlist[] = {"input", "trig", "table", "fadetime", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OOO_F, kwlist, &inputtmp, &trigtmp, &tabletmp, &self->fadetime))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;

    Py_XDECREF(self->trig);
    self->trig = trigtmp;
    trig_streamtmp = PyObject_CallMethod((PyObject *)self->trig, "_getStream", NULL);
    Py_INCREF(trig_streamtmp);
    Py_XDECREF(self->trig_stream);
    self->trig_stream = (Stream *)trig_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = (NewTable *)tabletmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    if ((self->fadetime * self->sr) >= (size * 0.5))
        self->fadetime = size * 0.499 / self->sr;
    if (self->fadetime == 0.0)
        self->fadeInSample = 0.0;
    else
        self->fadeInSample = MYROUND(self->fadetime * self->sr + 0.5);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigTableRec_getServer(TrigTableRec* self) { GET_SERVER };
static PyObject * TrigTableRec_getStream(TrigTableRec* self) { GET_STREAM };

static PyObject * TrigTableRec_play(TrigTableRec *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigTableRec_stop(TrigTableRec *self) { STOP };

static PyObject *
TrigTableRec_setTable(TrigTableRec *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->table);
    self->table = (NewTable *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

MYFLT *
TrigTableRec_getTrigsBuffer(TrigTableRec *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    


static PyMemberDef TrigTableRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigTableRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRec, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigTableRec, input), 0, "Input sound object."},
    {"trig", T_OBJECT_EX, offsetof(TrigTableRec, trig), 0, "Trigger object."},
    {"table", T_OBJECT_EX, offsetof(TrigTableRec, table), 0, "Table to record in."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRec_methods[] = {
    {"getServer", (PyCFunction)TrigTableRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigTableRec_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)TrigTableRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"setTable", (PyCFunction)TrigTableRec_setTable, METH_O, "Sets a new table."},
    {"play", (PyCFunction)TrigTableRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigTableRec_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject TrigTableRecType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigTableRec_base",         /*tp_name*/
    sizeof(TrigTableRec),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigTableRec_dealloc, /*tp_dealloc*/
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
    "TrigTableRec objects. Record audio input in a table object.",           /* tp_doc */
    (traverseproc)TrigTableRec_traverse,   /* tp_traverse */
    (inquiry)TrigTableRec_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigTableRec_methods,             /* tp_methods */
    TrigTableRec_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)TrigTableRec_init,      /* tp_init */
    0,                         /* tp_alloc */
    TrigTableRec_new,                 /* tp_new */
};

/************************************************************************************************/
/* TrigTableRecTrig trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    TrigTableRec *mainReader;
} TrigTableRecTrig;

static void
TrigTableRecTrig_compute_next_data_frame(TrigTableRecTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = TrigTableRec_getTrigsBuffer((TrigTableRec *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }
}

static int
TrigTableRecTrig_traverse(TrigTableRecTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
TrigTableRecTrig_clear(TrigTableRecTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
TrigTableRecTrig_dealloc(TrigTableRecTrig* self)
{
    free(self->data);
    TrigTableRecTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigTableRecTrig_deleteStream(TrigTableRecTrig *self) { DELETE_STREAM };

static PyObject *
TrigTableRecTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    TrigTableRecTrig *self;
    self = (TrigTableRecTrig *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigTableRecTrig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
TrigTableRecTrig_init(TrigTableRecTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (TrigTableRec *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigTableRecTrig_getServer(TrigTableRecTrig* self) { GET_SERVER };
static PyObject * TrigTableRecTrig_getStream(TrigTableRecTrig* self) { GET_STREAM };

static PyObject * TrigTableRecTrig_play(TrigTableRecTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigTableRecTrig_stop(TrigTableRecTrig *self) { STOP };

static PyMemberDef TrigTableRecTrig_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigTableRecTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRecTrig, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRecTrig_methods[] = {
    {"getServer", (PyCFunction)TrigTableRecTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigTableRecTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)TrigTableRecTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)TrigTableRecTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)TrigTableRecTrig_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject TrigTableRecTrigType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigTableRecTrig_base",         /*tp_name*/
    sizeof(TrigTableRecTrig),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigTableRecTrig_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigTableRecTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
    (traverseproc)TrigTableRecTrig_traverse,   /* tp_traverse */
    (inquiry)TrigTableRecTrig_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigTableRecTrig_methods,             /* tp_methods */
    TrigTableRecTrig_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)TrigTableRecTrig_init,      /* tp_init */
    0,                         /* tp_alloc */
    TrigTableRecTrig_new,                 /* tp_new */
};
