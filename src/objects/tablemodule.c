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
#include "sndfile.h"
#include "wind.h"

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
    pyo_table_VISIT
    Py_VISIT(self->amplist);
    return 0;
}

static int
HarmTable_clear(HarmTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->amplist);
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
    PyObject *amplist=NULL;
    HarmTable *self;
    self = (HarmTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->amplist = PyList_New(0);
    PyList_Append(self->amplist, PyFloat_FromDouble(1.));
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &amplist, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * HarmTable_getServer(HarmTable* self) { GET_SERVER };
static PyObject * HarmTable_getTableStream(HarmTable* self) { GET_TABLE_STREAM };
static PyObject * HarmTable_setData(HarmTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HarmTable_reset(HarmTable *self) { TABLE_RESET };
static PyObject * HarmTable_normalize(HarmTable *self) { NORMALIZE };
static PyObject * HarmTable_removeDC(HarmTable *self) { REMOVE_DC };
static PyObject * HarmTable_reverse(HarmTable *self) { REVERSE };
static PyObject * HarmTable_invert(HarmTable *self) { INVERT };
static PyObject * HarmTable_rectify(HarmTable *self) { RECTIFY };
static PyObject * HarmTable_bipolarGain(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * HarmTable_lowpass(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * HarmTable_fadein(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * HarmTable_fadeout(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * HarmTable_pow(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * HarmTable_copy(HarmTable *self, PyObject *arg) { COPY };
static PyObject * HarmTable_setTable(HarmTable *self, PyObject *arg) { SET_TABLE };
static PyObject * HarmTable_getTable(HarmTable *self) { GET_TABLE };
static PyObject * HarmTable_getViewTable(HarmTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * HarmTable_put(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HarmTable_get(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * HarmTable_add(HarmTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * HarmTable_sub(HarmTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * HarmTable_mul(HarmTable *self, PyObject *arg) { TABLE_MUL };

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
{"setTable", (PyCFunction)HarmTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)HarmTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)HarmTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)HarmTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"normalize", (PyCFunction)HarmTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)HarmTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)HarmTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)HarmTable_reverse, METH_NOARGS, "Reverse the table's data in time."},
{"invert", (PyCFunction)HarmTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)HarmTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)HarmTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)HarmTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)HarmTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)HarmTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)HarmTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"copy", (PyCFunction)HarmTable_copy, METH_O, "Copy data from table given in argument."},
{"setData", (PyCFunction)HarmTable_setData, METH_O, "Sets the table from samples in a text file."},
{"setSize", (PyCFunction)HarmTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HarmTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)HarmTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)HarmTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"replace", (PyCFunction)HarmTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"add", (PyCFunction)HarmTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)HarmTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)HarmTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
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
    MYFLT amplitude, val, ihalfsize, index, x;

    ampsize = PyList_Size(self->amplist);
    if (ampsize > 12)
        ampsize = 12;
    MYFLT array[ampsize];
    for(j=0; j<ampsize; j++) {
        array[j] =  PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(self->amplist, j)));
    }

    halfsize = self->size / 2;
    ihalfsize = 1.0 / halfsize;

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
    pyo_table_VISIT
    Py_VISIT(self->amplist);
    return 0;
}

static int
ChebyTable_clear(ChebyTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->amplist);
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
    PyObject *amplist=NULL;
    ChebyTable *self;
    self = (ChebyTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->amplist = PyList_New(0);
    PyList_Append(self->amplist, PyFloat_FromDouble(1.));
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &amplist, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * ChebyTable_getServer(ChebyTable* self) { GET_SERVER };
static PyObject * ChebyTable_getTableStream(ChebyTable* self) { GET_TABLE_STREAM };
static PyObject * ChebyTable_setData(ChebyTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ChebyTable_normalize(ChebyTable *self) { NORMALIZE };
static PyObject * ChebyTable_reset(ChebyTable *self) { TABLE_RESET };
static PyObject * ChebyTable_removeDC(ChebyTable *self) { REMOVE_DC };
static PyObject * ChebyTable_reverse(ChebyTable *self) { REVERSE };
static PyObject * ChebyTable_invert(ChebyTable *self) { INVERT };
static PyObject * ChebyTable_rectify(ChebyTable *self) { RECTIFY };
static PyObject * ChebyTable_bipolarGain(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * ChebyTable_lowpass(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * ChebyTable_fadein(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * ChebyTable_fadeout(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * ChebyTable_pow(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * ChebyTable_copy(ChebyTable *self, PyObject *arg) { COPY };
static PyObject * ChebyTable_setTable(ChebyTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ChebyTable_getTable(ChebyTable *self) { GET_TABLE };
static PyObject * ChebyTable_getViewTable(ChebyTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * ChebyTable_put(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ChebyTable_get(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ChebyTable_add(ChebyTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ChebyTable_sub(ChebyTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ChebyTable_mul(ChebyTable *self, PyObject *arg) { TABLE_MUL };

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

static PyObject *
ChebyTable_getNormTable(ChebyTable *self, PyObject *value)
{
    int i;
    int halfsize = self->size / 2;
    MYFLT maxval = 0.0;
    MYFLT val = 0.0, val2 = 0.0;
    MYFLT last = 0.0;
    long sym = PyInt_AS_LONG(value);
    MYFLT samps[halfsize];
    PyObject *samples = PyList_New(halfsize);

    if (sym == 0) {
        for (i=0; i<self->size; i++) {
            if (self->data[i] > maxval)
                maxval = self->data[i];
        }
        if (maxval > 1.0) {
            for (i=0; i<self->size; i++) {
                self->data[i] /= maxval;
            }
        }
        maxval = -1;
        for (i=0; i<halfsize; i++) {
            val = MYFABS(self->data[halfsize+i]);
            if (val > maxval)
                maxval = val;
            if (maxval > 0.0)
                samps[i] = 1. - maxval;
            else
                samps[i] = -1.;
        }
    }
    else {
        maxval = -1;
        for (i=0; i<halfsize; i++) {
            val = MYFABS(self->data[halfsize-i]);
            val2 = MYFABS(self->data[halfsize+i]);
            if (val2 > val)
                val = val2;
            if (val > maxval)
                maxval = val;
            if (maxval > 0.0)
                samps[i] = 1. / maxval;
            else
                samps[i] = -1.;
        }
    }

    maxval = 0.0;
    for (i=0; i<halfsize; i++) {
        val = samps[i];
        if (val > maxval)
            maxval = val;
    }

    for (i=0; i<halfsize; i++) {
        val = samps[i];
        if (val == -1.0)
            samps[i] = maxval;
    }

    last = samps[0];
    for (i=1; i<halfsize; i++) {
        last = samps[i] = samps[i] + (last - samps[i]) * 0.95;
    }

    for (i=0; i<halfsize; i++) {
        PyList_SET_ITEM(samples, i, PyFloat_FromDouble(samps[i]));
    }
    return samples;
}

static PyMemberDef ChebyTable_members[] = {
{"server", T_OBJECT_EX, offsetof(ChebyTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(ChebyTable, tablestream), 0, "Table stream object."},
{"amplist", T_OBJECT_EX, offsetof(ChebyTable, amplist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef ChebyTable_methods[] = {
{"getServer", (PyCFunction)ChebyTable_getServer, METH_NOARGS, "Returns server object."},
{"copy", (PyCFunction)ChebyTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)ChebyTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)ChebyTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)ChebyTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)ChebyTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)ChebyTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)ChebyTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)ChebyTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)ChebyTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)ChebyTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)ChebyTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)ChebyTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)ChebyTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)ChebyTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)ChebyTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)ChebyTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)ChebyTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"setSize", (PyCFunction)ChebyTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)ChebyTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)ChebyTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)ChebyTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"replace", (PyCFunction)ChebyTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"getNormTable", (PyCFunction)ChebyTable_getNormTable, METH_O, "Computes and returns the normalization function for the current polynomial"},
{"add", (PyCFunction)ChebyTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)ChebyTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)ChebyTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
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
    pyo_table_VISIT
    return 0;
}

static int
HannTable_clear(HannTable *self)
{
    pyo_table_CLEAR
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

    static char *kwlist[] = {"size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    HannTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * HannTable_getServer(HannTable* self) { GET_SERVER };
static PyObject * HannTable_getTableStream(HannTable* self) { GET_TABLE_STREAM };
static PyObject * HannTable_setData(HannTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HannTable_normalize(HannTable *self) { NORMALIZE };
static PyObject * HannTable_reset(HannTable *self) { TABLE_RESET };
static PyObject * HannTable_removeDC(HannTable *self) { REMOVE_DC };
static PyObject * HannTable_reverse(HannTable *self) { REVERSE };
static PyObject * HannTable_invert(HannTable *self) { INVERT };
static PyObject * HannTable_rectify(HannTable *self) { RECTIFY };
static PyObject * HannTable_bipolarGain(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * HannTable_lowpass(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * HannTable_fadein(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * HannTable_fadeout(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * HannTable_pow(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * HannTable_copy(HannTable *self, PyObject *arg) { COPY };
static PyObject * HannTable_setTable(HannTable *self, PyObject *arg) { SET_TABLE };
static PyObject * HannTable_getTable(HannTable *self) { GET_TABLE };
static PyObject * HannTable_getViewTable(HannTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * HannTable_put(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HannTable_get(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * HannTable_add(HannTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * HannTable_sub(HannTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * HannTable_mul(HannTable *self, PyObject *arg) { TABLE_MUL };

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
{"copy", (PyCFunction)HannTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)HannTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)HannTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)HannTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)HannTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)HannTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)HannTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)HannTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)HannTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)HannTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)HannTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)HannTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)HannTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)HannTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)HannTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)HannTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)HannTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"setSize", (PyCFunction)HannTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HannTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)HannTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)HannTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"add", (PyCFunction)HannTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)HannTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)HannTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
0,                         /* tp_alloc */
HannTable_new,                 /* tp_new */
};

/***********************/
/* SincTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    MYFLT freq;
    int windowed;
} SincTable;

static void
SincTable_generate(SincTable *self) {
    int i, half, halfMinusOne;
    MYFLT scl, val;

    half = self->size / 2;

    if (self->windowed) {
        halfMinusOne = half - 1;
        for(i=0; i<self->size; i++) {
            scl = (MYFLT)(i - half) / half * self->freq;
            if (scl == 0.0)
                val = 1.0;
            else
                val = (MYSIN(scl) / scl);
            val *= 0.5 + (MYCOS(TWOPI * (i - halfMinusOne) / self->size) * 0.5);
            self->data[i] = val;
        }
    }
    else {
        for(i=0; i<self->size; i++) {
            scl = (MYFLT)(i - half) / half * self->freq;
            if (scl == 0.0)
                val = 1.0;
            else
                val = (MYSIN(scl) / scl);
            self->data[i] = val;
        }
    }
    self->data[self->size] = self->data[0];
}

static int
SincTable_traverse(SincTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
SincTable_clear(SincTable *self)
{
    pyo_table_CLEAR
    return 0;
}

static void
SincTable_dealloc(SincTable* self)
{
    free(self->data);
    SincTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
SincTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SincTable *self;
    self = (SincTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->size = 8192;
    self->freq = TWOPI;
    self->windowed = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"freq", "windowed", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FII, kwlist, &self->freq, &self->windowed, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    SincTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * SincTable_getServer(SincTable* self) { GET_SERVER };
static PyObject * SincTable_getTableStream(SincTable* self) { GET_TABLE_STREAM };
static PyObject * SincTable_setData(SincTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * SincTable_normalize(SincTable *self) { NORMALIZE };
static PyObject * SincTable_reset(SincTable *self) { TABLE_RESET };
static PyObject * SincTable_removeDC(SincTable *self) { REMOVE_DC };
static PyObject * SincTable_reverse(SincTable *self) { REVERSE };
static PyObject * SincTable_invert(SincTable *self) { INVERT };
static PyObject * SincTable_rectify(SincTable *self) { RECTIFY };
static PyObject * SincTable_bipolarGain(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * SincTable_lowpass(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * SincTable_fadein(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * SincTable_fadeout(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * SincTable_pow(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * SincTable_copy(SincTable *self, PyObject *arg) { COPY };
static PyObject * SincTable_setTable(SincTable *self, PyObject *arg) { SET_TABLE };
static PyObject * SincTable_getTable(SincTable *self) { GET_TABLE };
static PyObject * SincTable_getViewTable(SincTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * SincTable_put(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * SincTable_get(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * SincTable_add(SincTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * SincTable_sub(SincTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * SincTable_mul(SincTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
SincTable_setFreq(SincTable *self, PyObject *value)
{

    if (! PyNumber_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The freq attribute value must be a number.");
        return PyInt_FromLong(-1);
    }

    self->freq = PyFloat_AsDouble(PyNumber_Float(value));

    SincTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SincTable_setWindowed(SincTable *self, PyObject *value)
{

    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The windowed attribute value must be a boolean.");
        return PyInt_FromLong(-1);
    }

    self->windowed = PyInt_AsLong(value);

    SincTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SincTable_setSize(SincTable *self, PyObject *value)
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

    SincTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SincTable_getSize(SincTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyMemberDef SincTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(SincTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(SincTable, tablestream), 0, "Table stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SincTable_methods[] = {
    {"getServer", (PyCFunction)SincTable_getServer, METH_NOARGS, "Returns server object."},
    {"copy", (PyCFunction)SincTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)SincTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)SincTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)SincTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)SincTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)SincTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)SincTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)SincTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)SincTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)SincTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)SincTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)SincTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)SincTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)SincTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)SincTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)SincTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)SincTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"setSize", (PyCFunction)SincTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)SincTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"setFreq", (PyCFunction)SincTable_setFreq, METH_O, "Sets the frequency, in radians, of the sinc function."},
    {"setWindowed", (PyCFunction)SincTable_setWindowed, METH_O, "If True, an hanning window is applied on the function."},
    {"put", (PyCFunction)SincTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)SincTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"add", (PyCFunction)SincTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)SincTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)SincTable_mul, METH_O, "Performs table multiplication."},
    {NULL}  /* Sentinel */
};

