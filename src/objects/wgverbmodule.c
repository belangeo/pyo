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

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"


static const MYFLT randomScaling = 0.5;
static const MYFLT reverbParams[8][3] = {
{ 2473.0, 0.0010, 3.100 },
{ 2767.0, 0.0011, 3.500 },
{ 3217.0, 0.0017, 1.110 },
{ 3557.0, 0.0006, 3.973 },
{ 3907.0, 0.0010, 2.341 },
{ 4127.0, 0.0011, 1.897 },
{ 2143.0, 0.0017, 0.891 },
{ 1933.0, 0.0006, 3.221 }
};

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
                self->rnd_value[j] = self->rnd_range[j] * (rand()/((MYFLT)(RAND_MAX)+1)) - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];
            
            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += (self->size[j]-1);
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
                self->rnd_value[j] = self->rnd_range[j] * (rand()/((MYFLT)(RAND_MAX)+1)) - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];
            
            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += (self->size[j]-1);
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
                self->rnd_value[j] = self->rnd_range[j] * (rand()/((MYFLT)(RAND_MAX)+1)) - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];
            
            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += (self->size[j]-1);
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
                self->rnd_value[j] = self->rnd_range[j] * (rand()/((MYFLT)(RAND_MAX)+1)) - self->rnd_halfRange[j];
                self->rnd_diff[j] = self->rnd_value[j] - self->rnd_oldValue[j];
            }
            self->rnd[j] = self->rnd_oldValue[j] + self->rnd_diff[j] * self->rnd_time[j];
            
            xind = self->in_count[j] - (self->delays[j] + self->rnd[j]);
            if (xind < 0)
                xind += (self->size[j]-1);
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
    Stream_setData(self->stream, self->data);
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
    free(self->data);
    for (i=0; i<8; i++) {
        free(self->buffer[i]);
    }    
    WGVerb_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * WGVerb_deleteStream(WGVerb *self) { DELETE_STREAM };

static PyObject *
WGVerb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    WGVerb *self;
    self = (WGVerb *)type->tp_alloc(type, 0);
    
    self->feedback = PyFloat_FromDouble(0.5);
    self->cutoff = PyFloat_FromDouble(5000.0);
    self->mix = PyFloat_FromDouble(0.5);
    self->lastFreq = self->damp = 0.0;
    
    for (i=0; i<8; i++) {
        self->in_count[i] = 0;
        self->lastSamples[i] = 0.0;
        self->rnd_value[i] = self->rnd_oldValue[i] = self->rnd_diff[i] = 0.0;
        self->rnd_time[i] = 1.0;
    }    
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
    
    return (PyObject *)self;
}

static int
WGVerb_init(WGVerb *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *feedbacktmp=NULL, *cutofftmp=NULL, *mixtmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i, j;
    
    static char *kwlist[] = {"input", "feedback", "cutoff", "mix", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOO", kwlist, &inputtmp, &feedbacktmp, &cutofftmp, &mixtmp, &multmp, &addtmp))
        return -1; 
    
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
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    for (i=0; i<8; i++) {
        self->size[i] = reverbParams[i][0] * (self->sr / 44100.0) + (int)(reverbParams[i][1] * self->sr + 0.5);
        self->buffer[i] = (MYFLT *)realloc(self->buffer[i], (self->size[i]+1) * sizeof(MYFLT));
        for (j=0; j<(self->size[i]+1); j++) {
            self->buffer[i][j] = 0.;
        }    
    }    
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * WGVerb_getServer(WGVerb* self) { GET_SERVER };
static PyObject * WGVerb_getStream(WGVerb* self) { GET_STREAM };
static PyObject * WGVerb_setMul(WGVerb *self, PyObject *arg) { SET_MUL };	
static PyObject * WGVerb_setAdd(WGVerb *self, PyObject *arg) { SET_ADD };	
static PyObject * WGVerb_setSub(WGVerb *self, PyObject *arg) { SET_SUB };	
static PyObject * WGVerb_setDiv(WGVerb *self, PyObject *arg) { SET_DIV };	

static PyObject * WGVerb_play(WGVerb *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * WGVerb_out(WGVerb *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * WGVerb_stop(WGVerb *self) { STOP };

static PyObject * WGVerb_multiply(WGVerb *self, PyObject *arg) { MULTIPLY };
static PyObject * WGVerb_inplace_multiply(WGVerb *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * WGVerb_add(WGVerb *self, PyObject *arg) { ADD };
static PyObject * WGVerb_inplace_add(WGVerb *self, PyObject *arg) { INPLACE_ADD };
static PyObject * WGVerb_sub(WGVerb *self, PyObject *arg) { SUB };
static PyObject * WGVerb_inplace_sub(WGVerb *self, PyObject *arg) { INPLACE_SUB };
static PyObject * WGVerb_div(WGVerb *self, PyObject *arg) { DIV };
static PyObject * WGVerb_inplace_div(WGVerb *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
WGVerb_setFeedback(WGVerb *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
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
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
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
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
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
{"deleteStream", (PyCFunction)WGVerb_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)WGVerb_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)WGVerb_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)WGVerb_stop, METH_NOARGS, "Stops computing."},
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
(binaryfunc)WGVerb_div,                   /*nb_divide*/
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
0,                                          /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
0,                       /*nb_oct*/
0,                       /*nb_hex*/
(binaryfunc)WGVerb_inplace_add,              /*inplace_add*/
(binaryfunc)WGVerb_inplace_sub,         /*inplace_subtract*/
(binaryfunc)WGVerb_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)WGVerb_inplace_div,           /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
0,              /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
0,      /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject WGVerbType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.WGVerb_base",         /*tp_name*/
sizeof(WGVerb),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)WGVerb_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
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
"WGVerb objects. WGVerb signal by x samples.",           /* tp_doc */
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
(initproc)WGVerb_init,      /* tp_init */
0,                         /* tp_alloc */
WGVerb_new,                 /* tp_new */
};
