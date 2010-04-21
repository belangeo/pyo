/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <math.h>
#include "interpolation.h"

float nointerp(float *buf, int index, float frac, int size) {
    return  buf[index];
}

float linear(float *buf, int index, float frac, int size) {
    float x1 = buf[index];
    float x2 = buf[index+1];
    return (x1 + (x2 - x1) * frac);
}

float cosine(float *buf, int index, float frac, int size) {
    float frac2;
    float x1 = buf[index];
    float x2 = buf[index+1];
    
    frac2 = (1.0 - cosf(frac * M_PI)) * 0.5;
    return (x1 * (1.0 - frac2) + x2 * frac2);
}

float cubic(float *buf, int index, float frac, int size) {
    float x0, x3, a0, a1, a2, a3;
    float x1 = buf[index];
    float x2 = buf[index+1];
    
    if (index == 0) {
        x0 = x1 + (x1 - x2);
        x3 = buf[index + 2];
    }
    else if (index >= (size-2)) {
        x0 = buf[index - 1];
        x3 = x2 + (x2 - x1);
    }    
    else {
        x0 = buf[index - 1];
        x3 = buf[index + 2];
    }
    
    a3 = frac * frac; a3 -= 1.0; a3 *= (1.0 / 6.0);
    a2 = (frac + 1.0) * 0.5; a0 = a2 - 1.0;
    a1 = a3 * 3.0; a2 -= a1; a0 -= a3; a1 -= frac;
    a0 *= frac; a1 *= frac; a2 *= frac; a3 *= frac; a1 += 1.0;

    return (a0*x0+a1*x1+a2*x2+a3*x3);
}