PyTypeObject SincTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.SincTable_base",         /*tp_name*/
    sizeof(SincTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SincTable_dealloc, /*tp_dealloc*/
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
    "SincTable objects. Generates a table filled with a sinc function.",  /* tp_doc */
    (traverseproc)SincTable_traverse,   /* tp_traverse */
    (inquiry)SincTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SincTable_methods,             /* tp_methods */
    SincTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SincTable_new,                 /* tp_new */
};

/***********************/
/* WinTable structure  */
/***********************/
typedef struct {
    pyo_table_HEAD
    int type;
} WinTable;

static void
WinTable_generate(WinTable *self) {
    gen_window(self->data, self->size, self->type);
    self->data[self->size] = self->data[0];
}

static int
WinTable_traverse(WinTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
WinTable_clear(WinTable *self)
{
    pyo_table_CLEAR
    return 0;
}

static void
WinTable_dealloc(WinTable* self)
{
    free(self->data);
    WinTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
WinTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    WinTable *self;
    self = (WinTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->size = 8192;
    self->type = 2;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"type", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ii", kwlist, &self->type, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    WinTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * WinTable_getServer(WinTable* self) { GET_SERVER };
static PyObject * WinTable_getTableStream(WinTable* self) { GET_TABLE_STREAM };
static PyObject * WinTable_setData(WinTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * WinTable_normalize(WinTable *self) { NORMALIZE };
static PyObject * WinTable_reset(WinTable *self) { TABLE_RESET };
static PyObject * WinTable_removeDC(WinTable *self) { REMOVE_DC };
static PyObject * WinTable_reverse(WinTable *self) { REVERSE };
static PyObject * WinTable_invert(WinTable *self) { INVERT };
static PyObject * WinTable_rectify(WinTable *self) { RECTIFY };
static PyObject * WinTable_bipolarGain(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * WinTable_lowpass(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * WinTable_fadein(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * WinTable_fadeout(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * WinTable_pow(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * WinTable_copy(WinTable *self, PyObject *arg) { COPY };
static PyObject * WinTable_setTable(WinTable *self, PyObject *arg) { SET_TABLE };
static PyObject * WinTable_getTable(WinTable *self) { GET_TABLE };
static PyObject * WinTable_getViewTable(WinTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * WinTable_put(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * WinTable_get(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * WinTable_add(WinTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * WinTable_sub(WinTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * WinTable_mul(WinTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
WinTable_setSize(WinTable *self, PyObject *value)
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

    WinTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
WinTable_getSize(WinTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
WinTable_setType(WinTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the type attribute.");
        return PyInt_FromLong(-1);
    }

    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The type attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }

    self->type = PyInt_AsLong(value);

    WinTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef WinTable_members[] = {
{"server", T_OBJECT_EX, offsetof(WinTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(WinTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef WinTable_methods[] = {
{"getServer", (PyCFunction)WinTable_getServer, METH_NOARGS, "Returns server object."},
{"copy", (PyCFunction)WinTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)WinTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)WinTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)WinTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)WinTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)WinTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)WinTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)WinTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)WinTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)WinTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)WinTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)WinTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)WinTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)WinTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)WinTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)WinTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)WinTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"setSize", (PyCFunction)WinTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)WinTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"setType", (PyCFunction)WinTable_setType, METH_O, "Sets the type of the table."},
{"put", (PyCFunction)WinTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)WinTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"add", (PyCFunction)WinTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)WinTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)WinTable_mul, METH_O, "Performs table multiplication."},
{NULL}  /* Sentinel */
};

PyTypeObject WinTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.WinTable_base",         /*tp_name*/
sizeof(WinTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)WinTable_dealloc, /*tp_dealloc*/
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
"WinTable objects. Generates a table filled with a hanning function.",  /* tp_doc */
(traverseproc)WinTable_traverse,   /* tp_traverse */
(inquiry)WinTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
WinTable_methods,             /* tp_methods */
WinTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
WinTable_new,                 /* tp_new */
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
    pyo_table_VISIT
    return 0;
}

static int
ParaTable_clear(ParaTable *self)
{
    pyo_table_CLEAR
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

    static char *kwlist[] = {"size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    ParaTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * ParaTable_getServer(ParaTable* self) { GET_SERVER };
static PyObject * ParaTable_getTableStream(ParaTable* self) { GET_TABLE_STREAM };
static PyObject * ParaTable_setData(ParaTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ParaTable_normalize(ParaTable *self) { NORMALIZE };
static PyObject * ParaTable_reset(ParaTable *self) { TABLE_RESET };
static PyObject * ParaTable_removeDC(ParaTable *self) { REMOVE_DC };
static PyObject * ParaTable_reverse(ParaTable *self) { REVERSE };
static PyObject * ParaTable_invert(ParaTable *self) { INVERT };
static PyObject * ParaTable_rectify(ParaTable *self) { RECTIFY };
static PyObject * ParaTable_bipolarGain(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * ParaTable_lowpass(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * ParaTable_fadein(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * ParaTable_fadeout(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * ParaTable_pow(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * ParaTable_copy(ParaTable *self, PyObject *arg) { COPY };
static PyObject * ParaTable_setTable(ParaTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ParaTable_getTable(ParaTable *self) { GET_TABLE };
static PyObject * ParaTable_getViewTable(ParaTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * ParaTable_put(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ParaTable_get(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ParaTable_add(ParaTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ParaTable_sub(ParaTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ParaTable_mul(ParaTable *self, PyObject *arg) { TABLE_MUL };

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
    {"copy", (PyCFunction)ParaTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)ParaTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)ParaTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)ParaTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)ParaTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)ParaTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)ParaTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)ParaTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)ParaTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)ParaTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)ParaTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)ParaTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)ParaTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)ParaTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)ParaTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)ParaTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)ParaTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"setSize", (PyCFunction)ParaTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)ParaTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"put", (PyCFunction)ParaTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)ParaTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"add", (PyCFunction)ParaTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)ParaTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)ParaTable_mul, METH_O, "Performs table multiplication."},
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
    0,      /* tp_init */
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
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
LinTable_clear(LinTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
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
    PyObject *pointslist=NULL;
    LinTable *self;
    self = (LinTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * LinTable_getServer(LinTable* self) { GET_SERVER };
static PyObject * LinTable_getTableStream(LinTable* self) { GET_TABLE_STREAM };
static PyObject * LinTable_setData(LinTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * LinTable_normalize(LinTable *self) { NORMALIZE };
static PyObject * LinTable_reset(LinTable *self) { TABLE_RESET };
static PyObject * LinTable_removeDC(LinTable *self) { REMOVE_DC };
static PyObject * LinTable_reverse(LinTable *self) { REVERSE };
static PyObject * LinTable_invert(LinTable *self) { INVERT };
static PyObject * LinTable_rectify(LinTable *self) { RECTIFY };
static PyObject * LinTable_bipolarGain(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * LinTable_lowpass(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * LinTable_fadein(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * LinTable_fadeout(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * LinTable_pow(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * LinTable_copy(LinTable *self, PyObject *arg) { COPY };
static PyObject * LinTable_setTable(LinTable *self, PyObject *arg) { SET_TABLE };
static PyObject * LinTable_getTable(LinTable *self) { GET_TABLE };
static PyObject * LinTable_getViewTable(LinTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * LinTable_put(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * LinTable_get(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * LinTable_add(LinTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * LinTable_sub(LinTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * LinTable_mul(LinTable *self, PyObject *arg) { TABLE_MUL };

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
{"copy", (PyCFunction)LinTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)LinTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)LinTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)LinTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)LinTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)LinTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)LinTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)LinTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)LinTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)LinTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)LinTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)LinTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)LinTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)LinTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)LinTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)LinTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)LinTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"setSize", (PyCFunction)LinTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)LinTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)LinTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)LinTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)LinTable_getPoints, METH_NOARGS, "Return the list of points."},
{"replace", (PyCFunction)LinTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"add", (PyCFunction)LinTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)LinTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)LinTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
0,                         /* tp_alloc */
LinTable_new,                 /* tp_new */
};

/***********************/
/* LogTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
} LogTable;

static void
LogTable_generate(LogTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup, *tup2;
    int x1, y1;
    MYFLT x2, y2, diff, range, logrange, logmin, ratio, low, high;

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
        if (x2 <= 0)
            x2 = 0.000001;
        if (y2 <= 0)
            y2 = 0.000001;
        if (x2 > y2) {
            low = y2;
            high = x2;
        }
        else {
            low = x2;
            high = y2;
        }

        steps = y1 - x1;
        range = high - low;
        logrange = MYLOG10(high) - MYLOG10(low);
        logmin = MYLOG10(low);
        if (steps <= 0)
            continue;
        if (range == 0) {
            for(j=0; j<steps; j++) {
                self->data[x1+j] = x2;
            }
        }
        else {
            diff = (y2 - x2) / steps;
            for(j=0; j<steps; j++) {
                ratio = ((x2 + diff * j) - low) / range;
                self->data[x1+j] = MYPOW(10, ratio * logrange + logmin);
            }
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
LogTable_traverse(LogTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
LogTable_clear(LogTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
    return 0;
}

static void
LogTable_dealloc(LogTable* self)
{
    free(self->data);
    LogTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
LogTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    LogTable *self;
    self = (LogTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

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
    LogTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * LogTable_getServer(LogTable* self) { GET_SERVER };
static PyObject * LogTable_getTableStream(LogTable* self) { GET_TABLE_STREAM };
static PyObject * LogTable_setData(LogTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * LogTable_normalize(LogTable *self) { NORMALIZE };
static PyObject * LogTable_reset(LogTable *self) { TABLE_RESET };
static PyObject * LogTable_removeDC(LogTable *self) { REMOVE_DC };
static PyObject * LogTable_reverse(LogTable *self) { REVERSE };
static PyObject * LogTable_invert(LogTable *self) { INVERT };
static PyObject * LogTable_rectify(LogTable *self) { RECTIFY };
static PyObject * LogTable_bipolarGain(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * LogTable_lowpass(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * LogTable_fadein(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * LogTable_fadeout(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * LogTable_pow(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * LogTable_copy(LogTable *self, PyObject *arg) { COPY };
static PyObject * LogTable_setTable(LogTable *self, PyObject *arg) { SET_TABLE };
static PyObject * LogTable_getTable(LogTable *self) { GET_TABLE };
static PyObject * LogTable_getViewTable(LogTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * LogTable_put(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * LogTable_get(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * LogTable_add(LogTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * LogTable_sub(LogTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * LogTable_mul(LogTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
LogTable_setSize(LogTable *self, PyObject *value)
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

    LogTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
LogTable_getSize(LogTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
LogTable_getPoints(LogTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
LogTable_replace(LogTable *self, PyObject *value)
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

    LogTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef LogTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(LogTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(LogTable, tablestream), 0, "Table stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(LogTable, pointslist), 0, "Harmonics amplitude values."},
    {NULL}  /* Sentinel */
};

