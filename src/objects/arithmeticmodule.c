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
{"out", (PyCFunction)M_Sin_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
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
    {"out", (PyCFunction)M_Cos_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
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
    {"out", (PyCFunction)M_Tan_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
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

/************/
/* M_Abs */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Abs;

static void
M_Abs_process(M_Abs *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < 0.0)
            self->data[i] = -inval;
        else 
            self->data[i] = inval;
    }
}

static void M_Abs_postprocessing_ii(M_Abs *self) { POST_PROCESSING_II };
static void M_Abs_postprocessing_ai(M_Abs *self) { POST_PROCESSING_AI };
static void M_Abs_postprocessing_ia(M_Abs *self) { POST_PROCESSING_IA };
static void M_Abs_postprocessing_aa(M_Abs *self) { POST_PROCESSING_AA };
static void M_Abs_postprocessing_ireva(M_Abs *self) { POST_PROCESSING_IREVA };
static void M_Abs_postprocessing_areva(M_Abs *self) { POST_PROCESSING_AREVA };
static void M_Abs_postprocessing_revai(M_Abs *self) { POST_PROCESSING_REVAI };
static void M_Abs_postprocessing_revaa(M_Abs *self) { POST_PROCESSING_REVAA };
static void M_Abs_postprocessing_revareva(M_Abs *self) { POST_PROCESSING_REVAREVA };

static void
M_Abs_setProcMode(M_Abs *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Abs_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Abs_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Abs_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Abs_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Abs_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Abs_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Abs_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Abs_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Abs_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Abs_postprocessing_revareva;
            break;
    }   
}

