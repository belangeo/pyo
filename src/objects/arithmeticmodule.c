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
/* M_Sin */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Sin;

static void
M_Sin_process(M_Sin *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = sinf(in[i]);
    }
}

static void M_Sin_postprocessing_ii(M_Sin *self) { POST_PROCESSING_II };
static void M_Sin_postprocessing_ai(M_Sin *self) { POST_PROCESSING_AI };
static void M_Sin_postprocessing_ia(M_Sin *self) { POST_PROCESSING_IA };
static void M_Sin_postprocessing_aa(M_Sin *self) { POST_PROCESSING_AA };
static void M_Sin_postprocessing_ireva(M_Sin *self) { POST_PROCESSING_IREVA };
static void M_Sin_postprocessing_areva(M_Sin *self) { POST_PROCESSING_AREVA };
static void M_Sin_postprocessing_revai(M_Sin *self) { POST_PROCESSING_REVAI };
static void M_Sin_postprocessing_revaa(M_Sin *self) { POST_PROCESSING_REVAA };
static void M_Sin_postprocessing_revareva(M_Sin *self) { POST_PROCESSING_REVAREVA };

static void
M_Sin_setProcMode(M_Sin *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Sin_process;
 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Sin_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Sin_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Sin_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Sin_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Sin_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Sin_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Sin_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Sin_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Sin_postprocessing_revareva;
            break;
    }   
}