static PyMethodDef LogTable_methods[] = {
    {"getServer", (PyCFunction)LogTable_getServer, METH_NOARGS, "Returns server object."},
    {"copy", (PyCFunction)LogTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)LogTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)LogTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)LogTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)LogTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)LogTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)LogTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)LogTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)LogTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)LogTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)LogTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)LogTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)LogTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)LogTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)LogTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)LogTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)LogTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"setSize", (PyCFunction)LogTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)LogTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"put", (PyCFunction)LogTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)LogTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"getPoints", (PyCFunction)LogTable_getPoints, METH_NOARGS, "Return the list of points."},
    {"replace", (PyCFunction)LogTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
    {"add", (PyCFunction)LogTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)LogTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)LogTable_mul, METH_O, "Performs table multiplication."},
    {NULL}  /* Sentinel */
};

PyTypeObject LogTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.LogTable_base",         /*tp_name*/
    sizeof(LogTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)LogTable_dealloc, /*tp_dealloc*/
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
    "LogTable objects. Generates a table filled with one or more logarothmic lines.",  /* tp_doc */
    (traverseproc)LogTable_traverse,   /* tp_traverse */
    (inquiry)LogTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    LogTable_methods,             /* tp_methods */
    LogTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    LogTable_new,                 /* tp_new */
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
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
CosTable_clear(CosTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
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
    PyObject *pointslist=NULL;
    CosTable *self;
    self = (CosTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * CosTable_getServer(CosTable* self) { GET_SERVER };
static PyObject * CosTable_getTableStream(CosTable* self) { GET_TABLE_STREAM };
static PyObject * CosTable_setData(CosTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CosTable_normalize(CosTable *self) { NORMALIZE };
static PyObject * CosTable_reset(CosTable *self) { TABLE_RESET };
static PyObject * CosTable_removeDC(CosTable *self) { REMOVE_DC };
static PyObject * CosTable_reverse(CosTable *self) { REVERSE };
static PyObject * CosTable_invert(CosTable *self) { INVERT };
static PyObject * CosTable_rectify(CosTable *self) { RECTIFY };
static PyObject * CosTable_bipolarGain(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * CosTable_lowpass(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * CosTable_fadein(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * CosTable_fadeout(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * CosTable_pow(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * CosTable_copy(CosTable *self, PyObject *arg) { COPY };
static PyObject * CosTable_setTable(CosTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CosTable_getTable(CosTable *self) { GET_TABLE };
static PyObject * CosTable_getViewTable(CosTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * CosTable_put(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CosTable_get(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CosTable_add(CosTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CosTable_sub(CosTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CosTable_mul(CosTable *self, PyObject *arg) { TABLE_MUL };

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
{"copy", (PyCFunction)CosTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)CosTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)CosTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)CosTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)CosTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setData", (PyCFunction)CosTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)CosTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)CosTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)CosTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)CosTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)CosTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)CosTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)CosTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)CosTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)CosTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)CosTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)CosTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"setSize", (PyCFunction)CosTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)CosTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"put", (PyCFunction)CosTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)CosTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getPoints", (PyCFunction)CosTable_getPoints, METH_NOARGS, "Return the list of points."},
{"replace", (PyCFunction)CosTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{"add", (PyCFunction)CosTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)CosTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)CosTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
0,                         /* tp_alloc */
CosTable_new,                 /* tp_new */
};

/***********************/
/* CosLogTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
} CosLogTable;

static void
CosLogTable_generate(CosLogTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup, *tup2;
    int x1, y1;
    MYFLT x2, y2, range, logrange, logmin, ratio, low, high, mu;

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
        if (x2 <= 0)
            x2 = 0.000001;
        if (y2 <= 0)
            y2 = 0.000001;
        if (x2 > y2) {
            low = y2;
            high = x2;
        }
        else {
            low = x2;
            high = y2;
        }

        steps = y1 - x1;
        range = high - low;
        logrange = MYLOG10(high) - MYLOG10(low);
        logmin = MYLOG10(low);
        if (steps <= 0)
            continue;
        if (range == 0) {
            for(j=0; j<steps; j++) {
                self->data[x1+j] = x2;
            }
        }
        else {
            for(j=0; j<steps; j++) {
                mu = (MYFLT)j / steps;
                mu = (1.0-MYCOS(mu*PI))*0.5;
                mu = x2 *(1.0-mu) + y2*mu;
                ratio = (mu - low) / range;
                self->data[x1+j] = MYPOW(10, ratio * logrange + logmin);
            }
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
CosLogTable_traverse(CosLogTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
CosLogTable_clear(CosLogTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
    return 0;
}

static void
CosLogTable_dealloc(CosLogTable* self)
{
    free(self->data);
    CosLogTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
CosLogTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    CosLogTable *self;
    self = (CosLogTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

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
    CosLogTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * CosLogTable_getServer(CosLogTable* self) { GET_SERVER };
static PyObject * CosLogTable_getTableStream(CosLogTable* self) { GET_TABLE_STREAM };
static PyObject * CosLogTable_setData(CosLogTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CosLogTable_normalize(CosLogTable *self) { NORMALIZE };
static PyObject * CosLogTable_reset(CosLogTable *self) { TABLE_RESET };
static PyObject * CosLogTable_removeDC(CosLogTable *self) { REMOVE_DC };
static PyObject * CosLogTable_reverse(CosLogTable *self) { REVERSE };
static PyObject * CosLogTable_invert(CosLogTable *self) { INVERT };
static PyObject * CosLogTable_rectify(CosLogTable *self) { RECTIFY };
static PyObject * CosLogTable_bipolarGain(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * CosLogTable_lowpass(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * CosLogTable_fadein(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * CosLogTable_fadeout(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * CosLogTable_pow(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * CosLogTable_copy(CosLogTable *self, PyObject *arg) { COPY };
static PyObject * CosLogTable_setTable(CosLogTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CosLogTable_getTable(CosLogTable *self) { GET_TABLE };
static PyObject * CosLogTable_getViewTable(CosLogTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * CosLogTable_put(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CosLogTable_get(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CosLogTable_add(CosLogTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CosLogTable_sub(CosLogTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CosLogTable_mul(CosLogTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
CosLogTable_setSize(CosLogTable *self, PyObject *value)
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

    CosLogTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
CosLogTable_getSize(CosLogTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
CosLogTable_getPoints(CosLogTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
CosLogTable_replace(CosLogTable *self, PyObject *value)
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

    CosLogTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef CosLogTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(CosLogTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(CosLogTable, tablestream), 0, "Table stream object."},
    {"pointslist", T_OBJECT_EX, offsetof(CosLogTable, pointslist), 0, "Harmonics amplitude values."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CosLogTable_methods[] = {
    {"getServer", (PyCFunction)CosLogTable_getServer, METH_NOARGS, "Returns server object."},
    {"copy", (PyCFunction)CosLogTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)CosLogTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)CosLogTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)CosLogTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)CosLogTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)CosLogTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)CosLogTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)CosLogTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)CosLogTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)CosLogTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)CosLogTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)CosLogTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)CosLogTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)CosLogTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)CosLogTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)CosLogTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)CosLogTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"setSize", (PyCFunction)CosLogTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)CosLogTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"put", (PyCFunction)CosLogTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)CosLogTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"getPoints", (PyCFunction)CosLogTable_getPoints, METH_NOARGS, "Return the list of points."},
    {"replace", (PyCFunction)CosLogTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
    {"add", (PyCFunction)CosLogTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)CosLogTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)CosLogTable_mul, METH_O, "Performs table multiplication."},
    {NULL}  /* Sentinel */
};

PyTypeObject CosLogTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.CosLogTable_base",         /*tp_name*/
    sizeof(CosLogTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CosLogTable_dealloc, /*tp_dealloc*/
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
    "CosLogTable objects. Generates a table filled with one or more Cosine-logarithmic lines.",  /* tp_doc */
    (traverseproc)CosLogTable_traverse,   /* tp_traverse */
    (inquiry)CosLogTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    CosLogTable_methods,             /* tp_methods */
    CosLogTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    CosLogTable_new,                 /* tp_new */
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
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
CurveTable_clear(CurveTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
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
    PyObject *pointslist=NULL;
    CurveTable *self;
    self = (CurveTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->tension = 0.0;
    self->bias = 0.0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "tension", "bias", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFFI, kwlist, &pointslist, &self->tension, &self->bias, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * CurveTable_getServer(CurveTable* self) { GET_SERVER };
static PyObject * CurveTable_getTableStream(CurveTable* self) { GET_TABLE_STREAM };
static PyObject * CurveTable_setData(CurveTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CurveTable_normalize(CurveTable * self) { NORMALIZE };
static PyObject * CurveTable_reset(CurveTable * self) { TABLE_RESET };
static PyObject * CurveTable_removeDC(CurveTable *self) { REMOVE_DC };
static PyObject * CurveTable_reverse(CurveTable *self) { REVERSE };
static PyObject * CurveTable_invert(CurveTable *self) { INVERT };
static PyObject * CurveTable_rectify(CurveTable *self) { RECTIFY };
static PyObject * CurveTable_bipolarGain(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * CurveTable_lowpass(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * CurveTable_fadein(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * CurveTable_fadeout(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * CurveTable_pow(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * CurveTable_copy(CurveTable *self, PyObject *arg) { COPY };
static PyObject * CurveTable_setTable(CurveTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CurveTable_getTable(CurveTable *self) { GET_TABLE };
static PyObject * CurveTable_getViewTable(CurveTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * CurveTable_put(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CurveTable_get(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CurveTable_add(CurveTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CurveTable_sub(CurveTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CurveTable_mul(CurveTable *self, PyObject *arg) { TABLE_MUL };

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
{"copy", (PyCFunction)CurveTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)CurveTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)CurveTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)CurveTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
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
{"reset", (PyCFunction)CurveTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)CurveTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)CurveTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)CurveTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)CurveTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)CurveTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)CurveTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)CurveTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)CurveTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)CurveTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"add", (PyCFunction)CurveTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)CurveTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)CurveTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
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
    pyo_table_VISIT
    Py_VISIT(self->pointslist);
    return 0;
}

static int
ExpTable_clear(ExpTable *self)
{
    pyo_table_CLEAR
    Py_CLEAR(self->pointslist);
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
    PyObject *pointslist=NULL;
    ExpTable *self;
    self = (ExpTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->exp = 10.0;
    self->inverse = 1;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "exp", "inverse", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFII, kwlist, &pointslist, &self->exp, &self->inverse, &self->size))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * ExpTable_getServer(ExpTable* self) { GET_SERVER };
static PyObject * ExpTable_getTableStream(ExpTable* self) { GET_TABLE_STREAM };
static PyObject * ExpTable_setData(ExpTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ExpTable_normalize(ExpTable * self) { NORMALIZE };
static PyObject * ExpTable_reset(ExpTable * self) { TABLE_RESET };
static PyObject * ExpTable_removeDC(ExpTable *self) { REMOVE_DC };
static PyObject * ExpTable_reverse(ExpTable *self) { REVERSE };
static PyObject * ExpTable_invert(ExpTable *self) { INVERT };
static PyObject * ExpTable_rectify(ExpTable *self) { RECTIFY };
static PyObject * ExpTable_bipolarGain(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * ExpTable_lowpass(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * ExpTable_fadein(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * ExpTable_fadeout(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * ExpTable_pow(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * ExpTable_copy(ExpTable *self, PyObject *arg) { COPY };
static PyObject * ExpTable_setTable(ExpTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ExpTable_getTable(ExpTable *self) { GET_TABLE };
static PyObject * ExpTable_getViewTable(ExpTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * ExpTable_put(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ExpTable_get(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ExpTable_add(ExpTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ExpTable_sub(ExpTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ExpTable_mul(ExpTable *self, PyObject *arg) { TABLE_MUL };

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
{"copy", (PyCFunction)ExpTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)ExpTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)ExpTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)ExpTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
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
{"reset", (PyCFunction)ExpTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)ExpTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)ExpTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)ExpTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)ExpTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)ExpTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)ExpTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)ExpTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)ExpTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)ExpTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"add", (PyCFunction)ExpTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)ExpTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)ExpTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
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
    MYFLT sr;
    MYFLT start;
    MYFLT stop;
    MYFLT crossfade;
    MYFLT insertPos;
} SndTable;

static void
SndTable_loadSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num, num_items, num_chnls, snd_size, start, stop;
    unsigned int num_count = 0;
    MYFLT *tmp;

    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("SndTable failed to open the file.\n");
        return;
    }
    snd_size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;

    if (self->stop <= 0 || self->stop <= self->start || (self->stop*self->sndSr) > snd_size)
        stop = snd_size;
    else
        stop = (unsigned int)(self->stop * self->sndSr);

    if (self->start < 0 || (self->start*self->sndSr) > snd_size)
        start = 0;
    else
        start = (unsigned int)(self->start * self->sndSr);

    self->size = stop - start;
    num_items = self->size * num_chnls;

    /* Allocate space for the data to be read, then read it. */
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    /* For sound longer than 1 minute, load 30 sec chunks. */
    if (self->size > (self->sndSr * 60 * num_chnls)) {
        tmp = (MYFLT *)malloc(self->sndSr * 30 * num_chnls * sizeof(MYFLT));
        sf_seek(sf, start, SEEK_SET);
        num_items = self->sndSr * 30 * num_chnls;
        do {
            num = SF_READ(sf, tmp, num_items);
            for (i=0; i<num; i++) {
                if ((i % num_chnls) == self->chnl) {
                    self->data[(int)(num_count++)] = tmp[i];
                }
            }
        } while (num == num_items);
        sf_close(sf);
    }
    /* For sound shorter than 1 minute, load sound in one pass. */
    else {
        tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
        sf_seek(sf, start, SEEK_SET);
        num = SF_READ(sf, tmp, num_items);
        sf_close(sf);
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                self->data[(int)(i/num_chnls)] = tmp[i];
            }
        }
    }

    self->data[self->size] = self->data[0];

    self->start = 0.0;
    self->stop = -1.0;
    free(tmp);
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setSamplingRate(self->tablestream, self->sndSr);
    TableStream_setData(self->tablestream, self->data);
}

static void
SndTable_appendSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num_items, num_chnls, snd_size, start, stop, to_load_size, cross_in_samps, cross_point, index, real_index;
    MYFLT *tmp, *tmp_data;
    MYFLT cross_amp;

    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("SndTable failed to open the file.\n");
        return;
    }
    snd_size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;

    if (self->stop <= 0 || self->stop <= self->start || (self->stop*self->sndSr) > snd_size)
        stop = snd_size;
    else
        stop = (unsigned int)(self->stop * self->sndSr);

    if (self->start < 0 || (self->start*self->sndSr) > snd_size)
        start = 0;
    else
        start = (unsigned int)(self->start * self->sndSr);

    to_load_size = stop - start;
    num_items = to_load_size * num_chnls;
    cross_in_samps = (unsigned int)(self->crossfade * self->sr);
    if (cross_in_samps >= to_load_size)
        cross_in_samps = to_load_size - 1;
    if (cross_in_samps >= self->size)
        cross_in_samps = self->size - 1;

    /* Allocate space for the data to be read, then read it. */
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    tmp_data = (MYFLT *)malloc(self->size * sizeof(MYFLT));

    sf_seek(sf, start, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);

    if (cross_in_samps != 0) {
        for (i=0; i<self->size; i++) {
            tmp_data[i] = self->data[i];
        }
    }

    cross_point = self->size - cross_in_samps;
    self->size = self->size + to_load_size - cross_in_samps;
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    if (cross_in_samps != 0) {
        for (i=0; i<cross_point; i++) {
            self->data[i] = tmp_data[i];
        }
    }

    if (self->crossfade == 0.0) {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                real_index = cross_point + index;
                self->data[real_index] = tmp[i];
            }
        }
    }
    else {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                real_index = cross_point + index;
                if (index < cross_in_samps) {
                    cross_amp = MYSQRT(index / (MYFLT)cross_in_samps);
                    self->data[real_index] = tmp[i] * cross_amp + tmp_data[real_index] * (1. - cross_amp);
                }
                else
                    self->data[real_index] = tmp[i];
            }
        }
    }

    self->data[self->size] = self->data[0];

    self->start = 0.0;
    self->stop = -1.0;
    free(tmp);
    free(tmp_data);
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setSamplingRate(self->tablestream, self->sndSr);
    TableStream_setData(self->tablestream, self->data);
}

