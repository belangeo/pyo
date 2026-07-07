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

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

#define DEFAULT_SRATE   44100.0
#define NUM_COMB         8
#define NUM_ALLPASS      4

static const MYFLT comb_delays[NUM_COMB] =
{
    1116.0 / DEFAULT_SRATE,
    1188.0 / DEFAULT_SRATE,
    1277.0 / DEFAULT_SRATE,
    1356.0 / DEFAULT_SRATE,
    1422.0 / DEFAULT_SRATE,
    1491.0 / DEFAULT_SRATE,
    1557.0 / DEFAULT_SRATE,
    1617.0 / DEFAULT_SRATE
};

static const MYFLT allpass_delays[NUM_ALLPASS] =
{
    556.0 / DEFAULT_SRATE,
    441.0 / DEFAULT_SRATE,
    341.0 / DEFAULT_SRATE,
    225.0 / DEFAULT_SRATE
};

static const MYFLT fixedGain   = 0.015;
static const MYFLT scaleDamp   = 0.5;
static const MYFLT scaleRoom   = 0.29;
static const MYFLT offsetRoom  = 0.7;
static const MYFLT allPassFeedBack = 0.5;

typedef struct
{
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *size;
    Stream *size_stream;
    PyObject *damp;
    Stream *damp_stream;
    PyObject *mix;
    Stream *mix_stream;
    int comb_nSamples[NUM_COMB];
    int comb_bufPos[NUM_COMB];
    MYFLT comb_filterState[NUM_COMB];
    MYFLT *comb_buf[NUM_COMB];
    int allpass_nSamples[NUM_ALLPASS];
    int allpass_bufPos[NUM_ALLPASS];
    MYFLT *allpass_buf[NUM_ALLPASS];
    int modebuffer[5];
    MYFLT srFactor;
} Freeverb;

