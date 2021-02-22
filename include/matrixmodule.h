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

#ifndef _MATRIXMODULE_H
#define _MATRIXMODULE_H

#include "Python.h"
#include "pyomodule.h"

typedef struct
{
    PyObject_HEAD
    int width;
    int height;
    MYFLT **data;
} MatrixStream;


#define MAKE_NEW_MATRIXSTREAM(self, type, rt_error) \
(self) = (MatrixStream *)(type)->tp_alloc((type), 0);   \
if ((self) == rt_error) { return rt_error; }    \
\
(self)->width = (self)->height = 0


extern int MatrixStream_getWidth(MatrixStream *self);
extern int MatrixStream_getHeight(MatrixStream *self);
extern MYFLT MatrixStream_getPointFromPos(MatrixStream *self, long x, long y);
extern MYFLT MatrixStream_getInterpPointFromPos(MatrixStream *self, MYFLT x, MYFLT y);
extern PyTypeObject MatrixStreamType;

#endif // _MATRIXMODULE_H