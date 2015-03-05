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
#include "pyomodule.h"

#ifndef _INTERPOLATION_
#define _INTERPOLATION_

MYFLT nointerp(MYFLT *buf, int index, MYFLT frac, int size);
MYFLT linear(MYFLT *buf, int index, MYFLT frac, int size);
MYFLT cosine(MYFLT *buf, int index, MYFLT frac, int size);
MYFLT cubic(MYFLT *buf, int index, MYFLT frac, int size);

#endif
