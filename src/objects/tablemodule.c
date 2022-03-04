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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <object.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "wind.h"
#include "fft.h"

#if !defined(_WIN32) && !defined(_WIN64)
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "tablemodule.h"

/*************************/
/* TableStream structure */
/*************************/
static void
TableStream_dealloc(TableStream* self)
{
    Py_TYPE(self)->tp_free((PyObject*)self);
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

T_SIZE_T
TableStream_getSize(TableStream *self)
{
    return self->size;
}

void
TableStream_setSize(TableStream *self, T_SIZE_T size)
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

#if PY_MAJOR_VERSION < 3
static Py_ssize_t TableStream_getReadBuffer(TableStream *self, Py_ssize_t index, const void **ptr) { TABLESTREAM_READ_WRITE_BUFFER };
static Py_ssize_t TableStream_getWriteBuffer(TableStream *self, Py_ssize_t index, const void **ptr) { TABLESTREAM_READ_WRITE_BUFFER };
static Py_ssize_t TableStream_getSegCount(TableStream *self, Py_ssize_t *lenp) { TABLESTREAM_SEG_COUNT };
#endif
static int TableStream_getBuffer(PyObject *obj, Py_buffer *view, int flags)
{
    TableStream *self = (TableStream *)obj;
    TABLESTREAM_GET_BUFFER
};

void TableStream_recordChunk(TableStream *self, MYFLT *data, T_SIZE_T datasize)
{
    int i;
    for (i = 0; i < datasize; i++)
    {
        self->data[self->pointer] = data[i] + self->data[self->pointer] * self->feedback;
        self->pointer++;

        if (self->pointer == self->size)
        {
            self->pointer = 0;
            self->data[self->size] = self->data[0];
        }
    } 
}

void TableStream_resetRecordingPointer(TableStream *self)
{
    self->pointer = 0;
}

void TableStream_setFeedback(TableStream *self, MYFLT feedback)
{
    self->feedback = feedback;
}

MYFLT TableStream_getFeedback(TableStream *self)
{
    return self->feedback;
}

void TableStream_record(TableStream *self, int pos, MYFLT value)
{
    self->data[pos] = value;
}

static PyBufferProcs TableStream_as_buffer =
{
#if PY_MAJOR_VERSION < 3
    (readbufferproc)TableStream_getReadBuffer,
    (writebufferproc)TableStream_getWriteBuffer,
    (segcountproc)TableStream_getSegCount,
    (charbufferproc)NULL,
#endif
    (getbufferproc)TableStream_getBuffer,
    (releasebufferproc)NULL,
};

PyTypeObject TableStreamType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TableStream", /*tp_name*/
    sizeof(TableStream), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)TableStream_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    0, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_as_async (tp_compare in Python 2)*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    0, /*tp_as_mapping*/
    0, /*tp_hash */
    0, /*tp_call*/
    0, /*tp_str*/
    0, /*tp_getattro*/
    0, /*tp_setattro*/
    &TableStream_as_buffer,                         /*tp_as_buffer*/
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
typedef struct
{
    pyo_table_HEAD
    PyObject *amplist;
} HarmTable;

static void
HarmTable_generate(HarmTable *self)
{
    int j, ampsize;
    T_SIZE_T i;
    MYFLT factor, amplitude, val;

    ampsize = PyList_Size(self->amplist);
    MYFLT array[ampsize];

    for (j = 0; j < ampsize; j++)
    {
        array[j] =  PyFloat_AsDouble(PyList_GET_ITEM(self->amplist, j));
    }

    factor = 1. / (self->size * 0.5) * PI;

    for (i = 0; i < self->size; i++)
    {
        val = 0;

        for (j = 0; j < ampsize; j++)
        {
            amplitude = array[j];

            if (amplitude != 0.0)
            {
                val += MYSIN((j + 1) * i * factor) * amplitude;
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
    PyMem_RawFree(self->data);
    HarmTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
HarmTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *amplist = NULL;
    HarmTable *self;
    self = (HarmTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->amplist = PyList_New(0);
    PyObject *init = PyFloat_FromDouble(1.);
    PyList_Append(self->amplist, init);
    Py_DECREF(init);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &amplist, &self->size))
        Py_RETURN_NONE;

    if (amplist)
    {
        Py_INCREF(amplist);
        Py_DECREF(self->amplist);
        self->amplist = amplist;
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    HarmTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * HarmTable_getServer(HarmTable* self) { GET_SERVER };
static PyObject * HarmTable_getTableStream(HarmTable* self) { GET_TABLE_STREAM };
static PyObject * HarmTable_setData(HarmTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HarmTable_reset(HarmTable *self) { TABLE_RESET };
static PyObject * HarmTable_normalize(HarmTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * HarmTable_copyData(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * HarmTable_rotate(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * HarmTable_setTable(HarmTable *self, PyObject *arg) { SET_TABLE };
static PyObject * HarmTable_getTable(HarmTable *self) { GET_TABLE };
static PyObject * HarmTable_getRate(HarmTable *self) { TABLE_GET_RATE };
static PyObject * HarmTable_put(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HarmTable_get(HarmTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * HarmTable_add(HarmTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * HarmTable_sub(HarmTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * HarmTable_mul(HarmTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * HarmTable_div(HarmTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
HarmTable_setSize(HarmTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    HarmTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
HarmTable_getSize(HarmTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyObject *
HarmTable_replace(HarmTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->amplist);
    self->amplist = value;

    HarmTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef HarmTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(HarmTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(HarmTable, tablestream), 0, NULL},
    {"amplist", T_OBJECT_EX, offsetof(HarmTable, amplist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef HarmTable_methods[] =
{
    {"getServer", (PyCFunction)HarmTable_getServer, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)HarmTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)HarmTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)HarmTable_getTableStream, METH_NOARGS, NULL},
    {"normalize", (PyCFunction)HarmTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)HarmTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)HarmTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)HarmTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)HarmTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)HarmTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)HarmTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)HarmTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)HarmTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)HarmTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)HarmTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"copy", (PyCFunction)HarmTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)HarmTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)HarmTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setData", (PyCFunction)HarmTable_setData, METH_O, NULL},
    {"setSize", (PyCFunction)HarmTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)HarmTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)HarmTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)HarmTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)HarmTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"replace", (PyCFunction)HarmTable_replace, METH_O, NULL},
    {"add", (PyCFunction)HarmTable_add, METH_O, NULL},
    {"sub", (PyCFunction)HarmTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)HarmTable_mul, METH_O, NULL},
    {"div", (PyCFunction)HarmTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject HarmTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.HarmTable_base",         /*tp_name*/
    sizeof(HarmTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)HarmTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)HarmTable_traverse,   /* tp_traverse */
    (inquiry)HarmTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *amplist;
} ChebyTable;

static void
ChebyTable_generate(ChebyTable *self)
{
    int j, ampsize, halfsize;
    T_SIZE_T i;
    MYFLT amplitude, val, ihalfsize, index, x;

    ampsize = PyList_Size(self->amplist);

    if (ampsize > 12)
        ampsize = 12;

    MYFLT array[ampsize];

    for (j = 0; j < ampsize; j++)
    {
        array[j] =  PyFloat_AsDouble(PyList_GET_ITEM(self->amplist, j));
    }

    halfsize = self->size / 2;
    ihalfsize = 1.0 / halfsize;

    x = 0.0;

    for (i = 0; i < self->size; i++)
    {
        val = 0;
        index = (i - halfsize) * ihalfsize;

        for (j = 0; j < ampsize; j++)
        {
            amplitude = array[j];

            switch (j)
            {
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

    val = self->data[self->size - 1];
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
    PyMem_RawFree(self->data);
    ChebyTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ChebyTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *amplist = NULL;
    ChebyTable *self;
    self = (ChebyTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->amplist = PyList_New(0);
    PyObject *init = PyFloat_FromDouble(1.);
    PyList_Append(self->amplist, init);
    Py_DECREF(init);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &amplist, &self->size))
        Py_RETURN_NONE;

    if (amplist)
    {
        Py_INCREF(amplist);
        Py_DECREF(self->amplist);
        self->amplist = amplist;
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    ChebyTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * ChebyTable_getServer(ChebyTable* self) { GET_SERVER };
static PyObject * ChebyTable_getTableStream(ChebyTable* self) { GET_TABLE_STREAM };
static PyObject * ChebyTable_setData(ChebyTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ChebyTable_normalize(ChebyTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * ChebyTable_copyData(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * ChebyTable_rotate(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * ChebyTable_setTable(ChebyTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ChebyTable_getTable(ChebyTable *self) { GET_TABLE };
static PyObject * ChebyTable_getRate(ChebyTable *self) { TABLE_GET_RATE };
static PyObject * ChebyTable_put(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ChebyTable_get(ChebyTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ChebyTable_add(ChebyTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ChebyTable_sub(ChebyTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ChebyTable_mul(ChebyTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * ChebyTable_div(ChebyTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
ChebyTable_setSize(ChebyTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    ChebyTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ChebyTable_getSize(ChebyTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyObject *
ChebyTable_replace(ChebyTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->amplist);
    self->amplist = value;

    ChebyTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ChebyTable_getNormTable(ChebyTable *self, PyObject *value)
{
    T_SIZE_T i, halfsize = self->size / 2;
    MYFLT maxval = 0.0;
    MYFLT val = 0.0, val2 = 0.0;
    MYFLT last = 0.0;
    long sym = PyLong_AsLong(value);
    MYFLT samps[halfsize];  // FIXME: Very large table would cause stack overflow.
    PyObject *samples = PyList_New(halfsize);

    if (sym == 0)
    {
        for (i = 0; i < self->size; i++)
        {
            if (self->data[i] > maxval)
                maxval = self->data[i];
        }

        if (maxval > 1.0)
        {
            for (i = 0; i < self->size; i++)
            {
                self->data[i] /= maxval;
            }
        }

        maxval = -1;

        for (i = 0; i < halfsize; i++)
        {
            val = MYFABS(self->data[halfsize + i]);

            if (val > maxval)
                maxval = val;

            if (maxval > 0.0)
                samps[i] = 1. - maxval;
            else
                samps[i] = -1.;
        }
    }
    else
    {
        maxval = -1;

        for (i = 0; i < halfsize; i++)
        {
            val = MYFABS(self->data[halfsize - i]);
            val2 = MYFABS(self->data[halfsize + i]);

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

    for (i = 0; i < halfsize; i++)
    {
        val = samps[i];

        if (val > maxval)
            maxval = val;
    }

    for (i = 0; i < halfsize; i++)
    {
        val = samps[i];

        if (val == -1.0)
            samps[i] = maxval;
    }

    last = samps[0];

    for (i = 1; i < halfsize; i++)
    {
        last = samps[i] = samps[i] + (last - samps[i]) * 0.95;
    }

    for (i = 0; i < halfsize; i++)
    {
        PyList_SET_ITEM(samples, i, PyFloat_FromDouble(samps[i]));
    }

    return samples;
}

static PyMemberDef ChebyTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ChebyTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(ChebyTable, tablestream), 0, NULL},
    {"amplist", T_OBJECT_EX, offsetof(ChebyTable, amplist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef ChebyTable_methods[] =
{
    {"getServer", (PyCFunction)ChebyTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)ChebyTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)ChebyTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)ChebyTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)ChebyTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)ChebyTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)ChebyTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)ChebyTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)ChebyTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)ChebyTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)ChebyTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)ChebyTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)ChebyTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)ChebyTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)ChebyTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)ChebyTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)ChebyTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)ChebyTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)ChebyTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)ChebyTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)ChebyTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)ChebyTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)ChebyTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)ChebyTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"replace", (PyCFunction)ChebyTable_replace, METH_O, NULL},
    {"getNormTable", (PyCFunction)ChebyTable_getNormTable, METH_O, NULL},
    {"add", (PyCFunction)ChebyTable_add, METH_O, NULL},
    {"sub", (PyCFunction)ChebyTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)ChebyTable_mul, METH_O, NULL},
    {"div", (PyCFunction)ChebyTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject ChebyTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.ChebyTable_base",         /*tp_name*/
    sizeof(ChebyTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ChebyTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)ChebyTable_traverse,   /* tp_traverse */
    (inquiry)ChebyTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
} HannTable;

