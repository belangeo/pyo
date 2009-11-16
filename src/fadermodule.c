#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    int modebuffer[2];
    int fademode;
    float attack;
    float release;
    float duration;
    float currentTime;
    float sampleToSec;
    float bufsizeToSec;
} Fader;

static void Fader_internal_stop(Fader *self) { 
    int i;
    Stream_setStreamActive(self->stream, 0);
    Stream_setStreamChnl(self->stream, 0);
    Stream_setStreamToDac(self->stream, 0);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0;
    }
}

static void
Fader_generate_auto(Fader *self) {
    float val;
    int i;

    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime <= self->attack)
            val = self->currentTime / self->attack;
        else if (self->currentTime > self->duration)
            val = 0.;
        else if (self->currentTime >= (self->duration - self->release))
            val = (self->duration - self->currentTime) / self->release;
        else
            val = 1.;
        
        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}

static void
Fader_generate_wait(Fader *self) {
    float val;
    int i;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->fademode == 0) {
            
            if (self->currentTime <= self->attack)
                val = self->currentTime / self->attack;
            else
                val = 1.;
        }    
        else {  
            if (self->currentTime <= self->release)
                val = 1. - self->currentTime / self->release;
            else 
                val = 0.;
        }    
        self->data[i] = val;
        self->currentTime += self->sampleToSec;    
    }
    if (self->fademode == 1 && self->currentTime > self->release)
        Fader_internal_stop((Fader *)self);
}

static void Fader_postprocessing_ii(Fader *self) { POST_PROCESSING_II };
static void Fader_postprocessing_ai(Fader *self) { POST_PROCESSING_AI };
static void Fader_postprocessing_ia(Fader *self) { POST_PROCESSING_IA };
static void Fader_postprocessing_aa(Fader *self) { POST_PROCESSING_AA };

static void
Fader_setProcMode(Fader *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    if (self->duration == 0.0)
        self->proc_func_ptr = Fader_generate_wait;
    else
        self->proc_func_ptr = Fader_generate_auto;        
        
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Fader_postprocessing_ii;
            break;
        case 1:   
            self->muladd_func_ptr = Fader_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Fader_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Fader_postprocessing_aa;
            break;
    }    
}

static void
Fader_compute_next_data_frame(Fader *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Fader_traverse(Fader *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Fader_clear(Fader *self)
{
    pyo_CLEAR
    return 0;
}

static void
Fader_dealloc(Fader* self)
{
    free(self->data);
    Fader_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Fader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Fader *self;
    self = (Fader *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    self->fademode = 0;
    self->attack = 0.01;
    self->release = 0.1;
    self->duration = 0.0;
    self->currentTime = 0.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Fader_compute_next_data_frame);
    self->mode_func_ptr = Fader_setProcMode;
    
    Stream_setStreamActive(self->stream, 0);
    
    self->sampleToSec = 1. / self->sr;
    self->bufsizeToSec = self->sampleToSec * self->bufsize;
    
    return (PyObject *)self;
}

static int
Fader_init(Fader *self, PyObject *args, PyObject *kwds)
{
    PyObject *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"fadein", "fadeout", "dur", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|fffOO", kwlist, &self->attack, &self->release, &self->duration, &multmp, &addtmp))
        return -1; 
 
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0;
    }
    Stream_setData(self->stream, self->data);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Fader_getServer(Fader* self) { GET_SERVER };
static PyObject * Fader_getStream(Fader* self) { GET_STREAM };
static PyObject * Fader_setMul(Fader *self, PyObject *arg) { SET_MUL };	
static PyObject * Fader_setAdd(Fader *self, PyObject *arg) { SET_ADD };	

static PyObject * Fader_play(Fader *self) 
{
    self->fademode = 0;
    self->currentTime = 0.0;
    PLAY
};

static PyObject *
Fader_stop(Fader *self)
{
    if (self->duration == 0.0) {
        self->fademode = 1;
        self->currentTime = 0.0;
    }
    else
        Fader_internal_stop((Fader *)self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * Fader_multiply(Fader *self, PyObject *arg) { MULTIPLY };
static PyObject * Fader_inplace_multiply(Fader *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Fader_add(Fader *self, PyObject *arg) { ADD };
static PyObject * Fader_inplace_add(Fader *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
Fader_setFadein(Fader *self, PyObject *arg)
{
    self->attack = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Fader_setFadeout(Fader *self, PyObject *arg)
{
    self->release = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Fader_setDur(Fader *self, PyObject *arg)
{
    self->duration = PyFloat_AsDouble(arg);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Fader_members[] = {
{"server", T_OBJECT_EX, offsetof(Fader, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Fader, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Fader, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Fader, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Fader_methods[] = {
{"getServer", (PyCFunction)Fader_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Fader_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Fader_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Fader_stop, METH_NOARGS, "Starts fadeout and stops computing."},
{"setMul", (PyCFunction)Fader_setMul, METH_O, "Sets Fader mul factor."},
{"setAdd", (PyCFunction)Fader_setAdd, METH_O, "Sets Fader add factor."},
{"setFadein", (PyCFunction)Fader_setFadein, METH_O, "Sets fadein time in seconds."},
{"setFadeout", (PyCFunction)Fader_setFadeout, METH_O, "Sets fadeout time in seconds."},
{"setDur", (PyCFunction)Fader_setAdd, METH_O, "Sets duration in seconds (0 means wait for stop method to start fadeout)."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Fader_as_number = {
(binaryfunc)Fader_add,                      /*nb_add*/
0,                 /*nb_subtract*/
(binaryfunc)Fader_multiply,                 /*nb_multiply*/
0,                   /*nb_divide*/
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
(binaryfunc)Fader_inplace_add,              /*inplace_add*/
0,         /*inplace_subtract*/
(binaryfunc)Fader_inplace_multiply,         /*inplace_multiply*/
0,           /*inplace_divide*/
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

PyTypeObject FaderType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Fader_base",         /*tp_name*/
sizeof(Fader),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Fader_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Fader_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Fader objects. Generates fadin and fadeout signal.",           /* tp_doc */
(traverseproc)Fader_traverse,   /* tp_traverse */
(inquiry)Fader_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Fader_methods,             /* tp_methods */
Fader_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Fader_init,      /* tp_init */
0,                         /* tp_alloc */
Fader_new,                 /* tp_new */
};

