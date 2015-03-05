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
#include "wind.h"
#include "pyomodule.h"
#include <math.h>

void gen_window(MYFLT *window, int size, int wintype) {
    int i;
    MYFLT arg;

    switch (wintype) {
        case 0:
            /* Rectangular */
            for (i=0; i<size; i++) {
                window[i] = 1.0;
            }
            break;
        case 1:
            /* Hamming */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.54 - 0.46 * MYCOS(arg * i);
            }
            break;
        case 2:
            /* Hanning */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.5 - 0.5 * MYCOS(arg * i);
            }
            break;
        case 3:
            /* Bartlett (triangle) */
            arg = 2.0 / (size-1);
            for (i = 0; i < (size-1)/2; i++) {
                window[i] = i * arg;
            }
            for ( ; i < size; i++) {
                window[i] = 2.0 - i * arg;
            }
            break;
        case 4:
            /* Blackman (3 term) */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.42323 - 0.49755*MYCOS(arg*i) + 0.07922*MYCOS(2*arg*i);
            }
            break;
        case 5:
            /* Blackman-Harris (4 term) */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.35875 - 0.48829*MYCOS(arg*i) + 0.14128*MYCOS(2*arg*i) - \
                            0.01168*MYCOS(3*arg*i);
            }
            break;
        case 6:
            /* Blackman-Harris (7 term) */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.2712203606 - 0.4334446123*MYCOS(arg*i) + \
                            0.21800412*MYCOS(2*arg*i) - 0.0657853433*MYCOS(3*arg*i) + \
                            0.0107618673*MYCOS(4*arg*i) - 0.0007700127*MYCOS(5*arg*i) + \
                            0.00001368088*MYCOS(6*arg*i);
            }
            break;
        case 7:
            /* Tuckey (alpha = 0.66) */
            arg = 0.66;
            for (i = 0; i < (int)(arg*size/2); i++) {
                window[i] = 0.5 * (1 + MYCOS(PI*(2*i/(arg*size)-1)));
            }
            for ( ; i < (int)(size*(1-arg/2)); i++) {
                window[i] = 1.0;
            }
            for ( ; i < size; i++) {
                window[i] = 0.5 * (1 + MYCOS(PI*(2*i/(arg*size)-2/arg+1)));
            }
            break;
        case 8:
            /* Sine */
            arg = PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = MYSIN(arg * i);
            }
            break;
        default:
            /* default is hanning */
            arg = 2.0 * PI / (size-1);
            for (i = 0; i < size; i++) {
                window[i] = 0.5 - 0.5 * MYCOS(arg * i);
            }
            break;
    }
    return;
}
