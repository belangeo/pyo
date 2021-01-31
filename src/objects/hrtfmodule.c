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
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"
#include "fft.h"
#include "wind.h"
#include "vbap.h"

/************************************************************************************************/
/* HRTFData streamer object */
/************************************************************************************************/
typedef struct
{
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

    for (i = 0; i < 14; i++)
    {
        int howmany = self->files_per_folder[i] * 2 - 1;

        for (j = 0; j < howmany; j++)
        {
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

    for (i = 0; i < 14; i++)
    {
        howmany = files_per_folder[i];
        self->hrtf_left[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));
        self->hrtf_right[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));

        for (j = 0; j < howmany; j++)
        {
            self->hrtf_left[i][j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            self->hrtf_right[i][j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));

            for (k = 0; k < self->length; k++)
            {
                self->hrtf_left[i][j][k] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(impulses, 0), i), j), k));
                self->hrtf_right[i][j][k] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(PyList_GET_ITEM(impulses, 1), i), j), k));
            }
        }

        for (j = 0; j < (howmany - 1); j++)
        {
            self->hrtf_left[i][howmany + j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
            self->hrtf_right[i][howmany + j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));

            for (k = 0; k < self->length; k++)
            {
                self->hrtf_left[i][howmany + j][k] = self->hrtf_right[i][howmany - 2 - j][k];
                self->hrtf_right[i][howmany + j][k] = self->hrtf_left[i][howmany - 2 - j][k];
            }
        }
    }

    /* Compute magnitudes and unwrapped phases for each impulse. */
    MYFLT re, im, ma, ph;
    int hsize = self->length / 2;
    int n8 = self->length >> 3;
    MYFLT outframe[self->length];

    for (i = 0; i < self->length; i++)
    {
        outframe[i] = 0.0;
    }

    MYFLT real[hsize], imag[hsize], magn[hsize], freq[hsize];

    for (i = 0; i < hsize; i++)
    {
        real[i] = imag[i] = magn[i] = freq[i] = 0.0;
    }

    MYFLT **twiddle = (MYFLT **)malloc(4 * sizeof(MYFLT *));

    for (i = 0; i < 4; i++)
        twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));

    fft_compute_split_twiddle(twiddle, self->length);

    self->mag_left = (MYFLT ***)realloc(self->mag_left, 14 * sizeof(MYFLT **));
    self->ang_left = (MYFLT ***)realloc(self->ang_left, 14 * sizeof(MYFLT **));
    self->mag_right = (MYFLT ***)realloc(self->mag_right, 14 * sizeof(MYFLT **));
    self->ang_right = (MYFLT ***)realloc(self->ang_right, 14 * sizeof(MYFLT **));

    for (i = 0; i < 14; i++)
    {
        howmany = files_per_folder[i];
        self->mag_left[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));
        self->ang_left[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));
        self->mag_right[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));
        self->ang_right[i] = (MYFLT **)malloc((howmany * 2 - 1) * sizeof(MYFLT *));

        for (j = 0; j < (howmany * 2 - 1); j++)
        {
            self->mag_left[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->ang_left[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->mag_right[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));
            self->ang_right[i][j] = (MYFLT *)malloc(hsize * sizeof(MYFLT));

            /* Left channel */
            realfft_split(self->hrtf_left[i][j], outframe, self->length, twiddle);
            real[0] = outframe[0];
            imag[0] = 0.0;

            for (k = 1; k < hsize; k++)
            {
                real[k] = outframe[k];
                imag[k] = outframe[self->length - k];
            }

            for (k = 0; k < hsize; k++)
            {
                re = real[k];
                im = imag[k];
                ma = MYSQRT(re * re + im * im);
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

            for (k = 1; k < hsize; k++)
            {
                real[k] = outframe[k];
                imag[k] = outframe[self->length - k];
            }

            for (k = 0; k < hsize; k++)
            {
                re = real[k];
                im = imag[k];
                ma = MYSQRT(re * re + im * im);
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

static PyMemberDef HRTFData_members[] =
{
    {NULL}  /* Sentinel */
};

static PyMethodDef HRTFData_methods[] =
{
    {NULL}  /* Sentinel */
};

PyTypeObject HRTFDataType =
{
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    "HRTFData objects. Store the HRIRs for a given set.",           /* tp_doc */
    (traverseproc)HRTFData_traverse,   /* tp_traverse */
    (inquiry)HRTFData_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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
typedef struct
{
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
HRTFSpatter_splitter(HRTFSpatter *self)
{
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

    for (i = 0; i < self->bufsize; i++)
    {
        if (self->hrtf_sample_count == 0)
        {
            if (self->modebuffer[0])
                azi = Stream_getData((Stream *)self->azi_stream)[i];
            else
                azi = PyFloat_AS_DOUBLE(self->azi);

            if (self->modebuffer[1])
                ele = Stream_getData((Stream *)self->ele_stream)[i];
            else
                ele = PyFloat_AS_DOUBLE(self->ele);

            if (azi < 0.0f)
            {
                azi += 360.0f;
            }

            if (azi >= 359.9999f)
            {
                azi = 359.9999f;
            }

            if (ele < -39.9999f)
            {
                ele = -39.9999f;
            }
            else if (ele >= 89.9999f)
            {
                ele = 89.9999f;
            }

            /* Removes the chirp at 360->0 degrees azimuth boundary. */
            if (MYFABS(self->hrtf_last_azi - azi) > 300.0f)
            {
                self->hrtf_last_azi = azi;
            }

            self->hrtf_last_azi = azi + (self->hrtf_last_azi - azi) * 0.5;
            self->hrtf_last_ele = ele + (self->hrtf_last_ele - ele) * 0.5;

            for (k = 0; k < self->length; k++)
            {
                self->previous_impulses[0][k] = self->current_impulses[0][k];
                self->previous_impulses[1][k] = self->current_impulses[1][k];
            }

            norm_elev = self->hrtf_last_ele * 0.1f;
            elev_index = (int)MYFLOOR(norm_elev);
            elev_index_array = elev_index + 4;
            elev_frac = norm_elev - elev_index;
            elev_frac_inv = 1.0f - elev_frac;

            if (norm_elev < 8.0f)   // If elevation is less than 80 degrees.
            {
                azim_index_down = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array]);
                azim_frac_down = (self->hrtf_last_azi / hrtf_diff[elev_index_array]) - azim_index_down;
                azim_frac_inv_down = 1.0f - azim_frac_down;
                azim_index_up = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array + 1]);
                azim_frac_up = (self->hrtf_last_azi / hrtf_diff[elev_index_array + 1]) - azim_index_up;
                azim_frac_inv_up = 1.0f - azim_frac_up;

                for (k = 0; k < hsize; k++)
                {
                    magL = elev_frac_inv *
                           (azim_frac_inv_down * mag_left[elev_index_array][azim_index_down][k] +
                            azim_frac_down * mag_left[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * mag_left[elev_index_array + 1][azim_index_up][k] +
                            azim_frac_up * mag_left[elev_index_array + 1][azim_index_up + 1][k]);
                    angL = elev_frac_inv *
                           (azim_frac_inv_down * ang_left[elev_index_array][azim_index_down][k] +
                            azim_frac_down * ang_left[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * ang_left[elev_index_array + 1][azim_index_up][k] +
                            azim_frac_up * ang_left[elev_index_array + 1][azim_index_up + 1][k]);
                    magR = elev_frac_inv *
                           (azim_frac_inv_down * mag_right[elev_index_array][azim_index_down][k] +
                            azim_frac_down * mag_right[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * mag_right[elev_index_array + 1][azim_index_up][k] +
                            azim_frac_up * mag_right[elev_index_array + 1][azim_index_up + 1][k]);
                    angR = elev_frac_inv *
                           (azim_frac_inv_down * ang_right[elev_index_array][azim_index_down][k] +
                            azim_frac_down * ang_right[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac *
                           (azim_frac_inv_up * ang_right[elev_index_array + 1][azim_index_up][k] +
                            azim_frac_up * ang_right[elev_index_array + 1][azim_index_up + 1][k]);
                    realL[k] = magL * MYCOS(angL);
                    imagL[k] = magL * MYSIN(angL);
                    realR[k] = magR * MYCOS(angR);
                    imagR[k] = magR * MYSIN(angR);
                }
            }
            else     // if elevation is 80 degrees or more, interpolation requires only three points (there's only one HRIR at 90 deg).
            {
                azim_index_down = (int)(self->hrtf_last_azi / hrtf_diff[elev_index_array]);
                azim_frac_down = (self->hrtf_last_azi / hrtf_diff[elev_index_array]) - azim_index_down;
                azim_frac_inv_down = 1.0f - azim_frac_down;

                for (k = 0; k < hsize; k++)
                {
                    magL = elev_frac_inv *
                           (azim_frac_inv_down * mag_left[elev_index_array][azim_index_down][k] +
                            azim_frac_down * mag_left[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac * mag_left[13][0][k];
                    angL = elev_frac_inv *
                           (azim_frac_inv_down * ang_left[elev_index_array][azim_index_down][k] +
                            azim_frac_down * ang_left[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac * ang_left[13][0][k];
                    magR = elev_frac_inv *
                           (azim_frac_inv_down * mag_right[elev_index_array][azim_index_down][k] +
                            azim_frac_down * mag_right[elev_index_array][azim_index_down + 1][k]) +
                           elev_frac * mag_right[13][0][k];
                    angR = elev_frac_inv *
                           (azim_frac_inv_down * ang_right[elev_index_array][azim_index_down][k] +
                            azim_frac_down * ang_right[elev_index_array][azim_index_down + 1][k]) +
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

            for (k = 1; k < hsize; k++)
            {
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
        self->buffer_streams[i + self->bufsize] = 0.0;

        for (k = 0; k < self->length; k++)
        {
            if (tmp_count < 0)
            {
                tmp_count += self->length;
            }

            sig = self->hrtf_input_tmp[tmp_count];
            self->buffer_streams[i] += sig * (cross_coeff * self->current_impulses[0][k] +
                                              cross_coeff_inv * self->previous_impulses[0][k]);
            self->buffer_streams[i + self->bufsize] += sig * (cross_coeff * self->current_impulses[1][k] +
                    cross_coeff_inv * self->previous_impulses[1][k]);
            tmp_count--;
        }

        self->hrtf_count++;

        if (self->hrtf_count >= self->length)
        {
            self->hrtf_count = 0;
        }

        self->hrtf_input_tmp[self->hrtf_count] = in[i];

        self->hrtf_sample_count += 1;

        if (self->hrtf_sample_count >= self->length)
        {
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

    for (i = 0; i < 2; i++)
    {
        free(self->current_impulses[i]);
        free(self->previous_impulses[i]);
    }

    free(self->current_impulses);
    free(self->previous_impulses);

    for (i = 0; i < 4; i++)
    {
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
    PyObject *inputtmp, *input_streamtmp, *hrtfdatatmp = NULL, *azitmp = NULL, *eletmp = NULL;
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

    if (azitmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAzimuth", "O", azitmp);
    }

    if (eletmp)
    {
        PyObject_CallMethod((PyObject *)self, "setElevation", "O", eletmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 2 * self->bufsize * sizeof(MYFLT));
    self->hrtf_input_tmp = (MYFLT *)realloc(self->hrtf_input_tmp, self->length * sizeof(MYFLT));
    self->current_impulses = (MYFLT **)realloc(self->current_impulses, 2 * sizeof(MYFLT *));
    self->previous_impulses = (MYFLT **)realloc(self->previous_impulses, 2 * sizeof(MYFLT *));

    for (k = 0; k < (2 * self->bufsize); k++)
    {
        self->buffer_streams[k] = 0.0;
    }

    for (j = 0; j < 2; j++)
    {
        self->current_impulses[j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));
        self->previous_impulses[j] = (MYFLT *)malloc(self->length * sizeof(MYFLT));

        for (k = 0; k < self->length; k++)
        {
            self->current_impulses[j][k] = 0.0;
            self->previous_impulses[j][k] = 0.0;
        }
    }

    for (k = 0; k < self->length; k++)
    {
        self->hrtf_input_tmp[k] = 0.0;
    }

    int n8 = self->length >> 3;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT *));

    for (i = 0; i < 4; i++)
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

    if (isNumber == 1)
    {
        self->azi = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
    }
    else
    {
        self->azi = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->azi, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->azi_stream);
        self->azi_stream = (Stream *)streamtmp;
        self->modebuffer[0] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
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

    if (isNumber == 1)
    {
        self->ele = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
    }
    else
    {
        self->ele = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ele, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ele_stream);
        self->ele_stream = (Stream *)streamtmp;
        self->modebuffer[1] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyMemberDef HRTFSpatter_members[] =
{
    {"server", T_OBJECT_EX, offsetof(HRTFSpatter, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(HRTFSpatter, stream), 0, "Stream object."},
    {"hrtfdata", T_OBJECT_EX, offsetof(HRTFSpatter, hrtfdata), 0, "HRIR dataset."},
    {"input", T_OBJECT_EX, offsetof(HRTFSpatter, input), 0, "Input sound object."},
    {"azi", T_OBJECT_EX, offsetof(HRTFSpatter, azi), 0, "Azimuth object."},
    {"ele", T_OBJECT_EX, offsetof(HRTFSpatter, ele), 0, "Elevation object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef HRTFSpatter_methods[] =
{
    {"getServer", (PyCFunction)HRTFSpatter_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)HRTFSpatter_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)HRTFSpatter_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)HRTFSpatter_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setAzimuth", (PyCFunction)HRTFSpatter_setAzimuth, METH_O, "Sets azimuth value between -180 and 180 degrees."},
    {"setElevation", (PyCFunction)HRTFSpatter_setElevation, METH_O, "Sets elevation value between -40 and 90 degrees."},
    {NULL}  /* Sentinel */
};

PyTypeObject HRTFSpatterType =
{
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
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
typedef struct
{
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

    switch (muladdmode)
    {
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

    for (i = 0; i < self->bufsize; i++)
    {
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
    PyObject *maintmp = NULL, *multmp = NULL, *addtmp = NULL;
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

    if (multmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp)
    {
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

static PyMemberDef HRTF_members[] =
{
    {"server", T_OBJECT_EX, offsetof(HRTF, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(HRTF, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(HRTF, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(HRTF, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef HRTF_methods[] =
{
    {"getServer", (PyCFunction)HRTF_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)HRTF_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)HRTF_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)HRTF_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)HRTF_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)HRTF_setMul, METH_O, "Sets HRTF mul factor."},
    {"setAdd", (PyCFunction)HRTF_setAdd, METH_O, "Sets HRTF add factor."},
    {"setSub", (PyCFunction)HRTF_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)HRTF_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods HRTF_as_number =
{
    (binaryfunc)HRTF_add,                      /*nb_add*/
    (binaryfunc)HRTF_sub,                 /*nb_subtract*/
    (binaryfunc)HRTF_multiply,                 /*nb_multiply*/
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
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    (binaryfunc)HRTF_inplace_add,              /*inplace_add*/
    (binaryfunc)HRTF_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)HRTF_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject HRTFType =
{
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    "HRTF objects. Reads one channel from a HRTFter.",           /* tp_doc */
    (traverseproc)HRTF_traverse,   /* tp_traverse */
    (inquiry)HRTF_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
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

static MYFLT BINAURAL_LEFT_HRTF_0[128] = {0.001495361328125, -0.00274658203125, 0.00384521484375, -0.005645751953125, 0.0067138671875, -0.00762939453125, 0.0091552734375, -0.00244140625, 0.25164794921875, 0.286895751953125, -0.32781982421875, -0.11444091796875, -0.044036865234375, -0.064453125, 0.348297119140625, 0.128021240234375, 0.244354248046875, 0.615478515625, 0.18951416015625, -0.406768798828125, -0.086944580078125, 0.039093017578125, -0.44964599609375, -0.373809814453125, -0.05902099609375, -0.112274169921875, -0.2093505859375, 0.0279541015625, 0.085418701171875, -0.010833740234375, 0.065460205078125, 0.149932861328125, 0.128326416015625, 0.08551025390625, 0.074798583984375, 0.000213623046875, -0.03741455078125, -0.003814697265625, 0.0484619140625, 0.002716064453125, -0.04498291015625, -0.040802001953125, -0.05401611328125, -0.08758544921875, -0.1322021484375, -0.15673828125, -0.12567138671875, -0.037139892578125, 0.0189208984375, 0.012725830078125, 0.027099609375, 0.061798095703125, 0.08172607421875, 0.06927490234375, 0.039825439453125, 0.02777099609375, 0.022369384765625, 0.00775146484375, -0.007568359375, -0.01116943359375, -0.022674560546875, -0.02850341796875, -0.024261474609375, -0.032989501953125, -0.035125732421875, -0.02197265625, -0.020355224609375, -0.024566650390625, -0.0133056640625, 0.000152587890625, -0.001190185546875, 0.003173828125, 0.015838623046875, 0.023162841796875, 0.016754150390625, 0.004302978515625, -0.00067138671875, 0.0006103515625, 0.000518798828125, -0.00396728515625, -0.0050048828125, -0.01373291015625, -0.0205078125, -0.01605224609375, -0.011871337890625, -0.010650634765625, -0.005462646484375, -0.000396728515625, -0.002685546875, -0.00579833984375, 0.00146484375, 0.01007080078125, 0.01043701171875, 0.0076904296875, 0.00341796875, -0.0009765625, -0.002838134765625, -0.003448486328125, -0.007049560546875, -0.00885009765625, -0.01025390625, -0.014923095703125, -0.015106201171875, -0.00787353515625, -0.000732421875, 0.002532958984375, 6.103515625e-05, -0.002777099609375, -0.002899169921875, -0.003326416015625, -0.00408935546875, -0.002899169921875, -0.003021240234375, -0.0045166015625, -0.00299072265625, -0.0018310546875, -0.001220703125, -0.000244140625, -0.000244140625, -0.0023193359375, -0.004180908203125, -0.003448486328125, -0.001556396484375, 0.000701904296875, 0.00189208984375, 0.000762939453125, 0.000274658203125, -0.00067138671875};
static MYFLT BINAURAL_RIGHT_HRTF_0[128] = {-9.1552734375e-05, 3.0517578125e-05, 0.0, -9.1552734375e-05, 6.103515625e-05, 0.0, 6.103515625e-05, 0.000152587890625, -9.1552734375e-05, 0.0003662109375, -0.000579833984375, 0.00091552734375, -0.00115966796875, 0.00128173828125, -0.001251220703125, 0.002288818359375, 0.007598876953125, 0.104827880859375, 0.082183837890625, -0.091400146484375, 0.0135498046875, -0.01043701171875, -0.06341552734375, 0.109832763671875, 0.106414794921875, 0.074554443359375, 0.20806884765625, 0.2091064453125, -0.054840087890625, -0.069061279296875, 0.0552978515625, -0.111175537109375, -0.21051025390625, -0.06414794921875, -0.045440673828125, -0.1259765625, -0.04718017578125, 0.016204833984375, -0.008697509765625, 0.00592041015625, 0.058074951171875, 0.05810546875, 0.037506103515625, 0.05841064453125, 0.04876708984375, 0.0037841796875, 0.00311279296875, 0.024078369140625, 0.00262451171875, -0.019561767578125, -0.022857666015625, -0.03192138671875, -0.048431396484375, -0.05194091796875, -0.04083251953125, -0.03619384765625, -0.028961181640625, -0.01556396484375, -0.005462646484375, 0.0013427734375, 0.004486083984375, 0.00335693359375, -0.001434326171875, -0.00390625, -0.001312255859375, 0.003326416015625, -0.000518798828125, -0.006072998046875, -0.0037841796875, -0.0054931640625, -0.010833740234375, -0.010955810546875, -0.012359619140625, -0.013336181640625, -0.00750732421875, -0.00286865234375, -0.004302978515625, -0.00390625, 0.0008544921875, 0.001983642578125, 3.0517578125e-05, 0.0, 0.001129150390625, -0.000762939453125, -0.00408935546875, -0.005096435546875, -0.00439453125, -0.006011962890625, -0.007354736328125, -0.005340576171875, -0.004913330078125, -0.006256103515625, -0.004913330078125, -0.003509521484375, -0.004119873046875, -0.003387451171875, -0.000732421875, -0.00091552734375, -0.003509521484375, -0.002410888671875, -0.000274658203125, -0.001220703125, -0.002349853515625, -0.002105712890625, -0.003692626953125, -0.005279541015625, -0.005035400390625, -0.0045166015625, -0.00390625, -0.003204345703125, -0.003509521484375, -0.004150390625, -0.002838134765625, -0.000946044921875, -0.000457763671875, -0.001434326171875, -0.001953125, -0.00128173828125, -0.001861572265625, -0.003326416015625, -0.00341796875, -0.0035400390625, -0.004669189453125, -0.005035400390625, -0.004669189453125, -0.00445556640625, -0.0037841796875, -0.002471923828125};
static MYFLT BINAURAL_LEFT_HRTF_1[128] = {0.0, -9.1552734375e-05, -9.1552734375e-05, 0.000152587890625, -6.103515625e-05, 0.0, 0.0001220703125, -0.0001220703125, 0.000396728515625, -0.00067138671875, 0.001007080078125, -0.0013427734375, 0.00152587890625, -0.001800537109375, 0.002593994140625, 0.009033203125, 0.12103271484375, 0.087799072265625, -0.11053466796875, 0.0184326171875, -0.01690673828125, -0.074676513671875, 0.132568359375, 0.116790771484375, 0.0811767578125, 0.237457275390625, 0.221466064453125, -0.079803466796875, -0.079742431640625, 0.05810546875, -0.14154052734375, -0.240325927734375, -0.063201904296875, -0.0462646484375, -0.138427734375, -0.041015625, 0.0322265625, -0.00762939453125, 0.008575439453125, 0.0657958984375, 0.060760498046875, 0.038238525390625, 0.061126708984375, 0.050537109375, -0.0001220703125, 0.001068115234375, 0.023162841796875, -0.00189208984375, -0.021484375, -0.02191162109375, -0.037628173828125, -0.05670166015625, -0.05767822265625, -0.04595947265625, -0.03436279296875, -0.02252197265625, -0.0074462890625, -0.00018310546875, 0.004302978515625, 0.0087890625, 0.007049560546875, 0.001068115234375, -0.002105712890625, -0.0009765625, 6.103515625e-05, -0.00274658203125, -0.006927490234375, -0.0072021484375, -0.00994873046875, -0.01458740234375, -0.013397216796875, -0.012847900390625, -0.011474609375, -0.006256103515625, -0.0035400390625, -0.00396728515625, -0.00360107421875, 0.000335693359375, 0.00244140625, 0.002105712890625, 0.001983642578125, 0.002166748046875, -0.0003662109375, -0.00433349609375, -0.00628662109375, -0.005035400390625, -0.006072998046875, -0.00799560546875, -0.0054931640625, -0.00421142578125, -0.005523681640625, -0.004852294921875, -0.003082275390625, -0.004241943359375, -0.004486083984375, -0.00213623046875, -0.00225830078125, -0.004669189453125, -0.002532958984375, 0.001129150390625, 0.000274658203125, -0.001922607421875, -0.002105712890625, -0.004119873046875, -0.005950927734375, -0.005218505859375, -0.004730224609375, -0.004058837890625, -0.003173828125, -0.0029296875, -0.003173828125, -0.00225830078125, -0.000762939453125, -9.1552734375e-05, -0.000701904296875, -0.001861572265625, -0.00152587890625, -0.002349853515625, -0.004119873046875, -0.004608154296875, -0.00518798828125, -0.005767822265625, -0.005615234375, -0.004119873046875, -0.003662109375, -0.003082275390625, -0.00225830078125, -0.002288818359375};
static MYFLT BINAURAL_RIGHT_HRTF_1[128] = {-0.000396728515625, 0.00091552734375, -0.001617431640625, 0.0025634765625, -0.00482177734375, 0.0067138671875, -0.008453369140625, 0.00946044921875, 0.01959228515625, 0.327728271484375, 0.1124267578125, -0.35467529296875, 0.038360595703125, -0.128753662109375, 0.01007080078125, 0.373565673828125, 0.08013916015625, 0.285888671875, 0.59112548828125, 0.057159423828125, -0.4461669921875, 0.031463623046875, 0.00518798828125, -0.520355224609375, -0.30059814453125, 0.01983642578125, -0.16802978515625, -0.216217041015625, 0.092742919921875, 0.0650634765625, -0.02911376953125, 0.086151123046875, 0.1732177734375, 0.10400390625, 0.075653076171875, 0.109283447265625, -0.013031005859375, -0.080047607421875, 0.019073486328125, 0.052825927734375, -0.032928466796875, -0.04632568359375, -0.031280517578125, -0.066375732421875, -0.09326171875, -0.09796142578125, -0.132080078125, -0.1309814453125, -0.039398193359375, 0.025787353515625, 0.012725830078125, 0.026153564453125, 0.061065673828125, 0.07781982421875, 0.063507080078125, 0.037689208984375, 0.028411865234375, 0.0191650390625, 0.0018310546875, -0.006256103515625, -0.0098876953125, -0.025177001953125, -0.025604248046875, -0.022552490234375, -0.03424072265625, -0.0322265625, -0.019195556640625, -0.022857666015625, -0.027557373046875, -0.01287841796875, -0.00054931640625, -0.001220703125, 0.0057373046875, 0.017120361328125, 0.021484375, 0.013824462890625, 0.005126953125, 0.001220703125, 0.0020751953125, -0.00213623046875, -0.00531005859375, -0.004974365234375, -0.014404296875, -0.017913818359375, -0.014007568359375, -0.01220703125, -0.011688232421875, -0.0047607421875, -9.1552734375e-05, -0.004608154296875, -0.005767822265625, 0.003814697265625, 0.010223388671875, 0.008514404296875, 0.006072998046875, 0.00299072265625, -0.001739501953125, -0.002655029296875, -0.003265380859375, -0.0074462890625, -0.008880615234375, -0.009674072265625, -0.013580322265625, -0.013214111328125, -0.006103515625, 0.000244140625, 0.0015869140625, -0.001983642578125, -0.003265380859375, -0.00250244140625, -0.003570556640625, -0.003814697265625, -0.001861572265625, -0.00311279296875, -0.00469970703125, -0.00262451171875, -0.001861572265625, -0.0013427734375, 0.000457763671875, 0.000213623046875, -0.0018310546875, -0.002777099609375, -0.00225830078125, -0.001953125, -0.000457763671875, -0.0003662109375, -0.001434326171875, 0.0003662109375};
static MYFLT BINAURAL_LEFT_HRTF_2[128] = {9.1552734375e-05, 9.1552734375e-05, 9.1552734375e-05, 0.000152587890625, 0.0001220703125, 0.000152587890625, 0.000152587890625, 0.0001220703125, 0.0001220703125, 9.1552734375e-05, 0.000152587890625, 0.00018310546875, 0.00018310546875, 0.000152587890625, 0.000244140625, 9.1552734375e-05, 0.000274658203125, 0.0001220703125, 0.000457763671875, 0.000213623046875, 0.00018310546875, 0.000396728515625, 0.001007080078125, -0.000213623046875, 0.00042724609375, 0.0020751953125, 0.03070068359375, 0.04376220703125, -0.002777099609375, 0.013671875, 0.00799560546875, -0.02685546875, 0.019683837890625, 0.0455322265625, 0.049652099609375, 0.0885009765625, 0.11865234375, 0.05828857421875, 0.01861572265625, 0.0484619140625, 0.002838134765625, -0.06341552734375, -0.048980712890625, -0.03704833984375, -0.0548095703125, -0.024810791015625, 0.01007080078125, 0.005157470703125, -0.0006103515625, 0.01025390625, 0.01556396484375, 0.0010986328125, -0.00714111328125, -0.01190185546875, -0.021026611328125, -0.018341064453125, -0.012908935546875, -0.0157470703125, -0.013671875, -0.006072998046875, -0.006195068359375, -0.015533447265625, -0.021484375, -0.01715087890625, -0.01123046875, -0.010650634765625, -0.00860595703125, -0.0023193359375, 0.001495361328125, 0.002410888671875, 0.001373291015625, -0.000701904296875, -0.004150390625, -0.008026123046875, -0.0089111328125, -0.00885009765625, -0.0093994140625, -0.00897216796875, -0.008575439453125, -0.008392333984375, -0.0069580078125, -0.005584716796875, -0.006011962890625, -0.00482177734375, -0.00238037109375, -0.0020751953125, -0.002532958984375, -0.00128173828125, -0.000244140625, -0.00054931640625, -0.00115966796875, -0.001617431640625, -0.002227783203125, -0.003875732421875, -0.0057373046875, -0.0067138671875, -0.0068359375, -0.00616455078125, -0.004852294921875, -0.003875732421875, -0.003936767578125, -0.003631591796875, -0.003143310546875, -0.002471923828125, -0.001983642578125, -0.00146484375, -0.000946044921875, -0.0010986328125, -0.00048828125, -0.000335693359375, -0.000823974609375, -0.00128173828125, -0.001708984375, -0.002685546875, -0.00421142578125, -0.00482177734375, -0.0052490234375, -0.0054931640625, -0.00518798828125, -0.004913330078125, -0.00445556640625, -0.00341796875, -0.0018310546875, -0.00067138671875, 0.0, 9.1552734375e-05, -0.000518798828125, -0.001129150390625};
static MYFLT BINAURAL_RIGHT_HRTF_2[128] = {0.0035400390625, -0.006011962890625, 0.006072998046875, 0.04205322265625, 0.45623779296875, 0.1639404296875, -0.609344482421875, -0.19586181640625, 0.243865966796875, 0.130645751953125, 0.048248291015625, 0.598907470703125, 0.64190673828125, -0.161163330078125, -0.14251708984375, 0.108551025390625, -0.12890625, -0.248931884765625, -0.22149658203125, -0.3280029296875, -0.216064453125, -0.08770751953125, -0.0576171875, -0.04193115234375, -0.060943603515625, 0.0587158203125, 0.0606689453125, 0.04071044921875, 0.12457275390625, 0.171173095703125, 0.0845947265625, 0.038970947265625, 0.06536865234375, 0.03607177734375, -0.04351806640625, -0.12396240234375, -0.13006591796875, -0.141204833984375, -0.13751220703125, -0.1292724609375, -0.084014892578125, -0.008758544921875, 0.03460693359375, 0.038177490234375, 0.02813720703125, 0.033355712890625, 0.04345703125, 0.046722412109375, 0.0384521484375, 0.037750244140625, 0.02655029296875, 0.0018310546875, -0.00738525390625, -0.00567626953125, -0.021148681640625, -0.03851318359375, -0.04071044921875, -0.04803466796875, -0.053802490234375, -0.03619384765625, -0.010345458984375, -0.000152587890625, 0.00213623046875, 0.008636474609375, 0.01458740234375, 0.015380859375, 0.018310546875, 0.024810791015625, 0.018524169921875, 0.0028076171875, -0.004852294921875, -0.00640869140625, -0.01483154296875, -0.013580322265625, -0.004638671875, -0.00732421875, -0.014862060546875, -0.01409912109375, -0.0081787109375, -0.007354736328125, -0.00177001953125, 0.00701904296875, 0.007080078125, 0.001068115234375, -0.001678466796875, 0.000213623046875, 0.0064697265625, 0.01116943359375, 0.0086669921875, -0.0028076171875, -0.013519287109375, -0.014739990234375, -0.01239013671875, -0.0081787109375, -0.00482177734375, -0.005462646484375, -0.01202392578125, -0.0157470703125, -0.008880615234375, 0.002532958984375, 0.00787353515625, 0.003997802734375, -0.0013427734375, -0.004974365234375, -0.004241943359375, 0.000152587890625, 0.001251220703125, -0.002044677734375, -0.0078125, -0.011260986328125, -0.0107421875, -0.00677490234375, -0.00213623046875, -0.00054931640625, -0.001678466796875, -0.003936767578125, -0.001251220703125, 0.004302978515625, 0.00567626953125, 0.00445556640625, 0.002655029296875, -0.001068115234375, -0.004547119140625, -0.002838134765625, 0.001373291015625, 0.001556396484375, 0.0003662109375, -0.000518798828125};
static MYFLT BINAURAL_LEFT_HRTF_3[128] = {0.0, 0.0, 3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 0.0, 0.0, 0.0, 0.0, -3.0517578125e-05, 3.0517578125e-05, 0.0, 0.0, 9.1552734375e-05, 3.0517578125e-05, 3.0517578125e-05, 6.103515625e-05, 9.1552734375e-05, -0.0001220703125, 0.0001220703125, 0.000335693359375, -6.103515625e-05, 0.00030517578125, 0.00042724609375, 0.00030517578125, 0.00518798828125, 0.03375244140625, 0.027130126953125, -0.0166015625, -0.00537109375, 0.0194091796875, 0.010223388671875, 0.002410888671875, 0.022186279296875, 0.068359375, 0.090179443359375, 0.063018798828125, 0.0406494140625, 0.021820068359375, 0.00347900390625, -0.00848388671875, -0.027587890625, -0.013397216796875, 0.01397705078125, 0.01953125, 0.02734375, 0.028167724609375, 0.00750732421875, -0.017242431640625, -0.02508544921875, -0.031494140625, -0.043060302734375, -0.045684814453125, -0.037933349609375, -0.032135009765625, -0.0247802734375, -0.00946044921875, -0.00091552734375, 0.003814697265625, 0.010162353515625, 0.014617919921875, 0.0113525390625, 0.003875732421875, 0.00042724609375, -0.002838134765625, -0.007659912109375, -0.009674072265625, -0.009674072265625, -0.01153564453125, -0.014495849609375, -0.014404296875, -0.0140380859375, -0.014312744140625, -0.015411376953125, -0.013885498046875, -0.009735107421875, -0.00689697265625, -0.002593994140625, -0.000579833984375, -0.000152587890625, 9.1552734375e-05, -0.0003662109375, -0.001678466796875, -0.00384521484375, -0.003692626953125, -0.003265380859375, -0.00445556640625, -0.00567626953125, -0.00579833984375, -0.005096435546875, -0.004638671875, -0.00396728515625, -0.005035400390625, -0.005218505859375, -0.00396728515625, -0.00238037109375, -0.0018310546875, -0.002685546875, -0.002410888671875, -0.00323486328125, -0.003662109375, -0.002899169921875, -0.00250244140625, -0.00299072265625, -0.003692626953125, -0.004302978515625, -0.005706787109375, -0.00628662109375, -0.0054931640625, -0.0045166015625, -0.004119873046875, -0.003631591796875, -0.0030517578125, -0.003021240234375, -0.00299072265625, -0.003662109375, -0.0040283203125, -0.00347900390625, -0.00250244140625, -0.0018310546875, -0.00103759765625, -0.00030517578125, -0.0001220703125, -0.00054931640625, -0.00146484375, -0.002227783203125, -0.002960205078125};
static MYFLT BINAURAL_RIGHT_HRTF_3[128] = {0.006805419921875, -0.01202392578125, 0.015869140625, -0.000701904296875, 0.4110107421875, 0.196197509765625, -0.6798095703125, -0.141845703125, 0.41094970703125, 0.067474365234375, 0.091339111328125, 0.6279296875, 0.158172607421875, -0.427703857421875, 0.192779541015625, 0.490203857421875, -0.089202880859375, -0.30572509765625, -0.111572265625, -0.240386962890625, -0.156768798828125, -0.0015869140625, -0.2349853515625, -0.29754638671875, -0.06329345703125, 0.091156005859375, 0.03997802734375, 0.03857421875, 0.137481689453125, 0.11383056640625, 0.059783935546875, 0.071990966796875, 0.016815185546875, -0.09197998046875, -0.050048828125, -0.014801025390625, -0.056060791015625, -0.09796142578125, -0.089111328125, -0.05169677734375, -0.026641845703125, 0.004974365234375, 0.0062255859375, -0.00933837890625, -0.0078125, 0.01385498046875, 0.02081298828125, 0.021240234375, 0.024444580078125, 0.0233154296875, 0.010162353515625, -0.00506591796875, -0.0040283203125, 0.001922607421875, -0.01617431640625, -0.02471923828125, -0.027374267578125, -0.036163330078125, -0.036285400390625, -0.016387939453125, -3.0517578125e-05, -0.005096435546875, -0.01068115234375, -0.003662109375, 0.00787353515625, 0.006927490234375, 0.012725830078125, 0.020233154296875, 0.01318359375, -0.002777099609375, -0.0030517578125, -0.00128173828125, -0.0089111328125, -0.006866455078125, 0.003082275390625, -0.005401611328125, -0.0162353515625, -0.0106201171875, -0.003814697265625, -0.003936767578125, 0.000701904296875, 0.00555419921875, -0.002593994140625, -0.007904052734375, -0.00390625, 0.0006103515625, 0.003082275390625, 0.0079345703125, 0.005096435546875, -0.004638671875, -0.009735107421875, -0.00579833984375, -0.00103759765625, 0.000762939453125, -0.00244140625, -0.00927734375, -0.015594482421875, -0.01458740234375, -0.00396728515625, 0.004669189453125, 0.0028076171875, -0.0050048828125, -0.009124755859375, -0.00927734375, -0.00439453125, 0.00213623046875, 0.00299072265625, -0.0037841796875, -0.0107421875, -0.012176513671875, -0.00726318359375, -0.0008544921875, 0.00244140625, 0.000579833984375, -0.003875732421875, -0.0048828125, -0.000823974609375, 0.003875732421875, 0.002899169921875, -0.000213623046875, -0.0023193359375, -0.004241943359375, -0.00457763671875, -0.00067138671875, 0.002288818359375, 0.000885009765625, -0.00140380859375, -0.002105712890625};
static MYFLT BINAURAL_LEFT_HRTF_4[128] = {-9.1552734375e-05, -6.103515625e-05, -6.103515625e-05, -3.0517578125e-05, 0.0, -9.1552734375e-05, -6.103515625e-05, 9.1552734375e-05, -3.0517578125e-05, 0.0001220703125, -0.00018310546875, 0.000335693359375, -0.000518798828125, 0.00054931640625, -0.000762939453125, 0.0009765625, -0.00091552734375, 0.001800537109375, 0.001617431640625, 0.065093994140625, 0.124969482421875, -0.031524658203125, -0.102630615234375, 0.01483154296875, 0.02691650390625, -0.031829833984375, 0.1041259765625, 0.27166748046875, 0.127410888671875, -0.00421142578125, 0.064910888671875, 0.03521728515625, -0.062774658203125, -0.06097412109375, -0.08074951171875, -0.136322021484375, -0.093994140625, -0.04412841796875, -0.061431884765625, -0.054290771484375, 0.000335693359375, 0.026458740234375, 0.013153076171875, 0.038726806640625, 0.06103515625, 0.0341796875, 0.010986328125, 0.0140380859375, 0.009918212890625, -0.00494384765625, -0.008941650390625, -0.010162353515625, -0.01397705078125, -0.018280029296875, -0.02069091796875, -0.02203369140625, -0.017852783203125, -0.00634765625, 0.000335693359375, -0.00250244140625, -0.0050048828125, -0.004730224609375, -0.006439208984375, -0.009124755859375, -0.0074462890625, -0.0079345703125, -0.014556884765625, -0.016204833984375, -0.008056640625, -0.003631591796875, -0.004638671875, -0.000732421875, 0.001129150390625, -0.00201416015625, -0.003997802734375, -0.000885009765625, 0.001739501953125, -0.00115966796875, -0.00360107421875, -0.0035400390625, -0.004669189453125, -0.006103515625, -0.004547119140625, -0.003021240234375, -0.004486083984375, -0.00567626953125, -0.006591796875, -0.0087890625, -0.008636474609375, -0.00323486328125, -0.000457763671875, -0.003692626953125, -0.004791259765625, -0.003265380859375, -0.002288818359375, -0.002288818359375, -0.00140380859375, -0.00030517578125, -0.001251220703125, -0.0025634765625, -0.003814697265625, -0.005523681640625, -0.00592041015625, -0.00494384765625, -0.0052490234375, -0.00689697265625, -0.00732421875, -0.00579833984375, -0.0030517578125, -0.00030517578125, 0.000701904296875, -0.00140380859375, -0.004608154296875, -0.005340576171875, -0.003631591796875, -0.00177001953125, -0.001678466796875, -0.002838134765625, -0.0042724609375, -0.0050048828125, -0.00439453125, -0.0035400390625, -0.003082275390625, -0.00323486328125, -0.00396728515625, -0.004058837890625, -0.003082275390625, -0.00146484375};
static MYFLT BINAURAL_RIGHT_HRTF_4[128] = {-0.0003662109375, 0.000762939453125, -0.000823974609375, 0.00054931640625, -0.000518798828125, 9.1552734375e-05, 9.1552734375e-05, -0.0006103515625, -3.0517578125e-05, 0.048583984375, 0.23309326171875, -0.003387451171875, -0.214019775390625, 0.074249267578125, 0.058868408203125, -0.117767333984375, 0.10491943359375, 0.393890380859375, 0.173553466796875, 0.067535400390625, 0.244873046875, 0.062957763671875, -0.1507568359375, -0.069122314453125, -0.112213134765625, -0.237060546875, -0.160003662109375, -0.129425048828125, -0.15869140625, -0.0908203125, -0.013824462890625, 0.01348876953125, 0.0216064453125, 0.0889892578125, 0.105682373046875, 0.04437255859375, 0.0323486328125, 0.03436279296875, -0.0015869140625, -0.032470703125, -0.029052734375, -0.031280517578125, -0.027618408203125, -0.038116455078125, -0.04833984375, -0.0577392578125, -0.04803466796875, -0.023773193359375, -0.020477294921875, -0.008819580078125, 0.01104736328125, 0.024200439453125, 0.024139404296875, 0.025146484375, 0.029998779296875, 0.02520751953125, 0.007476806640625, -0.00262451171875, -0.00341796875, -0.0135498046875, -0.021759033203125, -0.017364501953125, -0.016143798828125, -0.016326904296875, -0.014923095703125, -0.007354736328125, -0.003143310546875, -0.002410888671875, -0.00238037109375, -0.0015869140625, -0.000701904296875, -0.00201416015625, 0.00152587890625, 0.00421142578125, 0.000946044921875, -0.0032958984375, -0.0010986328125, -0.006622314453125, -0.014312744140625, -0.009979248046875, -0.0042724609375, -0.007232666015625, -0.00787353515625, -0.00531005859375, -0.0052490234375, -0.00469970703125, 0.001312255859375, 0.00360107421875, 0.000152587890625, 0.000518798828125, 0.002105712890625, 0.002197265625, 0.0025634765625, 0.0032958984375, 0.00091552734375, -0.002288818359375, -0.00421142578125, -0.005950927734375, -0.008087158203125, -0.009429931640625, -0.009796142578125, -0.00909423828125, -0.009521484375, -0.008819580078125, -0.00640869140625, -0.003753662109375, -0.00225830078125, -0.001800537109375, -0.00152587890625, -0.003021240234375, -0.003509521484375, -0.0029296875, -0.002655029296875, -0.003692626953125, -0.00469970703125, -0.005157470703125, -0.00457763671875, -0.003143310546875, -0.00164794921875, -0.000701904296875, -0.00054931640625, -0.0003662109375, 0.000213623046875, -0.0001220703125, -0.00054931640625, -0.000579833984375, -0.00054931640625, -0.001190185546875};
static MYFLT BINAURAL_LEFT_HRTF_5[128] = {0.00042724609375, -0.000579833984375, 0.00091552734375, -0.0010986328125, 0.0009765625, -0.001251220703125, 0.00103759765625, -0.001220703125, 0.001190185546875, -0.001708984375, 0.063140869140625, 0.21466064453125, -0.026580810546875, -0.15606689453125, 0.05072021484375, 0.00238037109375, -0.07891845703125, 0.139678955078125, 0.368194580078125, 0.183746337890625, 0.087493896484375, 0.180450439453125, 0.0306396484375, -0.10345458984375, -0.078338623046875, -0.15087890625, -0.206878662109375, -0.150360107421875, -0.140655517578125, -0.13055419921875, -0.0733642578125, -0.0213623046875, 0.02044677734375, 0.035888671875, 0.083465576171875, 0.09014892578125, 0.045623779296875, 0.03179931640625, 0.021759033203125, -0.004150390625, -0.0272216796875, -0.025726318359375, -0.025604248046875, -0.025299072265625, -0.033843994140625, -0.04132080078125, -0.05926513671875, -0.05377197265625, -0.029449462890625, -0.0186767578125, -0.00469970703125, 0.00970458984375, 0.01837158203125, 0.02276611328125, 0.027984619140625, 0.031585693359375, 0.024658203125, 0.007293701171875, -0.000640869140625, -0.003082275390625, -0.01239013671875, -0.018524169921875, -0.014068603515625, -0.012725830078125, -0.014129638671875, -0.013153076171875, -0.008270263671875, -0.0059814453125, -0.004730224609375, -0.005279541015625, -0.006927490234375, -0.006561279296875, -0.00482177734375, -0.0006103515625, 0.004486083984375, 0.00335693359375, 6.103515625e-05, 0.000244140625, -0.0050048828125, -0.011016845703125, -0.0089111328125, -0.004852294921875, -0.005889892578125, -0.004669189453125, -0.004425048828125, -0.007293701171875, -0.0084228515625, -0.002532958984375, 0.002105712890625, 0.00201416015625, 0.00274658203125, 0.0020751953125, 0.0010986328125, 0.0015869140625, 0.001708984375, 0.0, -0.002197265625, -0.003875732421875, -0.00531005859375, -0.006866455078125, -0.00726318359375, -0.0078125, -0.0087890625, -0.011016845703125, -0.010986328125, -0.008331298828125, -0.0048828125, -0.00201416015625, -0.000640869140625, -0.001251220703125, -0.0032958984375, -0.00360107421875, -0.002777099609375, -0.00213623046875, -0.00201416015625, -0.0030517578125, -0.004425048828125, -0.00421142578125, -0.00286865234375, -0.00103759765625, -3.0517578125e-05, -0.000152587890625, -0.0013427734375, -0.001556396484375, -0.001373291015625, -0.001373291015625, -0.000579833984375, -0.000701904296875};
static MYFLT BINAURAL_RIGHT_HRTF_5[128] = {-9.1552734375e-05, -6.103515625e-05, -3.0517578125e-05, -6.103515625e-05, -9.1552734375e-05, -0.0001220703125, 6.103515625e-05, 0.0, 0.00018310546875, -0.000213623046875, 0.000396728515625, -0.00054931640625, 0.00054931640625, -0.00079345703125, 0.001007080078125, -0.000946044921875, 0.00189208984375, 0.000946044921875, 0.0633544921875, 0.13848876953125, -0.02117919921875, -0.117889404296875, 0.003814697265625, 0.03314208984375, -0.0338134765625, 0.105743408203125, 0.300628662109375, 0.138336181640625, -0.01348876953125, 0.07281494140625, 0.044403076171875, -0.072174072265625, -0.074005126953125, -0.0826416015625, -0.14324951171875, -0.10943603515625, -0.04754638671875, -0.063079833984375, -0.0623779296875, 0.000640869140625, 0.0302734375, 0.012359619140625, 0.038330078125, 0.065093994140625, 0.04046630859375, 0.011871337890625, 0.0113525390625, 0.009521484375, -0.007720947265625, -0.00909423828125, -0.006195068359375, -0.017974853515625, -0.0269775390625, -0.026031494140625, -0.03350830078125, -0.0289306640625, -0.004241943359375, 0.009552001953125, 0.006866455078125, 0.002838134765625, 0.00677490234375, 0.0059814453125, -0.002166748046875, -0.00665283203125, -0.0108642578125, -0.014984130859375, -0.015777587890625, -0.01605224609375, -0.014434814453125, -0.008697509765625, -0.002532958984375, -0.0020751953125, -0.00653076171875, -0.004913330078125, 0.003143310546875, 0.004730224609375, 0.000640869140625, -0.00115966796875, -0.000885009765625, -0.002532958984375, -0.00457763671875, -0.003631591796875, -0.000640869140625, -0.002166748046875, -0.006317138671875, -0.008544921875, -0.011260986328125, -0.01129150390625, -0.00640869140625, -0.00274658203125, -0.004364013671875, -0.005767822265625, -0.0032958984375, -0.001922607421875, -0.00244140625, -0.000335693359375, 0.00164794921875, 0.001129150390625, -0.0013427734375, -0.003631591796875, -0.00518798828125, -0.00445556640625, -0.002960205078125, -0.003662109375, -0.00579833984375, -0.0076904296875, -0.00677490234375, -0.00433349609375, -0.001983642578125, -0.001617431640625, -0.00390625, -0.00677490234375, -0.007080078125, -0.003997802734375, -0.000946044921875, -0.00030517578125, -0.00140380859375, -0.0032958984375, -0.004364013671875, -0.003875732421875, -0.0028076171875, -0.002655029296875, -0.003631591796875, -0.0045166015625, -0.00469970703125, -0.003692626953125, -0.00164794921875, 0.000396728515625};
static MYFLT BINAURAL_LEFT_HRTF_6[128] = {0.00579833984375, -0.0091552734375, 0.01190185546875, -0.01702880859375, 0.209686279296875, 0.427032470703125, -0.417877197265625, -0.456085205078125, 0.318115234375, 0.23236083984375, -0.040252685546875, 0.43212890625, 0.43597412109375, -0.326690673828125, -0.054962158203125, 0.516937255859375, 0.150665283203125, -0.31646728515625, -0.1697998046875, -0.157745361328125, -0.21533203125, -0.04119873046875, -0.146484375, -0.308349609375, -0.158721923828125, 0.059722900390625, 0.068572998046875, 0.015655517578125, 0.105560302734375, 0.13311767578125, 0.07177734375, 0.0638427734375, 0.05322265625, -0.06219482421875, -0.08642578125, -0.017120361328125, -0.0263671875, -0.08575439453125, -0.10247802734375, -0.055938720703125, -0.030303955078125, -0.006439208984375, 0.005584716796875, -0.00274658203125, -0.009613037109375, 0.0081787109375, 0.023406982421875, 0.0205078125, 0.021514892578125, 0.02667236328125, 0.01513671875, -0.005706787109375, -0.0081787109375, -0.000762939453125, -0.013946533203125, -0.02740478515625, -0.024169921875, -0.028839111328125, -0.034454345703125, -0.02239990234375, -0.00048828125, -0.0006103515625, -0.008209228515625, -0.007568359375, 0.004669189453125, 0.0078125, 0.00848388671875, 0.01751708984375, 0.016571044921875, 0.000762939453125, -0.00555419921875, 0.0, -0.00543212890625, -0.009307861328125, -0.000518798828125, -0.00201416015625, -0.015411376953125, -0.013671875, -0.0048828125, -0.003570556640625, -0.002288818359375, 0.004730224609375, 0.001007080078125, -0.006622314453125, -0.004730224609375, 0.000885009765625, 0.002899169921875, 0.005401611328125, 0.00634765625, -0.000640869140625, -0.008148193359375, -0.007415771484375, -0.0028076171875, -0.000244140625, -0.001800537109375, -0.006988525390625, -0.01312255859375, -0.01593017578125, -0.00750732421875, 0.0029296875, 0.0042724609375, -0.004547119140625, -0.00946044921875, -0.008270263671875, -0.00421142578125, 0.001708984375, 0.002716064453125, -0.00238037109375, -0.009063720703125, -0.010589599609375, -0.0081787109375, -0.00408935546875, -0.00054931640625, 0.00030517578125, -0.001861572265625, -0.004058837890625, -0.0029296875, 0.001129150390625, 0.0025634765625, 0.001220703125, -0.000518798828125, -0.0028076171875, -0.004364013671875, -0.001678466796875, 0.002655029296875, 0.002349853515625, -0.000396728515625, -0.00201416015625};
static MYFLT BINAURAL_RIGHT_HRTF_6[128] = {3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 9.1552734375e-05, 3.0517578125e-05, 6.103515625e-05, 6.103515625e-05, 0.0, 3.0517578125e-05, 0.0, 3.0517578125e-05, 6.103515625e-05, -3.0517578125e-05, 3.0517578125e-05, 9.1552734375e-05, 3.0517578125e-05, 6.103515625e-05, 9.1552734375e-05, 0.000152587890625, -0.0001220703125, 0.000274658203125, 6.103515625e-05, 0.000335693359375, 0.000152587890625, 0.000823974609375, 0.002197265625, 0.031494140625, 0.041015625, -0.01324462890625, -0.01690673828125, 0.0194091796875, 0.013671875, -0.00091552734375, 0.015838623046875, 0.069793701171875, 0.099456787109375, 0.059783935546875, 0.027069091796875, 0.028656005859375, 0.02667236328125, 0.00360107421875, -0.02874755859375, -0.039581298828125, -0.036041259765625, -0.02056884765625, 0.00592041015625, 0.017730712890625, 0.025238037109375, 0.034210205078125, 0.016998291015625, -0.007537841796875, -0.005706787109375, -0.010894775390625, -0.03570556640625, -0.0455322265625, -0.033660888671875, -0.02349853515625, -0.0238037109375, -0.016448974609375, -0.006561279296875, -0.001861572265625, 0.001068115234375, 0.002044677734375, 0.000335693359375, 0.001251220703125, 0.00372314453125, 0.000762939453125, -0.00360107421875, -0.0045166015625, -0.0048828125, -0.0084228515625, -0.010772705078125, -0.0115966796875, -0.014190673828125, -0.01617431640625, -0.01556396484375, -0.01190185546875, -0.008636474609375, -0.007110595703125, -0.004913330078125, -0.003387451171875, -0.002655029296875, -0.0028076171875, -0.0020751953125, -0.000396728515625, 0.000732421875, 0.000213623046875, -0.00213623046875, -0.004150390625, -0.0035400390625, -0.00286865234375, -0.004241943359375, -0.0052490234375, -0.0052490234375, -0.005157470703125, -0.00482177734375, -0.00396728515625, -0.00360107421875, -0.0045166015625, -0.005340576171875, -0.004791259765625, -0.003387451171875, -0.002471923828125, -0.00189208984375, -0.001312255859375, -0.00225830078125, -0.0042724609375, -0.0059814453125, -0.005950927734375, -0.005035400390625, -0.00457763671875, -0.00396728515625, -0.003875732421875, -0.003814697265625, -0.00372314453125, -0.0040283203125, -0.004119873046875, -0.003326416015625, -0.00201416015625, -0.00128173828125, -0.00103759765625, -0.000762939453125, 0.000152587890625, 0.000701904296875, -9.1552734375e-05, -0.001495361328125, -0.00299072265625, -0.0037841796875};
static MYFLT BINAURAL_LEFT_HRTF_7[128] = {-0.000640869140625, 0.0006103515625, -0.004058837890625, 0.128021240234375, 0.530426025390625, -0.12567138671875, -0.65423583984375, 0.032745361328125, 0.281463623046875, 0.0323486328125, 0.185272216796875, 0.778167724609375, 0.32952880859375, -0.30596923828125, 0.014068603515625, 0.102630615234375, -0.184295654296875, -0.231048583984375, -0.2530517578125, -0.313446044921875, -0.14556884765625, -0.06707763671875, -0.0531005859375, -0.100799560546875, -0.052978515625, 0.089080810546875, 0.0408935546875, 0.04644775390625, 0.149810791015625, 0.152130126953125, 0.058624267578125, 0.062896728515625, 0.071441650390625, 0.01861572265625, -0.07958984375, -0.131378173828125, -0.123199462890625, -0.13909912109375, -0.143157958984375, -0.12640380859375, -0.053985595703125, 0.011962890625, 0.03924560546875, 0.033477783203125, 0.024444580078125, 0.033660888671875, 0.047393798828125, 0.0447998046875, 0.037200927734375, 0.036834716796875, 0.016357421875, -0.003143310546875, -0.0047607421875, -0.0091552734375, -0.0302734375, -0.0413818359375, -0.04266357421875, -0.051361083984375, -0.049224853515625, -0.02703857421875, -0.00579833984375, -0.0018310546875, 0.002044677734375, 0.010833740234375, 0.01568603515625, 0.01422119140625, 0.02081298828125, 0.028106689453125, 0.01483154296875, -0.001708984375, -0.003631591796875, -0.007904052734375, -0.016632080078125, -0.010162353515625, -0.0035400390625, -0.0111083984375, -0.017486572265625, -0.013275146484375, -0.0084228515625, -0.006317138671875, 0.002349853515625, 0.009033203125, 0.004638671875, -0.000518798828125, -0.0008544921875, 0.002288818359375, 0.00762939453125, 0.010223388671875, 0.004791259765625, -0.007568359375, -0.01470947265625, -0.01336669921875, -0.01007080078125, -0.006500244140625, -0.004486083984375, -0.00750732421875, -0.014739990234375, -0.01580810546875, -0.005218505859375, 0.005706787109375, 0.006378173828125, 0.000946044921875, -0.003448486328125, -0.005462646484375, -0.003143310546875, 0.001251220703125, 0.000823974609375, -0.003936767578125, -0.009246826171875, -0.0120849609375, -0.00994873046875, -0.00506591796875, -0.0009765625, -0.00079345703125, -0.003265380859375, -0.004425048828125, -0.00030517578125, 0.0047607421875, 0.00482177734375, 0.004119873046875, 0.00164794921875, -0.0025634765625, -0.003936767578125, -0.00067138671875, 0.0023193359375, 0.0008544921875, -0.001312255859375, -0.002960205078125};
static MYFLT BINAURAL_RIGHT_HRTF_7[128] = {0.0, 0.0, 0.0, 0.0, 3.0517578125e-05, 6.103515625e-05, 6.103515625e-05, 3.0517578125e-05, 6.103515625e-05, 0.0, -3.0517578125e-05, 3.0517578125e-05, 0.0, -3.0517578125e-05, 9.1552734375e-05, 3.0517578125e-05, -3.0517578125e-05, 0.00018310546875, -0.00018310546875, 0.00042724609375, -3.0517578125e-05, 0.000152587890625, 0.000396728515625, 0.000396728515625, -0.0001220703125, 6.103515625e-05, 0.000823974609375, 0.01776123046875, 0.044921875, 0.0125732421875, 0.004150390625, 0.01123046875, -0.017242431640625, 0.0166015625, 0.04168701171875, 0.03369140625, 0.06707763671875, 0.118255615234375, 0.08856201171875, 0.030517578125, 0.041107177734375, 0.021148681640625, -0.046417236328125, -0.05108642578125, -0.01947021484375, -0.027496337890625, -0.0299072265625, -0.01727294921875, -0.01885986328125, -0.018218994140625, -0.005889892578125, -0.002471923828125, -0.0189208984375, -0.02227783203125, -0.010528564453125, -0.008697509765625, -0.01123046875, -0.004669189453125, 0.001007080078125, 0.0023193359375, 0.004364013671875, 0.000213623046875, -0.0103759765625, -0.01715087890625, -0.0169677734375, -0.01544189453125, -0.017181396484375, -0.0146484375, -0.00909423828125, -0.00665283203125, -0.004180908203125, -0.003204345703125, -0.00335693359375, -0.00537109375, -0.006103515625, -0.004302978515625, -0.003936767578125, -0.0057373046875, -0.00653076171875, -0.0054931640625, -0.00592041015625, -0.006317138671875, -0.00634765625, -0.006805419921875, -0.00604248046875, -0.005279541015625, -0.004364013671875, -0.004058837890625, -0.0029296875, -0.001312255859375, -0.001617431640625, -0.002044677734375, -0.00244140625, -0.00250244140625, -0.00390625, -0.005889892578125, -0.005767822265625, -0.005157470703125, -0.004547119140625, -0.004119873046875, -0.003936767578125, -0.003814697265625, -0.00384521484375, -0.003082275390625, -0.003448486328125, -0.00323486328125, -0.002227783203125, -0.001312255859375, -0.000518798828125, -0.00067138671875, -0.000457763671875, -0.000946044921875, -0.0010986328125, -0.001983642578125, -0.0037841796875, -0.004974365234375, -0.00604248046875, -0.00604248046875, -0.006011962890625, -0.005462646484375, -0.0050048828125, -0.0040283203125, -0.00262451171875, -0.002105712890625, -0.0010986328125, -0.000457763671875, -0.000579833984375, -0.001312255859375};
static MYFLT BINAURAL_LEFT_HRTF_8[128] = {0.00067138671875, -0.000579833984375, 0.001068115234375, -0.00225830078125, 0.00244140625, -0.004058837890625, 0.0054931640625, -0.007049560546875, 0.009246826171875, -0.0103759765625, 0.11614990234375, 0.259857177734375, 0.13519287109375, -0.340484619140625, -0.746337890625, 0.013397216796875, 0.544158935546875, 0.63995361328125, 0.60418701171875, 0.061553955078125, -0.182159423828125, -0.04443359375, 0.049713134765625, -0.14599609375, -0.23565673828125, -0.29925537109375, -0.24700927734375, -0.080291748046875, -0.081817626953125, -0.063232421875, -0.051513671875, 0.0032958984375, 0.054229736328125, 0.082366943359375, 0.1126708984375, 0.080596923828125, 0.078094482421875, 0.07037353515625, 0.0399169921875, -0.00341796875, -0.011566162109375, -0.0296630859375, -0.034820556640625, -0.06304931640625, -0.0843505859375, -0.04620361328125, -0.04388427734375, -0.035491943359375, -0.003631591796875, 0.01824951171875, 0.029449462890625, 0.03179931640625, -0.008026123046875, -0.000274658203125, 0.021697998046875, 0.016510009765625, 0.042388916015625, 0.030731201171875, -0.0113525390625, -0.025177001953125, -0.007110595703125, -0.01806640625, -0.0648193359375, -0.075042724609375, -0.04736328125, -0.0330810546875, -0.033905029296875, -0.025726318359375, -0.009307861328125, 0.017120361328125, 0.032745361328125, 0.02630615234375, 0.016143798828125, 0.008331298828125, 0.011016845703125, 0.013031005859375, 0.001739501953125, -0.0106201171875, -0.0147705078125, -0.018310546875, -0.015838623046875, -0.008636474609375, -0.006683349609375, -0.0093994140625, -0.015655517578125, -0.014129638671875, -0.009063720703125, -0.003265380859375, 0.00341796875, 0.010101318359375, 0.01019287109375, 0.00604248046875, 0.00286865234375, 0.000885009765625, 0.003326416015625, 0.003662109375, -0.001495361328125, -0.010284423828125, -0.014862060546875, -0.014312744140625, -0.011322021484375, -0.009033203125, -0.0101318359375, -0.01068115234375, -0.007781982421875, -0.003814697265625, 0.0006103515625, 0.002288818359375, 0.002166748046875, 0.002685546875, 0.000885009765625, -0.00164794921875, -0.002655029296875, -0.00390625, -0.00439453125, -0.00347900390625, -0.003143310546875, -0.003326416015625, -0.0032958984375, -0.002777099609375, -0.002685546875, -0.002197265625, -0.001861572265625, -0.0008544921875, 6.103515625e-05, 0.000457763671875, 0.001190185546875, 0.00152587890625};
static MYFLT BINAURAL_RIGHT_HRTF_8[128] = {-3.0517578125e-05, -6.103515625e-05, -9.1552734375e-05, -3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, -3.0517578125e-05, -3.0517578125e-05, 3.0517578125e-05, 0.0, 3.0517578125e-05, 0.0, -0.000152587890625, 0.000152587890625, -0.00048828125, 0.000762939453125, -0.0010986328125, 0.001190185546875, 0.003509521484375, 0.056243896484375, 0.0802001953125, 0.0343017578125, -0.110626220703125, -0.216156005859375, -0.064453125, 0.099761962890625, 0.25018310546875, 0.28179931640625, 0.1346435546875, 0.02618408203125, 0.028656005859375, 0.028656005859375, -0.019989013671875, -0.056854248046875, -0.105316162109375, -0.095550537109375, -0.07025146484375, -0.077239990234375, -0.060760498046875, -0.048187255859375, -0.032257080078125, -0.000335693359375, 0.02459716796875, 0.037353515625, 0.041778564453125, 0.04583740234375, 0.05291748046875, 0.05401611328125, 0.0343017578125, 0.00384521484375, -0.00933837890625, -0.0029296875, -0.017578125, -0.0350341796875, -0.034027099609375, -0.030029296875, -0.02801513671875, -0.019775390625, -0.014801025390625, -0.00982666015625, -0.002838134765625, -0.006011962890625, 0.009765625, 0.028289794921875, 0.018829345703125, 0.007080078125, 0.010955810546875, 0.007049560546875, -0.007720947265625, -0.018524169921875, -0.024322509765625, -0.025665283203125, -0.023193359375, -0.0216064453125, -0.0223388671875, -0.02081298828125, -0.0140380859375, -0.009796142578125, -0.00958251953125, -0.00732421875, -0.00335693359375, -0.003387451171875, -0.007354736328125, -0.006072998046875, -0.00433349609375, -0.004638671875, -0.005279541015625, -0.004058837890625, -0.004425048828125, -0.006591796875, -0.006988525390625, -0.006011962890625, -0.00494384765625, -0.005615234375, -0.006011962890625, -0.004547119140625, -0.001983642578125, -0.00067138671875, 0.00018310546875, 0.00067138671875, -9.1552734375e-05, -0.000518798828125, -0.000946044921875, -0.00091552734375, -0.000762939453125, -0.002288818359375, -0.004241943359375, -0.005340576171875, -0.005096435546875, -0.0057373046875, -0.006622314453125, -0.00616455078125, -0.0045166015625, -0.002960205078125, -0.00213623046875, -0.001708984375, -0.002105712890625, -0.001617431640625, -0.001068115234375, -0.000701904296875, -0.000946044921875, -0.00225830078125, -0.00347900390625, -0.00341796875, -0.002716064453125, -0.00225830078125, -0.001739501953125, -0.00250244140625};
static MYFLT BINAURAL_LEFT_HRTF_9[128] = {-9.1552734375e-05, -9.1552734375e-05, 0.0, 3.0517578125e-05, 6.103515625e-05, -6.103515625e-05, -6.103515625e-05, 0.00018310546875, -9.1552734375e-05, 0.000152587890625, -9.1552734375e-05, -0.000213623046875, 9.1552734375e-05, -0.00048828125, 0.000946044921875, -0.001190185546875, 0.00140380859375, 0.005889892578125, 0.073211669921875, 0.08807373046875, 0.02117919921875, -0.152374267578125, -0.238494873046875, -0.027801513671875, 0.136932373046875, 0.29473876953125, 0.3055419921875, 0.113311767578125, 0.00616455078125, 0.028564453125, 0.02447509765625, -0.032989501953125, -0.073760986328125, -0.128448486328125, -0.10723876953125, -0.07373046875, -0.080047607421875, -0.06463623046875, -0.049560546875, -0.023651123046875, 0.005218505859375, 0.026092529296875, 0.0479736328125, 0.056915283203125, 0.051544189453125, 0.049713134765625, 0.04754638671875, 0.02935791015625, -0.004730224609375, -0.023193359375, -0.0057373046875, -0.016510009765625, -0.034637451171875, -0.031158447265625, -0.025726318359375, -0.02142333984375, -0.0137939453125, -0.012908935546875, -0.011444091796875, 0.00079345703125, -0.0013427734375, 0.008148193359375, 0.029296875, 0.022918701171875, 0.004913330078125, 0.004058837890625, 0.004180908203125, -0.005340576171875, -0.01873779296875, -0.02764892578125, -0.027008056640625, -0.02294921875, -0.02227783203125, -0.021575927734375, -0.02081298828125, -0.017608642578125, -0.01153564453125, -0.007659912109375, -0.002685546875, -0.0009765625, -0.003509521484375, -0.008758544921875, -0.007049560546875, -0.0010986328125, -0.0032958984375, -0.007049560546875, -0.00482177734375, -0.00390625, -0.006256103515625, -0.005706787109375, -0.005859375, -0.0078125, -0.007476806640625, -0.00604248046875, -0.0042724609375, -0.003814697265625, -0.00238037109375, 0.00152587890625, 0.002471923828125, 0.001495361328125, -0.000518798828125, -0.00146484375, -0.000396728515625, 0.000274658203125, -0.0009765625, -0.00408935546875, -0.00567626953125, -0.0054931640625, -0.005950927734375, -0.0068359375, -0.0064697265625, -0.005157470703125, -0.00457763671875, -0.0030517578125, -0.001556396484375, -0.001678466796875, -0.00103759765625, -0.0008544921875, -0.000579833984375, -0.0009765625, -0.00238037109375, -0.003692626953125, -0.00335693359375, -0.00189208984375, -0.001190185546875, -0.0008544921875, -0.002593994140625, -0.004302978515625};
static MYFLT BINAURAL_RIGHT_HRTF_9[128] = {0.000244140625, 0.000152587890625, 0.0, 0.00018310546875, -0.0013427734375, 0.00140380859375, -0.00286865234375, 0.0045166015625, -0.00628662109375, 0.008575439453125, -0.003143310546875, 0.16748046875, 0.234466552734375, 0.03997802734375, -0.459197998046875, -0.6162109375, 0.202545166015625, 0.53826904296875, 0.634552001953125, 0.4959716796875, -0.03814697265625, -0.187744140625, -0.003631591796875, 0.018096923828125, -0.163238525390625, -0.224884033203125, -0.292327880859375, -0.173248291015625, -0.062225341796875, -0.090240478515625, -0.062774658203125, -0.05364990234375, 0.00433349609375, 0.063446044921875, 0.095611572265625, 0.093292236328125, 0.07318115234375, 0.088348388671875, 0.066864013671875, 0.0213623046875, -0.004364013671875, -0.005767822265625, -0.037078857421875, -0.04315185546875, -0.063140869140625, -0.0694580078125, -0.04840087890625, -0.04266357421875, -0.023345947265625, -0.001068115234375, 0.014984130859375, 0.02838134765625, 0.030792236328125, -0.0106201171875, -0.00738525390625, 0.01824951171875, 0.02899169921875, 0.050445556640625, 0.02947998046875, -0.011383056640625, -0.01812744140625, -0.006561279296875, -0.02191162109375, -0.0614013671875, -0.079864501953125, -0.065216064453125, -0.034637451171875, -0.025146484375, -0.03045654296875, -0.0108642578125, 0.024627685546875, 0.036590576171875, 0.0234375, 0.0150146484375, 0.010894775390625, 0.0108642578125, 0.008819580078125, -0.00042724609375, -0.01080322265625, -0.014892578125, -0.0159912109375, -0.013946533203125, -0.009368896484375, -0.007659912109375, -0.010528564453125, -0.01580810546875, -0.015625, -0.009490966796875, -0.001739501953125, 0.0047607421875, 0.010101318359375, 0.009033203125, 0.0059814453125, 0.00250244140625, 0.001007080078125, 0.003387451171875, 0.002044677734375, -0.00439453125, -0.01214599609375, -0.0146484375, -0.01373291015625, -0.010986328125, -0.008453369140625, -0.008514404296875, -0.00775146484375, -0.0057373046875, -0.0030517578125, 0.000274658203125, 0.002288818359375, 0.00244140625, 0.00164794921875, -0.000152587890625, -0.002105712890625, -0.002532958984375, -0.003997802734375, -0.005584716796875, -0.00567626953125, -0.004241943359375, -0.002044677734375, -0.002197265625, -0.003082275390625, -0.002838134765625, -0.00177001953125, -0.000946044921875, -6.103515625e-05, 0.000213623046875, 0.000213623046875, 0.000885009765625};
static MYFLT BINAURAL_LEFT_HRTF_10[128] = {0.0, 0.0, 0.0, 0.0, 0.0, 3.0517578125e-05, 3.0517578125e-05, -6.103515625e-05, -3.0517578125e-05, -6.103515625e-05, -3.0517578125e-05, 0.0, 0.0, -3.0517578125e-05, -9.1552734375e-05, -6.103515625e-05, 0.0, 0.0001220703125, 9.1552734375e-05, -9.1552734375e-05, -6.103515625e-05, -3.0517578125e-05, 0.000640869140625, 0.0, -0.00048828125, 0.001312255859375, 0.023193359375, 0.04345703125, 0.046966552734375, -0.04119873046875, -0.145843505859375, -0.075103759765625, 0.052764892578125, 0.128265380859375, 0.1397705078125, 0.097991943359375, 0.0562744140625, 0.051483154296875, 0.05914306640625, 0.0384521484375, 0.01580810546875, -0.000885009765625, -0.014801025390625, -0.035552978515625, -0.04632568359375, -0.044830322265625, -0.050506591796875, -0.045745849609375, -0.03399658203125, -0.013885498046875, -0.007659912109375, -0.009613037109375, 0.01373291015625, 0.0303955078125, 0.02484130859375, 0.020782470703125, 0.020904541015625, 0.011993408203125, 0.001922607421875, -0.003143310546875, -0.017669677734375, -0.0301513671875, -0.027435302734375, -0.018310546875, -0.015289306640625, -0.013458251953125, -0.00848388671875, -0.003875732421875, -0.00262451171875, -0.006439208984375, -0.00311279296875, -0.00018310546875, -0.0047607421875, -0.00738525390625, -0.00390625, -0.002685546875, -0.007598876953125, -0.013153076171875, -0.01409912109375, -0.01019287109375, -0.007659912109375, -0.0079345703125, -0.008392333984375, -0.007293701171875, -0.007171630859375, -0.008087158203125, -0.0081787109375, -0.006744384765625, -0.006256103515625, -0.007720947265625, -0.0091552734375, -0.0091552734375, -0.008392333984375, -0.009368896484375, -0.0096435546875, -0.007843017578125, -0.0045166015625, -0.001312255859375, -6.103515625e-05, -0.000335693359375, -0.001220703125, -0.001495361328125, -0.002349853515625, -0.003387451171875, -0.0040283203125, -0.004425048828125, -0.004180908203125, -0.003936767578125, -0.003692626953125, -0.0035400390625, -0.003173828125, -0.002288818359375, -0.0015869140625, -0.001434326171875, -0.00146484375, -0.00164794921875, -0.00213623046875, -0.00299072265625, -0.0030517578125, -0.002685546875, -0.0028076171875, -0.00323486328125, -0.0037841796875, -0.00439453125, -0.0047607421875, -0.0040283203125, -0.002899169921875, -0.001800537109375};
static MYFLT BINAURAL_RIGHT_HRTF_10[128] = {-0.0009765625, 0.00103759765625, -0.002288818359375, 0.00360107421875, -0.005584716796875, 0.007843017578125, -0.01220703125, 0.12078857421875, 0.3291015625, 0.140960693359375, -0.6568603515625, -0.626007080078125, 0.440826416015625, 0.541961669921875, 0.38568115234375, 0.331634521484375, 0.094818115234375, -0.006256103515625, 0.082733154296875, 0.1693115234375, -0.1680908203125, -0.297149658203125, -0.2236328125, -0.204620361328125, -0.228668212890625, -0.15777587890625, -0.089599609375, -0.129791259765625, -0.040130615234375, 0.05633544921875, 0.123809814453125, 0.06292724609375, 0.10797119140625, 0.14691162109375, 0.079803466796875, 0.05908203125, 0.059295654296875, 0.006378173828125, -0.042022705078125, -0.01922607421875, -0.0655517578125, -0.10614013671875, -0.09002685546875, -0.051788330078125, -0.049774169921875, -0.040252685546875, 0.0025634765625, 0.024169921875, 0.020904541015625, 0.0103759765625, 0.028350830078125, 0.03424072265625, 0.01507568359375, 0.009124755859375, -0.012664794921875, -0.043304443359375, -0.05010986328125, -0.029083251953125, -0.029266357421875, -0.02813720703125, -0.01275634765625, -0.00640869140625, -0.00531005859375, -0.00091552734375, 0.0079345703125, 0.007476806640625, 0.00335693359375, -0.0028076171875, -0.0035400390625, -0.0126953125, -0.012786865234375, 0.00018310546875, 0.00689697265625, -0.003509521484375, -0.008575439453125, -0.003204345703125, -0.004364013671875, -0.005096435546875, 0.000640869140625, 0.003021240234375, -0.003265380859375, -0.00604248046875, -0.00494384765625, -0.00390625, 0.0013427734375, 0.006927490234375, 0.00347900390625, -0.00518798828125, -0.00830078125, -0.0072021484375, -0.006591796875, -0.000732421875, -0.00042724609375, -0.007080078125, -0.012054443359375, -0.007568359375, -0.003387451171875, -0.001434326171875, -0.001373291015625, -0.003448486328125, -0.00347900390625, -0.00323486328125, -0.001678466796875, -0.00079345703125, -0.001983642578125, -0.003753662109375, -0.003387451171875, -0.004730224609375, -0.005340576171875, -0.005340576171875, -0.005035400390625, -0.00555419921875, -0.005859375, -0.005126953125, -0.004241943359375, -0.0029296875, -0.001708984375, -0.000579833984375, 0.000579833984375, 0.000640869140625, 0.00067138671875, 0.00128173828125, 0.00146484375, 0.000457763671875, -0.001190185546875, -0.002166748046875, -0.001983642578125, -0.001129150390625};
static MYFLT BINAURAL_LEFT_HRTF_11[128] = {9.1552734375e-05, 0.0, -3.0517578125e-05, -6.103515625e-05, -6.103515625e-05, 3.0517578125e-05, 3.0517578125e-05, -3.0517578125e-05, -6.103515625e-05, -6.103515625e-05, 0.0, 9.1552734375e-05, -3.0517578125e-05, 0.0, -0.000213623046875, 0.0, -0.000335693359375, 0.000274658203125, -0.000213623046875, 0.0003662109375, 0.002044677734375, 0.0458984375, 0.10699462890625, 0.015899658203125, -0.16741943359375, -0.182647705078125, -0.01190185546875, 0.1597900390625, 0.22943115234375, 0.166412353515625, 0.06201171875, 0.050384521484375, 0.07550048828125, 0.0445556640625, -0.0128173828125, -0.03173828125, -0.0445556640625, -0.07012939453125, -0.07684326171875, -0.069305419921875, -0.063934326171875, -0.058135986328125, -0.040313720703125, -0.018646240234375, -0.018096923828125, -0.002655029296875, 0.03839111328125, 0.061004638671875, 0.058197021484375, 0.03564453125, 0.032806396484375, 0.026458740234375, 0.007537841796875, -0.00518798828125, -0.02899169921875, -0.04559326171875, -0.03399658203125, -0.01544189453125, -0.02044677734375, -0.028778076171875, -0.020294189453125, -0.005401611328125, -0.0010986328125, -0.000640869140625, 0.0009765625, -3.0517578125e-05, 0.002166748046875, 0.002532958984375, 0.0013427734375, 0.003082275390625, 0.00360107421875, -0.000152587890625, -0.006927490234375, -0.011810302734375, -0.01031494140625, -0.0106201171875, -0.0166015625, -0.02099609375, -0.024017333984375, -0.01824951171875, -0.00787353515625, -0.00518798828125, -0.005950927734375, -0.004913330078125, 0.00018310546875, 0.00323486328125, 0.002288818359375, 0.000579833984375, -0.003021240234375, -0.006378173828125, -0.005859375, -0.005615234375, -0.0074462890625, -0.008026123046875, -0.00714111328125, -0.00640869140625, -0.006805419921875, -0.007568359375, -0.00714111328125, -0.00543212890625, -0.003326416015625, -0.0015869140625, -0.001190185546875, -0.001312255859375, -0.001068115234375, -0.000762939453125, -0.0006103515625, -0.00128173828125, -0.002716064453125, -0.003814697265625, -0.004791259765625, -0.005157470703125, -0.00518798828125, -0.004150390625, -0.002471923828125, -0.001312255859375, -0.00146484375, -0.001861572265625, -0.001983642578125, -0.00189208984375, -0.001708984375, -0.002197265625, -0.002532958984375, -0.002593994140625, -0.00250244140625, -0.002777099609375, -0.004119873046875, -0.005126953125};
static MYFLT BINAURAL_RIGHT_HRTF_11[128] = {0.0, 0.00030517578125, 0.0, -3.0517578125e-05, -0.0003662109375, -9.1552734375e-05, -0.0001220703125, 3.0517578125e-05, -0.00030517578125, 0.00042724609375, -0.0008544921875, 0.056854248046875, 0.216644287109375, 0.092559814453125, -0.3140869140625, -0.3426513671875, -0.055755615234375, 0.27032470703125, 0.500152587890625, 0.328948974609375, 0.0279541015625, 0.0081787109375, 0.152374267578125, 0.106201171875, -0.1065673828125, -0.159271240234375, -0.1256103515625, -0.1507568359375, -0.154327392578125, -0.1072998046875, -0.104339599609375, -0.092681884765625, -0.0328369140625, 0.00738525390625, 0.028289794921875, 0.03607177734375, 0.07928466796875, 0.066680908203125, 0.045623779296875, 0.0537109375, 0.026123046875, 0.003570556640625, 0.01055908203125, 0.001861572265625, -0.02685546875, -0.043212890625, -0.040252685546875, -0.032318115234375, -0.042755126953125, -0.04486083984375, -0.03662109375, -0.02435302734375, -0.012847900390625, -0.005218505859375, -0.002471923828125, 0.01007080078125, 0.02294921875, 0.020416259765625, 0.012847900390625, 0.00469970703125, -0.010772705078125, -0.022491455078125, -0.01910400390625, -0.012176513671875, -0.0137939453125, -0.015167236328125, -0.0113525390625, -0.006988525390625, -0.0047607421875, -0.0054931640625, -0.004180908203125, -0.001861572265625, 0.000640869140625, -0.00164794921875, -0.001678466796875, -0.0001220703125, -0.000579833984375, -0.00323486328125, -0.0048828125, -0.00518798828125, -0.005859375, -0.00494384765625, -0.002044677734375, -0.001251220703125, -0.004180908203125, -0.00653076171875, -0.007232666015625, -0.007537841796875, -0.005706787109375, -0.001953125, -0.00128173828125, -0.00140380859375, -0.00152587890625, -0.003082275390625, -0.003997802734375, -0.00146484375, 0.000457763671875, -0.00091552734375, -0.004241943359375, -0.006622314453125, -0.006927490234375, -0.005218505859375, -0.0048828125, -0.006317138671875, -0.007080078125, -0.0062255859375, -0.0035400390625, -0.0020751953125, -0.002105712890625, -0.00201416015625, -0.001251220703125, -0.001190185546875, -0.001190185546875, -0.001861572265625, -0.00323486328125, -0.004058837890625, -0.0037841796875, -0.00372314453125, -0.004180908203125, -0.00396728515625, -0.00421142578125, -0.004638671875, -0.00311279296875, -0.0008544921875, 0.00128173828125, 0.00244140625, 0.00238037109375, 0.002349853515625};
static MYFLT BINAURAL_LEFT_HRTF_12[128] = {-9.1552734375e-05, 6.103515625e-05, 0.00030517578125, -3.0517578125e-05, 3.0517578125e-05, -0.000396728515625, 3.0517578125e-05, -0.000244140625, 0.0001220703125, -0.000274658203125, 0.000274658203125, 0.000244140625, 0.063934326171875, 0.212188720703125, 0.063720703125, -0.318023681640625, -0.33544921875, -0.031768798828125, 0.317169189453125, 0.47650146484375, 0.277862548828125, 0.0123291015625, 0.014251708984375, 0.155975341796875, 0.084197998046875, -0.1102294921875, -0.140625, -0.112640380859375, -0.14459228515625, -0.143402099609375, -0.099151611328125, -0.099273681640625, -0.081329345703125, -0.03118896484375, 0.00335693359375, 0.025787353515625, 0.045135498046875, 0.065887451171875, 0.047882080078125, 0.05303955078125, 0.050018310546875, 0.02410888671875, 0.011810302734375, 0.00811767578125, -0.000152587890625, -0.027252197265625, -0.042633056640625, -0.0396728515625, -0.03729248046875, -0.04571533203125, -0.0450439453125, -0.028289794921875, -0.0166015625, -0.0135498046875, -0.006256103515625, 0.005401611328125, 0.013336181640625, 0.015960693359375, 0.019500732421875, 0.016021728515625, 0.003936767578125, -0.01141357421875, -0.018341064453125, -0.015899658203125, -0.01080322265625, -0.010528564453125, -0.0145263671875, -0.0164794921875, -0.0120849609375, -0.00762939453125, -0.00732421875, -0.00677490234375, -0.006317138671875, -0.001922607421875, 0.0008544921875, 0.0015869140625, 0.00140380859375, -0.0001220703125, -0.00213623046875, -0.001373291015625, -0.001556396484375, -0.004913330078125, -0.006103515625, -0.00347900390625, -0.00225830078125, -0.005035400390625, -0.0074462890625, -0.009124755859375, -0.01068115234375, -0.007110595703125, -0.001800537109375, -0.001129150390625, -0.0020751953125, -0.00238037109375, -0.002838134765625, -0.00244140625, 0.000274658203125, 0.00079345703125, -0.001739501953125, -0.004608154296875, -0.005340576171875, -0.0052490234375, -0.00494384765625, -0.005462646484375, -0.00640869140625, -0.00665283203125, -0.005706787109375, -0.003814697265625, -0.00341796875, -0.003387451171875, -0.001739501953125, -9.1552734375e-05, -0.000335693359375, -0.001373291015625, -0.00262451171875, -0.004150390625, -0.004913330078125, -0.004119873046875, -0.003326416015625, -0.002899169921875, -0.00335693359375, -0.00433349609375, -0.003753662109375, -0.00177001953125, 6.103515625e-05, 0.001007080078125, 0.001190185546875, 0.001434326171875};
static MYFLT BINAURAL_RIGHT_HRTF_12[128] = {3.0517578125e-05, -6.103515625e-05, -6.103515625e-05, -6.103515625e-05, 6.103515625e-05, 6.103515625e-05, -3.0517578125e-05, -6.103515625e-05, 0.0, 6.103515625e-05, 6.103515625e-05, -3.0517578125e-05, 3.0517578125e-05, -0.000213623046875, 9.1552734375e-05, -0.00030517578125, 0.00030517578125, -0.000274658203125, 0.000579833984375, 0.003692626953125, 0.0595703125, 0.117828369140625, -0.010040283203125, -0.2017822265625, -0.178466796875, 0.019927978515625, 0.19366455078125, 0.2510986328125, 0.15911865234375, 0.044891357421875, 0.052398681640625, 0.083160400390625, 0.037567138671875, -0.025146484375, -0.040985107421875, -0.056488037109375, -0.0789794921875, -0.0802001953125, -0.071136474609375, -0.067718505859375, -0.057037353515625, -0.03515625, -0.0115966796875, -0.00872802734375, -0.00335693359375, 0.032806396484375, 0.058380126953125, 0.05975341796875, 0.038299560546875, 0.030364990234375, 0.023834228515625, 0.008026123046875, -0.003448486328125, -0.027923583984375, -0.04815673828125, -0.043060302734375, -0.023193359375, -0.0194091796875, -0.023345947265625, -0.01702880859375, -0.00482177734375, 0.003173828125, 0.005035400390625, 0.00543212890625, 0.003173828125, 0.000885009765625, -0.000335693359375, 0.0018310546875, 0.00421142578125, 0.000885009765625, -0.00482177734375, -0.009552001953125, -0.010955810546875, -0.010101318359375, -0.011749267578125, -0.018707275390625, -0.0250244140625, -0.020904541015625, -0.00933837890625, -0.0045166015625, -0.006683349609375, -0.0078125, -0.004058837890625, 0.0008544921875, 0.001708984375, -9.1552734375e-05, -0.002410888671875, -0.0020751953125, -0.001007080078125, -0.003021240234375, -0.006378173828125, -0.007598876953125, -0.00732421875, -0.006927490234375, -0.00750732421875, -0.008209228515625, -0.008392333984375, -0.007171630859375, -0.00518798828125, -0.003631591796875, -0.001953125, -0.000946044921875, -0.000762939453125, -0.00079345703125, 6.103515625e-05, 0.000213623046875, -0.001007080078125, -0.0028076171875, -0.004058837890625, -0.004241943359375, -0.005157470703125, -0.006134033203125, -0.0052490234375, -0.003143310546875, -0.00201416015625, -0.00213623046875, -0.002044677734375, -0.001861572265625, -0.00091552734375, -0.000274658203125, -0.00152587890625, -0.0028076171875, -0.003143310546875, -0.002899169921875, -0.00274658203125, -0.003509521484375, -0.004608154296875, -0.004150390625};
static MYFLT BINAURAL_LEFT_HRTF_13[128] = {-0.00079345703125, 0.000885009765625, -0.001922607421875, 0.00311279296875, -0.004913330078125, 0.007049560546875, -0.0113525390625, 0.116241455078125, 0.329437255859375, 0.12353515625, -0.66143798828125, -0.558990478515625, 0.44940185546875, 0.484283447265625, 0.34600830078125, 0.329986572265625, 0.11273193359375, 0.00506591796875, 0.104248046875, 0.16436767578125, -0.187225341796875, -0.282440185546875, -0.190643310546875, -0.19549560546875, -0.227020263671875, -0.15216064453125, -0.103668212890625, -0.141357421875, -0.04376220703125, 0.05291748046875, 0.116424560546875, 0.055999755859375, 0.104888916015625, 0.13983154296875, 0.07373046875, 0.05889892578125, 0.062225341796875, 0.007965087890625, -0.0303955078125, -0.002349853515625, -0.056671142578125, -0.1014404296875, -0.086944580078125, -0.049468994140625, -0.055816650390625, -0.0467529296875, -0.00030517578125, 0.013092041015625, 0.00604248046875, 0.00738525390625, 0.029693603515625, 0.0272216796875, 0.012115478515625, 0.011566162109375, -0.01019287109375, -0.03863525390625, -0.040924072265625, -0.020782470703125, -0.02679443359375, -0.02667236328125, -0.009490966796875, -0.004302978515625, -0.006561279296875, -0.0028076171875, 0.00830078125, 0.006378173828125, 0.0010986328125, -0.004547119140625, -0.006622314453125, -0.014251708984375, -0.012237548828125, -0.0006103515625, 0.004608154296875, -0.003082275390625, -0.006561279296875, -0.002532958984375, -0.00311279296875, -0.00262451171875, 0.002105712890625, 0.003021240234375, -0.00274658203125, -0.00555419921875, -0.0048828125, -0.00311279296875, 0.001373291015625, 0.00537109375, 0.001708984375, -0.006378173828125, -0.00933837890625, -0.006744384765625, -0.0052490234375, -0.001007080078125, -0.001129150390625, -0.007080078125, -0.01123046875, -0.0074462890625, -0.0030517578125, -0.0009765625, -0.001068115234375, -0.002532958984375, -0.0030517578125, -0.003021240234375, -0.001190185546875, -0.00054931640625, -0.002197265625, -0.00384521484375, -0.002960205078125, -0.004730224609375, -0.005645751953125, -0.0050048828125, -0.004791259765625, -0.005950927734375, -0.006439208984375, -0.005706787109375, -0.00421142578125, -0.002349853515625, -0.0013427734375, -0.000885009765625, 3.0517578125e-05, 0.000244140625, 0.000701904296875, 0.001861572265625, 0.0018310546875, 0.0009765625, -0.0003662109375, -0.00146484375, -0.0018310546875, -0.001251220703125};
static MYFLT BINAURAL_RIGHT_HRTF_13[128] = {3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 0.0, 6.103515625e-05, 3.0517578125e-05, 3.0517578125e-05, 0.0, 3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 3.0517578125e-05, 6.103515625e-05, 0.0, 0.0, 0.0, 3.0517578125e-05, 3.0517578125e-05, 0.000244140625, -6.103515625e-05, 9.1552734375e-05, -9.1552734375e-05, 0.00103759765625, -0.000152587890625, 0.0, -0.00146484375, 0.014007568359375, 0.037811279296875, 0.05242919921875, 0.00189208984375, -0.12677001953125, -0.11236572265625, 0.02044677734375, 0.103485107421875, 0.126129150390625, 0.1038818359375, 0.07806396484375, 0.06903076171875, 0.071441650390625, 0.05340576171875, 0.02337646484375, -0.0003662109375, -0.013397216796875, -0.0299072265625, -0.05255126953125, -0.050537109375, -0.04937744140625, -0.05584716796875, -0.046173095703125, -0.019439697265625, -0.01715087890625, -0.011566162109375, 0.021392822265625, 0.03143310546875, 0.0274658203125, 0.029266357421875, 0.027374267578125, 0.01031494140625, 0.010009765625, 0.006866455078125, -0.02294921875, -0.039306640625, -0.02911376953125, -0.01824951171875, -0.019378662109375, -0.015594482421875, -0.011077880859375, -0.00787353515625, -0.0028076171875, 0.000946044921875, -0.00030517578125, -0.003570556640625, -0.00518798828125, -0.0057373046875, -0.002471923828125, -0.001190185546875, -0.004058837890625, -0.011199951171875, -0.01422119140625, -0.0107421875, -0.007659912109375, -0.00799560546875, -0.008697509765625, -0.007171630859375, -0.0086669921875, -0.009521484375, -0.008148193359375, -0.006378173828125, -0.005706787109375, -0.007476806640625, -0.00836181640625, -0.008026123046875, -0.0079345703125, -0.008056640625, -0.00836181640625, -0.008026123046875, -0.0057373046875, -0.00238037109375, -0.000946044921875, -0.00140380859375, -0.001708984375, -0.00225830078125, -0.00299072265625, -0.002777099609375, -0.00213623046875, -0.003326416015625, -0.004547119140625, -0.003875732421875, -0.00274658203125, -0.00274658203125, -0.00323486328125, -0.002716064453125, -0.001678466796875, -0.001678466796875, -0.0015869140625, -0.00146484375, -0.00189208984375, -0.002655029296875, -0.003143310546875, -0.0028076171875, -0.0028076171875, -0.003173828125, -0.003204345703125, -0.003631591796875, -0.003997802734375, -0.00347900390625, -0.002349853515625, -0.00164794921875};
static MYFLT BINAURAL_LEFT_HRTF_14[128] = {-0.000152587890625, 0.000457763671875, -0.001220703125, 0.001556396484375, -0.002410888671875, 0.003173828125, -0.00421142578125, 0.005401611328125, -0.00634765625, 0.0654296875, 0.1702880859375, 0.237396240234375, 0.02557373046875, -0.454559326171875, -0.1126708984375, 0.332244873046875, 0.18524169921875, 0.10784912109375, 0.164825439453125, 0.140045166015625, 0.04827880859375, 0.017974853515625, -0.1427001953125, -0.225830078125, -0.1519775390625, -0.12408447265625, -0.138641357421875, -0.113677978515625, -0.0235595703125, 0.000396728515625, 0.025634765625, 0.04986572265625, 0.078643798828125, 0.072662353515625, 0.038787841796875, 0.048370361328125, 0.044830322265625, 0.0032958984375, -0.03057861328125, -0.012725830078125, -0.01300048828125, -0.0048828125, -0.002685546875, -0.04302978515625, -0.07659912109375, -0.068695068359375, -0.0460205078125, -0.02056884765625, 0.0067138671875, 0.02691650390625, 0.027679443359375, 0.015960693359375, 0.01934814453125, 0.02752685546875, 0.020050048828125, 0.001312255859375, -0.01025390625, -0.011505126953125, -0.00079345703125, 0.004180908203125, -0.013275146484375, -0.035675048828125, -0.0316162109375, -0.02203369140625, -0.021148681640625, -0.011566162109375, 0.00103759765625, 0.00360107421875, 0.001922607421875, -0.003509521484375, -0.014923095703125, -0.02130126953125, -0.014984130859375, -0.008148193359375, -0.01123046875, -0.01068115234375, -0.002960205078125, 0.000244140625, -0.001129150390625, 0.00189208984375, 0.004669189453125, 0.0059814453125, -0.0010986328125, -0.00775146484375, -0.009490966796875, -0.00616455078125, 0.0010986328125, -0.0018310546875, -0.010955810546875, -0.013092041015625, -0.007965087890625, -0.002685546875, -0.000885009765625, -0.0010986328125, -0.003662109375, -0.008819580078125, -0.011016845703125, -0.007232666015625, -0.001251220703125, 0.000762939453125, -0.0010986328125, -0.005767822265625, -0.00634765625, -0.001617431640625, 0.00323486328125, 0.00531005859375, 0.002899169921875, -0.003173828125, -0.0084228515625, -0.00994873046875, -0.005584716796875, 0.000518798828125, 0.00042724609375, -0.00799560546875, -0.01458740234375, -0.01190185546875, -0.0029296875, 0.008056640625, 0.00958251953125, -0.0003662109375, -0.009124755859375, -0.006988525390625, 6.103515625e-05, 0.0050048828125, 0.0025634765625, -0.003936767578125, -0.008880615234375, -0.009918212890625};
static MYFLT BINAURAL_RIGHT_HRTF_14[128] = {-0.000152587890625, -0.000213623046875, 0.000335693359375, -0.0001220703125, 0.00030517578125, -0.00042724609375, 0.0003662109375, -0.000457763671875, 0.0001220703125, 0.00018310546875, -0.000396728515625, 0.000885009765625, 0.007843017578125, 0.07318115234375, 0.10577392578125, 0.084259033203125, -0.21759033203125, -0.3160400390625, 0.146270751953125, 0.324493408203125, 0.167449951171875, 0.07196044921875, 0.105560302734375, 0.098052978515625, 0.0770263671875, 0.032806396484375, -0.101531982421875, -0.13909912109375, -0.1055908203125, -0.081146240234375, -0.1064453125, -0.08612060546875, -0.034515380859375, -0.020477294921875, 0.000457763671875, 0.034698486328125, 0.061798095703125, 0.037139892578125, 0.0389404296875, 0.0482177734375, 0.02978515625, 0.003631591796875, -0.00787353515625, -0.006378173828125, -0.007171630859375, -0.005859375, -0.0252685546875, -0.0467529296875, -0.05670166015625, -0.04296875, -0.018402099609375, -0.005279541015625, 0.00201416015625, 0.016265869140625, 0.019317626953125, 0.012542724609375, 0.01678466796875, 0.019500732421875, 0.007965087890625, -0.004608154296875, -0.01177978515625, -0.0118408203125, -0.010894775390625, -0.013427734375, -0.018157958984375, -0.023590087890625, -0.0201416015625, -0.008941650390625, -0.001434326171875, -0.0001220703125, 0.004669189453125, 0.005828857421875, 0.00323486328125, 0.00042724609375, -0.002532958984375, -0.011627197265625, -0.025390625, -0.027313232421875, -0.016143798828125, -0.01275634765625, -0.020355224609375, -0.022125244140625, -0.01202392578125, 0.001617431640625, 0.007354736328125, 0.003692626953125, -0.000579833984375, 0.00067138671875, 0.002899169921875, 0.002685546875, -0.001556396484375, -0.00592041015625, -0.00775146484375, -0.008087158203125, -0.008697509765625, -0.010498046875, -0.01031494140625, -0.005523681640625, -0.002838134765625, -0.00494384765625, -0.00787353515625, -0.006927490234375, -0.00335693359375, -0.000640869140625, -0.00054931640625, -0.001678466796875, -0.000701904296875, 6.103515625e-05, 0.00067138671875, 0.0015869140625, 0.0015869140625, -0.0008544921875, -0.00372314453125, -0.00537109375, -0.005401611328125, -0.004150390625, -0.003082275390625, -0.003204345703125, -0.0035400390625, -0.003814697265625, -0.002960205078125, -0.0010986328125, -0.00115966796875, -0.001861572265625, -0.00164794921875, -0.00140380859375, -0.00238037109375, -0.00299072265625};
static MYFLT BINAURAL_LEFT_HRTF_15[128] = {-0.000152587890625, -0.000213623046875, 0.000335693359375, -0.0001220703125, 0.00030517578125, -0.00042724609375, 0.0003662109375, -0.000457763671875, 0.0001220703125, 0.00018310546875, -0.000396728515625, 0.000885009765625, 0.007843017578125, 0.07318115234375, 0.10577392578125, 0.084259033203125, -0.21759033203125, -0.3160400390625, 0.146270751953125, 0.324493408203125, 0.167449951171875, 0.07196044921875, 0.105560302734375, 0.098052978515625, 0.0770263671875, 0.032806396484375, -0.101531982421875, -0.13909912109375, -0.1055908203125, -0.081146240234375, -0.1064453125, -0.08612060546875, -0.034515380859375, -0.020477294921875, 0.000457763671875, 0.034698486328125, 0.061798095703125, 0.037139892578125, 0.0389404296875, 0.0482177734375, 0.02978515625, 0.003631591796875, -0.00787353515625, -0.006378173828125, -0.007171630859375, -0.005859375, -0.0252685546875, -0.0467529296875, -0.05670166015625, -0.04296875, -0.018402099609375, -0.005279541015625, 0.00201416015625, 0.016265869140625, 0.019317626953125, 0.012542724609375, 0.01678466796875, 0.019500732421875, 0.007965087890625, -0.004608154296875, -0.01177978515625, -0.0118408203125, -0.010894775390625, -0.013427734375, -0.018157958984375, -0.023590087890625, -0.0201416015625, -0.008941650390625, -0.001434326171875, -0.0001220703125, 0.004669189453125, 0.005828857421875, 0.00323486328125, 0.00042724609375, -0.002532958984375, -0.011627197265625, -0.025390625, -0.027313232421875, -0.016143798828125, -0.01275634765625, -0.020355224609375, -0.022125244140625, -0.01202392578125, 0.001617431640625, 0.007354736328125, 0.003692626953125, -0.000579833984375, 0.00067138671875, 0.002899169921875, 0.002685546875, -0.001556396484375, -0.00592041015625, -0.00775146484375, -0.008087158203125, -0.008697509765625, -0.010498046875, -0.01031494140625, -0.005523681640625, -0.002838134765625, -0.00494384765625, -0.00787353515625, -0.006927490234375, -0.00335693359375, -0.000640869140625, -0.00054931640625, -0.001678466796875, -0.000701904296875, 6.103515625e-05, 0.00067138671875, 0.0015869140625, 0.0015869140625, -0.0008544921875, -0.00372314453125, -0.00537109375, -0.005401611328125, -0.004150390625, -0.003082275390625, -0.003204345703125, -0.0035400390625, -0.003814697265625, -0.002960205078125, -0.0010986328125, -0.00115966796875, -0.001861572265625, -0.00164794921875, -0.00140380859375, -0.00238037109375, -0.00299072265625};
static MYFLT BINAURAL_RIGHT_HRTF_15[128] = {-0.000152587890625, 0.000457763671875, -0.001220703125, 0.001556396484375, -0.002410888671875, 0.003173828125, -0.00421142578125, 0.005401611328125, -0.00634765625, 0.0654296875, 0.1702880859375, 0.237396240234375, 0.02557373046875, -0.454559326171875, -0.1126708984375, 0.332244873046875, 0.18524169921875, 0.10784912109375, 0.164825439453125, 0.140045166015625, 0.04827880859375, 0.017974853515625, -0.1427001953125, -0.225830078125, -0.1519775390625, -0.12408447265625, -0.138641357421875, -0.113677978515625, -0.0235595703125, 0.000396728515625, 0.025634765625, 0.04986572265625, 0.078643798828125, 0.072662353515625, 0.038787841796875, 0.048370361328125, 0.044830322265625, 0.0032958984375, -0.03057861328125, -0.012725830078125, -0.01300048828125, -0.0048828125, -0.002685546875, -0.04302978515625, -0.07659912109375, -0.068695068359375, -0.0460205078125, -0.02056884765625, 0.0067138671875, 0.02691650390625, 0.027679443359375, 0.015960693359375, 0.01934814453125, 0.02752685546875, 0.020050048828125, 0.001312255859375, -0.01025390625, -0.011505126953125, -0.00079345703125, 0.004180908203125, -0.013275146484375, -0.035675048828125, -0.0316162109375, -0.02203369140625, -0.021148681640625, -0.011566162109375, 0.00103759765625, 0.00360107421875, 0.001922607421875, -0.003509521484375, -0.014923095703125, -0.02130126953125, -0.014984130859375, -0.008148193359375, -0.01123046875, -0.01068115234375, -0.002960205078125, 0.000244140625, -0.001129150390625, 0.00189208984375, 0.004669189453125, 0.0059814453125, -0.0010986328125, -0.00775146484375, -0.009490966796875, -0.00616455078125, 0.0010986328125, -0.0018310546875, -0.010955810546875, -0.013092041015625, -0.007965087890625, -0.002685546875, -0.000885009765625, -0.0010986328125, -0.003662109375, -0.008819580078125, -0.011016845703125, -0.007232666015625, -0.001251220703125, 0.000762939453125, -0.0010986328125, -0.005767822265625, -0.00634765625, -0.001617431640625, 0.00323486328125, 0.00531005859375, 0.002899169921875, -0.003173828125, -0.0084228515625, -0.00994873046875, -0.005584716796875, 0.000518798828125, 0.00042724609375, -0.00799560546875, -0.01458740234375, -0.01190185546875, -0.0029296875, 0.008056640625, 0.00958251953125, -0.0003662109375, -0.009124755859375, -0.006988525390625, 6.103515625e-05, 0.0050048828125, 0.0025634765625, -0.003936767578125, -0.008880615234375, -0.009918212890625};

static MYFLT BINAURAL_SETUP_X[16] = {0.9239000082015991, 0.9239000082015991, 0.38269996643066406, -0.38269996643066406, -0.9239000082015991, -0.9239000082015991, -0.38269996643066406, 0.38269996643066406, 0.7093999981880188, 0.7093999981880188, 0.0, -0.7093999981880188, -0.7093999981880188, 0.0, 0.0, 0.0};
static MYFLT BINAURAL_SETUP_Y[16] = {-0.38269996643066406, 0.38269996643066406, 0.9239000082015991, 0.9239000082015991, 0.38269996643066406, -0.38269996643066406, -0.9239000082015991, -0.9239000082015991, -0.40960001945495605, 0.40960001945495605, 0.8192000389099121, 0.40960001945495605, -0.40960001945495605, -0.8192000389099121, -0.1736000031232834, 0.1736000031232834};
static MYFLT BINAURAL_SETUP_Z[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.5735999941825867, 0.5735999941825867, 0.5735999941825867, 0.5735999941825867, 0.5735999941825867, 0.5735999941825867, 0.9847999811172485, 0.9847999811172485};
static MYFLT BINAURAL_SETUP_AZIMUTH[16] = {337.5, 22.5, 67.5, 112.5, 157.5, 202.5, 247.5, 292.5, 330.0, 30.0, 90.0, 150.0, 210.0, 270.0, 270.0, 90.0};
static MYFLT BINAURAL_SETUP_ELEVATION[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 35.0, 35.0, 35.0, 35.0, 35.0, 35.0, 80.0, 80.0};

/************************************************************************************************/
/* Binaural main object */
/************************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *azi;
    Stream *azi_stream;
    PyObject *ele;
    Stream *ele_stream;
    PyObject *azispan;
    Stream *azispan_stream;
    PyObject *elespan;
    Stream *elespan_stream;
    VBAP_DATA *paramVBap;
    int hrtf_count[16];
    MYFLT last_azi;
    MYFLT last_ele;
    MYFLT last_azispan;
    MYFLT last_elespan;
    MYFLT hrtf_input_tmp[16][128];
    MYFLT hrtf_left_impulses[16][128];
    MYFLT hrtf_right_impulses[16][128];
    int modebuffer[4];
    MYFLT **vbap_buffer;
    MYFLT *buffer_streams;
} Binauraler;

static void
Binauraler_splitter(Binauraler *self)
{
    int i, o, k, tmp_count;
    MYFLT azi, ele, azispan, elespan, gain, y, sig;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[0] == 0)
        azi = PyFloat_AS_DOUBLE(self->azi);
    else
        azi = Stream_getData((Stream *)self->azi_stream)[0];

    if (self->modebuffer[1] == 0)
        ele = PyFloat_AS_DOUBLE(self->ele);
    else
        ele = Stream_getData((Stream *)self->ele_stream)[0];

    if (self->modebuffer[2] == 0)
        azispan = PyFloat_AS_DOUBLE(self->azispan);
    else
        azispan = Stream_getData((Stream *)self->azispan_stream)[0];

    if (self->modebuffer[3] == 0)
        elespan = PyFloat_AS_DOUBLE(self->elespan);
    else
        elespan = Stream_getData((Stream *)self->elespan_stream)[0];

    ele = ele < 0.0 ? 0.0 : ele > 90.0 ? 90.0 : ele;
    azispan = azispan < 0.0 ? 0.0 : azispan > 1.0 ? 1.0 : azispan;
    elespan = elespan < 0.0 ? 0.0 : elespan > 1.0 ? 1.0 : elespan;

    if (azi != self->last_azi || ele != self->last_ele ||
            azispan != self->last_azispan || elespan != self->last_elespan)
    {
        self->last_azi = azi;
        self->last_ele = ele;
        self->last_azispan = azispan;
        self->last_elespan = elespan;
        vbap2(azi, ele, azispan, elespan, self->paramVBap);
    }

    for (i = 0; i < self->bufsize * 2; i++)
    {
        self->buffer_streams[i] = 0.0;
    }

    for (o = 0; o < 16; o++)
    {
        memset(self->vbap_buffer[o], 0, sizeof(MYFLT) * self->bufsize);
        gain = self->paramVBap->gains[o];
        y = self->paramVBap->y[o];

        for (i = 0; i < self->bufsize; i++)
        {
            y = gain + (y - gain) * 0.99;

            if (y < 0.0000000000001f)
            {
                y = 0.0;
            }
            else
            {
                self->vbap_buffer[o][i] += in[i] * y;
            }
        }

        self->paramVBap->y[o] = y;

        for (i = 0; i < self->bufsize; i++)
        {
            tmp_count = self->hrtf_count[o];

            for (k = 0; k < 128; ++k)
            {
                if (tmp_count < 0)
                {
                    tmp_count += 128;
                }

                sig = self->hrtf_input_tmp[o][tmp_count];
                self->buffer_streams[i] += sig * self->hrtf_left_impulses[o][k];
                self->buffer_streams[i + self->bufsize] += sig * self->hrtf_right_impulses[o][k];
                tmp_count--;
            }

            self->hrtf_count[o]++;

            if (self->hrtf_count[o] >= 128)
            {
                self->hrtf_count[o] = 0;
            }

            self->hrtf_input_tmp[o][self->hrtf_count[o]] = self->vbap_buffer[o][i];
        }
    }
}

MYFLT *
Binauraler_getSamplesBuffer(Binauraler *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
Binauraler_setProcMode(Binauraler *self)
{
    self->proc_func_ptr = Binauraler_splitter;
}

static void
Binauraler_compute_next_data_frame(Binauraler *self)
{
    (*self->proc_func_ptr)(self);
}

static int
Binauraler_traverse(Binauraler *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->azi);
    Py_VISIT(self->azi_stream);
    Py_VISIT(self->ele);
    Py_VISIT(self->ele_stream);
    Py_VISIT(self->azispan);
    Py_VISIT(self->azispan_stream);
    Py_VISIT(self->elespan);
    Py_VISIT(self->elespan_stream);
    return 0;
}

static int
Binauraler_clear(Binauraler *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->azi);
    Py_CLEAR(self->azi_stream);
    Py_CLEAR(self->ele);
    Py_CLEAR(self->ele_stream);
    Py_CLEAR(self->azispan);
    Py_CLEAR(self->azispan_stream);
    Py_CLEAR(self->elespan);
    Py_CLEAR(self->elespan_stream);
    return 0;
}

static void
Binauraler_dealloc(Binauraler* self)
{
    int i;
    pyo_DEALLOC
    free(self->buffer_streams);
    free_vbap_data(self->paramVBap);

    for (i = 0; i < 16; i++)
    {
        free(self->vbap_buffer[i]);
    }

    free(self->vbap_buffer);
    Binauraler_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Binauraler_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j, k;
    PyObject *inputtmp, *input_streamtmp, *azitmp = NULL, *eletmp = NULL, *azispantmp = NULL, *elespantmp = NULL;
    Binauraler *self;
    self = (Binauraler *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Binauraler_compute_next_data_frame);
    self->mode_func_ptr = Binauraler_setProcMode;

    self->azi = PyFloat_FromDouble(0.0);
    self->ele = PyFloat_FromDouble(0.0);
    self->azispan = PyFloat_FromDouble(0.0);
    self->elespan = PyFloat_FromDouble(0.0);
    self->last_azi = self->last_ele = self->last_azispan = self->last_elespan = -1.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;

    static char *kwlist[] = {"input", "azi", "ele", "azispan", "elespan", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &azitmp, &eletmp, &azispantmp, &elespantmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (azitmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAzimuth", "O", azitmp);
    }

    if (eletmp)
    {
        PyObject_CallMethod((PyObject *)self, "setElevation", "O", eletmp);
    }

    if (azispantmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAzispan", "O", azispantmp);
    }

    if (elespantmp)
    {
        PyObject_CallMethod((PyObject *)self, "setElespan", "O", elespantmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    /* Initialize VBAP data. */
    ls lss[16];
    int outputPatches[16];

    for (i = 0; i < 16; i++)
    {
        lss[i].coords.x = BINAURAL_SETUP_X[i];
        lss[i].coords.y = BINAURAL_SETUP_Y[i];
        lss[i].coords.z = BINAURAL_SETUP_Z[i];
        lss[i].angles.azi = BINAURAL_SETUP_AZIMUTH[i];
        lss[i].angles.ele = BINAURAL_SETUP_ELEVATION[i];
        lss[i].angles.length = 1.0; // Always 1.0 for VBAP.
        outputPatches[i] = i + 1;
    }

    self->paramVBap = init_vbap_from_speakers(lss, 16, 3, outputPatches, 16, NULL);

    /* Initialize hrtf data. */
    for (i = 0; i < 16; i++)
    {
        self->hrtf_count[i] = 0;

        for (j = 0; j < 128; j++)
        {
            self->hrtf_input_tmp[i][j] = 0.0;
        }
    }

    for (i = 0; i < 128; i++)
    {
        self->hrtf_left_impulses[0][i] = BINAURAL_LEFT_HRTF_0[i];
        self->hrtf_right_impulses[0][i] = BINAURAL_RIGHT_HRTF_0[i];
        self->hrtf_left_impulses[1][i] = BINAURAL_LEFT_HRTF_1[i];
        self->hrtf_right_impulses[1][i] = BINAURAL_RIGHT_HRTF_1[i];
        self->hrtf_left_impulses[2][i] = BINAURAL_LEFT_HRTF_2[i];
        self->hrtf_right_impulses[2][i] = BINAURAL_RIGHT_HRTF_2[i];
        self->hrtf_left_impulses[3][i] = BINAURAL_LEFT_HRTF_3[i];
        self->hrtf_right_impulses[3][i] = BINAURAL_RIGHT_HRTF_3[i];
        self->hrtf_left_impulses[4][i] = BINAURAL_LEFT_HRTF_4[i];
        self->hrtf_right_impulses[4][i] = BINAURAL_RIGHT_HRTF_4[i];
        self->hrtf_left_impulses[5][i] = BINAURAL_LEFT_HRTF_5[i];
        self->hrtf_right_impulses[5][i] = BINAURAL_RIGHT_HRTF_5[i];
        self->hrtf_left_impulses[6][i] = BINAURAL_LEFT_HRTF_6[i];
        self->hrtf_right_impulses[6][i] = BINAURAL_RIGHT_HRTF_6[i];
        self->hrtf_left_impulses[7][i] = BINAURAL_LEFT_HRTF_7[i];
        self->hrtf_right_impulses[7][i] = BINAURAL_RIGHT_HRTF_7[i];
        self->hrtf_left_impulses[8][i] = BINAURAL_LEFT_HRTF_8[i];
        self->hrtf_right_impulses[8][i] = BINAURAL_RIGHT_HRTF_8[i];
        self->hrtf_left_impulses[9][i] = BINAURAL_LEFT_HRTF_9[i];
        self->hrtf_right_impulses[9][i] = BINAURAL_RIGHT_HRTF_9[i];
        self->hrtf_left_impulses[10][i] = BINAURAL_LEFT_HRTF_10[i];
        self->hrtf_right_impulses[10][i] = BINAURAL_RIGHT_HRTF_10[i];
        self->hrtf_left_impulses[11][i] = BINAURAL_LEFT_HRTF_11[i];
        self->hrtf_right_impulses[11][i] = BINAURAL_RIGHT_HRTF_11[i];
        self->hrtf_left_impulses[12][i] = BINAURAL_LEFT_HRTF_12[i];
        self->hrtf_right_impulses[12][i] = BINAURAL_RIGHT_HRTF_12[i];
        self->hrtf_left_impulses[13][i] = BINAURAL_LEFT_HRTF_13[i];
        self->hrtf_right_impulses[13][i] = BINAURAL_RIGHT_HRTF_13[i];
        self->hrtf_left_impulses[14][i] = BINAURAL_LEFT_HRTF_14[i];
        self->hrtf_right_impulses[14][i] = BINAURAL_RIGHT_HRTF_14[i];
        self->hrtf_left_impulses[15][i] = BINAURAL_LEFT_HRTF_15[i];
        self->hrtf_right_impulses[15][i] = BINAURAL_RIGHT_HRTF_15[i];
    }

    self->vbap_buffer = (MYFLT **)realloc(self->vbap_buffer, 16 * sizeof(MYFLT *));

    for (i = 0; i < 16; i++)
    {
        self->vbap_buffer[i] = (MYFLT *)malloc(self->bufsize * sizeof(MYFLT));
    }

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 2 * self->bufsize * sizeof(MYFLT));

    for (k = 0; k < (2 * self->bufsize); k++)
    {
        self->buffer_streams[k] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Binauraler_getServer(Binauraler* self) { GET_SERVER };
static PyObject * Binauraler_getStream(Binauraler* self) { GET_STREAM };

static PyObject * Binauraler_play(Binauraler *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Binauraler_stop(Binauraler *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
Binauraler_setAzimuth(Binauraler *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->azi);

    if (isNumber == 1)
    {
        self->azi = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
    }
    else
    {
        self->azi = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->azi, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->azi_stream);
        self->azi_stream = (Stream *)streamtmp;
        self->modebuffer[0] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
Binauraler_setElevation(Binauraler *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->ele);

    if (isNumber == 1)
    {
        self->ele = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
    }
    else
    {
        self->ele = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ele, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ele_stream);
        self->ele_stream = (Stream *)streamtmp;
        self->modebuffer[1] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
Binauraler_setAzispan(Binauraler *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->azispan);

    if (isNumber == 1)
    {
        self->azispan = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else
    {
        self->azispan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->azispan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->azispan_stream);
        self->azispan_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyObject *
Binauraler_setElespan(Binauraler *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->elespan);

    if (isNumber == 1)
    {
        self->elespan = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
    }
    else
    {
        self->elespan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->elespan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->elespan_stream);
        self->elespan_stream = (Stream *)streamtmp;
        self->modebuffer[3] = 1;
    }

    (*self->mode_func_ptr)(self);

    Py_RETURN_NONE;
}

static PyMemberDef Binauraler_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Binauraler, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Binauraler, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Binauraler, input), 0, "Input sound object."},
    {"azi", T_OBJECT_EX, offsetof(Binauraler, azi), 0, "Azimuth object."},
    {"ele", T_OBJECT_EX, offsetof(Binauraler, ele), 0, "Elevation object."},
    {"azispan", T_OBJECT_EX, offsetof(Binauraler, azispan), 0, "Azimuth spanning."},
    {"elespan", T_OBJECT_EX, offsetof(Binauraler, elespan), 0, "Elevation spanning."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Binauraler_methods[] =
{
    {"getServer", (PyCFunction)Binauraler_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Binauraler_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Binauraler_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Binauraler_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setAzimuth", (PyCFunction)Binauraler_setAzimuth, METH_O, "Sets azimuth value between -180 and 180 degrees."},
    {"setElevation", (PyCFunction)Binauraler_setElevation, METH_O, "Sets elevation value between -40 and 90 degrees."},
    {"setAzispan", (PyCFunction)Binauraler_setAzispan, METH_O, "Sets azimuth spanning value between 0 and 1."},
    {"setElespan", (PyCFunction)Binauraler_setElespan, METH_O, "Sets elevation spanning value between 0 and 1."},
    {NULL}  /* Sentinel */
};

PyTypeObject BinauralerType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Binauraler_base",                                   /*tp_name*/
    sizeof(Binauraler),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Binauraler_dealloc,                     /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    "Binauraler main objects.",           /* tp_doc */
    (traverseproc)Binauraler_traverse,                  /* tp_traverse */
    (inquiry)Binauraler_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Binauraler_methods,                                 /* tp_methods */
    Binauraler_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Binauraler_new,                                     /* tp_new */
};

/************************************************************************************************/
/* BinauralSpat streamer object */
/************************************************************************************************/
typedef struct
{
    pyo_audio_HEAD
    Binauraler *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} Binaural;

static void Binaural_postprocessing_ii(Binaural *self) { POST_PROCESSING_II };
static void Binaural_postprocessing_ai(Binaural *self) { POST_PROCESSING_AI };
static void Binaural_postprocessing_ia(Binaural *self) { POST_PROCESSING_IA };
static void Binaural_postprocessing_aa(Binaural *self) { POST_PROCESSING_AA };
static void Binaural_postprocessing_ireva(Binaural *self) { POST_PROCESSING_IREVA };
static void Binaural_postprocessing_areva(Binaural *self) { POST_PROCESSING_AREVA };
static void Binaural_postprocessing_revai(Binaural *self) { POST_PROCESSING_REVAI };
static void Binaural_postprocessing_revaa(Binaural *self) { POST_PROCESSING_REVAA };
static void Binaural_postprocessing_revareva(Binaural *self) { POST_PROCESSING_REVAREVA };

static void
Binaural_setProcMode(Binaural *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = Binaural_postprocessing_ii;
            break;

        case 1:
            self->muladd_func_ptr = Binaural_postprocessing_ai;
            break;

        case 2:
            self->muladd_func_ptr = Binaural_postprocessing_revai;
            break;

        case 10:
            self->muladd_func_ptr = Binaural_postprocessing_ia;
            break;

        case 11:
            self->muladd_func_ptr = Binaural_postprocessing_aa;
            break;

        case 12:
            self->muladd_func_ptr = Binaural_postprocessing_revaa;
            break;

        case 20:
            self->muladd_func_ptr = Binaural_postprocessing_ireva;
            break;

        case 21:
            self->muladd_func_ptr = Binaural_postprocessing_areva;
            break;

        case 22:
            self->muladd_func_ptr = Binaural_postprocessing_revareva;
            break;
    }
}

static void
Binaural_compute_next_data_frame(Binaural *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Binauraler_getSamplesBuffer((Binauraler *)self->mainSplitter);

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = tmp[i + offset];
    }

    (*self->muladd_func_ptr)(self);
}

static int
Binaural_traverse(Binaural *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
Binaural_clear(Binaural *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
Binaural_dealloc(Binaural* self)
{
    pyo_DEALLOC
    Binaural_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Binaural_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp = NULL, *multmp = NULL, *addtmp = NULL;
    Binaural *self;
    self = (Binaural *)type->tp_alloc(type, 0);

    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Binaural_compute_next_data_frame);
    self->mode_func_ptr = Binaural_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (Binauraler *)maintmp;

    if (multmp)
    {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp)
    {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Binaural_getServer(Binaural* self) { GET_SERVER };
static PyObject * Binaural_getStream(Binaural* self) { GET_STREAM };
static PyObject * Binaural_setMul(Binaural *self, PyObject *arg) { SET_MUL };
static PyObject * Binaural_setAdd(Binaural *self, PyObject *arg) { SET_ADD };
static PyObject * Binaural_setSub(Binaural *self, PyObject *arg) { SET_SUB };
static PyObject * Binaural_setDiv(Binaural *self, PyObject *arg) { SET_DIV };

static PyObject * Binaural_play(Binaural *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Binaural_out(Binaural *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Binaural_stop(Binaural *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Binaural_multiply(Binaural *self, PyObject *arg) { MULTIPLY };
static PyObject * Binaural_inplace_multiply(Binaural *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Binaural_add(Binaural *self, PyObject *arg) { ADD };
static PyObject * Binaural_inplace_add(Binaural *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Binaural_sub(Binaural *self, PyObject *arg) { SUB };
static PyObject * Binaural_inplace_sub(Binaural *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Binaural_div(Binaural *self, PyObject *arg) { DIV };
static PyObject * Binaural_inplace_div(Binaural *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Binaural_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Binaural, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Binaural, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Binaural, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Binaural, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Binaural_methods[] =
{
    {"getServer", (PyCFunction)Binaural_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Binaural_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Binaural_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Binaural_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Binaural_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)Binaural_setMul, METH_O, "Sets Binaural mul factor."},
    {"setAdd", (PyCFunction)Binaural_setAdd, METH_O, "Sets Binaural add factor."},
    {"setSub", (PyCFunction)Binaural_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Binaural_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Binaural_as_number =
{
    (binaryfunc)Binaural_add,                      /*nb_add*/
    (binaryfunc)Binaural_sub,                 /*nb_subtract*/
    (binaryfunc)Binaural_multiply,                 /*nb_multiply*/
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
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    (binaryfunc)Binaural_inplace_add,              /*inplace_add*/
    (binaryfunc)Binaural_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Binaural_inplace_multiply,         /*inplace_multiply*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)Binaural_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)Binaural_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject BinauralType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Binaural_base",         /*tp_name*/
    sizeof(Binaural),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Binaural_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &Binaural_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,  /*tp_flags*/
    "Binaural objects. Reads one channel from a Binauralter.",           /* tp_doc */
    (traverseproc)Binaural_traverse,   /* tp_traverse */
    (inquiry)Binaural_clear,           /* tp_clear */
    0,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    Binaural_methods,             /* tp_methods */
    Binaural_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    Binaural_new,                 /* tp_new */
};