static MYFLT
_clip(MYFLT x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

static int
Freeverb_calc_nsamples(Freeverb *self, MYFLT delTime)
{
    return (int)(delTime * self->sr + 0.5);
}

static void
Freeverb_transform_iii(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT siz = _clip(PyFloat_AS_DOUBLE(self->size));
    MYFLT dam = _clip(PyFloat_AS_DOUBLE(self->damp));
    MYFLT mix = _clip(PyFloat_AS_DOUBLE(self->mix));

    feedback = siz * scaleRoom + offsetRoom;
    damp = dam * scaleDamp;

    mix1 = MYSQRT(mix);
    mix2 = MYSQRT(1.0 - mix);

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_aii(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *siz = Stream_getData((Stream *)self->size_stream);
    MYFLT dam = _clip(PyFloat_AS_DOUBLE(self->damp));
    MYFLT mix = _clip(PyFloat_AS_DOUBLE(self->mix));

    damp = dam * scaleDamp;

    mix1 = MYSQRT(mix);
    mix2 = MYSQRT(1.0 - mix);

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        feedback = _clip(siz[j]) * scaleRoom + offsetRoom;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_iai(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT siz = _clip(PyFloat_AS_DOUBLE(self->size));
    MYFLT *dam = Stream_getData((Stream *)self->damp_stream);
    MYFLT mix = _clip(PyFloat_AS_DOUBLE(self->mix));

    feedback = siz * scaleRoom + offsetRoom;

    mix1 = MYSQRT(mix);
    mix2 = MYSQRT(1.0 - mix);

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        damp = _clip(dam[j]) * scaleDamp;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_aai(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *siz = Stream_getData((Stream *)self->size_stream);
    MYFLT *dam = Stream_getData((Stream *)self->damp_stream);
    MYFLT mix = _clip(PyFloat_AS_DOUBLE(self->mix));

    mix1 = MYSQRT(mix);
    mix2 = MYSQRT(1.0 - mix);

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        feedback = _clip(siz[j]) * scaleRoom + offsetRoom;
        damp = _clip(dam[j]) * scaleDamp;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_iia(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2, mixtmp;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT siz = _clip(PyFloat_AS_DOUBLE(self->size));
    MYFLT dam = _clip(PyFloat_AS_DOUBLE(self->damp));
    MYFLT *mix = Stream_getData((Stream *)self->mix_stream);

    feedback = siz * scaleRoom + offsetRoom;
    damp = dam * scaleDamp;

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        mixtmp = _clip(mix[i]);
        mix1 = MYSQRT(mixtmp);
        mix2 = MYSQRT(1.0 - mixtmp);
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_aia(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2, mixtmp;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *siz = Stream_getData((Stream *)self->size_stream);
    MYFLT dam = _clip(PyFloat_AS_DOUBLE(self->damp));
    MYFLT *mix = Stream_getData((Stream *)self->mix_stream);

    damp = dam * scaleDamp;

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        feedback = _clip(siz[j]) * scaleRoom + offsetRoom;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        mixtmp = _clip(mix[i]);
        mix1 = MYSQRT(mixtmp);
        mix2 = MYSQRT(1.0 - mixtmp);
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_iaa(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2, mixtmp;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT siz = _clip(PyFloat_AS_DOUBLE(self->size));
    MYFLT *dam = Stream_getData((Stream *)self->damp_stream);
    MYFLT *mix = Stream_getData((Stream *)self->mix_stream);

    feedback = siz * scaleRoom + offsetRoom;

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        damp = _clip(dam[j]) * scaleDamp;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        mixtmp = _clip(mix[i]);
        mix1 = MYSQRT(mixtmp);
        mix2 = MYSQRT(1.0 - mixtmp);
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void
Freeverb_transform_aaa(Freeverb *self)
{
    MYFLT x, feedback, damp, mix1, mix2, mixtmp;
    int i, j;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *siz = Stream_getData((Stream *)self->size_stream);
    MYFLT *dam = Stream_getData((Stream *)self->damp_stream);
    MYFLT *mix = Stream_getData((Stream *)self->mix_stream);

    MYFLT tmp[self->bufsize];
    memset(&tmp, 0, sizeof(tmp));

    for (j = 0; j < self->bufsize; j++)
    {
        feedback = _clip(siz[j]) * scaleRoom + offsetRoom;
        damp = _clip(dam[j]) * scaleDamp;

        for (i = 0; i < NUM_COMB; i++)
        {
            x = self->comb_buf[i][self->comb_bufPos[i]];
            tmp[j] += x;
            self->comb_filterState[i] = x + (self->comb_filterState[i] - x) * damp;
            x = self->comb_filterState[i] * feedback + in[j];
            self->comb_buf[i][self->comb_bufPos[i]] = x;
            self->comb_bufPos[i]++;

            if (self->comb_bufPos[i] >= self->comb_nSamples[i])
                self->comb_bufPos[i] = 0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        for (j = 0; j < self->bufsize; j++)
        {
            x = self->allpass_buf[i][self->allpass_bufPos[i]] - tmp[j];
            self->allpass_buf[i][self->allpass_bufPos[i]] *= allPassFeedBack;
            self->allpass_buf[i][self->allpass_bufPos[i]] += tmp[j];
            self->allpass_bufPos[i]++;

            if (self->allpass_bufPos[i] >= self->allpass_nSamples[i])
                self->allpass_bufPos[i] = 0;

            tmp[j] = x;
        }
    }

    for (i = 0; i < self->bufsize; i++)
    {
        mixtmp = _clip(mix[i]);
        mix1 = MYSQRT(mixtmp);
        mix2 = MYSQRT(1.0 - mixtmp);
        self->data[i] = (tmp[i] * fixedGain * mix1) + (in[i] * mix2);
    }
}

static void Freeverb_postprocessing_ii(Freeverb *self) { POST_PROCESSING_II };
static void Freeverb_postprocessing_ai(Freeverb *self) { POST_PROCESSING_AI };
static void Freeverb_postprocessing_ia(Freeverb *self) { POST_PROCESSING_IA };
static void Freeverb_postprocessing_aa(Freeverb *self) { POST_PROCESSING_AA };
static void Freeverb_postprocessing_ireva(Freeverb *self) { POST_PROCESSING_IREVA };
static void Freeverb_postprocessing_areva(Freeverb *self) { POST_PROCESSING_AREVA };
static void Freeverb_postprocessing_revai(Freeverb *self) { POST_PROCESSING_REVAI };
static void Freeverb_postprocessing_revaa(Freeverb *self) { POST_PROCESSING_REVAA };
static void Freeverb_postprocessing_revareva(Freeverb *self) { POST_PROCESSING_REVAREVA };

static void
Freeverb_setProcMode(Freeverb *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (procmode)
    {
        case 0:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_iii);
            break;

        case 1:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_aii);
            break;

        case 10:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_iai);
            break;

        case 11:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_aai);
            break;

        case 100:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_iia);
            break;

        case 101:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_aia);
            break;

        case 110:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_iaa);
            break;

        case 111:
            self->proc_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_transform_aaa);
            break;
    }

    switch (muladdmode)
    {
        case 0:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_ii);
            break;

        case 1:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_ai);
            break;

        case 2:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_revai);
            break;

        case 10:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_ia);
            break;

        case 11:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_aa);
            break;

        case 12:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_revaa);
            break;

        case 20:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_ireva);
            break;

        case 21:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_areva);
            break;

        case 22:
            self->muladd_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_postprocessing_revareva);
            break;
    }
}

