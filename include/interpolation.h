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

MYFLT nointerp(MYFLT *buf, T_SIZE_T index, MYFLT frac, T_SIZE_T size);
MYFLT linear(MYFLT *buf, T_SIZE_T index, MYFLT frac, T_SIZE_T size);
MYFLT cosine(MYFLT *buf, T_SIZE_T index, MYFLT frac, T_SIZE_T size);
MYFLT cubic(MYFLT *buf, T_SIZE_T index, MYFLT frac, T_SIZE_T size);

#endif
