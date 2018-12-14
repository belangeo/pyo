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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

/*****************************************************
Template for a simple audio fx processor.
*****************************************************/

/*****************************************************
Gain struct. Here comes every attributes required 
for the proper operation of the object.
*****************************************************/
typedef struct {
    /* Mandatory macro intializing common properties of PyoObjects */
    pyo_audio_HEAD
    /* modebuffer keeps trace of the type of attributes that can 
    be float or PyoObject. Processing function is selected to 
    optimize operations. Need at least 2 slots for mul & add. */
    int modebuffer[3];
    /* Object's attributes. Attribute that can be float or PyoObject
    must be defined as PyObject and must be coupled with a Stream
    object (to hold the vector of samples of the audio signal). */ 
    PyObject *input;
    Stream *input_stream;
    PyObject *db;
    Stream *db_stream;
    /* Floating-point values must always use the MYFLT macro which 
    handle float vs double builds. */
    MYFLT last_db;
    MYFLT gain_factor;
} Gain;

/**********************************************************************
Processing functions. The last letter(s) of the name indicate the type 
of object assigned to PyObject attribute, in this case"db" attribute. 
"i" is for floating-point value and "a" is for audio signal. 
**********************************************************************/
static void
Gain_process_i(Gain *self) {
    int i;
    /* Get the input signal stream */
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    /* Get the "db" attribute as a single value */
    MYFLT db = PyFloat_AS_DOUBLE(self->db);
    if (db <= -120.0)
        db = -120;
    if (db != self->last_db) {
        self->last_db = db;
        self->gain_factor = MYPOW(10.0, db * 0.05);
    }
    /* Process vector of samples of size self->bufsize ("buffersize"
    attribute of the server). self->data, created in "pyo_audio_HEAD"
    macro, is the vector of samples that is exposed to other objects. */
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i] * self->gain_factor;
    }
}

static void
Gain_process_a(Gain *self) {
    int i;
    MYFLT db;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    /* Get the "db" attribute as a vector of samples */
    MYFLT *db_st = Stream_getData((Stream *)self->db_stream);
        
    for (i=0; i<self->bufsize; i++) {
        db = db_st[i];
        if (db <= -120.0)
            db = -120;
        if (db != self->last_db) {
            self->last_db = db;
            self->gain_factor = MYPOW(10.0, db * 0.05);
        }
        self->data[i] = in[i] * self->gain_factor;
    }
}
/**********************************************************************
End of processing function definitions.
**********************************************************************/

/**********************************************************************
Post-Processing functions. These are functions where are applied "mul"
and "add" attributes. Macros are defined in pyomodule.h. Just keep them
and change the object's name. 
**********************************************************************/
static void Gain_postprocessing_ii(Gain *self) { POST_PROCESSING_II };
static void Gain_postprocessing_ai(Gain *self) { POST_PROCESSING_AI };
static void Gain_postprocessing_ia(Gain *self) { POST_PROCESSING_IA };
static void Gain_postprocessing_aa(Gain *self) { POST_PROCESSING_AA };
static void Gain_postprocessing_ireva(Gain *self) { POST_PROCESSING_IREVA };
static void Gain_postprocessing_areva(Gain *self) { POST_PROCESSING_AREVA };
static void Gain_postprocessing_revai(Gain *self) { POST_PROCESSING_REVAI };
static void Gain_postprocessing_revaa(Gain *self) { POST_PROCESSING_REVAA };
static void Gain_postprocessing_revareva(Gain *self) { POST_PROCESSING_REVAREVA };

/**********************************************************************
setProcMode is called everytime a new value is assigned to one of the
object's attributes. Here are specified pointers to processing and 
post-processing functions according to attribute types. 
**********************************************************************/
static void
Gain_setProcMode(Gain *self)
{
    int procmode, muladdmode;
    /* If there are more than one attribute, sum them here and add
    cases to the switch statement, on "procmode", below. */
    procmode = self->modebuffer[2];
    /* "muladdmode" swith statement should be left as is. */
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    switch (procmode) {
        case 0:    
            self->proc_func_ptr = Gain_process_i;
            break;
        case 1:    
            self->proc_func_ptr = Gain_process_a;
            break;
    } 
    switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Gain_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Gain_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Gain_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Gain_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Gain_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Gain_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Gain_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Gain_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Gain_postprocessing_revareva;
            break;
    }   
}

