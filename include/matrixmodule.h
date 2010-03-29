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

#include "Python.h"

#ifdef __MATRIX_MODULE

typedef struct {
    PyObject_HEAD
    int rowsize;
    int colsize;
    float *data;
} MatrixStream;


#define MAKE_NEW_MATRIXSTREAM(self, type, rt_error)	\
(self) = (MatrixStream *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }	\
\
(self)->rowsize = (self)->colsize = 0

#else

int MatrixStream_getRowSize(PyObject *self);
int MatrixStream_getColSize(PyObject *self);
float * MatrixStream_getData(PyObject *self);
extern PyTypeObject MatrixStreamType;

#endif
