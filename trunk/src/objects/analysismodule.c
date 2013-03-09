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
#include "interpolation.h"

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
    MYFLT follow;
    MYFLT last_freq;
    MYFLT factor;
} Follower;

static void
Follower_filters_i(Follower *self) {
    MYFLT absin, freq;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    freq = PyFloat_AS_DOUBLE(self->freq);

    if (freq != self->last_freq) {
        self->factor = MYEXP(-1.0 / (self->sr / freq));
        self->last_freq = freq;
    }
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow = self->data[i] = absin + self->factor * (self->follow - absin);
    }
}

static void
Follower_filters_a(Follower *self) {
    MYFLT freq, absin;
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->last_freq) {
            self->factor = MYEXP(-1.0 / (self->sr / freq));
            self->last_freq = freq;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        self->follow = self->data[i] = absin + self->factor * (self->follow - absin);        
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
    pyo_DEALLOC
    Follower_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Follower_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    Follower *self;
    self = (Follower *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(20);
    self->follow = 0.0;
    self->last_freq = -1.0;
    self->factor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower_compute_next_data_frame);
    self->mode_func_ptr = Follower_setProcMode;

    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        Py_RETURN_NONE; 
    
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
    
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
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
0,                          /* tp_init */
0,                                              /* tp_alloc */
Follower_new,                                     /* tp_new */
};