static void
HannTable_generate(HannTable *self)
{
    T_SIZE_T i, halfSize;
    MYFLT val;

    halfSize = self->size / 2 - 1;

    for (i = 0; i < self->size; i++)
    {
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
    PyMem_RawFree(self->data);
    HannTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
HannTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HannTable *self;
    self = (HannTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    HannTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * HannTable_getServer(HannTable* self) { GET_SERVER };
static PyObject * HannTable_getTableStream(HannTable* self) { GET_TABLE_STREAM };
static PyObject * HannTable_setData(HannTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * HannTable_normalize(HannTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * HannTable_copyData(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * HannTable_rotate(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * HannTable_setTable(HannTable *self, PyObject *arg) { SET_TABLE };
static PyObject * HannTable_getTable(HannTable *self) { GET_TABLE };
static PyObject * HannTable_getRate(HannTable *self) { TABLE_GET_RATE };
static PyObject * HannTable_put(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * HannTable_get(HannTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * HannTable_add(HannTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * HannTable_sub(HannTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * HannTable_mul(HannTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * HannTable_div(HannTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
HannTable_setSize(HannTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    HannTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
HannTable_getSize(HannTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef HannTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(HannTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(HannTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef HannTable_methods[] =
{
    {"getServer", (PyCFunction)HannTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)HannTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)HannTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)HannTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)HannTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)HannTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)HannTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)HannTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)HannTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)HannTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)HannTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)HannTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)HannTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)HannTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)HannTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)HannTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)HannTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)HannTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)HannTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)HannTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)HannTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)HannTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)HannTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)HannTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)HannTable_add, METH_O, NULL},
    {"sub", (PyCFunction)HannTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)HannTable_mul, METH_O, NULL},
    {"div", (PyCFunction)HannTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject HannTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.HannTable_base",         /*tp_name*/
    sizeof(HannTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)HannTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)HannTable_traverse,   /* tp_traverse */
    (inquiry)HannTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    MYFLT freq;
    int windowed;
} SincTable;

static void
SincTable_generate(SincTable *self)
{
    T_SIZE_T i, half, halfMinusOne;
    MYFLT scl, val;

    half = self->size / 2;

    if (self->windowed)
    {
        halfMinusOne = half - 1;

        for (i = 0; i < self->size; i++)
        {
            scl = (MYFLT)(i - half) / half * self->freq;

            if (scl == 0.0)
                val = 1.0;
            else
                val = (MYSIN(scl) / scl);

            val *= 0.5 + (MYCOS(TWOPI * (i - halfMinusOne) / self->size) * 0.5);
            self->data[i] = val;
        }
    }
    else
    {
        for (i = 0; i < self->size; i++)
        {
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
    PyMem_RawFree(self->data);
    SincTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SincTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SincTable *self;
    self = (SincTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 8192;
    self->freq = TWOPI;
    self->windowed = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"freq", "windowed", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FIN, kwlist, &self->freq, &self->windowed, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    SincTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * SincTable_getServer(SincTable* self) { GET_SERVER };
static PyObject * SincTable_getTableStream(SincTable* self) { GET_TABLE_STREAM };
static PyObject * SincTable_setData(SincTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * SincTable_normalize(SincTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * SincTable_copyData(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * SincTable_rotate(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * SincTable_setTable(SincTable *self, PyObject *arg) { SET_TABLE };
static PyObject * SincTable_getTable(SincTable *self) { GET_TABLE };
static PyObject * SincTable_getRate(SincTable *self) { TABLE_GET_RATE };
static PyObject * SincTable_put(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * SincTable_get(SincTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * SincTable_add(SincTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * SincTable_sub(SincTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * SincTable_mul(SincTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * SincTable_div(SincTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
SincTable_setFreq(SincTable *self, PyObject *value)
{

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The freq attribute value must be a number.");
        return PyLong_FromLong(-1);
    }

    self->freq = PyFloat_AsDouble(value);

    SincTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
SincTable_setWindowed(SincTable *self, PyObject *value)
{

    if (! PyLong_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The windowed attribute value must be a boolean.");
        return PyLong_FromLong(-1);
    }

    self->windowed = PyLong_AsLong(value);

    SincTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
SincTable_setSize(SincTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    SincTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
SincTable_getSize(SincTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef SincTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(SincTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(SincTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef SincTable_methods[] =
{
    {"getServer", (PyCFunction)SincTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)SincTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)SincTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)SincTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)SincTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)SincTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)SincTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)SincTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)SincTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)SincTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)SincTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)SincTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)SincTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)SincTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)SincTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)SincTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)SincTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)SincTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)SincTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)SincTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)SincTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)SincTable_getRate, METH_NOARGS, NULL},
    {"setFreq", (PyCFunction)SincTable_setFreq, METH_O, NULL},
    {"setWindowed", (PyCFunction)SincTable_setWindowed, METH_O, NULL},
    {"put", (PyCFunction)SincTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)SincTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)SincTable_add, METH_O, NULL},
    {"sub", (PyCFunction)SincTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)SincTable_mul, METH_O, NULL},
    {"div", (PyCFunction)SincTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject SincTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SincTable_base",         /*tp_name*/
    sizeof(SincTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SincTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)SincTable_traverse,   /* tp_traverse */
    (inquiry)SincTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    int type;
} WinTable;

static void
WinTable_generate(WinTable *self)
{
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
    PyMem_RawFree(self->data);
    WinTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
WinTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    WinTable *self;
    self = (WinTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 8192;
    self->type = 2;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"type", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|in", kwlist, &self->type, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    WinTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * WinTable_getServer(WinTable* self) { GET_SERVER };
static PyObject * WinTable_getTableStream(WinTable* self) { GET_TABLE_STREAM };
static PyObject * WinTable_setData(WinTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * WinTable_normalize(WinTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * WinTable_copyData(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * WinTable_rotate(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * WinTable_setTable(WinTable *self, PyObject *arg) { SET_TABLE };
static PyObject * WinTable_getTable(WinTable *self) { GET_TABLE };
static PyObject * WinTable_getRate(WinTable *self) { TABLE_GET_RATE };
static PyObject * WinTable_put(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * WinTable_get(WinTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * WinTable_add(WinTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * WinTable_sub(WinTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * WinTable_mul(WinTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * WinTable_div(WinTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
WinTable_setSize(WinTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    WinTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
WinTable_getSize(WinTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyObject *
WinTable_setType(WinTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the type attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyLong_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The type attribute value must be an integer.");
        return PyLong_FromLong(-1);
    }

    self->type = PyLong_AsLong(value);

    WinTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef WinTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(WinTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(WinTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef WinTable_methods[] =
{
    {"getServer", (PyCFunction)WinTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)WinTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)WinTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)WinTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)WinTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)WinTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)WinTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)WinTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)WinTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)WinTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)WinTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)WinTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)WinTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)WinTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)WinTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)WinTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)WinTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)WinTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)WinTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)WinTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)WinTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)WinTable_getRate, METH_NOARGS, NULL},
    {"setType", (PyCFunction)WinTable_setType, METH_O, NULL},
    {"put", (PyCFunction)WinTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)WinTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)WinTable_add, METH_O, NULL},
    {"sub", (PyCFunction)WinTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)WinTable_mul, METH_O, NULL},
    {"div", (PyCFunction)WinTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject WinTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.WinTable_base",         /*tp_name*/
    sizeof(WinTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)WinTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)WinTable_traverse,   /* tp_traverse */
    (inquiry)WinTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
} ParaTable;

static void
ParaTable_generate(ParaTable *self)
{
    T_SIZE_T i, sizeMinusOne;
    MYFLT rdur, rdur2, level, slope, curve;

    sizeMinusOne = self->size - 1;
    rdur = 1.0 / sizeMinusOne;
    rdur2 = rdur * rdur;
    level = 0.0;
    slope = 4.0 * (rdur - rdur2);
    curve = -8.0 * rdur2;

    for (i = 0; i < sizeMinusOne; i++)
    {
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
    PyMem_RawFree(self->data);
    ParaTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ParaTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ParaTable *self;
    self = (ParaTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|n", kwlist, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    ParaTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * ParaTable_getServer(ParaTable* self) { GET_SERVER };
static PyObject * ParaTable_getTableStream(ParaTable* self) { GET_TABLE_STREAM };
static PyObject * ParaTable_setData(ParaTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ParaTable_normalize(ParaTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * ParaTable_copyData(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * ParaTable_rotate(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * ParaTable_setTable(ParaTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ParaTable_getTable(ParaTable *self) { GET_TABLE };
static PyObject * ParaTable_getRate(ParaTable *self) { TABLE_GET_RATE };
static PyObject * ParaTable_put(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ParaTable_get(ParaTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ParaTable_add(ParaTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ParaTable_sub(ParaTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ParaTable_mul(ParaTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * ParaTable_div(ParaTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
ParaTable_setSize(ParaTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    ParaTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ParaTable_getSize(ParaTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef ParaTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ParaTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(ParaTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef ParaTable_methods[] =
{
    {"getServer", (PyCFunction)ParaTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)ParaTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)ParaTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)ParaTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)ParaTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)ParaTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)ParaTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)ParaTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)ParaTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)ParaTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)ParaTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)ParaTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)ParaTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)ParaTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)ParaTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)ParaTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)ParaTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)ParaTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)ParaTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)ParaTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)ParaTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)ParaTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)ParaTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)ParaTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)ParaTable_add, METH_O, NULL},
    {"sub", (PyCFunction)ParaTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)ParaTable_mul, METH_O, NULL},
    {"div", (PyCFunction)ParaTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject ParaTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.ParaTable_base",         /*tp_name*/
    sizeof(ParaTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ParaTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)ParaTable_traverse,   /* tp_traverse */
    (inquiry)ParaTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
} LinTable;

static void
LinTable_generate(LinTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup, *tup2;
    T_SIZE_T x1, y1;
    MYFLT x2, y2, diff;

    y1 = 0;
    y2 = 0.0;

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("LinTable error: There should be at least two points in a LinTable.\n");
        return;
    }

    for (i = 0; i < (listsize - 1); i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        x1 = PyLong_AsLong(PyNumber_Long(p1));
        x2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        tup2 = PyList_GET_ITEM(self->pointslist, i + 1);
        PyObject *p2 = PyTuple_GET_ITEM(tup2, 0);
        y1 = PyLong_AsLong(PyNumber_Long(p2));
        y2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup2, 1));

        Py_DECREF(p1);
        Py_DECREF(p2);

        steps = y1 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("LinTable error: point position smaller than previous one.\n");
            return;
        }

        diff = (y2 - x2) / steps;

        for (j = 0; j < steps; j++)
        {
            self->data[x1 + j] = x2 + diff * j;
        }
    }

    if (y1 < (self->size - 1))
    {
        self->data[y1] = y2;

        for (i = y1; i < self->size; i++)
        {
            self->data[i + 1] = 0.0;
        }

        self->data[self->size] = 0.0;
    }
    else
    {
        self->data[self->size - 1] = y2;
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
    PyMem_RawFree(self->data);
    LinTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
LinTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    LinTable *self;
    self = (LinTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    LinTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * LinTable_getServer(LinTable* self) { GET_SERVER };
static PyObject * LinTable_getTableStream(LinTable* self) { GET_TABLE_STREAM };
static PyObject * LinTable_setData(LinTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * LinTable_normalize(LinTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * LinTable_copyData(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * LinTable_rotate(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * LinTable_setTable(LinTable *self, PyObject *arg) { SET_TABLE };
static PyObject * LinTable_getTable(LinTable *self) { GET_TABLE };
static PyObject * LinTable_getRate(LinTable *self) { TABLE_GET_RATE };
static PyObject * LinTable_put(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * LinTable_get(LinTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * LinTable_add(LinTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * LinTable_sub(LinTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * LinTable_mul(LinTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * LinTable_div(LinTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
LinTable_setSize(LinTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    LinTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
LinTable_getSize(LinTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    LinTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef LinTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(LinTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(LinTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(LinTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef LinTable_methods[] =
{
    {"getServer", (PyCFunction)LinTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)LinTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)LinTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)LinTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)LinTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)LinTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)LinTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)LinTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)LinTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)LinTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)LinTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)LinTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)LinTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)LinTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)LinTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)LinTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)LinTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)LinTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)LinTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)LinTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)LinTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)LinTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)LinTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)LinTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)LinTable_getPoints, METH_NOARGS, NULL},
    {"replace", (PyCFunction)LinTable_replace, METH_O, NULL},
    {"add", (PyCFunction)LinTable_add, METH_O, NULL},
    {"sub", (PyCFunction)LinTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)LinTable_mul, METH_O, NULL},
    {"div", (PyCFunction)LinTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject LinTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.LinTable_base",         /*tp_name*/
    sizeof(LinTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)LinTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)LinTable_traverse,   /* tp_traverse */
    (inquiry)LinTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
} LogTable;

