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
#include "servermodule.h"

#define __MATRIX_MODULE
#include "matrixmodule.h"
#undef __MATRIX_MODULE

/*************************/
/* MatrixStream structure */
/*************************/
static void
MatrixStream_dealloc(MatrixStream* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
MatrixStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MatrixStream *self;
    MAKE_NEW_MATRIXSTREAM(self, type, NULL);
    return (PyObject *)self;
}

int
MatrixStream_getRowSize(MatrixStream *self)
{
    return self->rowsize;
}

int
MatrixStream_getColSize(MatrixStream *self)
{
    return self->colsize;
}

float **
MatrixStream_getData(MatrixStream *self)
{
    return (float **)self->data;
}    

void
MatrixStream_setData(MatrixStream *self, float **data)
{
    self->data = data;
}    

void
MatrixStream_setRowSize(MatrixStream *self, int size)
{
    self->rowsize = size;
}    

void
MatrixStream_setColSize(MatrixStream *self, int size)
{
    self->colsize = size;
}    

PyTypeObject MatrixStreamType = {
PyObject_HEAD_INIT(NULL)
0, /*ob_size*/
"_pyo.MatrixStream", /*tp_name*/
sizeof(MatrixStream), /*tp_basicsize*/
0, /*tp_itemsize*/
(destructor)MatrixStream_dealloc, /*tp_dealloc*/
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
"MatrixStream objects. For internal use only. Must never be instantiated by the user.", /* tp_doc */
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
MatrixStream_new, /* tp_new */
};

/***********************/
/* NewMatrix structure */
/***********************/
typedef struct {
    pyo_matrix_HEAD
    int row_pointer;
    int col_pointer;
} NewMatrix;

static PyObject *
NewMatrix_recordChunk(NewMatrix *self, float *data, int datasize)
{
    int i;

/*    for (i=0; i<datasize; i++) {
        self->data[self->pointer++] = data[i];
        if (self->pointer == self->size)
            self->pointer = 0;
    } */
    Py_INCREF(Py_None);
    return Py_None;
}

static int
NewMatrix_traverse(NewMatrix *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->matrixstream);
    return 0;
}

static int 
NewMatrix_clear(NewMatrix *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->matrixstream);
    return 0;
}

static void
NewMatrix_dealloc(NewMatrix* self)
{
    free(self->data);
    NewMatrix_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
NewMatrix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NewMatrix *self;
    
    self = (NewMatrix *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->row_pointer = self->col_pointer = 0;
    
    MAKE_NEW_MATRIXSTREAM(self->matrixstream, &MatrixStreamType, NULL);
    
    return (PyObject *)self;
}

static int
NewMatrix_init(NewMatrix *self, PyObject *args, PyObject *kwds)
{    
    int i, j;
    PyObject *inittmp=NULL;
    static char *kwlist[] = {"rows", "cols", "init", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii|O", kwlist, &self->rowsize, &self->colsize, &inittmp))
        return -1; 

    self->data = (float **)realloc(self->data, (self->rowsize + 1) * sizeof(float));

    for (i=0; i<(self->rowsize+1); i++) {
        self->data[i] = (float *)malloc((self->colsize + 1) * sizeof(float));
    }

    for(i=0; i<(self->rowsize+1); i++) {
        for (j=0; j<(self->colsize+1); j++) {
            self->data[i][j] = 0.0;
        }    
    }
    
    if (inittmp) {
        PyObject_CallMethod((PyObject *)self, "setMatrix", "O", inittmp);
    }
    
    MatrixStream_setRowSize(self->matrixstream, self->rowsize);
    MatrixStream_setColSize(self->matrixstream, self->colsize);
    MatrixStream_setData(self->matrixstream, self->data);

    Py_INCREF(self);
    return 0;
}

static PyObject * NewMatrix_getServer(NewMatrix* self) { GET_SERVER };
static PyObject * NewMatrix_getMatrixStream(NewMatrix* self) { GET_MATRIX_STREAM };
//static PyObject * NewMatrix_setData(NewMatrix *self, PyObject *arg) { SET_TABLE_DATA };
//static PyObject * NewMatrix_normalize(NewMatrix *self) { NORMALIZE };

static PyObject *
NewMatrix_getSize(NewMatrix *self)
{
    return Py_BuildValue("(ii)", self->rowsize, self->colsize);
};

static PyObject *
NewMatrix_getRate(NewMatrix *self)
{
    float sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr / self->rowsize);
};

static PyObject *
NewMatrix_getMatrix(NewMatrix *self)
{
    int i, j;
    PyObject *matrix, *samples;
    
    matrix = PyList_New(self->rowsize);
    for(i=0; i<self->rowsize; i++) {
        samples = PyList_New(self->colsize);
        for (j=0; j<self->colsize; j++) {
            PyList_SetItem(samples, j, PyFloat_FromDouble(self->data[i][j]));
        }    
        PyList_SetItem(matrix, i, samples);
    }
    
    return matrix;
};

static PyObject *
NewMatrix_setMatrix(NewMatrix *self, PyObject *value)
{
    int i, j;
    PyObject *innerlist;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The matrix value value must be a list.");
        return PyInt_FromLong(-1);
    }

    for(i=0; i<self->rowsize; i++) {
        innerlist = PyList_GetItem(value, i);
        for (j=0; j<self->colsize; j++) {
            self->data[i][j] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(innerlist, j)));
        }    
    }
    Py_INCREF(Py_None);
    return Py_None;    
}

static PyMemberDef NewMatrix_members[] = {
{"server", T_OBJECT_EX, offsetof(NewMatrix, server), 0, "Pyo server."},
{"matrixstream", T_OBJECT_EX, offsetof(NewMatrix, matrixstream), 0, "Matrix stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef NewMatrix_methods[] = {
{"getServer", (PyCFunction)NewMatrix_getServer, METH_NOARGS, "Returns server object."},
{"getMatrix", (PyCFunction)NewMatrix_getMatrix, METH_NOARGS, "Returns a list of matrix samples."},
{"getMatrixStream", (PyCFunction)NewMatrix_getMatrixStream, METH_NOARGS, "Returns matrixstream object created by this matrix."},
{"setMatrix", (PyCFunction)NewMatrix_setMatrix, METH_O, "Sets the matrix from a list of list of floats."},
//{"setData", (PyCFunction)NewMatrix_setData, METH_O, "Sets the table from samples in a text file."},
//{"normalize", (PyCFunction)NewMatrix_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"getSize", (PyCFunction)NewMatrix_getSize, METH_NOARGS, "Return the size of the matrix in samples."},
{"getRate", (PyCFunction)NewMatrix_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject NewMatrixType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.NewMatrix_base",         /*tp_name*/
sizeof(NewMatrix),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)NewMatrix_dealloc, /*tp_dealloc*/
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
"NewMatrix objects. Generates an empty matrix.",  /* tp_doc */
(traverseproc)NewMatrix_traverse,   /* tp_traverse */
(inquiry)NewMatrix_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
NewMatrix_methods,             /* tp_methods */
NewMatrix_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)NewMatrix_init,      /* tp_init */
0,                         /* tp_alloc */
NewMatrix_new,                 /* tp_new */
};


