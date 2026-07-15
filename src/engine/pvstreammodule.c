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
#include "pvstreammodule.h"

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

static PyType_Slot PVStreamType_slots[] =
{
    {Py_tp_dealloc, PVStream_dealloc},
    {Py_tp_doc, "\n\
    Phase Vocoder stream object. For internal use only. \n\n\
    "},
    {Py_tp_new, PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec PVStreamType_spec =
{
    "pyo.PVStream",
    sizeof(PVStream),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    PVStreamType_slots
};

PyTypeObject *
PyoCreatePVStreamType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &PVStreamType_spec, NULL);
}