/************/
/* Follower2 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    Stream *risetime_stream;
    PyObject *falltime;
    Stream *falltime_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    MYFLT follow;
    MYFLT last_risetime;
    MYFLT last_falltime;
    MYFLT risefactor;
    MYFLT fallfactor;
} Follower2;

static void
Follower2_filters_ii(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = 0.001;
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = 0.001;
    
    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_ai(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    falltime = PyFloat_AS_DOUBLE(self->falltime);
    if (falltime <= 0.0)
        falltime = 0.001;

    if (falltime != self->last_falltime) {
        self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
        self->last_falltime = falltime;
    }
    
    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = 0.001;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_ia(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    risetime = PyFloat_AS_DOUBLE(self->risetime);
    if (risetime <= 0.0)
        risetime = 0.001;
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);
    
    if (risetime != self->last_risetime) {
        self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
        self->last_risetime = risetime;
    }

    for (i=0; i<self->bufsize; i++) {
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = 0.001;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }        
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void
Follower2_filters_aa(Follower2 *self) {
    MYFLT absin, risetime, falltime;
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *rise = Stream_getData((Stream *)self->risetime_stream);
    MYFLT *fall = Stream_getData((Stream *)self->falltime_stream);

    for (i=0; i<self->bufsize; i++) {
        risetime = rise[i];
        if (risetime <= 0.0)
            risetime = 0.001;
        if (risetime != self->last_risetime) {
            self->risefactor = MYEXP(-1.0 / (self->sr * risetime));
            self->last_risetime = risetime;
        }
        falltime = fall[i];
        if (falltime <= 0.0)
            falltime = 0.001;
        if (falltime != self->last_falltime) {
            self->fallfactor = MYEXP(-1.0 / (self->sr * falltime));
            self->last_falltime = falltime;
        }        
        absin = in[i];
        if (absin < 0.0)
            absin = -absin;
        if (self->follow < absin)
            self->follow = self->data[i] = absin + self->risefactor * (self->follow - absin);
        else
            self->follow = self->data[i] = absin + self->fallfactor * (self->follow - absin);
    }
}

static void Follower2_postprocessing_ii(Follower2 *self) { POST_PROCESSING_II };
static void Follower2_postprocessing_ai(Follower2 *self) { POST_PROCESSING_AI };
static void Follower2_postprocessing_ia(Follower2 *self) { POST_PROCESSING_IA };
static void Follower2_postprocessing_aa(Follower2 *self) { POST_PROCESSING_AA };
static void Follower2_postprocessing_ireva(Follower2 *self) { POST_PROCESSING_IREVA };
static void Follower2_postprocessing_areva(Follower2 *self) { POST_PROCESSING_AREVA };
static void Follower2_postprocessing_revai(Follower2 *self) { POST_PROCESSING_REVAI };
static void Follower2_postprocessing_revaa(Follower2 *self) { POST_PROCESSING_REVAA };
static void Follower2_postprocessing_revareva(Follower2 *self) { POST_PROCESSING_REVAREVA };

static void
Follower2_setProcMode(Follower2 *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Follower2_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Follower2_filters_ai;
            break;
        case 10:    
            self->proc_func_ptr = Follower2_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Follower2_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Follower2_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Follower2_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Follower2_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Follower2_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Follower2_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Follower2_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Follower2_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Follower2_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Follower2_postprocessing_revareva;
            break;
    }   
}

static void
Follower2_compute_next_data_frame(Follower2 *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Follower2_traverse(Follower2 *self, visitproc visit, void *arg)
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
Follower2_clear(Follower2 *self)
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
Follower2_dealloc(Follower2* self)
{
    pyo_DEALLOC
    Follower2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Follower2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    Follower2 *self;
    self = (Follower2 *)type->tp_alloc(type, 0);
    
    self->risetime = PyFloat_FromDouble(0.01);
    self->falltime = PyFloat_FromDouble(0.1);
    self->follow = 0.0;
    self->last_risetime = -1.0;
    self->last_falltime = -1.0;
    self->risefactor = self->fallfactor = 0.99;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower2_compute_next_data_frame);
    self->mode_func_ptr = Follower2_setProcMode;

    static char *kwlist[] = {"input", "risetime", "falltime", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &risetimetmp, &falltimetmp, &multmp, &addtmp))
        Py_RETURN_NONE; 
    
    INIT_INPUT_STREAM
    
    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRisetime", "O", risetimetmp);
    }
    
    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFalltime", "O", falltimetmp);
    }
    
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

static PyObject * Follower2_getServer(Follower2* self) { GET_SERVER };
static PyObject * Follower2_getStream(Follower2* self) { GET_STREAM };
static PyObject * Follower2_setMul(Follower2 *self, PyObject *arg) { SET_MUL };	
static PyObject * Follower2_setAdd(Follower2 *self, PyObject *arg) { SET_ADD };	
static PyObject * Follower2_setSub(Follower2 *self, PyObject *arg) { SET_SUB };	
static PyObject * Follower2_setDiv(Follower2 *self, PyObject *arg) { SET_DIV };	

static PyObject * Follower2_play(Follower2 *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Follower2_stop(Follower2 *self) { STOP };

static PyObject * Follower2_multiply(Follower2 *self, PyObject *arg) { MULTIPLY };
static PyObject * Follower2_inplace_multiply(Follower2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Follower2_add(Follower2 *self, PyObject *arg) { ADD };
static PyObject * Follower2_inplace_add(Follower2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Follower2_sub(Follower2 *self, PyObject *arg) { SUB };
static PyObject * Follower2_inplace_sub(Follower2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Follower2_div(Follower2 *self, PyObject *arg) { DIV };
static PyObject * Follower2_inplace_div(Follower2 *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Follower2_setRisetime(Follower2 *self, PyObject *arg)
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
Follower2_setFalltime(Follower2 *self, PyObject *arg)
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

static PyMemberDef Follower2_members[] = {
    {"server", T_OBJECT_EX, offsetof(Follower2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Follower2, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Follower2, input), 0, "Input sound object."},
    {"risetime", T_OBJECT_EX, offsetof(Follower2, risetime), 0, "Risetime in second."},
    {"falltime", T_OBJECT_EX, offsetof(Follower2, falltime), 0, "Falltime in second."},
    {"mul", T_OBJECT_EX, offsetof(Follower2, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Follower2, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Follower2_methods[] = {
    {"getServer", (PyCFunction)Follower2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Follower2_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Follower2_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Follower2_stop, METH_NOARGS, "Stops computing."},
    {"setRisetime", (PyCFunction)Follower2_setRisetime, METH_O, "Sets filter risetime in second."},
    {"setFalltime", (PyCFunction)Follower2_setFalltime, METH_O, "Sets filter falltime in second."},
    {"setMul", (PyCFunction)Follower2_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Follower2_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Follower2_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Follower2_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Follower2_as_number = {
    (binaryfunc)Follower2_add,                         /*nb_add*/
    (binaryfunc)Follower2_sub,                         /*nb_subtract*/
    (binaryfunc)Follower2_multiply,                    /*nb_multiply*/
    (binaryfunc)Follower2_div,                                              /*nb_divide*/
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
    (binaryfunc)Follower2_inplace_add,                 /*inplace_add*/
    (binaryfunc)Follower2_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Follower2_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Follower2_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject Follower2Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Follower2_base",                                   /*tp_name*/
    sizeof(Follower2),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Follower2_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Follower2_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Follower2 objects. Envelope follower.",           /* tp_doc */
    (traverseproc)Follower2_traverse,                  /* tp_traverse */
    (inquiry)Follower2_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Follower2_methods,                                 /* tp_methods */
    Follower2_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    Follower2_new,                                     /* tp_new */
};

