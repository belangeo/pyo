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
#include "portmidi.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *callable;
    int ctlnumber;
    int toprint;
} CtlScan;

static void
CtlScan_setProcMode(CtlScan *self) {}

static void
CtlScan_compute_next_data_frame(CtlScan *self)
{   
    PmEvent *buffer;
    int i, count;

    buffer = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0) {
        PyObject *tup;
        for (i=count-1; i>=0; i--) {
            int status = Pm_MessageStatus(buffer[i].message);	// Temp note event holders
            int number = Pm_MessageData1(buffer[i].message);
            int value = Pm_MessageData2(buffer[i].message);

            if (status == 0xB0) {
                if (number != self->ctlnumber) {
                    self->ctlnumber = number;
                    tup = PyTuple_New(1);
                    PyTuple_SetItem(tup, 0, PyInt_FromLong(self->ctlnumber));
                    PyObject_Call((PyObject *)self->callable, tup, NULL);
                }
                if (self->toprint == 1)
                    printf("ctl number : %i, ctl value : %i\n", self->ctlnumber, value);    
            }
        }
    }
}

static int
CtlScan_traverse(CtlScan *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    return 0;
}

static int 
CtlScan_clear(CtlScan *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    return 0;
}

static void
CtlScan_dealloc(CtlScan* self)
{
    free(self->data);
    CtlScan_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * CtlScan_deleteStream(CtlScan *self) { DELETE_STREAM };

static PyObject *
CtlScan_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    
    CtlScan *self;
    self = (CtlScan *)type->tp_alloc(type, 0);

    self->ctlnumber = -1;
    self->toprint = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CtlScan_compute_next_data_frame);
    self->mode_func_ptr = CtlScan_setProcMode;
    
    return (PyObject *)self;
}

static int
CtlScan_init(CtlScan *self, PyObject *args, PyObject *kwds)
{
    PyObject *calltmp=NULL;

    static char *kwlist[] = {"callable", "toprint", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &calltmp, &self->toprint))
        return -1; 

    if (calltmp) {
        PyObject_CallMethod((PyObject *)self, "setFunction", "O", calltmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Py_INCREF(self);
    return 0;
}

static PyObject * CtlScan_getServer(CtlScan* self) { GET_SERVER };
static PyObject * CtlScan_getStream(CtlScan* self) { GET_STREAM };

static PyObject * CtlScan_play(CtlScan *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CtlScan_stop(CtlScan *self) { STOP };

static PyObject *
CtlScan_setFunction(CtlScan *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    tmp = arg;
    Py_XDECREF(self->callable);
    Py_INCREF(tmp);
    self->callable = tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
CtlScan_setToprint(CtlScan *self, PyObject *arg)
{
	
	if (PyInt_Check(arg)) {
	    self->toprint = PyInt_AsLong(arg);
    }
    
	Py_INCREF(Py_None);
	return Py_None;
}
static PyMemberDef CtlScan_members[] = {
    {"server", T_OBJECT_EX, offsetof(CtlScan, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(CtlScan, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CtlScan_methods[] = {
    {"getServer", (PyCFunction)CtlScan_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)CtlScan_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)CtlScan_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)CtlScan_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)CtlScan_stop, METH_NOARGS, "Stops computing."},
    {"setFunction", (PyCFunction)CtlScan_setFunction, METH_O, "Sets the function to be called."},
    {"setToprint", (PyCFunction)CtlScan_setToprint, METH_O, "If True, print values to the console."},
    {NULL}  /* Sentinel */
};

PyTypeObject CtlScanType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.CtlScan_base",         /*tp_name*/
    sizeof(CtlScan),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CtlScan_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "CtlScan objects. Retreive controller numbers from a Midi input.",           /* tp_doc */
    (traverseproc)CtlScan_traverse,   /* tp_traverse */
    (inquiry)CtlScan_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    CtlScan_methods,             /* tp_methods */
    CtlScan_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)CtlScan_init,      /* tp_init */
    0,                         /* tp_alloc */
    CtlScan_new,                 /* tp_new */
};


typedef struct {
    pyo_audio_HEAD
    int ctlnumber;
    MYFLT minscale;
    MYFLT maxscale;
    MYFLT value;
    MYFLT oldValue;
    MYFLT sampleToSec;
    int modebuffer[2];
} Midictl;

static void Midictl_postprocessing_ii(Midictl *self) { POST_PROCESSING_II };
static void Midictl_postprocessing_ai(Midictl *self) { POST_PROCESSING_AI };
static void Midictl_postprocessing_ia(Midictl *self) { POST_PROCESSING_IA };
static void Midictl_postprocessing_aa(Midictl *self) { POST_PROCESSING_AA };
static void Midictl_postprocessing_ireva(Midictl *self) { POST_PROCESSING_IREVA };
static void Midictl_postprocessing_areva(Midictl *self) { POST_PROCESSING_AREVA };
static void Midictl_postprocessing_revai(Midictl *self) { POST_PROCESSING_REVAI };
static void Midictl_postprocessing_revaa(Midictl *self) { POST_PROCESSING_REVAA };
static void Midictl_postprocessing_revareva(Midictl *self) { POST_PROCESSING_REVAREVA };

static void
Midictl_setProcMode(Midictl *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Midictl_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Midictl_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Midictl_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Midictl_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Midictl_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Midictl_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Midictl_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Midictl_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Midictl_postprocessing_revareva;
            break;
    }
}

// Take MIDI events and translate them...
void translateMidi(Midictl *self, PmEvent *buffer, int count)
{
    int i;
    for (i=count-1; i>=0; i--) {
        int status = Pm_MessageStatus(buffer[i].message);	// Temp note event holders
        int number = Pm_MessageData1(buffer[i].message);
        int value = Pm_MessageData2(buffer[i].message);

        if (status == 0xB0 && number == self->ctlnumber) {
            self->oldValue = self->value;
            self->value = (value / 127.) * (self->maxscale - self->minscale) + self->minscale;
            break;
        }
    }    
}

static void
Midictl_compute_next_data_frame(Midictl *self)
{   
    PmEvent *tmp;
    int i, count;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);

    if (count > 0)
        translateMidi((Midictl *)self, tmp, count);
    MYFLT step = (self->value - self->oldValue) / self->bufsize;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->oldValue + step;
    }  
    
    (*self->muladd_func_ptr)(self);
}

static int
Midictl_traverse(Midictl *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Midictl_clear(Midictl *self)
{
    pyo_CLEAR
    return 0;
}

static void
Midictl_dealloc(Midictl* self)
{
    free(self->data);
    Midictl_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Midictl_deleteStream(Midictl *self) { DELETE_STREAM };

static PyObject *
Midictl_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    
    Midictl *self;
    self = (Midictl *)type->tp_alloc(type, 0);

    self->value = 0.;
    self->oldValue = 0.;
    self->minscale = 0.;
    self->maxscale = 1.;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Midictl_compute_next_data_frame);
    self->mode_func_ptr = Midictl_setProcMode;
    
    return (PyObject *)self;
}

static int
Midictl_init(Midictl *self, PyObject *args, PyObject *kwds)
{
    PyObject *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"ctlnumber", "minscale", "maxscale", "init", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_I_FFFOO, kwlist, &self->ctlnumber, &self->minscale, &self->maxscale, &self->oldValue, &multmp, &addtmp))
        return -1; 
 
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->value = self->oldValue;

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Midictl_getServer(Midictl* self) { GET_SERVER };
static PyObject * Midictl_getStream(Midictl* self) { GET_STREAM };
static PyObject * Midictl_setMul(Midictl *self, PyObject *arg) { SET_MUL };	
static PyObject * Midictl_setAdd(Midictl *self, PyObject *arg) { SET_ADD };	
static PyObject * Midictl_setSub(Midictl *self, PyObject *arg) { SET_SUB };	
static PyObject * Midictl_setDiv(Midictl *self, PyObject *arg) { SET_DIV };	

