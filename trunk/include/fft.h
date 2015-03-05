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

#ifndef _FFT_
#define _FFT_
/* in-place split-radix real fft */
void realfft_split(MYFLT *data, MYFLT *outdata, int n, MYFLT **twiddle);
void irealfft_split(MYFLT *data, MYFLT *outdata, int n, MYFLT **twiddle);
void fft_compute_split_twiddle(MYFLT **twiddle, int size);
void fft_compute_window(MYFLT *window, int size, int wintype);
/* in-place radix-2 real fft (not used actually) */
void realfft_packed(MYFLT *data, MYFLT *outdata, int size, MYFLT *twiddle);
void irealfft_packed(MYFLT *data, MYFLT *outdata, int size, MYFLT *twiddle);
/* Prototype for array generation of twiddle factors */
void fft_compute_split_twiddle(MYFLT **twiddle, int size);
void fft_compute_radix2_twiddle(MYFLT *twiddle, int size);
#endif