/************/
/* ZCross */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT thresh;
    MYFLT lastValue;
    MYFLT lastSample;
    int modebuffer[2]; // need at least 2 slots for mul & add
} ZCross;

static void
ZCross_process(ZCross *self) {
    int i;
    int count = 0;
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
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
    self->lastValue = (MYFLT)count / self->bufsize;
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
    pyo_DEALLOC
    ZCross_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
ZCross_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    ZCross *self;
    self = (ZCross *)type->tp_alloc(type, 0);
    
    self->thresh = 0.0;
    self->lastValue = self->lastSample = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ZCross_compute_next_data_frame);
    self->mode_func_ptr = ZCross_setProcMode;

    static char *kwlist[] = {"input", "thresh", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FOO, kwlist, &inputtmp, &self->thresh, &multmp, &addtmp))
        Py_RETURN_NONE; 

    INIT_INPUT_STREAM
    
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
0,                          /* tp_init */
0,                                              /* tp_alloc */
ZCross_new,                                     /* tp_new */
};

static int
min_elem_pos(MYFLT *buf, int size ) {
    int i;
    int pos = 0;
    for (i=1; i<size; i++) {
        if (buf[i] < buf[pos])
            pos = i;
    }
    return pos;
}

static MYFLT
quadraticInterpolation(MYFLT *buf, int period, int size) {
    int x0, x2;
    MYFLT pitch, s0, s1, s2;
    x0 = period < 1 ? period : period - 1;
    x2 = period + 1 < size ? period + 1 : period;
    if (x0 == period)
        pitch = buf[period] <= buf[x2] ? period : x2;
    else if (x2 == period)
        pitch = buf[period] <= buf[x0] ? period : x0;
    else {
        s0 = buf[x0];
        s1 = buf[period];
        s2 = buf[x2];
        pitch = period + 0.5 * (s2 - s0) / (s2 - 2.0 * s1 + s0);
    }
    return pitch;
}

/************/
/* Yin */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT *input_buffer;
    MYFLT *yin_buffer;
    int winsize;
    int halfsize;
    int input_count;
    MYFLT tolerance;
    MYFLT pitch;
    MYFLT minfreq;
    MYFLT maxfreq;
    MYFLT cutoff;
    MYFLT last_cutoff;
    MYFLT y1;
    MYFLT c2;
    int modebuffer[2]; // need at least 2 slots for mul & add
} Yin;

