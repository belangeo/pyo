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
    MYFLT lastValue;
    MYFLT time;
    MYFLT currentTime;
    MYFLT sampleToSec;
} Print;

static void
Print_process_time(Print *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
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
    MYFLT inval;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

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
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "method", "interval", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IF, kwlist, &inputtmp, &self->method, &self->time))
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
    MYFLT *choice;
    MYFLT value;
    MYFLT last_input;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Snap;

static MYFLT
Snap_convert(Snap *self) {
    int midival;
    MYFLT val;
    
    midival = self->value;

    if (self->scale == 1)
        val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
    else if (self->scale == 2)
        val = MYPOW(1.0594630943593, midival - 60);
    else
        val = midival;
    
    return val;
}

static void
Snap_generate(Snap *self) {
    int i, j, pos;
    MYFLT intmp, diff, difftmp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] < (self->last_input-0.001) || in[i] > (self->last_input + 0.001)) {
            int oct = 0;
            self->last_input = intmp = in[i];
            while (intmp >= 12.0) {
                oct++;
                intmp -= 12.0;
            }
            diff = MYFABS(self->choice[0]-intmp);
            pos = 0;
            for (j=1; j<self->chSize; j++) {
                difftmp = MYFABS(self->choice[j]-intmp);
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
    self->choice = (MYFLT *)realloc(self->choice, self->chSize * sizeof(MYFLT));
    
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
    MYFLT amp2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    MYFLT inter = PyFloat_AS_DOUBLE(self->interp);

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
    MYFLT amp1, amp2;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    MYFLT *inter = Stream_getData((Stream *)self->interp_stream);
    
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

    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
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
    MYFLT currentValue;
    int flag;
    int modebuffer[3]; // need at least 2 slots for mul & add 
} SampHold;

static void
SampHold_filters_i(SampHold *self) {
    MYFLT ctrl;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT val = PyFloat_AS_DOUBLE(self->value);

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
    MYFLT ctrl, val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *ctrlsig = Stream_getData((Stream *)self->controlsig_stream);
    MYFLT *valsig = Stream_getData((Stream *)self->value_stream);
    
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
    MYFLT (*compare_func_ptr)(MYFLT, MYFLT); // true = 1.0, false = 0.0
    int modebuffer[3]; // need at least 2 slots for mul & add 
} Compare;

static MYFLT
Compare_lt(MYFLT in, MYFLT comp) {
    if (in < comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_elt(MYFLT in, MYFLT comp) {
    if (in <= comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_gt(MYFLT in, MYFLT comp) {
    if (in > comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_egt(MYFLT in, MYFLT comp) {
    if (in >= comp) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_eq(MYFLT in, MYFLT comp) {
    if (in >= (comp - 0.0001) && in <= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static MYFLT
Compare_neq(MYFLT in, MYFLT comp) {
    if (in <= (comp - 0.0001) || in >= (comp + 0.0001)) { return 1.0; }
    else { return 0.0; }
}

static void
Compare_process_i(Compare *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT comp = PyFloat_AS_DOUBLE(self->comp);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = (*self->compare_func_ptr)(in[i], comp);
    }
}

static void
Compare_process_a(Compare *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *comp = Stream_getData((Stream *)self->comp_stream);
    
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

/*****************/
/** Between object **/
/*****************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    Stream *min_stream;
    PyObject *max;
    Stream *max_stream;
    int modebuffer[4];
} Between;

static void
Between_transform_ii(Between *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        if(val >= mi && val < ma)
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_ai(Between *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        if(val >= mi[i] && val < ma)
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_ia(Between *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        if(val >= mi && val < ma[i])
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void
Between_transform_aa(Between *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        if(val >= mi[i] && val < ma[i])
            self->data[i] = 1.0;
        else
            self->data[i] = 0.0;
    }
}

static void Between_postprocessing_ii(Between *self) { POST_PROCESSING_II };
static void Between_postprocessing_ai(Between *self) { POST_PROCESSING_AI };
static void Between_postprocessing_ia(Between *self) { POST_PROCESSING_IA };
static void Between_postprocessing_aa(Between *self) { POST_PROCESSING_AA };
static void Between_postprocessing_ireva(Between *self) { POST_PROCESSING_IREVA };
static void Between_postprocessing_areva(Between *self) { POST_PROCESSING_AREVA };
static void Between_postprocessing_revai(Between *self) { POST_PROCESSING_REVAI };
static void Between_postprocessing_revaa(Between *self) { POST_PROCESSING_REVAA };
static void Between_postprocessing_revareva(Between *self) { POST_PROCESSING_REVAREVA };

static void
Between_setProcMode(Between *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Between_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Between_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Between_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Between_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Between_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Between_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Between_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Between_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Between_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Between_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Between_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Between_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Between_postprocessing_revareva;
            break;
    }   
}

static void
Between_compute_next_data_frame(Between *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Between_traverse(Between *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
Between_clear(Between *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
Between_dealloc(Between* self)
{
    free(self->data);
    Between_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Between_deleteStream(Between *self) { DELETE_STREAM };

static PyObject *
Between_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Between *self;
    self = (Between *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.0);
    self->max = PyFloat_FromDouble(1.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Between_compute_next_data_frame);
    self->mode_func_ptr = Between_setProcMode;
    
    return (PyObject *)self;
}

static int
Between_init(Between *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
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

static PyObject * Between_getServer(Between* self) { GET_SERVER };
static PyObject * Between_getStream(Between* self) { GET_STREAM };
static PyObject * Between_setMul(Between *self, PyObject *arg) { SET_MUL };	
static PyObject * Between_setAdd(Between *self, PyObject *arg) { SET_ADD };	
static PyObject * Between_setSub(Between *self, PyObject *arg) { SET_SUB };	
static PyObject * Between_setDiv(Between *self, PyObject *arg) { SET_DIV };	

static PyObject * Between_play(Between *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Between_out(Between *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Between_stop(Between *self) { STOP };

static PyObject * Between_multiply(Between *self, PyObject *arg) { MULTIPLY };
static PyObject * Between_inplace_multiply(Between *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Between_add(Between *self, PyObject *arg) { ADD };
static PyObject * Between_inplace_add(Between *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Between_sub(Between *self, PyObject *arg) { SUB };
static PyObject * Between_inplace_sub(Between *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Between_div(Between *self, PyObject *arg) { DIV };
static PyObject * Between_inplace_div(Between *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Between_setMin(Between *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Between_setMax(Between *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Between_members[] = {
    {"server", T_OBJECT_EX, offsetof(Between, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Between, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Between, input), 0, "Input sound object."},
    {"min", T_OBJECT_EX, offsetof(Between, min), 0, "Minimum possible value."},
    {"max", T_OBJECT_EX, offsetof(Between, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(Between, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Between, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Between_methods[] = {
    {"getServer", (PyCFunction)Between_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Between_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Between_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Between_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Between_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Between_stop, METH_NOARGS, "Stops computing."},
    {"setMin", (PyCFunction)Between_setMin, METH_O, "Sets the minimum value."},
    {"setMax", (PyCFunction)Between_setMax, METH_O, "Sets the maximum value."},
    {"setMul", (PyCFunction)Between_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Between_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Between_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Between_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Between_as_number = {
    (binaryfunc)Between_add,                      /*nb_add*/
    (binaryfunc)Between_sub,                 /*nb_subtract*/
    (binaryfunc)Between_multiply,                 /*nb_multiply*/
    (binaryfunc)Between_div,                   /*nb_divide*/
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
    (binaryfunc)Between_inplace_add,              /*inplace_add*/
    (binaryfunc)Between_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Between_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Between_inplace_div,           /*inplace_divide*/
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

PyTypeObject BetweenType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Between_base",         /*tp_name*/
    sizeof(Between),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Between_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Between_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Between objects. Outputs a trig if signal is between min and max values.",           /* tp_doc */
    (traverseproc)Between_traverse,   /* tp_traverse */
    (inquiry)Between_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Between_methods,             /* tp_methods */
    Between_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Between_init,      /* tp_init */
    0,                         /* tp_alloc */
    Between_new,                 /* tp_new */
};


/************/
/* Denorm */
/************/
#ifndef USE_DOUBLE
#define DENORM_RAND  ((MYFLT) ((rand()/((MYFLT)(RAND_MAX)*0.5+1) - 1.0) * (MYFLT)(1.0e-24)))
#else
#define DENORM_RAND  ((MYFLT) ((rand()/((MYFLT)(RAND_MAX)*0.5+1) - 1.0) * (MYFLT)(1.0e-60)))
#endif

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Denorm;

static void
Denorm_filters(Denorm *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i] + DENORM_RAND;
    }
}

static void Denorm_postprocessing_ii(Denorm *self) { POST_PROCESSING_II };
static void Denorm_postprocessing_ai(Denorm *self) { POST_PROCESSING_AI };
static void Denorm_postprocessing_ia(Denorm *self) { POST_PROCESSING_IA };
static void Denorm_postprocessing_aa(Denorm *self) { POST_PROCESSING_AA };
static void Denorm_postprocessing_ireva(Denorm *self) { POST_PROCESSING_IREVA };
static void Denorm_postprocessing_areva(Denorm *self) { POST_PROCESSING_AREVA };
static void Denorm_postprocessing_revai(Denorm *self) { POST_PROCESSING_REVAI };
static void Denorm_postprocessing_revaa(Denorm *self) { POST_PROCESSING_REVAA };
static void Denorm_postprocessing_revareva(Denorm *self) { POST_PROCESSING_REVAREVA };

static void
Denorm_setProcMode(Denorm *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Denorm_filters;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Denorm_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Denorm_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Denorm_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Denorm_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Denorm_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Denorm_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Denorm_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Denorm_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Denorm_postprocessing_revareva;
            break;
    }   
}

static void
Denorm_compute_next_data_frame(Denorm *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Denorm_traverse(Denorm *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Denorm_clear(Denorm *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Denorm_dealloc(Denorm* self)
{
    free(self->data);
    Denorm_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Denorm_deleteStream(Denorm *self) { DELETE_STREAM };

static PyObject *
Denorm_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Denorm *self;
    self = (Denorm *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Denorm_compute_next_data_frame);
    self->mode_func_ptr = Denorm_setProcMode;
    return (PyObject *)self;
}

static int
Denorm_init(Denorm *self, PyObject *args, PyObject *kwds)
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
    
    srand((unsigned)(time(0)));

    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Denorm_getServer(Denorm* self) { GET_SERVER };
static PyObject * Denorm_getStream(Denorm* self) { GET_STREAM };
static PyObject * Denorm_setMul(Denorm *self, PyObject *arg) { SET_MUL };	
static PyObject * Denorm_setAdd(Denorm *self, PyObject *arg) { SET_ADD };	
static PyObject * Denorm_setSub(Denorm *self, PyObject *arg) { SET_SUB };	
static PyObject * Denorm_setDiv(Denorm *self, PyObject *arg) { SET_DIV };	

static PyObject * Denorm_play(Denorm *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Denorm_out(Denorm *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Denorm_stop(Denorm *self) { STOP };

static PyObject * Denorm_multiply(Denorm *self, PyObject *arg) { MULTIPLY };
static PyObject * Denorm_inplace_multiply(Denorm *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Denorm_add(Denorm *self, PyObject *arg) { ADD };
static PyObject * Denorm_inplace_add(Denorm *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Denorm_sub(Denorm *self, PyObject *arg) { SUB };
static PyObject * Denorm_inplace_sub(Denorm *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Denorm_div(Denorm *self, PyObject *arg) { DIV };
static PyObject * Denorm_inplace_div(Denorm *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Denorm_members[] = {
    {"server", T_OBJECT_EX, offsetof(Denorm, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Denorm, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Denorm, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Denorm, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Denorm, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Denorm_methods[] = {
    {"getServer", (PyCFunction)Denorm_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Denorm_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Denorm_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Denorm_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Denorm_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Denorm_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)Denorm_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Denorm_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Denorm_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Denorm_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Denorm_as_number = {
    (binaryfunc)Denorm_add,                         /*nb_add*/
    (binaryfunc)Denorm_sub,                         /*nb_subtract*/
    (binaryfunc)Denorm_multiply,                    /*nb_multiply*/
    (binaryfunc)Denorm_div,                                              /*nb_divide*/
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
    (binaryfunc)Denorm_inplace_add,                 /*inplace_add*/
    (binaryfunc)Denorm_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Denorm_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Denorm_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject DenormType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Denorm_base",                                   /*tp_name*/
    sizeof(Denorm),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Denorm_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Denorm_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Denorm objects. Mixes low level noise to an input signal.",           /* tp_doc */
    (traverseproc)Denorm_traverse,                  /* tp_traverse */
    (inquiry)Denorm_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Denorm_methods,                                 /* tp_methods */
    Denorm_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Denorm_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Denorm_new,                                     /* tp_new */
};

/************/
/* DBToA */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastdb;
    MYFLT currentamp;
    int modebuffer[2]; // need at least 2 slots for mul & add
} DBToA;

static void
DBToA_process(DBToA *self) {
    int i;
    MYFLT db;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        db = in[i];
        if (db <= -120.0) {
            self->data[i] = self->currentamp = 0.0;
            self->lastdb = -120.0;
        }
        else if (db != self->lastdb) {
            self->data[i] = self->currentamp = MYPOW(10.0, db * 0.05);
            self->lastdb = db;
        }
        else
            self->data[i] = self->currentamp;
    }
}

static void DBToA_postprocessing_ii(DBToA *self) { POST_PROCESSING_II };
static void DBToA_postprocessing_ai(DBToA *self) { POST_PROCESSING_AI };
static void DBToA_postprocessing_ia(DBToA *self) { POST_PROCESSING_IA };
static void DBToA_postprocessing_aa(DBToA *self) { POST_PROCESSING_AA };
static void DBToA_postprocessing_ireva(DBToA *self) { POST_PROCESSING_IREVA };
static void DBToA_postprocessing_areva(DBToA *self) { POST_PROCESSING_AREVA };
static void DBToA_postprocessing_revai(DBToA *self) { POST_PROCESSING_REVAI };
static void DBToA_postprocessing_revaa(DBToA *self) { POST_PROCESSING_REVAA };
static void DBToA_postprocessing_revareva(DBToA *self) { POST_PROCESSING_REVAREVA };

static void
DBToA_setProcMode(DBToA *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = DBToA_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = DBToA_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = DBToA_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = DBToA_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = DBToA_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = DBToA_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = DBToA_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = DBToA_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = DBToA_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = DBToA_postprocessing_revareva;
            break;
    }   
}

static void
DBToA_compute_next_data_frame(DBToA *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
DBToA_traverse(DBToA *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
DBToA_clear(DBToA *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
DBToA_dealloc(DBToA* self)
{
    free(self->data);
    DBToA_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * DBToA_deleteStream(DBToA *self) { DELETE_STREAM };

static PyObject *
DBToA_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    DBToA *self;
    self = (DBToA *)type->tp_alloc(type, 0);
    
    self->lastdb = -120.0;
    self->currentamp = MYPOW(10.0, self->lastdb * 0.05);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, DBToA_compute_next_data_frame);
    self->mode_func_ptr = DBToA_setProcMode;
    return (PyObject *)self;
}

static int
DBToA_init(DBToA *self, PyObject *args, PyObject *kwds)
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

static PyObject * DBToA_getServer(DBToA* self) { GET_SERVER };
static PyObject * DBToA_getStream(DBToA* self) { GET_STREAM };
static PyObject * DBToA_setMul(DBToA *self, PyObject *arg) { SET_MUL };	
static PyObject * DBToA_setAdd(DBToA *self, PyObject *arg) { SET_ADD };	
static PyObject * DBToA_setSub(DBToA *self, PyObject *arg) { SET_SUB };	
static PyObject * DBToA_setDiv(DBToA *self, PyObject *arg) { SET_DIV };	

static PyObject * DBToA_play(DBToA *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * DBToA_out(DBToA *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * DBToA_stop(DBToA *self) { STOP };

static PyObject * DBToA_multiply(DBToA *self, PyObject *arg) { MULTIPLY };
static PyObject * DBToA_inplace_multiply(DBToA *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * DBToA_add(DBToA *self, PyObject *arg) { ADD };
static PyObject * DBToA_inplace_add(DBToA *self, PyObject *arg) { INPLACE_ADD };
static PyObject * DBToA_sub(DBToA *self, PyObject *arg) { SUB };
static PyObject * DBToA_inplace_sub(DBToA *self, PyObject *arg) { INPLACE_SUB };
static PyObject * DBToA_div(DBToA *self, PyObject *arg) { DIV };
static PyObject * DBToA_inplace_div(DBToA *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef DBToA_members[] = {
    {"server", T_OBJECT_EX, offsetof(DBToA, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(DBToA, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(DBToA, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(DBToA, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(DBToA, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef DBToA_methods[] = {
    {"getServer", (PyCFunction)DBToA_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)DBToA_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)DBToA_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)DBToA_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)DBToA_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)DBToA_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)DBToA_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)DBToA_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)DBToA_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)DBToA_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods DBToA_as_number = {
    (binaryfunc)DBToA_add,                         /*nb_add*/
    (binaryfunc)DBToA_sub,                         /*nb_subtract*/
    (binaryfunc)DBToA_multiply,                    /*nb_multiply*/
    (binaryfunc)DBToA_div,                                              /*nb_divide*/
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
    (binaryfunc)DBToA_inplace_add,                 /*inplace_add*/
    (binaryfunc)DBToA_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)DBToA_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)DBToA_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject DBToAType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.DBToA_base",                                   /*tp_name*/
    sizeof(DBToA),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)DBToA_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &DBToA_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "DBToA objects. Converts dB value to amplitude value.",           /* tp_doc */
    (traverseproc)DBToA_traverse,                  /* tp_traverse */
    (inquiry)DBToA_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    DBToA_methods,                                 /* tp_methods */
    DBToA_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)DBToA_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    DBToA_new,                                     /* tp_new */
};

/************/
/* AToDB */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    MYFLT lastamp;
    MYFLT currentdb;
    int modebuffer[2]; // need at least 2 slots for mul & add
} AToDB;

static void
AToDB_process(AToDB *self) {
    int i;
    MYFLT amp;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        amp = in[i];
        if (amp <= 0.000001) {
            self->data[i] = self->currentdb = -120.0;
            self->lastamp = 0.000001;
        }
        else if (amp != self->lastamp) {
            self->data[i] = self->currentdb = 20.0 * MYLOG10(amp);
            self->lastamp = amp;
        }
        else
            self->data[i] = self->currentdb;
    }
}

static void AToDB_postprocessing_ii(AToDB *self) { POST_PROCESSING_II };
static void AToDB_postprocessing_ai(AToDB *self) { POST_PROCESSING_AI };
static void AToDB_postprocessing_ia(AToDB *self) { POST_PROCESSING_IA };
static void AToDB_postprocessing_aa(AToDB *self) { POST_PROCESSING_AA };
static void AToDB_postprocessing_ireva(AToDB *self) { POST_PROCESSING_IREVA };
static void AToDB_postprocessing_areva(AToDB *self) { POST_PROCESSING_AREVA };
static void AToDB_postprocessing_revai(AToDB *self) { POST_PROCESSING_REVAI };
static void AToDB_postprocessing_revaa(AToDB *self) { POST_PROCESSING_REVAA };
static void AToDB_postprocessing_revareva(AToDB *self) { POST_PROCESSING_REVAREVA };

static void
AToDB_setProcMode(AToDB *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = AToDB_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = AToDB_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = AToDB_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = AToDB_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = AToDB_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = AToDB_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = AToDB_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = AToDB_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = AToDB_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = AToDB_postprocessing_revareva;
            break;
    }   
}

static void
AToDB_compute_next_data_frame(AToDB *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
AToDB_traverse(AToDB *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
AToDB_clear(AToDB *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
AToDB_dealloc(AToDB* self)
{
    free(self->data);
    AToDB_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * AToDB_deleteStream(AToDB *self) { DELETE_STREAM };

static PyObject *
AToDB_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    AToDB *self;
    self = (AToDB *)type->tp_alloc(type, 0);
    
    self->lastamp = 0.000001;
    self->currentdb = 20.0 * MYLOG10(self->lastamp);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, AToDB_compute_next_data_frame);
    self->mode_func_ptr = AToDB_setProcMode;
    return (PyObject *)self;
}

static int
AToDB_init(AToDB *self, PyObject *args, PyObject *kwds)
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

static PyObject * AToDB_getServer(AToDB* self) { GET_SERVER };
static PyObject * AToDB_getStream(AToDB* self) { GET_STREAM };
static PyObject * AToDB_setMul(AToDB *self, PyObject *arg) { SET_MUL };	
static PyObject * AToDB_setAdd(AToDB *self, PyObject *arg) { SET_ADD };	
static PyObject * AToDB_setSub(AToDB *self, PyObject *arg) { SET_SUB };	
static PyObject * AToDB_setDiv(AToDB *self, PyObject *arg) { SET_DIV };	

static PyObject * AToDB_play(AToDB *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * AToDB_out(AToDB *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * AToDB_stop(AToDB *self) { STOP };

static PyObject * AToDB_multiply(AToDB *self, PyObject *arg) { MULTIPLY };
static PyObject * AToDB_inplace_multiply(AToDB *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * AToDB_add(AToDB *self, PyObject *arg) { ADD };
static PyObject * AToDB_inplace_add(AToDB *self, PyObject *arg) { INPLACE_ADD };
static PyObject * AToDB_sub(AToDB *self, PyObject *arg) { SUB };
static PyObject * AToDB_inplace_sub(AToDB *self, PyObject *arg) { INPLACE_SUB };
static PyObject * AToDB_div(AToDB *self, PyObject *arg) { DIV };
static PyObject * AToDB_inplace_div(AToDB *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef AToDB_members[] = {
    {"server", T_OBJECT_EX, offsetof(AToDB, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(AToDB, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(AToDB, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(AToDB, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(AToDB, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef AToDB_methods[] = {
    {"getServer", (PyCFunction)AToDB_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)AToDB_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)AToDB_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)AToDB_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)AToDB_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)AToDB_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)AToDB_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)AToDB_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)AToDB_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)AToDB_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods AToDB_as_number = {
    (binaryfunc)AToDB_add,                         /*nb_add*/
    (binaryfunc)AToDB_sub,                         /*nb_subtract*/
    (binaryfunc)AToDB_multiply,                    /*nb_multiply*/
    (binaryfunc)AToDB_div,                                              /*nb_divide*/
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
    (binaryfunc)AToDB_inplace_add,                 /*inplace_add*/
    (binaryfunc)AToDB_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)AToDB_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)AToDB_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject AToDBType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.AToDB_base",                                   /*tp_name*/
    sizeof(AToDB),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)AToDB_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &AToDB_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "AToDB objects. Converts dB value to amplitude value.",           /* tp_doc */
    (traverseproc)AToDB_traverse,                  /* tp_traverse */
    (inquiry)AToDB_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    AToDB_methods,                                 /* tp_methods */
    AToDB_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)AToDB_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    AToDB_new,                                     /* tp_new */
};
