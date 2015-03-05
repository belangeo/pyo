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
#include "pyomodule.h"

typedef struct {
    PyObject_HEAD
    int fftsize;
    int olaps;
    MYFLT **magn;
    MYFLT **freq;
    int *count;
} PVStream;

extern int PVStream_getFFTsize(PVStream *self);
extern int PVStream_getOlaps(PVStream *self);
extern MYFLT ** PVStream_getMagn(PVStream *self);
extern MYFLT ** PVStream_getFreq(PVStream *self);
extern int * PVStream_getCount(PVStream *self);
extern void PVStream_setFFTsize(PVStream * self, int fftsize);
extern void PVStream_setOlaps(PVStream * self, int olaps);
extern void PVStream_setMagn(PVStream * self, MYFLT **data);
extern void PVStream_setFreq(PVStream * self, MYFLT **data);
extern void PVStream_setCount(PVStream * self, int *data);
extern PyTypeObject PVStreamType;

#define MAKE_NEW_PV_STREAM(self, type, rt_error) \
    (self) = (PVStream *)(type)->tp_alloc((type), 0); \
    if ((self) == rt_error) { return rt_error; } \
 \
    (self)->fftsize = 1024; \
    (self)->olaps = 4;

#ifdef __PV_STREAM_MODULE
/* include from pvstream.c */

#else
/* include from other modules to use API */

// #define Stream_setStreamObject(op, v) (((Stream *)(op))->streamobject = (v))

#endif
/* __PVSTREAMMODULE */