static void
Yin_process(Yin *self) {
    int i, j, period, tau = 0;
    MYFLT candidate, tmp = 0.0, tmp2 = 0.0, b = 0.0;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->cutoff != self->last_cutoff) {
        if (self->cutoff <= 1.0)
            self->cutoff = 1.0;
        else if (self->cutoff >= self->sr*0.5)
            self->cutoff = self->sr*0.5;        
        self->last_cutoff = self->cutoff;
        b = 2.0 - MYCOS(TWOPI * self->cutoff / self->sr);
        self->c2 = (b - MYSQRT(b * b - 1.0));
    }

    for (i=0; i<self->bufsize; i++) {
        self->y1 = in[i] + (self->y1 - in[i]) * self->c2;
        self->input_buffer[self->input_count] = self->y1;
        if (self->input_count++ == self->winsize) {
            self->input_count = 0;
 
            self->yin_buffer[0] = 1.0;
            for (tau = 1; tau < self->halfsize; tau++) {
                self->yin_buffer[tau] = 0.0;
                for (j = 0; j < self->halfsize; j++) {
                    tmp = self->input_buffer[j] - self->input_buffer[j+tau];
                    self->yin_buffer[tau] += tmp*tmp;
                }
                tmp2 += self->yin_buffer[tau];
                self->yin_buffer[tau] *= tau / tmp2;
                period = tau - 3;
                if (tau > 4 && (self->yin_buffer[period] < self->tolerance) &&
                    (self->yin_buffer[period] < self->yin_buffer[period+1])) {
                    candidate = quadraticInterpolation(self->yin_buffer, period, self->halfsize);
                    goto founded;
                }
            }
            candidate = quadraticInterpolation(self->yin_buffer, min_elem_pos(self->yin_buffer, self->halfsize), self->halfsize);

        founded:
            
            candidate = self->sr / candidate;
            if (candidate > self->minfreq && candidate < self->maxfreq)
                self->pitch = candidate;           

        }
        self->data[i] = self->pitch;
    }
}

static void Yin_postprocessing_ii(Yin *self) { POST_PROCESSING_II };
static void Yin_postprocessing_ai(Yin *self) { POST_PROCESSING_AI };
static void Yin_postprocessing_ia(Yin *self) { POST_PROCESSING_IA };
static void Yin_postprocessing_aa(Yin *self) { POST_PROCESSING_AA };
static void Yin_postprocessing_ireva(Yin *self) { POST_PROCESSING_IREVA };
static void Yin_postprocessing_areva(Yin *self) { POST_PROCESSING_AREVA };
static void Yin_postprocessing_revai(Yin *self) { POST_PROCESSING_REVAI };
static void Yin_postprocessing_revaa(Yin *self) { POST_PROCESSING_REVAA };
static void Yin_postprocessing_revareva(Yin *self) { POST_PROCESSING_REVAREVA };

static void
Yin_setProcMode(Yin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Yin_process;
 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Yin_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Yin_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Yin_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Yin_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Yin_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Yin_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Yin_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Yin_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Yin_postprocessing_revareva;
            break;
    }   
}

