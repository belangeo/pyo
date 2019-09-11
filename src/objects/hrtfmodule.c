/**************************************************************************
 * Copyright 2009-2018 Olivier Belanger                                   *
 *                                                                        *
 * self file is part of pyo, a python module to help digital signal       *
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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"
#include "fft.h"
#include "wind.h"

/************************************************************************************************/
/* HRTFData streamer object */
/************************************************************************************************/
typedef struct {
    PyObject_HEAD
    int length;
    MYFLT hrtf_diff[14];
    int files_per_folder[14];
    MYFLT ***hrtf_left;
    MYFLT ***hrtf_right;
    MYFLT ***mag_left;
    MYFLT ***ang_left;
    MYFLT ***mag_right;
    MYFLT ***ang_right;
} HRTFData;

static int
HRTFData_traverse(HRTFData *self, visitproc visit, void *arg)
{
    return 0;
}

static int
HRTFData_clear(HRTFData *self)
{
    return 0;
}

static void
HRTFData_dealloc(HRTFData* self)
{
    int i, j;
    for (i=0; i<14; i++) {
        int howmany = self->files_per_folder[i] * 2 - 1;
        for (j=0; j<howmany; j++) {
            free(self->hrtf_left[i][j]);
            free(self->hrtf_right[i][j]);
            free(self->mag_left[i][j]);
            free(self->ang_left[i][j]);
            free(self->mag_right[i][j]);
            free(self->ang_right[i][j]);
        }
        free(self->hrtf_left[i]);
        free(self->hrtf_right[i]);
        free(self->mag_left[i]);
        free(self->ang_left[i]);
        free(self->mag_right[i]);
        free(self->ang_right[i]);
    }
    free(self->hrtf_left);
    free(self->hrtf_right);
    free(self->mag_left);
    free(self->ang_left);
    free(self->mag_right);
    free(self->ang_right);
    HRTFData_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

MYFLT ***
HRTFData_getHRTFLeft(HRTFData *self)
{
    return (MYFLT ***)self->hrtf_left;
}

MYFLT ***
HRTFData_getHRTFRight(HRTFData *self)
{
    return (MYFLT ***)self->hrtf_right;
}

MYFLT ***
HRTFData_getMagLeft(HRTFData *self)
{
    return (MYFLT ***)self->mag_left;
}

MYFLT ***
HRTFData_getAngLeft(HRTFData *self)
{
    return (MYFLT ***)self->ang_left;
}

MYFLT ***
HRTFData_getMagRight(HRTFData *self)
{
    return (MYFLT ***)self->mag_right;
}

MYFLT ***
HRTFData_getAngRight(HRTFData *self)
{
    return (MYFLT ***)self->ang_right;
}

MYFLT *
HRTFData_getHRTFDiff(HRTFData *self)
{
    return (MYFLT *)self->hrtf_diff;
}

int HRTFData_getImpulseLength(HRTFData *self)
{
    return self->length;
}

static PyObject *
HRTFData_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j, k, howmany;
    PyObject *impulses; // list of lists of lists of samples

    HRTFData *self;
    self = (HRTFData *)type->tp_alloc(type, 0);

    /* Dataset descriptors. */
    self->length = 128;
    MYFLT hrtf_diff[14] = {6.5, 6.0, 5.0, 5.0, 5.0, 5.0, 5.0, 6.0, 6.5, 8.185, 10.0, 15.0, 30.0, 0.0};
    memcpy(self->hrtf_diff, hrtf_diff, sizeof(self->hrtf_diff));
    int files_per_folder[14] = {29, 31, 37, 37, 37, 37, 37, 31, 29, 23, 19, 13, 7, 1};
    memcpy(self->files_per_folder, files_per_folder, sizeof(self->files_per_folder));

    static char *kwlist[] = {"path", "length", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &impulses, &self->length))
        Py_RETURN_NONE;

    /* Store HRIRs. */
    self->hrtf_left = (MYFLT ***)realloc(self->hrtf_left, 14 * sizeof(MYFLT **));
    self->hrtf_right = (MYFLT ***)realloc(self->hrtf_right, 14 * sizeof(MYFLT **));
    for (i=0; i<14; i++) {
        howmany = files_per_folder[i];
        self->hrtf_left[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        self->hrtf_right[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        for (j=0; j<howmany; j++) {
            self->hrtf_left[i][j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            self->hrtf_right[i][j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            for (k=0; k<self->length; k++) {
                self->hrtf_left[i][j][k] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(impulses, 0), i), j), k));
                self->hrtf_right[i][j][k] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(impulses, 1), i), j), k));
            }
        }
        for (j=0; j<(howmany-1); j++) {
            self->hrtf_left[i][howmany+j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            self->hrtf_right[i][howmany+j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            for (k=0; k<self->length; k++) {
                self->hrtf_left[i][howmany+j][k] = self->hrtf_right[i][howmany-2-j][k];
                self->hrtf_right[i][howmany+j][k] = self->hrtf_left[i][howmany-2-j][k];
            }
        }
    }

    /* Compute magnitudes and unwrapped phases for each impulse. */
    MYFLT re, im, ma, ph;
    int hsize = self->length / 2;
    int n8 = self->length >> 3;
    MYFLT outframe[self->length];
    for (i=0; i<self->length; i++) {
        outframe[i] = 0.0;
    }
    MYFLT real[hsize], imag[hsize], magn[hsize], freq[hsize];
    for (i=0; i<hsize; i++) {
        real[i] = imag[i] = magn[i] = freq[i] = 0.0;
    }
    MYFLT **twiddle = (MYFLT **)malloc(4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(twiddle, self->length);

    self->mag_left = (MYFLT ***)realloc(self->mag_left, 14 * sizeof(MYFLT **));
    self->ang_left = (MYFLT ***)realloc(self->ang_left, 14 * sizeof(MYFLT **));
    self->mag_right = (MYFLT ***)realloc(self->mag_right, 14 * sizeof(MYFLT **));
    self->ang_right = (MYFLT ***)realloc(self->ang_right, 14 * sizeof(MYFLT **));
    for (i=0; i<14; i++) {
        howmany = files_per_folder[i];
        self->mag_left[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        self->ang_left[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        self->mag_right[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        self->ang_right[i] = (MYFLT **)malloc((howmany*2-1) * sizeof(MYFLT *));
        for (j=0; j<(howmany*2-1); j++) {
            self->mag_left[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->ang_left[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->mag_right[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->ang_right[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));

            /* Left channel */
            realfft_split(self->hrtf_left[i][j], outframe, self->length, twiddle);
            real[0] = outframe[0];
            imag[0] = 0.0;
            for (k=1; k<hsize; k++) {
                real[k] = outframe[k];
                imag[k] = outframe[self->length - k];
            }
            for (k=0; k<hsize; k++) {
                re = real[k];
                im = imag[k];
                ma = MYSQRT(re*re + im*im);
                ph = MYATAN2(im, re);
                while (ph > PI) ph -= TWOPI;
                while (ph < -PI) ph += TWOPI;
                self->mag_left[i][j][k] = ma;
                self->ang_left[i][j][k] = ph;
            }
            /* Right channel */
            realfft_split(self->hrtf_right[i][j], outframe, self->length, twiddle);
            real[0] = outframe[0];
            imag[0] = 0.0;
            for (k=1; k<hsize; k++) {
                real[k] = outframe[k];
                imag[k] = outframe[self->length - k];
            }
            for (k=0; k<hsize; k++) {
                re = real[k];
                im = imag[k];
                ma = MYSQRT(re*re + im*im);
                ph = MYATAN2(im, re);
                while (ph > PI) ph -= TWOPI;
                while (ph < -PI) ph += TWOPI;
                self->mag_right[i][j][k] = ma;
                self->ang_right[i][j][k] = ph;
            }
        }
    }

    return (PyObject *)self;
}

static PyMemberDef HRTFData_members[] = {
{NULL}  /* Sentinel */
};

static PyMethodDef HRTFData_methods[] = {
{NULL}  /* Sentinel */
};

PyTypeObject HRTFDataType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.HRTFData_base",         /*tp_name*/
sizeof(HRTFData),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HRTFData_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
0,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"HRTFData objects. Store the HRIRs for a given set.",           /* tp_doc */
(traverseproc)HRTFData_traverse,   /* tp_traverse */
(inquiry)HRTFData_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HRTFData_methods,             /* tp_methods */
HRTFData_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
HRTFData_new,                 /* tp_new */
};


/************************************************************************************************/
/* HRTFSpat main object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    HRTFData *hrtfdata;
    PyObject *input;
    Stream *input_stream;
    PyObject *azi;
    Stream *azi_stream;
    PyObject *ele;
    Stream *ele_stream;
    int length;
    int hrtf_count;
    int hrtf_sample_count;
    MYFLT hrtf_last_azi;
    MYFLT hrtf_last_ele;
    MYFLT *hrtf_input_tmp;
    MYFLT **current_impulses;
    MYFLT **previous_impulses;
    MYFLT **twiddle;
    int modebuffer[2];
    MYFLT *buffer_streams;
} HRTFSpatter;

static void
HRTFSpatter_splitter(HRTFSpatter *self) {
    int i, k, hsize = self->length / 2;
    int tmp_count, azim_index_down, azim_index_up, elev_index, elev_index_array;
    MYFLT azi, ele, norm_elev, sig, elev_frac, elev_frac_inv;
    MYFLT azim_frac_down, azim_frac_inv_down, azim_frac_up, azim_frac_inv_up, cross_coeff, cross_coeff_inv;
    MYFLT magL, angL, magR, angR;
    MYFLT inframeL[self->length], inframeR[self->length];
    MYFLT realL[hsize], imagL[hsize], realR[hsize], imagR[hsize];
    MYFLT *hrtf_diff = HRTFData_getHRTFDiff((HRTFData *)self->hrtfdata);
    MYFLT ***mag_left = HRTFData_getMagLeft((HRTFData *)self->hrtfdata);
    MYFLT ***ang_left = HRTFData_getAngLeft((HRTFData *)self->hrtfdata);
    MYFLT ***mag_right = HRTFData_getMagRight((HRTFData *)self->hrtfdata);
    MYFLT ***ang_right = HRTFData_getAngRight((HRTFData *)self->hrtfdata);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        if (self->hrtf_sample_count == 0) {
            if (self->modebuffer[0])
                azi = Stream_getData((Stream *)self->azi_stream)[i];
            else
                azi = PyFloat_AS_DOUBLE(self->azi);
            if (self->modebuffer[1])
                ele = Stream_getData((Stream *)self->ele_stream)[i];
            else
                ele = PyFloat_AS_DOUBLE(self->ele);

            if (azi < 0.0f) {
                azi += 360.0f;
            }
            if (azi >= 359.9999f) {
                azi = 359.9999f;
            }

            if (ele < -39.9999f) { 
                ele = -39.9999f;
            } else if (ele >= 89.9999f) {
                ele = 89.9999f;
            }

            /* Removes the chirp at 360->0 degrees azimuth boundary. */
            if (MYFABS(self->hrtf_last_azi - azi) > 300.0f) {
                self->hrtf_last_azi = azi;
            }

            self->hrtf_last_azi = azi + (self->hrtf_last_azi - azi) * 0.5;
            self->hrtf_last_ele = ele + (self->hrtf_last_ele - ele) * 0.5;

            for (k=0; k<self->length; k++) {
                self->previous_impulses[0][k] = self->current_impulses[0][k];
                self->previous_impulses[1][k] = self->current_impulses[1][k];
            }

            norm_elev = self->hrtf_last_ele * 0.1f;
            elev_index = (int)MYFLOOR(norm_elev);
            elev_index_array = elev_index + 4;
            elev_frac = norm_elev - elev_index;
            elev_frac_inv = 1.0f - elev_frac;
            if (norm_elev < 8.0f) { // If elevation is less than 80 degrees.
                azim_index_down = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array]);
                azim_frac_down = (self->hrtf_last_azi / hrtf_diff[elev_index_array]) - azim_index_down;
                azim_frac_inv_down = 1.0f - azim_frac_down;
                azim_index_up = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array+1]);
                azim_frac_up = (self->hrtf_last_azi / hrtf_diff[elev_index_array+1]) - azim_index_up;
                azim_frac_inv_up = 1.0f - azim_frac_up;
                for (k=0; k<hsize; k++) {
                    magL = elev_frac_inv *
                           (azim_frac_inv_down * mag_left[elev_index_array][azim_index_down][k] +
                           azim_frac_down * mag_left[elev_index_array][azim_index_down+1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * mag_left[elev_index_array+1][azim_index_up][k] +
                          azim_frac_up * mag_left[elev_index_array+1][azim_index_up+1][k]);
                    angL = elev_frac_inv *
                           (azim_frac_inv_down * ang_left[elev_index_array][azim_index_down][k] +
                           azim_frac_down * ang_left[elev_index_array][azim_index_down+1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * ang_left[elev_index_array+1][azim_index_up][k] +
                           azim_frac_up * ang_left[elev_index_array+1][azim_index_up+1][k]);
                    magR = elev_frac_inv *
                           (azim_frac_inv_down * mag_right[elev_index_array][azim_index_down][k] +
                           azim_frac_down * mag_right[elev_index_array][azim_index_down+1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * mag_right[elev_index_array+1][azim_index_up][k] +
                           azim_frac_up * mag_right[elev_index_array+1][azim_index_up+1][k]);
                    angR = elev_frac_inv *
                           (azim_frac_inv_down * ang_right[elev_index_array][azim_index_down][k] +
                           azim_frac_down * ang_right[elev_index_array][azim_index_down+1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * ang_right[elev_index_array+1][azim_index_up][k] +
                           azim_frac_up * ang_right[elev_index_array+1][azim_index_up+1][k]);
                    realL[k] = magL * MYCOS(angL);
                    imagL[k] = magL * MYSIN(angL);
                    realR[k] = magR * MYCOS(angR);
                    imagR[k] = magR * MYSIN(angR);
                }
            } else { // if elevation is 80 degrees or more, interpolation requires only three points (there's only one HRIR at 90 deg).
                azim_index_down = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array]);
                azim_frac_down = (self->hrtf_last_azi / hrtf_diff[elev_index_array]) - azim_index_down;
                azim_frac_inv_down = 1.0f - azim_frac_down;
                for (k=0; k<hsize; k++) {
                    magL = elev_frac_inv *
                           (azim_frac_inv_down * mag_left[elev_index_array][azim_index_down][k] +
                           azim_frac_down * mag_left[elev_index_array][azim_index_down+1][k]) +
                           elev_frac * mag_left[13][0][k];
                    angL = elev_frac_inv *
                           (azim_frac_inv_down * ang_left[elev_index_array][azim_index_down][k] +
                           azim_frac_down * ang_left[elev_index_array][azim_index_down+1][k]) +
                           elev_frac * ang_left[13][0][k];
                    magR = elev_frac_inv *
                           (azim_frac_inv_down * mag_right[elev_index_array][azim_index_down][k] +
                           azim_frac_down * mag_right[elev_index_array][azim_index_down+1][k]) +
                           elev_frac * mag_right[13][0][k];
                    angR = elev_frac_inv *
                           (azim_frac_inv_down * ang_right[elev_index_array][azim_index_down][k] +
                           azim_frac_down * ang_right[elev_index_array][azim_index_down+1][k]) +
                           elev_frac * ang_right[13][0][k];
                    realL[k] = magL * MYCOS(angL);
                    imagL[k] = magL * MYSIN(angL);
                    realR[k] = magR * MYCOS(angR);
                    imagR[k] = magR * MYSIN(angR);
                }
            }
            inframeL[0] = realL[0];
            inframeR[0] = realR[0];
            inframeL[hsize] = 0.0;
            inframeR[hsize] = 0.0;
            for (k=1; k<hsize; k++) {
                inframeL[k] = realL[k];
                inframeL[self->length - k] = imagL[k];
                inframeR[k] = realR[k];
                inframeR[self->length - k] = imagR[k];
            }
            irealfft_split(inframeL, self->current_impulses[0], self->length, self->twiddle);
            irealfft_split(inframeR, self->current_impulses[1], self->length, self->twiddle);
        }
        tmp_count = self->hrtf_count;
        cross_coeff = (MYFLT)self->hrtf_sample_count / (MYFLT)self->length;
        cross_coeff_inv = 1.0 - cross_coeff;
        self->buffer_streams[i] = 0.0;
        self->buffer_streams[i+self->bufsize] = 0.0;
        for (k=0; k<self->length; k++) {
            if (tmp_count < 0) {
                tmp_count += self->length;
            }
            sig = self->hrtf_input_tmp[tmp_count];
            self->buffer_streams[i] += sig * (cross_coeff * self->current_impulses[0][k] + 
                                              cross_coeff_inv * self->previous_impulses[0][k]);
            self->buffer_streams[i+self->bufsize] += sig * (cross_coeff * self->current_impulses[1][k] + 
                                                            cross_coeff_inv * self->previous_impulses[1][k]);
            tmp_count--;
        }
        self->hrtf_count++;
        if (self->hrtf_count >= self->length) {
            self->hrtf_count = 0;
        }
        self->hrtf_input_tmp[self->hrtf_count] = in[i];

        self->hrtf_sample_count += 1;
        if (self->hrtf_sample_count >= self->length) {
            self->hrtf_sample_count = 0;
        }
    }
}