static void
SndTable_prependSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num_items, num_chnls, snd_size, start, stop, to_load_size, cross_in_samps, cross_point;
    unsigned int index = 0;
    MYFLT *tmp, *tmp_data;
    MYFLT cross_amp;

    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("SndTable failed to open the file.\n");
        return;
    }
    snd_size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;

    if (self->stop <= 0 || self->stop <= self->start || (self->stop*self->sndSr) > snd_size)
        stop = snd_size;
    else
        stop = (unsigned int)(self->stop * self->sndSr);

    if (self->start < 0 || (self->start*self->sndSr) > snd_size)
        start = 0;
    else
        start = (unsigned int)(self->start * self->sndSr);

    to_load_size = stop - start;
    num_items = to_load_size * num_chnls;
    cross_in_samps = (unsigned int)(self->crossfade * self->sr);
    if (cross_in_samps >= to_load_size)
        cross_in_samps = to_load_size - 1;
    if (cross_in_samps >= self->size)
        cross_in_samps = self->size - 1;

    /* Allocate space for the data to be read, then read it. */
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    tmp_data = (MYFLT *)malloc(self->size * sizeof(MYFLT));

    sf_seek(sf, start, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);

    for (i=0; i<self->size; i++) {
        tmp_data[i] = self->data[i];
    }

    cross_point = to_load_size - cross_in_samps;
    self->size = self->size + to_load_size - cross_in_samps;
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    if (self->crossfade == 0.0) {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                self->data[index] = tmp[i];
            }
        }

    }
    else {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                if (index >= cross_point) {
                    cross_amp = MYSQRT((index-cross_point) / (MYFLT)cross_in_samps);
                    self->data[index] = tmp[i] * (1. - cross_amp) + tmp_data[index - cross_point] * cross_amp;
                }
                else
                    self->data[index] = tmp[i];
            }
        }
    }

    for (i=(index+1); i<self->size; i++) {
        self->data[i] = tmp_data[i - cross_point];
    }

    self->data[self->size] = self->data[0];

    self->start = 0.0;
    self->stop = -1.0;
    free(tmp);
    free(tmp_data);
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setSamplingRate(self->tablestream, self->sndSr);
    TableStream_setData(self->tablestream, self->data);
}

static void
SndTable_insertSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num_items, num_chnls, snd_size, start, stop, to_load_size;
    unsigned int cross_in_samps, cross_point, insert_point, index;
    unsigned int read_point = 0;
    unsigned int real_index = 0;
    MYFLT *tmp, *tmp_data;
    MYFLT cross_amp;

    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("SndTable failed to open the file.\n");
        return;
    }
    snd_size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;

    if (self->stop <= 0 || self->stop <= self->start || (self->stop*self->sndSr) > snd_size)
        stop = snd_size;
    else
        stop = (unsigned int)(self->stop * self->sndSr);

    if (self->start < 0 || (self->start*self->sndSr) > snd_size)
        start = 0;
    else
        start = (unsigned int)(self->start * self->sndSr);

    to_load_size = stop - start;
    num_items = to_load_size * num_chnls;

    insert_point = (unsigned int)(self->insertPos * self->sr);
    if (insert_point >= self->size)
        insert_point = self->size - 1;

    cross_in_samps = (unsigned int)(self->crossfade * self->sr);
    if (cross_in_samps >= (to_load_size/2))
        cross_in_samps = (to_load_size/2) - 5;
    if (cross_in_samps >= insert_point)
        cross_in_samps = insert_point - 5;
    if (cross_in_samps >= (self->size - insert_point))
        cross_in_samps = (self->size - insert_point) - 5;

    /* Allocate space for the data to be read, then read it. */
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    tmp_data = (MYFLT *)malloc(self->size * sizeof(MYFLT));

    sf_seek(sf, start, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);

    for (i=0; i<self->size; i++) {
        tmp_data[i] = self->data[i];
    }

    self->size = self->size + to_load_size - (cross_in_samps * 2);
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    cross_point = insert_point - cross_in_samps;

    for (i=0; i<cross_point; i++) {
        self->data[i] = tmp_data[i];
    }

    if (self->crossfade == 0.0) {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                self->data[index+cross_point] = tmp[i];
            }
        }

    }
    else {
        for (i=0; i<num_items; i++) {
            if ((i % num_chnls) == self->chnl) {
                index = (int)(i/num_chnls);
                real_index = index + cross_point;
                if (index <= cross_in_samps) {
                    cross_amp = MYSQRT(index / (MYFLT)cross_in_samps);
                    self->data[real_index] = tmp[i] * cross_amp + tmp_data[cross_point + index] * (1.0 - cross_amp);
                }
                else if (index >= (to_load_size - cross_in_samps)) {
                    cross_amp = MYSQRT((to_load_size - index) / (MYFLT)cross_in_samps);
                    read_point = cross_in_samps - (to_load_size - index) +insert_point;
                    self->data[real_index] = tmp[i] * cross_amp + tmp_data[read_point] * (1.0 - cross_amp);
                }
                else
                    self->data[real_index] = tmp[i];
            }
        }
    }

    read_point++;
    for (i=(real_index+1); i<self->size; i++) {
        self->data[i] = tmp_data[read_point];
        read_point++;
    }

    self->data[self->size] = self->data[0];

    self->start = 0.0;
    self->stop = -1.0;
    free(tmp);
    free(tmp_data);
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setSamplingRate(self->tablestream, self->sndSr);
    TableStream_setData(self->tablestream, self->data);
}

static int
SndTable_traverse(SndTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
SndTable_clear(SndTable *self)
{
    pyo_table_CLEAR
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
    int i;
    SndTable *self;

    self = (SndTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    self->sr = (MYFLT)PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));

    self->chnl = 0;
    self->stop = -1.0;
    self->crossfade = 0.0;
    self->insertPos = 0.0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"path", "chnl", "start", "stop", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_S_IFF, kwlist, &self->path, &self->chnl, &self->start, &self->stop))
        return PyInt_FromLong(-1);

    if (strcmp(self->path, "") == 0) {
        self->size = (int)self->sr;
        self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));
        for (i=0; i<self->size; i++) {
            self->data[i] = 0.0;
        }
        self->data[self->size] = self->data[0];
        self->start = 0.0;
        self->stop = -1.0;
        self->sndSr = (int)self->sr;
        TableStream_setSize(self->tablestream, self->size);
        TableStream_setSamplingRate(self->tablestream, (int)self->sr);
        TableStream_setData(self->tablestream, self->data);
    }
    else {
        SndTable_loadSound(self);
    }

    return (PyObject *)self;
}

