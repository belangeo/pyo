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
/******************************************************
**	                 FFT library
**
**  (one-dimensional complex and real FFTs for array
**  lengths of 2^n)
**
**	Author: Toth Laszlo (tothl@inf.u-szeged.hu)
**
**	Research Group on Artificial Intelligence
**  H-6720 Szeged, Aradi vertanuk tere 1, Hungary
**
**	Last modified: 97.05.29
**
**  Modified by belangeo 2011.05.25
**    - Added twiddle factors lookup table (radix-2)
**    - Added twiddle factors 2d lookup table (split-radix)
**
**  Original file can be found on musicdsp.org :
**  http://www.musicdsp.org/archive.php?classid=2#79
****************************************************** */
#include "fft.h"
#include "pyomodule.h"
#include <math.h>

void fft_compute_split_twiddle(MYFLT **twiddle, int size) {
    /* pre-compute split-radix twiddle factors in 2d array of length [4][size>>3] */
    int j;
    int n8 = size >> 3;
    MYFLT e = 2.0 * PI / size;
    MYFLT a = e;
    MYFLT a3;
    for(j=2; j<=n8; j++) {
        a3 = 3 * a;
        twiddle[0][j-1] = MYCOS(a);
        twiddle[1][j-1] = MYSIN(a);
        twiddle[2][j-1] = MYCOS(a3);
        twiddle[3][j-1] = MYSIN(a3);
        a = j * e;
    }
    return;
}

void fft_compute_radix2_twiddle(MYFLT *twiddle, int size) {
    /* pre-compute radix-2 twiddle factors in one array of length n */
    /* re[0], re[1], ..., re[n/2-1], im[0], im[1], ..., im[n/2-1] */
    int i;
    int hsize = size / 2;
    for (i=0; i<hsize; i++) {
        twiddle[i] = MYCOS(TWOPI/hsize*i);
        twiddle[hsize+i] = MYSIN(TWOPI/hsize*i);
    }

}
/****************************************************************
** Sorensen in-place split-radix FFT for real values
** data: array of doubles:
** re(0),re(1),re(2),...,re(size-1)
**
** output:
** re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
** normalized by array length
**
** Source:
** Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
** IEEE Trans. ASSP, ASSP-35, No. 6, June 1987
**************************************************************** */
void realfft_split(MYFLT *data, MYFLT *outdata, int n, MYFLT **twiddle) {

    int i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8;
    int pas, pos;
    MYFLT t1,t2,t3,t4,t5,t6,ss1,ss3,cc1,cc3,sqrt2;

    sqrt2 = 1.4142135623730951; /* sqrt(2.0) */
    n4 = n - 1;

    /* data shuffling */
    for (i=0,j=0,n2=n/2; i<n4 ; i++){
	    if (i<j){
			t1 = data[j];
			data[j] = data[i];
			data[i] = t1;
		}
	    k = n2;
	    while (k<=j){
			j -= k;
			k >>= 1;
		}
	    j += k;
    }

	/* length two butterflies */
	i0 = 0;
	id = 4;
    do {
        for (; i0<n4; i0+=id){
		    i1 = i0 + 1;
			t1 = data[i0];
			data[i0] = t1 + data[i1];
			data[i1] = t1 - data[i1];
	    }
	    id <<= 1;
        i0 = id - 2;
        id <<= 1;
    } while ( i0<n4 );

    /* L shaped butterflies */
    n2 = 2;
    for(k=n; k>2; k>>=1) {
	    n2 <<= 1; /* power of two from 4 to n */
	    n4 = n2 >> 2;
	    n8 = n2 >> 3;
	    pas = n / n2;
	    i1 = 0;
	    id = n2 << 1;
	    do {
	        for (; i1<n; i1+=id){
			    i2 = i1 + n4;
			    i3 = i2 + n4;
			    i4 = i3 + n4;
			    t1 = data[i4] + data[i3];
			    data[i4] -= data[i3];
			    data[i3] = data[i1] - t1;
			    data[i1] += t1;
			    if (n4!=1){
				    i0 = i1 + n8;
				    i2 += n8;
				    i3 += n8;
				    i4 += n8;
				    t1 = (data[i3] + data[i4]) / sqrt2;
				    t2 = (data[i3] - data[i4]) / sqrt2;
				    data[i4] = data[i2] - t1;
				    data[i3] = -data[i2] - t1;
				    data[i2] = data[i0] - t2;
				    data[i0] += t2;
			    }
	        }
		    id <<= 1;
	        i1 = id - n2;
	        id <<= 1;
	    } while ( i1<n );
	    for (j=2; j<=n8; j++){
	        pos = (j-1) * pas;
	        cc1 = twiddle[0][pos];
	        ss1 = twiddle[1][pos];
	        cc3 = twiddle[2][pos];
	        ss3 = twiddle[3][pos];
	        i = 0;
	        id = n2 << 1;
	        do {
		        for (; i<n; i+=id){
			        i1 = i + j - 1;
			        i2 = i1 + n4;
			        i3 = i2 + n4;
			        i4 = i3 + n4;
			        i5 = i + n4 - j + 1;
			        i6 = i5 + n4;
			        i7 = i6 + n4;
			        i8 = i7 + n4;
			        t1 = data[i3] * cc1 + data[i7] * ss1;
			        t2 = data[i7] * cc1 - data[i3] * ss1;
			        t3 = data[i4] * cc3 + data[i8] * ss3;
			        t4 = data[i8] * cc3 - data[i4] * ss3;
			        t5 = t1 + t3;
			        t6 = t2 + t4;
			        t3 = t1 - t3;
			        t4 = t2 - t4;
			        t2 = data[i6] + t6;
			        data[i3] = t6 - data[i6];
			        data[i8] = t2;
			        t2 = data[i2] - t3;
			        data[i7] = -data[i2] - t3;
			        data[i4] = t2;
			        t1 = data[i1] + t5;
			        data[i6] = data[i1] - t5;
			        data[i1] = t1;
			        t1 = data[i5] + t4;
			        data[i5] -= t4;
			        data[i2] = t1;
	            }
		        id <<= 1;
		        i = id - n2;
		        id <<= 1;
	        } while(i<n);
	    }
    }

	/* division with array length */
    for(i=0; i<n; i++)
        outdata[i] = data[i] / n;
}

