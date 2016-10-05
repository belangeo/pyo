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

#include "Python.h"
#include "pyomodule.h"

#ifdef __TABLE_MODULE

typedef struct {
    PyObject_HEAD
    int size;
    double samplingRate;
    MYFLT *data;
    Py_ssize_t shape[1]; /* 1-dimension array (must be set to table size) needed by the buffer protocol. */
} TableStream;


#define MAKE_NEW_TABLESTREAM(self, type, rt_error)	\
(self) = (TableStream *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }	\
\
(self)->size = 0

#else

int TableStream_getSize(PyObject *self);
double TableStream_getSamplingRate(PyObject *self);
MYFLT * TableStream_getData(PyObject *self);
extern PyTypeObject TableStreamType;

#endif