static void
Yin_compute_next_data_frame(Yin *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Yin_traverse(Yin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Yin_clear(Yin *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Yin_dealloc(Yin* self)
{
    pyo_DEALLOC
    free(self->input_buffer);
    free(self->yin_buffer);
    Yin_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Yin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    Yin *self;
    self = (Yin *)type->tp_alloc(type, 0);
    
    self->winsize = 1024;
    self->halfsize = 512;
    self->input_count = 0;
    self->pitch = 0.;
    self->tolerance = 0.15;
    self->minfreq = 40;
    self->maxfreq = 1000;
    self->cutoff = 1000;
    self->last_cutoff = -1.0;
    self->y1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Yin_compute_next_data_frame);
    self->mode_func_ptr = Yin_setProcMode;

    static char *kwlist[] = {"input", "tolerance", "minfreq", "maxfreq", "cutoff", "winsize", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFIOO, kwlist, &inputtmp, &self->tolerance, &self->minfreq, &self->maxfreq, &self->cutoff, &self->winsize, &multmp, &addtmp))
        Py_RETURN_NONE; 

    INIT_INPUT_STREAM
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (self->winsize % 2 == 1)
        self->winsize += 1;

    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->winsize * sizeof(MYFLT)); 
    for (i=0; i<self->winsize; i++)
        self->input_buffer[i] = 0.0;

    self->halfsize = self->winsize / 2;
    self->yin_buffer = (MYFLT *)realloc(self->yin_buffer, self->halfsize * sizeof(MYFLT)); 
    for (i=0; i<self->halfsize; i++)
        self->yin_buffer[i] = 0.0;
       
    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * Yin_getServer(Yin* self) { GET_SERVER };
static PyObject * Yin_getStream(Yin* self) { GET_STREAM };
static PyObject * Yin_setMul(Yin *self, PyObject *arg) { SET_MUL };	
static PyObject * Yin_setAdd(Yin *self, PyObject *arg) { SET_ADD };	
static PyObject * Yin_setSub(Yin *self, PyObject *arg) { SET_SUB };	
static PyObject * Yin_setDiv(Yin *self, PyObject *arg) { SET_DIV };	

static PyObject * Yin_play(Yin *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Yin_stop(Yin *self) { STOP };

static PyObject * Yin_multiply(Yin *self, PyObject *arg) { MULTIPLY };
static PyObject * Yin_inplace_multiply(Yin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Yin_add(Yin *self, PyObject *arg) { ADD };
static PyObject * Yin_inplace_add(Yin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Yin_sub(Yin *self, PyObject *arg) { SUB };
static PyObject * Yin_inplace_sub(Yin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Yin_div(Yin *self, PyObject *arg) { DIV };
static PyObject * Yin_inplace_div(Yin *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Yin_setTolerance(Yin *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->tolerance = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
  
	Py_RETURN_NONE;
}

static PyObject *
Yin_setMinfreq(Yin *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->minfreq = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
  
	Py_RETURN_NONE;
}

static PyObject *
Yin_setMaxfreq(Yin *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->maxfreq = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
  
	Py_RETURN_NONE;
}

static PyObject *
Yin_setCutoff(Yin *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->cutoff = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
  
	Py_RETURN_NONE;
}

static PyMemberDef Yin_members[] = {
{"server", T_OBJECT_EX, offsetof(Yin, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Yin, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Yin, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Yin, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Yin, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Yin_methods[] = {
{"getServer", (PyCFunction)Yin_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Yin_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Yin_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Yin_stop, METH_NOARGS, "Stops computing."},
{"setTolerance", (PyCFunction)Yin_setTolerance, METH_O, "Sets the tolerance factor."},
{"setMinfreq", (PyCFunction)Yin_setMinfreq, METH_O, "Sets the minimum frequency in output."},
{"setMaxfreq", (PyCFunction)Yin_setMaxfreq, METH_O, "Sets the maximum frequency in output."},
{"setCutoff", (PyCFunction)Yin_setCutoff, METH_O, "Sets the input lowpass filter cutoff frequency."},
{"setMul", (PyCFunction)Yin_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Yin_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Yin_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Yin_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Yin_as_number = {
(binaryfunc)Yin_add,                         /*nb_add*/
(binaryfunc)Yin_sub,                         /*nb_subtract*/
(binaryfunc)Yin_multiply,                    /*nb_multiply*/
(binaryfunc)Yin_div,                                              /*nb_divide*/
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
(binaryfunc)Yin_inplace_add,                 /*inplace_add*/
(binaryfunc)Yin_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Yin_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Yin_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject YinType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Yin_base",                                   /*tp_name*/
sizeof(Yin),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Yin_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Yin_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Yin objects. Pitch tracker using the Yin algorithm.",           /* tp_doc */
(traverseproc)Yin_traverse,                  /* tp_traverse */
(inquiry)Yin_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Yin_methods,                                 /* tp_methods */
Yin_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Yin_new,                                     /* tp_new */
};