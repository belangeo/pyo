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

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    float maxdelay;
    long size;
    int in_count;
    int modebuffer[4];
    float *buffer; // samples memory
} Delay;

static void
Delay_process_ii(Delay *self) {
    float val, xind, frac;
    int i, ind;

    float del = PyFloat_AS_DOUBLE(self->delay);
    float feed = PyFloat_AS_DOUBLE(self->feedback);
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    float sampdel = del * self->sr;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += (self->size-1);
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count] = in[i] + (val * feed);
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ai(Delay *self) {
    float val, xind, frac, sampdel, del;
    int i, ind;

    float *delobj = Stream_getData((Stream *)self->delay_stream);    
    float feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += (self->size-1);
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count++] = in[i]  + (val * feed);
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ia(Delay *self) {
    float val, xind, frac, feed;
    int i, ind;
    
    float del = PyFloat_AS_DOUBLE(self->delay);
    float *fdb = Stream_getData((Stream *)self->feedback_stream);    
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    float sampdel = del * self->sr;
       
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += (self->size-1);
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;

        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count++] = in[i] + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_aa(Delay *self) {
    float val, xind, frac, sampdel, feed, del;
    int i, ind;
    
    float *delobj = Stream_getData((Stream *)self->delay_stream);    
    float *fdb = Stream_getData((Stream *)self->feedback_stream);    
  
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += (self->size-1);
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count++] = in[i] + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Delay_postprocessing_ii(Delay *self) { POST_PROCESSING_II };
static void Delay_postprocessing_ai(Delay *self) { POST_PROCESSING_AI };
static void Delay_postprocessing_ia(Delay *self) { POST_PROCESSING_IA };
static void Delay_postprocessing_aa(Delay *self) { POST_PROCESSING_AA };
static void Delay_postprocessing_ireva(Delay *self) { POST_PROCESSING_IREVA };
static void Delay_postprocessing_areva(Delay *self) { POST_PROCESSING_AREVA };
static void Delay_postprocessing_revai(Delay *self) { POST_PROCESSING_REVAI };
static void Delay_postprocessing_revaa(Delay *self) { POST_PROCESSING_REVAA };
static void Delay_postprocessing_revareva(Delay *self) { POST_PROCESSING_REVAREVA };

static void
Delay_setProcMode(Delay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Delay_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Delay_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Delay_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Delay_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Delay_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Delay_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Delay_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Delay_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Delay_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Delay_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Delay_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Delay_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Delay_postprocessing_revareva;
            break;
    } 
}

static void
Delay_compute_next_data_frame(Delay *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Delay_traverse(Delay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->delay);    
    Py_VISIT(self->delay_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Delay_clear(Delay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->delay);    
    Py_CLEAR(self->delay_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Delay_dealloc(Delay* self)
{
    free(self->data);
    free(self->buffer);
    Delay_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Delay_deleteStream(Delay *self) { DELETE_STREAM };

static PyObject *
Delay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Delay *self;
    self = (Delay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0);
    self->feedback = PyFloat_FromDouble(0);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Delay_compute_next_data_frame);
    self->mode_func_ptr = Delay_setProcMode;
    
    return (PyObject *)self;
}

static int
Delay_init(Delay *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "delay", "feedback", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxdelay, &multmp, &addtmp))
        return -1; 

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = self->maxdelay * self->sr + 0.5;

    self->buffer = (float *)realloc(self->buffer, (self->size+1) * sizeof(float));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Delay_getServer(Delay* self) { GET_SERVER };
static PyObject * Delay_getStream(Delay* self) { GET_STREAM };
static PyObject * Delay_setMul(Delay *self, PyObject *arg) { SET_MUL };	
static PyObject * Delay_setAdd(Delay *self, PyObject *arg) { SET_ADD };	
static PyObject * Delay_setSub(Delay *self, PyObject *arg) { SET_SUB };	
static PyObject * Delay_setDiv(Delay *self, PyObject *arg) { SET_DIV };	

static PyObject * Delay_play(Delay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Delay_out(Delay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Delay_stop(Delay *self) { STOP };

static PyObject * Delay_multiply(Delay *self, PyObject *arg) { MULTIPLY };
static PyObject * Delay_inplace_multiply(Delay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Delay_add(Delay *self, PyObject *arg) { ADD };
static PyObject * Delay_inplace_add(Delay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Delay_sub(Delay *self, PyObject *arg) { SUB };
static PyObject * Delay_inplace_sub(Delay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Delay_div(Delay *self, PyObject *arg) { DIV };
static PyObject * Delay_inplace_div(Delay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Delay_setDelay(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Delay_setFeedback(Delay *self, PyObject *arg)
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
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Delay_members[] = {
    {"server", T_OBJECT_EX, offsetof(Delay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Delay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Delay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Delay, delay), 0, "Delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Delay, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Delay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Delay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Delay_methods[] = {
    {"getServer", (PyCFunction)Delay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Delay_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Delay_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Delay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Delay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Delay_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)Delay_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Delay_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
	{"setMul", (PyCFunction)Delay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Delay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Delay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Delay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Delay_as_number = {
    (binaryfunc)Delay_add,                      /*nb_add*/
    (binaryfunc)Delay_sub,                 /*nb_subtract*/
    (binaryfunc)Delay_multiply,                 /*nb_multiply*/
    (binaryfunc)Delay_div,                   /*nb_divide*/
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
    (binaryfunc)Delay_inplace_add,              /*inplace_add*/
    (binaryfunc)Delay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Delay_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Delay_inplace_div,           /*inplace_divide*/
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

PyTypeObject DelayType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Delay_base",         /*tp_name*/
    sizeof(Delay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Delay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Delay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Delay objects. Delay signal by x samples.",           /* tp_doc */
    (traverseproc)Delay_traverse,   /* tp_traverse */
    (inquiry)Delay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Delay_methods,             /* tp_methods */
    Delay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Delay_init,      /* tp_init */
    0,                         /* tp_alloc */
    Delay_new,                 /* tp_new */
};

/*********************/
/***** Waveguide *****/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *dur;
    Stream *dur_stream;
    float minfreq;
    float lastFreq;
    float lastSampDel;
    float lastDur;
    float lastFeed;
    long size;
    int in_count;
    int modebuffer[4];
    float lpsamp; // lowpass sample memory
    float coeffs[5]; // lagrange coefficients
    float lagrange[4]; // lagrange samples memories
    float xn1; // dc block input delay
    float yn1; // dc block output delay
    float *buffer; // samples memory
} Waveguide;

static void
Waveguide_process_ii(Waveguide *self) {
    float val, x, y, sampdel, frac, feed;
    int i, ind, isamp;
    
    float fr = PyFloat_AS_DOUBLE(self->freq);
    float dur = PyFloat_AS_DOUBLE(self->dur); 
    float *in = Stream_getData((Stream *)self->input_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    if (dur <= 0)
        dur = 0.1;
    
    
    sampdel = self->lastSampDel;
    feed = self->lastFeed;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = 1.0 / fr * self->sr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
        
        self->lastDur = dur;
        feed = powf(100, -(1.0/fr)/dur);
        self->lastFeed = feed;
    } 
    else if (dur != self->lastDur) {
        self->lastDur = dur;
        feed = powf(100, -(1.0/fr)/dur);
        self->lastFeed = feed;
    }
    
    /* pick a new value in th delay line */
    isamp = (int)sampdel;  
    for (i=0; i<self->bufsize; i++) {
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+(self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;
        
        /* cliping, needed? */
        if (x < -1.0)
            x = -1.0;
        else if (x > 1.0)
            x = 1.0;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count++] = in[i] + (x * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Waveguide_process_ai(Waveguide *self) {
    float val, x, y, sampdel, frac, feed, freq;
    int i, ind, isamp;
    
    float *fr =Stream_getData((Stream *)self->freq_stream);
    float dur = PyFloat_AS_DOUBLE(self->dur); 
    float *in = Stream_getData((Stream *)self->input_stream);
    
    /* Check dur boundary */
    if (dur <= 0)
        dur = 0.1;

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        /* Check frequency boundary */
        if (freq < self->minfreq)
            freq = self->minfreq;

        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = 1.0 / freq * self->sr - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
            
            self->lastDur = dur;
            feed = powf(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = powf(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }

        /* pick a new value in th delay line */
        isamp = (int)sampdel;        
        
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+(self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;
        
        /* cliping, needed? */
        if (x < -1.0)
            x = -1.0;
        else if (x > 1.0)
            x = 1.0;
        
        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count++] = in[i] + (x * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_ia(Waveguide *self) {
    float val, x, y, sampdel, frac, feed, dur;
    int i, ind, isamp;
    
    float fr = PyFloat_AS_DOUBLE(self->freq);
    float *du = Stream_getData((Stream *)self->dur_stream);
    float *in = Stream_getData((Stream *)self->input_stream);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;

    sampdel = self->lastSampDel;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = 1.0 / fr * self->sr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
    }
    
    /* pick a new value in th delay line */
    isamp = (int)sampdel;  
    for (i=0; i<self->bufsize; i++) {
        feed = self->lastFeed;
        dur = du[i];
        if (dur <= 0)
            dur = 0.1;
        if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = powf(100, -(1.0/fr)/dur);
            self->lastFeed = feed;
        }
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+(self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;
        
        /* cliping, needed? */
        if (x < -1.0)
            x = -1.0;
        else if (x > 1.0)
            x = 1.0;
        
        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count++] = in[i] + (x * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_aa(Waveguide *self) {
    float val, x, y, sampdel, frac, feed, freq, dur;
    int i, ind, isamp;
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *du = Stream_getData((Stream *)self->dur_stream); 
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        dur = du[i];
        /* Check boundaries */
        if (freq < self->minfreq)
            freq = self->minfreq;
        if (dur <= 0)
            dur = 0.1;
        
        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = 1.0 / freq * self->sr - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
            
            self->lastDur = dur;
            feed = powf(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = powf(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        
        /* pick a new value in th delay line */
        isamp = (int)sampdel;        
        
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+(self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;
        
        /* cliping, needed? */
        if (x < -1.0)
            x = -1.0;
        else if (x > 1.0)
            x = 1.0;
        
        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count++] = in[i] + (x * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Waveguide_postprocessing_ii(Waveguide *self) { POST_PROCESSING_II };
static void Waveguide_postprocessing_ai(Waveguide *self) { POST_PROCESSING_AI };
static void Waveguide_postprocessing_ia(Waveguide *self) { POST_PROCESSING_IA };
static void Waveguide_postprocessing_aa(Waveguide *self) { POST_PROCESSING_AA };
static void Waveguide_postprocessing_ireva(Waveguide *self) { POST_PROCESSING_IREVA };
static void Waveguide_postprocessing_areva(Waveguide *self) { POST_PROCESSING_AREVA };
static void Waveguide_postprocessing_revai(Waveguide *self) { POST_PROCESSING_REVAI };
static void Waveguide_postprocessing_revaa(Waveguide *self) { POST_PROCESSING_REVAA };
static void Waveguide_postprocessing_revareva(Waveguide *self) { POST_PROCESSING_REVAREVA };

static void
Waveguide_setProcMode(Waveguide *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Waveguide_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Waveguide_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Waveguide_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Waveguide_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Waveguide_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Waveguide_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Waveguide_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Waveguide_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Waveguide_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Waveguide_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Waveguide_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Waveguide_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Waveguide_postprocessing_revareva;
            break;
    } 
}

static void
Waveguide_compute_next_data_frame(Waveguide *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Waveguide_traverse(Waveguide *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
Waveguide_clear(Waveguide *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
Waveguide_dealloc(Waveguide* self)
{
    free(self->data);
    free(self->buffer);
    Waveguide_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Waveguide_deleteStream(Waveguide *self) { DELETE_STREAM };

static PyObject *
Waveguide_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Waveguide *self;
    self = (Waveguide *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->dur = PyFloat_FromDouble(0.99);
    self->minfreq = 20;
    self->lastFreq = -1.0;
    self->lastSampDel = -1.0;
    self->lastDur = -1.0;
    self->lastFeed = 0.0;
    self->in_count = 0;
    self->lpsamp = 0.0;
    for(i=0; i<4; i++) {
        self->lagrange[i] = 0.0;
    }    
    self->xn1 = 0.0;
    self->yn1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Waveguide_compute_next_data_frame);
    self->mode_func_ptr = Waveguide_setProcMode;
    
    return (PyObject *)self;
}

static int
Waveguide_init(Waveguide *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "freq", "dur", "minfreq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &freqtmp, &durtmp, &self->minfreq, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->size = (long)(1.0 / self->minfreq * self->sr + 0.5);
    
    self->buffer = (float *)realloc(self->buffer, (self->size+1) * sizeof(float));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Waveguide_getServer(Waveguide* self) { GET_SERVER };
static PyObject * Waveguide_getStream(Waveguide* self) { GET_STREAM };
static PyObject * Waveguide_setMul(Waveguide *self, PyObject *arg) { SET_MUL };	
static PyObject * Waveguide_setAdd(Waveguide *self, PyObject *arg) { SET_ADD };	
static PyObject * Waveguide_setSub(Waveguide *self, PyObject *arg) { SET_SUB };	
static PyObject * Waveguide_setDiv(Waveguide *self, PyObject *arg) { SET_DIV };	

static PyObject * Waveguide_play(Waveguide *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Waveguide_out(Waveguide *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Waveguide_stop(Waveguide *self) { STOP };

static PyObject * Waveguide_multiply(Waveguide *self, PyObject *arg) { MULTIPLY };
static PyObject * Waveguide_inplace_multiply(Waveguide *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Waveguide_add(Waveguide *self, PyObject *arg) { ADD };
static PyObject * Waveguide_inplace_add(Waveguide *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Waveguide_sub(Waveguide *self, PyObject *arg) { SUB };
static PyObject * Waveguide_inplace_sub(Waveguide *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Waveguide_div(Waveguide *self, PyObject *arg) { DIV };
static PyObject * Waveguide_inplace_div(Waveguide *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Waveguide_setFreq(Waveguide *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Waveguide_setDur(Waveguide *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Waveguide_members[] = {
{"server", T_OBJECT_EX, offsetof(Waveguide, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Waveguide, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Waveguide, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Waveguide, freq), 0, "Waveguide time in seconds."},
{"dur", T_OBJECT_EX, offsetof(Waveguide, dur), 0, "Feedback value."},
{"mul", T_OBJECT_EX, offsetof(Waveguide, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Waveguide, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Waveguide_methods[] = {
{"getServer", (PyCFunction)Waveguide_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Waveguide_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Waveguide_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Waveguide_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Waveguide_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Waveguide_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Waveguide_setFreq, METH_O, "Sets freq time in seconds."},
{"setDur", (PyCFunction)Waveguide_setDur, METH_O, "Sets dur value between 0 -> 1."},
{"setMul", (PyCFunction)Waveguide_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Waveguide_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Waveguide_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Waveguide_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Waveguide_as_number = {
(binaryfunc)Waveguide_add,                      /*nb_add*/
(binaryfunc)Waveguide_sub,                 /*nb_subtract*/
(binaryfunc)Waveguide_multiply,                 /*nb_multiply*/
(binaryfunc)Waveguide_div,                   /*nb_divide*/
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
(binaryfunc)Waveguide_inplace_add,              /*inplace_add*/
(binaryfunc)Waveguide_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Waveguide_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Waveguide_inplace_div,           /*inplace_divide*/
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

PyTypeObject WaveguideType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Waveguide_base",         /*tp_name*/
sizeof(Waveguide),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Waveguide_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Waveguide_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Waveguide objects. Waveguide signal by x samples.",           /* tp_doc */
(traverseproc)Waveguide_traverse,   /* tp_traverse */
(inquiry)Waveguide_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Waveguide_methods,             /* tp_methods */
Waveguide_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Waveguide_init,      /* tp_init */
0,                         /* tp_alloc */
Waveguide_new,                 /* tp_new */
};