MYFLT *
HRTFSpatter_getSamplesBuffer(HRTFSpatter *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
HRTFSpatter_setProcMode(HRTFSpatter *self)
{
    self->proc_func_ptr = HRTFSpatter_splitter;
}

static void
HRTFSpatter_compute_next_data_frame(HRTFSpatter *self)
{
    (*self->proc_func_ptr)(self);
}

static int
HRTFSpatter_traverse(HRTFSpatter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->hrtfdata);
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->azi);
    Py_VISIT(self->azi_stream);
    Py_VISIT(self->ele);
    Py_VISIT(self->ele_stream);
    return 0;
}

static int
HRTFSpatter_clear(HRTFSpatter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->hrtfdata);
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->azi);
    Py_CLEAR(self->azi_stream);
    Py_CLEAR(self->ele);
    Py_CLEAR(self->ele_stream);
    return 0;
}

static void
HRTFSpatter_dealloc(HRTFSpatter* self)
{
    int i;
    pyo_DEALLOC
    free(self->buffer_streams);
    free(self->hrtf_input_tmp);
    for (i=0; i<2; i++) {
        free(self->current_impulses[i]);
        free(self->previous_impulses[i]);
    }
    free(self->current_impulses);
    free(self->previous_impulses);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    HRTFSpatter_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
HRTFSpatter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j, k;
    PyObject *inputtmp, *input_streamtmp, *hrtfdatatmp=NULL, *azitmp=NULL, *eletmp=NULL;
    HRTFSpatter *self;
    self = (HRTFSpatter *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, HRTFSpatter_compute_next_data_frame);
    self->mode_func_ptr = HRTFSpatter_setProcMode;

    self->azi = PyFloat_FromDouble(0.0);
    self->ele = PyFloat_FromDouble(0.0);
    self->hrtf_sample_count = 0;
    self->hrtf_count = 0;
    self->hrtf_last_azi = 0.0;
    self->hrtf_last_ele = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    static char *kwlist[] = {"input", "hrtfdata", "azi", "ele", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &hrtfdatatmp, &azitmp, &eletmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->hrtfdata);
    Py_INCREF(hrtfdatatmp);
    self->hrtfdata = (HRTFData *)hrtfdatatmp;

    self->length = HRTFData_getImpulseLength(self->hrtfdata);

    if (azitmp) {
        PyObject_CallMethod((PyObject *)self, "setAzimuth", "O", azitmp);
    }

    if (eletmp) {
        PyObject_CallMethod((PyObject *)self, "setElevation", "O", eletmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 2 * self->bufsize * sizeof(MYFLT));
    self->hrtf_input_tmp = (MYFLT *)realloc(self->hrtf_input_tmp, self->length * sizeof(MYFLT));
    self->current_impulses = (MYFLT **)realloc(self->current_impulses, 2 * sizeof(MYFLT *));
    self->previous_impulses = (MYFLT **)realloc(self->previous_impulses, 2 * sizeof(MYFLT *));
    
    for (k=0; k<(2*self->bufsize); k++) {
        self->buffer_streams[k] = 0.0;
    }
    for (j=0; j<2; j++) {
        self->current_impulses[j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
        self->previous_impulses[j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
        for (k=0; k<self->length; k++) {
            self->current_impulses[j][k] = 0.0;
            self->previous_impulses[j][k] = 0.0;
        }
    }
    for (k=0; k<self->length; k++) {
        self->hrtf_input_tmp[k] = 0.0;
    }

    int n8 = self->length >> 3;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->length);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * HRTFSpatter_getServer(HRTFSpatter* self) { GET_SERVER };
static PyObject * HRTFSpatter_getStream(HRTFSpatter* self) { GET_STREAM };

static PyObject * HRTFSpatter_play(HRTFSpatter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * HRTFSpatter_stop(HRTFSpatter *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
HRTFSpatter_setAzimuth(HRTFSpatter *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->azi);
	if (isNumber == 1) {
		self->azi = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->azi = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->azi, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->azi_stream);
        self->azi_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
HRTFSpatter_setElevation(HRTFSpatter *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ele);
	if (isNumber == 1) {
		self->ele = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
	}
	else {
		self->ele = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ele, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ele_stream);
        self->ele_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef HRTFSpatter_members[] = {
{"server", T_OBJECT_EX, offsetof(HRTFSpatter, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(HRTFSpatter, stream), 0, "Stream object."},
{"hrtfdata", T_OBJECT_EX, offsetof(HRTFSpatter, hrtfdata), 0, "HRIR dataset."},
{"input", T_OBJECT_EX, offsetof(HRTFSpatter, input), 0, "Input sound object."},
{"azi", T_OBJECT_EX, offsetof(HRTFSpatter, azi), 0, "Azimuth object."},
{"ele", T_OBJECT_EX, offsetof(HRTFSpatter, ele), 0, "Elevation object."},
{NULL}  /* Sentinel */
};

static PyMethodDef HRTFSpatter_methods[] = {
{"getServer", (PyCFunction)HRTFSpatter_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)HRTFSpatter_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)HRTFSpatter_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)HRTFSpatter_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setAzimuth", (PyCFunction)HRTFSpatter_setAzimuth, METH_O, "Sets azimuth value between -180 and 180 degrees."},
{"setElevation", (PyCFunction)HRTFSpatter_setElevation, METH_O, "Sets elevation value between -40 and 90 degrees."},
{NULL}  /* Sentinel */
};

PyTypeObject HRTFSpatterType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.HRTFSpatter_base",                                   /*tp_name*/
sizeof(HRTFSpatter),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)HRTFSpatter_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
0,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"HRTFSpatter main objects.",           /* tp_doc */
(traverseproc)HRTFSpatter_traverse,                  /* tp_traverse */
(inquiry)HRTFSpatter_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
HRTFSpatter_methods,                                 /* tp_methods */
HRTFSpatter_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
HRTFSpatter_new,                                     /* tp_new */
};

/************************************************************************************************/
/* HRTFSpat streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    HRTFSpatter *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} HRTF;

static void HRTF_postprocessing_ii(HRTF *self) { POST_PROCESSING_II };
static void HRTF_postprocessing_ai(HRTF *self) { POST_PROCESSING_AI };
static void HRTF_postprocessing_ia(HRTF *self) { POST_PROCESSING_IA };
static void HRTF_postprocessing_aa(HRTF *self) { POST_PROCESSING_AA };
static void HRTF_postprocessing_ireva(HRTF *self) { POST_PROCESSING_IREVA };
static void HRTF_postprocessing_areva(HRTF *self) { POST_PROCESSING_AREVA };
static void HRTF_postprocessing_revai(HRTF *self) { POST_PROCESSING_REVAI };
static void HRTF_postprocessing_revaa(HRTF *self) { POST_PROCESSING_REVAA };
static void HRTF_postprocessing_revareva(HRTF *self) { POST_PROCESSING_REVAREVA };

static void
HRTF_setProcMode(HRTF *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = HRTF_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = HRTF_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = HRTF_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = HRTF_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = HRTF_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = HRTF_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = HRTF_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = HRTF_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = HRTF_postprocessing_revareva;
            break;
    }
}

static void
HRTF_compute_next_data_frame(HRTF *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = HRTFSpatter_getSamplesBuffer((HRTFSpatter *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
HRTF_traverse(HRTF *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
HRTF_clear(HRTF *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
HRTF_dealloc(HRTF* self)
{
    pyo_DEALLOC
    HRTF_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
HRTF_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    HRTF *self;
    self = (HRTF *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, HRTF_compute_next_data_frame);
    self->mode_func_ptr = HRTF_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (HRTFSpatter *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * HRTF_getServer(HRTF* self) { GET_SERVER };
static PyObject * HRTF_getStream(HRTF* self) { GET_STREAM };
static PyObject * HRTF_setMul(HRTF *self, PyObject *arg) { SET_MUL };
static PyObject * HRTF_setAdd(HRTF *self, PyObject *arg) { SET_ADD };
static PyObject * HRTF_setSub(HRTF *self, PyObject *arg) { SET_SUB };
static PyObject * HRTF_setDiv(HRTF *self, PyObject *arg) { SET_DIV };

static PyObject * HRTF_play(HRTF *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * HRTF_out(HRTF *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * HRTF_stop(HRTF *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * HRTF_multiply(HRTF *self, PyObject *arg) { MULTIPLY };
static PyObject * HRTF_inplace_multiply(HRTF *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * HRTF_add(HRTF *self, PyObject *arg) { ADD };
static PyObject * HRTF_inplace_add(HRTF *self, PyObject *arg) { INPLACE_ADD };
static PyObject * HRTF_sub(HRTF *self, PyObject *arg) { SUB };
static PyObject * HRTF_inplace_sub(HRTF *self, PyObject *arg) { INPLACE_SUB };
static PyObject * HRTF_div(HRTF *self, PyObject *arg) { DIV };
static PyObject * HRTF_inplace_div(HRTF *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef HRTF_members[] = {
{"server", T_OBJECT_EX, offsetof(HRTF, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(HRTF, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(HRTF, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(HRTF, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef HRTF_methods[] = {
{"getServer", (PyCFunction)HRTF_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)HRTF_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)HRTF_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)HRTF_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)HRTF_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)HRTF_setMul, METH_O, "Sets HRTF mul factor."},
{"setAdd", (PyCFunction)HRTF_setAdd, METH_O, "Sets HRTF add factor."},
{"setSub", (PyCFunction)HRTF_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)HRTF_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods HRTF_as_number = {
(binaryfunc)HRTF_add,                      /*nb_add*/
(binaryfunc)HRTF_sub,                 /*nb_subtract*/
(binaryfunc)HRTF_multiply,                 /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs,*/
0,                    /*nb_nonzero*/
0,                    /*nb_invert*/
0,               /*nb_lshift*/
0,              /*nb_rshift*/
0,              /*nb_and*/
0,              /*nb_xor*/
0,               /*nb_or*/
INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
(binaryfunc)HRTF_inplace_add,              /*inplace_add*/
(binaryfunc)HRTF_inplace_sub,         /*inplace_subtract*/
(binaryfunc)HRTF_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)HRTF_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)HRTF_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject HRTFType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.HRTF_base",         /*tp_name*/
sizeof(HRTF),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HRTF_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&HRTF_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"HRTF objects. Reads one channel from a HRTFter.",           /* tp_doc */
(traverseproc)HRTF_traverse,   /* tp_traverse */
(inquiry)HRTF_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HRTF_methods,             /* tp_methods */
HRTF_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
HRTF_new,                 /* tp_new */
};