static PyObject * SndTable_getServer(SndTable* self) { GET_SERVER };
static PyObject * SndTable_getTableStream(SndTable* self) { GET_TABLE_STREAM };
static PyObject * SndTable_setData(SndTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * SndTable_normalize(SndTable *self) { NORMALIZE };
static PyObject * SndTable_reset(SndTable *self) { TABLE_RESET };
static PyObject * SndTable_removeDC(SndTable *self) { REMOVE_DC };
static PyObject * SndTable_reverse(SndTable *self) { REVERSE };
static PyObject * SndTable_invert(SndTable *self) { INVERT };
static PyObject * SndTable_rectify(SndTable *self) { RECTIFY };
static PyObject * SndTable_bipolarGain(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * SndTable_lowpass(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * SndTable_fadein(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * SndTable_fadeout(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * SndTable_pow(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * SndTable_copy(SndTable *self, PyObject *arg) { COPY };
static PyObject * SndTable_setTable(SndTable *self, PyObject *arg) { SET_TABLE };
static PyObject * SndTable_getTable(SndTable *self) { GET_TABLE };
static PyObject * SndTable_put(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * SndTable_get(SndTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * SndTable_add(SndTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * SndTable_sub(SndTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * SndTable_mul(SndTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
SndTable_getViewTable(SndTable *self, PyObject *args, PyObject *kwds) {
    int i, j, y, w, h, h2, step, size;
    int count = 0;
    int yOffset = 0;
    MYFLT absin, fstep;
    MYFLT begin = 0.0;
    MYFLT end = -1.0;
    PyObject *samples, *tuple;
    PyObject *sizetmp = NULL;

    static char *kwlist[] = {"size", "begin", "end", "yOffset", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFFI, kwlist, &sizetmp, &begin, &end, &yOffset))
        return PyInt_FromLong(-1);

    if (end <= 0.0)
        end = self->size;
    else {
        end = end * self->sr;
        if (end > self->size)
            end = self->size;
    }

    if (begin < 0.0)
        begin = 0;
    else {
        begin = begin * self->sr;
        if (begin >= end)
            begin = 0;
    }
    size = (int)(end - begin);

    if (sizetmp) {
        if (PyTuple_Check(sizetmp)) {
            w = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 0));
            h = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 1));
        }
        else if (PyList_Check(sizetmp)) {
            w = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 0));
            h = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 1));
        }
        else {
            w = 500;
            h = 200;
        }
    }
    else {
        w = 500;
        h = 200;
    }
    h2 = h/2;
    step = (int)(size / (MYFLT)(w));
    fstep = (MYFLT)(w) / (size-1);

    if (step == 0) {
        samples = PyList_New(size);
        for (i=0; i<size; i++) {
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong((int)(i*fstep)));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(-self->data[i+(int)(begin)]*h2+h2+yOffset));
            PyList_SetItem(samples, i, tuple);
        }
    }
    else if (step < 32) {
        samples = PyList_New(w);
        for(i=0; i<w; i++) {
            absin = 0.0;
            for (j=0; j<step; j++) {
                absin += -self->data[(int)(begin)+count];
                count++;
            }
            y = (int)(absin / step * h2);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2+y+yOffset));
            PyList_SetItem(samples, i, tuple);
        }
    }
    else {
        samples = PyList_New(w*2);
        for(i=0; i<w; i++) {
            absin = 0.0;
            for (j=0; j<step; j++) {
                if (MYFABS(self->data[(int)(begin)+count]) > absin)
                    absin = -self->data[(int)(begin)+count];
                count++;
            }
            y = (int)(absin * h2);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2-y+yOffset));
            PyList_SetItem(samples, i*2, tuple);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2+y+yOffset));
            PyList_SetItem(samples, i*2+1, tuple);
        }
    }
    return samples;
};

static PyObject *
SndTable_getEnvelope(SndTable *self, PyObject *arg) {
    int i, j, step, points;
    long count;
    MYFLT absin, last;
    PyObject *samples;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	int isInt = PyInt_Check(arg);

    if (isInt) {
        count = 0;
        points = PyInt_AsLong(arg);
        step = self->size / points;
        samples = PyList_New(points);
        for(i=0; i<points; i++) {
            last = 0.0;
            absin = 0.0;
            for (j=0; j<step; j++) {
                if (MYFABS(self->data[count++]) > absin)
                    absin = self->data[count];
            }
            last = (absin + last) * 0.5;
            PyList_SetItem(samples, i, PyFloat_FromDouble(last));
        }
        return samples;
    }
    else {
        Py_INCREF(Py_None);
		return Py_None;
    }
};

static PyObject *
SndTable_setSound(SndTable *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"path", "chnl", "start", "stop", NULL};

    MYFLT stoptmp = -1.0;

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_S_IFF, kwlist, &self->path, &self->chnl, &self->start, &stoptmp)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    self->stop = stoptmp;
    SndTable_loadSound(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SndTable_append(SndTable *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"path", "crossfade", "chnl", "start", "stop", NULL};

    MYFLT stoptmp = -1.0;
    MYFLT crosstmp = 0.0;

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_S_FIFF, kwlist, &self->path, &crosstmp, &self->chnl, &self->start, &stoptmp)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    self->stop = stoptmp;
    if (crosstmp < 0.0)
        self->crossfade = 0.0;
    else
        self->crossfade = crosstmp;

    SndTable_appendSound(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SndTable_insert(SndTable *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"path", "pos", "crossfade", "chnl", "start", "stop", NULL};

    MYFLT stoptmp = -1.0;
    MYFLT crosstmp = 0.0;
    MYFLT postmp = 0.0;

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_S_FFIFF, kwlist, &self->path, &postmp, &crosstmp, &self->chnl, &self->start, &stoptmp)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    self->stop = stoptmp;
    if (crosstmp < 0.0)
        self->crossfade = 0.0;
    else
        self->crossfade = crosstmp;

    if (postmp <= 0.0)
        SndTable_prependSound(self);
    else if (postmp >= ((self->size-1) / self->sndSr))
        SndTable_appendSound(self);
    else {
        self->insertPos = postmp;
        SndTable_insertSound(self);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SndTable_setSize(SndTable *self, PyObject *value)
{
    Py_ssize_t i;

    self->size = PyInt_AsLong(value);

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));

    for(i=0; i<self->size; i++) {
        self->data[i] = 0.0;
    }
    self->data[self->size] = 0.0;
    self->start = 0.0;
    self->stop = -1.0;
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);

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
{"copy", (PyCFunction)SndTable_copy, METH_O, "Copy data from table given in argument."},
{"setTable", (PyCFunction)SndTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)SndTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)SndTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)SndTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"getEnvelope", (PyCFunction)SndTable_getEnvelope, METH_O, "Returns X points envelope follower of the table."},
{"setData", (PyCFunction)SndTable_setData, METH_O, "Sets the table from samples in a text file."},
{"normalize", (PyCFunction)SndTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)SndTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)SndTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)SndTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)SndTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)SndTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)SndTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)SndTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)SndTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)SndTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)SndTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"put", (PyCFunction)SndTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)SndTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"setSound", (PyCFunction)SndTable_setSound, METH_VARARGS|METH_KEYWORDS, "Load a new sound in the table."},
{"append", (PyCFunction)SndTable_append, METH_VARARGS|METH_KEYWORDS, "Append a sound in the table."},
{"insert", (PyCFunction)SndTable_insert, METH_VARARGS|METH_KEYWORDS, "Insert a sound in the table."},
{"setSize", (PyCFunction)SndTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)SndTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getRate", (PyCFunction)SndTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{"add", (PyCFunction)SndTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)SndTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)SndTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
0,                         /* tp_alloc */
SndTable_new,                 /* tp_new */
0,               /* tp_free */
0,               /* tp_is_gc */
0,               /* tp_bases */
0,               /* tp_mro */
0,               /* tp_cache */
0,               /* tp_subclasses */
0,               /* tp_weaklist */
0,               /* tp_del */
};

/***********************/
/* NewTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    MYFLT length;
    MYFLT feedback;
    MYFLT sr;
    int pointer;
} NewTable;

static PyObject *
NewTable_recordChunk(NewTable *self, MYFLT *data, int datasize)
{
    int i;

    if (self->feedback == 0.0) {
        for (i=0; i<datasize; i++) {
            self->data[self->pointer++] = data[i];
            if (self->pointer == self->size) {
                self->pointer = 0;
                self->data[self->size] = self->data[0];
            }
        }
    }
    else {
        for (i=0; i<datasize; i++) {
            self->data[self->pointer] = data[i] + self->data[self->pointer] * self->feedback;
            self->pointer++;
            if (self->pointer == self->size) {
                self->pointer = 0;
                self->data[self->size] = self->data[0];
            }
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static int
NewTable_traverse(NewTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
NewTable_clear(NewTable *self)
{
    pyo_table_CLEAR
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
    int i;
    PyObject *inittmp=NULL;
    NewTable *self;
    self = (NewTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointer = 0;
    self->feedback = 0.0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"length", "init", "feedback", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_OF, kwlist, &self->length, &inittmp, &self->feedback))
        Py_RETURN_NONE;

    self->sr = (MYFLT)PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    self->size = (int)(self->length * self->sr + 0.5);
    self->data = (MYFLT *)realloc(self->data, (self->size + 1) * sizeof(MYFLT));

    for (i=0; i<(self->size+1); i++) {
        self->data[i] = 0.;
    }

    TableStream_setSize(self->tablestream, self->size);

    if (inittmp && inittmp != Py_None) {
        PyObject_CallMethod((PyObject *)self, "setTable", "O", inittmp);
    }

    TableStream_setData(self->tablestream, self->data);
    TableStream_setSamplingRate(self->tablestream, self->sr);

    return (PyObject *)self;
}

static PyObject * NewTable_getServer(NewTable* self) { GET_SERVER };
static PyObject * NewTable_getTableStream(NewTable* self) { GET_TABLE_STREAM };
static PyObject * NewTable_setData(NewTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * NewTable_normalize(NewTable *self) { NORMALIZE };
static PyObject * NewTable_reset(NewTable *self) { TABLE_RESET };
static PyObject * NewTable_removeDC(NewTable *self) { REMOVE_DC };
static PyObject * NewTable_reverse(NewTable *self) { REVERSE };
static PyObject * NewTable_invert(NewTable *self) { INVERT };
static PyObject * NewTable_rectify(NewTable *self) { RECTIFY };
static PyObject * NewTable_bipolarGain(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * NewTable_lowpass(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * NewTable_fadein(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * NewTable_fadeout(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * NewTable_pow(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * NewTable_copy(NewTable *self, PyObject *arg) { COPY };
static PyObject * NewTable_setTable(NewTable *self, PyObject *arg) { SET_TABLE };
static PyObject * NewTable_getTable(NewTable *self) { GET_TABLE };
static PyObject * NewTable_put(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * NewTable_get(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * NewTable_add(NewTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * NewTable_sub(NewTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * NewTable_mul(NewTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
NewTable_getViewTable(NewTable *self, PyObject *args, PyObject *kwds) {
    int i, j, y, w, h, h2, step, size;
    int count = 0;
    int yOffset = 0;
    MYFLT absin, fstep;
    MYFLT begin = 0.0;
    MYFLT end = -1.0;
    PyObject *samples, *tuple;
    PyObject *sizetmp = NULL;

    static char *kwlist[] = {"size", "begin", "end", "yOffset", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFFI, kwlist, &sizetmp, &begin, &end, &yOffset))
        return PyInt_FromLong(-1);

    if (end <= 0.0)
        end = self->size;
    else {
        end = end * self->sr;
        if (end > self->size)
            end = self->size;
    }

    if (begin < 0.0)
        begin = 0;
    else {
        begin = begin * self->sr;
        if (begin >= end)
            begin = 0;
    }
    size = (int)(end - begin);

    if (sizetmp) {
        if (PyTuple_Check(sizetmp)) {
            w = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 0));
            h = PyInt_AsLong(PyTuple_GET_ITEM(sizetmp, 1));
        }
        else if (PyList_Check(sizetmp)) {
            w = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 0));
            h = PyInt_AsLong(PyList_GET_ITEM(sizetmp, 1));
        }
        else {
            w = 500;
            h = 200;
        }
    }
    else {
        w = 500;
        h = 200;
    }
    h2 = h/2;
    step = (int)(size / (MYFLT)(w));
    fstep = (MYFLT)(w) / (size-1);

    if (step == 0) {
        samples = PyList_New(size);
        for (i=0; i<size; i++) {
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong((int)(i*fstep)));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(-self->data[i+(int)(begin)]*h2+h2+yOffset));
            PyList_SetItem(samples, i, tuple);
        }
    }
    else if (step < 32) {
        samples = PyList_New(w);
        for(i=0; i<w; i++) {
            absin = 0.0;
            for (j=0; j<step; j++) {
                absin += -self->data[(int)(begin)+count];
                count++;
            }
            y = (int)(absin / step * h2);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2+y+yOffset));
            PyList_SetItem(samples, i, tuple);
        }
    }
    else {
        samples = PyList_New(w*2);
        for(i=0; i<w; i++) {
            absin = 0.0;
            for (j=0; j<step; j++) {
                if (MYFABS(self->data[(int)(begin)+count]) > absin)
                    absin = -self->data[(int)(begin)+count];
                count++;
            }
            y = (int)(absin * h2);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2-y+yOffset));
            PyList_SetItem(samples, i*2, tuple);
            tuple = PyTuple_New(2);
            PyTuple_SetItem(tuple, 0, PyInt_FromLong(i));
            PyTuple_SetItem(tuple, 1, PyInt_FromLong(h2+y+yOffset));
            PyList_SetItem(samples, i*2+1, tuple);
        }
    }
    return samples;
};

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
{"setTable", (PyCFunction)NewTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
{"getTable", (PyCFunction)NewTable_getTable, METH_NOARGS, "Returns a list of table samples."},
{"getViewTable", (PyCFunction)NewTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
{"getTableStream", (PyCFunction)NewTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setFeedback", (PyCFunction)NewTable_setFeedback, METH_O, "Feedback sets the amount of old data to mix with a new recording."},
{"setData", (PyCFunction)NewTable_setData, METH_O, "Sets the table from samples in a text file."},
{"copy", (PyCFunction)NewTable_copy, METH_O, "Copy data from table given in argument."},
{"normalize", (PyCFunction)NewTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"reset", (PyCFunction)NewTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
{"removeDC", (PyCFunction)NewTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
{"reverse", (PyCFunction)NewTable_reverse, METH_NOARGS, "Reverse the table's data."},
{"invert", (PyCFunction)NewTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
{"rectify", (PyCFunction)NewTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
{"bipolarGain", (PyCFunction)NewTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
{"lowpass", (PyCFunction)NewTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
{"fadein", (PyCFunction)NewTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
{"fadeout", (PyCFunction)NewTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
{"pow", (PyCFunction)NewTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
{"put", (PyCFunction)NewTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
{"get", (PyCFunction)NewTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
{"getSize", (PyCFunction)NewTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getLength", (PyCFunction)NewTable_getLength, METH_NOARGS, "Return the length of the table in seconds."},
{"getRate", (PyCFunction)NewTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{"add", (PyCFunction)NewTable_add, METH_O, "Performs table addition."},
{"sub", (PyCFunction)NewTable_sub, METH_O, "Performs table substraction."},
{"mul", (PyCFunction)NewTable_mul, METH_O, "Performs table multiplication."},
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
0,      /* tp_init */
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

