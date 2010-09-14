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

/************/
/* Follower */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add 
    // sample memories
    float x1;
    float x2;
    float y1;
    float y2;
    // variables
    float c;
    float w0;
    float alpha;
    // coefficients
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;
} Follower;

static void 
Follower_compute_coeffs_lp(Follower *self)
{
    self->b0 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Follower_compute_variables(Follower *self, float freq, float q)
{
    //float w0, c, alpha;
    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->w0 = TWOPI * freq / self->sr;
    self->c = cosf(self->w0);
    self->alpha = sinf(self->w0) / (2 * q);
    Follower_compute_coeffs_lp((Follower *)self);
}

static void
Follower_filters_i(Follower *self) {
    float absin, val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        val = ( (self->b0 * absin) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = absin;
        self->data[i] = val;
    }
}

static void
Follower_filters_a(Follower *self) {
    float val, absin;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Follower_compute_variables(self, fr[i], 1.0);
        absin = in[i] * in[i];
        val = ( (self->b0 * absin) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = absin;
        self->data[i] = val;
    }
}

static void Follower_postprocessing_ii(Follower *self) { POST_PROCESSING_II };
static void Follower_postprocessing_ai(Follower *self) { POST_PROCESSING_AI };
static void Follower_postprocessing_ia(Follower *self) { POST_PROCESSING_IA };
static void Follower_postprocessing_aa(Follower *self) { POST_PROCESSING_AA };
static void Follower_postprocessing_ireva(Follower *self) { POST_PROCESSING_IREVA };
static void Follower_postprocessing_areva(Follower *self) { POST_PROCESSING_AREVA };
static void Follower_postprocessing_revai(Follower *self) { POST_PROCESSING_REVAI };
static void Follower_postprocessing_revaa(Follower *self) { POST_PROCESSING_REVAA };
static void Follower_postprocessing_revareva(Follower *self) { POST_PROCESSING_REVAREVA };

static void
Follower_setProcMode(Follower *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            Follower_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), 1.0);
            self->proc_func_ptr = Follower_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = Follower_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Follower_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Follower_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Follower_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Follower_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Follower_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Follower_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Follower_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Follower_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Follower_postprocessing_revareva;
            break;
    }   
}

