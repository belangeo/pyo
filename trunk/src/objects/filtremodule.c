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
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    int filtertype;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} Biquad;

static void 
Biquad_compute_coeffs_lp(Biquad *self)
{
    self->b0 = self->b2 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_hp(Biquad *self)
{
    self->b0 = (1 + self->c) / 2;
    self->b1 = -(1 + self->c);
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_bp(Biquad *self)
{
    self->b0 = self->alpha;
    self->b1 = 0;
    self->b2 = -self->alpha;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_bs(Biquad *self)
{
    self->b0 = 1;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = 1;
    self->a0 = 1 + self->alpha;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_ap(Biquad *self)
{
    self->b0 = self->a2 = 1 - self->alpha;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = self->a0 = 1 + self->alpha;
}

static void
Biquad_compute_variables(Biquad *self, MYFLT freq, MYFLT q)
{    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
Biquad_filters_ii(Biquad *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ai(Biquad *self) {
    MYFLT val, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    
    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ia(Biquad *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr, q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_aa(Biquad *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void Biquad_postprocessing_ii(Biquad *self) { POST_PROCESSING_II };
static void Biquad_postprocessing_ai(Biquad *self) { POST_PROCESSING_AI };
static void Biquad_postprocessing_ia(Biquad *self) { POST_PROCESSING_IA };
static void Biquad_postprocessing_aa(Biquad *self) { POST_PROCESSING_AA };
static void Biquad_postprocessing_ireva(Biquad *self) { POST_PROCESSING_IREVA };
static void Biquad_postprocessing_areva(Biquad *self) { POST_PROCESSING_AREVA };
static void Biquad_postprocessing_revai(Biquad *self) { POST_PROCESSING_REVAI };
static void Biquad_postprocessing_revaa(Biquad *self) { POST_PROCESSING_REVAA };
static void Biquad_postprocessing_revareva(Biquad *self) { POST_PROCESSING_REVAREVA };

static void
Biquad_setProcMode(Biquad *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = Biquad_compute_coeffs_lp;
            break;
        case 1:
            self->coeffs_func_ptr = Biquad_compute_coeffs_hp;
            break;
        case 2:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bp;
            break;
        case 3:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bs;
            break;
        case 4:
            self->coeffs_func_ptr = Biquad_compute_coeffs_ap;
            break;
    }
    
	switch (procmode) {
        case 0:    
            Biquad_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Biquad_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Biquad_filters_ai;
            break;
        case 10:        
            self->proc_func_ptr = Biquad_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Biquad_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Biquad_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Biquad_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Biquad_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Biquad_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Biquad_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Biquad_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Biquad_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Biquad_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Biquad_postprocessing_revareva;
            break;
    }   
}

static void
Biquad_compute_next_data_frame(Biquad *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Biquad_traverse(Biquad *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->q);    
    Py_VISIT(self->q_stream);    
    return 0;
}

static int 
Biquad_clear(Biquad *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->q);    
    Py_CLEAR(self->q_stream);    
    return 0;
}

static void
Biquad_dealloc(Biquad* self)
{
    free(self->data);
    Biquad_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Biquad_deleteStream(Biquad *self) { DELETE_STREAM };

static PyObject *
Biquad_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Biquad *self;
    self = (Biquad *)type->tp_alloc(type, 0);
        
    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->filtertype = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Biquad_compute_next_data_frame);
    self->mode_func_ptr = Biquad_setProcMode;
    return (PyObject *)self;
}

static int
Biquad_init(Biquad *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "freq", "q", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->filtertype, &multmp, &addtmp))
        return -1; 

    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
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

static PyObject * Biquad_getServer(Biquad* self) { GET_SERVER };
static PyObject * Biquad_getStream(Biquad* self) { GET_STREAM };
static PyObject * Biquad_setMul(Biquad *self, PyObject *arg) { SET_MUL };	
static PyObject * Biquad_setAdd(Biquad *self, PyObject *arg) { SET_ADD };	
static PyObject * Biquad_setSub(Biquad *self, PyObject *arg) { SET_SUB };	
static PyObject * Biquad_setDiv(Biquad *self, PyObject *arg) { SET_DIV };	

static PyObject * Biquad_play(Biquad *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Biquad_out(Biquad *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquad_stop(Biquad *self) { STOP };

static PyObject * Biquad_multiply(Biquad *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquad_inplace_multiply(Biquad *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquad_add(Biquad *self, PyObject *arg) { ADD };
static PyObject * Biquad_inplace_add(Biquad *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquad_sub(Biquad *self, PyObject *arg) { SUB };
static PyObject * Biquad_inplace_sub(Biquad *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquad_div(Biquad *self, PyObject *arg) { DIV };
static PyObject * Biquad_inplace_div(Biquad *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquad_setFreq(Biquad *self, PyObject *arg)
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
Biquad_setQ(Biquad *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Biquad_setType(Biquad *self, PyObject *arg)
{
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Biquad_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquad, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquad, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquad, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Biquad, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Biquad, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Biquad, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquad, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquad_methods[] = {
    {"getServer", (PyCFunction)Biquad_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquad_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Biquad_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Biquad_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquad_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquad_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquad_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquad_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquad_setType, METH_O, "Sets filter type factor."},
	{"setMul", (PyCFunction)Biquad_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquad_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquad_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquad_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquad_as_number = {
    (binaryfunc)Biquad_add,                         /*nb_add*/
    (binaryfunc)Biquad_sub,                         /*nb_subtract*/
    (binaryfunc)Biquad_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquad_div,                                              /*nb_divide*/
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
    (binaryfunc)Biquad_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquad_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquad_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquad_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BiquadType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquad_base",                                   /*tp_name*/
    sizeof(Biquad),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquad_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquad_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquad objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Biquad_traverse,                  /* tp_traverse */
    (inquiry)Biquad_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquad_methods,                                 /* tp_methods */
    Biquad_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Biquad_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquad_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    int filtertype;
    int stages;
    // sample memories
    MYFLT *x1;
    MYFLT *x2;
    MYFLT *y1;
    MYFLT *y2;
    // variables
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} Biquadx;

static void
Biquadx_allocate_memories(Biquadx *self)
{
    self->x1 = (MYFLT *)realloc(self->x1, self->stages * sizeof(MYFLT));
    self->x2 = (MYFLT *)realloc(self->x2, self->stages * sizeof(MYFLT));
    self->y1 = (MYFLT *)realloc(self->y1, self->stages * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * sizeof(MYFLT));
    self->init = 1;
}

static void 
Biquadx_compute_coeffs_lp(Biquadx *self)
{
    self->b0 = self->b2 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquadx_compute_coeffs_hp(Biquadx *self)
{
    self->b0 = (1 + self->c) / 2;
    self->b1 = -(1 + self->c);
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquadx_compute_coeffs_bp(Biquadx *self)
{
    self->b0 = self->alpha;
    self->b1 = 0;
    self->b2 = -self->alpha;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquadx_compute_coeffs_bs(Biquadx *self)
{
    self->b0 = 1;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = 1;
    self->a0 = 1 + self->alpha;
    self->a2 = 1 - self->alpha;
}

static void 
Biquadx_compute_coeffs_ap(Biquadx *self)
{
    self->b0 = self->a2 = 1 - self->alpha;
    self->b1 = self->a1 = -2 * self->c;
    self->b2 = self->a0 = 1 + self->alpha;
}

static void
Biquadx_compute_variables(Biquadx *self, MYFLT freq, MYFLT q)
{    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
Biquadx_filters_ii(Biquadx *self) {
    MYFLT vin, vout;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }    
        self->init = 0;
    }
    
    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        vin = in[i];
        for (j=0; j<self->stages; j++) {   
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_ai(Biquadx *self) {
    MYFLT vin, vout, q;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }    
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    
    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr[i], q);
        vin = in[i];
        for (j=0; j<self->stages; j++) {   
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_ia(Biquadx *self) {
    MYFLT vin, vout, fr;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }    
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr, q[i]);
        vin = in[i];
        for (j=0; j<self->stages; j++) {   
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void
Biquadx_filters_aa(Biquadx *self) {
    MYFLT vin, vout;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (i=0; i<self->stages; i++) {
            self->x1[i] = self->x2[i] = self->y1[i] = self->y2[i] = in[0];
        }    
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    vout = 0.0;
    for (i=0; i<self->bufsize; i++) {
        Biquadx_compute_variables(self, fr[i], q[i]);
        vin = in[i];
        for (j=0; j<self->stages; j++) {   
            vout = ( (self->b0 * vin) + (self->b1 * self->x1[j]) + (self->b2 * self->x2[j]) - (self->a1 * self->y1[j]) - (self->a2 * self->y2[j]) ) / self->a0;
            self->x2[j] = self->x1[j];
            self->x1[j] = vin;;
            self->y2[j] = self->y1[j];
            self->y1[j] = vin = vout;
        }
        self->data[i] = vout;
    }
}

static void Biquadx_postprocessing_ii(Biquadx *self) { POST_PROCESSING_II };
static void Biquadx_postprocessing_ai(Biquadx *self) { POST_PROCESSING_AI };
static void Biquadx_postprocessing_ia(Biquadx *self) { POST_PROCESSING_IA };
static void Biquadx_postprocessing_aa(Biquadx *self) { POST_PROCESSING_AA };
static void Biquadx_postprocessing_ireva(Biquadx *self) { POST_PROCESSING_IREVA };
static void Biquadx_postprocessing_areva(Biquadx *self) { POST_PROCESSING_AREVA };
static void Biquadx_postprocessing_revai(Biquadx *self) { POST_PROCESSING_REVAI };
static void Biquadx_postprocessing_revaa(Biquadx *self) { POST_PROCESSING_REVAA };
static void Biquadx_postprocessing_revareva(Biquadx *self) { POST_PROCESSING_REVAREVA };

static void
Biquadx_setProcMode(Biquadx *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_lp;
            break;
        case 1:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_hp;
            break;
        case 2:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_bp;
            break;
        case 3:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_bs;
            break;
        case 4:
            self->coeffs_func_ptr = Biquadx_compute_coeffs_ap;
            break;
    }
    
	switch (procmode) {
        case 0:    
            Biquadx_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Biquadx_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Biquadx_filters_ai;
            break;
        case 10:        
            self->proc_func_ptr = Biquadx_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Biquadx_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Biquadx_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Biquadx_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Biquadx_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Biquadx_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Biquadx_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Biquadx_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Biquadx_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Biquadx_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Biquadx_postprocessing_revareva;
            break;
    }   
}

static void
Biquadx_compute_next_data_frame(Biquadx *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Biquadx_traverse(Biquadx *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->q);    
    Py_VISIT(self->q_stream);    
    return 0;
}

static int 
Biquadx_clear(Biquadx *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->q);    
    Py_CLEAR(self->q_stream);    
    return 0;
}

static void
Biquadx_dealloc(Biquadx* self)
{
    free(self->data);
    free(self->x1);
    free(self->x2);
    free(self->y1);
    free(self->y2);
    Biquadx_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Biquadx_deleteStream(Biquadx *self) { DELETE_STREAM };

static PyObject *
Biquadx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Biquadx *self;
    self = (Biquadx *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->filtertype = 0;
    self->stages = 4;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Biquadx_compute_next_data_frame);
    self->mode_func_ptr = Biquadx_setProcMode;
    return (PyObject *)self;
}

static int
Biquadx_init(Biquadx *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "q", "type", "stages", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->filtertype, &self->stages, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Biquadx_allocate_memories(self);

    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Biquadx_getServer(Biquadx* self) { GET_SERVER };
static PyObject * Biquadx_getStream(Biquadx* self) { GET_STREAM };
static PyObject * Biquadx_setMul(Biquadx *self, PyObject *arg) { SET_MUL };	
static PyObject * Biquadx_setAdd(Biquadx *self, PyObject *arg) { SET_ADD };	
static PyObject * Biquadx_setSub(Biquadx *self, PyObject *arg) { SET_SUB };	
static PyObject * Biquadx_setDiv(Biquadx *self, PyObject *arg) { SET_DIV };	

static PyObject * Biquadx_play(Biquadx *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Biquadx_out(Biquadx *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquadx_stop(Biquadx *self) { STOP };

static PyObject * Biquadx_multiply(Biquadx *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquadx_inplace_multiply(Biquadx *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquadx_add(Biquadx *self, PyObject *arg) { ADD };
static PyObject * Biquadx_inplace_add(Biquadx *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquadx_sub(Biquadx *self, PyObject *arg) { SUB };
static PyObject * Biquadx_inplace_sub(Biquadx *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquadx_div(Biquadx *self, PyObject *arg) { DIV };
static PyObject * Biquadx_inplace_div(Biquadx *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquadx_setFreq(Biquadx *self, PyObject *arg)
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
Biquadx_setQ(Biquadx *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Biquadx_setType(Biquadx *self, PyObject *arg)
{
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Biquadx_setStages(Biquadx *self, PyObject *arg)
{
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->stages = PyInt_AsLong(arg);
        Biquadx_allocate_memories(self);
	}

	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Biquadx_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquadx, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquadx, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquadx, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Biquadx, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Biquadx, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Biquadx, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquadx, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquadx_methods[] = {
    {"getServer", (PyCFunction)Biquadx_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquadx_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Biquadx_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Biquadx_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquadx_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquadx_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquadx_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquadx_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquadx_setType, METH_O, "Sets filter type factor."},
    {"setStages", (PyCFunction)Biquadx_setStages, METH_O, "Sets the number of filtering stages."},
	{"setMul", (PyCFunction)Biquadx_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquadx_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquadx_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquadx_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquadx_as_number = {
    (binaryfunc)Biquadx_add,                         /*nb_add*/
    (binaryfunc)Biquadx_sub,                         /*nb_subtract*/
    (binaryfunc)Biquadx_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquadx_div,                                              /*nb_divide*/
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
    (binaryfunc)Biquadx_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquadx_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquadx_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquadx_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BiquadxType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquadx_base",                                   /*tp_name*/
    sizeof(Biquadx),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquadx_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquadx_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquadx objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Biquadx_traverse,                  /* tp_traverse */
    (inquiry)Biquadx_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquadx_methods,                                 /* tp_methods */
    Biquadx_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Biquadx_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquadx_new,                                     /* tp_new */
};

/*** Typical EQ filter ***/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *boost;
    Stream *boost_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[5]; // need at least 2 slots for mul & add 
    int filtertype;
    // sample memories
    MYFLT x1;
    MYFLT x2;
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT A;
    MYFLT c;
    MYFLT w0;
    MYFLT alpha;
    // coefficients
    MYFLT b0;
    MYFLT b1;
    MYFLT b2;
    MYFLT a0;
    MYFLT a1;
    MYFLT a2;
} EQ;

static void 
EQ_compute_coeffs_peak(EQ *self)
{
    MYFLT alphaMul = self->alpha * self->A;
    MYFLT alphaDiv = self->alpha / self->A;
    
    self->b0 = 1.0 + alphaMul;
    self->b1 = self->a1 = -2.0 * self->c;
    self->b2 = 1.0 - alphaMul;
    self->a0 = 1.0 + alphaDiv;
    self->a2 = 1.0 - alphaDiv;
}

static void
EQ_compute_coeffs_lowshelf(EQ *self) 
{
    MYFLT twoSqrtAAlpha = MYSQRT(self->A * 2.0)*self->alpha;
    MYFLT AminOneC = (self->A - 1.0) * self->c;
    MYFLT AAddOneC = (self->A + 1.0) * self->c;
    
    self->b0 = self->A * ((self->A + 1.0) - AminOneC + twoSqrtAAlpha);
    self->b1 = 2.0 * self->A * ((self->A - 1.0) - AAddOneC);
    self->b2 = self->A * ((self->A + 1.0) - AminOneC - twoSqrtAAlpha);
    self->a0 = (self->A + 1.0) + AminOneC + twoSqrtAAlpha;
    self->a1 = -2.0 * ((self->A - 1.0) + AAddOneC);
    self->a2 = (self->A + 1.0) + AminOneC - twoSqrtAAlpha;
}    

static void
EQ_compute_coeffs_highshelf(EQ *self) 
{
    MYFLT twoSqrtAAlpha = MYSQRT(self->A * 2.0)*self->alpha;
    MYFLT AminOneC = (self->A - 1.0) * self->c;
    MYFLT AAddOneC = (self->A + 1.0) * self->c;
    
    self->b0 = self->A * ((self->A + 1.0) + AminOneC + twoSqrtAAlpha);
    self->b1 = -2.0 * self->A * ((self->A - 1.0) + AAddOneC);
    self->b2 = self->A * ((self->A + 1.0) + AminOneC - twoSqrtAAlpha);
    self->a0 = (self->A + 1.0) - AminOneC + twoSqrtAAlpha;
    self->a1 = 2.0 * ((self->A - 1.0) - AAddOneC);
    self->a2 = (self->A + 1.0) - AminOneC - twoSqrtAAlpha;
}    

static void
EQ_compute_variables(EQ *self, MYFLT freq, MYFLT q, MYFLT boost)
{    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->A = MYPOW(10.0, boost/40.0);
    self->w0 = TWOPI * freq / self->sr;
    self->c = MYCOS(self->w0);
    self->alpha = MYSIN(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
EQ_filters_iii(EQ *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aii(EQ *self) {
    MYFLT val, q, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    boost = PyFloat_AS_DOUBLE(self->boost);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q, boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iai(EQ *self) {
    MYFLT val, fr, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    boost = PyFloat_AS_DOUBLE(self->boost);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q[i], boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aai(EQ *self) {
    MYFLT val, boost;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    boost = PyFloat_AS_DOUBLE(self->boost);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q[i], boost);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iia(EQ *self) {
    MYFLT val, fr, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    fr = PyFloat_AS_DOUBLE(self->freq);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);

    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q, boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aia(EQ *self) {
    MYFLT val, q;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q, boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_iaa(EQ *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr, q[i], boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
EQ_filters_aaa(EQ *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    MYFLT *boost = Stream_getData((Stream *)self->boost_stream);
    
    for (i=0; i<self->bufsize; i++) {
        EQ_compute_variables(self, fr[i], q[i], boost[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void EQ_postprocessing_ii(EQ *self) { POST_PROCESSING_II };
static void EQ_postprocessing_ai(EQ *self) { POST_PROCESSING_AI };
static void EQ_postprocessing_ia(EQ *self) { POST_PROCESSING_IA };
static void EQ_postprocessing_aa(EQ *self) { POST_PROCESSING_AA };
static void EQ_postprocessing_ireva(EQ *self) { POST_PROCESSING_IREVA };
static void EQ_postprocessing_areva(EQ *self) { POST_PROCESSING_AREVA };
static void EQ_postprocessing_revai(EQ *self) { POST_PROCESSING_REVAI };
static void EQ_postprocessing_revaa(EQ *self) { POST_PROCESSING_REVAA };
static void EQ_postprocessing_revareva(EQ *self) { POST_PROCESSING_REVAREVA };

static void
EQ_setProcMode(EQ *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = EQ_compute_coeffs_peak;
            break;
        case 1:
            self->coeffs_func_ptr = EQ_compute_coeffs_lowshelf;
            break;
        case 2:
            self->coeffs_func_ptr = EQ_compute_coeffs_highshelf;
            break;
    }
    
	switch (procmode) {
        case 0:    
            EQ_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q), PyFloat_AS_DOUBLE(self->boost));
            self->proc_func_ptr = EQ_filters_iii;
            break;
        case 1:    
            self->proc_func_ptr = EQ_filters_aii;
            break;
        case 10:        
            self->proc_func_ptr = EQ_filters_iai;
            break;
        case 11:    
            self->proc_func_ptr = EQ_filters_aai;
            break;
        case 100:    
            self->proc_func_ptr = EQ_filters_iia;
            break;
        case 101:    
            self->proc_func_ptr = EQ_filters_aia;
            break;
        case 110:        
            self->proc_func_ptr = EQ_filters_iaa;
            break;
        case 111:    
            self->proc_func_ptr = EQ_filters_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = EQ_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = EQ_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = EQ_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = EQ_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = EQ_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = EQ_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = EQ_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = EQ_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = EQ_postprocessing_revareva;
            break;
    }   
}

static void
EQ_compute_next_data_frame(EQ *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
EQ_traverse(EQ *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->q);    
    Py_VISIT(self->q_stream);    
    Py_VISIT(self->boost);    
    Py_VISIT(self->boost_stream);
    return 0;
}

static int 
EQ_clear(EQ *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->q);    
    Py_CLEAR(self->q_stream);
    Py_CLEAR(self->boost);
    Py_CLEAR(self->boost_stream);
    return 0;
}

static void
EQ_dealloc(EQ* self)
{
    free(self->data);
    EQ_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * EQ_deleteStream(EQ *self) { DELETE_STREAM };

static PyObject *
EQ_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    EQ *self;
    self = (EQ *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->boost = PyFloat_FromDouble(-3.0);
    self->filtertype = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->init = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, EQ_compute_next_data_frame);
    self->mode_func_ptr = EQ_setProcMode;
    return (PyObject *)self;
}

static int
EQ_init(EQ *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *boosttmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "q", "boost", "type", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &boosttmp, &self->filtertype, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }

    if (boosttmp) {
        PyObject_CallMethod((PyObject *)self, "setBoost", "O", boosttmp);
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

static PyObject * EQ_getServer(EQ* self) { GET_SERVER };
static PyObject * EQ_getStream(EQ* self) { GET_STREAM };
static PyObject * EQ_setMul(EQ *self, PyObject *arg) { SET_MUL };	
static PyObject * EQ_setAdd(EQ *self, PyObject *arg) { SET_ADD };	
static PyObject * EQ_setSub(EQ *self, PyObject *arg) { SET_SUB };	
static PyObject * EQ_setDiv(EQ *self, PyObject *arg) { SET_DIV };	

static PyObject * EQ_play(EQ *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * EQ_out(EQ *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * EQ_stop(EQ *self) { STOP };

static PyObject * EQ_multiply(EQ *self, PyObject *arg) { MULTIPLY };
static PyObject * EQ_inplace_multiply(EQ *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * EQ_add(EQ *self, PyObject *arg) { ADD };
static PyObject * EQ_inplace_add(EQ *self, PyObject *arg) { INPLACE_ADD };
static PyObject * EQ_sub(EQ *self, PyObject *arg) { SUB };
static PyObject * EQ_inplace_sub(EQ *self, PyObject *arg) { INPLACE_SUB };
static PyObject * EQ_div(EQ *self, PyObject *arg) { DIV };
static PyObject * EQ_inplace_div(EQ *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
EQ_setFreq(EQ *self, PyObject *arg)
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
EQ_setQ(EQ *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
EQ_setBoost(EQ *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->boost);
	if (isNumber == 1) {
		self->boost = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->boost = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->boost, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->boost_stream);
        self->boost_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
EQ_setType(EQ *self, PyObject *arg)
{
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef EQ_members[] = {
{"server", T_OBJECT_EX, offsetof(EQ, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(EQ, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(EQ, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(EQ, freq), 0, "Cutoff frequency in cycle per second."},
{"q", T_OBJECT_EX, offsetof(EQ, q), 0, "Q factor."},
{"boost", T_OBJECT_EX, offsetof(EQ, boost), 0, "Boost factor."},
{"mul", T_OBJECT_EX, offsetof(EQ, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(EQ, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef EQ_methods[] = {
{"getServer", (PyCFunction)EQ_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)EQ_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)EQ_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)EQ_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)EQ_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)EQ_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)EQ_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setQ", (PyCFunction)EQ_setQ, METH_O, "Sets filter Q factor."},
{"setBoost", (PyCFunction)EQ_setBoost, METH_O, "Sets filter boost factor."},
{"setType", (PyCFunction)EQ_setType, METH_O, "Sets filter type factor."},
{"setMul", (PyCFunction)EQ_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)EQ_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)EQ_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)EQ_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods EQ_as_number = {
(binaryfunc)EQ_add,                         /*nb_add*/
(binaryfunc)EQ_sub,                         /*nb_subtract*/
(binaryfunc)EQ_multiply,                    /*nb_multiply*/
(binaryfunc)EQ_div,                                              /*nb_divide*/
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
(binaryfunc)EQ_inplace_add,                 /*inplace_add*/
(binaryfunc)EQ_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)EQ_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)EQ_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject EQType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.EQ_base",                                   /*tp_name*/
sizeof(EQ),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)EQ_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&EQ_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"EQ objects. Generates a biquadratic filter.",           /* tp_doc */
(traverseproc)EQ_traverse,                  /* tp_traverse */
(inquiry)EQ_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
EQ_methods,                                 /* tp_methods */
EQ_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)EQ_init,                          /* tp_init */
0,                                              /* tp_alloc */
EQ_new,                                     /* tp_new */
};

/* Performs portamento on audio signal */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    Stream *risetime_stream;
    Stream *falltime_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    MYFLT y1; // sample memory
    MYFLT x1;
    int dir;
} Port;

static void 
direction(Port *self, MYFLT val)
{
    if (val == self->x1)
        return;
    else if (val > self->x1)
        self->dir = 1;
    else
        self->dir = 0;
    self->x1 = val;
}    
    
static void
Port_filters_ii(Port *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT risetime = PyFloat_AS_DOUBLE(self->risetime);
    MYFLT falltime = PyFloat_AS_DOUBLE(self->falltime);
    MYFLT risefactor = 1. / ((risetime + 0.001) * self->sr);
    MYFLT fallfactor = 1. / ((falltime + 0.001) * self->sr);
    MYFLT factors[2] = {fallfactor, risefactor};

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        val = self->y1 + (in[i] - self->y1) * factors[self->dir];
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ai(Port *self) {
    MYFLT val, risefactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *risetime = Stream_getData((Stream *)self->risetime_stream);
    MYFLT falltime = PyFloat_AS_DOUBLE(self->falltime);
    MYFLT fallfactor = 1. / ((falltime + 0.001) * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = (*risetime++ + 0.001) * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) * fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ia(Port *self) {
    MYFLT val, fallfactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *falltime = Stream_getData((Stream *)self->falltime_stream);
    MYFLT risetime = PyFloat_AS_DOUBLE(self->risetime);
    MYFLT risefactor = 1. / ((risetime + 0.001) * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        fallfactor = (*falltime++ + 0.001) * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) * risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_aa(Port *self) {
    MYFLT val, risefactor, fallfactor;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *risetime = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *falltime = Stream_getData((Stream *)self->falltime_stream);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = (*risetime++ + 0.001) * self->sr;  
        fallfactor = (*falltime++ + 0.001) * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void Port_postprocessing_ii(Port *self) { POST_PROCESSING_II };
static void Port_postprocessing_ai(Port *self) { POST_PROCESSING_AI };
static void Port_postprocessing_ia(Port *self) { POST_PROCESSING_IA };
static void Port_postprocessing_aa(Port *self) { POST_PROCESSING_AA };
static void Port_postprocessing_ireva(Port *self) { POST_PROCESSING_IREVA };
static void Port_postprocessing_areva(Port *self) { POST_PROCESSING_AREVA };
static void Port_postprocessing_revai(Port *self) { POST_PROCESSING_REVAI };
static void Port_postprocessing_revaa(Port *self) { POST_PROCESSING_REVAA };
static void Port_postprocessing_revareva(Port *self) { POST_PROCESSING_REVAREVA };

static void
Port_setProcMode(Port *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Port_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Port_filters_ai;
            break;
        case 10:    
            self->proc_func_ptr = Port_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Port_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Port_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Port_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Port_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Port_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Port_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Port_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Port_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Port_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Port_postprocessing_revareva;
            break;
    }  
}

static void
Port_compute_next_data_frame(Port *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Port_traverse(Port *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);    
    Py_VISIT(self->risetime_stream);    
    Py_VISIT(self->falltime);    
    Py_VISIT(self->falltime_stream);    
    return 0;
}

static int 
Port_clear(Port *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);    
    Py_CLEAR(self->risetime_stream);    
    Py_CLEAR(self->falltime);    
    Py_CLEAR(self->falltime_stream);    
    return 0;
}

static void
Port_dealloc(Port* self)
{
    free(self->data);
    Port_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Port_deleteStream(Port *self) { DELETE_STREAM };

static PyObject *
Port_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Port *self;
    self = (Port *)type->tp_alloc(type, 0);
    
    self->risetime = PyFloat_FromDouble(0.05);
    self->falltime = PyFloat_FromDouble(0.05);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->y1 = 0.0;
    self->x1 = 0.0;
    self->dir = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Port_compute_next_data_frame);
    self->mode_func_ptr = Port_setProcMode;
    return (PyObject *)self;
}

static int
Port_init(Port *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    MYFLT inittmp = 0.0;
    
    static char *kwlist[] = {"input", "risetime", "falltime", "init", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &risetimetmp, &falltimetmp, &inittmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    if (inittmp != 0.0)
        self->x1 = self->y1 = inittmp;
        
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Port_getServer(Port* self) { GET_SERVER };
static PyObject * Port_getStream(Port* self) { GET_STREAM };
static PyObject * Port_setMul(Port *self, PyObject *arg) { SET_MUL };	
static PyObject * Port_setAdd(Port *self, PyObject *arg) { SET_ADD };	
static PyObject * Port_setSub(Port *self, PyObject *arg) { SET_SUB };	
static PyObject * Port_setDiv(Port *self, PyObject *arg) { SET_DIV };	

static PyObject * Port_play(Port *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Port_out(Port *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Port_stop(Port *self) { STOP };

static PyObject * Port_multiply(Port *self, PyObject *arg) { MULTIPLY };
static PyObject * Port_inplace_multiply(Port *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Port_add(Port *self, PyObject *arg) { ADD };
static PyObject * Port_inplace_add(Port *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Port_sub(Port *self, PyObject *arg) { SUB };
static PyObject * Port_inplace_sub(Port *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Port_div(Port *self, PyObject *arg) { DIV };
static PyObject * Port_inplace_div(Port *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Port_setRiseTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->risetime);
	if (isNumber == 1) {
		self->risetime = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->risetime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->risetime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->risetime_stream);
        self->risetime_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Port_setFallTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->falltime);
	if (isNumber == 1) {
		self->falltime = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->falltime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->falltime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->falltime_stream);
        self->falltime_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Port_members[] = {
{"server", T_OBJECT_EX, offsetof(Port, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Port, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Port, input), 0, "Input sound object."},
{"risetime", T_OBJECT_EX, offsetof(Port, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Port, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Port, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Port, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Port_methods[] = {
{"getServer", (PyCFunction)Port_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Port_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Port_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Port_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Port_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Port_stop, METH_NOARGS, "Stops computing."},
{"setRiseTime", (PyCFunction)Port_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Port_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setMul", (PyCFunction)Port_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Port_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Port_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Port_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Port_as_number = {
(binaryfunc)Port_add,                         /*nb_add*/
(binaryfunc)Port_sub,                         /*nb_subtract*/
(binaryfunc)Port_multiply,                    /*nb_multiply*/
(binaryfunc)Port_div,                                              /*nb_divide*/
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
(binaryfunc)Port_inplace_add,                 /*inplace_add*/
(binaryfunc)Port_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Port_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Port_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject PortType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Port_base",                                   /*tp_name*/
sizeof(Port),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Port_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Port_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Port objects. Generates a portamento filter.",           /* tp_doc */
(traverseproc)Port_traverse,                  /* tp_traverse */
(inquiry)Port_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Port_methods,                                 /* tp_methods */
Port_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Port_init,                          /* tp_init */
0,                                              /* tp_alloc */
Port_new,                                     /* tp_new */
};

/************/
/* Tone */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add 
    MYFLT lastFreq;
    // sample memories
    MYFLT y1;
    // variables
    MYFLT c1;
    MYFLT c2;
} Tone;

static void
Tone_filters_i(Tone *self) {
    MYFLT val, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        b = 2.0 - MYCOS(TWOPI * fr / self->sr);
        self->c2 = (b - MYSQRT(b * b - 1.0));
        self->c1 = 1.0 - self->c2;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void
Tone_filters_a(Tone *self) {
    MYFLT val, freq, b;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
        
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            b = 2.0 - MYCOS(TWOPI * freq / self->sr);
            self->c2 = (b - MYSQRT(b * b - 1.0));
            self->c1 = 1.0 - self->c2;
        }
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void Tone_postprocessing_ii(Tone *self) { POST_PROCESSING_II };
static void Tone_postprocessing_ai(Tone *self) { POST_PROCESSING_AI };
static void Tone_postprocessing_ia(Tone *self) { POST_PROCESSING_IA };
static void Tone_postprocessing_aa(Tone *self) { POST_PROCESSING_AA };
static void Tone_postprocessing_ireva(Tone *self) { POST_PROCESSING_IREVA };
static void Tone_postprocessing_areva(Tone *self) { POST_PROCESSING_AREVA };
static void Tone_postprocessing_revai(Tone *self) { POST_PROCESSING_REVAI };
static void Tone_postprocessing_revaa(Tone *self) { POST_PROCESSING_REVAA };
static void Tone_postprocessing_revareva(Tone *self) { POST_PROCESSING_REVAREVA };

static void
Tone_setProcMode(Tone *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Tone_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = Tone_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Tone_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Tone_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Tone_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Tone_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Tone_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Tone_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Tone_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Tone_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Tone_postprocessing_revareva;
            break;
    }   
}

static void
Tone_compute_next_data_frame(Tone *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Tone_traverse(Tone *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Tone_clear(Tone *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Tone_dealloc(Tone* self)
{
    free(self->data);
    Tone_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Tone_deleteStream(Tone *self) { DELETE_STREAM };

static PyObject *
Tone_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Tone *self;
    self = (Tone *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->y1 = self->c1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Tone_compute_next_data_frame);
    self->mode_func_ptr = Tone_setProcMode;
    return (PyObject *)self;
}

static int
Tone_init(Tone *self, PyObject *args, PyObject *kwds)
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

static PyObject * Tone_getServer(Tone* self) { GET_SERVER };
static PyObject * Tone_getStream(Tone* self) { GET_STREAM };
static PyObject * Tone_setMul(Tone *self, PyObject *arg) { SET_MUL };	
static PyObject * Tone_setAdd(Tone *self, PyObject *arg) { SET_ADD };	
static PyObject * Tone_setSub(Tone *self, PyObject *arg) { SET_SUB };	
static PyObject * Tone_setDiv(Tone *self, PyObject *arg) { SET_DIV };	

static PyObject * Tone_play(Tone *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Tone_out(Tone *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Tone_stop(Tone *self) { STOP };

static PyObject * Tone_multiply(Tone *self, PyObject *arg) { MULTIPLY };
static PyObject * Tone_inplace_multiply(Tone *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Tone_add(Tone *self, PyObject *arg) { ADD };
static PyObject * Tone_inplace_add(Tone *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Tone_sub(Tone *self, PyObject *arg) { SUB };
static PyObject * Tone_inplace_sub(Tone *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Tone_div(Tone *self, PyObject *arg) { DIV };
static PyObject * Tone_inplace_div(Tone *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Tone_setFreq(Tone *self, PyObject *arg)
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

static PyMemberDef Tone_members[] = {
{"server", T_OBJECT_EX, offsetof(Tone, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Tone, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Tone, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Tone, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Tone, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Tone, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Tone_methods[] = {
{"getServer", (PyCFunction)Tone_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Tone_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Tone_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Tone_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Tone_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Tone_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Tone_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Tone_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Tone_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Tone_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Tone_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Tone_as_number = {
(binaryfunc)Tone_add,                         /*nb_add*/
(binaryfunc)Tone_sub,                         /*nb_subtract*/
(binaryfunc)Tone_multiply,                    /*nb_multiply*/
(binaryfunc)Tone_div,                                              /*nb_divide*/
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
(binaryfunc)Tone_inplace_add,                 /*inplace_add*/
(binaryfunc)Tone_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Tone_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Tone_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject ToneType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Tone_base",                                   /*tp_name*/
sizeof(Tone),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Tone_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Tone_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Tone objects. One-pole recursive lowpass filter.",           /* tp_doc */
(traverseproc)Tone_traverse,                  /* tp_traverse */
(inquiry)Tone_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Tone_methods,                                 /* tp_methods */
Tone_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Tone_init,                          /* tp_init */
0,                                              /* tp_alloc */
Tone_new,                                     /* tp_new */
};

/************/
/* DCBlock */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
    // sample memories
    MYFLT x1;
    MYFLT y1;
} DCBlock;

static void
DCBlock_filters(DCBlock *self) {
    MYFLT x, y;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        x = in[i];
        y = x - self->x1 + 0.995 * self->y1;
        self->x1 = x;
        self->data[i] = self->y1 = y;
    }
}

static void DCBlock_postprocessing_ii(DCBlock *self) { POST_PROCESSING_II };
static void DCBlock_postprocessing_ai(DCBlock *self) { POST_PROCESSING_AI };
static void DCBlock_postprocessing_ia(DCBlock *self) { POST_PROCESSING_IA };
static void DCBlock_postprocessing_aa(DCBlock *self) { POST_PROCESSING_AA };
static void DCBlock_postprocessing_ireva(DCBlock *self) { POST_PROCESSING_IREVA };
static void DCBlock_postprocessing_areva(DCBlock *self) { POST_PROCESSING_AREVA };
static void DCBlock_postprocessing_revai(DCBlock *self) { POST_PROCESSING_REVAI };
static void DCBlock_postprocessing_revaa(DCBlock *self) { POST_PROCESSING_REVAA };
static void DCBlock_postprocessing_revareva(DCBlock *self) { POST_PROCESSING_REVAREVA };

static void
DCBlock_setProcMode(DCBlock *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
       
    self->proc_func_ptr = DCBlock_filters;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = DCBlock_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = DCBlock_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = DCBlock_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = DCBlock_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = DCBlock_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = DCBlock_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = DCBlock_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = DCBlock_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = DCBlock_postprocessing_revareva;
            break;
    }   
}

static void
DCBlock_compute_next_data_frame(DCBlock *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
DCBlock_traverse(DCBlock *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
DCBlock_clear(DCBlock *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
DCBlock_dealloc(DCBlock* self)
{
    free(self->data);
    DCBlock_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * DCBlock_deleteStream(DCBlock *self) { DELETE_STREAM };

static PyObject *
DCBlock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    DCBlock *self;
    self = (DCBlock *)type->tp_alloc(type, 0);
    
    self->x1 = self->y1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, DCBlock_compute_next_data_frame);
    self->mode_func_ptr = DCBlock_setProcMode;
    return (PyObject *)self;
}

static int
DCBlock_init(DCBlock *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
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

static PyObject * DCBlock_getServer(DCBlock* self) { GET_SERVER };
static PyObject * DCBlock_getStream(DCBlock* self) { GET_STREAM };
static PyObject * DCBlock_setMul(DCBlock *self, PyObject *arg) { SET_MUL };	
static PyObject * DCBlock_setAdd(DCBlock *self, PyObject *arg) { SET_ADD };	
static PyObject * DCBlock_setSub(DCBlock *self, PyObject *arg) { SET_SUB };	
static PyObject * DCBlock_setDiv(DCBlock *self, PyObject *arg) { SET_DIV };	

static PyObject * DCBlock_play(DCBlock *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * DCBlock_out(DCBlock *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * DCBlock_stop(DCBlock *self) { STOP };

static PyObject * DCBlock_multiply(DCBlock *self, PyObject *arg) { MULTIPLY };
static PyObject * DCBlock_inplace_multiply(DCBlock *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * DCBlock_add(DCBlock *self, PyObject *arg) { ADD };
static PyObject * DCBlock_inplace_add(DCBlock *self, PyObject *arg) { INPLACE_ADD };
static PyObject * DCBlock_sub(DCBlock *self, PyObject *arg) { SUB };
static PyObject * DCBlock_inplace_sub(DCBlock *self, PyObject *arg) { INPLACE_SUB };
static PyObject * DCBlock_div(DCBlock *self, PyObject *arg) { DIV };
static PyObject * DCBlock_inplace_div(DCBlock *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef DCBlock_members[] = {
{"server", T_OBJECT_EX, offsetof(DCBlock, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(DCBlock, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(DCBlock, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(DCBlock, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(DCBlock, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef DCBlock_methods[] = {
{"getServer", (PyCFunction)DCBlock_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)DCBlock_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)DCBlock_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)DCBlock_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)DCBlock_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)DCBlock_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)DCBlock_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)DCBlock_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)DCBlock_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)DCBlock_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods DCBlock_as_number = {
(binaryfunc)DCBlock_add,                         /*nb_add*/
(binaryfunc)DCBlock_sub,                         /*nb_subtract*/
(binaryfunc)DCBlock_multiply,                    /*nb_multiply*/
(binaryfunc)DCBlock_div,                                              /*nb_divide*/
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
(binaryfunc)DCBlock_inplace_add,                 /*inplace_add*/
(binaryfunc)DCBlock_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)DCBlock_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)DCBlock_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject DCBlockType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.DCBlock_base",                                   /*tp_name*/
sizeof(DCBlock),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)DCBlock_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&DCBlock_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"DCBlock objects. Implements the DC blocking filter.",           /* tp_doc */
(traverseproc)DCBlock_traverse,                  /* tp_traverse */
(inquiry)DCBlock_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
DCBlock_methods,                                 /* tp_methods */
DCBlock_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)DCBlock_init,                          /* tp_init */
0,                                              /* tp_alloc */
DCBlock_new,                                     /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT maxDelay;
    long size;
    int in_count;
    int modebuffer[4];
    MYFLT *buffer; // samples memory
} Allpass;

static void
Allpass_process_ii(Allpass *self) {
    MYFLT val, xind, frac;
    int i, ind;
    
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxDelay)
        del = self->maxDelay;
    MYFLT sampdel = del * self->sr;
    
    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;
        
        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_ai(Allpass *self) {
    MYFLT val, xind, frac, sampdel, del;
    int i, ind;
    
    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);    
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
    
    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxDelay)
            del = self->maxDelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;
        
        self->buffer[self->in_count] = in[i]  + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_ia(Allpass *self) {
    MYFLT val, xind, frac, feed;
    int i, ind;
    
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);    
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxDelay)
        del = self->maxDelay;
    MYFLT sampdel = del * self->sr;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;

        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Allpass_process_aa(Allpass *self) {
    MYFLT val, xind, frac, sampdel, feed, del;
    int i, ind;
    
    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);    
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);    
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        if (del < 0.)
            del = 0.;
        else if (del > self->maxDelay)
            del = self->maxDelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val * (1.0 - (feed * feed)) + in[i] * -feed;
          
        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Allpass_postprocessing_ii(Allpass *self) { POST_PROCESSING_II };
static void Allpass_postprocessing_ai(Allpass *self) { POST_PROCESSING_AI };
static void Allpass_postprocessing_ia(Allpass *self) { POST_PROCESSING_IA };
static void Allpass_postprocessing_aa(Allpass *self) { POST_PROCESSING_AA };
static void Allpass_postprocessing_ireva(Allpass *self) { POST_PROCESSING_IREVA };
static void Allpass_postprocessing_areva(Allpass *self) { POST_PROCESSING_AREVA };
static void Allpass_postprocessing_revai(Allpass *self) { POST_PROCESSING_REVAI };
static void Allpass_postprocessing_revaa(Allpass *self) { POST_PROCESSING_REVAA };
static void Allpass_postprocessing_revareva(Allpass *self) { POST_PROCESSING_REVAREVA };

static void
Allpass_setProcMode(Allpass *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Allpass_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Allpass_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Allpass_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Allpass_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Allpass_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Allpass_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Allpass_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Allpass_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Allpass_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Allpass_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Allpass_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Allpass_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Allpass_postprocessing_revareva;
            break;
    } 
}

static void
Allpass_compute_next_data_frame(Allpass *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Allpass_traverse(Allpass *self, visitproc visit, void *arg)
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
Allpass_clear(Allpass *self)
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
Allpass_dealloc(Allpass* self)
{
    free(self->data);
    free(self->buffer);
    Allpass_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Allpass_deleteStream(Allpass *self) { DELETE_STREAM };

static PyObject *
Allpass_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Allpass *self;
    self = (Allpass *)type->tp_alloc(type, 0);
    
    self->delay = PyFloat_FromDouble(0);
    self->feedback = PyFloat_FromDouble(0);
    self->maxDelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Allpass_compute_next_data_frame);
    self->mode_func_ptr = Allpass_setProcMode;
    
    return (PyObject *)self;
}

static int
Allpass_init(Allpass *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "delay", "feedback", "maxDelay", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxDelay, &multmp, &addtmp))
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
    
    self->size = self->maxDelay * self->sr + 0.5;
    
    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Allpass_getServer(Allpass* self) { GET_SERVER };
static PyObject * Allpass_getStream(Allpass* self) { GET_STREAM };
static PyObject * Allpass_setMul(Allpass *self, PyObject *arg) { SET_MUL };	
static PyObject * Allpass_setAdd(Allpass *self, PyObject *arg) { SET_ADD };	
static PyObject * Allpass_setSub(Allpass *self, PyObject *arg) { SET_SUB };	
static PyObject * Allpass_setDiv(Allpass *self, PyObject *arg) { SET_DIV };	

static PyObject * Allpass_play(Allpass *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Allpass_out(Allpass *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Allpass_stop(Allpass *self) { STOP };

static PyObject * Allpass_multiply(Allpass *self, PyObject *arg) { MULTIPLY };
static PyObject * Allpass_inplace_multiply(Allpass *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Allpass_add(Allpass *self, PyObject *arg) { ADD };
static PyObject * Allpass_inplace_add(Allpass *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Allpass_sub(Allpass *self, PyObject *arg) { SUB };
static PyObject * Allpass_inplace_sub(Allpass *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Allpass_div(Allpass *self, PyObject *arg) { DIV };
static PyObject * Allpass_inplace_div(Allpass *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Allpass_setDelay(Allpass *self, PyObject *arg)
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
Allpass_setFeedback(Allpass *self, PyObject *arg)
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

static PyMemberDef Allpass_members[] = {
    {"server", T_OBJECT_EX, offsetof(Allpass, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Allpass, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Allpass, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Allpass, delay), 0, "delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Allpass, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Allpass, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Allpass, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Allpass_methods[] = {
    {"getServer", (PyCFunction)Allpass_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Allpass_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Allpass_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Allpass_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Allpass_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Allpass_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)Allpass_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Allpass_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
	{"setMul", (PyCFunction)Allpass_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Allpass_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Allpass_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Allpass_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Allpass_as_number = {
    (binaryfunc)Allpass_add,                      /*nb_add*/
    (binaryfunc)Allpass_sub,                 /*nb_subtract*/
    (binaryfunc)Allpass_multiply,                 /*nb_multiply*/
    (binaryfunc)Allpass_div,                   /*nb_divide*/
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
    (binaryfunc)Allpass_inplace_add,              /*inplace_add*/
    (binaryfunc)Allpass_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Allpass_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Allpass_inplace_div,           /*inplace_divide*/
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

PyTypeObject AllpassType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Allpass_base",         /*tp_name*/
    sizeof(Allpass),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Allpass_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Allpass_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Allpass objects. Allpass signal by x samples.",           /* tp_doc */
    (traverseproc)Allpass_traverse,   /* tp_traverse */
    (inquiry)Allpass_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Allpass_methods,             /* tp_methods */
    Allpass_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Allpass_init,      /* tp_init */
    0,                         /* tp_alloc */
    Allpass_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *bw;
    Stream *bw_stream;
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    MYFLT oneOnSr;
    // sample memories
    MYFLT y1;
    MYFLT y2;
    // coefficients
    MYFLT alpha;
    MYFLT beta;
} Allpass2;

static void
Allpass2_compute_variables(Allpass2 *self, MYFLT freq, MYFLT bw)
{    
    MYFLT radius, angle;
    if (freq <= 1) 
        freq = 1;
    else if (freq >= (self->sr/2.0))
        freq = self->sr/2.0;
    
    radius = MYPOW(E, -PI * bw * self->oneOnSr);
    angle = TWOPI * freq * self->oneOnSr;
    
    self->alpha = radius * radius;
    self->beta = -2.0 * radius * MYCOS(angle);
}

static void
Allpass2_filters_ii(Allpass2 *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_ai(Allpass2 *self) {
    MYFLT val, bw;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    bw = PyFloat_AS_DOUBLE(self->bw);
    
    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr[i], bw);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_ia(Allpass2 *self) {
    MYFLT val, fr;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *bw = Stream_getData((Stream *)self->bw_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr, bw[i]);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void
Allpass2_filters_aa(Allpass2 *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *bw = Stream_getData((Stream *)self->bw_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Allpass2_compute_variables(self, fr[i], bw[i]);
        val = in[i] + (self->y1 * -self->beta) + (self->y2 * -self->alpha);
        self->data[i] = (val * self->alpha) + (self->y1 * self->beta) + self->y2;
        self->y2 = self->y1;
        self->y1 = val;
    }
}

static void Allpass2_postprocessing_ii(Allpass2 *self) { POST_PROCESSING_II };
static void Allpass2_postprocessing_ai(Allpass2 *self) { POST_PROCESSING_AI };
static void Allpass2_postprocessing_ia(Allpass2 *self) { POST_PROCESSING_IA };
static void Allpass2_postprocessing_aa(Allpass2 *self) { POST_PROCESSING_AA };
static void Allpass2_postprocessing_ireva(Allpass2 *self) { POST_PROCESSING_IREVA };
static void Allpass2_postprocessing_areva(Allpass2 *self) { POST_PROCESSING_AREVA };
static void Allpass2_postprocessing_revai(Allpass2 *self) { POST_PROCESSING_REVAI };
static void Allpass2_postprocessing_revaa(Allpass2 *self) { POST_PROCESSING_REVAA };
static void Allpass2_postprocessing_revareva(Allpass2 *self) { POST_PROCESSING_REVAREVA };

static void
Allpass2_setProcMode(Allpass2 *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            Allpass2_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->bw));
            self->proc_func_ptr = Allpass2_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Allpass2_filters_ai;
            break;
        case 10:        
            self->proc_func_ptr = Allpass2_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Allpass2_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Allpass2_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Allpass2_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Allpass2_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Allpass2_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Allpass2_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Allpass2_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Allpass2_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Allpass2_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Allpass2_postprocessing_revareva;
            break;
    }   
}

static void
Allpass2_compute_next_data_frame(Allpass2 *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Allpass2_traverse(Allpass2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->bw);    
    Py_VISIT(self->bw_stream);    
    return 0;
}

static int 
Allpass2_clear(Allpass2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->bw);    
    Py_CLEAR(self->bw_stream);    
    return 0;
}

static void
Allpass2_dealloc(Allpass2* self)
{
    free(self->data);
    Allpass2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Allpass2_deleteStream(Allpass2 *self) { DELETE_STREAM };

static PyObject *
Allpass2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Allpass2 *self;
    self = (Allpass2 *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->bw = PyFloat_FromDouble(100);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;
    
    INIT_OBJECT_COMMON
    
    self->oneOnSr = 1.0 / self->sr;
    
    Stream_setFunctionPtr(self->stream, Allpass2_compute_next_data_frame);
    self->mode_func_ptr = Allpass2_setProcMode;
    return (PyObject *)self;
}

static int
Allpass2_init(Allpass2 *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *bwtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "bw", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &freqtmp, &bwtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (bwtmp) {
        PyObject_CallMethod((PyObject *)self, "setBw", "O", bwtmp);
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

static PyObject * Allpass2_getServer(Allpass2* self) { GET_SERVER };
static PyObject * Allpass2_getStream(Allpass2* self) { GET_STREAM };
static PyObject * Allpass2_setMul(Allpass2 *self, PyObject *arg) { SET_MUL };	
static PyObject * Allpass2_setAdd(Allpass2 *self, PyObject *arg) { SET_ADD };	
static PyObject * Allpass2_setSub(Allpass2 *self, PyObject *arg) { SET_SUB };	
static PyObject * Allpass2_setDiv(Allpass2 *self, PyObject *arg) { SET_DIV };	

static PyObject * Allpass2_play(Allpass2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Allpass2_out(Allpass2 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Allpass2_stop(Allpass2 *self) { STOP };

static PyObject * Allpass2_multiply(Allpass2 *self, PyObject *arg) { MULTIPLY };
static PyObject * Allpass2_inplace_multiply(Allpass2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Allpass2_add(Allpass2 *self, PyObject *arg) { ADD };
static PyObject * Allpass2_inplace_add(Allpass2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Allpass2_sub(Allpass2 *self, PyObject *arg) { SUB };
static PyObject * Allpass2_inplace_sub(Allpass2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Allpass2_div(Allpass2 *self, PyObject *arg) { DIV };
static PyObject * Allpass2_inplace_div(Allpass2 *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Allpass2_setFreq(Allpass2 *self, PyObject *arg)
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
Allpass2_setBw(Allpass2 *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->bw);
	if (isNumber == 1) {
		self->bw = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->bw = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bw, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bw_stream);
        self->bw_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Allpass2_members[] = {
{"server", T_OBJECT_EX, offsetof(Allpass2, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Allpass2, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Allpass2, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Allpass2, freq), 0, "Cutoff frequency in cycle per second."},
{"bw", T_OBJECT_EX, offsetof(Allpass2, bw), 0, "Bandwidth."},
{"mul", T_OBJECT_EX, offsetof(Allpass2, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Allpass2, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Allpass2_methods[] = {
{"getServer", (PyCFunction)Allpass2_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Allpass2_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Allpass2_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Allpass2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Allpass2_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Allpass2_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Allpass2_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setBw", (PyCFunction)Allpass2_setBw, METH_O, "Sets filter bandwidth."},
{"setMul", (PyCFunction)Allpass2_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Allpass2_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Allpass2_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Allpass2_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Allpass2_as_number = {
(binaryfunc)Allpass2_add,                         /*nb_add*/
(binaryfunc)Allpass2_sub,                         /*nb_subtract*/
(binaryfunc)Allpass2_multiply,                    /*nb_multiply*/
(binaryfunc)Allpass2_div,                                              /*nb_divide*/
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
(binaryfunc)Allpass2_inplace_add,                 /*inplace_add*/
(binaryfunc)Allpass2_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Allpass2_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Allpass2_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject Allpass2Type = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Allpass2_base",                                   /*tp_name*/
sizeof(Allpass2),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Allpass2_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Allpass2_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Allpass2 objects. Second order allpass filter.",           /* tp_doc */
(traverseproc)Allpass2_traverse,                  /* tp_traverse */
(inquiry)Allpass2_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Allpass2_methods,                                 /* tp_methods */
Allpass2_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Allpass2_init,                          /* tp_init */
0,                                              /* tp_alloc */
Allpass2_new,                                     /* tp_new */
};

/*******************/
/***** Phaser ******/
/*******************/

MYFLT HALF_COS_ARRAY[513] = {1.0, 0.99998110153278696, 0.99992440684545181, 0.99982991808087995, 0.99969763881045715, 0.99952757403393411, 0.99931973017923825, 0.99907411510222999, 0.99879073808640628, 0.99846960984254973, 0.99811074250832332, 0.99771414964781235, 0.99727984625101107, 0.99680784873325645, 0.99629817493460782, 0.99575084411917214, 0.99516587697437664, 0.99454329561018584, 0.99388312355826691, 0.9931853857710996, 0.99245010862103322, 0.99167731989928998, 0.99086704881491472, 0.99001932599367026, 0.98913418347688054, 0.98821165472021921, 0.9872517745924454, 0.98625457937408512, 0.98522010675606064, 0.98414839583826585, 0.98303948712808786, 0.98189342253887657, 0.98071024538836005, 0.97949000039700762, 0.97823273368633901, 0.9769384927771817, 0.97560732658787452, 0.97423928543241856, 0.97283442101857576, 0.97139278644591409, 0.96991443620380113, 0.96839942616934394, 0.96684781360527761, 0.96525965715780015, 0.96363501685435693, 0.96197395410137099, 0.96027653168192206, 0.95854281375337425, 0.95677286584495025, 0.95496675485525528, 0.95312454904974775, 0.95124631805815985, 0.94933213287186513, 0.94738206584119555, 0.94539619067270686, 0.9433745824263926, 0.94131731751284708, 0.9392244736903772, 0.93709613006206383, 0.9349323670727715, 0.93273326650610799, 0.93049891148133324, 0.92822938645021758, 0.92592477719384991, 0.92358517081939495, 0.92121065575680161, 0.91880132175545981, 0.91635725988080907, 0.91387856251089561, 0.91136532333288145, 0.90881763733950294, 0.9062356008254806, 0.90361931138387919, 0.90096886790241915, 0.89828437055973898, 0.89556592082160869, 0.89281362143709486, 0.89002757643467667, 0.88720789111831455, 0.8843546720634694, 0.88146802711307481, 0.87854806537346075, 0.87559489721022943, 0.8726086342440843, 0.86958938934661101, 0.86653727663601088, 0.86345241147278784, 0.86033491045538835, 0.85718489141579368, 0.85400247341506719, 0.8507877767388532, 0.84754092289283123, 0.8442620345981231, 0.84095123578665476, 0.8376086515964718, 0.83423440836700968, 0.83082863363431847, 0.82739145612624232, 0.82392300575755428, 0.82042341362504534, 0.81689281200256991, 0.81333133433604599, 0.80973911523841147, 0.80611629048453592, 0.80246299700608914, 0.79877937288636502, 0.7950655573550629, 0.79132169078302494, 0.78754791467693042, 0.78374437167394739, 0.77991120553634141, 0.77604856114604148, 0.77215658449916424, 0.76823542270049605, 0.76428522395793219, 0.7603061375768756, 0.75629831395459302, 0.75226190457453135, 0.74819706200059122, 0.7441039398713607, 0.73998269289430851, 0.73583347683993672, 0.73165644853589207, 0.72745176586103977, 0.72321958773949491, 0.71896007413461649, 0.71467338604296105, 0.71035968548819706, 0.70601913551498185, 0.70165190018279788, 0.69725814455975277, 0.69283803471633953, 0.68839173771916018, 0.68391942162461061, 0.6794212554725293, 0.67489740927980701, 0.67034805403396192, 0.66577336168667567, 0.66117350514729512, 0.65654865827629605, 0.65189899587871258, 0.64722469369752944, 0.6425259284070397, 0.63780287760616672, 0.63305571981175202, 0.62828463445180749, 0.62348980185873359, 0.61867140326250347, 0.61382962078381298, 0.60896463742719675, 0.60407663707411186, 0.59916580447598711, 0.59423232524724023, 0.58927638585826192, 0.58429817362836856, 0.57929787671872113, 0.57427568412521424, 0.56923178567133192, 0.56416637200097319, 0.55907963457124654, 0.55397176564523298, 0.5488429582847193, 0.5436934063429012, 0.53852330445705543, 0.53333284804118442, 0.52812223327862839, 0.52289165711465235, 0.51764131724900009, 0.51237141212842374, 0.50708214093918114, 0.50177370359950879, 0.49644630075206486, 0.49110013375634509, 0.48573540468107329, 0.48035231629656205, 0.47495107206705045, 0.46953187614301212, 0.46409493335344021, 0.45864044919810504, 0.45316862983978612, 0.44767968209648135, 0.44217381343358825, 0.43665123195606403, 0.43111214640055828, 0.42555676612752463, 0.41998530111330729, 0.41439796194220363, 0.40879495979850627, 0.40317650645851943, 0.39754281428255606, 0.3918940962069094, 0.38623056573580644, 0.38055243693333718, 0.3748599244153632, 0.36915324334140731, 0.36343260940651945, 0.35769823883312568, 0.35195034836285416, 0.34618915524834432, 0.34041487724503472, 0.33462773260293199, 0.32882794005836308, 0.32301571882570607, 0.31719128858910622, 0.31135486949417079, 0.30550668213964982, 0.29964694756909749, 0.29377588726251663, 0.28789372312798917, 0.28200067749328667, 0.27609697309746906, 0.27018283308246382, 0.26425848098463345, 0.25832414072632598, 0.25238003660741054, 0.24642639329680122, 0.24046343582396335, 0.23449138957040974, 0.22851048026118126, 0.22252093395631445, 0.21652297704229864, 0.21051683622351761, 0.20450273851368242, 0.19848091122724945, 0.19245158197082995, 0.18641497863458675, 0.1803713293836198, 0.17432086264934399, 0.16826380712085329, 0.16220039173627876, 0.15613084567413366, 0.1500553983446527, 0.14397427938112045, 0.13788771863119115, 0.13179594614820278, 0.12569919218247999, 0.11959768717263308, 0.11349166173684638, 0.10738134666416307, 0.10126697290576155, 0.095148771566225324, 0.089026973894809708, 0.082901811276699419, 0.076773515224264705, 0.070642317368309157, 0.064508449449316344, 0.058372143308689985, 0.052233630879990445, 0.046093144180169916, 0.039950915300801082, 0.033807176399306589, 0.027662159690182372, 0.021516097436222258, 0.01536922193973846, 0.0092217655337806046, 0.0030739605733557966, -0.0030739605733554522, -0.0092217655337804832, -0.015369221939738116, -0.021516097436222133, -0.027662159690182025, -0.033807176399306464, -0.039950915300800735, -0.046093144180169791, -0.052233630879990098, -0.05837214330868986, -0.064508449449316232, -0.07064231736830906, -0.076773515224264371, -0.082901811276699308, -0.089026973894809375, -0.095148771566225213, -0.10126697290576121, -0.10738134666416296, -0.11349166173684605, -0.11959768717263299, -0.12569919218247966, -0.13179594614820267, -0.13788771863119104, -0.14397427938112034, -0.15005539834465259, -0.15613084567413354, -0.16220039173627843, -0.16826380712085318, -0.17432086264934366, -0.18037132938361969, -0.18641497863458642, -0.19245158197082984, -0.19848091122724912, -0.20450273851368231, -0.21051683622351727, -0.21652297704229853, -0.22252093395631434, -0.22851048026118118, -0.23449138957040966, -0.24046343582396323, -0.24642639329680088, -0.25238003660741043, -0.25832414072632565, -0.26425848098463334, -0.27018283308246349, -0.27609697309746895, -0.28200067749328633, -0.28789372312798905, -0.2937758872625163, -0.29964694756909738, -0.30550668213964971, -0.31135486949417068, -0.31719128858910589, -0.32301571882570601, -0.32882794005836274, -0.33462773260293188, -0.34041487724503444, -0.3461891552483442, -0.35195034836285388, -0.35769823883312557, -0.36343260940651911, -0.3691532433414072, -0.37485992441536287, -0.38055243693333707, -0.38623056573580633, -0.39189409620690935, -0.39754281428255578, -0.40317650645851938, -0.408794959798506, -0.41439796194220352, -0.41998530111330723, -0.42555676612752458, -0.43111214640055795, -0.43665123195606392, -0.44217381343358819, -0.44767968209648107, -0.45316862983978584, -0.45864044919810493, -0.46409493335344015, -0.46953187614301223, -0.47495107206704995, -0.48035231629656183, -0.4857354046810729, -0.49110013375634509, -0.4964463007520647, -0.50177370359950857, -0.5070821409391808, -0.51237141212842352, -0.51764131724899998, -0.52289165711465191, -0.52812223327862795, -0.53333284804118419, -0.53852330445705532, -0.5436934063429012, -0.54884295828471885, -0.55397176564523276, -0.55907963457124621, -0.56416637200097308, -0.5692317856713317, -0.57427568412521401, -0.57929787671872079, -0.58429817362836844, -0.5892763858582617, -0.5942323252472399, -0.59916580447598666, -0.60407663707411174, -0.60896463742719653, -0.61382962078381298, -0.61867140326250303, -0.62348980185873337, -0.62828463445180716, -0.6330557198117519, -0.6378028776061665, -0.64252592840703937, -0.64722469369752911, -0.65189899587871247, -0.65654865827629583, -0.66117350514729478, -0.66577336168667522, -0.67034805403396169, -0.67489740927980679, -0.6794212554725293, -0.68391942162461028, -0.68839173771915996, -0.6928380347163392, -0.69725814455975266, -0.70165190018279777, -0.70601913551498163, -0.71035968548819683, -0.71467338604296105, -0.71896007413461638, -0.72321958773949468, -0.72745176586103955, -0.73165644853589207, -0.73583347683993661, -0.73998269289430874, -0.74410393987136036, -0.74819706200059111, -0.75226190457453113, -0.75629831395459302, -0.76030613757687548, -0.76428522395793208, -0.76823542270049594, -0.77215658449916424, -0.77604856114604126, -0.77991120553634119, -0.78374437167394717, -0.78754791467693031, -0.79132169078302472, -0.7950655573550629, -0.79877937288636469, -0.80246299700608903, -0.80611629048453581, -0.80973911523841147, -0.81333133433604599, -0.8168928120025698, -0.82042341362504512, -0.82392300575755417, -0.82739145612624221, -0.83082863363431825, -0.83423440836700946, -0.8376086515964718, -0.84095123578665465, -0.8442620345981231, -0.84754092289283089, -0.85078777673885309, -0.85400247341506696, -0.85718489141579368, -0.86033491045538824, -0.86345241147278773, -0.86653727663601066, -0.86958938934661101, -0.87260863424408419, -0.87559489721022921, -0.87854806537346053, -0.88146802711307481, -0.88435467206346929, -0.88720789111831455, -0.89002757643467667, -0.89281362143709475, -0.89556592082160857, -0.89828437055973898, -0.90096886790241903, -0.90361931138387908, -0.90623560082548038, -0.90881763733950294, -0.91136532333288134, -0.9138785625108955, -0.91635725988080885, -0.91880132175545981, -0.92121065575680139, -0.92358517081939495, -0.9259247771938498, -0.92822938645021758, -0.93049891148133312, -0.93273326650610799, -0.9349323670727715, -0.93709613006206383, -0.93922447369037709, -0.94131731751284708, -0.9433745824263926, -0.94539619067270697, -0.94738206584119544, -0.94933213287186502, -0.95124631805815973, -0.95312454904974775, -0.95496675485525517, -0.95677286584495025, -0.95854281375337413, -0.96027653168192206, -0.96197395410137099, -0.96363501685435693, -0.96525965715780004, -0.9668478136052775, -0.96839942616934394, -0.96991443620380113, -0.97139278644591398, -0.97283442101857565, -0.97423928543241844, -0.97560732658787452, -0.9769384927771817, -0.9782327336863389, -0.97949000039700751, -0.98071024538836005, -0.98189342253887657, -0.98303948712808775, -0.98414839583826574, -0.98522010675606064, -0.98625457937408501, -0.9872517745924454, -0.98821165472021921, -0.98913418347688054, -0.99001932599367015, -0.99086704881491472, -0.99167731989928998, -0.99245010862103311, -0.99318538577109949, -0.99388312355826691, -0.99454329561018584, -0.99516587697437653, -0.99575084411917214, -0.99629817493460782, -0.99680784873325645, -0.99727984625101107, -0.99771414964781235, -0.99811074250832332, -0.99846960984254973, -0.99879073808640628, -0.99907411510222999, -0.99931973017923825, -0.99952757403393411, -0.99969763881045715, -0.99982991808087995, -0.99992440684545181, -0.99998110153278685, -1.0, -1.0}; 

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *spread;
    Stream *spread_stream;
    PyObject *q;
    Stream *q_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    int stages;
    int modebuffer[6]; // need at least 2 slots for mul & add 
    MYFLT halfSr;
    MYFLT minusPiOnSr;
    MYFLT twoPiOnSr;
    MYFLT norm_arr_pos;
    MYFLT tmp;
    // sample memories
    MYFLT *y1;
    MYFLT *y2;
    // coefficients
    MYFLT *alpha;
    MYFLT *beta;
} Phaser;

static MYFLT
Phaser_clip(MYFLT x) {
    if (x < -1.0)
        return -1.0;
    else if (x > 1.0)
        return 1.0;
    else 
        return x;
}    

static void
Phaser_compute_variables(Phaser *self, MYFLT freq, MYFLT spread, MYFLT q)
{    
    int i, ipart;
    MYFLT radius, angle, fr, qfactor, pos, fpart;

    qfactor = 1.0 / q * self->minusPiOnSr;
    fr = freq;
    for (i=0; i<self->stages; i++) {
        if (fr <= 1) 
            fr = 1;
        else if (fr >= self->halfSr)
            fr = self->halfSr;
    
        radius = MYPOW(E, fr * qfactor);
        angle = fr * self->twoPiOnSr;
    
        self->alpha[i] = radius * radius;
        
        pos = angle * self->norm_arr_pos;
        ipart = (int)pos;
        fpart = pos - ipart;
        self->beta[i] = -2.0 * radius * (HALF_COS_ARRAY[i] * (1.0 - fpart) + HALF_COS_ARRAY[i+1] * fpart);
        fr *= spread;
    }    
}

static void
Phaser_filters_iii(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aii(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iai(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aai(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT q = PyFloat_AS_DOUBLE(self->q);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iia(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread, q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread, q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aia(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT spread = PyFloat_AS_DOUBLE(self->spread);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread, q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_iaa(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT freq = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq, spread[i], q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void
Phaser_filters_aaa(Phaser *self) {
    MYFLT val;
    int i, j;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *spread = Stream_getData((Stream *)self->spread_stream);
    MYFLT *q = Stream_getData((Stream *)self->q_stream);
    
    if (self->modebuffer[5] == 0) {
        MYFLT feed = Phaser_clip(PyFloat_AS_DOUBLE(self->feedback));
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q[i]);
            self->tmp = in[i] + self->tmp * feed;
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }    
    else {
        MYFLT *feed = Stream_getData((Stream *)self->feedback_stream);
        for (i=0; i<self->bufsize; i++) {
            Phaser_compute_variables(self, freq[i], spread[i], q[i]);
            self->tmp = in[i] + self->tmp * Phaser_clip(feed[i]);
            for (j=0; j<self->stages; j++) {
                val = self->tmp + (self->y1[j] * -self->beta[j]) + (self->y2[j] * -self->alpha[j]);
                self->tmp = (val * self->alpha[j]) + (self->y1[j] * self->beta[j]) + self->y2[j];
                self->y2[j] = self->y1[j];
                self->y1[j] = val;
            }
            self->data[i] = self->tmp;
        }
    }
}

static void Phaser_postprocessing_ii(Phaser *self) { POST_PROCESSING_II };
static void Phaser_postprocessing_ai(Phaser *self) { POST_PROCESSING_AI };
static void Phaser_postprocessing_ia(Phaser *self) { POST_PROCESSING_IA };
static void Phaser_postprocessing_aa(Phaser *self) { POST_PROCESSING_AA };
static void Phaser_postprocessing_ireva(Phaser *self) { POST_PROCESSING_IREVA };
static void Phaser_postprocessing_areva(Phaser *self) { POST_PROCESSING_AREVA };
static void Phaser_postprocessing_revai(Phaser *self) { POST_PROCESSING_REVAI };
static void Phaser_postprocessing_revaa(Phaser *self) { POST_PROCESSING_REVAA };
static void Phaser_postprocessing_revareva(Phaser *self) { POST_PROCESSING_REVAREVA };

static void
Phaser_setProcMode(Phaser *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            Phaser_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->spread), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Phaser_filters_iii;
            break;
        case 1:    
            self->proc_func_ptr = Phaser_filters_aii;
            break;
        case 10:        
            self->proc_func_ptr = Phaser_filters_iai;
            break;
        case 11:    
            self->proc_func_ptr = Phaser_filters_aai;
            break;
        case 100:    
            self->proc_func_ptr = Phaser_filters_iia;
            break;
        case 101:    
            self->proc_func_ptr = Phaser_filters_aia;
            break;
        case 110:        
            self->proc_func_ptr = Phaser_filters_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Phaser_filters_aaa;
            break;            
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Phaser_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Phaser_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Phaser_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Phaser_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Phaser_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Phaser_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Phaser_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Phaser_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Phaser_postprocessing_revareva;
            break;
    }   
}

static void
Phaser_compute_next_data_frame(Phaser *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Phaser_traverse(Phaser *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->spread);    
    Py_VISIT(self->spread_stream);    
    Py_VISIT(self->q);    
    Py_VISIT(self->q_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Phaser_clear(Phaser *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->spread);    
    Py_CLEAR(self->spread_stream);    
    Py_CLEAR(self->q);    
    Py_CLEAR(self->q_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Phaser_dealloc(Phaser* self)
{
    free(self->data);
    free(self->y1);
    free(self->y2);
    free(self->alpha);
    free(self->beta);
    Phaser_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Phaser_deleteStream(Phaser *self) { DELETE_STREAM };

static PyObject *
Phaser_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Phaser *self;
    self = (Phaser *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000.0);
    self->spread = PyFloat_FromDouble(1.0);
    self->q = PyFloat_FromDouble(10.0);
    self->feedback = PyFloat_FromDouble(0.0);
    self->tmp = 0.0;
    self->stages = 8;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;
    
    INIT_OBJECT_COMMON
    
    self->halfSr = self->sr / 2.0;
    self->minusPiOnSr = -PI / self->sr;
    self->twoPiOnSr = TWOPI / self->sr;
    self->norm_arr_pos = 1.0 / PI * 512.0;
    
    Stream_setFunctionPtr(self->stream, Phaser_compute_next_data_frame);
    self->mode_func_ptr = Phaser_setProcMode;
    return (PyObject *)self;
}

static int
Phaser_init(Phaser *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *spreadtmp=NULL, *qtmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "spread", "q", "feedback", "num", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOiOO", kwlist, &inputtmp, &freqtmp, &spreadtmp, &qtmp, &feedbacktmp, &self->stages, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM

    self->y1 = (MYFLT *)realloc(self->y1, self->stages * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->stages * sizeof(MYFLT));
    self->alpha = (MYFLT *)realloc(self->alpha, self->stages * sizeof(MYFLT));
    self->beta = (MYFLT *)realloc(self->beta, self->stages * sizeof(MYFLT));
    
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }
    
    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
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
    
    (*self->mode_func_ptr)(self);
    
    for (i=0; i<self->stages; i++) {
        self->y1[i] = self->y2[i] = 0.0;
    }
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Phaser_getServer(Phaser* self) { GET_SERVER };
static PyObject * Phaser_getStream(Phaser* self) { GET_STREAM };
static PyObject * Phaser_setMul(Phaser *self, PyObject *arg) { SET_MUL };	
static PyObject * Phaser_setAdd(Phaser *self, PyObject *arg) { SET_ADD };	
static PyObject * Phaser_setSub(Phaser *self, PyObject *arg) { SET_SUB };	
static PyObject * Phaser_setDiv(Phaser *self, PyObject *arg) { SET_DIV };	

static PyObject * Phaser_play(Phaser *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Phaser_out(Phaser *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Phaser_stop(Phaser *self) { STOP };

static PyObject * Phaser_multiply(Phaser *self, PyObject *arg) { MULTIPLY };
static PyObject * Phaser_inplace_multiply(Phaser *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Phaser_add(Phaser *self, PyObject *arg) { ADD };
static PyObject * Phaser_inplace_add(Phaser *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Phaser_sub(Phaser *self, PyObject *arg) { SUB };
static PyObject * Phaser_inplace_sub(Phaser *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Phaser_div(Phaser *self, PyObject *arg) { DIV };
static PyObject * Phaser_inplace_div(Phaser *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Phaser_setFreq(Phaser *self, PyObject *arg)
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
Phaser_setSpread(Phaser *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->spread);
	if (isNumber == 1) {
		self->spread = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Phaser_setQ(Phaser *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Phaser_setFeedback(Phaser *self, PyObject *arg)
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
        self->modebuffer[5] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Phaser_members[] = {
    {"server", T_OBJECT_EX, offsetof(Phaser, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Phaser, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Phaser, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Phaser, freq), 0, "Base frequency in Hertz."},
    {"spread", T_OBJECT_EX, offsetof(Phaser, spread), 0, "Frequencies spreading factor."},
    {"q", T_OBJECT_EX, offsetof(Phaser, q), 0, "Q factor."},
    {"feedback", T_OBJECT_EX, offsetof(Phaser, feedback), 0, "Feedback factor."},
    {"mul", T_OBJECT_EX, offsetof(Phaser, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Phaser, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Phaser_methods[] = {
    {"getServer", (PyCFunction)Phaser_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Phaser_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Phaser_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Phaser_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Phaser_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Phaser_stop, METH_NOARGS, "Stops computing."},
    {"setFreq", (PyCFunction)Phaser_setFreq, METH_O, "Sets base frequency in Hertz."},
    {"setSpread", (PyCFunction)Phaser_setSpread, METH_O, "Sets spreading factor."},
    {"setQ", (PyCFunction)Phaser_setQ, METH_O, "Sets filter Q factor."},
    {"setFeedback", (PyCFunction)Phaser_setFeedback, METH_O, "Sets filter Feedback factor."},
    {"setMul", (PyCFunction)Phaser_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Phaser_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Phaser_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Phaser_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Phaser_as_number = {
    (binaryfunc)Phaser_add,                         /*nb_add*/
    (binaryfunc)Phaser_sub,                         /*nb_subtract*/
    (binaryfunc)Phaser_multiply,                    /*nb_multiply*/
    (binaryfunc)Phaser_div,                                              /*nb_divide*/
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
    (binaryfunc)Phaser_inplace_add,                 /*inplace_add*/
    (binaryfunc)Phaser_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Phaser_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Phaser_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject PhaserType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Phaser_base",                                   /*tp_name*/
    sizeof(Phaser),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Phaser_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Phaser_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Phaser objects. Multi-stages second order allpass filters.",           /* tp_doc */
    (traverseproc)Phaser_traverse,                  /* tp_traverse */
    (inquiry)Phaser_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Phaser_methods,                                 /* tp_methods */
    Phaser_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Phaser_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Phaser_new,                                     /* tp_new */
};