static void
DataTable_record(DataTable *self, int pos, MYFLT value)
{
    self->data[pos] = value;
}

static int
DataTable_traverse(DataTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
DataTable_clear(DataTable *self)
{
    pyo_table_CLEAR
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
    int i;
    PyObject *inittmp=NULL;
    DataTable *self;
    self = (DataTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->pointer = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"size", "init", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i|O", kwlist, &self->size, &inittmp))
        Py_RETURN_NONE;

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

    return (PyObject *)self;
}

static PyObject * DataTable_getServer(DataTable* self) { GET_SERVER };
static PyObject * DataTable_getTableStream(DataTable* self) { GET_TABLE_STREAM };
static PyObject * DataTable_setData(DataTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * DataTable_normalize(DataTable *self) { NORMALIZE };
static PyObject * DataTable_reset(DataTable *self) { TABLE_RESET };
static PyObject * DataTable_removeDC(DataTable *self) { REMOVE_DC };
static PyObject * DataTable_reverse(DataTable *self) { REVERSE };
static PyObject * DataTable_invert(DataTable *self) { INVERT };
static PyObject * DataTable_rectify(DataTable *self) { RECTIFY };
static PyObject * DataTable_bipolarGain(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * DataTable_lowpass(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * DataTable_fadein(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * DataTable_fadeout(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * DataTable_pow(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * DataTable_copy(DataTable *self, PyObject *arg) { COPY };
static PyObject * DataTable_setTable(DataTable *self, PyObject *arg) { SET_TABLE };
static PyObject * DataTable_getTable(DataTable *self) { GET_TABLE };
static PyObject * DataTable_getViewTable(DataTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * DataTable_put(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * DataTable_get(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * DataTable_add(DataTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * DataTable_sub(DataTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * DataTable_mul(DataTable *self, PyObject *arg) { TABLE_MUL };

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

static PyMemberDef DataTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(DataTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(DataTable, tablestream), 0, "Table stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef DataTable_methods[] = {
    {"getServer", (PyCFunction)DataTable_getServer, METH_NOARGS, "Returns server object."},
    {"copy", (PyCFunction)DataTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)DataTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)DataTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)DataTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)DataTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)DataTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)DataTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)DataTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)DataTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)DataTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)DataTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)DataTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)DataTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)DataTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)DataTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)DataTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)DataTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"put", (PyCFunction)DataTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)DataTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"getSize", (PyCFunction)DataTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
    {"getRate", (PyCFunction)DataTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
    {"add", (PyCFunction)DataTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)DataTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)DataTable_mul, METH_O, "Performs table multiplication."},
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
    0,      /* tp_init */
    0,                         /* tp_alloc */
    DataTable_new,                 /* tp_new */
};

/***********************/
/* AtanTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    MYFLT slope;
} AtanTable;

static void
AtanTable_generate(AtanTable *self) {
    int i, hsize;
    MYFLT drv, invhsize, val, t, fac = 0;

    hsize = self->size / 2;
    invhsize = 1.0 / hsize;

    drv = 1 - self->slope;
    drv = drv * drv * drv * PI;
    for(i=0; i<=hsize; i++) {
        t = i * invhsize - 1;
        val = MYATAN2(t, drv);
        if (i == 0)
            fac = 1.0 / -val;
        val = val * fac;
        self->data[i] = val;
        self->data[self->size - i] = -val;
    }
}

static int
AtanTable_traverse(AtanTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
AtanTable_clear(AtanTable *self)
{
    pyo_table_CLEAR
    return 0;
}

static void
AtanTable_dealloc(AtanTable* self)
{
    free(self->data);
    AtanTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
AtanTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    AtanTable *self;
    self = (AtanTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();

    self->size = 8192;
    self->slope = 0.5;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"slope", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->slope, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
	TableStream_setData(self->tablestream, self->data);
    AtanTable_generate(self);

    double sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL));
    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * AtanTable_getServer(AtanTable* self) { GET_SERVER };
static PyObject * AtanTable_getTableStream(AtanTable* self) { GET_TABLE_STREAM };
static PyObject * AtanTable_setData(AtanTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * AtanTable_normalize(AtanTable *self) { NORMALIZE };
static PyObject * AtanTable_reset(AtanTable *self) { TABLE_RESET };
static PyObject * AtanTable_removeDC(AtanTable *self) { REMOVE_DC };
static PyObject * AtanTable_reverse(AtanTable *self) { REVERSE };
static PyObject * AtanTable_invert(AtanTable *self) { INVERT };
static PyObject * AtanTable_rectify(AtanTable *self) { RECTIFY };
static PyObject * AtanTable_bipolarGain(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * AtanTable_lowpass(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * AtanTable_fadein(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * AtanTable_fadeout(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * AtanTable_pow(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * AtanTable_copy(AtanTable *self, PyObject *arg) { COPY };
static PyObject * AtanTable_setTable(AtanTable *self, PyObject *arg) { SET_TABLE };
static PyObject * AtanTable_getTable(AtanTable *self) { GET_TABLE };
static PyObject * AtanTable_getViewTable(AtanTable *self, PyObject *args, PyObject *kwds) { GET_VIEW_TABLE };
static PyObject * AtanTable_put(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * AtanTable_get(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * AtanTable_add(AtanTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * AtanTable_sub(AtanTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * AtanTable_mul(AtanTable *self, PyObject *arg) { TABLE_MUL };

static PyObject *
AtanTable_setSlope(AtanTable *self, PyObject *value)
{

    if (! PyNumber_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The slope attribute value must be a number.");
        return PyInt_FromLong(-1);
    }

    self->slope = PyFloat_AsDouble(PyNumber_Float(value));
    if (self->slope < 0.0)
        self->slope = 0.0;
    else if (self->slope > 1.0)
        self->slope = 1.0;

    AtanTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
AtanTable_setSize(AtanTable *self, PyObject *value)
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

    AtanTable_generate(self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
AtanTable_getSize(AtanTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyMemberDef AtanTable_members[] = {
    {"server", T_OBJECT_EX, offsetof(AtanTable, server), 0, "Pyo server."},
    {"tablestream", T_OBJECT_EX, offsetof(AtanTable, tablestream), 0, "Table stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef AtanTable_methods[] = {
    {"getServer", (PyCFunction)AtanTable_getServer, METH_NOARGS, "Returns server object."},
    {"copy", (PyCFunction)AtanTable_copy, METH_O, "Copy data from table given in argument."},
    {"setTable", (PyCFunction)AtanTable_setTable, METH_O, "Sets the table content from a list of floats (must be the same size as the object size)."},
    {"getTable", (PyCFunction)AtanTable_getTable, METH_NOARGS, "Returns a list of table samples."},
    {"getViewTable", (PyCFunction)AtanTable_getViewTable, METH_VARARGS|METH_KEYWORDS, "Returns a list of pixel coordinates for drawing the table."},
    {"getTableStream", (PyCFunction)AtanTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
    {"setData", (PyCFunction)AtanTable_setData, METH_O, "Sets the table from samples in a text file."},
    {"normalize", (PyCFunction)AtanTable_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
    {"reset", (PyCFunction)AtanTable_reset, METH_NOARGS, "Resets table samples to 0.0"},
    {"removeDC", (PyCFunction)AtanTable_removeDC, METH_NOARGS, "Filter out DC offset from the table's data."},
    {"reverse", (PyCFunction)AtanTable_reverse, METH_NOARGS, "Reverse the table's data."},
    {"invert", (PyCFunction)AtanTable_invert, METH_NOARGS, "Reverse the table's data in amplitude."},
    {"rectify", (PyCFunction)AtanTable_rectify, METH_NOARGS, "Positive rectification of the table's data."},
    {"bipolarGain", (PyCFunction)AtanTable_bipolarGain, METH_VARARGS|METH_KEYWORDS, "Apply different amp values to positive and negative samples."},
    {"lowpass", (PyCFunction)AtanTable_lowpass, METH_VARARGS|METH_KEYWORDS, "Apply a one-pole lowpass filter on table's samples."},
    {"fadein", (PyCFunction)AtanTable_fadein, METH_VARARGS|METH_KEYWORDS, "Apply a gradual increase in the level of the table's samples."},
    {"fadeout", (PyCFunction)AtanTable_fadeout, METH_VARARGS|METH_KEYWORDS, "Apply a gradual decrease in the level of the table's samples."},
    {"pow", (PyCFunction)AtanTable_pow, METH_VARARGS|METH_KEYWORDS, "Apply a power function on each sample in the table."},
    {"setSize", (PyCFunction)AtanTable_setSize, METH_O, "Sets the size of the table in samples"},
    {"getSize", (PyCFunction)AtanTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
    {"setSlope", (PyCFunction)AtanTable_setSlope, METH_O, "Sets the slope of the atan function."},
    {"put", (PyCFunction)AtanTable_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the table."},
    {"get", (PyCFunction)AtanTable_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the table."},
    {"add", (PyCFunction)AtanTable_add, METH_O, "Performs table addition."},
    {"sub", (PyCFunction)AtanTable_sub, METH_O, "Performs table substraction."},
    {"mul", (PyCFunction)AtanTable_mul, METH_O, "Performs table multiplication."},
    {NULL}  /* Sentinel */
};

PyTypeObject AtanTableType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.AtanTable_base",         /*tp_name*/
    sizeof(AtanTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)AtanTable_dealloc, /*tp_dealloc*/
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
    "AtanTable objects. Generates a table filled with a sinc function.",  /* tp_doc */
    (traverseproc)AtanTable_traverse,   /* tp_traverse */
    (inquiry)AtanTable_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    AtanTable_methods,             /* tp_methods */
    AtanTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    AtanTable_new,                 /* tp_new */
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
    TriggerStream *trig_stream;
    MYFLT *time_buffer_streams;
    MYFLT *buffer;
} TableRec;

static void
TableRec_compute_next_data_frame(TableRec *self)
{
    int i, num, upBound;
    MYFLT val;
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    if (!self->active) {
        for (i=0; i<self->bufsize; i++) {
            self->time_buffer_streams[i] = self->pointer;
        }
    }

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
        upBound = (int)(size - self->fadeInSample);

        //MYFLT buffer[num];
        //memset(&buffer, 0, sizeof(buffer));
        for (i=0; i<self->bufsize; i++) {
            self->buffer[i] = 0.0;
        }
        MYFLT *in = Stream_getData((Stream *)self->input_stream);

        for (i=0; i<num; i++) {
            if (self->pointer < self->fadeInSample)
                val = self->pointer / self->fadeInSample;
            else if (self->pointer >= upBound)
                val = (size - (self->pointer+1)) / self->fadeInSample;
            else
                val = 1.;
            self->buffer[i] = in[i] * val;
            self->time_buffer_streams[i] = self->pointer++;
        }
        NewTable_recordChunk((NewTable *)self->table, self->buffer, num);

        if (num < self->bufsize) {
            for (i=num; i<self->bufsize; i++) {
                self->time_buffer_streams[i] = self->pointer;
            }
        }
    }
}