/**********************************************************************
"compute_next_data_frame" is the function called by the server on each
processing loop. Processing function are executed first and then, the
post-processing function is called. 
**********************************************************************/
static void
Gain_compute_next_data_frame(Gain *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

/**********************************************************************
Garbage-collector. Every PyObject and Stream objects must be added to
"traverse" and "clear" functions to allow the interpreter to clean memory 
when ref count drop to zero. These functions must be registered as 
"tp_traverse" and "tp_clear" in the object's PyTypeObject structure below.
Py_TPFLAGS_HAVE_GC flag must be added to the class flags (tp_flags).
**********************************************************************/
static int
Gain_traverse(Gain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->db);    
    Py_VISIT(self->db_stream);    
    return 0;
}

static int 
Gain_clear(Gain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->db);    
    Py_CLEAR(self->db_stream);    
    return 0;
}

/**********************************************************************
Deallocation function, registered as "tp_dealloc". Here must be freed
every malloc'ed and realloc'ed variables of the object.
**********************************************************************/
static void
Gain_dealloc(Gain* self)
{
    pyo_DEALLOC
    Gain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

/**********************************************************************
Function called at the object creation, registered as "tp_new".
**********************************************************************/
static PyObject *
Gain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *dbtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    /* Object's allocation */
    Gain *self;
    self = (Gain *)type->tp_alloc(type, 0);
    
    /* Initialization of object's attributes */
    self->db = PyFloat_FromDouble(0.5);
    self->last_db = -130.0;
    self->gain_factor = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    
    /* Initialization of common properties of PyoObject */
    INIT_OBJECT_COMMON

    /* Assign the stream's pointer to the object's processing callback. */
    /* The stream struct is what is registered in the server. */
    Stream_setFunctionPtr(self->stream, Gain_compute_next_data_frame);
    /* Assign setProcMode to the common mode_func_ptr pointer */
    self->mode_func_ptr = Gain_setProcMode;

    /* Object's keyword list. These are arguments to the object's creation */
    static char *kwlist[] = {"input", "db", "mul", "add", NULL};
    
    /* Argument parsing. If there is float values in the type list ("O|OOO"),
    a macro must be given to handle float vs double argument. See pyomodule.h
    for the list of macros already available. */
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &dbtmp, &multmp, &addtmp))
        Py_RETURN_NONE;

    /* This macro handles fx's audio input initialization */
    INIT_INPUT_STREAM
    
    /* If argument is present, called the corresponding setter function */
    if (dbtmp) {
        PyObject_CallMethod((PyObject *)self, "setDB", "O", dbtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    /* Add the object's stream struct to the server registry */
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    /* Call setProcMode to be sure pointers are correctly assigned */
    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

/**********************************************************************
Functions common to almost all PyoObjects. The macros are defined in
pyomodule.h. Sometime the "out" function is removed to prevent sending 
non-normalized signal (like the fft analysis of FFT object) to the soundcard. 
Object dependant initializations can be added before PLAY, OUT and STOP macros.
**********************************************************************/
static PyObject * Gain_getServer(Gain *self) { GET_SERVER };
static PyObject * Gain_getStream(Gain *self) { GET_STREAM };
static PyObject * Gain_setMul(Gain *self, PyObject *arg) { SET_MUL };    
static PyObject * Gain_setAdd(Gain *self, PyObject *arg) { SET_ADD };    
static PyObject * Gain_setSub(Gain *self, PyObject *arg) { SET_SUB };    
static PyObject * Gain_setDiv(Gain *self, PyObject *arg) { SET_DIV };    

static PyObject * Gain_play(Gain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Gain_out(Gain *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Gain_stop(Gain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Gain_multiply(Gain *self, PyObject *arg) { MULTIPLY };
static PyObject * Gain_inplace_multiply(Gain *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Gain_add(Gain *self, PyObject *arg) { ADD };
static PyObject * Gain_inplace_add(Gain *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Gain_sub(Gain *self, PyObject *arg) { SUB };
static PyObject * Gain_inplace_sub(Gain *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Gain_div(Gain *self, PyObject *arg) { DIV };
static PyObject * Gain_inplace_div(Gain *self, PyObject *arg) { INPLACE_DIV };

/**********************************************************************
Setter function for attributes. Here, we need to check if the value is
a float or an audio signal and adjust consequently its slot in the 
"modebuffer" array.
**********************************************************************/
static PyObject *
Gain_setDB(Gain *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;
    
    if (arg == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    
    int isNumber = PyNumber_Check(arg);
    
    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->db);
    /* If it's a numeric value */
    if (isNumber == 1) {
        self->db = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    /* If it's an audio signal */
    else {
        self->db = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->db, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->db_stream);
        self->db_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }
    
    /* Reassign processing function pointers */
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

/**********************************************************************
Object's members descriptors. Here should appear the server and stream 
refs and also every PyObjects declared in the object's struct.
Registered as "tp_members".
**********************************************************************/
static PyMemberDef Gain_members[] = {
{"server", T_OBJECT_EX, offsetof(Gain, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Gain, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Gain, input), 0, "Input sound object."},
{"db", T_OBJECT_EX, offsetof(Gain, db), 0, "Gain factor in decibel."},
{"mul", T_OBJECT_EX, offsetof(Gain, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Gain, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

/**********************************************************************
Object's method descriptors. Here should appear every methods needed to
be exposed to the python interpreter, casted to PyCFunction. 
Registered as "tp_methods".
**********************************************************************/
static PyMethodDef Gain_methods[] = {
{"getServer", (PyCFunction)Gain_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Gain_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Gain_play, METH_VARARGS|METH_KEYWORDS, "Starts dbuting without sending sound to soundcard."},
{"out", (PyCFunction)Gain_out, METH_VARARGS|METH_KEYWORDS, "Starts dbuting and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Gain_stop, METH_VARARGS|METH_KEYWORDS, "Stops dbuting."},
{"setDB", (PyCFunction)Gain_setDB, METH_O, "Sets gain factor in decibels."},
{"setMul", (PyCFunction)Gain_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Gain_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Gain_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Gain_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

/**********************************************************************
Number protocol struct. This allow to override the behaviour of mathematical
operations on the object. At the moment of the writing, only +, -, * and /,
and the corresponding "inplace" operations (a *= 1) are implemented. More
to comme in the future. Registered as "tp_as_number".
**********************************************************************/
static PyNumberMethods Gain_as_number = {
(binaryfunc)Gain_add,                           /*nb_add*/
(binaryfunc)Gain_sub,                           /*nb_subtract*/
(binaryfunc)Gain_multiply,                      /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO    /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)Gain_inplace_add,                   /*inplace_add*/
(binaryfunc)Gain_inplace_sub,                   /*inplace_subtract*/
(binaryfunc)Gain_inplace_multiply,              /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Gain_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Gain_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

/**************************************************************
Object's type declaration. The type's name should be "XXXType", 
where XXX is replaced by the name of the object.
Fields in PyTypeObject that are not used should be 0.
**************************************************************/
PyTypeObject GainType = {
PyVarObject_HEAD_INIT(NULL, 0)
/* How the object will be exposed to the 
python interpreter. The name of the C component 
of a PyoObject should be "XXX_base", where XXX
is replaced by the name of the object. These 
objects are actually never directly created
by the user. They are used inside the object's
Python class to handle multi-channel expansion.*/
"_pyo.Gain_base",                      /*tp_name*/
sizeof(Gain),                                  /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Gain_dealloc,                       /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_dbare*/
0,                                              /*tp_repr*/
&Gain_as_number,                                /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Gain objects. Adjusts the gain of an input signal according to a value in decibels.", /* tp_doc */
(traverseproc)Gain_traverse,                    /* tp_traverse */
(inquiry)Gain_clear,                            /* tp_clear */
0,                                              /* tp_richdbare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Gain_methods,                                   /* tp_methods */
Gain_members,                                   /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                                              /* tp_init */
0,                                              /* tp_alloc */
Gain_new,                                       /* tp_new */
};
