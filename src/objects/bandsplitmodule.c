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
    PyObject *q;
    Stream *q_stream;
    int bands;
    MYFLT min_freq;
    MYFLT max_freq;
    int init;
    MYFLT halfSr;
    MYFLT TwoPiOnSr;
    MYFLT *band_freqs;
    // sample memories
    MYFLT *x1;
    MYFLT *x2;
    MYFLT *y1;
    MYFLT *y2;
    // coefficients
    MYFLT *b0;
    MYFLT *b1;
    MYFLT *b2;
    MYFLT *a0;
    MYFLT *a1;
    MYFLT *a2;
    MYFLT *buffer_streams;
    int modebuffer[1];
} BandSplitter;


static void
BandSplitter_compute_variables(BandSplitter *self, MYFLT q)
{    
    int i;
    MYFLT freq;
    for (i=0; i<self->bands; i++) {
        freq = self->band_freqs[i];
        if (freq <= 1) 
            freq = 1;
        else if (freq >= self->halfSr)
            freq = self->halfSr;
    
        MYFLT w0 = self->TwoPiOnSr * freq;
        MYFLT c = MYCOS(w0);
        MYFLT alpha = MYSIN(w0) / (2 * q);
    
        self->b0[i] = alpha;
        self->b2[i] = -alpha;
        self->a0[i] = 1 + alpha;
        self->a1[i] = -2 * c;
        self->a2[i] = 1 - alpha;
    }    
}

static void
BandSplitter_setFrequencies(BandSplitter *self)
{
    int i;
    MYFLT frac = 1. / self->bands;
    for (i=0; i<self->bands; i++) {        
        self->band_freqs[i] = MYPOW(MYPOW(self->max_freq/self->min_freq, frac), i) * self->min_freq;
    }
}

static void
BandSplitter_filters_i(BandSplitter *self) {
    MYFLT val;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (j=0; j<self->bands; j++) {
            self->x1[j] = self->x2[j] = self->y1[j] = self->y2[j] = in[0];
        }    
        self->init = 0;
    }
    
    for (j=0; j<self->bands; j++) {
        for (i=0; i<self->bufsize; i++) {        
            val = ( (self->b0[j] * in[i]) + (self->b2[j] * self->x2[j]) - (self->a1[j] * self->y1[j]) - (self->a2[j] * self->y2[j]) ) / self->a0[j];
            self->y2[j] = self->y1[j];
            self->y1[j] = val;
            self->x2[j] = self->x1[j];
            self->x1[j] = in[i];
            self->buffer_streams[i + j * self->bufsize] = val;
        }    
    }
}

static void
BandSplitter_filters_a(BandSplitter *self) {
    MYFLT val;
    int j, i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        for (j=0; j<self->bands; j++) {
            self->x1[j] = self->x2[j] = self->y1[j] = self->y2[j] = in[0];
        }    
        self->init = 0;
    }

    MYFLT *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        BandSplitter_compute_variables((BandSplitter *)self, q[i]);
        for (j=0; j<self->bands; j++) {
            val = ( (self->b0[j] * in[i]) + (self->b2[j] * self->x2[j]) - (self->a1[j] * self->y1[j]) - (self->a2[j] * self->y2[j]) ) / self->a0[j];
            self->y2[j] = self->y1[j];
            self->y1[j] = val;
            self->x2[j] = self->x1[j];
            self->x1[j] = in[i];
            self->buffer_streams[i + j * self->bufsize] = val;
        }    
    }
}

MYFLT *
BandSplitter_getSamplesBuffer(BandSplitter *self)
{
    return (MYFLT *)self->buffer_streams;
}    

static void
BandSplitter_setProcMode(BandSplitter *self)
{
    int procmode;
    procmode = self->modebuffer[0];
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = BandSplitter_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = BandSplitter_filters_a;
            break;
    }    
}

