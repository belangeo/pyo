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
    int ctlnumber;
    float minscale;
    float maxscale;
    float value;
    float oldValue;
    float sampleToSec;
    int modebuffer[2];
} Midictl;

static void Midictl_postprocessing_ii(Midictl *self) { POST_PROCESSING_II };
static void Midictl_postprocessing_ai(Midictl *self) { POST_PROCESSING_AI };
static void Midictl_postprocessing_ia(Midictl *self) { POST_PROCESSING_IA };
static void Midictl_postprocessing_aa(Midictl *self) { POST_PROCESSING_AA };

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
        case 10:        
            self->muladd_func_ptr = Midictl_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Midictl_postprocessing_aa;
            break;
    }    
}

// Take MIDI events and translate them into OpenGL parameters
void translateMidi(Midictl *self, PmEvent buffer[10])
{
	int status = Pm_MessageStatus(buffer[0].message);	// Temp note event holders
	int number = Pm_MessageData1(buffer[0].message);
	int value = Pm_MessageData2(buffer[0].message);

    if (status == 0xB0 && number == self->ctlnumber) {
        self->oldValue = self->value;
        self->value = (value / 127.) * (self->maxscale - self->minscale) + self->minscale;
    }    
/*	
    switch (status)
	{
        case 0x90:					// It's a note!
            printf("%i, %i\n",pitch, velocity);
            self->freq = 8.175798 * powf(1.0594633, pitch);
            break;
        case 0xB0:					// It's a controller!
            printf("%i, %i\n",pitch, velocity);
            break;
    }
*/ 
}    
    