static void
LogTable_generate(LogTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup, *tup2;
    T_SIZE_T x1, y1;
    MYFLT x2, y2, diff, range, logrange, logmin, ratio, low, high;

    y1 = 0;
    y2 = 0.0;

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("LogTable error: There should be at least two points in a LogTable.\n");
        return;
    }

    for (i = 0; i < (listsize - 1); i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        x1 = PyLong_AsLong(PyNumber_Long(p1));
        x2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        tup2 = PyList_GET_ITEM(self->pointslist, i + 1);
        PyObject *p2 = PyTuple_GET_ITEM(tup2, 0);
        y1 = PyLong_AsLong(PyNumber_Long(p2));
        y2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup2, 1));

        Py_DECREF(p1);
        Py_DECREF(p2);

        if (x2 <= 0)
            x2 = 0.000001;

        if (y2 <= 0)
            y2 = 0.000001;

        if (x2 > y2)
        {
            low = y2;
            high = x2;
        }
        else
        {
            low = x2;
            high = y2;
        }

        steps = y1 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("LogTable error: point position smaller than previous one.\n");
            return;
        }

        range = high - low;
        logrange = MYLOG10(high) - MYLOG10(low);
        logmin = MYLOG10(low);

        if (range == 0)
        {
            for (j = 0; j < steps; j++)
            {
                self->data[x1 + j] = x2;
            }
        }
        else
        {
            diff = (y2 - x2) / steps;

            for (j = 0; j < steps; j++)
            {
                ratio = ((x2 + diff * j) - low) / range;
                self->data[x1 + j] = MYPOW(10, ratio * logrange + logmin);
            }
        }
    }

    if (y1 < (self->size - 1))
    {
        self->data[y1] = y2;

        for (i = y1; i < self->size; i++)
        {
            self->data[i + 1] = 0.0;
        }

        self->data[self->size] = 0.0;
    }
    else
    {
        self->data[self->size - 1] = y2;
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
    PyMem_RawFree(self->data);
    LogTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
LogTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    LogTable *self;
    self = (LogTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    LogTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * LogTable_getServer(LogTable* self) { GET_SERVER };
static PyObject * LogTable_getTableStream(LogTable* self) { GET_TABLE_STREAM };
static PyObject * LogTable_setData(LogTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * LogTable_normalize(LogTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * LogTable_copyData(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * LogTable_rotate(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * LogTable_setTable(LogTable *self, PyObject *arg) { SET_TABLE };
static PyObject * LogTable_getTable(LogTable *self) { GET_TABLE };
static PyObject * LogTable_getRate(LogTable *self) { TABLE_GET_RATE };
static PyObject * LogTable_put(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * LogTable_get(LogTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * LogTable_add(LogTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * LogTable_sub(LogTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * LogTable_mul(LogTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * LogTable_div(LogTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
LogTable_setSize(LogTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    LogTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
LogTable_getSize(LogTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    LogTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef LogTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(LogTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(LogTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(LogTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef LogTable_methods[] =
{
    {"getServer", (PyCFunction)LogTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)LogTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)LogTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)LogTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)LogTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)LogTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)LogTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)LogTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)LogTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)LogTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)LogTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)LogTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)LogTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)LogTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)LogTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)LogTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)LogTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)LogTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)LogTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)LogTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)LogTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)LogTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)LogTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)LogTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)LogTable_getPoints, METH_NOARGS, NULL},
    {"replace", (PyCFunction)LogTable_replace, METH_O, NULL},
    {"add", (PyCFunction)LogTable_add, METH_O, NULL},
    {"sub", (PyCFunction)LogTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)LogTable_mul, METH_O, NULL},
    {"div", (PyCFunction)LogTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject LogTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.LogTable_base",         /*tp_name*/
    sizeof(LogTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)LogTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)LogTable_traverse,   /* tp_traverse */
    (inquiry)LogTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
} CosTable;

static void
CosTable_generate(CosTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup, *tup2;
    T_SIZE_T x1, y1;
    MYFLT x2, y2, mu, mu2;

    y1 = 0;
    y2 = 0.0;

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("CosTable error: There should be at least two points in a CosTable.\n");
        return;
    }

    for (i = 0; i < (listsize - 1); i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        x1 = PyLong_AsLong(PyNumber_Long(p1));
        x2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        tup2 = PyList_GET_ITEM(self->pointslist, i + 1);
        PyObject *p2 = PyTuple_GET_ITEM(tup2, 0);
        y1 = PyLong_AsLong(PyNumber_Long(p2));
        y2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup2, 1));

        Py_DECREF(p1);
        Py_DECREF(p2);

        steps = y1 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("CosTable error: point position smaller than previous one.\n");
            return;
        }

        for (j = 0; j < steps; j++)
        {
            mu = (MYFLT)j / steps;
            mu2 = (1.0 - MYCOS(mu * PI)) / 2.0;
            self->data[x1 + j] = x2 * (1.0 - mu2) + y2 * mu2;
        }
    }

    if (y1 < (self->size - 1))
    {
        self->data[y1] = y2;

        for (i = y1; i < self->size; i++)
        {
            self->data[i + 1] = 0.0;
        }

        self->data[self->size] = 0.0;
    }
    else
    {
        self->data[self->size - 1] = y2;
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
    PyMem_RawFree(self->data);
    CosTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CosTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    CosTable *self;
    self = (CosTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    CosTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * CosTable_getServer(CosTable* self) { GET_SERVER };
static PyObject * CosTable_getTableStream(CosTable* self) { GET_TABLE_STREAM };
static PyObject * CosTable_setData(CosTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CosTable_normalize(CosTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * CosTable_copyData(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * CosTable_rotate(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * CosTable_setTable(CosTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CosTable_getTable(CosTable *self) { GET_TABLE };
static PyObject * CosTable_getRate(CosTable *self) { TABLE_GET_RATE };
static PyObject * CosTable_put(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CosTable_get(CosTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CosTable_add(CosTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CosTable_sub(CosTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CosTable_mul(CosTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * CosTable_div(CosTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
CosTable_setSize(CosTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    CosTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
CosTable_getSize(CosTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    CosTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef CosTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CosTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(CosTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(CosTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef CosTable_methods[] =
{
    {"getServer", (PyCFunction)CosTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)CosTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)CosTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)CosTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)CosTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)CosTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)CosTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)CosTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)CosTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)CosTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)CosTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)CosTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)CosTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)CosTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)CosTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)CosTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)CosTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)CosTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)CosTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)CosTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)CosTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)CosTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)CosTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)CosTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)CosTable_getPoints, METH_NOARGS, NULL},
    {"replace", (PyCFunction)CosTable_replace, METH_O, NULL},
    {"add", (PyCFunction)CosTable_add, METH_O, NULL},
    {"sub", (PyCFunction)CosTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)CosTable_mul, METH_O, NULL},
    {"div", (PyCFunction)CosTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject CosTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CosTable_base",         /*tp_name*/
    sizeof(CosTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CosTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)CosTable_traverse,   /* tp_traverse */
    (inquiry)CosTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
} CosLogTable;

static void
CosLogTable_generate(CosLogTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup, *tup2;
    T_SIZE_T x1, y1;
    MYFLT x2, y2, range, logrange, logmin, ratio, low, high, mu;

    y1 = 0;
    y2 = 0.0;

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("CosLogTable error: There should be at least two points in a CosLogTable.\n");
        return;
    }

    for (i = 0; i < (listsize - 1); i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        x1 = PyLong_AsLong(PyNumber_Long(p1));
        x2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        tup2 = PyList_GET_ITEM(self->pointslist, i + 1);
        PyObject *p2 = PyTuple_GET_ITEM(tup2, 0);
        y1 = PyLong_AsLong(PyNumber_Long(p2));
        y2 = PyFloat_AsDouble(PyTuple_GET_ITEM(tup2, 1));

        Py_DECREF(p1);
        Py_DECREF(p2);

        if (x2 <= 0)
            x2 = 0.000001;

        if (y2 <= 0)
            y2 = 0.000001;

        if (x2 > y2)
        {
            low = y2;
            high = x2;
        }
        else
        {
            low = x2;
            high = y2;
        }

        steps = y1 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("CosLogTable error: point position smaller than previous one.\n");
            return;
        }

        range = high - low;
        logrange = MYLOG10(high) - MYLOG10(low);
        logmin = MYLOG10(low);

        if (range == 0)
        {
            for (j = 0; j < steps; j++)
            {
                self->data[x1 + j] = x2;
            }
        }
        else
        {
            for (j = 0; j < steps; j++)
            {
                mu = (MYFLT)j / steps;
                mu = (1.0 - MYCOS(mu * PI)) * 0.5;
                mu = x2 * (1.0 - mu) + y2 * mu;
                ratio = (mu - low) / range;
                self->data[x1 + j] = MYPOW(10, ratio * logrange + logmin);
            }
        }
    }

    if (y1 < (self->size - 1))
    {
        self->data[y1] = y2;

        for (i = y1; i < self->size; i++)
        {
            self->data[i + 1] = 0.0;
        }

        self->data[self->size] = 0.0;
    }
    else
    {
        self->data[self->size - 1] = y2;
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
    PyMem_RawFree(self->data);
    CosLogTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CosLogTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    CosLogTable *self;
    self = (CosLogTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|On", kwlist, &pointslist, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    CosLogTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * CosLogTable_getServer(CosLogTable* self) { GET_SERVER };
static PyObject * CosLogTable_getTableStream(CosLogTable* self) { GET_TABLE_STREAM };
static PyObject * CosLogTable_setData(CosLogTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CosLogTable_normalize(CosLogTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * CosLogTable_copyData(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * CosLogTable_rotate(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * CosLogTable_setTable(CosLogTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CosLogTable_getTable(CosLogTable *self) { GET_TABLE };
static PyObject * CosLogTable_getRate(CosLogTable *self) { TABLE_GET_RATE };
static PyObject * CosLogTable_put(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CosLogTable_get(CosLogTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CosLogTable_add(CosLogTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CosLogTable_sub(CosLogTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CosLogTable_mul(CosLogTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * CosLogTable_div(CosLogTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
CosLogTable_setSize(CosLogTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    CosLogTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
CosLogTable_getSize(CosLogTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    CosLogTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef CosLogTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CosLogTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(CosLogTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(CosLogTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef CosLogTable_methods[] =
{
    {"getServer", (PyCFunction)CosLogTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)CosLogTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)CosLogTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)CosLogTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)CosLogTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)CosLogTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)CosLogTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)CosLogTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)CosLogTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)CosLogTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)CosLogTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)CosLogTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)CosLogTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)CosLogTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)CosLogTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)CosLogTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)CosLogTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)CosLogTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)CosLogTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)CosLogTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)CosLogTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)CosLogTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)CosLogTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)CosLogTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)CosLogTable_getPoints, METH_NOARGS, NULL},
    {"replace", (PyCFunction)CosLogTable_replace, METH_O, NULL},
    {"add", (PyCFunction)CosLogTable_add, METH_O, NULL},
    {"sub", (PyCFunction)CosLogTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)CosLogTable_mul, METH_O, NULL},
    {"div", (PyCFunction)CosLogTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject CosLogTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CosLogTable_base",         /*tp_name*/
    sizeof(CosLogTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CosLogTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)CosLogTable_traverse,   /* tp_traverse */
    (inquiry)CosLogTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
    MYFLT tension;
    MYFLT bias;
} CurveTable;

static void
CurveTable_generate(CurveTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup;
    T_SIZE_T x1, x2;
    MYFLT y0, y1, y2, y3;
    MYFLT m0, m1, mu, mu2, mu3;
    MYFLT a0, a1, a2, a3;

    for (i = 0; i < self->size; i++)
    {
        self->data[i] = 0.0;
    }

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("CurveTable error: There should be at least two points in a CurveTable.\n");
        return;
    }

    T_SIZE_T times[listsize + 2];
    MYFLT values[listsize + 2];

    for (i = 0; i < listsize; i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        times[i + 1] = PyLong_AsLong(PyNumber_Long(p1));
        values[i + 1] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        Py_DECREF(p1);
    }

    // sets imaginary points
    times[0] = times[1] - times[2];

    if (values[1] < values[2])
        values[0] = values[1] - values[2];
    else
        values[0] = values[1] + values[2];

    T_SIZE_T endP = listsize + 1;
    times[endP] = times[endP - 2] - times[endP - 1];

    if (values[endP - 2] < values[endP - 1])
        values[endP] = values[endP - 1] + values[endP - 2];
    else
        values[endP] = values[endP - 1] - values[endP - 2];

    for (i = 1; i < listsize; i++)
    {
        x1 = times[i];
        x2 = times[i + 1];
        y0 = values[i - 1];
        y1 = values[i];
        y2 = values[i + 1];
        y3 = values[i + 2];

        steps = x2 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("CurveTable error: point position smaller than previous one.\n");
            return;
        }

        for (j = 0; j < steps; j++)
        {
            mu = (MYFLT)j / steps;
            mu2 = mu * mu;
            mu3 = mu2 * mu;
            m0 = (y1 - y0) * (1.0 + self->bias) * (1.0 - self->tension) / 2.0;
            m0 += (y2 - y1) * (1.0 - self->bias) * (1.0 - self->tension) / 2.0;
            m1 = (y2 - y1) * (1.0 + self->bias) * (1.0 - self->tension) / 2.0;
            m1 += (y3 - y2) * (1.0 - self->bias) * (1.0 - self->tension) / 2.0;
            a0 = 2.0 * mu3 - 3.0 * mu2 + 1.0;
            a1 = mu3 - 2.0 * mu2 + mu;
            a2 = mu3 - mu2;
            a3 = -2.0 * mu3 + 3.0 * mu2;

            self->data[x1 + j] = (a0 * y1 + a1 * m0 + a2 * m1 + a3 * y2);
        }
    }

    self->data[self->size] = self->data[self->size - 1];
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
    PyMem_RawFree(self->data);
    CurveTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
CurveTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    CurveTable *self;
    self = (CurveTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->tension = 0.0;
    self->bias = 0.0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "tension", "bias", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFFN, kwlist, &pointslist, &self->tension, &self->bias, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    CurveTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * CurveTable_getServer(CurveTable* self) { GET_SERVER };
static PyObject * CurveTable_getTableStream(CurveTable* self) { GET_TABLE_STREAM };
static PyObject * CurveTable_setData(CurveTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * CurveTable_normalize(CurveTable * self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * CurveTable_copyData(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * CurveTable_rotate(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * CurveTable_setTable(CurveTable *self, PyObject *arg) { SET_TABLE };
static PyObject * CurveTable_getTable(CurveTable *self) { GET_TABLE };
static PyObject * CurveTable_getRate(CurveTable *self) { TABLE_GET_RATE };
static PyObject * CurveTable_put(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * CurveTable_get(CurveTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * CurveTable_add(CurveTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * CurveTable_sub(CurveTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * CurveTable_mul(CurveTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * CurveTable_div(CurveTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
CurveTable_setTension(CurveTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the tension attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The tension attribute value must be a float.");
        return PyLong_FromLong(-1);
    }

    self->tension = PyFloat_AsDouble(value);

    CurveTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
CurveTable_setBias(CurveTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the bias attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The bias attribute value must be a float.");
        return PyLong_FromLong(-1);
    }

    self->bias = PyFloat_AsDouble(value);

    CurveTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
CurveTable_setSize(CurveTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    CurveTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
CurveTable_getSize(CurveTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    CurveTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef CurveTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(CurveTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(CurveTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(CurveTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef CurveTable_methods[] =
{
    {"getServer", (PyCFunction)CurveTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)CurveTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)CurveTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)CurveTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)CurveTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)CurveTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)CurveTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)CurveTable_setData, METH_O, NULL},
    {"setSize", (PyCFunction)CurveTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)CurveTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)CurveTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)CurveTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)CurveTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)CurveTable_getPoints, METH_NOARGS, NULL},
    {"setTension", (PyCFunction)CurveTable_setTension, METH_O, NULL},
    {"setBias", (PyCFunction)CurveTable_setBias, METH_O, NULL},
    {"replace", (PyCFunction)CurveTable_replace, METH_O, NULL},
    {"normalize", (PyCFunction)CurveTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)CurveTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)CurveTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)CurveTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)CurveTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)CurveTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)CurveTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)CurveTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)CurveTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)CurveTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)CurveTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)CurveTable_add, METH_O, NULL},
    {"sub", (PyCFunction)CurveTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)CurveTable_mul, METH_O, NULL},
    {"div", (PyCFunction)CurveTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject CurveTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.CurveTable_base",         /*tp_name*/
    sizeof(CurveTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CurveTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)CurveTable_traverse,   /* tp_traverse */
    (inquiry)CurveTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    PyObject *pointslist;
    MYFLT exp;
    int inverse;
} ExpTable;

static void
ExpTable_generate(ExpTable *self)
{
    T_SIZE_T i, j, steps;
    T_SIZE_T listsize;
    PyObject *tup;
    T_SIZE_T x1, x2;
    MYFLT y1, y2, range, inc, pointer, scl;

    for (i = 0; i < self->size; i++)
    {
        self->data[i] = 0.0;
    }

    listsize = PyList_Size(self->pointslist);

    if (listsize < 2)
    {
        PySys_WriteStderr("ExpTable error: There should be at least two points in a ExpTable.\n");
        return;
    }

    T_SIZE_T times[listsize];
    MYFLT values[listsize];

    for (i = 0; i < listsize; i++)
    {
        tup = PyList_GET_ITEM(self->pointslist, i);
        PyObject *p1 = PyTuple_GET_ITEM(tup, 0);
        times[i] = PyLong_AsLong(PyNumber_Long(p1));
        values[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
        Py_DECREF(p1);
    }

    y1 = y2 = 0.0;

    for (i = 0; i < (listsize - 1); i++)
    {
        x1 = times[i];
        x2 = times[i + 1];
        y1 = values[i];
        y2 = values[i + 1];

        range = y2 - y1;
        steps = x2 - x1;

        if (steps <= 0)
        {
            PySys_WriteStderr("ExpTable error: point position smaller than previous one.\n");
            return;
        }

        inc = 1.0 / steps;
        pointer = 0.0;

        if (self->inverse == 1)
        {
            if (range >= 0)
            {
                for (j = 0; j < steps; j++)
                {
                    scl = MYPOW(pointer, self->exp);
                    self->data[x1 + j] = scl * range + y1;
                    pointer += inc;
                }
            }
            else
            {
                for (j = 0; j < steps; j++)
                {
                    scl = 1.0 - MYPOW(1.0 - pointer, self->exp);
                    self->data[x1 + j] = scl * range + y1;
                    pointer += inc;
                }
            }
        }
        else
        {
            for (j = 0; j < steps; j++)
            {
                scl = MYPOW(pointer, self->exp);
                self->data[x1 + j] = scl * range + y1;
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
    PyMem_RawFree(self->data);
    ExpTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
ExpTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist = NULL;
    ExpTable *self;
    self = (ExpTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->pointslist = PyList_New(0);
    self->size = 8192;
    self->exp = 10.0;
    self->inverse = 1;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"list", "exp", "inverse", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__OFIN, kwlist, &pointslist, &self->exp, &self->inverse, &self->size))
        Py_RETURN_NONE;

    if (pointslist)
    {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else
    {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyLong_FromLong(self->size), PyFloat_FromDouble(1.)));
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    ExpTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * ExpTable_getServer(ExpTable* self) { GET_SERVER };
static PyObject * ExpTable_getTableStream(ExpTable* self) { GET_TABLE_STREAM };
static PyObject * ExpTable_setData(ExpTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * ExpTable_normalize(ExpTable * self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * ExpTable_copyData(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * ExpTable_rotate(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * ExpTable_setTable(ExpTable *self, PyObject *arg) { SET_TABLE };
static PyObject * ExpTable_getTable(ExpTable *self) { GET_TABLE };
static PyObject * ExpTable_getRate(ExpTable *self) { TABLE_GET_RATE };
static PyObject * ExpTable_put(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * ExpTable_get(ExpTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * ExpTable_add(ExpTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * ExpTable_sub(ExpTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * ExpTable_mul(ExpTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * ExpTable_div(ExpTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
ExpTable_setExp(ExpTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the exp attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The exp attribute value must be a float.");
        return PyLong_FromLong(-1);
    }

    self->exp = PyFloat_AsDouble(value);

    ExpTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ExpTable_setInverse(ExpTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the inverse attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyLong_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The inverse attribute value must be a boolean (True or False or 0 or 1).");
        return PyLong_FromLong(-1);
    }

    self->inverse = PyLong_AsLong(value);

    ExpTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ExpTable_setSize(ExpTable *self, PyObject *value)
{
    TABLE_SET_SIZE_WITH_POINT_LIST

    ExpTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
ExpTable_getSize(ExpTable *self)
{
    return PyLong_FromLong(self->size);
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
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyList_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value;

    ExpTable_generate(self);

    Py_RETURN_NONE;
}

static PyMemberDef ExpTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(ExpTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(ExpTable, tablestream), 0, NULL},
    {"pointslist", T_OBJECT_EX, offsetof(ExpTable, pointslist), 0, NULL},
    {NULL}  /* Sentinel */
};
static PyMethodDef ExpTable_methods[] =
{
    {"getServer", (PyCFunction)ExpTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)ExpTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)ExpTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)ExpTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)ExpTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)ExpTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)ExpTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)ExpTable_setData, METH_O, NULL},
    {"setSize", (PyCFunction)ExpTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)ExpTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)ExpTable_getRate, METH_NOARGS, NULL},
    {"put", (PyCFunction)ExpTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)ExpTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getPoints", (PyCFunction)ExpTable_getPoints, METH_NOARGS, NULL},
    {"setExp", (PyCFunction)ExpTable_setExp, METH_O, NULL},
    {"setInverse", (PyCFunction)ExpTable_setInverse, METH_O, NULL},
    {"replace", (PyCFunction)ExpTable_replace, METH_O, NULL},
    {"normalize", (PyCFunction)ExpTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)ExpTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)ExpTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)ExpTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)ExpTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)ExpTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)ExpTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)ExpTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)ExpTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)ExpTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)ExpTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)ExpTable_add, METH_O, NULL},
    {"sub", (PyCFunction)ExpTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)ExpTable_mul, METH_O, NULL},
    {"div", (PyCFunction)ExpTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject ExpTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.ExpTable_base",         /*tp_name*/
    sizeof(ExpTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ExpTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)ExpTable_traverse,   /* tp_traverse */
    (inquiry)ExpTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
/* NewTable structure */
/***********************/
typedef struct
{
    pyo_table_HEAD
    MYFLT length;
    MYFLT feedback;
    MYFLT sr;
} NewTable;

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
    PyMem_RawFree(self->data);
    NewTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NewTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    T_SIZE_T i;
    PyObject *inittmp = NULL;
    NewTable *self;
    self = (NewTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->feedback = 0.0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"length", "init", "feedback", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_OF, kwlist, &self->length, &inittmp, &self->feedback))
        Py_RETURN_NONE;

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    self->sr = (MYFLT)PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    self->size = (T_SIZE_T)(self->length * self->sr + 0.5);
    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));

    for (i = 0; i < (self->size + 1); i++)
    {
        self->data[i] = 0.;
    }

    TableStream_setFeedback(self->tablestream, self->feedback);

    TableStream_setSize(self->tablestream, self->size);

    if (inittmp && PyList_Check(inittmp))
    {
        if (PyList_Size(inittmp) < self->size)
        {
            for (i = 0; i < (self->size - PyList_Size(inittmp)); i++)
            {
                PyList_Append(inittmp, PyFloat_FromDouble(0.0));
            }
            PySys_WriteStdout("Warning: NewTable data length < size... padded with 0s.\n");
        }
        else if (PyList_Size(inittmp) > self->size)
        {
            inittmp = PyList_GetSlice(inittmp, 0, self->size);
            PySys_WriteStdout("Warning: NewTable data length > size... truncated to size.\n");
        }
        PyObject_CallMethod((PyObject *)self, "setTable", "O", inittmp);
    }

    TableStream_setData(self->tablestream, self->data);
    TableStream_setSamplingRate(self->tablestream, self->sr);

    return (PyObject *)self;
}

