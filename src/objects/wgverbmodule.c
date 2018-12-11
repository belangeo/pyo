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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

#define NUM_REFS  13

static const MYFLT randomScaling = 0.5;
static const MYFLT reverbParams[8][4] = {
{ 2473.0, 0.0010, 3.100, 2503.0 },
{ 2767.0, 0.0011, 3.500, 2749.0 },
{ 3217.0, 0.0017, 1.110, 3187.0 },
{ 3557.0, 0.0006, 3.973, 3583.0 },
{ 3907.0, 0.0010, 2.341, 3929.0 },
{ 4127.0, 0.0011, 1.897, 4093.0 },
{ 2143.0, 0.0017, 0.891, 2131.0 },
{ 1933.0, 0.0006, 3.221, 1951.0 }};

static const MYFLT first_ref_delays[NUM_REFS] = {283, 467, 587, 677, 757, 911, 1117, 1223, 1307, 1429, 1553, 1613, 1783};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    PyObject *cutoff;
    Stream *cutoff_stream;
    PyObject *mix;
    Stream *mix_stream;
    void (*mix_func_ptr)();
    int modebuffer[5];
    MYFLT total_signal;
    MYFLT delays[8];
    long size[8];
    int in_count[8];
    MYFLT *buffer[8];
    // lowpass
    MYFLT damp;
    MYFLT lastFreq;
    // sample memories
    MYFLT lastSamples[8];
    // jitters
    MYFLT rnd[8];
    MYFLT rnd_value[8];
    MYFLT rnd_oldValue[8];
    MYFLT rnd_diff[8];
    MYFLT rnd_time[8];
    MYFLT rnd_timeInc[8];
    MYFLT rnd_range[8];
    MYFLT rnd_halfRange[8];
} WGVerb;