/*******************************************************************
** Sorensen in-place inverse split-radix FFT for real values
** data: array of doubles:
** re(0),re(1),re(2),...,re(size/2),im(size/2-1),...,im(1)
**
** output:
** re(0),re(1),re(2),...,re(size-1)
** NOT normalized by array length
**
** Source:
** Sorensen et al: Real-Valued Fast Fourier Transform Algorithms,
** IEEE Trans. ASSP, ASSP-35, No. 6, June 1987
****************************************************************** */

void irealfft_split(MYFLT *data, MYFLT *outdata, int n, MYFLT **twiddle) {

    int i,j,k,i5,i6,i7,i8,i0,id,i1,i2,i3,i4,n2,n4,n8,n1;
    int pas, pos;
    MYFLT t1,t2,t3,t4,t5,ss1,ss3,cc1,cc3,sqrt2;

    sqrt2 = 1.4142135623730951; /* sqrt(2.0) */

    n1 = n - 1;
    n2 = n << 1;
    for(k=n; k>2; k>>=1) {
	    id = n2;
	    n2 >>= 1;
	    n4 = n2 >> 2;
	    n8 = n2 >> 3;
	    pas = n / n2;
	    i1 = 0;
	    do {
	        for (; i1<n; i1+=id) {
			    i2 = i1 + n4;
			    i3 = i2 + n4;
			    i4 = i3 + n4;
			    t1 = data[i1] - data[i3];
			    data[i1] += data[i3];
			    data[i2] *= 2;
			    data[i3] = t1 - 2 * data[i4];
			    data[i4] = t1 + 2 * data[i4];
			    if (n4!=1) {
				    i0 = i1 + n8;
				    i2 += n8;
				    i3 += n8;
				    i4 += n8;
				    t1 = (data[i2] - data[i0]) / sqrt2;
				    t2 = (data[i4] + data[i3]) / sqrt2;
				    data[i0] += data[i2];
				    data[i2] = data[i4] - data[i3];
				    data[i3] = 2 * (-t2 - t1);
				    data[i4] = 2 * (-t2 + t1);
			    }
	        }
		    id <<= 1;
	        i1 = id - n2;
	        id <<= 1;
	    } while ( i1<n1 );
	    for (j=2; j<=n8; j++) {
	        pos = (j-1) * pas;
	        cc1 = twiddle[0][pos];
	        ss1 = twiddle[1][pos];
	        cc3 = twiddle[2][pos];
	        ss3 = twiddle[3][pos];
	        i = 0;
	        id = n2 << 1;
	        do {
		        for (; i<n; i+=id) {
			        i1 = i + j - 1;
			        i2 = i1 + n4;
			        i3 = i2 + n4;
			        i4 = i3 + n4;
			        i5 = i + n4 - j + 1;
			        i6 = i5 + n4;
			        i7 = i6 + n4;
			        i8 = i7 + n4;
			        t1 = data[i1] - data[i6];
			        data[i1] += data[i6];
			        t2 = data[i5] - data[i2];
			        data[i5] += data[i2];
			        t3 = data[i8] + data[i3];
			        data[i6] = data[i8] - data[i3];
			        t4 = data[i4] + data[i7];
			        data[i2] = data[i4] - data[i7];
			        t5 = t1 - t4;
			        t1 += t4;
			        t4 = t2 - t3;
			        t2 += t3;
			        data[i3] = t5 * cc1 + t4 * ss1;
			        data[i7] = -t4 * cc1 + t5 * ss1;
			        data[i4] = t1 * cc3 - t2 * ss3;
			        data[i8] = t2 * cc3 + t1 * ss3;
			    }
		        id <<= 1;
		        i = id - n2;
		        id <<= 1;
		    } while(i<n1);
	    }
    }

    /*----------------------*/
	i0 = 0;
	id = 4;
    do {
        for (; i0<n1; i0+=id) {
		    i1 = i0 + 1;
			t1 = data[i0];
			data[i0] = t1 + data[i1];
			data[i1] = t1 - data[i1];
	    }
	    id <<= 1;
        i0 = id - 2;
        id <<= 1;
    } while ( i0<n1 );

    /* data shuffling */
    for (i=0,j=0,n2=n/2; i<n1 ; i++) {
	    if (i<j) {
		    t1 = data[j];
			data[j] = data[i];
			data[i] = t1;
		}
	    k = n2;
	    while (k<=j) {
			j -= k;
			k >>= 1;
		}
	    j += k;
    }
    for (i=0; i<n; i++)
        outdata[i] = data[i];
}