static MYFLT *
TableRec_getTimeBuffer(TableRec *self) {
    return self->time_buffer_streams;
}

static int
TableRec_traverse(TableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
TableRec_clear(TableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
TableRec_dealloc(TableRec* self)
{
    pyo_DEALLOC
    free(self->buffer);
    free(self->trigsBuffer);
    free(self->time_buffer_streams);
    TableRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp;
    TableRec *self;
    self = (TableRec *)type->tp_alloc(type, 0);

    self->pointer = 0;
    self->active = 1;
    self->fadetime = 0.;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableRec_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 0);

    static char *kwlist[] = {"input", "table", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_F, kwlist, &inputtmp, &tabletmp, &self->fadetime))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableRec must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    Py_INCREF(tabletmp);
    self->table = (NewTable *)tabletmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer = (MYFLT *)realloc(self->buffer, self->bufsize * sizeof(MYFLT));
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->time_buffer_streams = (MYFLT *)realloc(self->time_buffer_streams, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->buffer[i] = self->trigsBuffer[i] = self->time_buffer_streams[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    if ((self->fadetime * self->sr) >= (size * 0.5))
        self->fadetime = size * 0.499 / self->sr;
    if (self->fadetime == 0.0)
        self->fadeInSample = 0.0;
    else
        self->fadeInSample = MYFLOOR(self->fadetime * self->sr);

    return (PyObject *)self;
}

static PyObject * TableRec_getServer(TableRec* self) { GET_SERVER };
static PyObject * TableRec_getStream(TableRec* self) { GET_STREAM };
static PyObject * TableRec_getTriggerStream(TableRec* self) { GET_TRIGGER_STREAM };

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

static PyMemberDef TableRec_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRec, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRec, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(TableRec, trig_stream), 0, "Trigger Stream object."},
{"input", T_OBJECT_EX, offsetof(TableRec, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TableRec, table), 0, "Table to record in."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRec_methods[] = {
{"getServer", (PyCFunction)TableRec_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRec_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)TableRec_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
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
0,      /* tp_init */
0,                         /* tp_alloc */
TableRec_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    TableRec *mainPlayer;
    int modebuffer[2];
} TableRecTimeStream;

static void TableRecTimeStream_postprocessing_ii(TableRecTimeStream *self) { POST_PROCESSING_II };
static void TableRecTimeStream_postprocessing_ai(TableRecTimeStream *self) { POST_PROCESSING_AI };
static void TableRecTimeStream_postprocessing_ia(TableRecTimeStream *self) { POST_PROCESSING_IA };
static void TableRecTimeStream_postprocessing_aa(TableRecTimeStream *self) { POST_PROCESSING_AA };
static void TableRecTimeStream_postprocessing_ireva(TableRecTimeStream *self) { POST_PROCESSING_IREVA };
static void TableRecTimeStream_postprocessing_areva(TableRecTimeStream *self) { POST_PROCESSING_AREVA };
static void TableRecTimeStream_postprocessing_revai(TableRecTimeStream *self) { POST_PROCESSING_REVAI };
static void TableRecTimeStream_postprocessing_revaa(TableRecTimeStream *self) { POST_PROCESSING_REVAA };
static void TableRecTimeStream_postprocessing_revareva(TableRecTimeStream *self) { POST_PROCESSING_REVAREVA };

static void
TableRecTimeStream_setProcMode(TableRecTimeStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TableRecTimeStream_postprocessing_revareva;
            break;
    }
}

static void
TableRecTimeStream_compute_next_data_frame(TableRecTimeStream *self)
{
    int i;
    MYFLT *tmp;
    tmp = TableRec_getTimeBuffer((TableRec *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TableRecTimeStream_traverse(TableRecTimeStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TableRecTimeStream_clear(TableRecTimeStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TableRecTimeStream_dealloc(TableRecTimeStream* self)
{
    pyo_DEALLOC
    TableRecTimeStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableRecTimeStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TableRecTimeStream *self;
    self = (TableRecTimeStream *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TableRecTimeStream_compute_next_data_frame);
    self->mode_func_ptr = TableRecTimeStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TableRec *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TableRecTimeStream_getServer(TableRecTimeStream* self) { GET_SERVER };
static PyObject * TableRecTimeStream_getStream(TableRecTimeStream* self) { GET_STREAM };
static PyObject * TableRecTimeStream_setMul(TableRecTimeStream *self, PyObject *arg) { SET_MUL };
static PyObject * TableRecTimeStream_setAdd(TableRecTimeStream *self, PyObject *arg) { SET_ADD };
static PyObject * TableRecTimeStream_setSub(TableRecTimeStream *self, PyObject *arg) { SET_SUB };
static PyObject * TableRecTimeStream_setDiv(TableRecTimeStream *self, PyObject *arg) { SET_DIV };

static PyObject * TableRecTimeStream_play(TableRecTimeStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableRecTimeStream_out(TableRecTimeStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TableRecTimeStream_stop(TableRecTimeStream *self) { STOP };

static PyObject * TableRecTimeStream_multiply(TableRecTimeStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TableRecTimeStream_inplace_multiply(TableRecTimeStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TableRecTimeStream_add(TableRecTimeStream *self, PyObject *arg) { ADD };
static PyObject * TableRecTimeStream_inplace_add(TableRecTimeStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TableRecTimeStream_sub(TableRecTimeStream *self, PyObject *arg) { SUB };
static PyObject * TableRecTimeStream_inplace_sub(TableRecTimeStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TableRecTimeStream_div(TableRecTimeStream *self, PyObject *arg) { DIV };
static PyObject * TableRecTimeStream_inplace_div(TableRecTimeStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TableRecTimeStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TableRecTimeStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TableRecTimeStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TableRecTimeStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TableRecTimeStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TableRecTimeStream_methods[] = {
    {"getServer", (PyCFunction)TableRecTimeStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TableRecTimeStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TableRecTimeStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TableRecTimeStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TableRecTimeStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TableRecTimeStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TableRecTimeStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TableRecTimeStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TableRecTimeStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TableRecTimeStream_as_number = {
    (binaryfunc)TableRecTimeStream_add,                         /*nb_add*/
    (binaryfunc)TableRecTimeStream_sub,                         /*nb_subtract*/
    (binaryfunc)TableRecTimeStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TableRecTimeStream_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)TableRecTimeStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TableRecTimeStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TableRecTimeStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TableRecTimeStream_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TableRecTimeStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TableRecTimeStream_base",         /*tp_name*/
    sizeof(TableRecTimeStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TableRecTimeStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TableRecTimeStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TableRecTimeStream objects. Returns the current recording time, in samples, of a TableRec object.",           /* tp_doc */
    (traverseproc)TableRecTimeStream_traverse,   /* tp_traverse */
    (inquiry)TableRecTimeStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TableRecTimeStream_methods,             /* tp_methods */
    TableRecTimeStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TableRecTimeStream_new,                 /* tp_new */
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
    MYFLT *buffer;
    int last_size;
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
TableMorph_alloc_memories(TableMorph *self)
{
    int i, size;
    size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    self->last_size = size;
    self->buffer = (MYFLT *)realloc(self->buffer, size * sizeof(MYFLT));
    for (i=0; i<size; i++) {
        self->buffer[i] = 0.0;
    }
}

static void
TableMorph_compute_next_data_frame(TableMorph *self)
{
    int i, x, y;
    MYFLT input, interp, interp1, interp2;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    int len = PyList_Size(self->sources);

    if (size != self->last_size)
        TableMorph_alloc_memories(self);

    input = TableMorph_clip(in[0]);

    interp = input * (len - 1);
    x = (int)(interp);
    y = x + 1;

    MYFLT *tab1 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, x), "getTableStream", ""));
    MYFLT *tab2 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, y), "getTableStream", ""));

    interp = MYFMOD(interp, 1.0);
    interp1 = 1. - interp;
    interp2 = interp;

    //MYFLT buffer[size];
    for (i=0; i<size; i++) {
        self->buffer[i] = tab1[i] * interp1 + tab2[i] * interp2;
    }

    NewTable_recordChunk((NewTable *)self->table, self->buffer, size);
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
    pyo_DEALLOC
    free(self->buffer);
    TableMorph_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableMorph_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *sourcestmp;
    TableMorph *self;
    self = (TableMorph *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableMorph_compute_next_data_frame);

    static char *kwlist[] = {"input", "table", "sources", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &tabletmp, &sourcestmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableMorph must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    Py_INCREF(tabletmp);
    self->table = (PyObject *)tabletmp;

    Py_XDECREF(self->sources);
    Py_INCREF(sourcestmp);
    self->sources = (PyObject *)sourcestmp;

    TableMorph_alloc_memories(self);

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
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
0,      /* tp_init */
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
    PyObject *trigger;
    Stream *trigger_stream;
    NewTable *table;
    int pointer;
    int active;
    MYFLT fadetime;
    MYFLT fadeInSample;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    MYFLT *time_buffer_streams;
} TrigTableRec;

static void
TrigTableRec_compute_next_data_frame(TrigTableRec *self)
{
    int i, j, num, upBound;
    MYFLT val;
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trig = Stream_getData((Stream *)self->trigger_stream);

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

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
                self->time_buffer_streams[i] = self->pointer++;
            }
            NewTable_recordChunk((NewTable *)self->table, buffer, num);

            if (num < self->bufsize) {
                for (i=num; i<self->bufsize; i++) {
                    self->time_buffer_streams[i] = self->pointer;
                }
            }
        }
    }
    else {
        for (j=0; j<self->bufsize; j++) {
            self->time_buffer_streams[j] = self->pointer;
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
                    self->time_buffer_streams[i+j] = self->pointer++;
                }
                NewTable_recordChunk((NewTable *)self->table, buffer, num);

                if (num < (self->bufsize-j)) {
                    for (i=num; i<(self->bufsize-j); i++) {
                        self->time_buffer_streams[i+j] = self->pointer;
                    }
                }
                break;
            }
        }
    }
}

static MYFLT *
TrigTableRec_getTimeBuffer(TrigTableRec *self) {
    return self->time_buffer_streams;
}

static int
TrigTableRec_traverse(TrigTableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->trigger);
    Py_VISIT(self->trigger_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
TrigTableRec_clear(TrigTableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->trigger);
    Py_CLEAR(self->trigger_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
TrigTableRec_dealloc(TrigTableRec* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    TrigTableRec_clear(self);
    free(self->time_buffer_streams);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigTableRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *trigtmp, *trig_streamtmp, *tabletmp;
    TrigTableRec *self;
    self = (TrigTableRec *)type->tp_alloc(type, 0);

    self->pointer = 0;
    self->active = 0;
    self->fadetime = 0.;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TrigTableRec_compute_next_data_frame);

    static char *kwlist[] = {"input", "trig", "table", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OOO_F, kwlist, &inputtmp, &trigtmp, &tabletmp, &self->fadetime))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->trigger);
    Py_INCREF(trigtmp);
    self->trigger = trigtmp;
    trig_streamtmp = PyObject_CallMethod((PyObject *)self->trigger, "_getStream", NULL);
    Py_INCREF(trig_streamtmp);
    Py_XDECREF(self->trigger_stream);
    self->trigger_stream = (Stream *)trig_streamtmp;

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TrigTableRec must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    Py_INCREF(tabletmp);
    self->table = (NewTable *)tabletmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->time_buffer_streams = (MYFLT *)realloc(self->time_buffer_streams, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = self->time_buffer_streams[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    if ((self->fadetime * self->sr) >= (size * 0.5))
        self->fadetime = size * 0.499 / self->sr;
    if (self->fadetime == 0.0)
        self->fadeInSample = 0.0;
    else
        self->fadeInSample = MYROUND(self->fadetime * self->sr + 0.5);

    return (PyObject *)self;
}

static PyObject * TrigTableRec_getServer(TrigTableRec* self) { GET_SERVER };
static PyObject * TrigTableRec_getStream(TrigTableRec* self) { GET_STREAM };
static PyObject * TrigTableRec_getTriggerStream(TrigTableRec* self) { GET_TRIGGER_STREAM };

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

static PyMemberDef TrigTableRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigTableRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRec, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(TrigTableRec, trig_stream), 0, "Trigger Stream object."},
    {"input", T_OBJECT_EX, offsetof(TrigTableRec, input), 0, "Input sound object."},
    {"trigger", T_OBJECT_EX, offsetof(TrigTableRec, trigger), 0, "Trigger object."},
    {"table", T_OBJECT_EX, offsetof(TrigTableRec, table), 0, "Table to record in."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRec_methods[] = {
    {"getServer", (PyCFunction)TrigTableRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigTableRec_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)TrigTableRec_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
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
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigTableRec_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    TrigTableRec *mainPlayer;
    int modebuffer[2];
} TrigTableRecTimeStream;

static void TrigTableRecTimeStream_postprocessing_ii(TrigTableRecTimeStream *self) { POST_PROCESSING_II };
static void TrigTableRecTimeStream_postprocessing_ai(TrigTableRecTimeStream *self) { POST_PROCESSING_AI };
static void TrigTableRecTimeStream_postprocessing_ia(TrigTableRecTimeStream *self) { POST_PROCESSING_IA };
static void TrigTableRecTimeStream_postprocessing_aa(TrigTableRecTimeStream *self) { POST_PROCESSING_AA };
static void TrigTableRecTimeStream_postprocessing_ireva(TrigTableRecTimeStream *self) { POST_PROCESSING_IREVA };
static void TrigTableRecTimeStream_postprocessing_areva(TrigTableRecTimeStream *self) { POST_PROCESSING_AREVA };
static void TrigTableRecTimeStream_postprocessing_revai(TrigTableRecTimeStream *self) { POST_PROCESSING_REVAI };
static void TrigTableRecTimeStream_postprocessing_revaa(TrigTableRecTimeStream *self) { POST_PROCESSING_REVAA };
static void TrigTableRecTimeStream_postprocessing_revareva(TrigTableRecTimeStream *self) { POST_PROCESSING_REVAREVA };

static void
TrigTableRecTimeStream_setProcMode(TrigTableRecTimeStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = TrigTableRecTimeStream_postprocessing_revareva;
            break;
    }
}

static void
TrigTableRecTimeStream_compute_next_data_frame(TrigTableRecTimeStream *self)
{
    int i;
    MYFLT *tmp;
    tmp = TrigTableRec_getTimeBuffer((TrigTableRec *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }
    (*self->muladd_func_ptr)(self);
}

static int
TrigTableRecTimeStream_traverse(TrigTableRecTimeStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
TrigTableRecTimeStream_clear(TrigTableRecTimeStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
TrigTableRecTimeStream_dealloc(TrigTableRecTimeStream* self)
{
    pyo_DEALLOC
    TrigTableRecTimeStream_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TrigTableRecTimeStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    TrigTableRecTimeStream *self;
    self = (TrigTableRecTimeStream *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigTableRecTimeStream_compute_next_data_frame);
    self->mode_func_ptr = TrigTableRecTimeStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (TrigTableRec *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * TrigTableRecTimeStream_getServer(TrigTableRecTimeStream* self) { GET_SERVER };
static PyObject * TrigTableRecTimeStream_getStream(TrigTableRecTimeStream* self) { GET_STREAM };
static PyObject * TrigTableRecTimeStream_setMul(TrigTableRecTimeStream *self, PyObject *arg) { SET_MUL };
static PyObject * TrigTableRecTimeStream_setAdd(TrigTableRecTimeStream *self, PyObject *arg) { SET_ADD };
static PyObject * TrigTableRecTimeStream_setSub(TrigTableRecTimeStream *self, PyObject *arg) { SET_SUB };
static PyObject * TrigTableRecTimeStream_setDiv(TrigTableRecTimeStream *self, PyObject *arg) { SET_DIV };

static PyObject * TrigTableRecTimeStream_play(TrigTableRecTimeStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TrigTableRecTimeStream_out(TrigTableRecTimeStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigTableRecTimeStream_stop(TrigTableRecTimeStream *self) { STOP };

static PyObject * TrigTableRecTimeStream_multiply(TrigTableRecTimeStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigTableRecTimeStream_inplace_multiply(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigTableRecTimeStream_add(TrigTableRecTimeStream *self, PyObject *arg) { ADD };
static PyObject * TrigTableRecTimeStream_inplace_add(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigTableRecTimeStream_sub(TrigTableRecTimeStream *self, PyObject *arg) { SUB };
static PyObject * TrigTableRecTimeStream_inplace_sub(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigTableRecTimeStream_div(TrigTableRecTimeStream *self, PyObject *arg) { DIV };
static PyObject * TrigTableRecTimeStream_inplace_div(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigTableRecTimeStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRecTimeStream_methods[] = {
    {"getServer", (PyCFunction)TrigTableRecTimeStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)TrigTableRecTimeStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)TrigTableRecTimeStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)TrigTableRecTimeStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)TrigTableRecTimeStream_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)TrigTableRecTimeStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)TrigTableRecTimeStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)TrigTableRecTimeStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)TrigTableRecTimeStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigTableRecTimeStream_as_number = {
    (binaryfunc)TrigTableRecTimeStream_add,                         /*nb_add*/
    (binaryfunc)TrigTableRecTimeStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigTableRecTimeStream_multiply,                    /*nb_multiply*/
    (binaryfunc)TrigTableRecTimeStream_div,                                              /*nb_divide*/
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
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)TrigTableRecTimeStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigTableRecTimeStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigTableRecTimeStream_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)TrigTableRecTimeStream_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TrigTableRecTimeStreamType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.TrigTableRecTimeStream_base",         /*tp_name*/
    sizeof(TrigTableRecTimeStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigTableRecTimeStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &TrigTableRecTimeStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "TrigTableRecTimeStream objects. Returns the current recording time, in samples, of a TableRec object.",           /* tp_doc */
    (traverseproc)TrigTableRecTimeStream_traverse,   /* tp_traverse */
    (inquiry)TrigTableRecTimeStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    TrigTableRecTimeStream_methods,             /* tp_methods */
    TrigTableRecTimeStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    TrigTableRecTimeStream_new,                 /* tp_new */
};

/******************************/
/* TablePut object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    DataTable *table;
    int pointer;
    int active;
    MYFLT last_value;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} TablePut;

static void
TablePut_compute_next_data_frame(TablePut *self)
{
    int i;
    int size = PyInt_AsLong(DataTable_getSize((DataTable *)self->table));
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    if (self->active == 1) {
        for (i=0; i<self->bufsize; i++) {
            if (in[i] != self->last_value) {
                self->last_value = in[i];
                DataTable_record((DataTable *)self->table, self->pointer++, self->last_value);
                if (self->pointer >= size) {
                    self->active = 0;
                    self->trigsBuffer[i] = 1.0;
                    break;
                }
            }
        }
    }
}

static int
TablePut_traverse(TablePut *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
TablePut_clear(TablePut *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
TablePut_dealloc(TablePut* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    TablePut_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TablePut_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp;
    TablePut *self;
    self = (TablePut *)type->tp_alloc(type, 0);

    self->pointer = 0;
    self->active = 1;
    self->last_value = 0.0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TablePut_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 0);

    static char *kwlist[] = {"input", "table", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &tabletmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TablePut must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    Py_INCREF(tabletmp);
    self->table = (DataTable *)tabletmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    return (PyObject *)self;
}

static PyObject * TablePut_getServer(TablePut* self) { GET_SERVER };
static PyObject * TablePut_getStream(TablePut* self) { GET_STREAM };
static PyObject * TablePut_getTriggerStream(TablePut* self) { GET_TRIGGER_STREAM };

static PyObject * TablePut_play(TablePut *self, PyObject *args, PyObject *kwds)
{
    self->pointer = 0;
    self->active = 1;
    PLAY
};

static PyObject * TablePut_stop(TablePut *self) { STOP };

static PyObject *
TablePut_setTable(TablePut *self, PyObject *arg)
{
	PyObject *tmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->table);
    self->table = (DataTable *)tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef TablePut_members[] = {
{"server", T_OBJECT_EX, offsetof(TablePut, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TablePut, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(TablePut, trig_stream), 0, "Trigger Stream object."},
{"input", T_OBJECT_EX, offsetof(TablePut, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TablePut, table), 0, "Table to record in."},
{NULL}  /* Sentinel */
};

static PyMethodDef TablePut_methods[] = {
{"getServer", (PyCFunction)TablePut_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TablePut_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)TablePut_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
{"setTable", (PyCFunction)TablePut_setTable, METH_O, "Sets a new data table."},
{"play", (PyCFunction)TablePut_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TablePut_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TablePutType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TablePut_base",         /*tp_name*/
sizeof(TablePut),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TablePut_dealloc, /*tp_dealloc*/
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
"TablePut objects. Record new value in input in a data table object.",           /* tp_doc */
(traverseproc)TablePut_traverse,   /* tp_traverse */
(inquiry)TablePut_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TablePut_methods,             /* tp_methods */
TablePut_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
TablePut_new,                 /* tp_new */
};

/******************************/
/* TableWrite object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *pos;
    Stream *pos_stream;
    NewTable *table;
} TableWrite;

static void
TableWrite_compute_next_data_frame(TableWrite *self)
{
    int i, ipos;
    PyObject *table;

    table = PyObject_CallMethod((PyObject *)self->table, "getTableStream", "");
    MYFLT *tablelist = TableStream_getData((TableStream *)table);
    int size = TableStream_getSize((TableStream *)table);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);

    for (i=0; i<self->bufsize; i++) {
        ipos = (int)(pos[i] * size);
        if (ipos < 0)
            ipos = 0;
        else if (ipos >= size)
            ipos = size - 1;
        tablelist[ipos] = in[i];
    }
}

static int
TableWrite_traverse(TableWrite *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->pos);
    Py_VISIT(self->pos_stream);
    return 0;
}

static int
TableWrite_clear(TableWrite *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->pos);
    Py_CLEAR(self->pos_stream);
    return 0;
}

static void
TableWrite_dealloc(TableWrite* self)
{
    pyo_DEALLOC
    TableWrite_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableWrite_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *postmp, *tabletmp;
    TableWrite *self;
    self = (TableWrite *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableWrite_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 1);

    static char *kwlist[] = {"input", "pos", "table", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &postmp, &tabletmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (postmp) {
        PyObject_CallMethod((PyObject *)self, "setPos", "O", postmp);
    }

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableWrite must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    Py_INCREF(tabletmp);
    self->table = (NewTable *)tabletmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
}

static PyObject * TableWrite_getServer(TableWrite* self) { GET_SERVER };
static PyObject * TableWrite_getStream(TableWrite* self) { GET_STREAM };

static PyObject * TableWrite_play(TableWrite *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableWrite_stop(TableWrite *self) { STOP };

static PyObject *
TableWrite_setPos(TableWrite *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	tmp = arg;
	if (PyObject_HasAttrString((PyObject *)tmp, "server") == 0) {
        PyErr_SetString(PyExc_TypeError, "\"pos\" argument of TableWrite must be a PyoObject.\n");
        Py_RETURN_NONE;
	}

	Py_INCREF(tmp);
	Py_XDECREF(self->pos);

    self->pos = tmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->pos, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->pos_stream);
    self->pos_stream = (Stream *)streamtmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
TableWrite_setTable(TableWrite *self, PyObject *arg)
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

static PyMemberDef TableWrite_members[] = {
{"server", T_OBJECT_EX, offsetof(TableWrite, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableWrite, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TableWrite, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TableWrite, table), 0, "Table to record in."},
{"pos", T_OBJECT_EX, offsetof(TableWrite, pos), 0, "Position in the Table to record in."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableWrite_methods[] = {
{"getServer", (PyCFunction)TableWrite_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableWrite_getStream, METH_NOARGS, "Returns stream object."},
{"setTable", (PyCFunction)TableWrite_setTable, METH_O, "Sets a new table."},
{"setPos", (PyCFunction)TableWrite_setPos, METH_O, "Sets position in the sound table."},
{"play", (PyCFunction)TableWrite_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableWrite_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableWriteType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableWrite_base",         /*tp_name*/
sizeof(TableWrite),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableWrite_dealloc, /*tp_dealloc*/
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
"TableWrite objects. Record audio input in a table object.",           /* tp_doc */
(traverseproc)TableWrite_traverse,   /* tp_traverse */
(inquiry)TableWrite_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableWrite_methods,             /* tp_methods */
TableWrite_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
TableWrite_new,                 /* tp_new */
};