static void
BandSplitter_compute_next_data_frame(BandSplitter *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
BandSplitter_traverse(BandSplitter *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
BandSplitter_clear(BandSplitter *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
BandSplitter_dealloc(BandSplitter* self)
{
    free(self->data);
    free(self->band_freqs);
    free(self->x1);
    free(self->x2);
    free(self->y1);
    free(self->y2);
    free(self->b0);
    free(self->b1);
    free(self->b2);
    free(self->a0);
    free(self->a1);
    free(self->a2);
    free(self->buffer_streams);
    BandSplitter_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * BandSplitter_deleteStream(BandSplitter *self) { DELETE_STREAM };

static PyObject *
BandSplitter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    BandSplitter *self;
    self = (BandSplitter *)type->tp_alloc(type, 0);


    self->bands = 4;
    self->q = PyFloat_FromDouble(1.);
    self->init = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BandSplitter_compute_next_data_frame);
    self->mode_func_ptr = BandSplitter_setProcMode;
    
    self->halfSr = self->sr / 2.;
    self->TwoPiOnSr = TWOPI / self->sr;
    return (PyObject *)self;
}

static int
BandSplitter_init(BandSplitter *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *qtmp=NULL;
    
    static char *kwlist[] = {"input", "bands", "min", "max", "q", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IFFO, kwlist, &inputtmp, &self->bands, &self->min_freq, &self->max_freq, &qtmp))
        return -1; 

    INIT_INPUT_STREAM
 
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->band_freqs = (MYFLT *)realloc(self->band_freqs, self->bands * sizeof(MYFLT));
    
    self->x1 = (MYFLT *)realloc(self->x1, self->bands * sizeof(MYFLT));
    self->x2 = (MYFLT *)realloc(self->x2, self->bands * sizeof(MYFLT));
    self->y1 = (MYFLT *)realloc(self->y1, self->bands * sizeof(MYFLT));
    self->y2 = (MYFLT *)realloc(self->y2, self->bands * sizeof(MYFLT));

    self->b0 = (MYFLT *)realloc(self->b0, self->bands * sizeof(MYFLT));
    self->b1 = (MYFLT *)realloc(self->b1, self->bands * sizeof(MYFLT));
    self->b2 = (MYFLT *)realloc(self->b2, self->bands * sizeof(MYFLT));
    self->a0 = (MYFLT *)realloc(self->a0, self->bands * sizeof(MYFLT));
    self->a1 = (MYFLT *)realloc(self->a1, self->bands * sizeof(MYFLT));
    self->a2 = (MYFLT *)realloc(self->a2, self->bands * sizeof(MYFLT));

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->bands * self->bufsize * sizeof(MYFLT));

    BandSplitter_setFrequencies((BandSplitter *)self);

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
    }
    else {
        BandSplitter_compute_variables((BandSplitter *)self, PyFloat_AS_DOUBLE(self->q));
    }

    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject *
BandSplitter_setQ(BandSplitter *self, PyObject *arg)
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
        self->modebuffer[0] = 0;
        BandSplitter_compute_variables((BandSplitter *)self, PyFloat_AS_DOUBLE(self->q));
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject * BandSplitter_getServer(BandSplitter* self) { GET_SERVER };
static PyObject * BandSplitter_getStream(BandSplitter* self) { GET_STREAM };

static PyObject * BandSplitter_play(BandSplitter *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BandSplitter_stop(BandSplitter *self) { STOP };

static PyMemberDef BandSplitter_members[] = {
{"server", T_OBJECT_EX, offsetof(BandSplitter, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(BandSplitter, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(BandSplitter, input), 0, "Input sound object."},
{"q", T_OBJECT_EX, offsetof(BandSplitter, q), 0, "Filters Q."},
{NULL}  /* Sentinel */
};

static PyMethodDef BandSplitter_methods[] = {
{"getServer", (PyCFunction)BandSplitter_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)BandSplitter_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)BandSplitter_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"setQ", (PyCFunction)BandSplitter_setQ, METH_O, "Sets the filters Q."},
{"play", (PyCFunction)BandSplitter_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)BandSplitter_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject BandSplitterType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.BandSplitter_base",                                   /*tp_name*/
sizeof(BandSplitter),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)BandSplitter_dealloc,                     /*tp_dealloc*/
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
"BandSplitter objects. Split audio stream in multiple frequency bands.",           /* tp_doc */
(traverseproc)BandSplitter_traverse,                  /* tp_traverse */
(inquiry)BandSplitter_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
BandSplitter_methods,                                 /* tp_methods */
BandSplitter_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)BandSplitter_init,                          /* tp_init */
0,                                              /* tp_alloc */
BandSplitter_new,                                     /* tp_new */
};