static PyObject * NewTable_getServer(NewTable* self) { GET_SERVER };
static PyObject * NewTable_getTableStream(NewTable* self) { GET_TABLE_STREAM };
static PyObject * NewTable_setData(NewTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * NewTable_normalize(NewTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * NewTable_copyData(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * NewTable_rotate(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * NewTable_setTable(NewTable *self, PyObject *arg) { SET_TABLE };
static PyObject * NewTable_getTable(NewTable *self) { GET_TABLE };
static PyObject * NewTable_getRate(NewTable *self) { TABLE_GET_RATE };
static PyObject * NewTable_put(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * NewTable_get(NewTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * NewTable_add(NewTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * NewTable_sub(NewTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * NewTable_mul(NewTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * NewTable_div(NewTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
NewTable_setLength(NewTable *self, PyObject *value)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the length attribute.");
        return PyLong_FromLong(-1);
    }

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The length attribute value must be a number.");
        return PyLong_FromLong(-1);
    }

    MYFLT tmp = (MYFLT)PyFloat_AsDouble(value);
    MYFLT diff = tmp - self->length;

    T_SIZE_T size = (T_SIZE_T)(tmp * self->sr + 0.5);

    MYFLT *data = (MYFLT *)PyMem_RawRealloc(self->data, (size + 1) * sizeof(MYFLT));
    if (data == NULL)
    {
        Py_RETURN_NONE;
    }

    self->data = data;
    self->size = size;
    self->length = tmp;
    TableStream_setData(self->tablestream, self->data);
    TableStream_setSize(self->tablestream, self->size);

    if (diff > 0)
    {
        MYFLT startf = self->length - diff;
        T_SIZE_T starti = (T_SIZE_T)(startf * self->sr + 0.5);

        for ( ; starti < (self->size + 1); starti++)
        {
            self->data[starti] = 0.0;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
NewTable_setSize(NewTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    MYFLT oldLength = self->length;
    self->length = self->size / self->sr;
    MYFLT diff = self->length - oldLength;

    if (diff > 0)
    {
        MYFLT startf = self->length - diff;
        T_SIZE_T starti = (T_SIZE_T)(startf * self->sr);

        for ( ; starti < (self->size + 1); starti++)
        {
            self->data[starti] = 0.0;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
NewTable_getSize(NewTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyObject *
NewTable_getLength(NewTable *self)
{
    return PyFloat_FromDouble(self->length);
};

static PyObject *
NewTable_setFeedback(NewTable *self, PyObject *value)
{
    MYFLT feed;

    if (PyNumber_Check(value))
    {
        feed = PyFloat_AsDouble(value);

        if (feed < -1.0)
            feed = -1.0;
        else if (feed > 1.0)
            feed = 1.0;

        self->feedback = feed;
        TableStream_setFeedback(self->tablestream, feed);
    }

    Py_RETURN_NONE;
}

static PyMemberDef NewTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(NewTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(NewTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef NewTable_methods[] =
{
    {"getServer", (PyCFunction)NewTable_getServer, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)NewTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)NewTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)NewTable_getTableStream, METH_NOARGS, NULL},
    {"setFeedback", (PyCFunction)NewTable_setFeedback, METH_O, NULL},
    {"setData", (PyCFunction)NewTable_setData, METH_O, NULL},
    {"setLength", (PyCFunction)NewTable_setLength, METH_O, NULL},
    {"setSize", (PyCFunction)NewTable_setSize, METH_O, NULL},
    {"copy", (PyCFunction)NewTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)NewTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)NewTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"normalize", (PyCFunction)NewTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)NewTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)NewTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)NewTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)NewTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)NewTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)NewTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)NewTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)NewTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)NewTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)NewTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"put", (PyCFunction)NewTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)NewTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getSize", (PyCFunction)NewTable_getSize, METH_NOARGS, NULL},
    {"getLength", (PyCFunction)NewTable_getLength, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)NewTable_getRate, METH_NOARGS, NULL},
    {"add", (PyCFunction)NewTable_add, METH_O, NULL},
    {"sub", (PyCFunction)NewTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)NewTable_mul, METH_O, NULL},
    {"div", (PyCFunction)NewTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject NewTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.NewTable_base",         /*tp_name*/
    sizeof(NewTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NewTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)NewTable_traverse,   /* tp_traverse */
    (inquiry)NewTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    MYFLT sr;
} DataTable;

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
    PyMem_RawFree(self->data);
    DataTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
DataTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    T_SIZE_T i;
    PyObject *inittmp = NULL;
    DataTable *self;
    self = (DataTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"size", "init", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "n|O", kwlist, &self->size, &inittmp))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));

    for (i = 0; i < (self->size + 1); i++)
    {
        self->data[i] = 0.;
    }

    TableStream_setSize(self->tablestream, self->size);

    if (inittmp)
    {
        PyObject_CallMethod((PyObject *)self, "setTable", "O", inittmp);
    }

    TableStream_setData(self->tablestream, self->data);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    self->sr = (MYFLT)PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, self->sr);

    return (PyObject *)self;
}

static PyObject * DataTable_getServer(DataTable* self) { GET_SERVER };
static PyObject * DataTable_getTableStream(DataTable* self) { GET_TABLE_STREAM };
static PyObject * DataTable_setData(DataTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * DataTable_normalize(DataTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * DataTable_copyData(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * DataTable_rotate(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * DataTable_setTable(DataTable *self, PyObject *arg) { SET_TABLE };
static PyObject * DataTable_getTable(DataTable *self) { GET_TABLE };
static PyObject * DataTable_getRate(DataTable *self) { TABLE_GET_RATE };
static PyObject * DataTable_put(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * DataTable_get(DataTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * DataTable_add(DataTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * DataTable_sub(DataTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * DataTable_mul(DataTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * DataTable_div(DataTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
DataTable_getSize(DataTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef DataTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(DataTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(DataTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef DataTable_methods[] =
{
    {"getServer", (PyCFunction)DataTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)DataTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)DataTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)DataTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)DataTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)DataTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)DataTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)DataTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)DataTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)DataTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)DataTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)DataTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)DataTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)DataTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)DataTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)DataTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)DataTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)DataTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)DataTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"put", (PyCFunction)DataTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)DataTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getSize", (PyCFunction)DataTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)DataTable_getRate, METH_NOARGS, NULL},
    {"add", (PyCFunction)DataTable_add, METH_O, NULL},
    {"sub", (PyCFunction)DataTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)DataTable_mul, METH_O, NULL},
    {"div", (PyCFunction)DataTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject DataTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.DataTable_base",         /*tp_name*/
    sizeof(DataTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)DataTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)DataTable_traverse,   /* tp_traverse */
    (inquiry)DataTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_table_HEAD
    MYFLT slope;
} AtanTable;

static void
AtanTable_generate(AtanTable *self)
{
    T_SIZE_T i, hsize;
    MYFLT drv, invhsize, val, t, fac = 0;

    hsize = self->size / 2;
    invhsize = 1.0 / hsize;

    drv = 1 - self->slope;
    drv = drv * drv * drv * PI;

    for (i = 0; i <= hsize; i++)
    {
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
    PyMem_RawFree(self->data);
    AtanTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
AtanTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    AtanTable *self;
    self = (AtanTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 8192;
    self->slope = 0.5;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"slope", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_N, kwlist, &self->slope, &self->size))
        Py_RETURN_NONE;

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
    AtanTable_generate(self);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, sr);

    return (PyObject *)self;
}

static PyObject * AtanTable_getServer(AtanTable* self) { GET_SERVER };
static PyObject * AtanTable_getTableStream(AtanTable* self) { GET_TABLE_STREAM };
static PyObject * AtanTable_setData(AtanTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * AtanTable_normalize(AtanTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
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
static PyObject * AtanTable_copyData(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * AtanTable_rotate(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * AtanTable_setTable(AtanTable *self, PyObject *arg) { SET_TABLE };
static PyObject * AtanTable_getTable(AtanTable *self) { GET_TABLE };
static PyObject * AtanTable_getRate(AtanTable *self) { TABLE_GET_RATE };
static PyObject * AtanTable_put(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * AtanTable_get(AtanTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * AtanTable_add(AtanTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * AtanTable_sub(AtanTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * AtanTable_mul(AtanTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * AtanTable_div(AtanTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
AtanTable_setSlope(AtanTable *self, PyObject *value)
{

    if (! PyNumber_Check(value))
    {
        PyErr_SetString(PyExc_TypeError, "The slope attribute value must be a number.");
        return PyLong_FromLong(-1);
    }

    self->slope = PyFloat_AsDouble(value);

    if (self->slope < 0.0)
        self->slope = 0.0;
    else if (self->slope > 1.0)
        self->slope = 1.0;

    AtanTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
AtanTable_setSize(AtanTable *self, PyObject *value)
{
    TABLE_SET_SIZE

    AtanTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
AtanTable_getSize(AtanTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef AtanTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(AtanTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(AtanTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef AtanTable_methods[] =
{
    {"getServer", (PyCFunction)AtanTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)AtanTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)AtanTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)AtanTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)AtanTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)AtanTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)AtanTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)AtanTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)AtanTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)AtanTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)AtanTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)AtanTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)AtanTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)AtanTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)AtanTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)AtanTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)AtanTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)AtanTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)AtanTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)AtanTable_setSize, METH_O, NULL},
    {"getSize", (PyCFunction)AtanTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)AtanTable_getRate, METH_NOARGS, NULL},
    {"setSlope", (PyCFunction)AtanTable_setSlope, METH_O, NULL},
    {"put", (PyCFunction)AtanTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)AtanTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)AtanTable_add, METH_O, NULL},
    {"sub", (PyCFunction)AtanTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)AtanTable_mul, METH_O, NULL},
    {"div", (PyCFunction)AtanTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject AtanTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.AtanTable_base",         /*tp_name*/
    sizeof(AtanTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)AtanTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)AtanTable_traverse,   /* tp_traverse */
    (inquiry)AtanTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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

static int
isPowerOfTwo(int x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

/***********************/
/* PadSynthTable structure */
/***********************/
typedef struct
{
    pyo_table_HEAD
    MYFLT **twiddle;
    MYFLT basefreq;
    MYFLT spread;
    MYFLT bw;
    MYFLT bwscl;
    int nharms;
    MYFLT damp;
    double sr;
    MYFLT *amp;
    MYFLT *inframe;
    int allocated;
} PadSynthTable;

static void
PadSynthTable_gen_twiddle(PadSynthTable *self)
{
    int i, n8;
    n8 = self->size >> 3;

    if (self->allocated)
    {
        for (i = 0; i < 4; i++)
            PyMem_RawFree(self->twiddle[i]);
    }

    self->twiddle = (MYFLT **)PyMem_RawRealloc(self->twiddle, 4 * sizeof(MYFLT *));

    for (i = 0; i < 4; i++)
        self->twiddle[i] = (MYFLT *)PyMem_RawMalloc(n8 * sizeof(MYFLT));

    fft_compute_split_twiddle(self->twiddle, self->size);

    self->allocated = 1;
}

static void
PadSynthTable_generate(PadSynthTable *self)
{
    T_SIZE_T i, hsize = self->size / 2;
    int nh;
    MYFLT bfac, i2sr, bfonsr, nhspd, bwhz, bwi, fi, gain, absv, max, x, phase;
    MYFLT ifsize = 1.0 / (MYFLT)self->size;
    MYFLT twopirndmax = TWOPI / (MYFLT)RAND_MAX;

    for (i = 0; i < hsize; i++)
    {
        self->amp[i] = 0.0;
    }

    bfac = (MYPOW(2.0, self->bw / 1200.0) - 1.0) * self->basefreq;
    i2sr = 1.0 / (2.0 * self->sr);
    bfonsr = self->basefreq / self->sr;
    gain = self->damp;

    for (nh = 1; nh < self->nharms; nh++)
    {
        nhspd = MYPOW(nh, self->spread);
        bwhz = bfac * MYPOW(nhspd, self->bwscl);
        bwi = 1.0 / (bwhz * i2sr);
        fi = bfonsr * nhspd;

        for (i = 0; i < hsize; i++)
        {
            // harmonic profile.
            x = (i * ifsize - fi) * bwi;
            x *= x;

            if (x < 14.71280603)
                self->amp[i] += MYEXP(-x) * bwi * gain;
        }

        gain *= self->damp;
    }

    phase = rand() * twopirndmax;
    self->inframe[0] = self->amp[0] * MYCOS(phase);
    self->inframe[hsize] = 0.0;

    for (i = 1; i < hsize; i++)
    {
        phase = rand() * twopirndmax;
        self->inframe[i] = self->amp[i] * MYCOS(phase);
        self->inframe[self->size - i] = self->amp[i] * MYSIN(phase);
    }

    irealfft_split(self->inframe, self->data, self->size, self->twiddle);

    max = 0.0;

    for (i = 0; i < self->size; i++)
    {
        absv = MYFABS(self->data[i]);

        if (absv > max)
            max = absv;
    }

    if (max < 1e-5)
        max = 1e-5;

    max = 1.0 / (max * 1.4142);

    for (i = 0; i < self->size; i++)
    {
        self->data[i] *= max;
    }

    self->data[self->size] = self->data[0];
}

static int
PadSynthTable_traverse(PadSynthTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
PadSynthTable_clear(PadSynthTable *self)
{
    pyo_table_CLEAR
    return 0;
}

static void
PadSynthTable_dealloc(PadSynthTable* self)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        PyMem_RawFree(self->twiddle[i]);
    }

    PyMem_RawFree(self->twiddle);
    PyMem_RawFree(self->data);
    PyMem_RawFree(self->amp);
    PyMem_RawFree(self->inframe);
    PadSynthTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
PadSynthTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PadSynthTable *self;
    self = (PadSynthTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->size = 262144;
    self->basefreq = 440;
    self->spread = 1.0;
    self->bw = 50.0;
    self->bwscl = 1.0;
    self->nharms = 64;
    self->damp = 0.7;
    self->allocated = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"basefreq", "spread", "bw", "bwscl", "nharms", "damp", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFFFIFN, kwlist, &self->basefreq, &self->spread, &self->bw,
                                      &self->bwscl, &self->nharms, &self->damp, &self->size))
        Py_RETURN_NONE;

    if (!isPowerOfTwo(self->size))
    {
        int k = 1;

        while (k < self->size)
            k *= 2;

        self->size = k;
        PySys_WriteStdout("PadSynthTable size must be a power-of-2, using the next power-of-2 greater than size : %ld\n", (long)self->size);
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    self->amp = (MYFLT *)PyMem_RawRealloc(self->amp, (self->size / 2) * sizeof(MYFLT));
    self->inframe = (MYFLT *)PyMem_RawRealloc(self->inframe, self->size * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    self->sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, self->sr);

    PadSynthTable_gen_twiddle(self);

    srand(time(NULL));
    PadSynthTable_generate(self);

    return (PyObject *)self;
}