static void
Midictl_compute_next_data_frame(Midictl *self)
{   
    PmEvent *tmp;
    int i;

    tmp = Server_getMidiEventBuffer((Server *)self->server);
    translateMidi((Midictl *)self, tmp);
    float step = (self->value - self->oldValue) / self->bufsize;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->oldValue + step;
    }  
    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
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

    static char *kwlist[] = {"ctlnumber", "minscale", "maxscale", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i|ffOO", kwlist, &self->ctlnumber, &self->minscale, &self->maxscale, &multmp, &addtmp))
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

    Midictl_compute_next_data_frame((Midictl *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Midictl_getServer(Midictl* self) { GET_SERVER };
static PyObject * Midictl_getStream(Midictl* self) { GET_STREAM };
static PyObject * Midictl_setMul(Midictl *self, PyObject *arg) { SET_MUL };	
static PyObject * Midictl_setAdd(Midictl *self, PyObject *arg) { SET_ADD };	

static PyObject * Midictl_play(Midictl *self) { PLAY };
static PyObject * Midictl_out(Midictl *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Midictl_stop(Midictl *self) { STOP };

static PyObject * Midictl_multiply(Midictl *self, PyObject *arg) { MULTIPLY };
static PyObject * Midictl_inplace_multiply(Midictl *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Midictl_add(Midictl *self, PyObject *arg) { ADD };
static PyObject * Midictl_inplace_add(Midictl *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef Midictl_members[] = {
    {"server", T_OBJECT_EX, offsetof(Midictl, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Midictl, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Midictl, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Midictl, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Midictl_methods[] = {
    //{"getMidictl", (PyCFunction)Midictl_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Midictl_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Midictl_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Midictl_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Midictl_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Midictl_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Midictl_stop, METH_NOARGS, "Stops computing."},
	{"setMul", (PyCFunction)Midictl_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Midictl_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Midictl_as_number = {
    (binaryfunc)Midictl_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Midictl_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Midictl_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Midictl_inplace_multiply,         /*inplace_multiply*/
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

int whichVoice(int *buf, int pitch, int len) {
    int i, voice;
    for (i=0; i<len; i++) {
        if (buf[i*2] == pitch) {
            voice = i;
            break;
        }
    }
    return voice;
}

// Take MIDI events and keep track of notes
void grabMidiNotes(MidiNote *self, PmEvent buffer[1])
{
    int i, voice;
	int status = Pm_MessageStatus(buffer[0].message);	// Temp note event holders
	int pitch = Pm_MessageData1(buffer[0].message);
	int velocity = Pm_MessageData2(buffer[0].message);
    
    if (status == 0x90) {
        if (pitchIsIn(self->notebuf, pitch, self->voices) == 0 && velocity > 0) {
            voice = firstEmpty(self->notebuf, self->voices);
            if (voice > -1) {
                self->notebuf[voice*2] = pitch;
                self->notebuf[voice*2+1] = velocity;
                //printf("%i, %i, %i, %i, %i, %i, %i, %i\n", self->notebuf[0], self->notebuf[1], self->notebuf[2], self->notebuf[3], self->notebuf[4], self->notebuf[5], self->notebuf[6], self->notebuf[7]);
            }    
        }    
        else if (pitchIsIn(self->notebuf, pitch, self->voices) == 1 && velocity == 0) {
            voice = whichVoice(self->notebuf, pitch, self->voices);
            self->notebuf[voice*2] = -1;
            self->notebuf[voice*2+1] = 0;
            //printf("%i, %i, %i, %i, %i, %i, %i, %i\n", self->notebuf[0], self->notebuf[1], self->notebuf[2], self->notebuf[3], self->notebuf[4], self->notebuf[5], self->notebuf[6], self->notebuf[7]);
        }
    }
}    

static void
MidiNote_compute_next_data_frame(MidiNote *self)
{   
    PmEvent *tmp;
    
    tmp = Server_getMidiEventBuffer((Server *)self->server);
    grabMidiNotes((MidiNote *)self, tmp);  
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
    
    MidiNote *self;
    self = (MidiNote *)type->tp_alloc(type, 0);

    self->voices = 10;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MidiNote_compute_next_data_frame);
    self->mode_func_ptr = MidiNote_setProcMode;
    
    return (PyObject *)self;
}

static int
MidiNote_init(MidiNote *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"voices", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->voices))
        return -1; 

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->notebuf = (int *)realloc(self->notebuf, self->voices * 2 * sizeof(int));

    for (i=0; i<self->voices; i++) {
        self->notebuf[i*2] = -1;
        self->notebuf[i*2+1] = 0;
    }
    
    (*self->mode_func_ptr)(self);
    
    MidiNote_compute_next_data_frame((MidiNote *)self);
    
    Py_INCREF(self);
    return 0;
}

int MidiNote_getValue(MidiNote *self, int voice, int which)
{
    return self->notebuf[voice*2+which];
}

static PyObject * MidiNote_getServer(MidiNote* self) { GET_SERVER };
static PyObject * MidiNote_getStream(MidiNote* self) { GET_STREAM };

static PyObject * MidiNote_play(MidiNote *self) { PLAY };
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
{"play", (PyCFunction)MidiNote_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
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
    int scale; /* 0 = midi, 1 = hertz, 2 = transpo */
    int centralkey;
} Notein;

static void Notein_postprocessing_ii(Notein *self) { POST_PROCESSING_II };
static void Notein_postprocessing_ai(Notein *self) { POST_PROCESSING_AI };
static void Notein_postprocessing_ia(Notein *self) { POST_PROCESSING_IA };
static void Notein_postprocessing_aa(Notein *self) { POST_PROCESSING_AA };

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
        case 10:        
            self->muladd_func_ptr = Notein_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Notein_postprocessing_aa;
            break;
    }    
}

static void
Notein_compute_next_data_frame(Notein *self)
{
    int i;
    int tmp = MidiNote_getValue(self->handler, self->voice, self->mode);
    
    if (self->mode == 0 && tmp != -1) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = 8.175798 * powf(1.0594633, tmp);
        }   
    } 
    else if (self->mode == 1) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = tmp / 127.;;
        }         
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
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
    Notein *self;
    self = (Notein *)type->tp_alloc(type, 0);
    
    self->voice = 0;
    self->mode = 0;
    self->scale = 0;
    self->centralkey = 69;
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
    int i;
    PyObject *handlertmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"handler", "voice", "mode", "scale", "centralkey", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiiOO", kwlist, &handlertmp, &self->voice, &self->mode, &self->scale, &self->centralkey, &multmp, &addtmp))
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

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.;
    }  
    
    Notein_compute_next_data_frame((Notein *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Notein_getServer(Notein* self) { GET_SERVER };
static PyObject * Notein_getStream(Notein* self) { GET_STREAM };
static PyObject * Notein_setMul(Notein *self, PyObject *arg) { SET_MUL };	
static PyObject * Notein_setAdd(Notein *self, PyObject *arg) { SET_ADD };	

static PyObject * Notein_play(Notein *self) { PLAY };
static PyObject * Notein_out(Notein *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Notein_stop(Notein *self) { STOP };

static PyObject * Notein_multiply(Notein *self, PyObject *arg) { MULTIPLY };
static PyObject * Notein_inplace_multiply(Notein *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Notein_add(Notein *self, PyObject *arg) { ADD };
static PyObject * Notein_inplace_add(Notein *self, PyObject *arg) { INPLACE_ADD };

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
{"play", (PyCFunction)Notein_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Notein_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Notein_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Notein_setMul, METH_O, "Sets Notein mul factor."},
{"setAdd", (PyCFunction)Notein_setAdd, METH_O, "Sets Notein add factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Notein_as_number = {
(binaryfunc)Notein_add,                      /*nb_add*/
0,                 /*nb_subtract*/
(binaryfunc)Notein_multiply,                 /*nb_multiply*/
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
(binaryfunc)Notein_inplace_add,              /*inplace_add*/
0,         /*inplace_subtract*/
(binaryfunc)Notein_inplace_multiply,         /*inplace_multiply*/
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

