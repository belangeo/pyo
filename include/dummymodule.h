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
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add
} Dummy;

extern PyObject * Dummy_initialize(Dummy *self);

#define MAKE_NEW_DUMMY(self, type, rt_error)	\
(self) = (Dummy *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }
