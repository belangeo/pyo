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
    PyObject *transpo;
    Stream *transpo_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT *envelope;
    MYFLT winsize;
	MYFLT pointerPos;
    int in_count;
    MYFLT *buffer; // samples memory
    int modebuffer[4];
} Harmonizer;

static void
Harmonizer_transform_ii(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    ratio = MYPOW(2.0, trans/12.0);
	rate = (ratio-1.0) / self->winsize;	
    inc = -rate / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
		/* first overlap */
		pos = self->pointerPos;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
        
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] = val * amp;

		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos >= 1)
            pos -= 1.0;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0.0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count] = in[i]  + (self->data[i] * feed);
        if (self->in_count == 0)
            self->buffer[(int)self->sr] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }    
}

static void
Harmonizer_transform_ai(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
	    
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		ratio = MYPOW(2.0, trans[i]/12.0);
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos >= 1)
            pos -= 1.0;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count] = in[i]  + (self->data[i] * feed);
        if (self->in_count == 0)
            self->buffer[(int)self->sr] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }  
}

static void
Harmonizer_transform_ia(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT trans = PyFloat_AS_DOUBLE(self->transpo);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
	ratio = MYPOW(2.0, trans/12.0);
	rate = (ratio-1.0) / self->winsize;	
    inc = -rate / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
		/* first overlap */
		pos = self->pointerPos;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count] = in[i]  + (self->data[i] * feed[i]);
        if (self->in_count == 0)
            self->buffer[(int)self->sr] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }  
}

static void
Harmonizer_transform_aa(Harmonizer *self) {
    MYFLT val, amp, inc, ratio, rate, del, xind, pos, envpos, fpart;
    int i, ipart;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *trans = Stream_getData((Stream *)self->transpo_stream);
    MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
	
	MYFLT oneOnWinsize = 1.0 / self->winsize;
	MYFLT oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
		ratio = MYPOW(2.0, trans[i]/12.0);
		rate = (ratio-1.0) * oneOnWinsize;	
		inc = -rate * oneOnSr;;
		
		/* first overlap */
		pos = self->pointerPos;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] = val * amp;
		
		/* second overlap */
		pos = self->pointerPos + 0.5;
        if (pos > 1)
            pos -= 1.0;
		envpos = pos * 8192.0;
		ipart = (int)envpos;
		fpart = envpos - ipart;
		amp = self->envelope[ipart] + (self->envelope[ipart+1] - self->envelope[ipart]) * fpart;
		
		del = pos * self->winsize;
        xind = self->in_count - (del * self->sr);
        if (xind < 0)
            xind += self->sr;
        ipart = (int)xind;
        fpart = xind - ipart;
        val = self->buffer[ipart] + (self->buffer[ipart+1] - self->buffer[ipart]) * fpart;
        self->data[i] += (val * amp);
		
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
		
		self->buffer[self->in_count] = in[i]  + (self->data[i] * feed[i]);
        if (self->in_count == 0)
            self->buffer[(int)self->sr] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->sr)
            self->in_count = 0;
    }
} 

static void Harmonizer_feedbacktprocessing_ii(Harmonizer *self) { POST_PROCESSING_II };
static void Harmonizer_feedbacktprocessing_ai(Harmonizer *self) { POST_PROCESSING_AI };
static void Harmonizer_feedbacktprocessing_ia(Harmonizer *self) { POST_PROCESSING_IA };
static void Harmonizer_feedbacktprocessing_aa(Harmonizer *self) { POST_PROCESSING_AA };
static void Harmonizer_feedbacktprocessing_ireva(Harmonizer *self) { POST_PROCESSING_IREVA };
static void Harmonizer_feedbacktprocessing_areva(Harmonizer *self) { POST_PROCESSING_AREVA };
static void Harmonizer_feedbacktprocessing_revai(Harmonizer *self) { POST_PROCESSING_REVAI };
static void Harmonizer_feedbacktprocessing_revaa(Harmonizer *self) { POST_PROCESSING_REVAA };
static void Harmonizer_feedbacktprocessing_revareva(Harmonizer *self) { POST_PROCESSING_REVAREVA };

static void
Harmonizer_setProcMode(Harmonizer *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Harmonizer_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Harmonizer_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Harmonizer_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Harmonizer_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Harmonizer_feedbacktprocessing_revareva;
            break;
    }   
}

static void
Harmonizer_compute_next_data_frame(Harmonizer *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Harmonizer_traverse(Harmonizer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->transpo);    
    Py_VISIT(self->transpo_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Harmonizer_clear(Harmonizer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->transpo);    
    Py_CLEAR(self->transpo_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Harmonizer_dealloc(Harmonizer* self)
{
    free(self->data);   
    free(self->buffer);
    free(self->envelope);
    Harmonizer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Harmonizer_deleteStream(Harmonizer *self) { DELETE_STREAM };

static PyObject *
Harmonizer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Harmonizer *self;
    self = (Harmonizer *)type->tp_alloc(type, 0);

    self->transpo = PyFloat_FromDouble(-7.0);
    self->feedback = PyFloat_FromDouble(0.0);
    self->winsize = 0.1;
    self->pointerPos = 1.0;
	self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Harmonizer_compute_next_data_frame);
    self->mode_func_ptr = Harmonizer_setProcMode;

    return (PyObject *)self;
}

static int
Harmonizer_init(Harmonizer *self, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT wintmp;
    PyObject *inputtmp, *input_streamtmp, *transpotmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "transpo", "feedback", "winsize", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &transpotmp, &feedbacktmp, &wintmp, &multmp, &addtmp))
        return -1; 

	INIT_INPUT_STREAM

    if (transpotmp) {
        PyObject_CallMethod((PyObject *)self, "setTranspo", "O", transpotmp);
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

    self->buffer = (MYFLT *)realloc(self->buffer, (self->sr+1) * sizeof(MYFLT));
    for (i=0; i<(self->sr+1); i++) {
        self->buffer[i] = 0.;
    }    
	
    if (wintmp <= 1.0)
        self->winsize = wintmp;
    else
        printf("Harmonizer : winsize larger than 1.0 second, keeping default value.\n");

    self->envelope = (MYFLT *)realloc(self->envelope, 8193 * sizeof(MYFLT));
    for (i=0; i<8192; i++) {
        self->envelope[i] = 0.5 + (MYCOS(TWOPI * (i - 4095) / 8192.0) * 0.5);
        /* if (i < 4096) {
            self->envelope[i] = MYSQRT(i / 4096.0);
        }
        else {
            self->envelope[i] = MYSQRT((8191-i) / 4096.0);
        } */
    }
    self->envelope[8192] = 0.0;

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Harmonizer_getServer(Harmonizer* self) { GET_SERVER };
static PyObject * Harmonizer_getStream(Harmonizer* self) { GET_STREAM };
static PyObject * Harmonizer_setMul(Harmonizer *self, PyObject *arg) { SET_MUL };	
static PyObject * Harmonizer_setAdd(Harmonizer *self, PyObject *arg) { SET_ADD };	
static PyObject * Harmonizer_setSub(Harmonizer *self, PyObject *arg) { SET_SUB };	
static PyObject * Harmonizer_setDiv(Harmonizer *self, PyObject *arg) { SET_DIV };	

static PyObject * Harmonizer_play(Harmonizer *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Harmonizer_out(Harmonizer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Harmonizer_stop(Harmonizer *self) { STOP };

static PyObject * Harmonizer_multiply(Harmonizer *self, PyObject *arg) { MULTIPLY };
static PyObject * Harmonizer_inplace_multiply(Harmonizer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Harmonizer_add(Harmonizer *self, PyObject *arg) { ADD };
static PyObject * Harmonizer_inplace_add(Harmonizer *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Harmonizer_sub(Harmonizer *self, PyObject *arg) { SUB };
static PyObject * Harmonizer_inplace_sub(Harmonizer *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Harmonizer_div(Harmonizer *self, PyObject *arg) { DIV };
static PyObject * Harmonizer_inplace_div(Harmonizer *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Harmonizer_setTranspo(Harmonizer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->transpo);
	if (isNumber == 1) {
		self->transpo = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->transpo = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->transpo, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->transpo_stream);
        self->transpo_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Harmonizer_setFeedback(Harmonizer *self, PyObject *arg)
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

static PyObject *
Harmonizer_setWinsize(Harmonizer *self, PyObject *arg)
{
	MYFLT wintmp;
	if (arg != NULL) {
        wintmp = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
		if (wintmp <= 1.0)
			self->winsize = wintmp;
        else
            printf("winsize larger than 1.0 second!\n");
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Harmonizer_members[] = {
    {"server", T_OBJECT_EX, offsetof(Harmonizer, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Harmonizer, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Harmonizer, input), 0, "Input sound object."},
    {"transpo", T_OBJECT_EX, offsetof(Harmonizer, transpo), 0, "Transposition factor."},
    {"feedback", T_OBJECT_EX, offsetof(Harmonizer, feedback), 0, "Feedback factor."},
    {"mul", T_OBJECT_EX, offsetof(Harmonizer, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Harmonizer, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Harmonizer_methods[] = {
    {"getServer", (PyCFunction)Harmonizer_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Harmonizer_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Harmonizer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Harmonizer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Harmonizer_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Harmonizer_stop, METH_NOARGS, "Stops computing."},
	{"setTranspo", (PyCFunction)Harmonizer_setTranspo, METH_O, "Sets global transpo factor."},
    {"setFeedback", (PyCFunction)Harmonizer_setFeedback, METH_O, "Sets feedback factor."},
    {"setWinsize", (PyCFunction)Harmonizer_setWinsize, METH_O, "Sets the window size."},
	{"setMul", (PyCFunction)Harmonizer_setMul, METH_O, "Sets granulator mul factor."},
	{"setAdd", (PyCFunction)Harmonizer_setAdd, METH_O, "Sets granulator add factor."},
    {"setSub", (PyCFunction)Harmonizer_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Harmonizer_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Harmonizer_as_number = {
    (binaryfunc)Harmonizer_add,                      /*nb_add*/
    (binaryfunc)Harmonizer_sub,                 /*nb_subtract*/
    (binaryfunc)Harmonizer_multiply,                 /*nb_multiply*/
    (binaryfunc)Harmonizer_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_feedback*/
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
    (binaryfunc)Harmonizer_inplace_add,              /*inplace_add*/
    (binaryfunc)Harmonizer_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Harmonizer_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Harmonizer_inplace_div,           /*inplace_divide*/
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

PyTypeObject HarmonizerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_transpo*/
    "_pyo.Harmonizer_base",         /*tp_name*/
    sizeof(Harmonizer),         /*tp_basictranspo*/
    0,                         /*tp_itemtranspo*/
    (destructor)Harmonizer_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Harmonizer_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Harmonizer objects. Harmonize an input sound.",           /* tp_doc */
    (traverseproc)Harmonizer_traverse,   /* tp_traverse */
    (inquiry)Harmonizer_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Harmonizer_methods,             /* tp_methods */
    Harmonizer_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Harmonizer_init,      /* tp_init */
    0,                         /* tp_alloc */
    Harmonizer_new,                 /* tp_new */
};

