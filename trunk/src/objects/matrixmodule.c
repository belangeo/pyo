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

float *
MatrixStream_getData(MatrixStream *self)
{
    return (float *)self->data;
}    

void
MatrixStream_setData(MatrixStream *self, float *data)
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