static void
M_Sin_compute_next_data_frame(M_Sin *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Sin_traverse(M_Sin *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Sin_clear(M_Sin *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Sin_dealloc(M_Sin* self)
{
    free(self->data);
    M_Sin_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Sin_deleteStream(M_Sin *self) { DELETE_STREAM };

static PyObject *
M_Sin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Sin *self;
    self = (M_Sin *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Sin_compute_next_data_frame);
    self->mode_func_ptr = M_Sin_setProcMode;
    return (PyObject *)self;
}

static int
M_Sin_init(M_Sin *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    M_Sin_compute_next_data_frame((M_Sin *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Sin_getServer(M_Sin* self) { GET_SERVER };
static PyObject * M_Sin_getStream(M_Sin* self) { GET_STREAM };
static PyObject * M_Sin_setMul(M_Sin *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Sin_setAdd(M_Sin *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Sin_setSub(M_Sin *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Sin_setDiv(M_Sin *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Sin_play(M_Sin *self) { PLAY };
static PyObject * M_Sin_out(M_Sin *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Sin_stop(M_Sin *self) { STOP };

static PyObject * M_Sin_multiply(M_Sin *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Sin_inplace_multiply(M_Sin *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Sin_add(M_Sin *self, PyObject *arg) { ADD };
static PyObject * M_Sin_inplace_add(M_Sin *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Sin_sub(M_Sin *self, PyObject *arg) { SUB };
static PyObject * M_Sin_inplace_sub(M_Sin *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Sin_div(M_Sin *self, PyObject *arg) { DIV };
static PyObject * M_Sin_inplace_div(M_Sin *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Sin_members[] = {
{"server", T_OBJECT_EX, offsetof(M_Sin, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(M_Sin, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(M_Sin, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(M_Sin, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(M_Sin, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef M_Sin_methods[] = {
{"getServer", (PyCFunction)M_Sin_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)M_Sin_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)M_Sin_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)M_Sin_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)M_Sin_stop, METH_NOARGS, "Stops computing."},
{"out", (PyCFunction)M_Sin_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"setMul", (PyCFunction)M_Sin_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)M_Sin_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)M_Sin_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)M_Sin_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods M_Sin_as_number = {
(binaryfunc)M_Sin_add,                         /*nb_add*/
(binaryfunc)M_Sin_sub,                         /*nb_subtract*/
(binaryfunc)M_Sin_multiply,                    /*nb_multiply*/
(binaryfunc)M_Sin_div,                                              /*nb_divide*/
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
(binaryfunc)M_Sin_inplace_add,                 /*inplace_add*/
(binaryfunc)M_Sin_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)M_Sin_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)M_Sin_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_SinType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.M_Sin_base",                                   /*tp_name*/
sizeof(M_Sin),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)M_Sin_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&M_Sin_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"M_Sin objects. Performs sin function on audio samples.",           /* tp_doc */
(traverseproc)M_Sin_traverse,                  /* tp_traverse */
(inquiry)M_Sin_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
M_Sin_methods,                                 /* tp_methods */
M_Sin_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)M_Sin_init,                          /* tp_init */
0,                                              /* tp_alloc */
M_Sin_new,                                     /* tp_new */
};

/************/
/* M_Cos */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Cos;

static void
M_Cos_process(M_Cos *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = cosf(in[i]);
    }
}

static void M_Cos_postprocessing_ii(M_Cos *self) { POST_PROCESSING_II };
static void M_Cos_postprocessing_ai(M_Cos *self) { POST_PROCESSING_AI };
static void M_Cos_postprocessing_ia(M_Cos *self) { POST_PROCESSING_IA };
static void M_Cos_postprocessing_aa(M_Cos *self) { POST_PROCESSING_AA };
static void M_Cos_postprocessing_ireva(M_Cos *self) { POST_PROCESSING_IREVA };
static void M_Cos_postprocessing_areva(M_Cos *self) { POST_PROCESSING_AREVA };
static void M_Cos_postprocessing_revai(M_Cos *self) { POST_PROCESSING_REVAI };
static void M_Cos_postprocessing_revaa(M_Cos *self) { POST_PROCESSING_REVAA };
static void M_Cos_postprocessing_revareva(M_Cos *self) { POST_PROCESSING_REVAREVA };

static void
M_Cos_setProcMode(M_Cos *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Cos_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Cos_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Cos_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Cos_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Cos_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Cos_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Cos_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Cos_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Cos_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Cos_postprocessing_revareva;
            break;
    }   
}

static void
M_Cos_compute_next_data_frame(M_Cos *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Cos_traverse(M_Cos *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Cos_clear(M_Cos *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Cos_dealloc(M_Cos* self)
{
    free(self->data);
    M_Cos_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Cos_deleteStream(M_Cos *self) { DELETE_STREAM };

static PyObject *
M_Cos_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Cos *self;
    self = (M_Cos *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Cos_compute_next_data_frame);
    self->mode_func_ptr = M_Cos_setProcMode;
    return (PyObject *)self;
}

static int
M_Cos_init(M_Cos *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    M_Cos_compute_next_data_frame((M_Cos *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Cos_getServer(M_Cos* self) { GET_SERVER };
static PyObject * M_Cos_getStream(M_Cos* self) { GET_STREAM };
static PyObject * M_Cos_setMul(M_Cos *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Cos_setAdd(M_Cos *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Cos_setSub(M_Cos *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Cos_setDiv(M_Cos *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Cos_play(M_Cos *self) { PLAY };
static PyObject * M_Cos_out(M_Cos *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Cos_stop(M_Cos *self) { STOP };

static PyObject * M_Cos_multiply(M_Cos *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Cos_inplace_multiply(M_Cos *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Cos_add(M_Cos *self, PyObject *arg) { ADD };
static PyObject * M_Cos_inplace_add(M_Cos *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Cos_sub(M_Cos *self, PyObject *arg) { SUB };
static PyObject * M_Cos_inplace_sub(M_Cos *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Cos_div(M_Cos *self, PyObject *arg) { DIV };
static PyObject * M_Cos_inplace_div(M_Cos *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Cos_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Cos, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Cos, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Cos, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Cos, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Cos, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Cos_methods[] = {
    {"getServer", (PyCFunction)M_Cos_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Cos_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Cos_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Cos_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Cos_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Cos_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Cos_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Cos_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Cos_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Cos_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Cos_as_number = {
    (binaryfunc)M_Cos_add,                         /*nb_add*/
    (binaryfunc)M_Cos_sub,                         /*nb_subtract*/
    (binaryfunc)M_Cos_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Cos_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Cos_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Cos_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Cos_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Cos_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_CosType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Cos_base",                                   /*tp_name*/
    sizeof(M_Cos),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Cos_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Cos_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Cos objects. Performs sin function on audio samples.",           /* tp_doc */
    (traverseproc)M_Cos_traverse,                  /* tp_traverse */
    (inquiry)M_Cos_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Cos_methods,                                 /* tp_methods */
    M_Cos_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Cos_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Cos_new,                                     /* tp_new */
};

/************/
/* M_Tan */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Tan;

static void
M_Tan_process(M_Tan *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tanf(in[i]);
    }
}

static void M_Tan_postprocessing_ii(M_Tan *self) { POST_PROCESSING_II };
static void M_Tan_postprocessing_ai(M_Tan *self) { POST_PROCESSING_AI };
static void M_Tan_postprocessing_ia(M_Tan *self) { POST_PROCESSING_IA };
static void M_Tan_postprocessing_aa(M_Tan *self) { POST_PROCESSING_AA };
static void M_Tan_postprocessing_ireva(M_Tan *self) { POST_PROCESSING_IREVA };
static void M_Tan_postprocessing_areva(M_Tan *self) { POST_PROCESSING_AREVA };
static void M_Tan_postprocessing_revai(M_Tan *self) { POST_PROCESSING_REVAI };
static void M_Tan_postprocessing_revaa(M_Tan *self) { POST_PROCESSING_REVAA };
static void M_Tan_postprocessing_revareva(M_Tan *self) { POST_PROCESSING_REVAREVA };

static void
M_Tan_setProcMode(M_Tan *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Tan_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Tan_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Tan_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Tan_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Tan_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Tan_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Tan_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Tan_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Tan_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Tan_postprocessing_revareva;
            break;
    }   
}

static void
M_Tan_compute_next_data_frame(M_Tan *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Tan_traverse(M_Tan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Tan_clear(M_Tan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Tan_dealloc(M_Tan* self)
{
    free(self->data);
    M_Tan_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Tan_deleteStream(M_Tan *self) { DELETE_STREAM };

static PyObject *
M_Tan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Tan *self;
    self = (M_Tan *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Tan_compute_next_data_frame);
    self->mode_func_ptr = M_Tan_setProcMode;
    return (PyObject *)self;
}

static int
M_Tan_init(M_Tan *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    M_Tan_compute_next_data_frame((M_Tan *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Tan_getServer(M_Tan* self) { GET_SERVER };
static PyObject * M_Tan_getStream(M_Tan* self) { GET_STREAM };
static PyObject * M_Tan_setMul(M_Tan *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Tan_setAdd(M_Tan *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Tan_setSub(M_Tan *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Tan_setDiv(M_Tan *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Tan_play(M_Tan *self) { PLAY };
static PyObject * M_Tan_out(M_Tan *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Tan_stop(M_Tan *self) { STOP };

static PyObject * M_Tan_multiply(M_Tan *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Tan_inplace_multiply(M_Tan *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Tan_add(M_Tan *self, PyObject *arg) { ADD };
static PyObject * M_Tan_inplace_add(M_Tan *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Tan_sub(M_Tan *self, PyObject *arg) { SUB };
static PyObject * M_Tan_inplace_sub(M_Tan *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Tan_div(M_Tan *self, PyObject *arg) { DIV };
static PyObject * M_Tan_inplace_div(M_Tan *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Tan_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Tan, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Tan, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Tan, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Tan, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Tan, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Tan_methods[] = {
    {"getServer", (PyCFunction)M_Tan_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Tan_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Tan_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Tan_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Tan_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Tan_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Tan_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Tan_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Tan_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Tan_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Tan_as_number = {
    (binaryfunc)M_Tan_add,                         /*nb_add*/
    (binaryfunc)M_Tan_sub,                         /*nb_subtract*/
    (binaryfunc)M_Tan_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Tan_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Tan_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Tan_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Tan_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Tan_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_TanType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Tan_base",                                   /*tp_name*/
    sizeof(M_Tan),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Tan_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Tan_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Tan objects. Performs sin function on audio samples.",           /* tp_doc */
    (traverseproc)M_Tan_traverse,                  /* tp_traverse */
    (inquiry)M_Tan_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Tan_methods,                                 /* tp_methods */
    M_Tan_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Tan_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Tan_new,                                     /* tp_new */
};