/* *****************************************************
** Decimation-in-freq radix-2 in-place butterfly
** data: array of doubles:
** re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
** it means that size=array_length/2 !!
**
** suggested use:
** intput in normal order
** output in bit-reversed order
**
** Source: Rabiner-Gold: Theory and Application of DSP,
** Prentice Hall,1978
******************************************************* */
void dif_butterfly(MYFLT *data, int size, MYFLT *twiddle){

    int angle,astep,dl;
    MYFLT xr,yr,xi,yi,wr,wi,dr,di;
    MYFLT *l1, *l2, *end, *ol2;

    astep = 1;
	end = data + size + size;
    for(dl=size; dl>1; dl>>=1, astep+=astep) {
	    l1 = data;
        l2 = data + dl;
        for(;l2<end;l1=l2,l2=l2+dl) {
		    ol2 = l2;
            for(angle=0; l1<ol2; l1+=2, l2+=2) {
                wr = twiddle[angle];
                wi = -twiddle[size+angle]; /* size here is half the FFT size */
                xr = *l1 + *l2;
                xi = *(l1+1) + *(l2+1);
                dr = *l1 - *l2;
                di = *(l1+1) - *(l2+1);
                yr = dr * wr - di * wi;
                yi = dr * wi + di * wr;
                *(l1) = xr; *(l1+1) = xi;
                *(l2) = yr; *(l2+1) = yi;
                angle += astep;
		    }
        }
    }
}

/* *****************************************************
** Decimation-in-time radix-2 in-place inverse butterfly
** data: array of doubles:
** re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
** it means that size=array_length/2 !!
**
** suggested use:
** intput in bit-reversed order
** output in normal order
**
** Source: Rabiner-Gold: Theory and Application of DSP,
** Prentice Hall,1978
******************************************************* */
void inverse_dit_butterfly(MYFLT *data, int size, MYFLT *twiddle){

    int angle,astep,dl;
	MYFLT xr,yr,xi,yi,wr,wi,dr,di;
	MYFLT *l1, *l2, *end, *ol2;

    astep = size >> 1;
	end = data + size + size;
    for(dl=2; astep>0; dl+=dl, astep>>=1) {
        l1 = data;
        l2 = data + dl;
        for(; l2<end; l1=l2, l2=l2+dl) {
	        ol2=l2;
            for(angle=0; l1<ol2; l1+=2, l2+=2) {
                wr = twiddle[angle];
                wi = twiddle[size+angle]; /* size here is half the FFT size */
                xr = *l1;
                xi = *(l1+1);
                yr = *l2;
                yi = *(l2+1);
                dr = yr * wr - yi * wi;
                di = yr * wi + yi * wr;
                *(l1) = xr + dr; *(l1+1) = xi + di;
                *(l2) = xr - dr; *(l2+1) = xi - di;
                angle += astep;
		    }
        }
    }
}

