#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

/* Sine object */
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    float pointerPos;
} Sine;

static void
Sine_readframes_ii(Sine *self) {
    float delta, fr, ph, val;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase) * TWOPI;
    delta = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->pointerPos > 1.0)
            self->pointerPos -= 1.0;
        val = sinf(self->pointerPos * TWOPI + ph);
        self->data[i] = val;
        self->pointerPos += delta;
    }
}

static void
Sine_readframes_ai(Sine *self) {
    float delta, ph, val;
    int i;
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase) * TWOPI;
    
    for (i=0; i<self->bufsize; i++) {
        delta = fr[i] / self->sr;
        if (self->pointerPos > 1.0)
            self->pointerPos -= 1.0;
        val = sinf(self->pointerPos * TWOPI + ph);
        self->data[i] = val;
        self->pointerPos += delta;
    }
}

static void
Sine_readframes_ia(Sine *self) {
    float delta, fr, val;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    delta = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->pointerPos > 1.0)
            self->pointerPos -= 1.0;
        val = sinf(self->pointerPos * TWOPI + (ph[i] * TWOPI));
        self->data[i] = val;
        self->pointerPos += delta;
    }
}

static void
Sine_readframes_aa(Sine *self) {
    float delta, val;
    int i;
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    
    for (i=0; i<self->bufsize; i++) {
        delta = fr[i] / self->sr;
        if (self->pointerPos > 1.0)
            self->pointerPos -= 1.0;
        val = sinf(self->pointerPos * TWOPI + (ph[i] * TWOPI));
        self->data[i] = val;
        self->pointerPos += delta;
    }
}

static void Sine_postprocessing_ii(Sine *self) { POST_PROCESSING_II };
static void Sine_postprocessing_ai(Sine *self) { POST_PROCESSING_AI };
static void Sine_postprocessing_ia(Sine *self) { POST_PROCESSING_IA };
static void Sine_postprocessing_aa(Sine *self) { POST_PROCESSING_AA };

static void
Sine_setProcMode(Sine *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Sine_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Sine_readframes_ai;
            break;
        case 10:    
            self->proc_func_ptr = Sine_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Sine_readframes_aa;
            break;
    } 
    
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Sine_postprocessing_ii;
            break;
        case 1:   
            self->muladd_func_ptr = Sine_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Sine_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Sine_postprocessing_aa;
            break;
    }    
}