/************************************************************************************************/
/* BandSplit streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    BandSplitter *mainSplitter;
    int modebuffer[2];
    int chnl; 
} BandSplit;

static void BandSplit_postprocessing_ii(BandSplit *self) { POST_PROCESSING_II };
static void BandSplit_postprocessing_ai(BandSplit *self) { POST_PROCESSING_AI };
static void BandSplit_postprocessing_ia(BandSplit *self) { POST_PROCESSING_IA };
static void BandSplit_postprocessing_aa(BandSplit *self) { POST_PROCESSING_AA };
static void BandSplit_postprocessing_ireva(BandSplit *self) { POST_PROCESSING_IREVA };
static void BandSplit_postprocessing_areva(BandSplit *self) { POST_PROCESSING_AREVA };
static void BandSplit_postprocessing_revai(BandSplit *self) { POST_PROCESSING_REVAI };
static void BandSplit_postprocessing_revaa(BandSplit *self) { POST_PROCESSING_REVAA };
static void BandSplit_postprocessing_revareva(BandSplit *self) { POST_PROCESSING_REVAREVA };

static void
BandSplit_setProcMode(BandSplit *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = BandSplit_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = BandSplit_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = BandSplit_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = BandSplit_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = BandSplit_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = BandSplit_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = BandSplit_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = BandSplit_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = BandSplit_postprocessing_revareva;
            break;
    }
}

static void
BandSplit_compute_next_data_frame(BandSplit *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = BandSplitter_getSamplesBuffer((BandSplitter *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
BandSplit_traverse(BandSplit *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
BandSplit_clear(BandSplit *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
BandSplit_dealloc(BandSplit* self)
{
    free(self->data);
    BandSplit_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * BandSplit_deleteStream(BandSplit *self) { DELETE_STREAM };

static PyObject *
BandSplit_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    BandSplit *self;
    self = (BandSplit *)type->tp_alloc(type, 0);
    
    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, BandSplit_compute_next_data_frame);
    self->mode_func_ptr = BandSplit_setProcMode;
    
    return (PyObject *)self;
}

static int
BandSplit_init(BandSplit *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (BandSplitter *)maintmp;
    
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

static PyObject * BandSplit_getServer(BandSplit* self) { GET_SERVER };
static PyObject * BandSplit_getStream(BandSplit* self) { GET_STREAM };
static PyObject * BandSplit_setMul(BandSplit *self, PyObject *arg) { SET_MUL };	
static PyObject * BandSplit_setAdd(BandSplit *self, PyObject *arg) { SET_ADD };	
static PyObject * BandSplit_setSub(BandSplit *self, PyObject *arg) { SET_SUB };	
static PyObject * BandSplit_setDiv(BandSplit *self, PyObject *arg) { SET_DIV };	

static PyObject * BandSplit_play(BandSplit *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * BandSplit_out(BandSplit *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * BandSplit_stop(BandSplit *self) { STOP };

static PyObject * BandSplit_multiply(BandSplit *self, PyObject *arg) { MULTIPLY };
static PyObject * BandSplit_inplace_multiply(BandSplit *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * BandSplit_add(BandSplit *self, PyObject *arg) { ADD };
static PyObject * BandSplit_inplace_add(BandSplit *self, PyObject *arg) { INPLACE_ADD };
static PyObject * BandSplit_sub(BandSplit *self, PyObject *arg) { SUB };
static PyObject * BandSplit_inplace_sub(BandSplit *self, PyObject *arg) { INPLACE_SUB };
static PyObject * BandSplit_div(BandSplit *self, PyObject *arg) { DIV };
static PyObject * BandSplit_inplace_div(BandSplit *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef BandSplit_members[] = {
{"server", T_OBJECT_EX, offsetof(BandSplit, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(BandSplit, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(BandSplit, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(BandSplit, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef BandSplit_methods[] = {
{"getServer", (PyCFunction)BandSplit_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)BandSplit_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)BandSplit_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)BandSplit_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)BandSplit_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)BandSplit_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)BandSplit_setMul, METH_O, "Sets BandSplit mul factor."},
{"setAdd", (PyCFunction)BandSplit_setAdd, METH_O, "Sets BandSplit add factor."},
{"setSub", (PyCFunction)BandSplit_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)BandSplit_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods BandSplit_as_number = {
(binaryfunc)BandSplit_add,                      /*nb_add*/
(binaryfunc)BandSplit_sub,                 /*nb_subtract*/
(binaryfunc)BandSplit_multiply,                 /*nb_multiply*/
(binaryfunc)BandSplit_div,                   /*nb_divide*/
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
(binaryfunc)BandSplit_inplace_add,              /*inplace_add*/
(binaryfunc)BandSplit_inplace_sub,         /*inplace_subtract*/
(binaryfunc)BandSplit_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)BandSplit_inplace_div,           /*inplace_divide*/
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

PyTypeObject BandSplitType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.BandSplit_base",         /*tp_name*/
sizeof(BandSplit),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)BandSplit_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&BandSplit_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"BandSplit objects. Reads one band from a BandSplitter process.",           /* tp_doc */
(traverseproc)BandSplit_traverse,   /* tp_traverse */
(inquiry)BandSplit_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
BandSplit_methods,             /* tp_methods */
BandSplit_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)BandSplit_init,      /* tp_init */
0,                         /* tp_alloc */
BandSplit_new,                 /* tp_new */
};