static void
M_Abs_compute_next_data_frame(M_Abs *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Abs_traverse(M_Abs *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Abs_clear(M_Abs *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Abs_dealloc(M_Abs* self)
{
    free(self->data);
    M_Abs_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Abs_deleteStream(M_Abs *self) { DELETE_STREAM };

static PyObject *
M_Abs_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Abs *self;
    self = (M_Abs *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Abs_compute_next_data_frame);
    self->mode_func_ptr = M_Abs_setProcMode;
    return (PyObject *)self;
}

static int
M_Abs_init(M_Abs *self, PyObject *args, PyObject *kwds)
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
    
    M_Abs_compute_next_data_frame((M_Abs *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Abs_getServer(M_Abs* self) { GET_SERVER };
static PyObject * M_Abs_getStream(M_Abs* self) { GET_STREAM };
static PyObject * M_Abs_setMul(M_Abs *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Abs_setAdd(M_Abs *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Abs_setSub(M_Abs *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Abs_setDiv(M_Abs *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Abs_play(M_Abs *self) { PLAY };
static PyObject * M_Abs_out(M_Abs *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Abs_stop(M_Abs *self) { STOP };

static PyObject * M_Abs_multiply(M_Abs *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Abs_inplace_multiply(M_Abs *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Abs_add(M_Abs *self, PyObject *arg) { ADD };
static PyObject * M_Abs_inplace_add(M_Abs *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Abs_sub(M_Abs *self, PyObject *arg) { SUB };
static PyObject * M_Abs_inplace_sub(M_Abs *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Abs_div(M_Abs *self, PyObject *arg) { DIV };
static PyObject * M_Abs_inplace_div(M_Abs *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Abs_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Abs, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Abs, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Abs, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Abs, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Abs, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Abs_methods[] = {
    {"getServer", (PyCFunction)M_Abs_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Abs_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Abs_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Abs_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Abs_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Abs_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Abs_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Abs_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Abs_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Abs_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Abs_as_number = {
    (binaryfunc)M_Abs_add,                         /*nb_add*/
    (binaryfunc)M_Abs_sub,                         /*nb_subtract*/
    (binaryfunc)M_Abs_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Abs_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Abs_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Abs_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Abs_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Abs_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_AbsType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Abs_base",                                   /*tp_name*/
    sizeof(M_Abs),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Abs_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Abs_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Abs objects. Performs abs function on audio samples.",           /* tp_doc */
    (traverseproc)M_Abs_traverse,                  /* tp_traverse */
    (inquiry)M_Abs_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Abs_methods,                                 /* tp_methods */
    M_Abs_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Abs_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Abs_new,                                     /* tp_new */
};

/************/
/* M_Sqrt */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Sqrt;

static void
M_Sqrt_process(M_Sqrt *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval < 0.0)
            self->data[i] = 0.0;
        else 
            self->data[i] = sqrtf(inval);
    }
}

static void M_Sqrt_postprocessing_ii(M_Sqrt *self) { POST_PROCESSING_II };
static void M_Sqrt_postprocessing_ai(M_Sqrt *self) { POST_PROCESSING_AI };
static void M_Sqrt_postprocessing_ia(M_Sqrt *self) { POST_PROCESSING_IA };
static void M_Sqrt_postprocessing_aa(M_Sqrt *self) { POST_PROCESSING_AA };
static void M_Sqrt_postprocessing_ireva(M_Sqrt *self) { POST_PROCESSING_IREVA };
static void M_Sqrt_postprocessing_areva(M_Sqrt *self) { POST_PROCESSING_AREVA };
static void M_Sqrt_postprocessing_revai(M_Sqrt *self) { POST_PROCESSING_REVAI };
static void M_Sqrt_postprocessing_revaa(M_Sqrt *self) { POST_PROCESSING_REVAA };
static void M_Sqrt_postprocessing_revareva(M_Sqrt *self) { POST_PROCESSING_REVAREVA };

static void
M_Sqrt_setProcMode(M_Sqrt *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Sqrt_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Sqrt_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Sqrt_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Sqrt_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Sqrt_postprocessing_revareva;
            break;
    }   
}

static void
M_Sqrt_compute_next_data_frame(M_Sqrt *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Sqrt_traverse(M_Sqrt *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Sqrt_clear(M_Sqrt *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Sqrt_dealloc(M_Sqrt* self)
{
    free(self->data);
    M_Sqrt_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Sqrt_deleteStream(M_Sqrt *self) { DELETE_STREAM };

static PyObject *
M_Sqrt_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Sqrt *self;
    self = (M_Sqrt *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Sqrt_compute_next_data_frame);
    self->mode_func_ptr = M_Sqrt_setProcMode;
    return (PyObject *)self;
}

static int
M_Sqrt_init(M_Sqrt *self, PyObject *args, PyObject *kwds)
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
    
    M_Sqrt_compute_next_data_frame((M_Sqrt *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Sqrt_getServer(M_Sqrt* self) { GET_SERVER };
static PyObject * M_Sqrt_getStream(M_Sqrt* self) { GET_STREAM };
static PyObject * M_Sqrt_setMul(M_Sqrt *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Sqrt_setAdd(M_Sqrt *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Sqrt_setSub(M_Sqrt *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Sqrt_setDiv(M_Sqrt *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Sqrt_play(M_Sqrt *self) { PLAY };
static PyObject * M_Sqrt_out(M_Sqrt *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Sqrt_stop(M_Sqrt *self) { STOP };

static PyObject * M_Sqrt_multiply(M_Sqrt *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Sqrt_inplace_multiply(M_Sqrt *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Sqrt_add(M_Sqrt *self, PyObject *arg) { ADD };
static PyObject * M_Sqrt_inplace_add(M_Sqrt *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Sqrt_sub(M_Sqrt *self, PyObject *arg) { SUB };
static PyObject * M_Sqrt_inplace_sub(M_Sqrt *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Sqrt_div(M_Sqrt *self, PyObject *arg) { DIV };
static PyObject * M_Sqrt_inplace_div(M_Sqrt *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Sqrt_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Sqrt, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Sqrt, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Sqrt, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Sqrt, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Sqrt, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Sqrt_methods[] = {
    {"getServer", (PyCFunction)M_Sqrt_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Sqrt_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Sqrt_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Sqrt_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Sqrt_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Sqrt_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Sqrt_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Sqrt_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Sqrt_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Sqrt_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Sqrt_as_number = {
    (binaryfunc)M_Sqrt_add,                         /*nb_add*/
    (binaryfunc)M_Sqrt_sub,                         /*nb_subtract*/
    (binaryfunc)M_Sqrt_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Sqrt_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Sqrt_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Sqrt_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Sqrt_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Sqrt_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_SqrtType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Sqrt_base",                                   /*tp_name*/
    sizeof(M_Sqrt),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Sqrt_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Sqrt_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Sqrt objects. Performs sqrt function on audio samples.",           /* tp_doc */
    (traverseproc)M_Sqrt_traverse,                  /* tp_traverse */
    (inquiry)M_Sqrt_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Sqrt_methods,                                 /* tp_methods */
    M_Sqrt_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Sqrt_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Sqrt_new,                                     /* tp_new */
};

/************/
/* M_Log */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log;

static void
M_Log_process(M_Log *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else 
            self->data[i] = logf(inval);
    }
}

static void M_Log_postprocessing_ii(M_Log *self) { POST_PROCESSING_II };
static void M_Log_postprocessing_ai(M_Log *self) { POST_PROCESSING_AI };
static void M_Log_postprocessing_ia(M_Log *self) { POST_PROCESSING_IA };
static void M_Log_postprocessing_aa(M_Log *self) { POST_PROCESSING_AA };
static void M_Log_postprocessing_ireva(M_Log *self) { POST_PROCESSING_IREVA };
static void M_Log_postprocessing_areva(M_Log *self) { POST_PROCESSING_AREVA };
static void M_Log_postprocessing_revai(M_Log *self) { POST_PROCESSING_REVAI };
static void M_Log_postprocessing_revaa(M_Log *self) { POST_PROCESSING_REVAA };
static void M_Log_postprocessing_revareva(M_Log *self) { POST_PROCESSING_REVAREVA };

static void
M_Log_setProcMode(M_Log *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Log_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Log_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Log_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Log_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Log_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Log_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Log_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Log_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Log_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Log_postprocessing_revareva;
            break;
    }   
}

static void
M_Log_compute_next_data_frame(M_Log *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Log_traverse(M_Log *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Log_clear(M_Log *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log_dealloc(M_Log* self)
{
    free(self->data);
    M_Log_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Log_deleteStream(M_Log *self) { DELETE_STREAM };

static PyObject *
M_Log_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Log *self;
    self = (M_Log *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log_compute_next_data_frame);
    self->mode_func_ptr = M_Log_setProcMode;
    return (PyObject *)self;
}

static int
M_Log_init(M_Log *self, PyObject *args, PyObject *kwds)
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
    
    M_Log_compute_next_data_frame((M_Log *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Log_getServer(M_Log* self) { GET_SERVER };
static PyObject * M_Log_getStream(M_Log* self) { GET_STREAM };
static PyObject * M_Log_setMul(M_Log *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Log_setAdd(M_Log *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Log_setSub(M_Log *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Log_setDiv(M_Log *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Log_play(M_Log *self) { PLAY };
static PyObject * M_Log_out(M_Log *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log_stop(M_Log *self) { STOP };

static PyObject * M_Log_multiply(M_Log *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log_inplace_multiply(M_Log *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log_add(M_Log *self, PyObject *arg) { ADD };
static PyObject * M_Log_inplace_add(M_Log *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log_sub(M_Log *self, PyObject *arg) { SUB };
static PyObject * M_Log_inplace_sub(M_Log *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log_div(M_Log *self, PyObject *arg) { DIV };
static PyObject * M_Log_inplace_div(M_Log *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log_methods[] = {
    {"getServer", (PyCFunction)M_Log_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Log_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Log_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log_as_number = {
    (binaryfunc)M_Log_add,                         /*nb_add*/
    (binaryfunc)M_Log_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_LogType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log_base",                                   /*tp_name*/
    sizeof(M_Log),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log objects. Performs natural log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log_traverse,                  /* tp_traverse */
    (inquiry)M_Log_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log_methods,                                 /* tp_methods */
    M_Log_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Log_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log_new,                                     /* tp_new */
};

/************/
/* M_Log10 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log10;

static void
M_Log10_process(M_Log10 *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else 
            self->data[i] = log10f(inval);
    }
}

static void M_Log10_postprocessing_ii(M_Log10 *self) { POST_PROCESSING_II };
static void M_Log10_postprocessing_ai(M_Log10 *self) { POST_PROCESSING_AI };
static void M_Log10_postprocessing_ia(M_Log10 *self) { POST_PROCESSING_IA };
static void M_Log10_postprocessing_aa(M_Log10 *self) { POST_PROCESSING_AA };
static void M_Log10_postprocessing_ireva(M_Log10 *self) { POST_PROCESSING_IREVA };
static void M_Log10_postprocessing_areva(M_Log10 *self) { POST_PROCESSING_AREVA };
static void M_Log10_postprocessing_revai(M_Log10 *self) { POST_PROCESSING_REVAI };
static void M_Log10_postprocessing_revaa(M_Log10 *self) { POST_PROCESSING_REVAA };
static void M_Log10_postprocessing_revareva(M_Log10 *self) { POST_PROCESSING_REVAREVA };

static void
M_Log10_setProcMode(M_Log10 *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Log10_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Log10_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Log10_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Log10_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Log10_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Log10_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Log10_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Log10_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Log10_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Log10_postprocessing_revareva;
            break;
    }   
}

static void
M_Log10_compute_next_data_frame(M_Log10 *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Log10_traverse(M_Log10 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Log10_clear(M_Log10 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log10_dealloc(M_Log10* self)
{
    free(self->data);
    M_Log10_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Log10_deleteStream(M_Log10 *self) { DELETE_STREAM };

static PyObject *
M_Log10_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Log10 *self;
    self = (M_Log10 *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log10_compute_next_data_frame);
    self->mode_func_ptr = M_Log10_setProcMode;
    return (PyObject *)self;
}

static int
M_Log10_init(M_Log10 *self, PyObject *args, PyObject *kwds)
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
    
    M_Log10_compute_next_data_frame((M_Log10 *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Log10_getServer(M_Log10* self) { GET_SERVER };
static PyObject * M_Log10_getStream(M_Log10* self) { GET_STREAM };
static PyObject * M_Log10_setMul(M_Log10 *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Log10_setAdd(M_Log10 *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Log10_setSub(M_Log10 *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Log10_setDiv(M_Log10 *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Log10_play(M_Log10 *self) { PLAY };
static PyObject * M_Log10_out(M_Log10 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log10_stop(M_Log10 *self) { STOP };

static PyObject * M_Log10_multiply(M_Log10 *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log10_inplace_multiply(M_Log10 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log10_add(M_Log10 *self, PyObject *arg) { ADD };
static PyObject * M_Log10_inplace_add(M_Log10 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log10_sub(M_Log10 *self, PyObject *arg) { SUB };
static PyObject * M_Log10_inplace_sub(M_Log10 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log10_div(M_Log10 *self, PyObject *arg) { DIV };
static PyObject * M_Log10_inplace_div(M_Log10 *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log10_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log10, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log10, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log10, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log10, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log10, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log10_methods[] = {
    {"getServer", (PyCFunction)M_Log10_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log10_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Log10_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Log10_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log10_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log10_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log10_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log10_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log10_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log10_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log10_as_number = {
    (binaryfunc)M_Log10_add,                         /*nb_add*/
    (binaryfunc)M_Log10_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log10_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log10_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log10_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log10_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log10_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log10_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_Log10Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log10_base",                                   /*tp_name*/
    sizeof(M_Log10),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log10_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log10_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log10 objects. Performs base 10 log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log10_traverse,                  /* tp_traverse */
    (inquiry)M_Log10_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log10_methods,                                 /* tp_methods */
    M_Log10_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Log10_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log10_new,                                     /* tp_new */
};

/************/
/* M_Log2 */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
} M_Log2;

static void
M_Log2_process(M_Log2 *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval <= 0.0)
            self->data[i] = 0.0;
        else 
            self->data[i] = log2f(inval);
    }
}

static void M_Log2_postprocessing_ii(M_Log2 *self) { POST_PROCESSING_II };
static void M_Log2_postprocessing_ai(M_Log2 *self) { POST_PROCESSING_AI };
static void M_Log2_postprocessing_ia(M_Log2 *self) { POST_PROCESSING_IA };
static void M_Log2_postprocessing_aa(M_Log2 *self) { POST_PROCESSING_AA };
static void M_Log2_postprocessing_ireva(M_Log2 *self) { POST_PROCESSING_IREVA };
static void M_Log2_postprocessing_areva(M_Log2 *self) { POST_PROCESSING_AREVA };
static void M_Log2_postprocessing_revai(M_Log2 *self) { POST_PROCESSING_REVAI };
static void M_Log2_postprocessing_revaa(M_Log2 *self) { POST_PROCESSING_REVAA };
static void M_Log2_postprocessing_revareva(M_Log2 *self) { POST_PROCESSING_REVAREVA };

static void
M_Log2_setProcMode(M_Log2 *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = M_Log2_process;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = M_Log2_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = M_Log2_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = M_Log2_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = M_Log2_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = M_Log2_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = M_Log2_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = M_Log2_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = M_Log2_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = M_Log2_postprocessing_revareva;
            break;
    }   
}

static void
M_Log2_compute_next_data_frame(M_Log2 *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
M_Log2_traverse(M_Log2 *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
M_Log2_clear(M_Log2 *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
M_Log2_dealloc(M_Log2* self)
{
    free(self->data);
    M_Log2_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * M_Log2_deleteStream(M_Log2 *self) { DELETE_STREAM };

static PyObject *
M_Log2_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    M_Log2 *self;
    self = (M_Log2 *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, M_Log2_compute_next_data_frame);
    self->mode_func_ptr = M_Log2_setProcMode;
    return (PyObject *)self;
}

static int
M_Log2_init(M_Log2 *self, PyObject *args, PyObject *kwds)
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
    
    M_Log2_compute_next_data_frame((M_Log2 *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * M_Log2_getServer(M_Log2* self) { GET_SERVER };
static PyObject * M_Log2_getStream(M_Log2* self) { GET_STREAM };
static PyObject * M_Log2_setMul(M_Log2 *self, PyObject *arg) { SET_MUL };	
static PyObject * M_Log2_setAdd(M_Log2 *self, PyObject *arg) { SET_ADD };	
static PyObject * M_Log2_setSub(M_Log2 *self, PyObject *arg) { SET_SUB };	
static PyObject * M_Log2_setDiv(M_Log2 *self, PyObject *arg) { SET_DIV };	

static PyObject * M_Log2_play(M_Log2 *self) { PLAY };
static PyObject * M_Log2_out(M_Log2 *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * M_Log2_stop(M_Log2 *self) { STOP };

static PyObject * M_Log2_multiply(M_Log2 *self, PyObject *arg) { MULTIPLY };
static PyObject * M_Log2_inplace_multiply(M_Log2 *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * M_Log2_add(M_Log2 *self, PyObject *arg) { ADD };
static PyObject * M_Log2_inplace_add(M_Log2 *self, PyObject *arg) { INPLACE_ADD };
static PyObject * M_Log2_sub(M_Log2 *self, PyObject *arg) { SUB };
static PyObject * M_Log2_inplace_sub(M_Log2 *self, PyObject *arg) { INPLACE_SUB };
static PyObject * M_Log2_div(M_Log2 *self, PyObject *arg) { DIV };
static PyObject * M_Log2_inplace_div(M_Log2 *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef M_Log2_members[] = {
    {"server", T_OBJECT_EX, offsetof(M_Log2, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(M_Log2, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(M_Log2, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(M_Log2, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(M_Log2, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef M_Log2_methods[] = {
    {"getServer", (PyCFunction)M_Log2_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)M_Log2_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)M_Log2_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)M_Log2_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)M_Log2_stop, METH_NOARGS, "Stops computing."},
    {"out", (PyCFunction)M_Log2_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setMul", (PyCFunction)M_Log2_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)M_Log2_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)M_Log2_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)M_Log2_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods M_Log2_as_number = {
    (binaryfunc)M_Log2_add,                         /*nb_add*/
    (binaryfunc)M_Log2_sub,                         /*nb_subtract*/
    (binaryfunc)M_Log2_multiply,                    /*nb_multiply*/
    (binaryfunc)M_Log2_div,                                              /*nb_divide*/
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
    (binaryfunc)M_Log2_inplace_add,                 /*inplace_add*/
    (binaryfunc)M_Log2_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)M_Log2_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)M_Log2_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject M_Log2Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.M_Log2_base",                                   /*tp_name*/
    sizeof(M_Log2),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)M_Log2_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &M_Log2_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "M_Log2 objects. Performs base 2 log function on audio samples.",           /* tp_doc */
    (traverseproc)M_Log2_traverse,                  /* tp_traverse */
    (inquiry)M_Log2_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    M_Log2_methods,                                 /* tp_methods */
    M_Log2_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)M_Log2_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    M_Log2_new,                                     /* tp_new */
};