static PyObject * PadSynthTable_getServer(PadSynthTable* self) { GET_SERVER };
static PyObject * PadSynthTable_getTableStream(PadSynthTable* self) { GET_TABLE_STREAM };
static PyObject * PadSynthTable_setData(PadSynthTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * PadSynthTable_normalize(PadSynthTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
static PyObject * PadSynthTable_reset(PadSynthTable *self) { TABLE_RESET };
static PyObject * PadSynthTable_removeDC(PadSynthTable *self) { REMOVE_DC };
static PyObject * PadSynthTable_reverse(PadSynthTable *self) { REVERSE };
static PyObject * PadSynthTable_invert(PadSynthTable *self) { INVERT };
static PyObject * PadSynthTable_rectify(PadSynthTable *self) { RECTIFY };
static PyObject * PadSynthTable_bipolarGain(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * PadSynthTable_lowpass(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * PadSynthTable_fadein(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * PadSynthTable_fadeout(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * PadSynthTable_pow(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * PadSynthTable_copy(PadSynthTable *self, PyObject *arg) { COPY };
static PyObject * PadSynthTable_copyData(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * PadSynthTable_rotate(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * PadSynthTable_setTable(PadSynthTable *self, PyObject *arg) { SET_TABLE };
static PyObject * PadSynthTable_getTable(PadSynthTable *self) { GET_TABLE };
static PyObject * PadSynthTable_getRate(PadSynthTable *self) { TABLE_GET_RATE };
static PyObject * PadSynthTable_put(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * PadSynthTable_get(PadSynthTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * PadSynthTable_add(PadSynthTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * PadSynthTable_sub(PadSynthTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * PadSynthTable_mul(PadSynthTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * PadSynthTable_div(PadSynthTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
PadSynthTable_setBaseFreq(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"basefreq", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->basefreq, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setSpread(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"spread", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->spread, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setBw(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"bw", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->bw, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setBwScl(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"bwscl", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->bwscl, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setNharms(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"nharms", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i|i", kwlist, &self->nharms, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setDamp(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"damp", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &self->damp, &generate))
        Py_RETURN_NONE;

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_setSize(PadSynthTable *self, PyObject *args, PyObject *kwds)
{
    int generate = 1;

    static char *kwlist[] = {"size", "generate", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "n|i", kwlist, &self->size, &generate))
        Py_RETURN_NONE;

    if (!isPowerOfTwo(self->size))
    {
        int k = 1;

        while (k < self->size)
            k *= 2;

        self->size = k;
        PySys_WriteStdout("PadSynthTable size must be a power-of-2, using the next power-of-2 greater than size : %ld\n", (long)self->size);
    }

    self->data = (MYFLT *)PyMem_RawRealloc(self->data, (self->size + 1) * sizeof(MYFLT));
    self->amp = (MYFLT *)PyMem_RawRealloc(self->amp, (self->size / 2) * sizeof(MYFLT));
    self->inframe = (MYFLT *)PyMem_RawRealloc(self->inframe, self->size * sizeof(MYFLT));
    TableStream_setSize(self->tablestream, self->size);

    PadSynthTable_gen_twiddle(self);

    if (generate)
        PadSynthTable_generate(self);

    Py_RETURN_NONE;
}

static PyObject *
PadSynthTable_getSize(PadSynthTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef PadSynthTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(PadSynthTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(PadSynthTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef PadSynthTable_methods[] =
{
    {"getServer", (PyCFunction)PadSynthTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)PadSynthTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)PadSynthTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)PadSynthTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)PadSynthTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)PadSynthTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)PadSynthTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)PadSynthTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)PadSynthTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)PadSynthTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)PadSynthTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)PadSynthTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)PadSynthTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)PadSynthTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)PadSynthTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)PadSynthTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)PadSynthTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)PadSynthTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)PadSynthTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getSize", (PyCFunction)PadSynthTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)PadSynthTable_getRate, METH_NOARGS, NULL},
    {"setBaseFreq", (PyCFunction)PadSynthTable_setBaseFreq, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSpread", (PyCFunction)PadSynthTable_setSpread, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setBw", (PyCFunction)PadSynthTable_setBw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setBwScl", (PyCFunction)PadSynthTable_setBwScl, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setNharms", (PyCFunction)PadSynthTable_setNharms, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setDamp", (PyCFunction)PadSynthTable_setDamp, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setSize", (PyCFunction)PadSynthTable_setSize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"put", (PyCFunction)PadSynthTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)PadSynthTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction)PadSynthTable_add, METH_O, NULL},
    {"sub", (PyCFunction)PadSynthTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)PadSynthTable_mul, METH_O, NULL},
    {"div", (PyCFunction)PadSynthTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject PadSynthTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.PadSynthTable_base",         /*tp_name*/
    sizeof(PadSynthTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PadSynthTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)PadSynthTable_traverse,   /* tp_traverse */
    (inquiry)PadSynthTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    PadSynthTable_methods,             /* tp_methods */
    PadSynthTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    PadSynthTable_new,                 /* tp_new */
};

/******************************/
/* TableRec object definition */
/******************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *table;
    T_SIZE_T pointer;
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
    int i;
    MYFLT val;
    T_SIZE_T num, upBound;
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    if (!self->active)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            self->time_buffer_streams[i] = self->pointer;
        }
    }

    if ((size - self->pointer) >= self->bufsize)
        num = self->bufsize;
    else
    {
        num = size - self->pointer;

        if (self->active == 1)
        {
            if (num <= 0)
                self->trigsBuffer[0] = 1.0;
            else
                self->trigsBuffer[num - 1] = 1.0;

            self->active = 0;
        }
    }

    if (self->pointer < size)
    {
        upBound = (T_SIZE_T)(size - self->fadeInSample);

        for (i = 0; i < self->bufsize; i++)
        {
            self->buffer[i] = 0.0;
        }

        MYFLT *in = Stream_getData((Stream *)self->input_stream);

        for (i = 0; i < num; i++)
        {
            if (self->pointer < self->fadeInSample)
                val = self->pointer / self->fadeInSample;
            else if (self->pointer >= upBound)
                val = (size - (self->pointer + 1)) / self->fadeInSample;
            else
                val = 1.;

            self->buffer[i] = in[i] * val;
            self->time_buffer_streams[i] = self->pointer++;
        }

        TableStream_recordChunk((TableStream *)self->table, self->buffer, num);

        if (num < self->bufsize)
        {
            for (i = num; i < self->bufsize; i++)
            {
                self->time_buffer_streams[i] = self->pointer;
            }
        }
    }
}

static MYFLT *
TableRec_getTimeBuffer(TableRec *self)
{
    return self->time_buffer_streams;
}

static int
TableRec_traverse(TableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int
TableRec_clear(TableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
TableRec_dealloc(TableRec* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->buffer);
    PyMem_RawFree(self->trigsBuffer);
    PyMem_RawFree(self->time_buffer_streams);
    TableRec_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
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

    static char *kwlist[] = {"input", "table", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_F, kwlist, &inputtmp, &tabletmp, &self->fadetime))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableRec must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer = (MYFLT *)PyMem_RawRealloc(self->buffer, self->bufsize * sizeof(MYFLT));
    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->time_buffer_streams = (MYFLT *)PyMem_RawRealloc(self->time_buffer_streams, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->buffer[i] = self->trigsBuffer[i] = self->time_buffer_streams[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

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
    int j;

    for (j = 0; j < self->bufsize; j++)
    {
        self->time_buffer_streams[j] = 0;
    }

    self->pointer = 0;
    self->active = 1;
    TableStream_resetRecordingPointer((TableStream *)self->table);
    PLAY
};

static PyObject * TableRec_stop(TableRec *self, PyObject *args, PyObject *kwds)
{
    int i, nearestBuf = 0;
    float wait = 0.0;

    static char *kwlist[] = {"wait", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|f", kwlist, &wait))
        return PyLong_FromLong(-1);

    if (wait == 0)
    {
        Stream_setStreamActive(self->stream, 0);
        Stream_setStreamChnl(self->stream, 0);
        Stream_setStreamToDac(self->stream, 0);

        for (i = 0; i < self->bufsize; i++)
        {
            self->time_buffer_streams[i] = self->pointer;
            self->data[i] = 0;
        }
    }
    else
    {
        Stream_resetBufferCount(self->stream);
        nearestBuf = (int)roundf((wait * self->sr) / self->bufsize + 0.5);
        Stream_setDuration(self->stream, nearestBuf);
    }

    Py_RETURN_NONE;
};

static PyObject *
TableRec_setTable(TableRec *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyMemberDef TableRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TableRec, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TableRec, stream), 0, NULL},
    {"trig_stream", T_OBJECT_EX, offsetof(TableRec, trig_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TableRec, input), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(TableRec, table), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TableRec_methods[] =
{
    {"getServer", (PyCFunction)TableRec_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TableRec_getStream, METH_NOARGS, NULL},
    {"_getTriggerStream", (PyCFunction)TableRec_getTriggerStream, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)TableRec_setTable, METH_O, NULL},
    {"play", (PyCFunction)TableRec_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TableRec_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject TableRecType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TableRec_base",         /*tp_name*/
    sizeof(TableRec),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TableRec_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TableRec_traverse,   /* tp_traverse */
    (inquiry)TableRec_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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

typedef struct
{
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
TableRecTimeStream_setProcMode(TableRecTimeStream *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
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

    for (i = 0; i < self->bufsize; i++)
    {
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
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TableRecTimeStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp = NULL;
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
static PyObject * TableRecTimeStream_stop(TableRecTimeStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TableRecTimeStream_multiply(TableRecTimeStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TableRecTimeStream_inplace_multiply(TableRecTimeStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TableRecTimeStream_add(TableRecTimeStream *self, PyObject *arg) { ADD };
static PyObject * TableRecTimeStream_inplace_add(TableRecTimeStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TableRecTimeStream_sub(TableRecTimeStream *self, PyObject *arg) { SUB };
static PyObject * TableRecTimeStream_inplace_sub(TableRecTimeStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TableRecTimeStream_div(TableRecTimeStream *self, PyObject *arg) { DIV };
static PyObject * TableRecTimeStream_inplace_div(TableRecTimeStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TableRecTimeStream_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TableRecTimeStream, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TableRecTimeStream, stream), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(TableRecTimeStream, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(TableRecTimeStream, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TableRecTimeStream_methods[] =
{
    {"getServer", (PyCFunction)TableRecTimeStream_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TableRecTimeStream_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)TableRecTimeStream_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)TableRecTimeStream_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TableRecTimeStream_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)TableRecTimeStream_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)TableRecTimeStream_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)TableRecTimeStream_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)TableRecTimeStream_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TableRecTimeStream_as_number =
{
    (binaryfunc)TableRecTimeStream_add,                         /*nb_add*/
    (binaryfunc)TableRecTimeStream_sub,                         /*nb_subtract*/
    (binaryfunc)TableRecTimeStream_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)TableRecTimeStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TableRecTimeStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TableRecTimeStream_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)TableRecTimeStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)TableRecTimeStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TableRecTimeStreamType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TableRecTimeStream_base",         /*tp_name*/
    sizeof(TableRecTimeStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TableRecTimeStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TableRecTimeStream_traverse,   /* tp_traverse */
    (inquiry)TableRecTimeStream_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *table;
    PyObject *sources;
    MYFLT *buffer;
    T_SIZE_T last_size;
} TableMorph;

static MYFLT
TableMorph_clip(MYFLT x)
{
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
    T_SIZE_T i;
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);
    self->last_size = size;
    self->buffer = (MYFLT *)PyMem_RawRealloc(self->buffer, size * sizeof(MYFLT));

    for (i = 0; i < size; i++)
    {
        self->buffer[i] = 0.0;
    }
}

static void
TableMorph_compute_next_data_frame(TableMorph *self)
{
    T_SIZE_T i;
    int x, y;
    MYFLT input, interp, interp1, interp2;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);
    int len = PyList_Size(self->sources);

    if (size != self->last_size)
        TableMorph_alloc_memories(self);

    input = TableMorph_clip(in[0]);

    interp = input * (len - 1);
    x = (int)(interp);
    y = x + 1;

    MYFLT *tab1 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, x), "getTableStream", ""));
    MYFLT *tab2 = TableStream_getData((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, y), "getTableStream", ""));
    T_SIZE_T size1 = TableStream_getSize((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, x), "getTableStream", ""));
    T_SIZE_T size2 = TableStream_getSize((TableStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, y), "getTableStream", ""));

    size = size < size1 ? size : size1;
    size = size < size2 ? size : size2;

    interp = MYFMOD(interp, 1.0);
    interp1 = 1. - interp;
    interp2 = interp;

    for (i = 0; i < size; i++)
    {
        self->buffer[i] = tab1[i] * interp1 + tab2[i] * interp2;
    }

    TableStream_recordChunk((TableStream *)self->table, self->buffer, size);
}

