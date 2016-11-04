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
#include "pyomodule.h"

#define __PV_STREAM_MODULE
#include "pvstreammodule.h"
#undef __PV_STREAM_MODULE

/************************/
/* PVStream object */
/************************/
static void
PVStream_dealloc(PVStream* self)
{
    self->magn = NULL;
    self->freq = NULL;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

int
PVStream_getFFTsize(PVStream *self)
{
    return self->fftsize;
}

int
PVStream_getOlaps(PVStream *self)
{
    return self->olaps;
}

MYFLT **
PVStream_getMagn(PVStream *self)
{
    return (MYFLT **)self->magn;
}

MYFLT **
PVStream_getFreq(PVStream *self)
{
    return (MYFLT **)self->freq;
}

int *
PVStream_getCount(PVStream *self)
{
    return (int *)self->count;
}

void
PVStream_setFFTsize(PVStream *self, int fftsize)
{
    self->fftsize = fftsize;
}

void
PVStream_setOlaps(PVStream *self, int olaps)
{
    self->olaps = olaps;
}

void
PVStream_setMagn(PVStream *self, MYFLT **data)
{
    self->magn = data;
}

void
PVStream_setFreq(PVStream *self, MYFLT **data)
{
    self->freq = data;
}

void
PVStream_setCount(PVStream *self, int *data)
{
    self->count = data;
}

PyTypeObject PVStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pyo.PVStream", /*tp_name*/
    sizeof(PVStream), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)PVStream_dealloc, /*tp_dealloc*/
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
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "\n\
    Phase Vocoder stream object. For internal use only. \n\n\
    ", /* tp_doc */
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
    0, /* tp_new */
};