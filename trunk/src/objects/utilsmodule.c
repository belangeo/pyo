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
/* Print */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int method; // 0 -> interval, 1 -> change
    float lastValue;
    float time;
    float currentTime;
    float sampleToSec;
} Print;

static void
Print_process_time(Print *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= self->time) {
            self->currentTime = 0.0;
            printf("%f\n", in[i]);
        }
        self->currentTime += self->sampleToSec;
    }
}

static void
Print_process_change(Print *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < (self->lastValue-0.00001) || inval > (self->lastValue+0.00001)) {
            printf("%f\n", inval);
            self->lastValue = inval;
        }
    }    
}

static void
Print_setProcMode(Print *self)
{    
    if (self->method < 0 || self->method > 1)
        self->method = 0;
        
    switch (self->method) {
        case 0:
            self->proc_func_ptr = Print_process_time;
            break;
        case 1:
            self->proc_func_ptr = Print_process_change;
            break;
    }        
}

static void
Print_compute_next_data_frame(Print *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
Print_traverse(Print *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Print_clear(Print *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Print_dealloc(Print* self)
{
    free(self->data);
    Print_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Print_deleteStream(Print *self) { DELETE_STREAM };

static PyObject *
Print_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Print *self;
    self = (Print *)type->tp_alloc(type, 0);

    self->method = 0;
    self->time = 0.25;
    self->lastValue = -99999.0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Print_compute_next_data_frame);
    self->mode_func_ptr = Print_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;
    
    return (PyObject *)self;
}

static int
Print_init(Print *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "method", "interval", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|if", kwlist, &inputtmp, &self->method, &self->time))
        return -1; 
    
    INIT_INPUT_STREAM

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Print_getServer(Print* self) { GET_SERVER };
static PyObject * Print_getStream(Print* self) { GET_STREAM };

static PyObject * Print_play(Print *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Print_stop(Print *self) { STOP };

static PyObject *
Print_setMethod(Print *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->method = PyInt_AsLong(arg);
        (*self->mode_func_ptr)(self);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Print_setInterval(Print *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->time = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Print_members[] = {
{"server", T_OBJECT_EX, offsetof(Print, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Print, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Print, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Print_methods[] = {
{"getServer", (PyCFunction)Print_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Print_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Print_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Print_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Print_stop, METH_NOARGS, "Stops computing."},
{"setMethod", (PyCFunction)Print_setMethod, METH_O, "Sets the printing method."},
{"setInterval", (PyCFunction)Print_setInterval, METH_O, "Sets the time interval."},
{NULL}  /* Sentinel */
};

PyTypeObject PrintType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Print_base",                                   /*tp_name*/
sizeof(Print),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Print_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,                                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Print objects. Print the current value of the input object.",           /* tp_doc */
(traverseproc)Print_traverse,                  /* tp_traverse */
(inquiry)Print_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Print_methods,                                 /* tp_methods */
Print_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Print_init,                          /* tp_init */
0,                                              /* tp_alloc */
Print_new,                                     /* tp_new */
};

/*********************************************************************************************/
/* Snap ********************************************************************************/
/*********************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int scale; // 0 = Midi, 1 = frequency, 2 = transpo
    int chSize;
    float *choice;
    float value;
    float last_input;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Snap;

static float
Snap_convert(Snap *self) {
    int midival;
    float val;
    
    midival = self->value;

    if (self->scale == 1)
        val = 8.175798 * powf(1.0594633, midival);
    else if (self->scale == 2)
        val = powf(1.0594633, midival - 60);
    else
        val = midival;
    
    return val;
}

static void
Snap_generate(Snap *self) {
    int i, j, pos;
    float intmp, diff, difftmp;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] < (self->last_input-0.001) || in[i] > (self->last_input + 0.001)) {
            int oct = 0;
            self->last_input = intmp = in[i];
            while (intmp >= 12.0) {
                oct++;
                intmp -= 12.0;
            }
            diff = fabsf(self->choice[0]-intmp);
            pos = 0;
            for (j=1; j<self->chSize; j++) {
                difftmp = fabsf(self->choice[j]-intmp);
                if (difftmp < diff) {
                    diff = difftmp;
                    pos = j;
                }    
            }
            self->value = self->choice[pos] + (12.0 * oct);
            self->value = Snap_convert(self);  
        }
     
        self->data[i] = self->value;
    }
}

static void Snap_postprocessing_ii(Snap *self) { POST_PROCESSING_II };
static void Snap_postprocessing_ai(Snap *self) { POST_PROCESSING_AI };
static void Snap_postprocessing_ia(Snap *self) { POST_PROCESSING_IA };
static void Snap_postprocessing_aa(Snap *self) { POST_PROCESSING_AA };
static void Snap_postprocessing_ireva(Snap *self) { POST_PROCESSING_IREVA };
static void Snap_postprocessing_areva(Snap *self) { POST_PROCESSING_AREVA };
static void Snap_postprocessing_revai(Snap *self) { POST_PROCESSING_REVAI };
static void Snap_postprocessing_revaa(Snap *self) { POST_PROCESSING_REVAA };
static void Snap_postprocessing_revareva(Snap *self) { POST_PROCESSING_REVAREVA };

static void
Snap_setProcMode(Snap *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Snap_generate;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Snap_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Snap_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Snap_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Snap_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Snap_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Snap_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Snap_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Snap_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Snap_postprocessing_revareva;
            break;
    }  
}

static void
Snap_compute_next_data_frame(Snap *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Snap_traverse(Snap *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Snap_clear(Snap *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Snap_dealloc(Snap* self)
{
    free(self->data);
    free(self->choice);
    Snap_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Snap_deleteStream(Snap *self) { DELETE_STREAM };

static PyObject *
Snap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Snap *self;
    self = (Snap *)type->tp_alloc(type, 0);
    
    self->value = self->last_input = 0.;
    self->scale = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Snap_compute_next_data_frame);
    self->mode_func_ptr = Snap_setProcMode;
    return (PyObject *)self;
}

static int
Snap_init(Snap *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *choicetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "choice", "scale", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &inputtmp, &choicetmp, &self->scale, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (choicetmp) {
        PyObject_CallMethod((PyObject *)self, "setChoice", "O", choicetmp);
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

static PyObject * Snap_getServer(Snap* self) { GET_SERVER };
static PyObject * Snap_getStream(Snap* self) { GET_STREAM };
static PyObject * Snap_setMul(Snap *self, PyObject *arg) { SET_MUL };	
static PyObject * Snap_setAdd(Snap *self, PyObject *arg) { SET_ADD };	
static PyObject * Snap_setSub(Snap *self, PyObject *arg) { SET_SUB };	
static PyObject * Snap_setDiv(Snap *self, PyObject *arg) { SET_DIV };	

static PyObject * Snap_play(Snap *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Snap_out(Snap *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Snap_stop(Snap *self) { STOP };

static PyObject * Snap_multiply(Snap *self, PyObject *arg) { MULTIPLY };
static PyObject * Snap_inplace_multiply(Snap *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Snap_add(Snap *self, PyObject *arg) { ADD };
static PyObject * Snap_inplace_add(Snap *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Snap_sub(Snap *self, PyObject *arg) { SUB };
static PyObject * Snap_inplace_sub(Snap *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Snap_div(Snap *self, PyObject *arg) { DIV };
static PyObject * Snap_inplace_div(Snap *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Snap_setChoice(Snap *self, PyObject *arg)
{
    int i;
	PyObject *tmp;
	
	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The choice attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    tmp = arg;
    self->chSize = PyList_Size(tmp);
    self->choice = (float *)realloc(self->choice, self->chSize * sizeof(float));
    
    for (i=0; i<self->chSize; i++) {
        self->choice[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(tmp, i)));
    }
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Snap_setScale(Snap *self, PyObject *arg)
{	
    int tmp;
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyInt_Check(arg);
	
	if (isNumber == 1) {
		tmp = PyInt_AsLong(arg);
        if (tmp >= 0 && tmp <= 2)
            self->scale = tmp;
        else
            printf("scale attribute must be an integer {0, 1, 2}\n");
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Snap_members[] = {
    {"server", T_OBJECT_EX, offsetof(Snap, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Snap, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Snap, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Snap, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Snap, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Snap_methods[] = {
    {"getServer", (PyCFunction)Snap_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Snap_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Snap_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Snap_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Snap_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Snap_stop, METH_NOARGS, "Stops computing."},
    {"setChoice", (PyCFunction)Snap_setChoice, METH_O, "Sets output scale."},
    {"setScale", (PyCFunction)Snap_setScale, METH_O, "Sets new portamento time."},
    {"setMul", (PyCFunction)Snap_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Snap_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Snap_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Snap_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Snap_as_number = {
    (binaryfunc)Snap_add,                         /*nb_add*/
    (binaryfunc)Snap_sub,                         /*nb_subtract*/
    (binaryfunc)Snap_multiply,                    /*nb_multiply*/
    (binaryfunc)Snap_div,                                              /*nb_divide*/
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
    (binaryfunc)Snap_inplace_add,                 /*inplace_add*/
    (binaryfunc)Snap_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Snap_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Snap_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject SnapType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Snap_base",                                   /*tp_name*/
    sizeof(Snap),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Snap_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Snap_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Snap objects. Snap input values on an arbitrary Midi scale.",           /* tp_doc */
    (traverseproc)Snap_traverse,                  /* tp_traverse */
    (inquiry)Snap_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Snap_methods,                                 /* tp_methods */
    Snap_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Snap_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Snap_new,                                     /* tp_new */
};

/************/
/* Interp */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *input2;
    Stream *input2_stream;
    PyObject *interp;
    Stream *interp_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add 
} Interp;

static void
Interp_filters_i(Interp *self) {
    float amp2;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);
    float inter = PyFloat_AS_DOUBLE(self->interp);

    if (inter < 0.0)
        inter = 0.0;
    else if (inter > 1.0)
        inter = 1.0;
    
    amp2 = 1.0 - inter; 
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i] * amp2 + in2[i] * inter;
    }
}

static void
Interp_filters_a(Interp *self) {
    float amp1, amp2;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);
    float *inter = Stream_getData((Stream *)self->interp_stream);
    
    for (i=0; i<self->bufsize; i++) {
        amp1 = inter[i];
        if (amp1 < 0.0)
            amp1 = 0.0;
        else if (amp1 > 1.0)
            amp1 = 1.0;
        
        amp2 = 1.0 - amp1; 
        self->data[i] = in[i] * amp2 + in2[i] * amp1;
    }
}

static void Interp_postprocessing_ii(Interp *self) { POST_PROCESSING_II };
static void Interp_postprocessing_ai(Interp *self) { POST_PROCESSING_AI };
static void Interp_postprocessing_ia(Interp *self) { POST_PROCESSING_IA };
static void Interp_postprocessing_aa(Interp *self) { POST_PROCESSING_AA };
static void Interp_postprocessing_ireva(Interp *self) { POST_PROCESSING_IREVA };
static void Interp_postprocessing_areva(Interp *self) { POST_PROCESSING_AREVA };
static void Interp_postprocessing_revai(Interp *self) { POST_PROCESSING_REVAI };
static void Interp_postprocessing_revaa(Interp *self) { POST_PROCESSING_REVAA };
static void Interp_postprocessing_revareva(Interp *self) { POST_PROCESSING_REVAREVA };

static void
Interp_setProcMode(Interp *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Interp_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = Interp_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Interp_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Interp_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Interp_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Interp_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Interp_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Interp_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Interp_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Interp_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Interp_postprocessing_revareva;
            break;
    }   
}

static void
Interp_compute_next_data_frame(Interp *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Interp_traverse(Interp *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    Py_VISIT(self->interp);    
    Py_VISIT(self->interp_stream);    
    return 0;
}

static int 
Interp_clear(Interp *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    Py_CLEAR(self->interp);    
    Py_CLEAR(self->interp_stream);    
    return 0;
}

static void
Interp_dealloc(Interp* self)
{
    free(self->data);
    Interp_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Interp_deleteStream(Interp *self) { DELETE_STREAM };

static PyObject *
Interp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Interp *self;
    self = (Interp *)type->tp_alloc(type, 0);
    
    self->interp = PyFloat_FromDouble(.5);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Interp_compute_next_data_frame);
    self->mode_func_ptr = Interp_setProcMode;
    return (PyObject *)self;
}

static int
Interp_init(Interp *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *interptmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "input2", "interp", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &input2tmp, &interptmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM

    Py_XDECREF(self->input2); \
    self->input2 = input2tmp; \
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL); \
    Py_INCREF(input2_streamtmp); \
    Py_XDECREF(self->input2_stream); \
    self->input2_stream = (Stream *)input2_streamtmp;
    
    if (interptmp) {
        PyObject_CallMethod((PyObject *)self, "setInterp", "O", interptmp);
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

static PyObject * Interp_getServer(Interp* self) { GET_SERVER };
static PyObject * Interp_getStream(Interp* self) { GET_STREAM };
static PyObject * Interp_setMul(Interp *self, PyObject *arg) { SET_MUL };	
static PyObject * Interp_setAdd(Interp *self, PyObject *arg) { SET_ADD };	
static PyObject * Interp_setSub(Interp *self, PyObject *arg) { SET_SUB };	
static PyObject * Interp_setDiv(Interp *self, PyObject *arg) { SET_DIV };	

static PyObject * Interp_play(Interp *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Interp_out(Interp *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Interp_stop(Interp *self) { STOP };

static PyObject * Interp_multiply(Interp *self, PyObject *arg) { MULTIPLY };
static PyObject * Interp_inplace_multiply(Interp *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Interp_add(Interp *self, PyObject *arg) { ADD };
static PyObject * Interp_inplace_add(Interp *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Interp_sub(Interp *self, PyObject *arg) { SUB };
static PyObject * Interp_inplace_sub(Interp *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Interp_div(Interp *self, PyObject *arg) { DIV };
static PyObject * Interp_inplace_div(Interp *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Interp_setInterp(Interp *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->interp);
	if (isNumber == 1) {
		self->interp = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->interp = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->interp, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->interp_stream);
        self->interp_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Interp_members[] = {
{"server", T_OBJECT_EX, offsetof(Interp, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Interp, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Interp, input), 0, "Input sound object."},
{"input2", T_OBJECT_EX, offsetof(Interp, input2), 0, "Second input sound object."},
{"interp", T_OBJECT_EX, offsetof(Interp, interp), 0, "Cutoff interpuency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Interp, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Interp, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Interp_methods[] = {
{"getServer", (PyCFunction)Interp_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Interp_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Interp_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Interp_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Interp_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Interp_stop, METH_NOARGS, "Stops computing."},
{"setInterp", (PyCFunction)Interp_setInterp, METH_O, "Sets filter cutoff interpuency in cycle per second."},
{"setMul", (PyCFunction)Interp_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Interp_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Interp_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Interp_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Interp_as_number = {
(binaryfunc)Interp_add,                         /*nb_add*/
(binaryfunc)Interp_sub,                         /*nb_subtract*/
(binaryfunc)Interp_multiply,                    /*nb_multiply*/
(binaryfunc)Interp_div,                                              /*nb_divide*/
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
(binaryfunc)Interp_inplace_add,                 /*inplace_add*/
(binaryfunc)Interp_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Interp_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Interp_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject InterpType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Interp_base",                                   /*tp_name*/
sizeof(Interp),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Interp_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Interp_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Interp objects. Interpolates between 2 audio streams.",           /* tp_doc */
(traverseproc)Interp_traverse,                  /* tp_traverse */
(inquiry)Interp_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Interp_methods,                                 /* tp_methods */
Interp_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Interp_init,                          /* tp_init */
0,                                              /* tp_alloc */
Interp_new,                                     /* tp_new */
};

/************/
/* SampHold */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *controlsig;
    Stream *controlsig_stream;
    PyObject *value;
    Stream *value_stream;
    float currentValue;
    int flag;
    int modebuffer[3]; // need at least 2 slots for mul & add 
} SampHold;

static void
SampHold_filters_i(SampHold *self) {
    float ctrl;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    float val = PyFloat_AS_DOUBLE(self->value);

    for (i=0; i<self->bufsize; i++) {
        ctrl = ctrlsig[i];
        if (ctrl > (val - 0.001) && ctrl < (val + 0.001)) {
            if (self->flag == 1) {
                self->currentValue = in[i];
                self->flag = 0;
            }    
        }
        else
            self->flag = 1;
        self->data[i] = self->currentValue;
    }
}

static void
SampHold_filters_a(SampHold *self) {
    float ctrl, val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    float *valsig = Stream_getData((Stream *)self->value_stream);
    
    for (i=0; i<self->bufsize; i++) {
        ctrl = ctrlsig[i];
        val = valsig[i];
        if (ctrl > (val - 0.001) && ctrl < (val + 0.001)) {
            if (self->flag == 1) {
                self->currentValue = in[i];
                self->flag = 0;
            }    
        }
        else
            self->flag = 1;
        self->data[i] = self->currentValue;
    }
}

static void SampHold_postprocessing_ii(SampHold *self) { POST_PROCESSING_II };
static void SampHold_postprocessing_ai(SampHold *self) { POST_PROCESSING_AI };
static void SampHold_postprocessing_ia(SampHold *self) { POST_PROCESSING_IA };
static void SampHold_postprocessing_aa(SampHold *self) { POST_PROCESSING_AA };
static void SampHold_postprocessing_ireva(SampHold *self) { POST_PROCESSING_IREVA };
static void SampHold_postprocessing_areva(SampHold *self) { POST_PROCESSING_AREVA };
static void SampHold_postprocessing_revai(SampHold *self) { POST_PROCESSING_REVAI };
static void SampHold_postprocessing_revaa(SampHold *self) { POST_PROCESSING_REVAA };
static void SampHold_postprocessing_revareva(SampHold *self) { POST_PROCESSING_REVAREVA };

static void
SampHold_setProcMode(SampHold *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = SampHold_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = SampHold_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SampHold_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SampHold_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SampHold_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SampHold_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SampHold_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SampHold_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SampHold_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SampHold_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SampHold_postprocessing_revareva;
            break;
    }   
}

static void
SampHold_compute_next_data_frame(SampHold *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SampHold_traverse(SampHold *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->controlsig);
    Py_VISIT(self->controlsig_stream);
    Py_VISIT(self->value);    
    Py_VISIT(self->value_stream);    
    return 0;
}

static int 
SampHold_clear(SampHold *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->controlsig);
    Py_CLEAR(self->controlsig_stream);
    Py_CLEAR(self->value);    
    Py_CLEAR(self->value_stream);    
    return 0;
}

static void
SampHold_dealloc(SampHold* self)
{
    free(self->data);
    SampHold_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SampHold_deleteStream(SampHold *self) { DELETE_STREAM };

static PyObject *
SampHold_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    SampHold *self;
    self = (SampHold *)type->tp_alloc(type, 0);
    
    self->value = PyFloat_FromDouble(0.0);
    self->currentValue = 0.0;
    self->flag = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SampHold_compute_next_data_frame);
    self->mode_func_ptr = SampHold_setProcMode;
    return (PyObject *)self;
}

static int
SampHold_init(SampHold *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *controlsigtmp, *controlsig_streamtmp, *valuetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "controlsig", "value", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &controlsigtmp, &valuetmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    Py_XDECREF(self->controlsig); \
    self->controlsig = controlsigtmp; \
    controlsig_streamtmp = PyObject_CallMethod((PyObject *)self->controlsig, "_getStream", NULL); \
    Py_INCREF(controlsig_streamtmp); \
    Py_XDECREF(self->controlsig_stream); \
    self->controlsig_stream = (Stream *)controlsig_streamtmp;
    
    if (valuetmp) {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
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

static PyObject * SampHold_getServer(SampHold* self) { GET_SERVER };
static PyObject * SampHold_getStream(SampHold* self) { GET_STREAM };
static PyObject * SampHold_setMul(SampHold *self, PyObject *arg) { SET_MUL };	
static PyObject * SampHold_setAdd(SampHold *self, PyObject *arg) { SET_ADD };	
static PyObject * SampHold_setSub(SampHold *self, PyObject *arg) { SET_SUB };	
static PyObject * SampHold_setDiv(SampHold *self, PyObject *arg) { SET_DIV };	

static PyObject * SampHold_play(SampHold *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SampHold_out(SampHold *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SampHold_stop(SampHold *self) { STOP };

static PyObject * SampHold_multiply(SampHold *self, PyObject *arg) { MULTIPLY };
static PyObject * SampHold_inplace_multiply(SampHold *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SampHold_add(SampHold *self, PyObject *arg) { ADD };
static PyObject * SampHold_inplace_add(SampHold *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SampHold_sub(SampHold *self, PyObject *arg) { SUB };
static PyObject * SampHold_inplace_sub(SampHold *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SampHold_div(SampHold *self, PyObject *arg) { DIV };
static PyObject * SampHold_inplace_div(SampHold *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SampHold_setValue(SampHold *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->value);
	if (isNumber == 1) {
		self->value = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->value = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->value, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->value_stream);
        self->value_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef SampHold_members[] = {
{"server", T_OBJECT_EX, offsetof(SampHold, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SampHold, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(SampHold, input), 0, "Input sound object."},
{"controlsig", T_OBJECT_EX, offsetof(SampHold, controlsig), 0, "Control input object."},
{"value", T_OBJECT_EX, offsetof(SampHold, value), 0, "Trigger value."},
{"mul", T_OBJECT_EX, offsetof(SampHold, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SampHold, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SampHold_methods[] = {
{"getServer", (PyCFunction)SampHold_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SampHold_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SampHold_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SampHold_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SampHold_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SampHold_stop, METH_NOARGS, "Stops computing."},
{"setValue", (PyCFunction)SampHold_setValue, METH_O, "Sets trigger value."},
{"setMul", (PyCFunction)SampHold_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)SampHold_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)SampHold_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SampHold_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SampHold_as_number = {
(binaryfunc)SampHold_add,                         /*nb_add*/
(binaryfunc)SampHold_sub,                         /*nb_subtract*/
(binaryfunc)SampHold_multiply,                    /*nb_multiply*/
(binaryfunc)SampHold_div,                                              /*nb_divide*/
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
(binaryfunc)SampHold_inplace_add,                 /*inplace_add*/
(binaryfunc)SampHold_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)SampHold_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)SampHold_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject SampHoldType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.SampHold_base",                                   /*tp_name*/
sizeof(SampHold),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)SampHold_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&SampHold_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"SampHold objects. SampHoldolates between 2 audio streams.",           /* tp_doc */
(traverseproc)SampHold_traverse,                  /* tp_traverse */
(inquiry)SampHold_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
SampHold_methods,                                 /* tp_methods */
SampHold_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)SampHold_init,                          /* tp_init */
0,                                              /* tp_alloc */
SampHold_new,                                     /* tp_new */
};

/************/
/* Compare */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *comp;
    Stream *comp_stream;
    float (*compare_func_ptr)(float, float); // true = 1.0, false = 0.0
    int modebuffer[3]; // need at least 2 slots for mul & add 
} Compare;

static float
Compare_lt(float in, float comp) {
    if (in < comp) { return 1.0; }
    else { return 0.0; }
}

static float
Compare_elt(float in, float comp) {
    if (in <= comp) { return 1.0; }
    else { return 0.0; }
}

static float
Compare_gt(float in, float comp) {
    if (in > comp) { return 1.0; }
    else { return 0.0; }
}

static float
Compare_egt(float in, float comp) {
    if (in >= comp) { return 1.0; }
    else { return 0.0; }
}

static float
Compare_eq(float in, float comp) {
    if (in >= (comp - 0.0001) && in <= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static float
Compare_neq(float in, float comp) {
    if (in <= (comp - 0.0001) || in >= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static void
Compare_process_i(Compare *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float comp = PyFloat_AS_DOUBLE(self->comp);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = (*self->compare_func_ptr)(in[i], comp);
    }
}

static void
Compare_process_a(Compare *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *comp = Stream_getData((Stream *)self->comp_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = (*self->compare_func_ptr)(in[i], comp[i]);
    }
}

static void Compare_postprocessing_ii(Compare *self) { POST_PROCESSING_II };
static void Compare_postprocessing_ai(Compare *self) { POST_PROCESSING_AI };
static void Compare_postprocessing_ia(Compare *self) { POST_PROCESSING_IA };
static void Compare_postprocessing_aa(Compare *self) { POST_PROCESSING_AA };
static void Compare_postprocessing_ireva(Compare *self) { POST_PROCESSING_IREVA };
static void Compare_postprocessing_areva(Compare *self) { POST_PROCESSING_AREVA };
static void Compare_postprocessing_revai(Compare *self) { POST_PROCESSING_REVAI };
static void Compare_postprocessing_revaa(Compare *self) { POST_PROCESSING_REVAA };
static void Compare_postprocessing_revareva(Compare *self) { POST_PROCESSING_REVAREVA };

static void
Compare_setProcMode(Compare *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Compare_process_i;
            break;
        case 1:    
            self->proc_func_ptr = Compare_process_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Compare_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Compare_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Compare_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Compare_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Compare_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Compare_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Compare_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Compare_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Compare_postprocessing_revareva;
            break;
    }   
}

static void
Compare_compute_next_data_frame(Compare *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Compare_traverse(Compare *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->comp);
    Py_VISIT(self->comp_stream);
    return 0;
}

static int 
Compare_clear(Compare *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->comp);
    Py_CLEAR(self->comp_stream);
    return 0;
}

static void
Compare_dealloc(Compare* self)
{
    free(self->data);
    Compare_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Compare_deleteStream(Compare *self) { DELETE_STREAM };

static PyObject *
Compare_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Compare *self;
    self = (Compare *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    self->compare_func_ptr = Compare_lt;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Compare_compute_next_data_frame);
    self->mode_func_ptr = Compare_setProcMode;
    return (PyObject *)self;
}

static int
Compare_init(Compare *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *comptmp, *modetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "comp", "mode", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &comptmp, &modetmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (comptmp) {
        PyObject_CallMethod((PyObject *)self, "setComp", "O", comptmp);
    }
    
    if (modetmp) {
        PyObject_CallMethod((PyObject *)self, "setMode", "O", modetmp);
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

static PyObject * Compare_getServer(Compare* self) { GET_SERVER };
static PyObject * Compare_getStream(Compare* self) { GET_STREAM };
static PyObject * Compare_setMul(Compare *self, PyObject *arg) { SET_MUL };	
static PyObject * Compare_setAdd(Compare *self, PyObject *arg) { SET_ADD };	
static PyObject * Compare_setSub(Compare *self, PyObject *arg) { SET_SUB };	
static PyObject * Compare_setDiv(Compare *self, PyObject *arg) { SET_DIV };	

static PyObject * Compare_play(Compare *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Compare_stop(Compare *self) { STOP };

static PyObject * Compare_multiply(Compare *self, PyObject *arg) { MULTIPLY };
static PyObject * Compare_inplace_multiply(Compare *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Compare_add(Compare *self, PyObject *arg) { ADD };
static PyObject * Compare_inplace_add(Compare *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Compare_sub(Compare *self, PyObject *arg) { SUB };
static PyObject * Compare_inplace_sub(Compare *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Compare_div(Compare *self, PyObject *arg) { DIV };
static PyObject * Compare_inplace_div(Compare *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Compare_setComp(Compare *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
    
	if (arg == NULL) {
		Py_RETURN_NONE;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_XDECREF(self->comp);
	if (isNumber == 1) {
		self->comp = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->comp = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->comp, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->comp_stream);
        self->comp_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Compare_setMode(Compare *self, PyObject *arg)
{	
	if (arg == NULL) {
		Py_RETURN_NONE;
	}
    
	if (! PyInt_Check(arg)) {
        printf("mode should be a comparison operator as a string\n");
		Py_RETURN_NONE;
    }

    int tmp = PyInt_AsLong(arg);
    
    if (tmp == 0)
        self->compare_func_ptr = Compare_lt;
    else if (tmp == 1)
        self->compare_func_ptr = Compare_elt;
    else if (tmp == 2)
        self->compare_func_ptr = Compare_gt;
    else if (tmp == 3)
        self->compare_func_ptr = Compare_egt;
    else if (tmp == 4)
        self->compare_func_ptr = Compare_eq;
    else if (tmp == 5)
        self->compare_func_ptr = Compare_neq;
    
    Py_RETURN_NONE;
}

static PyMemberDef Compare_members[] = {
{"server", T_OBJECT_EX, offsetof(Compare, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Compare, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Compare, input), 0, "Input sound object."},
{"comp", T_OBJECT_EX, offsetof(Compare, comp), 0, "Comparison object."},
{"mul", T_OBJECT_EX, offsetof(Compare, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Compare, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Compare_methods[] = {
{"getServer", (PyCFunction)Compare_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Compare_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Compare_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Compare_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Compare_stop, METH_NOARGS, "Stops computing."},
{"setComp", (PyCFunction)Compare_setComp, METH_O, "Sets the comparison object."},
{"setMode", (PyCFunction)Compare_setMode, METH_O, "Sets the comparison mode."},
{"setMul", (PyCFunction)Compare_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Compare_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Compare_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Compare_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Compare_as_number = {
(binaryfunc)Compare_add,                         /*nb_add*/
(binaryfunc)Compare_sub,                         /*nb_subtract*/
(binaryfunc)Compare_multiply,                    /*nb_multiply*/
(binaryfunc)Compare_div,                         /*nb_divide*/
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
(binaryfunc)Compare_inplace_add,                 /*inplace_add*/
(binaryfunc)Compare_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Compare_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Compare_inplace_div,                 /*inplace_divide*/
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

PyTypeObject CompareType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Compare_base",                                   /*tp_name*/
sizeof(Compare),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Compare_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Compare_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Compare objects. Comparison between 2 audio streams.",           /* tp_doc */
(traverseproc)Compare_traverse,                  /* tp_traverse */
(inquiry)Compare_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Compare_methods,                                 /* tp_methods */
Compare_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Compare_init,                          /* tp_init */
0,                                              /* tp_alloc */
Compare_new,                                     /* tp_new */
};
