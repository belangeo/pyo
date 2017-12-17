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

// This header includes some compatibility code between python 2 and python 3

#include "Python.h"

#ifndef _PY_2TO3_H
#define _PY_2TO3_H


#if PY_MAJOR_VERSION >= 3
/* This is the default on Python3 and constant has been removed. */
#define Py_TPFLAGS_CHECKTYPES 0
/* This seems to be a Python 2 thing only for compatibility with even older versions of Python.
 * It has been removed in Python 3. */
#define Py_TPFLAGS_HAVE_NEWBUFFER 0
#define PyFloat_FromString(a,removed_parameter) PyFloat_FromString(a)

/* PyInt -> PyLong remapping */
#define PyInt_AsLong PyLong_AsLong
#define PyInt_Check PyLong_Check
#define PyInt_FromString PyLong_FromString
#define PyInt_FromUnicode PyLong_FromUnicode
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromSize_t PyLong_FromSize_t
#define PyInt_FromSsize_t PyLong_FromSsize_t
#define PyInt_AsLong PyLong_AsLong
// Note: Slightly different semantics, the macro does not do any error checking
#define PyInt_AS_LONG PyLong_AsLong
#define PyInt_AsSsize_t PyLong_AsSsize_t
#define PyInt_AsUnsignedLongMask PyLong_AsUnsignedLongMask
#define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask

#define PyNumber_Int PyNumber_Long
#define PyInt_Type PyLong_Type

#endif /* PY_MAJOR_VERSION >= 3 */

#endif

// See PEP 238
#define PyNumber_Divide PyNumber_TrueDivide
#define PyNumber_InPlaceDivide PyNumber_InPlaceTrueDivide

#if PY_MAJOR_VERSION >= 3
// nb_coerce, nb_oct and nb_hex fields have been removed in Python 3
#define INITIALIZE_NB_COERCE_ZERO
#define INITIALIZE_NB_OCT_ZERO
#define INITIALIZE_NB_HEX_ZERO
#define INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO
#define INITIALIZE_NB_DIVIDE_ZERO
#else
#define INITIALIZE_NB_COERCE_ZERO 0,
#define INITIALIZE_NB_OCT_ZERO 0,
#define INITIALIZE_NB_HEX_ZERO 0,
#define INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO 0,
#define INITIALIZE_NB_DIVIDE_ZERO 0,
#endif

/* Unicode/string handling. */
#if PY_MAJOR_VERSION >= 3
#define PY_STRING_CHECK(a) PyUnicode_Check(a) 
#define PY_STRING_AS_STRING(a) PyUnicode_AsUTF8(a)
#define PY_UNICODE_AS_UNICODE(a) PyUnicode_AsUTF8(a)
#define PY_BYTES_FROM_STRING(a) PyBytes_FromString(a)
#else
#define PY_STRING_CHECK(a) (PyUnicode_Check(a) || PyBytes_Check(a))
#define PY_STRING_AS_STRING(a) PyBytes_AsString(a)
#define PY_UNICODE_AS_UNICODE(a) PyBytes_AsString(PyUnicode_AsASCIIString(a))
#define PY_BYTES_FROM_STRING(a) PyString_FromString(a)
#endif
