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
    PyObject *pan;
    Stream *pan_stream;
    PyObject *spread;
    Stream *spread_stream;
    int chnls;
    int modebuffer[2];
    float *buffer_streams;
} Panner;

static float
P_clip(float p) {
    if (p < 0.0)
        return 0.0;
    else if (p > 1.0)
        return 1.0;
    else
        return p;
}

static void
Panner_splitter_st_i(Panner *self) {
    float val, inval;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float pan = PyFloat_AS_DOUBLE(self->pan);
    pan = P_clip(pan);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        val = inval * sqrtf(1.0 - pan);
        self->buffer_streams[i] = val;
        val = inval * sqrtf(pan);
        self->buffer_streams[i+self->bufsize] = val;
    }    
}

static void
Panner_splitter_st_a(Panner *self) {
    float val, inval, panval;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *pan = Stream_getData((Stream *)self->pan_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        panval = P_clip(pan[i]);
        val = inval * sqrtf(1.0 - panval);
        self->buffer_streams[i] = val;
        val = inval * sqrtf(panval);
        self->buffer_streams[i+self->bufsize] = val;
    }    
}

static void
Panner_splitter_ii(Panner *self) {
    float val, inval, phase, sprd;
    int j, i;
    float *in = Stream_getData((Stream *)self->input_stream);

    float pan = PyFloat_AS_DOUBLE(self->pan);
    float spd = PyFloat_AS_DOUBLE(self->spread);
    
    pan = P_clip(pan);
    spd = P_clip(spd);
    
    sprd = 20.0 - (sqrtf(spd) * 20.0) + 0.1;

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        for (j=0; j<self->chnls; j++) {
            phase = j / (float)self->chnls;
            val = inval * powf(cosf((pan - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }    
}

static void
Panner_splitter_ai(Panner *self) {
    float val, inval, phase, sprd;
    int j, i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *pan = Stream_getData((Stream *)self->pan_stream);
    float spd = PyFloat_AS_DOUBLE(self->spread);

    spd = P_clip(spd);
    
    sprd = 20.0 - (sqrtf(spd) * 20.0) + 0.1;
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        for (j=0; j<self->chnls; j++) {
            phase = j / (float)self->chnls;
            val = inval * powf(cosf((P_clip(pan[i]) - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }    
}

static void
Panner_splitter_ia(Panner *self) {
    float val, inval, phase, spdval, sprd;
    int j, i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float pan = PyFloat_AS_DOUBLE(self->pan);
    float *spd = Stream_getData((Stream *)self->spread_stream);

    pan = P_clip(pan);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        spdval = P_clip(spd[i]);
        sprd = 20.0 - (sqrtf(spdval) * 20.0) + 0.1;
        for (j=0; j<self->chnls; j++) {
            phase = j / (float)self->chnls;
            val = inval * powf(cosf((pan - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }    
}

static void
Panner_splitter_aa(Panner *self) {
    float val, inval, phase, spdval, sprd;
    int j, i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *pan = Stream_getData((Stream *)self->pan_stream);
    float *spd = Stream_getData((Stream *)self->spread_stream);
    
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        spdval = P_clip(spd[i]);
        sprd = 20.0 - (sqrtf(spdval) * 20.0) + 0.1;
        for (j=0; j<self->chnls; j++) {
            phase = j / (float)self->chnls;
            val = inval * powf(cosf((P_clip(pan[i]) - phase) * TWOPI) * 0.5 + 0.5, sprd);
            self->buffer_streams[i+j*self->bufsize] = val;
        }
    }    
}

float *
Panner_getSamplesBuffer(Panner *self)
{
    return (float *)self->buffer_streams;
}    

static void
Panner_setProcMode(Panner *self)
{        
    int procmode;
    procmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    if (self->chnls > 2) {
        switch (procmode) {
            case 0:        
                self->proc_func_ptr = Panner_splitter_ii;
                break;
            case 1:    
                self->proc_func_ptr = Panner_splitter_ai;
                break;
            case 10:    
                self->proc_func_ptr = Panner_splitter_ia;
                break;
            case 11:    
                self->proc_func_ptr = Panner_splitter_aa;
                break;
        }         
    } 
    else {
        switch (self->modebuffer[0]) {
            case 0:        
                self->proc_func_ptr = Panner_splitter_st_i;
                break;
            case 1:    
                self->proc_func_ptr = Panner_splitter_st_a;
                break;
        }         
    }         
}

static void
Panner_compute_next_data_frame(Panner *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
Panner_traverse(Panner *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->pan);
    Py_VISIT(self->pan_stream);
    Py_VISIT(self->spread);
    Py_VISIT(self->spread_stream);
    return 0;
}

static int 
Panner_clear(Panner *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->pan);
    Py_CLEAR(self->pan_stream);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->spread_stream);
    return 0;
}

static void
Panner_dealloc(Panner* self)
{
    free(self->data);
    free(self->buffer_streams);
    Panner_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Panner_deleteStream(Panner *self) { DELETE_STREAM };

static PyObject *
Panner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Panner *self;
    self = (Panner *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Panner_compute_next_data_frame);
    self->mode_func_ptr = Panner_setProcMode;

    self->pan = PyFloat_FromDouble(0.5);
    self->spread = PyFloat_FromDouble(0.5);
    self->chnls = 2;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    return (PyObject *)self;
}

static int
Panner_init(Panner *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *pantmp=NULL, *spreadtmp=NULL;

    static char *kwlist[] = {"input", "outs", "pan", "spread", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->chnls, &pantmp, &spreadtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;

    if (pantmp) {
        PyObject_CallMethod((PyObject *)self, "setPan", "O", pantmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (float *)realloc(self->buffer_streams, self->chnls * self->bufsize * sizeof(float));

    (*self->mode_func_ptr)(self);

    Panner_compute_next_data_frame((Panner *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Panner_getServer(Panner* self) { GET_SERVER };
static PyObject * Panner_getStream(Panner* self) { GET_STREAM };

static PyObject * Panner_play(Panner *self) { PLAY };
static PyObject * Panner_stop(Panner *self) { STOP };

static PyObject *
Panner_setPan(Panner *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pan);
	if (isNumber == 1) {
		self->pan = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->pan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pan_stream);
        self->pan_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Panner_setSpread(Panner *self, PyObject *arg)
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
        self->modebuffer[1] = 0;
	}
	else {
		self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Panner_members[] = {
{"server", T_OBJECT_EX, offsetof(Panner, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Panner, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Panner, input), 0, "Input sound object."},
{"pan", T_OBJECT_EX, offsetof(Panner, pan), 0, "Pan object."},
{"spread", T_OBJECT_EX, offsetof(Panner, spread), 0, "Spread object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Panner_methods[] = {
{"getServer", (PyCFunction)Panner_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Panner_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Panner_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Panner_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Panner_stop, METH_NOARGS, "Stops computing."},
{"setPan", (PyCFunction)Panner_setPan, METH_O, "Sets panning value between 0 and 1."},
{"setSpread", (PyCFunction)Panner_setSpread, METH_O, "Sets spreading value between 0 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject PannerType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Panner_base",                                   /*tp_name*/
sizeof(Panner),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Panner_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
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
"Panner main objects.",           /* tp_doc */
(traverseproc)Panner_traverse,                  /* tp_traverse */
(inquiry)Panner_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Panner_methods,                                 /* tp_methods */
Panner_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Panner_init,                          /* tp_init */
0,                                              /* tp_alloc */
Panner_new,                                     /* tp_new */
};

/************************************************************************************************/
/* Pan streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    Panner *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} Pan;

static void Pan_postprocessing_ii(Pan *self) { POST_PROCESSING_II };
static void Pan_postprocessing_ai(Pan *self) { POST_PROCESSING_AI };
static void Pan_postprocessing_ia(Pan *self) { POST_PROCESSING_IA };
static void Pan_postprocessing_aa(Pan *self) { POST_PROCESSING_AA };
static void Pan_postprocessing_ireva(Pan *self) { POST_PROCESSING_IREVA };
static void Pan_postprocessing_areva(Pan *self) { POST_PROCESSING_AREVA };
static void Pan_postprocessing_revai(Pan *self) { POST_PROCESSING_REVAI };
static void Pan_postprocessing_revaa(Pan *self) { POST_PROCESSING_REVAA };
static void Pan_postprocessing_revareva(Pan *self) { POST_PROCESSING_REVAREVA };

static void
Pan_setProcMode(Pan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Pan_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Pan_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Pan_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Pan_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Pan_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Pan_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Pan_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Pan_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Pan_postprocessing_revareva;
            break;
    }
}

static void
Pan_compute_next_data_frame(Pan *self)
{
    int i;
    float *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = Panner_getSamplesBuffer((Panner *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Pan_traverse(Pan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
Pan_clear(Pan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
Pan_dealloc(Pan* self)
{
    free(self->data);
    Pan_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pan_deleteStream(Pan *self) { DELETE_STREAM };

static PyObject *
Pan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Pan *self;
    self = (Pan *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pan_compute_next_data_frame);
    self->mode_func_ptr = Pan_setProcMode;
    
    return (PyObject *)self;
}

static int
Pan_init(Pan *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (Panner *)maintmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Pan_compute_next_data_frame((Pan *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Pan_getServer(Pan* self) { GET_SERVER };
static PyObject * Pan_getStream(Pan* self) { GET_STREAM };
static PyObject * Pan_setMul(Pan *self, PyObject *arg) { SET_MUL };	
static PyObject * Pan_setAdd(Pan *self, PyObject *arg) { SET_ADD };	
static PyObject * Pan_setSub(Pan *self, PyObject *arg) { SET_SUB };	
static PyObject * Pan_setDiv(Pan *self, PyObject *arg) { SET_DIV };	

static PyObject * Pan_play(Pan *self) { PLAY };
static PyObject * Pan_out(Pan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pan_stop(Pan *self) { STOP };

static PyObject * Pan_multiply(Pan *self, PyObject *arg) { MULTIPLY };
static PyObject * Pan_inplace_multiply(Pan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pan_add(Pan *self, PyObject *arg) { ADD };
static PyObject * Pan_inplace_add(Pan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pan_sub(Pan *self, PyObject *arg) { SUB };
static PyObject * Pan_inplace_sub(Pan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pan_div(Pan *self, PyObject *arg) { DIV };
static PyObject * Pan_inplace_div(Pan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Pan_members[] = {
{"server", T_OBJECT_EX, offsetof(Pan, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pan, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Pan, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pan, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pan_methods[] = {
{"getServer", (PyCFunction)Pan_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pan_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pan_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pan_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pan_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pan_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Pan_setMul, METH_O, "Sets Pan mul factor."},
{"setAdd", (PyCFunction)Pan_setAdd, METH_O, "Sets Pan add factor."},
{"setSub", (PyCFunction)Pan_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Pan_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pan_as_number = {
(binaryfunc)Pan_add,                      /*nb_add*/
(binaryfunc)Pan_sub,                 /*nb_subtract*/
(binaryfunc)Pan_multiply,                 /*nb_multiply*/
(binaryfunc)Pan_div,                   /*nb_divide*/
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
(binaryfunc)Pan_inplace_add,              /*inplace_add*/
(binaryfunc)Pan_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pan_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Pan_inplace_div,           /*inplace_divide*/
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

PyTypeObject PanType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pan_base",         /*tp_name*/
sizeof(Pan),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pan_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Pan_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Pan objects. Reads one band from a Panner.",           /* tp_doc */
(traverseproc)Pan_traverse,   /* tp_traverse */
(inquiry)Pan_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pan_methods,             /* tp_methods */
Pan_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pan_init,      /* tp_init */
0,                         /* tp_alloc */
Pan_new,                 /* tp_new */
};

/*********************/
/*** Simple panner ***/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *pan;
    Stream *pan_stream;
    int chnls;
    int k1;
    int k2;
    int modebuffer[1];
    float *buffer_streams;
} SPanner;

static void
SPanner_splitter_st_i(SPanner *self) {
    float val, inval;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float pan = PyFloat_AS_DOUBLE(self->pan);
    pan = P_clip(pan);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        val = inval * sqrtf(1.0 - pan);
        self->buffer_streams[i] = val;
        val = inval * sqrtf(pan);
        self->buffer_streams[i+self->bufsize] = val;
    }    
}

static void
SPanner_splitter_st_a(SPanner *self) {
    float val, inval, panval;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *pan = Stream_getData((Stream *)self->pan_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        panval = P_clip(pan[i]);
        val = inval * sqrtf(1.0 - panval);
        self->buffer_streams[i] = val;
        val = inval * sqrtf(panval);
        self->buffer_streams[i+self->bufsize] = val;
    }    
}

static void
SPanner_splitter_i(SPanner *self) {
    float val, inval, min, pan1, pan2;
    int j, i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float pan = PyFloat_AS_DOUBLE(self->pan);
    float fchnls = (float)self->chnls;

    for (i=0; i<self->bufsize; i++) {
        self->buffer_streams[i+self->k1] = 0.0;
        self->buffer_streams[i+self->k2] = 0.0;
    }
    
    min = 0;
    self->k1 = 0;
    self->k2 = self->bufsize;

    for (j=self->chnls; j>0; j--) {
        int j1 = j - 1;
        min = j1 / fchnls;
        if (pan > min) {
            self->k1 = j1 * self->bufsize;
            if (j == self->chnls)
                self->k2 = 0;
            else
                self->k2 = j * self->bufsize;
            break;
        }
    }    

    pan = P_clip((pan - min) * self->chnls);
    pan1 = sqrtf(1.0 - pan);
    pan2 = sqrtf(pan);
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        val = inval * pan1;
        self->buffer_streams[i+self->k1] = val;
        val = inval * pan2;
        self->buffer_streams[i+self->k2] = val;
    }    
}

static void
SPanner_splitter_a(SPanner *self) {
    float val, inval, min, pan;
    int i, j, j1, len;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *apan = Stream_getData((Stream *)self->pan_stream);
    float fchnls = (float)self->chnls;
    
    len = self->chnls * self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }
    
    for (i=0; i<self->bufsize; i++) {
        pan = apan[i];
        inval = in[i];
        min = 0;
        self->k1 = 0;
        self->k2 = self->bufsize;
        
        for (j=self->chnls; j>0; j--) {
            j1 = j - 1;
            min = j1 / fchnls;
            if (pan > min) {
                self->k1 = j1 * self->bufsize;
                if (j == self->chnls)
                    self->k2 = 0;
                else                    
                    self->k2 = j * self->bufsize;
                break;
            }
        }    
        
        pan = P_clip((pan - min) * self->chnls);
        val = inval * sqrtf(1.0 - pan);
        self->buffer_streams[i+self->k1] = val;
        val = inval * sqrtf(pan);
        self->buffer_streams[i+self->k2] = val;
    }    
}

float *
SPanner_getSamplesBuffer(SPanner *self)
{
    return (float *)self->buffer_streams;
}    

static void
SPanner_setProcMode(SPanner *self)
{        
    int procmode;
    procmode = self->modebuffer[0];
    
    if (self->chnls > 2) {
        switch (procmode) {
            case 0:        
                self->proc_func_ptr = SPanner_splitter_i;
                break;
            case 1:    
                self->proc_func_ptr = SPanner_splitter_a;
                break;
        }         
    } 
    else {
        switch (self->modebuffer[0]) {
            case 0:        
                self->proc_func_ptr = SPanner_splitter_st_i;
                break;
            case 1:    
                self->proc_func_ptr = SPanner_splitter_st_a;
                break;
        }         
    }         
}

static void
SPanner_compute_next_data_frame(SPanner *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
SPanner_traverse(SPanner *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->pan);
    Py_VISIT(self->pan_stream);
    return 0;
}

static int 
SPanner_clear(SPanner *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->pan);
    Py_CLEAR(self->pan_stream);
    return 0;
}

static void
SPanner_dealloc(SPanner* self)
{
    free(self->data);
    free(self->buffer_streams);
    SPanner_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SPanner_deleteStream(SPanner *self) { DELETE_STREAM };

static PyObject *
SPanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SPanner *self;
    self = (SPanner *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SPanner_compute_next_data_frame);
    self->mode_func_ptr = SPanner_setProcMode;
    
    self->pan = PyFloat_FromDouble(0.5);
    self->chnls = 2;
    self->k1 = 0;
    self->k2 = self->bufsize;
    self->modebuffer[0] = 0;
    
    return (PyObject *)self;
}

static int
SPanner_init(SPanner *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *pantmp=NULL;
    
    static char *kwlist[] = {"input", "outs", "pan", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iO", kwlist, &inputtmp, &self->chnls, &pantmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (pantmp) {
        PyObject_CallMethod((PyObject *)self, "setPan", "O", pantmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->buffer_streams = (float *)realloc(self->buffer_streams, self->chnls * self->bufsize * sizeof(float));
    
    (*self->mode_func_ptr)(self);

    int len = self->chnls*self->bufsize;
    for (i=0; i<len; i++) {
        self->buffer_streams[i] = 0.0;
    }
    
    SPanner_compute_next_data_frame((SPanner *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * SPanner_getServer(SPanner* self) { GET_SERVER };
static PyObject * SPanner_getStream(SPanner* self) { GET_STREAM };

static PyObject * SPanner_play(SPanner *self) { PLAY };
static PyObject * SPanner_stop(SPanner *self) { STOP };

static PyObject *
SPanner_setPan(SPanner *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pan);
	if (isNumber == 1) {
		self->pan = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->pan = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pan, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pan_stream);
        self->pan_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef SPanner_members[] = {
{"server", T_OBJECT_EX, offsetof(SPanner, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SPanner, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(SPanner, input), 0, "Input sound object."},
{"pan", T_OBJECT_EX, offsetof(SPanner, pan), 0, "Pan object."},
{NULL}  /* Sentinel */
};

static PyMethodDef SPanner_methods[] = {
{"getServer", (PyCFunction)SPanner_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SPanner_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SPanner_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SPanner_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)SPanner_stop, METH_NOARGS, "Stops computing."},
{"setPan", (PyCFunction)SPanner_setPan, METH_O, "Sets panning value between 0 and 1."},
{NULL}  /* Sentinel */
};

PyTypeObject SPannerType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.SPanner_base",                                   /*tp_name*/
sizeof(SPanner),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)SPanner_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
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
"SPanner main objects. Simple equal power panner",           /* tp_doc */
(traverseproc)SPanner_traverse,                  /* tp_traverse */
(inquiry)SPanner_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
SPanner_methods,                                 /* tp_methods */
SPanner_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)SPanner_init,                          /* tp_init */
0,                                              /* tp_alloc */
SPanner_new,                                     /* tp_new */
};

/************************************************************************************************/
/* SSPan streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    SPanner *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} SPan;

static void SPan_postprocessing_ii(SPan *self) { POST_PROCESSING_II };
static void SPan_postprocessing_ai(SPan *self) { POST_PROCESSING_AI };
static void SPan_postprocessing_ia(SPan *self) { POST_PROCESSING_IA };
static void SPan_postprocessing_aa(SPan *self) { POST_PROCESSING_AA };
static void SPan_postprocessing_ireva(SPan *self) { POST_PROCESSING_IREVA };
static void SPan_postprocessing_areva(SPan *self) { POST_PROCESSING_AREVA };
static void SPan_postprocessing_revai(SPan *self) { POST_PROCESSING_REVAI };
static void SPan_postprocessing_revaa(SPan *self) { POST_PROCESSING_REVAA };
static void SPan_postprocessing_revareva(SPan *self) { POST_PROCESSING_REVAREVA };

static void
SPan_setProcMode(SPan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SPan_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SPan_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SPan_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SPan_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SPan_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SPan_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SPan_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SPan_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SPan_postprocessing_revareva;
            break;
    }
}

static void
SPan_compute_next_data_frame(SPan *self)
{
    int i;
    float *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SPanner_getSamplesBuffer((SPanner *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SPan_traverse(SPan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
SPan_clear(SPan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
SPan_dealloc(SPan* self)
{
    free(self->data);
    SPan_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SPan_deleteStream(SPan *self) { DELETE_STREAM };

static PyObject *
SPan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SPan *self;
    self = (SPan *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SPan_compute_next_data_frame);
    self->mode_func_ptr = SPan_setProcMode;
    
    return (PyObject *)self;
}

static int
SPan_init(SPan *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (SPanner *)maintmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    SPan_compute_next_data_frame((SPan *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * SPan_getServer(SPan* self) { GET_SERVER };
static PyObject * SPan_getStream(SPan* self) { GET_STREAM };
static PyObject * SPan_setMul(SPan *self, PyObject *arg) { SET_MUL };	
static PyObject * SPan_setAdd(SPan *self, PyObject *arg) { SET_ADD };	
static PyObject * SPan_setSub(SPan *self, PyObject *arg) { SET_SUB };	
static PyObject * SPan_setDiv(SPan *self, PyObject *arg) { SET_DIV };	

static PyObject * SPan_play(SPan *self) { PLAY };
static PyObject * SPan_out(SPan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SPan_stop(SPan *self) { STOP };

static PyObject * SPan_multiply(SPan *self, PyObject *arg) { MULTIPLY };
static PyObject * SPan_inplace_multiply(SPan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SPan_add(SPan *self, PyObject *arg) { ADD };
static PyObject * SPan_inplace_add(SPan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SPan_sub(SPan *self, PyObject *arg) { SUB };
static PyObject * SPan_inplace_sub(SPan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SPan_div(SPan *self, PyObject *arg) { DIV };
static PyObject * SPan_inplace_div(SPan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SPan_members[] = {
{"server", T_OBJECT_EX, offsetof(SPan, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SPan, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(SPan, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SPan, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SPan_methods[] = {
{"getServer", (PyCFunction)SPan_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SPan_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SPan_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SPan_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SPan_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SPan_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)SPan_setMul, METH_O, "Sets SPan mul factor."},
{"setAdd", (PyCFunction)SPan_setAdd, METH_O, "Sets SPan add factor."},
{"setSub", (PyCFunction)SPan_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SPan_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SPan_as_number = {
(binaryfunc)SPan_add,                      /*nb_add*/
(binaryfunc)SPan_sub,                 /*nb_subtract*/
(binaryfunc)SPan_multiply,                 /*nb_multiply*/
(binaryfunc)SPan_div,                   /*nb_divide*/
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
(binaryfunc)SPan_inplace_add,              /*inplace_add*/
(binaryfunc)SPan_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SPan_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)SPan_inplace_div,           /*inplace_divide*/
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

PyTypeObject SPanType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SPan_base",         /*tp_name*/
sizeof(SPan),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SPan_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&SPan_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SPan objects. Reads one band from a SPanner.",           /* tp_doc */
(traverseproc)SPan_traverse,   /* tp_traverse */
(inquiry)SPan_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SPan_methods,             /* tp_methods */
SPan_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SPan_init,      /* tp_init */
0,                         /* tp_alloc */
SPan_new,                 /* tp_new */
};