static void
Follower_compute_next_data_frame(Follower *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Follower_traverse(Follower *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Follower_clear(Follower *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Follower_dealloc(Follower* self)
{
    free(self->data);
    Follower_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Follower_deleteStream(Follower *self) { DELETE_STREAM };

static PyObject *
Follower_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Follower *self;
    self = (Follower *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->x1 = self->x2 = self->y1 = self->y2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower_compute_next_data_frame);
    self->mode_func_ptr = Follower_setProcMode;
    return (PyObject *)self;
}

static int
Follower_init(Follower *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Follower_getServer(Follower* self) { GET_SERVER };
static PyObject * Follower_getStream(Follower* self) { GET_STREAM };
static PyObject * Follower_setMul(Follower *self, PyObject *arg) { SET_MUL };	
static PyObject * Follower_setAdd(Follower *self, PyObject *arg) { SET_ADD };	
static PyObject * Follower_setSub(Follower *self, PyObject *arg) { SET_SUB };	
static PyObject * Follower_setDiv(Follower *self, PyObject *arg) { SET_DIV };	

static PyObject * Follower_play(Follower *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Follower_stop(Follower *self) { STOP };

static PyObject * Follower_multiply(Follower *self, PyObject *arg) { MULTIPLY };
static PyObject * Follower_inplace_multiply(Follower *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Follower_add(Follower *self, PyObject *arg) { ADD };
static PyObject * Follower_inplace_add(Follower *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Follower_sub(Follower *self, PyObject *arg) { SUB };
static PyObject * Follower_inplace_sub(Follower *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Follower_div(Follower *self, PyObject *arg) { DIV };
static PyObject * Follower_inplace_div(Follower *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Follower_setFreq(Follower *self, PyObject *arg)
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

static PyMemberDef Follower_members[] = {
{"server", T_OBJECT_EX, offsetof(Follower, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Follower, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Follower, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Follower, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Follower, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Follower, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Follower_methods[] = {
{"getServer", (PyCFunction)Follower_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Follower_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Follower_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Follower_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Follower_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Follower_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Follower_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Follower_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Follower_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Follower_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Follower_as_number = {
(binaryfunc)Follower_add,                         /*nb_add*/
(binaryfunc)Follower_sub,                         /*nb_subtract*/
(binaryfunc)Follower_multiply,                    /*nb_multiply*/
(binaryfunc)Follower_div,                                              /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)Follower_inplace_add,                 /*inplace_add*/
(binaryfunc)Follower_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Follower_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Follower_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject FollowerType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Follower_base",                                   /*tp_name*/
sizeof(Follower),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Follower_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Follower_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Follower objects. Envelope follower.",           /* tp_doc */
(traverseproc)Follower_traverse,                  /* tp_traverse */
(inquiry)Follower_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Follower_methods,                                 /* tp_methods */
Follower_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Follower_init,                          /* tp_init */
0,                                              /* tp_alloc */
Follower_new,                                     /* tp_new */
};

/************/
/* ZCross */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    float thresh;
    float lastValue;
    float lastSample;
    int modebuffer[2]; // need at least 2 slots for mul & add
} ZCross;

static void
ZCross_process(ZCross *self) {
    int i;
    int count = 0;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->lastValue;
        inval = in[i];
        if (self->lastSample >= 0.0) {
            if (inval < 0.0 && (self->lastSample-inval) > self->thresh)
                count++;
        }
        else {
            if (inval >= 0.0 && (inval-self->lastSample) > self->thresh)
                count++;
        }
        self->lastSample = inval;
    }
    self->lastValue = (float)count / self->bufsize;
}

static void ZCross_postprocessing_ii(ZCross *self) { POST_PROCESSING_II };
static void ZCross_postprocessing_ai(ZCross *self) { POST_PROCESSING_AI };
static void ZCross_postprocessing_ia(ZCross *self) { POST_PROCESSING_IA };
static void ZCross_postprocessing_aa(ZCross *self) { POST_PROCESSING_AA };
static void ZCross_postprocessing_ireva(ZCross *self) { POST_PROCESSING_IREVA };
static void ZCross_postprocessing_areva(ZCross *self) { POST_PROCESSING_AREVA };
static void ZCross_postprocessing_revai(ZCross *self) { POST_PROCESSING_REVAI };
static void ZCross_postprocessing_revaa(ZCross *self) { POST_PROCESSING_REVAA };
static void ZCross_postprocessing_revareva(ZCross *self) { POST_PROCESSING_REVAREVA };

static void
ZCross_setProcMode(ZCross *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = ZCross_process;
 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = ZCross_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = ZCross_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = ZCross_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = ZCross_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = ZCross_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = ZCross_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = ZCross_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = ZCross_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = ZCross_postprocessing_revareva;
            break;
    }   
}

static void
ZCross_compute_next_data_frame(ZCross *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
ZCross_traverse(ZCross *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
ZCross_clear(ZCross *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
ZCross_dealloc(ZCross* self)
{
    free(self->data);
    ZCross_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * ZCross_deleteStream(ZCross *self) { DELETE_STREAM };

static PyObject *
ZCross_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    ZCross *self;
    self = (ZCross *)type->tp_alloc(type, 0);
    
    self->thresh = 0.0;
    self->lastValue = self->lastSample = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ZCross_compute_next_data_frame);
    self->mode_func_ptr = ZCross_setProcMode;
    return (PyObject *)self;
}

static int
ZCross_init(ZCross *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "thresh", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|fOO", kwlist, &inputtmp, &self->thresh, &multmp, &addtmp))
        return -1; 

    INIT_INPUT_STREAM
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * ZCross_getServer(ZCross* self) { GET_SERVER };
static PyObject * ZCross_getStream(ZCross* self) { GET_STREAM };
static PyObject * ZCross_setMul(ZCross *self, PyObject *arg) { SET_MUL };	
static PyObject * ZCross_setAdd(ZCross *self, PyObject *arg) { SET_ADD };	
static PyObject * ZCross_setSub(ZCross *self, PyObject *arg) { SET_SUB };	
static PyObject * ZCross_setDiv(ZCross *self, PyObject *arg) { SET_DIV };	

static PyObject * ZCross_play(ZCross *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ZCross_stop(ZCross *self) { STOP };

static PyObject * ZCross_multiply(ZCross *self, PyObject *arg) { MULTIPLY };
static PyObject * ZCross_inplace_multiply(ZCross *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ZCross_add(ZCross *self, PyObject *arg) { ADD };
static PyObject * ZCross_inplace_add(ZCross *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ZCross_sub(ZCross *self, PyObject *arg) { SUB };
static PyObject * ZCross_inplace_sub(ZCross *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ZCross_div(ZCross *self, PyObject *arg) { DIV };
static PyObject * ZCross_inplace_div(ZCross *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ZCross_setThresh(ZCross *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->thresh = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
  
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef ZCross_members[] = {
{"server", T_OBJECT_EX, offsetof(ZCross, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(ZCross, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(ZCross, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(ZCross, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(ZCross, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef ZCross_methods[] = {
{"getServer", (PyCFunction)ZCross_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)ZCross_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)ZCross_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)ZCross_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)ZCross_stop, METH_NOARGS, "Stops computing."},
{"setThresh", (PyCFunction)ZCross_setThresh, METH_O, "Sets the threshold factor."},
{"setMul", (PyCFunction)ZCross_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)ZCross_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)ZCross_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)ZCross_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods ZCross_as_number = {
(binaryfunc)ZCross_add,                         /*nb_add*/
(binaryfunc)ZCross_sub,                         /*nb_subtract*/
(binaryfunc)ZCross_multiply,                    /*nb_multiply*/
(binaryfunc)ZCross_div,                                              /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
0,                                              /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
0,                                              /*nb_oct*/
0,                                              /*nb_hex*/
(binaryfunc)ZCross_inplace_add,                 /*inplace_add*/
(binaryfunc)ZCross_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)ZCross_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)ZCross_inplace_div,                                              /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
0,                                              /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
0,                                              /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ZCrossType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.ZCross_base",                                   /*tp_name*/
sizeof(ZCross),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)ZCross_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&ZCross_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"ZCross objects. Returns number of zero crossing.",           /* tp_doc */
(traverseproc)ZCross_traverse,                  /* tp_traverse */
(inquiry)ZCross_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
ZCross_methods,                                 /* tp_methods */
ZCross_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)ZCross_init,                          /* tp_init */
0,                                              /* tp_alloc */
ZCross_new,                                     /* tp_new */
};