static void
Freeverb_compute_next_data_frame(Freeverb *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Freeverb_traverse(Freeverb *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->size);
    Py_VISIT(self->damp);
    Py_VISIT(self->mix);
    return 0;
}

static int
Freeverb_clear(Freeverb *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->size);
    Py_CLEAR(self->damp);
    Py_CLEAR(self->mix);
    return 0;
}

static void
Freeverb_dealloc(Freeverb* self)
{
    int i;
    pyo_DEALLOC

    for (i = 0; i < NUM_COMB; i++)
    {
        PyMem_RawFree(self->comb_buf[i]);
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        PyMem_RawFree(self->allpass_buf[i]);
    }

    Freeverb_clear(self);
    Py_CLEAR(self->stream);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Freeverb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j, rndSamps;
    MYFLT nsamps;
    PyObject *inputtmp, *input_streamtmp, *sizetmp = NULL, *damptmp = NULL, *mixtmp = NULL, *multmp = NULL, *addtmp = NULL;
    Freeverb *self;
    self = (Freeverb *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->size = PyFloat_FromDouble(.5);
    self->damp = PyFloat_FromDouble(.5);
    self->mix = PyFloat_FromDouble(.5);
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;
    self->modebuffer[4] = 0;

    self->srFactor = pow((DEFAULT_SRATE / self->sr), 0.8);

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PYO_AUDIO_CALLBACK(Freeverb_compute_next_data_frame));
    self->mode_func_ptr = PYO_AUDIO_CALLBACK(Freeverb_setProcMode);

    static char *kwlist[] = {"input", "size", "damp", "mix", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOO", kwlist, &inputtmp, &sizetmp, &damptmp, &mixtmp, &multmp, &addtmp)) {
        Py_DECREF(self);
        return NULL;
    }

    INIT_INPUT_STREAM

    if (sizetmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setSize", sizetmp);
    }

    if (damptmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setDamp", damptmp);
    }

    if (mixtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMix", mixtmp);
    }

    if (multmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setMul", multmp);
    }

    if (addtmp)
    {
        PYO_CALL_METHOD_O_OR_RETURN_NULL(self, "setAdd", addtmp);
    }

    PYO_ADD_STREAM_OR_RETURN_NULL(self);

    (*self->mode_func_ptr)(self);

    Server_generateSeed((Server *)self->server, FREEVERB_ID);

    rndSamps = (RANDOM_UNIFORM * 20 + 10) / DEFAULT_SRATE;

    for (i = 0; i < NUM_COMB; i++)
    {
        nsamps = Freeverb_calc_nsamples((Freeverb *)self, comb_delays[i] + rndSamps);
        self->comb_buf[i] = (MYFLT *)PyMem_RawRealloc(self->comb_buf[i], (nsamps + 1) * sizeof(MYFLT));
        self->comb_nSamples[i] = nsamps;
        self->comb_bufPos[i] = 0;
        self->comb_filterState[i] = 0.0;

        for (j = 0; j < nsamps; j++)
        {
            self->comb_buf[i][j] = 0.0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        nsamps = Freeverb_calc_nsamples((Freeverb *)self, allpass_delays[i] + rndSamps);
        self->allpass_buf[i] = (MYFLT *)PyMem_RawRealloc(self->allpass_buf[i], (nsamps + 1) * sizeof(MYFLT));
        self->allpass_nSamples[i] = nsamps;
        self->allpass_bufPos[i] = 0;

        for (j = 0; j < nsamps; j++)
        {
            self->allpass_buf[i][j] = 0.0;
        }
    }

    return (PyObject *)self;
}

static PyObject * Freeverb_getServer(Freeverb* self) { GET_SERVER };
static PyObject * Freeverb_getStream(Freeverb* self) { GET_STREAM };
static PyObject * Freeverb_setMul(Freeverb *self, PyObject *arg) { SET_MUL };
static PyObject * Freeverb_setAdd(Freeverb *self, PyObject *arg) { SET_ADD };
static PyObject * Freeverb_setSub(Freeverb *self, PyObject *arg) { SET_SUB };
static PyObject * Freeverb_setDiv(Freeverb *self, PyObject *arg) { SET_DIV };

static PyObject * Freeverb_play(Freeverb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Freeverb_out(Freeverb *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Freeverb_stop(Freeverb *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Freeverb_multiply(Freeverb *self, PyObject *arg) { MULTIPLY };
static PyObject * Freeverb_inplace_multiply(Freeverb *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Freeverb_add(Freeverb *self, PyObject *arg) { ADD };
static PyObject * Freeverb_inplace_add(Freeverb *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Freeverb_sub(Freeverb *self, PyObject *arg) { SUB };
static PyObject * Freeverb_inplace_sub(Freeverb *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Freeverb_div(Freeverb *self, PyObject *arg) { DIV };
static PyObject * Freeverb_inplace_div(Freeverb *self, PyObject *arg) { INPLACE_DIV };

static PyObject * Freeverb_setSize(Freeverb *self, PyObject *arg) { SET_PARAM(self->size, self->size_stream, 2); }
static PyObject * Freeverb_setDamp(Freeverb *self, PyObject *arg) { SET_PARAM(self->damp, self->damp_stream, 3); }
static PyObject * Freeverb_setMix(Freeverb *self, PyObject *arg) {SET_PARAM(self->mix, self->mix_stream, 4); }

static PyObject *
Freeverb_reset(Freeverb *self)
{
    int i, j;

    for (i = 0; i < NUM_COMB; i++)
    {
        self->comb_bufPos[i] = 0;
        self->comb_filterState[i] = 0.0;

        for (j = 0; j < self->comb_nSamples[i]; j++)
        {
            self->comb_buf[i][j] = 0.0;
        }
    }

    for (i = 0; i < NUM_ALLPASS; i++)
    {
        self->allpass_bufPos[i] = 0;

        for (j = 0; j < self->allpass_nSamples[i]; j++)
        {
            self->allpass_buf[i][j] = 0.0;
        }
    }

    Py_RETURN_NONE;
}

static PyMemberDef Freeverb_members[] =
{
    {"server", T_OBJECT_EX, offsetof(Freeverb, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Freeverb, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Freeverb, input), 0, "Input sound object."},
    {"size", T_OBJECT_EX, offsetof(Freeverb, size), 0, "Room size."},
    {"damp", T_OBJECT_EX, offsetof(Freeverb, damp), 0, "High frequencies damp factor."},
    {"mix", T_OBJECT_EX, offsetof(Freeverb, mix), 0, "Mix between dry and wet signal."},
    {"mul", T_OBJECT_EX, offsetof(Freeverb, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Freeverb, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Freeverb_methods[] =
{
    {"getServer", (PyCFunction)Freeverb_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Freeverb_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Freeverb_play, METH_VARARGS | METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Freeverb_out, METH_VARARGS | METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Freeverb_stop, METH_VARARGS | METH_KEYWORDS, "Stops computing."},
    {"reset", (PyCFunction)Freeverb_reset, METH_NOARGS, "Reset the delay lines."},
    {"setSize", (PyCFunction)Freeverb_setSize, METH_O, "Sets distortion size factor (0 -> 1)."},
    {"setDamp", (PyCFunction)Freeverb_setDamp, METH_O, "Sets lowpass filter damp factor."},
    {"setMix", (PyCFunction)Freeverb_setMix, METH_O, "Sets the mix factor."},
    {"setMul", (PyCFunction)Freeverb_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Freeverb_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Freeverb_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Freeverb_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyType_Slot FreeverbType_slots[] = {
    {Py_tp_dealloc, Freeverb_dealloc},
    {Py_tp_doc, "Freeverb objects. Jezar's Freeverb ."},
    {Py_tp_traverse, Freeverb_traverse},
    {Py_tp_clear, Freeverb_clear},
    {Py_tp_methods, Freeverb_methods},
    {Py_tp_members, Freeverb_members},
    {Py_nb_add, Freeverb_add},
    {Py_nb_subtract, Freeverb_sub},
    {Py_nb_multiply, Freeverb_multiply},
    {Py_nb_true_divide, Freeverb_div},
    {Py_nb_inplace_add, Freeverb_inplace_add},
    {Py_nb_inplace_subtract, Freeverb_inplace_sub},
    {Py_nb_inplace_multiply, Freeverb_inplace_multiply},
    {Py_nb_inplace_true_divide, Freeverb_inplace_div},
    {Py_tp_new, Freeverb_new},
    {0, NULL}
};

static PyType_Spec FreeverbType_spec =
{
    "_pyo.Freeverb_base",
    sizeof(Freeverb),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    FreeverbType_slots
};

PyTypeObject *
PyoCreateFreeverbType(PyObject *module)
{
    return (PyTypeObject *)PyType_FromModuleAndSpec(module, &FreeverbType_spec, NULL);
}
