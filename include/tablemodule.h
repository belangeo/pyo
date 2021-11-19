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

#ifndef _TABLEMODULE_H
#define _TABLEMODULE_H

#include "Python.h"
#include "pyomodule.h"

typedef struct
{
    PyObject_HEAD
    T_SIZE_T size;
    double samplingRate;
    MYFLT *data;
    Py_ssize_t shape[1]; /* 1-dimension array (must be set to table size) needed by the buffer protocol. */
    T_SIZE_T pointer; /* writing pointer. */
    MYFLT feedback; /* Recording feedback. */ 
} TableStream;


#define MAKE_NEW_TABLESTREAM(self, type, rt_error)  \
(self) = (TableStream *)(type)->tp_alloc((type), 0);    \
if ((self) == rt_error) { return rt_error; }    \
\
(self)->size = 0; \
(self)->pointer = 0; \
(self)->feedback = 0.0


extern T_SIZE_T TableStream_getSize(TableStream *self);
extern double TableStream_getSamplingRate(TableStream *self);
extern MYFLT * TableStream_getData(TableStream *self);
extern void TableStream_recordChunk(TableStream *self, MYFLT *data, T_SIZE_T datasize);
extern void TableStream_resetRecordingPointer(TableStream *self);
extern void TableStream_setFeedback(TableStream *self, MYFLT feedback);
extern MYFLT TableStream_getFeedback(TableStream *self);
extern void TableStream_record(TableStream *self, int pos, MYFLT value);
extern PyTypeObject TableStreamType;

#endif // _TABLEMODULE_H