static PyObject * Midictl_play(Midictl *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Midictl_stop(Midictl *self) { STOP };

static PyObject * Midictl_multiply(Midictl *self, PyObject *arg) { MULTIPLY };
static PyObject * Midictl_inplace_multiply(Midictl *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Midictl_add(Midictl *self, PyObject *arg) { ADD };
static PyObject * Midictl_inplace_add(Midictl *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Midictl_sub(Midictl *self, PyObject *arg) { SUB };
static PyObject * Midictl_inplace_sub(Midictl *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Midictl_div(Midictl *self, PyObject *arg) { DIV };
static PyObject * Midictl_inplace_div(Midictl *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Midictl_members[] = {
    {"server", T_OBJECT_EX, offsetof(Midictl, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Midictl, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Midictl, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Midictl, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Midictl_methods[] = {
    {"getServer", (PyCFunction)Midictl_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Midictl_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Midictl_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Midictl_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)Midictl_stop, METH_NOARGS, "Stops computing."},
	{"setMul", (PyCFunction)Midictl_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Midictl_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Midictl_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Midictl_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Midictl_as_number = {
    (binaryfunc)Midictl_add,                      /*nb_add*/
    (binaryfunc)Midictl_sub,                 /*nb_subtract*/
    (binaryfunc)Midictl_multiply,                 /*nb_multiply*/
    (binaryfunc)Midictl_div,                   /*nb_divide*/
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
    (binaryfunc)Midictl_inplace_add,              /*inplace_add*/
    (binaryfunc)Midictl_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Midictl_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Midictl_inplace_div,           /*inplace_divide*/
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

PyTypeObject MidictlType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Midictl_base",         /*tp_name*/
    sizeof(Midictl),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Midictl_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Midictl_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Midictl objects. Retreive audio from an input channel.",           /* tp_doc */
    (traverseproc)Midictl_traverse,   /* tp_traverse */
    (inquiry)Midictl_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Midictl_methods,             /* tp_methods */
    Midictl_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Midictl_init,      /* tp_init */
    0,                         /* tp_alloc */
    Midictl_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    int *notebuf; /* pitch, velocity, ... */
    int voices;
    int vcount;
    int scale; /* 0 = midi, 1 = hertz, 2 = transpo */
    int first;
    int last;
    int centralkey;
} MidiNote;

static void
MidiNote_setProcMode(MidiNote *self) {};

int
pitchIsIn(int *buf, int pitch, int len) {
    int i;
    int isIn = 0;
    for (i=0; i<len; i++) {
        if (buf[i*2] == pitch) {
            isIn = 1;
            break;
        }    
    }
    return isIn;
}

/* no more used */
int firstEmpty(int *buf, int len) {
    int i;
    int voice = -1;
    for (i=0; i<len; i++) {
        if (buf[i*2+1] == 0) {
            voice = i;
            break;
        }    
    }
    return voice;
}

int nextEmptyVoice(int *buf, int voice, int len) {
    int i, tmp;
    int next = -1;
    for (i=0; i<len; i++) {
        tmp = (i + voice) % len;
        if (buf[tmp*2+1] == 0) {
            next = tmp;
            break;
        }
    }    
    return next;    
}

int whichVoice(int *buf, int pitch, int len) {
    int i;
    int voice = 0;
    for (i=0; i<len; i++) {
        if (buf[i*2] == pitch) {
            voice = i;
            break;
        }
    }
    return voice;
}

// Take MIDI events and keep track of notes
void grabMidiNotes(MidiNote *self, PmEvent *buffer, int count)
{
    int i, voice;
    for (i=0; i<count; i++) {
        int status = Pm_MessageStatus(buffer[i].message);	// Temp note event holders
        int pitch = Pm_MessageData1(buffer[i].message);
        int velocity = Pm_MessageData2(buffer[i].message);
        // int timestamp = buffer[i].timestamp;
        // printf("pitch : %i, velocity : %i, timestamp : %i\n", pitch, velocity, timestamp);
    
        //printf("%i, %i, %i\n", status, pitch, velocity);
        if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) {
            if (pitchIsIn(self->notebuf, pitch, self->voices) == 0 && velocity > 0 && pitch >= self->first && pitch <= self->last) {
                //printf("%i, %i, %i\n", status, pitch, velocity);
                //voice = firstEmpty(self->notebuf, self->voices);
                //voice = self->vcount;
                //self->vcount++;
                //if (self->vcount == self->voices) 
                //    self->vcount = 0;
                voice = nextEmptyVoice(self->notebuf, self->vcount, self->voices);
                if (voice != -1) {
                    self->vcount = voice;
                    self->notebuf[voice*2] = pitch;
                    self->notebuf[voice*2+1] = velocity;
                }    
            }    
            else if (pitchIsIn(self->notebuf, pitch, self->voices) == 1 && velocity == 0 && pitch >= self->first && pitch <= self->last) {
                //printf("%i, %i, %i\n", status, pitch, velocity);
                voice = whichVoice(self->notebuf, pitch, self->voices);
                self->notebuf[voice*2] = -1;
                self->notebuf[voice*2+1] = 0.;
            }
            else if (pitchIsIn(self->notebuf, pitch, self->voices) == 1 && (status & 0xF0) == 0x80 && pitch >= self->first && pitch <= self->last) {
                //printf("%i, %i, %i\n", status, pitch, velocity);
                voice = whichVoice(self->notebuf, pitch, self->voices);
                self->notebuf[voice*2] = -1;
                self->notebuf[voice*2+1] = 0.;
            }
        }
    }    
}    

static void
MidiNote_compute_next_data_frame(MidiNote *self)
{   
    PmEvent *tmp;
    int count;
    
    tmp = Server_getMidiEventBuffer((Server *)self->server);
    count = Server_getMidiEventCount((Server *)self->server);
    if (count > 0)
        grabMidiNotes((MidiNote *)self, tmp, count);  
}

static int
MidiNote_traverse(MidiNote *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
MidiNote_clear(MidiNote *self)
{
    pyo_CLEAR
    return 0;
}

static void
MidiNote_dealloc(MidiNote* self)
{
    free(self->data);
    MidiNote_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * MidiNote_deleteStream(MidiNote *self) { DELETE_STREAM };

static PyObject *
MidiNote_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    
    MidiNote *self;
    self = (MidiNote *)type->tp_alloc(type, 0);

    self->voices = 10;
    self->vcount = 0;
    self->scale = 0;
    self->first = 0;
    self->last = 127;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MidiNote_compute_next_data_frame);
    self->mode_func_ptr = MidiNote_setProcMode;
    
    return (PyObject *)self;
}

static int
MidiNote_init(MidiNote *self, PyObject *args, PyObject *kwds)
{
    int i;
    
    static char *kwlist[] = {"voices", "scale", "first", "last", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iiii", kwlist, &self->voices, &self->scale, &self->first, &self->last))
        return -1; 

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->notebuf = (int *)realloc(self->notebuf, self->voices * 2 * sizeof(int));

    for (i=0; i<self->voices; i++) {
        self->notebuf[i*2] = -1;
        self->notebuf[i*2+1] = 0;
    }

    self->centralkey = (self->first + self->last) / 2;
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

MYFLT MidiNote_getValue(MidiNote *self, int voice, int which)
{
    MYFLT val = -1.0;
    int midival = self->notebuf[voice*2+which];
    if (which == 0 && midival != -1) {
        if (self->scale == 0)
            val = midival;
        else if (self->scale == 1)
            val = 8.1757989156437 * MYPOW(1.0594630943593, midival);
        else if (self->scale == 2)
            val = MYPOW(1.0594630943593, midival - self->centralkey);
    }
    else if (which == 0)
        val = (MYFLT)midival;
    else if (which == 1)
        val = (MYFLT)midival / 127.;
    return val;
}

static PyObject * MidiNote_getServer(MidiNote* self) { GET_SERVER };
static PyObject * MidiNote_getStream(MidiNote* self) { GET_STREAM };

static PyObject * MidiNote_play(MidiNote *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiNote_stop(MidiNote *self) { STOP };

static PyMemberDef MidiNote_members[] = {
{"server", T_OBJECT_EX, offsetof(MidiNote, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(MidiNote, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef MidiNote_methods[] = {
{"getServer", (PyCFunction)MidiNote_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)MidiNote_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)MidiNote_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)MidiNote_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)MidiNote_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject MidiNoteType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.MidiNote_base",         /*tp_name*/
sizeof(MidiNote),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)MidiNote_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"MidiNote objects. Retreive midi note from a midi input.",           /* tp_doc */
(traverseproc)MidiNote_traverse,   /* tp_traverse */
(inquiry)MidiNote_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
MidiNote_methods,             /* tp_methods */
MidiNote_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)MidiNote_init,      /* tp_init */
0,                         /* tp_alloc */
MidiNote_new,                 /* tp_new */
};


/* Notein streamer */
typedef struct {
    pyo_audio_HEAD
    MidiNote *handler;
    int modebuffer[2];
    int voice;
    int mode; /* 0 = pitch, 1 = velocity */
} Notein;

static void Notein_postprocessing_ii(Notein *self) { POST_PROCESSING_II };
static void Notein_postprocessing_ai(Notein *self) { POST_PROCESSING_AI };
static void Notein_postprocessing_ia(Notein *self) { POST_PROCESSING_IA };
static void Notein_postprocessing_aa(Notein *self) { POST_PROCESSING_AA };
static void Notein_postprocessing_ireva(Notein *self) { POST_PROCESSING_IREVA };
static void Notein_postprocessing_areva(Notein *self) { POST_PROCESSING_AREVA };
static void Notein_postprocessing_revai(Notein *self) { POST_PROCESSING_REVAI };
static void Notein_postprocessing_revaa(Notein *self) { POST_PROCESSING_REVAA };
static void Notein_postprocessing_revareva(Notein *self) { POST_PROCESSING_REVAREVA };

static void
Notein_setProcMode(Notein *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Notein_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Notein_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Notein_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Notein_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Notein_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Notein_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Notein_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Notein_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Notein_postprocessing_revareva;
            break;
    }
}

static void
Notein_compute_next_data_frame(Notein *self)
{
    int i;
    MYFLT tmp = MidiNote_getValue(self->handler, self->voice, self->mode);
    
    if (self->mode == 0 && tmp != -1) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = tmp;
        }    
    } 
    else if (self->mode == 1) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = tmp;
        }         
        (*self->muladd_func_ptr)(self);
    }    
}

static int
Notein_traverse(Notein *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->handler);
    return 0;
}

static int 
Notein_clear(Notein *self)
{
    pyo_CLEAR
    Py_CLEAR(self->handler);    
    return 0;
}

static void
Notein_dealloc(Notein* self)
{
    free(self->data);
    Notein_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Notein_deleteStream(Notein *self) { DELETE_STREAM };

static PyObject *
Notein_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Notein *self;
    self = (Notein *)type->tp_alloc(type, 0);
    
    self->voice = 0;
    self->mode = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Notein_compute_next_data_frame);
    self->mode_func_ptr = Notein_setProcMode;
    
    return (PyObject *)self;
}

static int
Notein_init(Notein *self, PyObject *args, PyObject *kwds)
{
    PyObject *handlertmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"handler", "voice", "mode", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiOO", kwlist, &handlertmp, &self->voice, &self->mode, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->handler);
    Py_INCREF(handlertmp);
    self->handler = (MidiNote *)handlertmp;
    
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

static PyObject * Notein_getServer(Notein* self) { GET_SERVER };
static PyObject * Notein_getStream(Notein* self) { GET_STREAM };
static PyObject * Notein_setMul(Notein *self, PyObject *arg) { SET_MUL };	
static PyObject * Notein_setAdd(Notein *self, PyObject *arg) { SET_ADD };	
static PyObject * Notein_setSub(Notein *self, PyObject *arg) { SET_SUB };	
static PyObject * Notein_setDiv(Notein *self, PyObject *arg) { SET_DIV };	

static PyObject * Notein_play(Notein *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Notein_stop(Notein *self) { STOP };

static PyObject * Notein_multiply(Notein *self, PyObject *arg) { MULTIPLY };
static PyObject * Notein_inplace_multiply(Notein *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Notein_add(Notein *self, PyObject *arg) { ADD };
static PyObject * Notein_inplace_add(Notein *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Notein_sub(Notein *self, PyObject *arg) { SUB };
static PyObject * Notein_inplace_sub(Notein *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Notein_div(Notein *self, PyObject *arg) { DIV };
static PyObject * Notein_inplace_div(Notein *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Notein_members[] = {
{"server", T_OBJECT_EX, offsetof(Notein, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Notein, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Notein, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Notein, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Notein_methods[] = {
{"getServer", (PyCFunction)Notein_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Notein_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Notein_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Notein_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Notein_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Notein_setMul, METH_O, "Sets Notein mul factor."},
{"setAdd", (PyCFunction)Notein_setAdd, METH_O, "Sets Notein add factor."},
{"setSub", (PyCFunction)Notein_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Notein_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Notein_as_number = {
(binaryfunc)Notein_add,                      /*nb_add*/
(binaryfunc)Notein_sub,                 /*nb_subtract*/
(binaryfunc)Notein_multiply,                 /*nb_multiply*/
(binaryfunc)Notein_div,                   /*nb_divide*/
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
(binaryfunc)Notein_inplace_add,              /*inplace_add*/
(binaryfunc)Notein_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Notein_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Notein_inplace_div,           /*inplace_divide*/
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

PyTypeObject NoteinType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Notein_base",         /*tp_name*/
sizeof(Notein),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Notein_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Notein_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Notein objects. Stream pitch or velocity from a Notein voice.",           /* tp_doc */
(traverseproc)Notein_traverse,   /* tp_traverse */
(inquiry)Notein_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Notein_methods,             /* tp_methods */
Notein_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Notein_init,      /* tp_init */
0,                         /* tp_alloc */
Notein_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2];
    int fademode;
    int changed;
    MYFLT topValue;
    MYFLT offsetAmp;
    MYFLT initAmp;
    MYFLT sustainAmp;
    MYFLT attack;
    MYFLT decay;
    MYFLT sustain;
    MYFLT release;
    double currentTime;
    MYFLT sampleToSec;
} MidiAdsr;

static void
MidiAdsr_generates(MidiAdsr *self) {
    MYFLT val;
    int i;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        if (self->fademode == 0 && in[i] > 0.0) {
            self->fademode = 1;
            self->initAmp = in[i];
            self->offsetAmp = self->data[i];
            self->sustainAmp = self->initAmp * self->sustain;
            self->currentTime = 0.0;
        }
        else if (self->fademode == 1 && in[i] == 0.0) {
            self->fademode = 0;
            self->currentTime = 0.0;
        }
        
        if (self->fademode == 1) {
            if (self->currentTime <= self->attack)
                val = self->currentTime / self->attack * (self->initAmp - self->offsetAmp) + self->offsetAmp;
            else if (self->currentTime <= (self->attack + self->decay))
                val = (self->decay - (self->currentTime - self->attack)) / self->decay * (self->initAmp - self->sustainAmp) + self->sustainAmp;
            else
                val = self->sustainAmp;
            self->topValue = val;
        }    
        else {  
            if (self->currentTime <= self->release)
                val = self->topValue * (1. - self->currentTime / self->release);
            else 
                val = 0.;
        }    
        self->data[i] = val;
        self->currentTime += self->sampleToSec;    
    }
}

static void MidiAdsr_postprocessing_ii(MidiAdsr *self) { POST_PROCESSING_II };
static void MidiAdsr_postprocessing_ai(MidiAdsr *self) { POST_PROCESSING_AI };
static void MidiAdsr_postprocessing_ia(MidiAdsr *self) { POST_PROCESSING_IA };
static void MidiAdsr_postprocessing_aa(MidiAdsr *self) { POST_PROCESSING_AA };
static void MidiAdsr_postprocessing_ireva(MidiAdsr *self) { POST_PROCESSING_IREVA };
static void MidiAdsr_postprocessing_areva(MidiAdsr *self) { POST_PROCESSING_AREVA };
static void MidiAdsr_postprocessing_revai(MidiAdsr *self) { POST_PROCESSING_REVAI };
static void MidiAdsr_postprocessing_revaa(MidiAdsr *self) { POST_PROCESSING_REVAA };
static void MidiAdsr_postprocessing_revareva(MidiAdsr *self) { POST_PROCESSING_REVAREVA };

static void
MidiAdsr_setProcMode(MidiAdsr *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = MidiAdsr_generates;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = MidiAdsr_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = MidiAdsr_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = MidiAdsr_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = MidiAdsr_postprocessing_revareva;
            break;
    }   
}

static void
MidiAdsr_compute_next_data_frame(MidiAdsr *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
MidiAdsr_traverse(MidiAdsr *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
MidiAdsr_clear(MidiAdsr *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
MidiAdsr_dealloc(MidiAdsr* self)
{
    free(self->data);
    MidiAdsr_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * MidiAdsr_deleteStream(MidiAdsr *self) { DELETE_STREAM };

static PyObject *
MidiAdsr_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MidiAdsr *self;
    self = (MidiAdsr *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    self->topValue = 0.0;
    self->fademode = 0;
    self->changed = 0;
    self->attack = 0.01;
    self->decay = 0.05;
    self->sustain = 0.707;
    self->release = 0.1;
    self->currentTime = 0.0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MidiAdsr_compute_next_data_frame);
    self->mode_func_ptr = MidiAdsr_setProcMode;

    self->sampleToSec = 1. / self->sr;
    
    return (PyObject *)self;
}

static int
MidiAdsr_init(MidiAdsr *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "attack", "decay", "sustain", "release", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFOO, kwlist, &inputtmp, &self->attack, &self->decay, &self->sustain, &self->release, &multmp, &addtmp))
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

static PyObject * MidiAdsr_getServer(MidiAdsr* self) { GET_SERVER };
static PyObject * MidiAdsr_getStream(MidiAdsr* self) { GET_STREAM };
static PyObject * MidiAdsr_setMul(MidiAdsr *self, PyObject *arg) { SET_MUL };	
static PyObject * MidiAdsr_setAdd(MidiAdsr *self, PyObject *arg) { SET_ADD };	
static PyObject * MidiAdsr_setSub(MidiAdsr *self, PyObject *arg) { SET_SUB };	
static PyObject * MidiAdsr_setDiv(MidiAdsr *self, PyObject *arg) { SET_DIV };	

static PyObject * MidiAdsr_play(MidiAdsr *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MidiAdsr_stop(MidiAdsr *self) { STOP }

static PyObject * MidiAdsr_multiply(MidiAdsr *self, PyObject *arg) { MULTIPLY };
static PyObject * MidiAdsr_inplace_multiply(MidiAdsr *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MidiAdsr_add(MidiAdsr *self, PyObject *arg) { ADD };
static PyObject * MidiAdsr_inplace_add(MidiAdsr *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MidiAdsr_sub(MidiAdsr *self, PyObject *arg) { SUB };
static PyObject * MidiAdsr_inplace_sub(MidiAdsr *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MidiAdsr_div(MidiAdsr *self, PyObject *arg) { DIV };
static PyObject * MidiAdsr_inplace_div(MidiAdsr *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
MidiAdsr_setAttack(MidiAdsr *self, PyObject *arg)
{
    self->attack = PyFloat_AsDouble(PyNumber_Float(arg));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
MidiAdsr_setDecay(MidiAdsr *self, PyObject *arg)
{
    self->decay = PyFloat_AsDouble(PyNumber_Float(arg));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
MidiAdsr_setSustain(MidiAdsr *self, PyObject *arg)
{
    self->sustain = PyFloat_AsDouble(PyNumber_Float(arg));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
MidiAdsr_setRelease(MidiAdsr *self, PyObject *arg)
{
    self->release = PyFloat_AsDouble(PyNumber_Float(arg));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef MidiAdsr_members[] = {
    {"server", T_OBJECT_EX, offsetof(MidiAdsr, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MidiAdsr, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MidiAdsr, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(MidiAdsr, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MidiAdsr, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiAdsr_methods[] = {
    {"getServer", (PyCFunction)MidiAdsr_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MidiAdsr_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)MidiAdsr_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)MidiAdsr_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiAdsr_stop, METH_NOARGS, "Starts fadeout and stops computing."},
    {"setMul", (PyCFunction)MidiAdsr_setMul, METH_O, "Sets MidiAdsr mul factor."},
    {"setAdd", (PyCFunction)MidiAdsr_setAdd, METH_O, "Sets MidiAdsr add factor."},
    {"setSub", (PyCFunction)MidiAdsr_setSub, METH_O, "Sets inverse add factor."},
    {"setAttack", (PyCFunction)MidiAdsr_setAttack, METH_O, "Sets attack time in seconds."},
    {"setDecay", (PyCFunction)MidiAdsr_setDecay, METH_O, "Sets decay time in seconds."},
    {"setSustain", (PyCFunction)MidiAdsr_setSustain, METH_O, "Sets sustain level in percent of note amplitude."},
    {"setRelease", (PyCFunction)MidiAdsr_setRelease, METH_O, "Sets release time in seconds."},
    {"setDiv", (PyCFunction)MidiAdsr_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MidiAdsr_as_number = {
    (binaryfunc)MidiAdsr_add,                      /*nb_add*/
    (binaryfunc)MidiAdsr_sub,                 /*nb_subtract*/
    (binaryfunc)MidiAdsr_multiply,                 /*nb_multiply*/
    (binaryfunc)MidiAdsr_div,                   /*nb_divide*/
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
    (binaryfunc)MidiAdsr_inplace_add,              /*inplace_add*/
    (binaryfunc)MidiAdsr_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)MidiAdsr_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)MidiAdsr_inplace_div,           /*inplace_divide*/
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

PyTypeObject MidiAdsrType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.MidiAdsr_base",         /*tp_name*/
    sizeof(MidiAdsr),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MidiAdsr_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &MidiAdsr_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "MidiAdsr objects. Generates MidiAdsr envelope signal.",           /* tp_doc */
    (traverseproc)MidiAdsr_traverse,   /* tp_traverse */
    (inquiry)MidiAdsr_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MidiAdsr_methods,             /* tp_methods */
    MidiAdsr_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MidiAdsr_init,      /* tp_init */
    0,                         /* tp_alloc */
    MidiAdsr_new,                 /* tp_new */
};