static void
Sine_compute_next_data_frame(Sine *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Sine_traverse(Sine *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    return 0;
}

static int 
Sine_clear(Sine *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    return 0;
}

static void
Sine_dealloc(Sine* self)
{
    free(self->data);
    Sine_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Sine_deleteStream(Sine *self) { DELETE_STREAM };

static PyObject *
Sine_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Sine *self;
    self = (Sine *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->phase = PyFloat_FromDouble(0.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Sine_compute_next_data_frame);
    self->mode_func_ptr = Sine_setProcMode;
    
    return (PyObject *)self;
}

static int
Sine_init(Sine *self, PyObject *args, PyObject *kwds)
{
    PyObject *freqtmp=NULL, *phasetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"freq", "phase", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &freqtmp, &phasetmp, &multmp, &addtmp))
        return -1; 
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
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
    
    Sine_compute_next_data_frame((Sine *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Sine_getServer(Sine* self) { GET_SERVER };
static PyObject * Sine_getStream(Sine* self) { GET_STREAM };
static PyObject * Sine_setMul(Sine *self, PyObject *arg) { SET_MUL };	
static PyObject * Sine_setAdd(Sine *self, PyObject *arg) { SET_ADD };	

static PyObject * Sine_play(Sine *self) { PLAY };
static PyObject * Sine_out(Sine *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Sine_stop(Sine *self) { STOP };

static PyObject * Sine_multiply(Sine *self, PyObject *arg) { MULTIPLY };
static PyObject * Sine_inplace_multiply(Sine *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Sine_add(Sine *self, PyObject *arg) { ADD };
static PyObject * Sine_inplace_add(Sine *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
Sine_setFreq(Sine *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Sine_setPhase(Sine *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->phase);
	if (isNumber == 1) {
		self->phase = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->phase = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->phase, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->phase_stream);
        self->phase_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Sine_members[] = {
{"server", T_OBJECT_EX, offsetof(Sine, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Sine, stream), 0, "Stream object."},
{"freq", T_OBJECT_EX, offsetof(Sine, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Sine, phase), 0, "Phase of signal (0 -> 1)"},
{"mul", T_OBJECT_EX, offsetof(Sine, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Sine, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Sine_methods[] = {
{"getServer", (PyCFunction)Sine_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Sine_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Sine_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Sine_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Sine_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Sine_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Sine_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Sine_setPhase, METH_O, "Sets oscillator phase between 0 and 1."},
{"setMul", (PyCFunction)Sine_setMul, METH_O, "Sets Sine mul factor."},
{"setAdd", (PyCFunction)Sine_setAdd, METH_O, "Sets Sine add factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Sine_as_number = {
(binaryfunc)Sine_add,                      /*nb_add*/
0,                 /*nb_subtract*/
(binaryfunc)Sine_multiply,                 /*nb_multiply*/
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
(binaryfunc)Sine_inplace_add,              /*inplace_add*/
0,         /*inplace_subtract*/
(binaryfunc)Sine_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject SineType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Sine_base",         /*tp_name*/
sizeof(Sine),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Sine_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Sine_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"Sine objects. Generates a sinewave.",           /* tp_doc */
(traverseproc)Sine_traverse,   /* tp_traverse */
(inquiry)Sine_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Sine_methods,             /* tp_methods */
Sine_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Sine_init,      /* tp_init */
0,                         /* tp_alloc */
Sine_new,                 /* tp_new */
};

/* Osc object */
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3];
    float pointerPos;
} Osc;

static void
Osc_readframes_i(Osc *self) {
    float fr, inc, fpart, val, x, x1;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);

    fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr * size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos = fmodf((self->pointerPos + inc), size);
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        ipart = (int)self->pointerPos;
        fpart = self->pointerPos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        val = x + (x1 - x) * fpart;
        self->data[i] = val;
    }
}

static void
Osc_readframes_a(Osc *self) {
    float inc, fpart, val, x, x1;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * size / self->sr;
        self->pointerPos = fmodf((self->pointerPos + inc), size);
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        ipart = (int)self->pointerPos;
        fpart = self->pointerPos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        val = x + (x1 - x) * fpart;
        self->data[i] = val;
    }
}

static void Osc_postprocessing_ii(Osc *self) { POST_PROCESSING_II };
static void Osc_postprocessing_ai(Osc *self) { POST_PROCESSING_AI };
static void Osc_postprocessing_ia(Osc *self) { POST_PROCESSING_IA };
static void Osc_postprocessing_aa(Osc *self) { POST_PROCESSING_AA };

static void
Osc_setProcMode(Osc *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Osc_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = Osc_readframes_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Osc_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Osc_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Osc_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Osc_postprocessing_aa;
            break;
    }    
}

static void
Osc_compute_next_data_frame(Osc *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Osc_traverse(Osc *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Osc_clear(Osc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Osc_dealloc(Osc* self)
{
    free(self->data);
    Osc_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Osc_deleteStream(Osc *self) { DELETE_STREAM };

static PyObject *
Osc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Osc *self;
    self = (Osc *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(1000);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    self->pointerPos = 0.;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Osc_compute_next_data_frame);
    self->mode_func_ptr = Osc_setProcMode;

    return (PyObject *)self;
}

static int
Osc_init(Osc *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"table", "freq", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &tabletmp, &freqtmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
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

    Osc_compute_next_data_frame((Osc *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Osc_getServer(Osc* self) { GET_SERVER };
static PyObject * Osc_getStream(Osc* self) { GET_STREAM };
static PyObject * Osc_setMul(Osc *self, PyObject *arg) { SET_MUL };	
static PyObject * Osc_setAdd(Osc *self, PyObject *arg) { SET_ADD };	

static PyObject * Osc_play(Osc *self) { PLAY };
static PyObject * Osc_out(Osc *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Osc_stop(Osc *self) { STOP };

static PyObject * Osc_multiply(Osc *self, PyObject *arg) { MULTIPLY };
static PyObject * Osc_inplace_multiply(Osc *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Osc_add(Osc *self, PyObject *arg) { ADD };
static PyObject * Osc_inplace_add(Osc *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
Osc_getTable(Osc* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Osc_setFreq(Osc *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Osc_members[] = {
    {"server", T_OBJECT_EX, offsetof(Osc, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Osc, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Osc, table), 0, "Waveform table."},
    {"freq", T_OBJECT_EX, offsetof(Osc, freq), 0, "Frequency in cycle per second."},
    {"mul", T_OBJECT_EX, offsetof(Osc, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Osc, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Osc_methods[] = {
    {"getTable", (PyCFunction)Osc_getTable, METH_NOARGS, "Returns waveform table object."},
    {"getServer", (PyCFunction)Osc_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Osc_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Osc_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Osc_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Osc_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Osc_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Osc_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
	{"setMul", (PyCFunction)Osc_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Osc_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Osc_as_number = {
    (binaryfunc)Osc_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Osc_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Osc_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Osc_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject OscType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Osc_base",         /*tp_name*/
    sizeof(Osc),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Osc_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Osc_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Osc objects. Generates an oscillatory waveform.",           /* tp_doc */
    (traverseproc)Osc_traverse,   /* tp_traverse */
    (inquiry)Osc_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Osc_methods,             /* tp_methods */
    Osc_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Osc_init,      /* tp_init */
    0,                         /* tp_alloc */
    Osc_new,                 /* tp_new */
};

