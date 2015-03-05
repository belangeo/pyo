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
 *                                                                        *
 * Envelope window generator :                                            *
 *      0 : Rectangular (no window)                                       *
 *      1 : Hamming                                                       *
 *      2 : Hanning                                                       *
 *      3 : Bartlett (triangular)                                         *
 *      4 : Blackman 3-term                                               *
 *      5 : Blackman-Harris 4-term                                        *
 *      6 : Blackman-Harris 7-term                                        *
 *      7 : Tuckey (alpha = 0.66)                                         *
 *      8 : Sine (half-sine window)                                       *
 *************************************************************************/

#include "pyomodule.h"

#ifndef _WIND_
#define _WIND_
void gen_window(MYFLT *window, int size, int wintype);
#endif