static int
TableMorph_traverse(TableMorph *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->sources);
    return 0;
}

static int
TableMorph_clear(TableMorph *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->sources);
    return 0;
}

static void
TableMorph_dealloc(TableMorph* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->buffer);
    TableMorph_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
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

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableMorph must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

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
static PyObject * TableMorph_stop(TableMorph *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
TableMorph_setTable(TableMorph *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyObject *
TableMorph_setSources(TableMorph *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (! PyList_Check(arg))
    {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyLong_FromLong(-1);
    }

    Py_INCREF(arg);
    Py_DECREF(self->sources);
    self->sources = arg;

    Py_RETURN_NONE;
}

static PyMemberDef TableMorph_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TableMorph, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TableMorph, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TableMorph, input), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(TableMorph, table), 0, NULL},
    {"sources", T_OBJECT_EX, offsetof(TableMorph, sources), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TableMorph_methods[] =
{
    {"getServer", (PyCFunction)TableMorph_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TableMorph_getStream, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)TableMorph_setTable, METH_O, NULL},
    {"setSources", (PyCFunction)TableMorph_setSources, METH_O, NULL},
    {"play", (PyCFunction)TableMorph_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TableMorph_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject TableMorphType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TableMorph_base",         /*tp_name*/
    sizeof(TableMorph),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TableMorph_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TableMorph_traverse,   /* tp_traverse */
    (inquiry)TableMorph_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *trigger;
    Stream *trigger_stream;
    PyObject *table;
    T_SIZE_T pointer;
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
    int i, j, num;
    MYFLT val;
    T_SIZE_T upBound;
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trig = Stream_getData((Stream *)self->trigger_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    if (self->active == 1)
    {
        if ((size - self->pointer) >= self->bufsize)
            num = self->bufsize;
        else
        {
            num = size - self->pointer;

            if (self->active == 1)
            {
                if (num <= 0)
                    self->trigsBuffer[0] = 1.0;
                else
                    self->trigsBuffer[num - 1] = 1.0;

                self->active = 0;
            }
        }

        if (self->pointer < size)
        {
            upBound = size - self->fadeInSample;

            MYFLT buffer[num];
            memset(&buffer, 0, sizeof(buffer));

            for (i = 0; i < num; i++)
            {
                if (self->pointer < self->fadeInSample)
                    val = self->pointer / self->fadeInSample;
                else if (self->pointer > upBound)
                    val = (size - self->pointer) / self->fadeInSample;
                else
                    val = 1.;

                buffer[i] = in[i] * val;
                self->time_buffer_streams[i] = self->pointer++;
            }

            TableStream_recordChunk((TableStream *)self->table, buffer, num);

            if (num < self->bufsize)
            {
                for (i = num; i < self->bufsize; i++)
                {
                    self->time_buffer_streams[i] = self->pointer;
                }
            }
        }
    }
    else
    {
        for (j = 0; j < self->bufsize; j++)
        {
            self->time_buffer_streams[j] = self->pointer;

            if (trig[j] == 1.0)
            {
                self->active = 1;
                self->pointer = 0;
                TableStream_resetRecordingPointer((TableStream *)self->table);

                if (size >= self->bufsize)
                    num = self->bufsize - j;
                else
                {
                    num = size < (self->bufsize - j) ? size : (self->bufsize - j);

                    if (self->active == 1)
                    {
                        if (num <= 0)
                            self->trigsBuffer[0] = 1.0;
                        else
                            self->trigsBuffer[num - 1] = 1.0;

                        self->active = 0;
                    }
                }

                upBound = size - self->fadeInSample;

                MYFLT buffer[num];
                memset(&buffer, 0, sizeof(buffer));

                for (i = 0; i < num; i++)
                {
                    if (self->pointer < self->fadeInSample)
                    {
                        val = self->pointer / self->fadeInSample;
                    }
                    else if (self->pointer > upBound)
                        val = (size - self->pointer) / self->fadeInSample;
                    else
                        val = 1.;

                    buffer[i] = in[i + j] * val;
                    self->time_buffer_streams[i + j] = self->pointer++;
                }

                TableStream_recordChunk((TableStream *)self->table, buffer, num);

                if (num < (self->bufsize - j))
                {
                    for (i = num; i < (self->bufsize - j); i++)
                    {
                        self->time_buffer_streams[i + j] = self->pointer;
                    }
                }

                break;
            }
        }
    }
}

static MYFLT *
TrigTableRec_getTimeBuffer(TrigTableRec *self)
{
    return self->time_buffer_streams;
}

static int
TrigTableRec_traverse(TrigTableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->trigger);
    return 0;
}

static int
TrigTableRec_clear(TrigTableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->trigger);
    return 0;
}