static void
WGVerb_process_ii(WGVerb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt;
    int i, j, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
    MYFLT freq = PyFloat_AS_DOUBLE(self->cutoff);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    if (freq != self->lastFreq) {
        self->lastFreq = freq;
        self->damp = 2.0 - MYCOS(TWOPI * freq / self->sr);
        self->damp = (self->damp - MYSQRT(self->damp * self->damp - 1.0));
    }

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        junction = self->total_signal * .25;
        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            self->rnd_time[j] += self->rnd_timeInc[j];
            if (self->rnd_time[j] < 0.0)
                self->rnd_time[j] += 1.0;
            else if (self->rnd_time[j] >= 1.0) {
                self->rnd_time[j] -= 1.0;
                self->rnd_oldValue[j] = self->rnd_value[j];
                self->rnd_value[j] = self->rnd_range[j] * RANDOM_UNIFORM - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];

            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += self->size[j];
            ind = (int)xind;
            frac = xind - ind;
            x = self->buffer[j][ind];
            x1 = self->buffer[j][ind+1];
            val = x + (x1 - x) * frac;
            val *= feed;
            filt = (self->lastSamples[j] - val) * self->damp + val;
            self->total_signal += filt;

            self->buffer[j][self->in_count[j]] = inval + junction - self->lastSamples[j];
            self->lastSamples[j] = filt;
            if(self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][0];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
WGVerb_process_ai(WGVerb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, feed;
    int i, j, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *feedback = Stream_getData((Stream *)self->feedback_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->cutoff);

    if (freq != self->lastFreq) {
        self->lastFreq = freq;
        self->damp = 2.0 - MYCOS(TWOPI * freq / self->sr);
        self->damp = (self->damp - MYSQRT(self->damp * self->damp - 1.0));
    }

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        feed = feedback[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        junction = self->total_signal * .25;
        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            self->rnd_time[j] += self->rnd_timeInc[j];
            if (self->rnd_time[j] < 0.0)
                self->rnd_time[j] += 1.0;
            else if (self->rnd_time[j] >= 1.0) {
                self->rnd_time[j] -= 1.0;
                self->rnd_oldValue[j] = self->rnd_value[j];
                self->rnd_value[j] = self->rnd_range[j] * RANDOM_UNIFORM - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];

            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += self->size[j];
            ind = (int)xind;
            frac = xind - ind;
            x = self->buffer[j][ind];
            x1 = self->buffer[j][ind+1];
            val = x + (x1 - x) * frac;
            val *= feed;
            filt = (self->lastSamples[j] - val) * self->damp + val;
            self->total_signal += filt;

            self->buffer[j][self->in_count[j]] = inval + junction - self->lastSamples[j];
            self->lastSamples[j] = filt;
            if(self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][0];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
WGVerb_process_ia(WGVerb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, freq;
    int i, j, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
    MYFLT *cutoff = Stream_getData((Stream *)self->cutoff_stream);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        freq = cutoff[i];
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            self->damp = 2.0 - MYCOS(TWOPI * freq / self->sr);
            self->damp = (self->damp - MYSQRT(self->damp * self->damp - 1.0));
        }
        junction = self->total_signal * .25;
        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            self->rnd_time[j] += self->rnd_timeInc[j];
            if (self->rnd_time[j] < 0.0)
                self->rnd_time[j] += 1.0;
            else if (self->rnd_time[j] >= 1.0) {
                self->rnd_time[j] -= 1.0;
                self->rnd_oldValue[j] = self->rnd_value[j];
                self->rnd_value[j] = self->rnd_range[j] * RANDOM_UNIFORM - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];

            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += self->size[j];
            ind = (int)xind;
            frac = xind - ind;
            x = self->buffer[j][ind];
            x1 = self->buffer[j][ind+1];
            val = x + (x1 - x) * frac;
            val *= feed;
            filt = (self->lastSamples[j] - val) * self->damp + val;
            self->total_signal += filt;

            self->buffer[j][self->in_count[j]] = inval + junction - self->lastSamples[j];
            self->lastSamples[j] = filt;
            if(self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][0];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
WGVerb_process_aa(WGVerb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, feed, freq;
    int i, j, ind;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *feedback = Stream_getData((Stream *)self->feedback_stream);
    MYFLT *cutoff = Stream_getData((Stream *)self->cutoff_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        feed = feedback[i];
        freq = cutoff[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            self->damp = 2.0 - MYCOS(TWOPI * freq / self->sr);
            self->damp = (self->damp - MYSQRT(self->damp * self->damp - 1.0));
        }
        junction = self->total_signal * .25;
        self->total_signal = 0.0;
        for (j=0; j<8; j++) {
            self->rnd_time[j] += self->rnd_timeInc[j];
            if (self->rnd_time[j] < 0.0)
                self->rnd_time[j] += 1.0;
            else if (self->rnd_time[j] >= 1.0) {
                self->rnd_time[j] -= 1.0;
                self->rnd_oldValue[j] = self->rnd_value[j];
                self->rnd_value[j] = self->rnd_range[j] * RANDOM_UNIFORM - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];

            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += self->size[j];
            ind = (int)xind;
            frac = xind - ind;
            x = self->buffer[j][ind];
            x1 = self->buffer[j][ind+1];
            val = x + (x1 - x) * frac;
            val *= feed;
            filt = (self->lastSamples[j] - val) * self->damp + val;
            self->total_signal += filt;

            self->buffer[j][self->in_count[j]] = inval + junction - self->lastSamples[j];
            self->lastSamples[j] = filt;
            if(self->in_count[j] == 0)
                self->buffer[j][self->size[j]] = self->buffer[j][0];
            self->in_count[j]++;
            if (self->in_count[j] >= self->size[j])
                self->in_count[j] = 0;
        }
        self->data[i] = self->total_signal * 0.25;
    }
}

static void
WGVerb_mix_i(WGVerb *self) {
    int i;
    MYFLT val;

    MYFLT mix = PyFloat_AS_DOUBLE(self->mix);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (mix < 0.0)
        mix = 0.0;
    else if (mix > 1.0)
        mix = 1.0;

    for (i=0; i<self->bufsize; i++) {
        val = in[i] * (1.0 - mix) + self->data[i] * mix;
        self->data[i] = val;
    }
}

static void
WGVerb_mix_a(WGVerb *self) {
    int i;
    MYFLT mix, val;

    MYFLT *mi = Stream_getData((Stream *)self->mix_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        mix = mi[i];
        if (mix < 0.0)
            mix = 0.0;
        else if (mix > 1.0)
            mix = 1.0;

        val = in[i] * (1.0 - mix) + self->data[i] * mix;
        self->data[i] = val;
    }
}

static void WGVerb_postprocessing_ii(WGVerb *self) { POST_PROCESSING_II };
static void WGVerb_postprocessing_ai(WGVerb *self) { POST_PROCESSING_AI };
static void WGVerb_postprocessing_ia(WGVerb *self) { POST_PROCESSING_IA };
static void WGVerb_postprocessing_aa(WGVerb *self) { POST_PROCESSING_AA };
static void WGVerb_postprocessing_ireva(WGVerb *self) { POST_PROCESSING_IREVA };
static void WGVerb_postprocessing_areva(WGVerb *self) { POST_PROCESSING_AREVA };
static void WGVerb_postprocessing_revai(WGVerb *self) { POST_PROCESSING_REVAI };
static void WGVerb_postprocessing_revaa(WGVerb *self) { POST_PROCESSING_REVAA };
static void WGVerb_postprocessing_revareva(WGVerb *self) { POST_PROCESSING_REVAREVA };

static void
WGVerb_setProcMode(WGVerb *self)
{
    int procmode, muladdmode, mixmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    mixmode = self->modebuffer[4];

	switch (procmode) {
        case 0:
            self->proc_func_ptr = WGVerb_process_ii;
            break;
        case 1:
            self->proc_func_ptr = WGVerb_process_ai;
            break;
        case 10:
            self->proc_func_ptr = WGVerb_process_ia;
            break;
        case 11:
            self->proc_func_ptr = WGVerb_process_aa;
            break;
    }
    switch (mixmode) {
        case 0:
            self->mix_func_ptr = WGVerb_mix_i;
            break;
        case 1:
            self->mix_func_ptr = WGVerb_mix_a;
            break;
    }

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = WGVerb_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = WGVerb_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = WGVerb_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = WGVerb_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = WGVerb_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = WGVerb_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = WGVerb_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = WGVerb_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = WGVerb_postprocessing_revareva;
            break;
    }
}

static void
WGVerb_compute_next_data_frame(WGVerb *self)
{
    (*self->proc_func_ptr)(self);
    (*self->mix_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
WGVerb_traverse(WGVerb *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->feedback);
    Py_VISIT(self->feedback_stream);
    Py_VISIT(self->cutoff);
    Py_VISIT(self->cutoff_stream);
    Py_VISIT(self->mix);
    Py_VISIT(self->mix_stream);
    return 0;
}

static int
WGVerb_clear(WGVerb *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->feedback);
    Py_CLEAR(self->feedback_stream);
    Py_CLEAR(self->cutoff);
    Py_CLEAR(self->cutoff_stream);
    Py_CLEAR(self->mix);
    Py_CLEAR(self->mix_stream);
    return 0;
}

static void
WGVerb_dealloc(WGVerb* self)
{
    int i;
    pyo_DEALLOC
    for (i=0; i<8; i++) {
        free(self->buffer[i]);
    }
    WGVerb_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
WGVerb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp, *input_streamtmp, *feedbacktmp=NULL, *cutofftmp=NULL, *mixtmp=NULL, *multmp=NULL, *addtmp=NULL;
    WGVerb *self;
    self = (WGVerb *)type->tp_alloc(type, 0);

    self->feedback = PyFloat_FromDouble(0.5);
    self->cutoff = PyFloat_FromDouble(5000.0);
    self->mix = PyFloat_FromDouble(0.5);
    self->lastFreq = self->damp = 0.0;

    self->total_signal = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, WGVerb_compute_next_data_frame);
    self->mode_func_ptr = WGVerb_setProcMode;

    for (i=0; i<8; i++) {
        self->in_count[i] = 0;
        self->lastSamples[i] = 0.0;
        self->rnd[i] = self->rnd_value[i] = self->rnd_oldValue[i] = self->rnd_diff[i] = 0.0;
        self->rnd_time[i] = 1.0;
        self->rnd_timeInc[i] = reverbParams[i][2] * randomScaling / self->sr;
        self->rnd_range[i] = reverbParams[i][1] * randomScaling * self->sr;
        self->rnd_halfRange[i] = self->rnd_range[i] * 0.5;
        self->delays[i] = reverbParams[i][0] * (self->sr / 44100.0);
    }

    static char *kwlist[] = {"input", "feedback", "cutoff", "mix", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOO", kwlist, &inputtmp, &feedbacktmp, &cutofftmp, &mixtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }

    if (cutofftmp) {
        PyObject_CallMethod((PyObject *)self, "setCutoff", "O", cutofftmp);
    }

    if (mixtmp) {
        PyObject_CallMethod((PyObject *)self, "setMix", "O", mixtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    for (i=0; i<8; i++) {
        self->size[i] = reverbParams[i][0] * (self->sr / 44100.0) + (int)(reverbParams[i][1] * self->sr + 0.5);
        self->buffer[i] = (MYFLT *)realloc(self->buffer[i], (self->size[i]+1) * sizeof(MYFLT));
        for (j=0; j<(self->size[i]+1); j++) {
            self->buffer[i][j] = 0.;
        }
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * WGVerb_getServer(WGVerb* self) { GET_SERVER };
static PyObject * WGVerb_getStream(WGVerb* self) { GET_STREAM };
static PyObject * WGVerb_setMul(WGVerb *self, PyObject *arg) { SET_MUL };
static PyObject * WGVerb_setAdd(WGVerb *self, PyObject *arg) { SET_ADD };
static PyObject * WGVerb_setSub(WGVerb *self, PyObject *arg) { SET_SUB };
static PyObject * WGVerb_setDiv(WGVerb *self, PyObject *arg) { SET_DIV };

static PyObject * WGVerb_play(WGVerb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * WGVerb_out(WGVerb *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * WGVerb_stop(WGVerb *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * WGVerb_multiply(WGVerb *self, PyObject *arg) { MULTIPLY };
static PyObject * WGVerb_inplace_multiply(WGVerb *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * WGVerb_add(WGVerb *self, PyObject *arg) { ADD };
static PyObject * WGVerb_inplace_add(WGVerb *self, PyObject *arg) { INPLACE_ADD };
static PyObject * WGVerb_sub(WGVerb *self, PyObject *arg) { SUB };
static PyObject * WGVerb_inplace_sub(WGVerb *self, PyObject *arg) { INPLACE_SUB };
static PyObject * WGVerb_div(WGVerb *self, PyObject *arg) { DIV };
static PyObject * WGVerb_inplace_div(WGVerb *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
WGVerb_reset(WGVerb *self)
{
    int i, j;
    for (i=0; i<8; i++) {
        self->in_count[i] = 0;
        self->lastSamples[i] = 0.0;
        for (j=0; j<(self->size[i]+1); j++) {
            self->buffer[i][j] = 0.;
        }
    }
    self->total_signal = 0.0;

	Py_RETURN_NONE;
}

static PyObject *
WGVerb_setFeedback(WGVerb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
WGVerb_setCutoff(WGVerb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->cutoff);
	if (isNumber == 1) {
		self->cutoff = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->cutoff = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->cutoff, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->cutoff_stream);
        self->cutoff_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
WGVerb_setMix(WGVerb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->mix);
	if (isNumber == 1) {
		self->mix = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->mix = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->mix, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->mix_stream);
        self->mix_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef WGVerb_members[] = {
{"server", T_OBJECT_EX, offsetof(WGVerb, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(WGVerb, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(WGVerb, input), 0, "Input sound object."},
{"feedback", T_OBJECT_EX, offsetof(WGVerb, feedback), 0, "Feedback value."},
{"cutoff", T_OBJECT_EX, offsetof(WGVerb, cutoff), 0, "WGVerb lowpass filter cutoff."},
{"mix", T_OBJECT_EX, offsetof(WGVerb, mix), 0, "Balance between dry and wet signals."},
{"mul", T_OBJECT_EX, offsetof(WGVerb, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(WGVerb, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef WGVerb_methods[] = {
{"getServer", (PyCFunction)WGVerb_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)WGVerb_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)WGVerb_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)WGVerb_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)WGVerb_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"reset", (PyCFunction)WGVerb_reset, METH_NOARGS, "Reset the delay line."},
{"setFeedback", (PyCFunction)WGVerb_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
{"setCutoff", (PyCFunction)WGVerb_setCutoff, METH_O, "Sets lowpass filter cutoff."},
{"setMix", (PyCFunction)WGVerb_setMix, METH_O, "Sets balance between dry and wet signals."},
{"setMul", (PyCFunction)WGVerb_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)WGVerb_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)WGVerb_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)WGVerb_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods WGVerb_as_number = {
(binaryfunc)WGVerb_add,                      /*nb_add*/
(binaryfunc)WGVerb_sub,                 /*nb_subtract*/
(binaryfunc)WGVerb_multiply,                 /*nb_multiply*/
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
(binaryfunc)WGVerb_inplace_add,              /*inplace_add*/
(binaryfunc)WGVerb_inplace_sub,         /*inplace_subtract*/
(binaryfunc)WGVerb_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)WGVerb_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)WGVerb_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject WGVerbType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.WGVerb_base",         /*tp_name*/
sizeof(WGVerb),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)WGVerb_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&WGVerb_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"WGVerb objects. Waveguide-based reverberation network.",           /* tp_doc */
(traverseproc)WGVerb_traverse,   /* tp_traverse */
(inquiry)WGVerb_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
WGVerb_methods,             /* tp_methods */
WGVerb_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
WGVerb_new,                 /* tp_new */
};

/***************/
/**** STRev ****/
/***************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *inpos;
    Stream *inpos_stream;
    PyObject *revtime;
    Stream *revtime_stream;
    PyObject *cutoff;
    Stream *cutoff_stream;
    PyObject *mix;
    Stream *mix_stream;
    void (*mix_func_ptr)();
    int modebuffer[4];
    MYFLT firstRefGain;
    MYFLT total_signal[2];
    MYFLT delays[2][8];
    long size[2][8];
    int in_count[2][8];
    MYFLT *buffer[2][8];
    MYFLT *ref_buffer[NUM_REFS];
    int ref_size[NUM_REFS];
    int ref_in_count[NUM_REFS];
    MYFLT avg_time;
    MYFLT srfac;
    // lowpass
    MYFLT damp[2];
    MYFLT lastFreq;
    MYFLT nyquist;
    MYFLT lastInpos;
    // sample memories
    MYFLT lastSamples[2][8];
    // jitters
    MYFLT rnd[2][8];
    MYFLT rnd_value[2][8];
    MYFLT rnd_oldValue[2][8];
    MYFLT rnd_diff[2][8];
    MYFLT rnd_time[2][8];
    MYFLT rnd_timeInc[2][8];
    MYFLT rnd_range[2][8];
    MYFLT rnd_halfRange[2][8];
    MYFLT *buffer_streams;
    MYFLT *input_buffer[2];
} STReverb;

static void
STReverb_process_ii(STReverb *self) {
    int i, j, k, k2, ind, half;
    MYFLT val, x, x1, xind, frac, junction, inval, filt, amp1, amp2, b, f, sum_ref, feed, step, invp;
    MYFLT ref_amp_l[NUM_REFS];
    MYFLT ref_amp_r[NUM_REFS];
    MYFLT ref_buf[2];

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT inpos = PyFloat_AS_DOUBLE(self->inpos);
    if (self->modebuffer[1] == 0)
        feed = PyFloat_AS_DOUBLE(self->revtime);
    else
        feed = Stream_getData((Stream *)self->revtime_stream)[0];
    MYFLT freq = PyFloat_AS_DOUBLE(self->cutoff);

    if (inpos < 0.0)
        inpos = 0.0;
    else if (inpos > 1.0)
        inpos = 1.0;

    if (feed < 0.01)
        feed = 0.01;
    feed = MYPOW(100.0, -self->avg_time/feed);

    if (freq < 20.0)
        freq = 20.0;
    else if (freq > self->nyquist)
        freq = self->nyquist;

    if (freq != self->lastFreq || inpos != self->lastInpos) {
        self->lastFreq = freq;
        self->lastInpos = inpos;
        f = ((1.0 - inpos) * 0.3 + 0.7) * freq;
        b = 2.0 - MYCOS(TWOPI * f / self->sr);
        self->damp[0] = (b - MYSQRT(b * b - 1.0));
        f = (inpos * 0.3 + 0.7) * freq;
        b = 2.0 - MYCOS(TWOPI * f / self->sr);
        self->damp[1] = (b - MYSQRT(b * b - 1.0));
    }

    /* position of the source and first reflexions */
    amp1 = 1.0 - inpos;
    amp2 = inpos;
    half = (NUM_REFS - 1) / 2;
    if (inpos <= 0.5) {
        step = (0.5 - inpos) / half;
        ref_amp_l[half] = ref_amp_r[half] = 0.5;
        for (k=0; k<half; k++) {
            k2 = NUM_REFS - 1 - k;
            ref_amp_r[k] = ref_amp_l[k2] = inpos + step * k;
            ref_amp_l[k] = ref_amp_r[k2] = 1.0 - ref_amp_r[k];
            ref_amp_r[k2] *= inpos + 0.5;
        }
    }
    else {
        invp = 1.0 - inpos;
        step = (0.5 - invp) / half;
        ref_amp_l[half] = ref_amp_r[half] = 0.5;
        for (k=0; k<half; k++) {
            k2 = NUM_REFS - 1 - k;
            ref_amp_l[k] = ref_amp_r[k2] = invp + step * k;
            ref_amp_r[k] = ref_amp_l[k2] = 1.0 - ref_amp_l[k];
            ref_amp_l[k2] *= invp + 0.5;
        }
    }

    for (i=0; i<self->bufsize; i++) {
        self->input_buffer[0][i] = in[i] * amp1;
        self->input_buffer[1][i] = in[i] * amp2;
        ref_buf[0] = ref_buf[1] = 0.0;
        for (k=0; k<NUM_REFS; k++) {
            sum_ref = self->ref_buffer[k][self->ref_in_count[k]];
            self->ref_buffer[k][self->ref_in_count[k]] = in[i];
            self->ref_in_count[k]++;
            if (self->ref_in_count[k] == self->ref_size[k])
                self->ref_in_count[k] = 0;
            ref_buf[0] += sum_ref * ref_amp_l[k];
            ref_buf[1] += sum_ref * ref_amp_r[k];
        }
        for (k=0; k<2; k++) {
            inval = self->input_buffer[k][i] * 0.8 + self->input_buffer[1-k][i] * 0.2 + ref_buf[k] * 0.1;
            junction = self->total_signal[k] * .25;
            self->total_signal[k] = ref_buf[k] * self->firstRefGain;
            for (j=0; j<8; j++) {
                self->rnd_time[k][j] += self->rnd_timeInc[k][j];
                if (self->rnd_time[k][j] < 0.0)
                    self->rnd_time[k][j] += 1.0;
                else if (self->rnd_time[k][j] >= 1.0) {
                    self->rnd_time[k][j] -= 1.0;
                    self->rnd_oldValue[k][j] = self->rnd_value[k][j];
                    self->rnd_value[k][j] = self->rnd_range[k][j] * RANDOM_UNIFORM - self->rnd_halfRange[k][j];
                    self->rnd_diff[k][j] = self->rnd_value[k][j] - self->rnd_oldValue[k][j];
                }
                self->rnd[k][j] = self->rnd_oldValue[k][j] + self->rnd_diff[k][j] * self->rnd_time[k][j];

                xind = self->in_count[k][j] - (self->delays[k][j] + self->rnd[k][j]);
                if (xind < 0)
                    xind += self->size[k][j];
                ind = (int)xind;
                frac = xind - ind;
                x = self->buffer[k][j][ind];
                x1 = self->buffer[k][j][ind+1];
                val = x + (x1 - x) * frac;
                val *= feed;
                filt = val + (self->lastSamples[k][j] - val) * self->damp[k];
                self->total_signal[k] += filt;

                self->buffer[k][j][self->in_count[k][j]] = inval + junction - self->lastSamples[k][j];
                self->lastSamples[k][j] = filt;
                if(self->in_count[k][j] == 0)
                    self->buffer[k][j][self->size[k][j]] = self->buffer[k][j][0];
                self->in_count[k][j]++;
                if (self->in_count[k][j] >= self->size[k][j])
                    self->in_count[k][j] = 0;
            }
            self->buffer_streams[i+k*self->bufsize] = self->total_signal[k] * 0.25;
        }
    }
}

static void
STReverb_process_ai(STReverb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, amp1, amp2, b, f, sum_ref, inpos, feed, step, invp;
    MYFLT ref_amp_l[NUM_REFS];
    MYFLT ref_amp_r[NUM_REFS];
    MYFLT ref_buf[2];
    int i, j, k, k2, ind, half;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *pos = Stream_getData((Stream *)self->inpos_stream);
    if (self->modebuffer[1] == 0)
        feed = PyFloat_AS_DOUBLE(self->revtime);
    else
        feed = Stream_getData((Stream *)self->revtime_stream)[0];
    MYFLT freq = PyFloat_AS_DOUBLE(self->cutoff);

    if (feed < 0.01)
        feed = 0.01;
    feed = MYPOW(100.0, -self->avg_time/feed);

    if (freq < 20.0)
        freq = 20.0;
    else if (freq > self->nyquist)
        freq = self->nyquist;

    for (i=0; i<self->bufsize; i++) {
        inpos = pos[i];
        if (inpos < 0.0)
            inpos = 0.0;
        else if (inpos > 1.0)
            inpos = 1.0;
        if (freq != self->lastFreq || inpos != self->lastInpos) {
            self->lastFreq = freq;
            self->lastInpos = inpos;
            f = ((1.0 - inpos) * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[0] = (b - MYSQRT(b * b - 1.0));
            f = (inpos * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[1] = (b - MYSQRT(b * b - 1.0));
        }

        /* position of the source and first reflexions */
        amp1 = 1.0 - inpos;
        amp2 = inpos;
        half = (NUM_REFS - 1) / 2;
        if (inpos <= 0.5) {
            step = (0.5 - inpos) / half;
            ref_amp_l[half] = ref_amp_r[half] = 0.5;
            for (k=0; k<half; k++) {
                k2 = NUM_REFS - 1 - k;
                ref_amp_r[k] = ref_amp_l[k2] = inpos + step * k;
                ref_amp_l[k] = ref_amp_r[k2] = 1.0 - ref_amp_r[k];
                ref_amp_r[k2] *= inpos + 0.5;
            }
        }
        else {
            invp = 1.0 - inpos;
            step = (0.5 - invp) / half;
            ref_amp_l[half] = ref_amp_r[half] = 0.5;
            for (k=0; k<half; k++) {
                k2 = NUM_REFS - 1 - k;
                ref_amp_l[k] = ref_amp_r[k2] = invp + step * k;
                ref_amp_r[k] = ref_amp_l[k2] = 1.0 - ref_amp_l[k];
                ref_amp_l[k2] *= invp + 0.5;
            }
        }

        self->input_buffer[0][i] = in[i] * amp1;
        self->input_buffer[1][i] = in[i] * amp2;
        ref_buf[0] = ref_buf[1] = 0.0;
        for (k=0; k<NUM_REFS; k++) {
            sum_ref = self->ref_buffer[k][self->ref_in_count[k]];
            self->ref_buffer[k][self->ref_in_count[k]] = in[i];
            self->ref_in_count[k]++;
            if (self->ref_in_count[k] == self->ref_size[k])
                self->ref_in_count[k] = 0;
            ref_buf[0] += sum_ref * ref_amp_l[k];
            ref_buf[1] += sum_ref * ref_amp_r[k];
        }
        for (k=0; k<2; k++) {
            inval = self->input_buffer[k][i] * 0.8 + self->input_buffer[1-k][i] * 0.2 + ref_buf[k] * 0.1;
            junction = self->total_signal[k] * .25;
            self->total_signal[k] = ref_buf[k] * self->firstRefGain;
            for (j=0; j<8; j++) {
                self->rnd_time[k][j] += self->rnd_timeInc[k][j];
                if (self->rnd_time[k][j] < 0.0)
                    self->rnd_time[k][j] += 1.0;
                else if (self->rnd_time[k][j] >= 1.0) {
                    self->rnd_time[k][j] -= 1.0;
                    self->rnd_oldValue[k][j] = self->rnd_value[k][j];
                    self->rnd_value[k][j] = self->rnd_range[k][j] * RANDOM_UNIFORM - self->rnd_halfRange[k][j];
                    self->rnd_diff[k][j] = self->rnd_value[k][j] - self->rnd_oldValue[k][j];
                }
                self->rnd[k][j] = self->rnd_oldValue[k][j] + self->rnd_diff[k][j] * self->rnd_time[k][j];

                xind = self->in_count[k][j] - (self->delays[k][j] + self->rnd[k][j]);
                if (xind < 0)
                    xind += self->size[k][j];
                ind = (int)xind;
                frac = xind - ind;
                x = self->buffer[k][j][ind];
                x1 = self->buffer[k][j][ind+1];
                val = x + (x1 - x) * frac;
                val *= feed;
                filt = val + (self->lastSamples[k][j] - val) * self->damp[k];
                self->total_signal[k] += filt;

                self->buffer[k][j][self->in_count[k][j]] = inval + junction - self->lastSamples[k][j];
                self->lastSamples[k][j] = filt;
                if(self->in_count[k][j] == 0)
                    self->buffer[k][j][self->size[k][j]] = self->buffer[k][j][0];
                self->in_count[k][j]++;
                if (self->in_count[k][j] >= self->size[k][j])
                    self->in_count[k][j] = 0;
            }
            self->buffer_streams[i+k*self->bufsize] = self->total_signal[k] * 0.25;
        }
    }
}

static void
STReverb_process_ia(STReverb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, amp1, amp2, b, f, sum_ref, feed, freq, step, invp;
    MYFLT ref_amp_l[NUM_REFS];
    MYFLT ref_amp_r[NUM_REFS];
    MYFLT ref_buf[2];
    int i, j, k, k2, ind, half;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT inpos = PyFloat_AS_DOUBLE(self->inpos);
    if (self->modebuffer[1] == 0)
        feed = PyFloat_AS_DOUBLE(self->revtime);
    else
        feed = Stream_getData((Stream *)self->revtime_stream)[0];
    MYFLT *fr = Stream_getData((Stream *)self->cutoff_stream);

    if (inpos < 0.0)
        inpos = 0.0;
    else if (inpos > 1.0)
        inpos = 1.0;

    if (feed < 0.01)
        feed = 0.01;
    feed = MYPOW(100.0, -self->avg_time/feed);

    /* position of the source and first reflexions */
    amp1 = 1.0 - inpos;
    amp2 = inpos;
    half = (NUM_REFS - 1) / 2;
    if (inpos <= 0.5) {
        step = (0.5 - inpos) / half;
        ref_amp_l[half] = ref_amp_r[half] = 0.5;
        for (k=0; k<half; k++) {
            k2 = NUM_REFS - 1 - k;
            ref_amp_r[k] = ref_amp_l[k2] = inpos + step * k;
            ref_amp_l[k] = ref_amp_r[k2] = 1.0 - ref_amp_r[k];
            ref_amp_r[k2] *= inpos + 0.5;
        }
    }
    else {
        invp = 1.0 - inpos;
        step = (0.5 - invp) / half;
        ref_amp_l[half] = ref_amp_r[half] = 0.5;
        for (k=0; k<half; k++) {
            k2 = NUM_REFS - 1 - k;
            ref_amp_l[k] = ref_amp_r[k2] = invp + step * k;
            ref_amp_r[k] = ref_amp_l[k2] = 1.0 - ref_amp_l[k];
            ref_amp_l[k2] *= invp + 0.5;
        }
    }

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq < 20.0)
            freq = 20.0;
        else if (freq > self->nyquist)
            freq = self->nyquist;

        if (freq != self->lastFreq || inpos != self->lastInpos) {
            self->lastFreq = freq;
            self->lastInpos = inpos;
            f = ((1.0 - inpos) * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[0] = (b - MYSQRT(b * b - 1.0));
            f = (inpos * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[1] = (b - MYSQRT(b * b - 1.0));
        }

        self->input_buffer[0][i] = in[i] * amp1;
        self->input_buffer[1][i] = in[i] * amp2;
        ref_buf[0] = ref_buf[1] = 0.0;
        for (k=0; k<NUM_REFS; k++) {
            sum_ref = self->ref_buffer[k][self->ref_in_count[k]];
            self->ref_buffer[k][self->ref_in_count[k]] = in[i];
            self->ref_in_count[k]++;
            if (self->ref_in_count[k] == self->ref_size[k])
                self->ref_in_count[k] = 0;
            ref_buf[0] += sum_ref * ref_amp_l[k];
            ref_buf[1] += sum_ref * ref_amp_r[k];
        }
        for (k=0; k<2; k++) {
            inval = self->input_buffer[k][i] * 0.8 + self->input_buffer[1-k][i] * 0.2 + ref_buf[k] * 0.1;
            junction = self->total_signal[k] * .25;
            self->total_signal[k] = ref_buf[k] * self->firstRefGain;
            for (j=0; j<8; j++) {
                self->rnd_time[k][j] += self->rnd_timeInc[k][j];
                if (self->rnd_time[k][j] < 0.0)
                    self->rnd_time[k][j] += 1.0;
                else if (self->rnd_time[k][j] >= 1.0) {
                    self->rnd_time[k][j] -= 1.0;
                    self->rnd_oldValue[k][j] = self->rnd_value[k][j];
                    self->rnd_value[k][j] = self->rnd_range[k][j] * RANDOM_UNIFORM - self->rnd_halfRange[k][j];
                    self->rnd_diff[k][j] = self->rnd_value[k][j] - self->rnd_oldValue[k][j];
                }
                self->rnd[k][j] = self->rnd_oldValue[k][j] + self->rnd_diff[k][j] * self->rnd_time[k][j];

                xind = self->in_count[k][j] - (self->delays[k][j] + self->rnd[k][j]);
                if (xind < 0)
                    xind += self->size[k][j];
                ind = (int)xind;
                frac = xind - ind;
                x = self->buffer[k][j][ind];
                x1 = self->buffer[k][j][ind+1];
                val = x + (x1 - x) * frac;
                val *= feed;
                filt = val + (self->lastSamples[k][j] - val) * self->damp[k];
                self->total_signal[k] += filt;

                self->buffer[k][j][self->in_count[k][j]] = inval + junction - self->lastSamples[k][j];
                self->lastSamples[k][j] = filt;
                if(self->in_count[k][j] == 0)
                    self->buffer[k][j][self->size[k][j]] = self->buffer[k][j][0];
                self->in_count[k][j]++;
                if (self->in_count[k][j] >= self->size[k][j])
                    self->in_count[k][j] = 0;
            }
            self->buffer_streams[i+k*self->bufsize] = self->total_signal[k] * 0.25;
        }
    }
}

static void
STReverb_process_aa(STReverb *self) {
    MYFLT val, x, x1, xind, frac, junction, inval, filt, amp1, amp2, b, f, sum_ref, inpos, feed, freq, step, invp;
    MYFLT ref_amp_l[NUM_REFS];
    MYFLT ref_amp_r[NUM_REFS];
    MYFLT ref_buf[2];
    int i, j, k, k2, ind, half;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *pos = Stream_getData((Stream *)self->inpos_stream);
    if (self->modebuffer[1] == 0)
        feed = PyFloat_AS_DOUBLE(self->revtime);
    else
        feed = Stream_getData((Stream *)self->revtime_stream)[0];
    MYFLT *fr = Stream_getData((Stream *)self->cutoff_stream);

    if (feed < 0.01)
        feed = 0.01;
    feed = MYPOW(100.0, -self->avg_time/feed);

    for (i=0; i<self->bufsize; i++) {
        inpos = pos[i];
        freq = fr[i];
        if (inpos < 0.0)
            inpos = 0.0;
        else if (inpos > 1.0)
            inpos = 1.0;
        if (freq < 20.0)
            freq = 20.0;
        else if (freq > self->nyquist)
            freq = self->nyquist;

        if (freq != self->lastFreq || inpos != self->lastInpos) {
            self->lastFreq = freq;
            self->lastInpos = inpos;
            f = ((1.0 - inpos) * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[0] = (b - MYSQRT(b * b - 1.0));
            f = (inpos * 0.3 + 0.7) * freq;
            b = 2.0 - MYCOS(TWOPI * f / self->sr);
            self->damp[1] = (b - MYSQRT(b * b - 1.0));
        }

        /* position of the source and first reflexions */
        amp1 = 1.0 - inpos;
        amp2 = inpos;
        half = (NUM_REFS - 1) / 2;
        if (inpos <= 0.5) {
            step = (0.5 - inpos) / half;
            ref_amp_l[half] = ref_amp_r[half] = 0.5;
            for (k=0; k<half; k++) {
                k2 = NUM_REFS - 1 - k;
                ref_amp_r[k] = ref_amp_l[k2] = inpos + step * k;
                ref_amp_l[k] = ref_amp_r[k2] = 1.0 - ref_amp_r[k];
                ref_amp_r[k2] *= inpos + 0.5;
            }
        }
        else {
            invp = 1.0 - inpos;
            step = (0.5 - invp) / half;
            ref_amp_l[half] = ref_amp_r[half] = 0.5;
            for (k=0; k<half; k++) {
                k2 = NUM_REFS - 1 - k;
                ref_amp_l[k] = ref_amp_r[k2] = invp + step * k;
                ref_amp_r[k] = ref_amp_l[k2] = 1.0 - ref_amp_l[k];
                ref_amp_l[k2] *= invp + 0.5;
            }
        }

        self->input_buffer[0][i] = in[i] * amp1;
        self->input_buffer[1][i] = in[i] * amp2;
        ref_buf[0] = ref_buf[1] = 0.0;
        for (k=0; k<NUM_REFS; k++) {
            sum_ref = self->ref_buffer[k][self->ref_in_count[k]];
            self->ref_buffer[k][self->ref_in_count[k]] = in[i];
            self->ref_in_count[k]++;
            if (self->ref_in_count[k] == self->ref_size[k])
                self->ref_in_count[k] = 0;
            ref_buf[0] += sum_ref * ref_amp_l[k];
            ref_buf[1] += sum_ref * ref_amp_r[k];
        }
        for (k=0; k<2; k++) {
            inval = self->input_buffer[k][i] * 0.8 + self->input_buffer[1-k][i] * 0.2 + ref_buf[k] * 0.1;
            junction = self->total_signal[k] * .25;
            self->total_signal[k] = ref_buf[k] * self->firstRefGain;
            for (j=0; j<8; j++) {
                self->rnd_time[k][j] += self->rnd_timeInc[k][j];
                if (self->rnd_time[k][j] < 0.0)
                    self->rnd_time[k][j] += 1.0;
                else if (self->rnd_time[k][j] >= 1.0) {
                    self->rnd_time[k][j] -= 1.0;
                    self->rnd_oldValue[k][j] = self->rnd_value[k][j];
                    self->rnd_value[k][j] = self->rnd_range[k][j] * RANDOM_UNIFORM - self->rnd_halfRange[k][j];
                    self->rnd_diff[k][j] = self->rnd_value[k][j] - self->rnd_oldValue[k][j];
                }
                self->rnd[k][j] = self->rnd_oldValue[k][j] + self->rnd_diff[k][j] * self->rnd_time[k][j];

                xind = self->in_count[k][j] - (self->delays[k][j] + self->rnd[k][j]);
                if (xind < 0)
                    xind += self->size[k][j];
                ind = (int)xind;
                frac = xind - ind;
                x = self->buffer[k][j][ind];
                x1 = self->buffer[k][j][ind+1];
                val = x + (x1 - x) * frac;
                val *= feed;
                filt = val + (self->lastSamples[k][j] - val) * self->damp[k];
                self->total_signal[k] += filt;

                self->buffer[k][j][self->in_count[k][j]] = inval + junction - self->lastSamples[k][j];
                self->lastSamples[k][j] = filt;
                if(self->in_count[k][j] == 0)
                    self->buffer[k][j][self->size[k][j]] = self->buffer[k][j][0];
                self->in_count[k][j]++;
                if (self->in_count[k][j] >= self->size[k][j])
                    self->in_count[k][j] = 0;
            }
            self->buffer_streams[i+k*self->bufsize] = self->total_signal[k] * 0.25;
        }
    }
}

static void
STReverb_mix_i(STReverb *self) {
    int i, k;
    MYFLT val;

    MYFLT mix = PyFloat_AS_DOUBLE(self->mix);

    if (mix < 0.0)
        mix = 0.0;
    else if (mix > 1.0)
        mix = 1.0;

    for (i=0; i<self->bufsize; i++) {
        for (k=0; k<2; k++) {
            val = self->input_buffer[k][i] + (self->buffer_streams[i+k*self->bufsize] - self->input_buffer[k][i]) * mix;
            self->buffer_streams[i+k*self->bufsize] = val;
        }
    }
}

static void
STReverb_mix_a(STReverb *self) {
    int i, k;
    MYFLT mix, val;

    MYFLT *mi = Stream_getData((Stream *)self->mix_stream);

    for (i=0; i<self->bufsize; i++) {
        mix = mi[i];
        if (mix < 0.0)
            mix = 0.0;
        else if (mix > 1.0)
            mix = 1.0;

        for (k=0; k<2; k++) {
            val = self->input_buffer[k][i] + (self->buffer_streams[i+k*self->bufsize] - self->input_buffer[k][i]) * mix;
            self->buffer_streams[i+k*self->bufsize] = val;
        }
    }
}

static void
STReverb_setProcMode(STReverb *self)
{
    int procmode, mixmode;
    procmode = self->modebuffer[0] + self->modebuffer[2] * 10;
    mixmode = self->modebuffer[3];

	switch (procmode) {
        case 0:
            self->proc_func_ptr = STReverb_process_ii;
            break;
        case 1:
            self->proc_func_ptr = STReverb_process_ai;
            break;
        case 10:
            self->proc_func_ptr = STReverb_process_ia;
            break;
        case 11:
            self->proc_func_ptr = STReverb_process_aa;
            break;
    }
    switch (mixmode) {
        case 0:
            self->mix_func_ptr = STReverb_mix_i;
            break;
        case 1:
            self->mix_func_ptr = STReverb_mix_a;
            break;
    }
}

MYFLT *
STReverb_getSamplesBuffer(STReverb *self)
{
    return (MYFLT *)self->buffer_streams;
}

static void
STReverb_compute_next_data_frame(STReverb *self)
{
    (*self->proc_func_ptr)(self);
    (*self->mix_func_ptr)(self);
}

static int
STReverb_traverse(STReverb *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->inpos);
    Py_VISIT(self->inpos_stream);
    Py_VISIT(self->revtime);
    Py_VISIT(self->revtime_stream);
    Py_VISIT(self->cutoff);
    Py_VISIT(self->cutoff_stream);
    Py_VISIT(self->mix);
    Py_VISIT(self->mix_stream);
    return 0;
}

static int
STReverb_clear(STReverb *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->inpos);
    Py_CLEAR(self->inpos_stream);
    Py_CLEAR(self->revtime);
    Py_CLEAR(self->revtime_stream);
    Py_CLEAR(self->cutoff);
    Py_CLEAR(self->cutoff_stream);
    Py_CLEAR(self->mix);
    Py_CLEAR(self->mix_stream);
    return 0;
}

static void
STReverb_dealloc(STReverb* self)
{
    int i, k;
    pyo_DEALLOC
    for (k=0; k<2; k++) {
            free(self->input_buffer[k]);
        for (i=0; i<8; i++) {
            free(self->buffer[k][i]);
        }
    }
    for (i=0; i<NUM_REFS; i++) {
        free(self->ref_buffer[i]);
    }
    free(self->buffer_streams);
    STReverb_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
STReverb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j, k, din;
    long maxsize;
    MYFLT roomSize = 1.0;
    MYFLT firstRefTmp = -3.0;
    PyObject *inputtmp, *input_streamtmp, *inpostmp=NULL, *revtimetmp=NULL, *cutofftmp=NULL, *mixtmp=NULL;
    STReverb *self;
    self = (STReverb *)type->tp_alloc(type, 0);

    self->inpos = PyFloat_FromDouble(0.5);
    self->revtime = PyFloat_FromDouble(0.5);
    self->cutoff = PyFloat_FromDouble(5000.0);
    self->mix = PyFloat_FromDouble(0.5);
    self->total_signal[0] = self->total_signal[1] = self->lastFreq = self->damp[0] = self->damp[1] = 0.0;
    self->lastInpos = -1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON

    self->nyquist = self->sr * 0.49;
    self->srfac = self->sr / 44100.0;

    Stream_setFunctionPtr(self->stream, STReverb_compute_next_data_frame);
    self->mode_func_ptr = STReverb_setProcMode;

    static char *kwlist[] = {"input", "inpos", "revtime", "cutoff", "mix", "roomSize", "firstRefGain", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOOOFF, kwlist, &inputtmp, &inpostmp, &revtimetmp, &cutofftmp, &mixtmp, &roomSize, &firstRefTmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (inpostmp) {
        PyObject_CallMethod((PyObject *)self, "setInpos", "O", inpostmp);
    }

    if (revtimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRevtime", "O", revtimetmp);
    }

    if (cutofftmp) {
        PyObject_CallMethod((PyObject *)self, "setCutoff", "O", cutofftmp);
    }

    if (mixtmp) {
        PyObject_CallMethod((PyObject *)self, "setMix", "O", mixtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->firstRefGain = MYPOW(10.0, firstRefTmp * 0.05);
    if (roomSize < 0.25)
        roomSize = 0.25;
    else if (roomSize > 4.0)
        roomSize = 4.0;

    self->avg_time = 0.0;
    for (k=0; k<2; k++) {
        din = k * 3;
        for (i=0; i<8; i++) {
            self->in_count[k][i] = 0;
            self->lastSamples[k][i] = 0.0;
            self->rnd[k][i] = self->rnd_value[k][i] = self->rnd_oldValue[k][i] = self->rnd_diff[k][i] = 0.0;
            self->rnd_time[k][i] = 1.0;
            self->rnd_timeInc[k][i] = reverbParams[i][2] * randomScaling / self->sr;
            self->rnd_range[k][i] = reverbParams[i][1] * randomScaling * self->sr;
            self->rnd_halfRange[k][i] = self->rnd_range[k][i] * 0.5;
            self->delays[k][i] = reverbParams[i][din] * self->srfac * roomSize;
            self->avg_time += self->delays[k][i] / self->sr;
            self->size[k][i] = reverbParams[i][din] * self->srfac * roomSize + (int)(reverbParams[i][1] * self->sr + 0.5);
            maxsize = reverbParams[i][din] * self->srfac * 4.0 + (int)(reverbParams[i][1] * self->sr + 0.5);
            self->buffer[k][i] = (MYFLT *)realloc(self->buffer[k][i], (maxsize+1) * sizeof(MYFLT));
            for (j=0; j<(maxsize+1); j++) {
                self->buffer[k][i][j] = 0.;
            }
        }
    }
    self->avg_time /= 16.0;

    for (k=0; k<NUM_REFS; k++) {
        self->ref_in_count[k] = 0;
        self->ref_size[k] = (int)(first_ref_delays[k] * self->srfac * roomSize + 0.5);
        maxsize = (int)(first_ref_delays[k] * self->srfac * 4.0 + 0.5);
        self->ref_buffer[k] = (MYFLT *)realloc(self->ref_buffer[k], (maxsize+1) * sizeof(MYFLT));
        for (i=0; i<(maxsize+1); i++) {
            self->ref_buffer[k][i] = 0.0;
        }
    }

    for (k=0; k<2; k++) {
        self->input_buffer[k] = (MYFLT *)realloc(self->input_buffer[k], self->bufsize * sizeof(MYFLT));
        for (i=0; i<self->bufsize; i++) {
            self->input_buffer[k][i] = 0.0;
        }
    }

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 2 * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(2 * self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * STReverb_getServer(STReverb* self) { GET_SERVER };
static PyObject * STReverb_getStream(STReverb* self) { GET_STREAM };

static PyObject * STReverb_play(STReverb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * STReverb_stop(STReverb *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
STReverb_reset(STReverb *self)
{
    int i, j, k, maxsize;
    
    for (k=0; k<2; k++) {
        for (i=0; i<8; i++) {
            self->in_count[k][i] = 0;
            self->lastSamples[k][i] = 0.0;
            for (j=0; j<self->size[k][i]; j++) {
                self->buffer[k][i][j] = 0.;
            }
        }
    }
    for (k=0; k<NUM_REFS; k++) {
        self->ref_in_count[k] = 0;
        maxsize = (int)(first_ref_delays[k] * self->srfac * 4.0 + 0.5);
        for (i=0; i<(maxsize+1); i++) {
            self->ref_buffer[k][i] = 0.0;
        }
    }
    for (k=0; k<2; k++) {
        for (i=0; i<self->bufsize; i++) {
            self->input_buffer[k][i] = 0.0;
        }
    }

    for (i=0; i<(2 * self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    self->total_signal[0] = self->total_signal[1] = 0.0;

	Py_RETURN_NONE;
}

static PyObject *
STReverb_setInpos(STReverb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->inpos);
	if (isNumber == 1) {
		self->inpos = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->inpos = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->inpos, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->inpos_stream);
        self->inpos_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
STReverb_setRevtime(STReverb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->revtime);
	if (isNumber == 1) {
		self->revtime = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
	}
	else {
		self->revtime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->revtime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->revtime_stream);
        self->revtime_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
STReverb_setCutoff(STReverb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->cutoff);
	if (isNumber == 1) {
		self->cutoff = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->cutoff = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->cutoff, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->cutoff_stream);
        self->cutoff_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
STReverb_setMix(STReverb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->mix);
	if (isNumber == 1) {
		self->mix = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->mix = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->mix, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->mix_stream);
        self->mix_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
STReverb_setRoomSize(STReverb *self, PyObject *arg)
{
    int i, j, k, din;
    long maxsize;
	MYFLT roomSize;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
        roomSize = PyFloat_AsDouble(arg);
        if (roomSize < 0.25)
            roomSize = 0.25;
        else if (roomSize > 4.0)
            roomSize = 4.0;

        self->avg_time = 0.0;
        for (k=0; k<2; k++) {
            din = k * 3;
            for (i=0; i<8; i++) {
                self->in_count[k][i] = 0;
                self->lastSamples[k][i] = 0.0;
                self->rnd[k][i] = self->rnd_value[k][i] = self->rnd_oldValue[k][i] = self->rnd_diff[k][i] = 0.0;
                self->rnd_time[k][i] = 1.0;
                self->delays[k][i] = reverbParams[i][din] * self->srfac * roomSize;
                self->avg_time += self->delays[k][i] / self->sr;
                self->size[k][i] = reverbParams[i][din] * self->srfac * roomSize + (int)(reverbParams[i][1] * self->sr + 0.5);
                maxsize = reverbParams[i][din] * self->srfac * 2 + (int)(reverbParams[i][1] * self->sr + 0.5);
                for (j=0; j<(maxsize+1); j++) {
                    self->buffer[k][i][j] = 0.;
                }
            }
        }
        self->avg_time /= 16.0;

        for (k=0; k<NUM_REFS; k++) {
            self->ref_in_count[k] = 0;
            self->ref_size[k] = (int)(first_ref_delays[k] * self->srfac * roomSize + 0.5);
            maxsize = (int)(first_ref_delays[k] * self->srfac * 2 + 0.5);
            for (i=0; i<(maxsize+1); i++) {
                self->ref_buffer[k][i] = 0.0;
            }
        }
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
STReverb_setFirstRefGain(STReverb *self, PyObject *arg)
{
	MYFLT tmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
        tmp = PyFloat_AsDouble(arg);
        self->firstRefGain = MYPOW(10.0, tmp * 0.05);
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef STReverb_members[] = {
{"server", T_OBJECT_EX, offsetof(STReverb, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(STReverb, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(STReverb, input), 0, "Input sound object."},
{"inpos", T_OBJECT_EX, offsetof(STReverb, inpos), 0, "Position left-right of the source."},
{"revtime", T_OBJECT_EX, offsetof(STReverb, revtime), 0, "Reverb duration value."},
{"cutoff", T_OBJECT_EX, offsetof(STReverb, cutoff), 0, "STReverb lowpass filter cutoff."},
{"mix", T_OBJECT_EX, offsetof(STReverb, mix), 0, "Balance between dry and wet signals."},
{NULL}  /* Sentinel */
};

static PyMethodDef STReverb_methods[] = {
{"getServer", (PyCFunction)STReverb_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)STReverb_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)STReverb_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)STReverb_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"reset", (PyCFunction)STReverb_reset, METH_NOARGS, "Reset the delay line."},
{"setInpos", (PyCFunction)STReverb_setInpos, METH_O, "Sets position of the source between 0 -> 1."},
{"setRevtime", (PyCFunction)STReverb_setRevtime, METH_O, "Sets reverb duration in seconds."},
{"setCutoff", (PyCFunction)STReverb_setCutoff, METH_O, "Sets lowpass filter cutoff."},
{"setMix", (PyCFunction)STReverb_setMix, METH_O, "Sets balance between dry and wet signals."},
{"setFirstRefGain", (PyCFunction)STReverb_setFirstRefGain, METH_O, "Sets gain of the first reflexions."},
{"setRoomSize", (PyCFunction)STReverb_setRoomSize, METH_O, "Sets room size scaler."},
{NULL}  /* Sentinel */
};

PyTypeObject STReverbType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.STReverb_base",         /*tp_name*/
sizeof(STReverb),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)STReverb_dealloc, /*tp_dealloc*/
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
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"STReverb objects. Waveguide-based reverberation network.",           /* tp_doc */
(traverseproc)STReverb_traverse,   /* tp_traverse */
(inquiry)STReverb_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
STReverb_methods,             /* tp_methods */
STReverb_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
STReverb_new,                 /* tp_new */
};

/************************************************************************************************/
/* STReverb streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    STReverb *mainSplitter;
    int modebuffer[2];
    int chnl; // STRev order
} STRev;

static void STRev_postprocessing_ii(STRev *self) { POST_PROCESSING_II };
static void STRev_postprocessing_ai(STRev *self) { POST_PROCESSING_AI };
static void STRev_postprocessing_ia(STRev *self) { POST_PROCESSING_IA };
static void STRev_postprocessing_aa(STRev *self) { POST_PROCESSING_AA };
static void STRev_postprocessing_ireva(STRev *self) { POST_PROCESSING_IREVA };
static void STRev_postprocessing_areva(STRev *self) { POST_PROCESSING_AREVA };
static void STRev_postprocessing_revai(STRev *self) { POST_PROCESSING_REVAI };
static void STRev_postprocessing_revaa(STRev *self) { POST_PROCESSING_REVAA };
static void STRev_postprocessing_revareva(STRev *self) { POST_PROCESSING_REVAREVA };

static void
STRev_setProcMode(STRev *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = STRev_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = STRev_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = STRev_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = STRev_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = STRev_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = STRev_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = STRev_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = STRev_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = STRev_postprocessing_revareva;
            break;
    }
}

static void
STRev_compute_next_data_frame(STRev *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = STReverb_getSamplesBuffer((STReverb *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
STRev_traverse(STRev *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int
STRev_clear(STRev *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);
    return 0;
}

static void
STRev_dealloc(STRev* self)
{
    pyo_DEALLOC
    STRev_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
STRev_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    STRev *self;
    self = (STRev *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, STRev_compute_next_data_frame);
    self->mode_func_ptr = STRev_setProcMode;

    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (STReverb *)maintmp;

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

static PyObject * STRev_getServer(STRev* self) { GET_SERVER };
static PyObject * STRev_getStream(STRev* self) { GET_STREAM };
static PyObject * STRev_setMul(STRev *self, PyObject *arg) { SET_MUL };
static PyObject * STRev_setAdd(STRev *self, PyObject *arg) { SET_ADD };
static PyObject * STRev_setSub(STRev *self, PyObject *arg) { SET_SUB };
static PyObject * STRev_setDiv(STRev *self, PyObject *arg) { SET_DIV };

static PyObject * STRev_play(STRev *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * STRev_out(STRev *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * STRev_stop(STRev *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * STRev_multiply(STRev *self, PyObject *arg) { MULTIPLY };
static PyObject * STRev_inplace_multiply(STRev *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * STRev_add(STRev *self, PyObject *arg) { ADD };
static PyObject * STRev_inplace_add(STRev *self, PyObject *arg) { INPLACE_ADD };
static PyObject * STRev_sub(STRev *self, PyObject *arg) { SUB };
static PyObject * STRev_inplace_sub(STRev *self, PyObject *arg) { INPLACE_SUB };
static PyObject * STRev_div(STRev *self, PyObject *arg) { DIV };
static PyObject * STRev_inplace_div(STRev *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef STRev_members[] = {
{"server", T_OBJECT_EX, offsetof(STRev, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(STRev, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(STRev, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(STRev, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef STRev_methods[] = {
{"getServer", (PyCFunction)STRev_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)STRev_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)STRev_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)STRev_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)STRev_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setMul", (PyCFunction)STRev_setMul, METH_O, "Sets STRev mul factor."},
{"setAdd", (PyCFunction)STRev_setAdd, METH_O, "Sets STRev add factor."},
{"setSub", (PyCFunction)STRev_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)STRev_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods STRev_as_number = {
(binaryfunc)STRev_add,                      /*nb_add*/
(binaryfunc)STRev_sub,                 /*nb_subtract*/
(binaryfunc)STRev_multiply,                 /*nb_multiply*/
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
(binaryfunc)STRev_inplace_add,              /*inplace_add*/
(binaryfunc)STRev_inplace_sub,         /*inplace_subtract*/
(binaryfunc)STRev_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)STRev_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)STRev_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject STRevType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.STRev_base",         /*tp_name*/
sizeof(STRev),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)STRev_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
0,                         /*tp_repr*/
&STRev_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"STRev objects. Reads one channel from a STReverb object.",           /* tp_doc */
(traverseproc)STRev_traverse,   /* tp_traverse */
(inquiry)STRev_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
STRev_methods,             /* tp_methods */
STRev_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
0,      /* tp_init */
0,                         /* tp_alloc */
STRev_new,                 /* tp_new */
};