/* *****************************************************
** data shuffling into bit-reversed order
** data: array of doubles:
** re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
** it means that size=array_length/2 !!
**
** Source: Rabiner-Gold: Theory and Application of DSP,
** Prentice Hall,1978
******************************************************* */
void unshuffle(MYFLT *data, int size){

    int i,j,k,l,m;
    MYFLT re,im;

    l = size - 1;
    m = size >> 1;
    for (i=0, j=0; i<l ; i++) {
        if (i < j) {
            re = data[j+j]; im = data[j+j+1];
            data[j+j] = data[i+i]; data[j+j+1] = data[i+i+1];
            data[i+i] = re; data[i+i+1] = im;
        }
        k = m;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

/* *****************************************************
** used by realfft
** parameters as above
**
** Source: Brigham: The Fast Fourier Transform
** Prentice Hall, ?
***************************************************** */
void realize(MYFLT *data, int size) {

	MYFLT xr,yr,xi,yi,wr,wi,dr,di,ang,astep;
	MYFLT *l1, *l2;

	l1 = data;
	l2 = data + size + size - 2;
    xr = *l1;
    xi = *(l1+1);
    *l1 = xr + xi;
    *(l1+1) = xr - xi;
	l1 += 2;
	astep = PI / size;
    for(ang=astep; l1<=l2; l1+=2, l2-=2, ang+=astep) {
        xr = (*l1 + *l2) / 2;
        yi = (-(*l1) + (*l2)) / 2;
        yr = (*(l1+1) + *(l2+1)) / 2;
        xi = (*(l1+1) - *(l2+1)) / 2;
        wr = cos(ang);
        wi = -sin(ang);
        dr = yr * wr - yi * wi;
        di = yr * wi + yi * wr;
        *l1 = xr + dr;
        *(l1+1) = xi + di;
        *l2 = xr - dr;
        *(l2+1) = -xi + di;
	}
}

/* *****************************************************
** used by inverse realfft
** parameters as above
**
** Source: Brigham: The Fast Fourier Transform
** Prentice Hall, ?
****************************************************** */
void unrealize(MYFLT *data, int size) {

	MYFLT xr,yr,xi,yi,wr,wi,dr,di,ang,astep;
	MYFLT *l1, *l2;

	l1 = data;
	l2 = data + size + size - 2;
    xr = (*l1) / 2;
    xi = (*(l1+1)) / 2;
    *l1 = xr + xi;
    *(l1+1) = xr - xi;
	l1 += 2;
	astep = PI / size;
    for(ang=astep; l1<=l2; l1+=2, l2-=2, ang+=astep){
        xr = (*l1+*l2) / 2;
        yi = -(-(*l1) + (*l2)) / 2;
        yr = (*(l1+1) + *(l2+1)) / 2;
        xi = (*(l1+1) - *(l2+1)) / 2;
        wr = cos(ang);
        wi = -sin(ang);
        dr = yr * wr - yi * wi;
        di = yr * wi + yi * wr;
        *l2 = xr + dr;
        *(l1+1) = xi + di;
        *l1 = xr - dr;
        *(l2+1) = -xi + di;
	}
}

/* *****************************************************
** in-place Radix-2 FFT for real values
** (by the so-called "packing method")
** data: array of doubles:
** re(0),re(1),re(2),...,re(size-1)
**
** output:
** re(0),re(size/2),re(1),im(1),re(2),im(2),...,re(size/2-1),im(size/2-1)
** normalized by array length
**
** Source: see the routines it calls ...
******************************************************* */
void realfft_packed(MYFLT *data, MYFLT *outdata, int size, MYFLT *twiddle) {

    int i;

	size >>= 1;
	dif_butterfly(data, size, twiddle);
	unshuffle(data, size);
	realize(data, size);

	size <<= 1;
	for (i=0; i<size; i++)
	    outdata[i] = data[i] / size;
}

/* *****************************************************
** in-place Radix-2 inverse FFT for real values
** (by the so-called "packing method")
** data: array of doubles:
** re(0),re(size/2),re(1),im(1),re(2),im(2),...,re(size/2-1),im(size/2-1)
**
** output:
** re(0),re(1),re(2),...,re(size-1)
** NOT normalized by array length
**
** Source: see the routines it calls ...
******************************************************* */
void irealfft_packed(MYFLT *data, MYFLT *outdata, int size, MYFLT *twiddle) {

    int i;

	size >>= 1;
	unrealize(data, size);
	unshuffle(data, size);
	inverse_dit_butterfly(data, size, twiddle);

	size <<= 1;
	for (i=0; i<size; i++)
	    outdata[i] = data[i] * 2;
}