static void
TrigTableRec_dealloc(TrigTableRec* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->trigsBuffer);
    PyMem_RawFree(self->time_buffer_streams);
    TrigTableRec_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
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

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TrigTableRec must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->time_buffer_streams = (MYFLT *)PyMem_RawRealloc(self->time_buffer_streams, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = self->time_buffer_streams[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);
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
static PyObject * TrigTableRec_stop(TrigTableRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
TrigTableRec_setTable(TrigTableRec *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyMemberDef TrigTableRec_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigTableRec, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRec, stream), 0, NULL},
    {"trig_stream", T_OBJECT_EX, offsetof(TrigTableRec, trig_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TrigTableRec, input), 0, NULL},
    {"trigger", T_OBJECT_EX, offsetof(TrigTableRec, trigger), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(TrigTableRec, table), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRec_methods[] =
{
    {"getServer", (PyCFunction)TrigTableRec_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TrigTableRec_getStream, METH_NOARGS, NULL},
    {"_getTriggerStream", (PyCFunction)TrigTableRec_getTriggerStream, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)TrigTableRec_setTable, METH_O, NULL},
    {"play", (PyCFunction)TrigTableRec_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TrigTableRec_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject TrigTableRecType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TrigTableRec_base",         /*tp_name*/
    sizeof(TrigTableRec),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigTableRec_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TrigTableRec_traverse,   /* tp_traverse */
    (inquiry)TrigTableRec_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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

typedef struct
{
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
TrigTableRecTimeStream_setProcMode(TrigTableRecTimeStream *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
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

    for (i = 0; i < self->bufsize; i++)
    {
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
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TrigTableRecTimeStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp = NULL;
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
static PyObject * TrigTableRecTimeStream_stop(TrigTableRecTimeStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * TrigTableRecTimeStream_multiply(TrigTableRecTimeStream *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigTableRecTimeStream_inplace_multiply(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigTableRecTimeStream_add(TrigTableRecTimeStream *self, PyObject *arg) { ADD };
static PyObject * TrigTableRecTimeStream_inplace_add(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigTableRecTimeStream_sub(TrigTableRecTimeStream *self, PyObject *arg) { SUB };
static PyObject * TrigTableRecTimeStream_inplace_sub(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigTableRecTimeStream_div(TrigTableRecTimeStream *self, PyObject *arg) { DIV };
static PyObject * TrigTableRecTimeStream_inplace_div(TrigTableRecTimeStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef TrigTableRecTimeStream_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, stream), 0, NULL},
    {"mul", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, mul), 0, NULL},
    {"add", T_OBJECT_EX, offsetof(TrigTableRecTimeStream, add), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TrigTableRecTimeStream_methods[] =
{
    {"getServer", (PyCFunction)TrigTableRecTimeStream_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TrigTableRecTimeStream_getStream, METH_NOARGS, NULL},
    {"play", (PyCFunction)TrigTableRecTimeStream_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"out", (PyCFunction)TrigTableRecTimeStream_out, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TrigTableRecTimeStream_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setMul", (PyCFunction)TrigTableRecTimeStream_setMul, METH_O, NULL},
    {"setAdd", (PyCFunction)TrigTableRecTimeStream_setAdd, METH_O, NULL},
    {"setSub", (PyCFunction)TrigTableRecTimeStream_setSub, METH_O, NULL},
    {"setDiv", (PyCFunction)TrigTableRecTimeStream_setDiv, METH_O, NULL},
    {NULL}  /* Sentinel */
};

static PyNumberMethods TrigTableRecTimeStream_as_number =
{
    (binaryfunc)TrigTableRecTimeStream_add,                         /*nb_add*/
    (binaryfunc)TrigTableRecTimeStream_sub,                         /*nb_subtract*/
    (binaryfunc)TrigTableRecTimeStream_multiply,                    /*nb_multiply*/
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
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    (binaryfunc)TrigTableRecTimeStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)TrigTableRecTimeStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)TrigTableRecTimeStream_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)TrigTableRecTimeStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)TrigTableRecTimeStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject TrigTableRecTimeStreamType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TrigTableRecTimeStream_base",         /*tp_name*/
    sizeof(TrigTableRecTimeStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TrigTableRecTimeStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TrigTableRecTimeStream_traverse,   /* tp_traverse */
    (inquiry)TrigTableRecTimeStream_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *table;
    T_SIZE_T pointer;
    int active;
    MYFLT last_value;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} TablePut;

static void
TablePut_compute_next_data_frame(TablePut *self)
{
    int i;
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        self->trigsBuffer[i] = 0.0;
    }

    if (self->active == 1)
    {
        for (i = 0; i < self->bufsize; i++)
        {
            if (in[i] != self->last_value)
            {
                self->last_value = in[i];
                TableStream_record((TableStream *)self->table, self->pointer++, self->last_value);

                if (self->pointer >= size)
                {
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
    return 0;
}

static int
TablePut_clear(TablePut *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
TablePut_dealloc(TablePut* self)
{
    pyo_DEALLOC
    PyMem_RawFree(self->trigsBuffer);
    TablePut_clear(self);
    Py_TYPE(self->trig_stream)->tp_free((PyObject*)self->trig_stream);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
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

    static char *kwlist[] = {"input", "table", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &tabletmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TablePut must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)PyMem_RawRealloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i = 0; i < self->bufsize; i++)
    {
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

static PyObject * TablePut_stop(TablePut *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
TablePut_setTable(TablePut *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyMemberDef TablePut_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TablePut, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TablePut, stream), 0, NULL},
    {"trig_stream", T_OBJECT_EX, offsetof(TablePut, trig_stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TablePut, input), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(TablePut, table), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TablePut_methods[] =
{
    {"getServer", (PyCFunction)TablePut_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TablePut_getStream, METH_NOARGS, NULL},
    {"_getTriggerStream", (PyCFunction)TablePut_getTriggerStream, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)TablePut_setTable, METH_O, NULL},
    {"play", (PyCFunction)TablePut_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TablePut_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject TablePutType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TablePut_base",         /*tp_name*/
    sizeof(TablePut),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TablePut_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TablePut_traverse,   /* tp_traverse */
    (inquiry)TablePut_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *pos;
    Stream *pos_stream;
    PyObject *table;
    int mode;
    int maxwindow;
    T_SIZE_T lastPos;
    MYFLT lastValue;
    T_SIZE_T count;
    MYFLT accum;
    MYFLT valInTable;
} TableWrite;

static void
TableWrite_compute_next_data_frame(TableWrite *self)
{
    T_SIZE_T i, j, ipos;

    MYFLT feed = TableStream_getFeedback((TableStream *)self->table);
    MYFLT *tablelist = TableStream_getData((TableStream *)self->table);
    T_SIZE_T size = TableStream_getSize((TableStream *)self->table);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->mode == 0)
            ipos = (T_SIZE_T)(pos[i] * size + 0.5);
        else
            ipos = (T_SIZE_T)(pos[i] + 0.5);

        if (ipos >= 0 && ipos < size)
        {
            if (self->lastPos < 0)   /* Init case. */
            {
                self->valInTable = tablelist[ipos];
                self->count = 1;
                self->accum = in[i];
                tablelist[ipos] = in[i] + tablelist[ipos] * feed;
            }
            else if (ipos == self->lastPos)   /* Same position, average inputs. */
            {
                self->count += 1;
                self->accum += in[i];
                tablelist[ipos] = self->accum / self->count + self->valInTable * feed;
            }
            else   /* Position changed. */
            {
                T_SIZE_T steps, dir;

                if (ipos > self->lastPos)   /* Move forward. */
                {
                    steps = ipos - self->lastPos;

                    if (steps > self->maxwindow)
                        steps = 1;

                    dir = 1;
                }
                else   /* Move backward. */
                {
                    steps = self->lastPos - ipos;

                    if (steps > self->maxwindow)
                        steps = 1;

                    dir = -1;
                }

                self->valInTable = tablelist[ipos];
                self->count = 1;
                self->accum = in[i];

                if (steps < 2)   /* Moved one sample, no need to interpolate. */
                {
                    tablelist[ipos] = in[i] + tablelist[ipos] * feed;
                }
                else   /* Interpolate between last pos and current pos. */
                {
                    MYFLT inc = (in[i] - self->lastValue) / steps;

                    for (j = 1; j <= steps; j++)
                    {
                        MYFLT val = self->lastValue + inc * j;
                        tablelist[self->lastPos + j * dir] = val + tablelist[self->lastPos + j * dir] * feed;
                    }
                }
            }

            self->lastPos = ipos;
            self->lastValue = in[i];
        }
    }
}

static int
TableWrite_traverse(TableWrite *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->pos);
    return 0;
}

static int
TableWrite_clear(TableWrite *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->pos);
    return 0;
}

static void
TableWrite_dealloc(TableWrite* self)
{
    pyo_DEALLOC
    TableWrite_clear(self);
    Py_TYPE(self->stream)->tp_free((PyObject*)self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
TableWrite_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *postmp, *tabletmp;
    TableWrite *self;
    self = (TableWrite *)type->tp_alloc(type, 0);

    self->pos = PyFloat_FromDouble(0.0);

    self->mode = 0;
    self->maxwindow = 1024;
    self->lastPos = -1;
    self->lastValue = 0.0;
    self->count = 0;
    self->accum = 0.0;
    self->valInTable = 0.0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableWrite_compute_next_data_frame);

    static char *kwlist[] = {"input", "pos", "table", "mode", "maxwindow", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOOii", kwlist, &inputtmp, &postmp, &tabletmp, &self->mode, &self->maxwindow))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (postmp)
    {
        PyObject_CallMethod((PyObject *)self, "setPos", "O", postmp);
    }

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 )
    {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of TableWrite must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
}

static PyObject * TableWrite_getServer(TableWrite* self) { GET_SERVER };
static PyObject * TableWrite_getStream(TableWrite* self) { GET_STREAM };

static PyObject * TableWrite_play(TableWrite *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * TableWrite_stop(TableWrite *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
TableWrite_setPos(TableWrite *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    if (PyObject_HasAttrString((PyObject *)arg, "server") == 0)
    {
        PyErr_SetString(PyExc_TypeError, "\"pos\" argument of TableWrite must be a PyoObject.\n");
        Py_RETURN_NONE;
    }

    Py_DECREF(self->pos);

    self->pos = arg;
    Py_INCREF(self->pos);
    PyObject *streamtmp = PyObject_CallMethod((PyObject *)self->pos, "_getStream", NULL);
    self->pos_stream = (Stream *)streamtmp;
    Py_INCREF(self->pos_stream);

    Py_RETURN_NONE;
}

static PyObject *
TableWrite_setTable(TableWrite *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)arg, "getTableStream", "");

    Py_RETURN_NONE;
}

static PyMemberDef TableWrite_members[] =
{
    {"server", T_OBJECT_EX, offsetof(TableWrite, server), 0, NULL},
    {"stream", T_OBJECT_EX, offsetof(TableWrite, stream), 0, NULL},
    {"input", T_OBJECT_EX, offsetof(TableWrite, input), 0, NULL},
    {"table", T_OBJECT_EX, offsetof(TableWrite, table), 0, NULL},
    {"pos", T_OBJECT_EX, offsetof(TableWrite, pos), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef TableWrite_methods[] =
{
    {"getServer", (PyCFunction)TableWrite_getServer, METH_NOARGS, NULL},
    {"_getStream", (PyCFunction)TableWrite_getStream, METH_NOARGS, NULL},
    {"setTable", (PyCFunction)TableWrite_setTable, METH_O, NULL},
    {"setPos", (PyCFunction)TableWrite_setPos, METH_O, NULL},
    {"play", (PyCFunction)TableWrite_play, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop", (PyCFunction)TableWrite_stop, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject TableWriteType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.TableWrite_base",         /*tp_name*/
    sizeof(TableWrite),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)TableWrite_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    0,           /* tp_doc */
    (traverseproc)TableWrite_traverse,   /* tp_traverse */
    (inquiry)TableWrite_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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

/*************************/
/* SharedTable structure */
/*************************/
typedef struct
{
    pyo_table_HEAD
    char *name;
    int create;
    int fd;
    double sr;
} SharedTable;

static int
SharedTable_traverse(SharedTable *self, visitproc visit, void *arg)
{
    pyo_table_VISIT
    return 0;
}

static int
SharedTable_clear(SharedTable *self)
{
    pyo_table_CLEAR
    return 0;
}

static void
SharedTable_dealloc(SharedTable* self)
{
#if !defined(_WIN32) && !defined(_WIN64)
    close(self->fd);

    if (self->create)
        shm_unlink(self->name);

#endif
    SharedTable_clear(self);
    Py_TYPE(self->tablestream)->tp_free((PyObject*)self->tablestream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SharedTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    T_SIZE_T i;
    SharedTable *self;
    self = (SharedTable *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    static char *kwlist[] = {"name", "create", "size", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "sin", kwlist, &self->name, &self->create, &self->size))
        Py_RETURN_NONE;

#if !defined(_WIN32) && !defined(_WIN64)

    /* Open shared memory object. */
    if (self->create)
    {
        self->fd = shm_open(self->name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

        if (self->fd == -1)
        {
            PySys_WriteStdout("SharedTable: failed to create shared memory.\n");
            Py_RETURN_NONE;
        }

        if (ftruncate(self->fd, sizeof(MYFLT) * (self->size + 1)) == -1)
        {
            PySys_WriteStdout("SharedTable: failed to truncate shared memory.\n");
            close(self->fd);
            shm_unlink(self->name);
            Py_RETURN_NONE;
        }
    }
    else
    {
        self->fd = shm_open(self->name, O_RDWR, 0);

        if (self->fd == -1)
        {
            PySys_WriteStdout("SharedTable: failed to create shared memory.\n");
            Py_RETURN_NONE;
        }
    }

    /* Map shared memory object. */
    self->data = mmap(NULL, sizeof(MYFLT) * (self->size + 1),
                      PROT_READ | PROT_WRITE, MAP_SHARED, self->fd, 0);

    if (self->data == MAP_FAILED)
    {
        PySys_WriteStdout("SharedTable: failed to mmap shared memory.\n");
        close(self->fd);

        if (self->create)
            shm_unlink(self->name);

        Py_RETURN_NONE;
    }

    /* Initialize the memory. */
    if (self->create)
    {
        for (i = 0; i <= self->size; i++)
        {
            self->data[i] = 0.0;
        }
    }

#endif

    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);

    PyObject *srobj = PyObject_CallMethod(self->server, "getSamplingRate", NULL);
    self->sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    TableStream_setSamplingRate(self->tablestream, self->sr);

    return (PyObject *)self;
}

static PyObject * SharedTable_getServer(SharedTable* self) { GET_SERVER };
static PyObject * SharedTable_getTableStream(SharedTable* self) { GET_TABLE_STREAM };
static PyObject * SharedTable_setData(SharedTable *self, PyObject *arg) { SET_TABLE_DATA };
static PyObject * SharedTable_normalize(SharedTable *self, PyObject *args, PyObject *kwds) { NORMALIZE };
static PyObject * SharedTable_reset(SharedTable *self) { TABLE_RESET };
static PyObject * SharedTable_removeDC(SharedTable *self) { REMOVE_DC };
static PyObject * SharedTable_reverse(SharedTable *self) { REVERSE };
static PyObject * SharedTable_invert(SharedTable *self) { INVERT };
static PyObject * SharedTable_rectify(SharedTable *self) { RECTIFY };
static PyObject * SharedTable_bipolarGain(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_BIPOLAR_GAIN };
static PyObject * SharedTable_lowpass(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_LOWPASS };
static PyObject * SharedTable_fadein(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEIN };
static PyObject * SharedTable_fadeout(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_FADEOUT };
static PyObject * SharedTable_pow(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_POWER };
static PyObject * SharedTable_copy(SharedTable *self, PyObject *arg) { COPY };
static PyObject * SharedTable_copyData(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_COPYDATA };
static PyObject * SharedTable_rotate(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_ROTATE };
static PyObject * SharedTable_setTable(SharedTable *self, PyObject *arg) { SET_TABLE };
static PyObject * SharedTable_getTable(SharedTable *self) { GET_TABLE };
static PyObject * SharedTable_getRate(SharedTable *self) { TABLE_GET_RATE };
static PyObject * SharedTable_put(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_PUT };
static PyObject * SharedTable_get(SharedTable *self, PyObject *args, PyObject *kwds) { TABLE_GET };
static PyObject * SharedTable_add(SharedTable *self, PyObject *arg) { TABLE_ADD };
static PyObject * SharedTable_sub(SharedTable *self, PyObject *arg) { TABLE_SUB };
static PyObject * SharedTable_mul(SharedTable *self, PyObject *arg) { TABLE_MUL };
static PyObject * SharedTable_div(SharedTable *self, PyObject *arg) { TABLE_DIV };

static PyObject *
SharedTable_getSize(SharedTable *self)
{
    return PyLong_FromLong(self->size);
};

static PyMemberDef SharedTable_members[] =
{
    {"server", T_OBJECT_EX, offsetof(SharedTable, server), 0, NULL},
    {"tablestream", T_OBJECT_EX, offsetof(SharedTable, tablestream), 0, NULL},
    {NULL}  /* Sentinel */
};

static PyMethodDef SharedTable_methods[] =
{
    {"getServer", (PyCFunction)SharedTable_getServer, METH_NOARGS, NULL},
    {"copy", (PyCFunction)SharedTable_copy, METH_O, NULL},
    {"copyData", (PyCFunction)SharedTable_copyData, METH_VARARGS | METH_KEYWORDS, NULL},
    {"rotate", (PyCFunction)SharedTable_rotate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"setTable", (PyCFunction)SharedTable_setTable, METH_O, NULL},
    {"getTable", (PyCFunction)SharedTable_getTable, METH_NOARGS, NULL},
    {"getTableStream", (PyCFunction)SharedTable_getTableStream, METH_NOARGS, NULL},
    {"setData", (PyCFunction)SharedTable_setData, METH_O, NULL},
    {"normalize", (PyCFunction)SharedTable_normalize, METH_VARARGS | METH_KEYWORDS, NULL},
    {"reset", (PyCFunction)SharedTable_reset, METH_NOARGS, NULL},
    {"removeDC", (PyCFunction)SharedTable_removeDC, METH_NOARGS, NULL},
    {"reverse", (PyCFunction)SharedTable_reverse, METH_NOARGS, NULL},
    {"invert", (PyCFunction)SharedTable_invert, METH_NOARGS, NULL},
    {"rectify", (PyCFunction)SharedTable_rectify, METH_NOARGS, NULL},
    {"bipolarGain", (PyCFunction)SharedTable_bipolarGain, METH_VARARGS | METH_KEYWORDS, NULL},
    {"lowpass", (PyCFunction)SharedTable_lowpass, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadein", (PyCFunction)SharedTable_fadein, METH_VARARGS | METH_KEYWORDS, NULL},
    {"fadeout", (PyCFunction)SharedTable_fadeout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pow", (PyCFunction)SharedTable_pow, METH_VARARGS | METH_KEYWORDS, NULL},
    {"put", (PyCFunction)SharedTable_put, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get", (PyCFunction)SharedTable_get, METH_VARARGS | METH_KEYWORDS, NULL},
    {"getSize", (PyCFunction)SharedTable_getSize, METH_NOARGS, NULL},
    {"getRate", (PyCFunction)SharedTable_getRate, METH_NOARGS, NULL},
    {"add", (PyCFunction)SharedTable_add, METH_O, NULL},
    {"sub", (PyCFunction)SharedTable_sub, METH_O, NULL},
    {"mul", (PyCFunction)SharedTable_mul, METH_O, NULL},
    {"div", (PyCFunction)SharedTable_div, METH_O, NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject SharedTableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SharedTable_base",         /*tp_name*/
    sizeof(SharedTable),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SharedTable_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    0,  /* tp_doc */
    (traverseproc)SharedTable_traverse,   /* tp_traverse */
    (inquiry)SharedTable_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    SharedTable_methods,             /* tp_methods */
    SharedTable_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SharedTable_new,                 /* tp_new */
};
