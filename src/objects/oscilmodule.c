#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

static float
_clip(float x) {
    if (x < 0.0)
        return 0.0;
    else if (x >= 1.0)
        return 1.0;
    else
        return x;
}

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
    delta = fr / self->sr * TWOPI;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->pointerPos > TWOPI)
            self->pointerPos -= TWOPI;
        val = sinf(self->pointerPos + ph);
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
        delta = fr[i] / self->sr * TWOPI;
        if (self->pointerPos > TWOPI)
            self->pointerPos -= TWOPI;
        val = sinf(self->pointerPos + ph);
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
    delta = fr / self->sr * TWOPI;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->pointerPos > TWOPI)
            self->pointerPos -= TWOPI;
        val = sinf(self->pointerPos + (ph[i] * TWOPI));
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
        delta = fr[i] / self->sr * TWOPI;
        if (self->pointerPos > TWOPI)
            self->pointerPos -= TWOPI;
        val = sinf(self->pointerPos + (ph[i] * TWOPI));
        self->data[i] = val;
        self->pointerPos += delta;
    }
}

static void Sine_postprocessing_ii(Sine *self) { POST_PROCESSING_II };
static void Sine_postprocessing_ai(Sine *self) { POST_PROCESSING_AI };
static void Sine_postprocessing_ia(Sine *self) { POST_PROCESSING_IA };
static void Sine_postprocessing_aa(Sine *self) { POST_PROCESSING_AA };
static void Sine_postprocessing_ireva(Sine *self) { POST_PROCESSING_IREVA };
static void Sine_postprocessing_areva(Sine *self) { POST_PROCESSING_AREVA };
static void Sine_postprocessing_revai(Sine *self) { POST_PROCESSING_REVAI };
static void Sine_postprocessing_revaa(Sine *self) { POST_PROCESSING_REVAA };
static void Sine_postprocessing_revareva(Sine *self) { POST_PROCESSING_REVAREVA };

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
        case 2:    
            self->muladd_func_ptr = Sine_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Sine_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Sine_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Sine_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Sine_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Sine_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Sine_postprocessing_revareva;
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
static PyObject * Sine_setSub(Sine *self, PyObject *arg) { SET_SUB };	
static PyObject * Sine_setDiv(Sine *self, PyObject *arg) { SET_DIV };	

static PyObject * Sine_play(Sine *self) { PLAY };
static PyObject * Sine_out(Sine *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Sine_stop(Sine *self) { STOP };

static PyObject * Sine_multiply(Sine *self, PyObject *arg) { MULTIPLY };
static PyObject * Sine_inplace_multiply(Sine *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Sine_add(Sine *self, PyObject *arg) { ADD };
static PyObject * Sine_inplace_add(Sine *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Sine_sub(Sine *self, PyObject *arg) { SUB };
static PyObject * Sine_inplace_sub(Sine *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Sine_div(Sine *self, PyObject *arg) { DIV };
static PyObject * Sine_inplace_div(Sine *self, PyObject *arg) { INPLACE_DIV };

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
{"setSub", (PyCFunction)Sine_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Sine_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Sine_as_number = {
(binaryfunc)Sine_add,                      /*nb_add*/
(binaryfunc)Sine_sub,                 /*nb_subtract*/
(binaryfunc)Sine_multiply,                 /*nb_multiply*/
(binaryfunc)Sine_div,                   /*nb_divide*/
0,                /*nb_remainder*/
0,                   /*nb_divmod*/
0,                   /*nb_power*/
0,                  /*nb_neg*/
0,                /*nb_pos*/
0,                  /*(unaryfunc)array_abs*/
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
(binaryfunc)Sine_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Sine_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Sine_inplace_div,           /*inplace_divide*/
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

/**************/
/* Osc object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    float pointerPos;
} Osc;

static void
Osc_readframes_ii(Osc *self) {
    float fr, ph, pos, inc, fpart, x, x1;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);

    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    inc = fr * size / self->sr;

    ph *= size;
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size)
            self->pointerPos -= size;
        pos = self->pointerPos + ph;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        self->data[i] = x + (x1 - x) * fpart;
    }
}

static void
Osc_readframes_ai(Osc *self) {
    float inc, ph, pos, fpart, x, x1, sizeOnSr;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    ph *= size;
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size)
            self->pointerPos -= size;
        pos = self->pointerPos + ph;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        self->data[i] = x + (x1 - x) * fpart;
    }
}

static void
Osc_readframes_ia(Osc *self) {
    float fr, pha, pos, inc, fpart, x, x1;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pha = ph[i] * size;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size)
            self->pointerPos -= size;
        pos = self->pointerPos + pha;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        self->data[i] = x + (x1 - x) * fpart;
    }
}

static void
Osc_readframes_aa(Osc *self) {
    float inc, pha, pos, fpart, x, x1, sizeOnSr;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *ph = Stream_getData((Stream *)self->phase_stream);

    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        pha = ph[i] * size;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size)
            self->pointerPos -= size;
        pos = self->pointerPos + pha;
        if (pos >= size)
            pos -= size;
        ipart = (int)pos;
        fpart = pos - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        self->data[i] = x + (x1 - x) * fpart;
    }
}

static void Osc_postprocessing_ii(Osc *self) { POST_PROCESSING_II };
static void Osc_postprocessing_ai(Osc *self) { POST_PROCESSING_AI };
static void Osc_postprocessing_ia(Osc *self) { POST_PROCESSING_IA };
static void Osc_postprocessing_aa(Osc *self) { POST_PROCESSING_AA };
static void Osc_postprocessing_ireva(Osc *self) { POST_PROCESSING_IREVA };
static void Osc_postprocessing_areva(Osc *self) { POST_PROCESSING_AREVA };
static void Osc_postprocessing_revai(Osc *self) { POST_PROCESSING_REVAI };
static void Osc_postprocessing_revaa(Osc *self) { POST_PROCESSING_REVAA };
static void Osc_postprocessing_revareva(Osc *self) { POST_PROCESSING_REVAREVA };

static void
Osc_setProcMode(Osc *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Osc_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Osc_readframes_ai;
            break;
        case 10:        
            self->proc_func_ptr = Osc_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Osc_readframes_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Osc_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Osc_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Osc_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Osc_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Osc_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Osc_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Osc_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Osc_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Osc_postprocessing_revareva;
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
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Osc_clear(Osc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
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
    self->phase = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Osc_compute_next_data_frame);
    self->mode_func_ptr = Osc_setProcMode;

    return (PyObject *)self;
}

static int
Osc_init(Osc *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *freqtmp=NULL, *phasetmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"table", "freq", "phase", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &tabletmp, &freqtmp, &phasetmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }

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
static PyObject * Osc_setSub(Osc *self, PyObject *arg) { SET_SUB };	
static PyObject * Osc_setDiv(Osc *self, PyObject *arg) { SET_DIV };	

static PyObject * Osc_play(Osc *self) { PLAY };
static PyObject * Osc_out(Osc *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Osc_stop(Osc *self) { STOP };

static PyObject * Osc_multiply(Osc *self, PyObject *arg) { MULTIPLY };
static PyObject * Osc_inplace_multiply(Osc *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Osc_add(Osc *self, PyObject *arg) { ADD };
static PyObject * Osc_inplace_add(Osc *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Osc_sub(Osc *self, PyObject *arg) { SUB };
static PyObject * Osc_inplace_sub(Osc *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Osc_div(Osc *self, PyObject *arg) { DIV };
static PyObject * Osc_inplace_div(Osc *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Osc_getTable(Osc* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Osc_setTable(Osc *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

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

static PyObject *
Osc_setPhase(Osc *self, PyObject *arg)
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

static PyMemberDef Osc_members[] = {
    {"server", T_OBJECT_EX, offsetof(Osc, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Osc, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Osc, table), 0, "Waveform table."},
    {"freq", T_OBJECT_EX, offsetof(Osc, freq), 0, "Frequency in cycle per second."},
    {"phase", T_OBJECT_EX, offsetof(Osc, phase), 0, "Oscillator phase."},
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
    {"setTable", (PyCFunction)Osc_setTable, METH_O, "Sets oscillator table."},
	{"setFreq", (PyCFunction)Osc_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
    {"setPhase", (PyCFunction)Osc_setPhase, METH_O, "Sets oscillator phase."},
	{"setMul", (PyCFunction)Osc_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Osc_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Osc_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)Osc_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Osc_as_number = {
    (binaryfunc)Osc_add,                      /*nb_add*/
    (binaryfunc)Osc_sub,                 /*nb_subtract*/
    (binaryfunc)Osc_multiply,                 /*nb_multiply*/
    (binaryfunc)Osc_div,                   /*nb_divide*/
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
    (binaryfunc)Osc_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Osc_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Osc_inplace_div,           /*inplace_divide*/
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

/**************/
/* Phasor object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    int modebuffer[4];
    float pointerPos;
} Phasor;

static void
Phasor_readframes_ii(Phasor *self) {
    float fr, ph, pos, inc;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    if (ph < 0.0)
        ph = 0.0;
    else if (ph > 1.0)
        ph = 1.0;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pos = self->pointerPos + ph;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_ai(Phasor *self) {
    float inc, ph, pos, oneOnSr;
    int i;
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    if (ph < 0.0)
        ph = 0.0;
    else if (ph > 1.0)
        ph = 1.0;
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        pos = self->pointerPos + ph;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_ia(Phasor *self) {
    float fr, pha, pos, inc;
    int i;
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        pha = ph[i];
        if (pha < 0.0)
            pha = 0.0;
        else if (pha > 1.0)
            pha = 1.0;

        pos = self->pointerPos + pha;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void
Phasor_readframes_aa(Phasor *self) {
    float pha, pos, inc, oneOnSr;
    int i;
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *ph = Stream_getData((Stream *)self->phase_stream);

    oneOnSr = 1.0 / self->sr;

    for (i=0; i<self->bufsize; i++) {
        pha = ph[i];
        if (pha < 0.0)
            pha = 0.0;
        else if (pha > 1.0)
            pha = 1.0;
        
        pos = self->pointerPos + pha;
        if (pos > 1)
            pos -= 1.0;
        self->data[i] = pos;
        
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
}

static void Phasor_postprocessing_ii(Phasor *self) { POST_PROCESSING_II };
static void Phasor_postprocessing_ai(Phasor *self) { POST_PROCESSING_AI };
static void Phasor_postprocessing_ia(Phasor *self) { POST_PROCESSING_IA };
static void Phasor_postprocessing_aa(Phasor *self) { POST_PROCESSING_AA };
static void Phasor_postprocessing_ireva(Phasor *self) { POST_PROCESSING_IREVA };
static void Phasor_postprocessing_areva(Phasor *self) { POST_PROCESSING_AREVA };
static void Phasor_postprocessing_revai(Phasor *self) { POST_PROCESSING_REVAI };
static void Phasor_postprocessing_revaa(Phasor *self) { POST_PROCESSING_REVAA };
static void Phasor_postprocessing_revareva(Phasor *self) { POST_PROCESSING_REVAREVA };

static void
Phasor_setProcMode(Phasor *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Phasor_readframes_ii;
            break;
        case 1:    
            self->proc_func_ptr = Phasor_readframes_ai;
            break;
        case 10:        
            self->proc_func_ptr = Phasor_readframes_ia;
            break;
        case 11:    
            self->proc_func_ptr = Phasor_readframes_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Phasor_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Phasor_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Phasor_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Phasor_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Phasor_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Phasor_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Phasor_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Phasor_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Phasor_postprocessing_revareva;
            break;
    } 
}

static void
Phasor_compute_next_data_frame(Phasor *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Phasor_traverse(Phasor *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Phasor_clear(Phasor *self)
{
    pyo_CLEAR
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Phasor_dealloc(Phasor* self)
{
    free(self->data);
    Phasor_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Phasor_deleteStream(Phasor *self) { DELETE_STREAM };

static PyObject *
Phasor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Phasor *self;
    self = (Phasor *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->phase = PyFloat_FromDouble(0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Phasor_compute_next_data_frame);
    self->mode_func_ptr = Phasor_setProcMode;
    
    return (PyObject *)self;
}

static int
Phasor_init(Phasor *self, PyObject *args, PyObject *kwds)
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
    
    Phasor_compute_next_data_frame((Phasor *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Phasor_getServer(Phasor* self) { GET_SERVER };
static PyObject * Phasor_getStream(Phasor* self) { GET_STREAM };
static PyObject * Phasor_setMul(Phasor *self, PyObject *arg) { SET_MUL };	
static PyObject * Phasor_setAdd(Phasor *self, PyObject *arg) { SET_ADD };	
static PyObject * Phasor_setSub(Phasor *self, PyObject *arg) { SET_SUB };	
static PyObject * Phasor_setDiv(Phasor *self, PyObject *arg) { SET_DIV };	

static PyObject * Phasor_play(Phasor *self) { PLAY };
static PyObject * Phasor_out(Phasor *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Phasor_stop(Phasor *self) { STOP };

static PyObject * Phasor_multiply(Phasor *self, PyObject *arg) { MULTIPLY };
static PyObject * Phasor_inplace_multiply(Phasor *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Phasor_add(Phasor *self, PyObject *arg) { ADD };
static PyObject * Phasor_inplace_add(Phasor *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Phasor_sub(Phasor *self, PyObject *arg) { SUB };
static PyObject * Phasor_inplace_sub(Phasor *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Phasor_div(Phasor *self, PyObject *arg) { DIV };
static PyObject * Phasor_inplace_div(Phasor *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Phasor_setFreq(Phasor *self, PyObject *arg)
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
Phasor_setPhase(Phasor *self, PyObject *arg)
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

static PyMemberDef Phasor_members[] = {
{"server", T_OBJECT_EX, offsetof(Phasor, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Phasor, stream), 0, "Stream object."},
{"freq", T_OBJECT_EX, offsetof(Phasor, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Phasor, phase), 0, "Phasorillator phase."},
{"mul", T_OBJECT_EX, offsetof(Phasor, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Phasor, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Phasor_methods[] = {
{"getServer", (PyCFunction)Phasor_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Phasor_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Phasor_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Phasor_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Phasor_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Phasor_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Phasor_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Phasor_setPhase, METH_O, "Sets oscillator phase."},
{"setMul", (PyCFunction)Phasor_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Phasor_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Phasor_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Phasor_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Phasor_as_number = {
(binaryfunc)Phasor_add,                      /*nb_add*/
(binaryfunc)Phasor_sub,                 /*nb_subtract*/
(binaryfunc)Phasor_multiply,                 /*nb_multiply*/
(binaryfunc)Phasor_div,                   /*nb_divide*/
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
(binaryfunc)Phasor_inplace_add,              /*inplace_add*/
(binaryfunc)Phasor_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Phasor_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Phasor_inplace_div,           /*inplace_divide*/
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

PyTypeObject PhasorType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Phasor_base",         /*tp_name*/
sizeof(Phasor),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Phasor_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Phasor_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Phasor objects. Phase incrementor from 0 to 1.",           /* tp_doc */
(traverseproc)Phasor_traverse,   /* tp_traverse */
(inquiry)Phasor_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Phasor_methods,             /* tp_methods */
Phasor_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Phasor_init,      /* tp_init */
0,                         /* tp_alloc */
Phasor_new,                 /* tp_new */
};

/**************/
/* Pointer object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *index;
    Stream *index_stream;
    int modebuffer[2];
} Pointer;

static void
Pointer_readframes_a(Pointer *self) {
    float ph, fpart, x, x1;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *pha = Stream_getData((Stream *)self->index_stream);
    
    for (i=0; i<self->bufsize; i++) {
        ph = pha[i] * size;
        if (ph < 0)
            ph += size;
        else if (ph >= size) {
            while (ph >= size) {
                ph -= size;
            }
        }    
        ipart = (int)ph;
        fpart = ph - ipart;
        x = tablelist[ipart];
        x1 = tablelist[ipart+1];
        self->data[i] = x + (x1 - x) * fpart;
    }
}

static void Pointer_postprocessing_ii(Pointer *self) { POST_PROCESSING_II };
static void Pointer_postprocessing_ai(Pointer *self) { POST_PROCESSING_AI };
static void Pointer_postprocessing_ia(Pointer *self) { POST_PROCESSING_IA };
static void Pointer_postprocessing_aa(Pointer *self) { POST_PROCESSING_AA };
static void Pointer_postprocessing_ireva(Pointer *self) { POST_PROCESSING_IREVA };
static void Pointer_postprocessing_areva(Pointer *self) { POST_PROCESSING_AREVA };
static void Pointer_postprocessing_revai(Pointer *self) { POST_PROCESSING_REVAI };
static void Pointer_postprocessing_revaa(Pointer *self) { POST_PROCESSING_REVAA };
static void Pointer_postprocessing_revareva(Pointer *self) { POST_PROCESSING_REVAREVA };

static void
Pointer_setProcMode(Pointer *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Pointer_readframes_a;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Pointer_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Pointer_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Pointer_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Pointer_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Pointer_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Pointer_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Pointer_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Pointer_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Pointer_postprocessing_revareva;
            break;
    } 
}

static void
Pointer_compute_next_data_frame(Pointer *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Pointer_traverse(Pointer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->index);    
    Py_VISIT(self->index_stream);    
    return 0;
}

static int 
Pointer_clear(Pointer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->index);    
    Py_CLEAR(self->index_stream);    
    return 0;
}

static void
Pointer_dealloc(Pointer* self)
{
    free(self->data);
    Pointer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pointer_deleteStream(Pointer *self) { DELETE_STREAM };

static PyObject *
Pointer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Pointer *self;
    self = (Pointer *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pointer_compute_next_data_frame);
    self->mode_func_ptr = Pointer_setProcMode;
    
    return (PyObject *)self;
}

static int
Pointer_init(Pointer *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *indextmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "index", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &tabletmp, &indextmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (indextmp) {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }

    PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Pointer_compute_next_data_frame((Pointer *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Pointer_getServer(Pointer* self) { GET_SERVER };
static PyObject * Pointer_getStream(Pointer* self) { GET_STREAM };
static PyObject * Pointer_setMul(Pointer *self, PyObject *arg) { SET_MUL };	
static PyObject * Pointer_setAdd(Pointer *self, PyObject *arg) { SET_ADD };	
static PyObject * Pointer_setSub(Pointer *self, PyObject *arg) { SET_SUB };	
static PyObject * Pointer_setDiv(Pointer *self, PyObject *arg) { SET_DIV };	

static PyObject * Pointer_play(Pointer *self) { PLAY };
static PyObject * Pointer_out(Pointer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pointer_stop(Pointer *self) { STOP };

static PyObject * Pointer_multiply(Pointer *self, PyObject *arg) { MULTIPLY };
static PyObject * Pointer_inplace_multiply(Pointer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pointer_add(Pointer *self, PyObject *arg) { ADD };
static PyObject * Pointer_inplace_add(Pointer *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pointer_sub(Pointer *self, PyObject *arg) { SUB };
static PyObject * Pointer_inplace_sub(Pointer *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pointer_div(Pointer *self, PyObject *arg) { DIV };
static PyObject * Pointer_inplace_div(Pointer *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Pointer_getTable(Pointer* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Pointer_setTable(Pointer *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pointer_setIndex(Pointer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	if (isNumber == 1) {
		printf("Pointer index attributes must be a PyoObject.\n");
        Py_INCREF(Py_None);
        return Py_None;
	}
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_XDECREF(self->index);

    self->index = tmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->index_stream);
    self->index_stream = (Stream *)streamtmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Pointer_members[] = {
{"server", T_OBJECT_EX, offsetof(Pointer, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pointer, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(Pointer, table), 0, "Waveform table."},
{"index", T_OBJECT_EX, offsetof(Pointer, index), 0, "Reader index."},
{"mul", T_OBJECT_EX, offsetof(Pointer, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pointer, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pointer_methods[] = {
{"getTable", (PyCFunction)Pointer_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)Pointer_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pointer_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pointer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pointer_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pointer_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pointer_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)Pointer_setTable, METH_O, "Sets oscillator table."},
{"setIndex", (PyCFunction)Pointer_setIndex, METH_O, "Sets reader index."},
{"setMul", (PyCFunction)Pointer_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Pointer_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Pointer_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Pointer_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pointer_as_number = {
(binaryfunc)Pointer_add,                      /*nb_add*/
(binaryfunc)Pointer_sub,                 /*nb_subtract*/
(binaryfunc)Pointer_multiply,                 /*nb_multiply*/
(binaryfunc)Pointer_div,                   /*nb_divide*/
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
(binaryfunc)Pointer_inplace_add,              /*inplace_add*/
(binaryfunc)Pointer_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pointer_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Pointer_inplace_div,           /*inplace_divide*/
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

PyTypeObject PointerType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pointer_base",         /*tp_name*/
sizeof(Pointer),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pointer_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Pointer_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Pointer objects. Read a waveform table with a pointer index.",           /* tp_doc */
(traverseproc)Pointer_traverse,   /* tp_traverse */
(inquiry)Pointer_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pointer_methods,             /* tp_methods */
Pointer_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pointer_init,      /* tp_init */
0,                         /* tp_alloc */
Pointer_new,                 /* tp_new */
};

/**************/
/* Pulsar object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *env;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *phase;
    Stream *phase_stream;
    PyObject *frac;
    Stream *frac_stream;
    int modebuffer[5];
    float pointerPos;
} Pulsar;

static void
Pulsar_readframes_iii(Pulsar *self) {
    float fr, ph, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, x, x1, tmp;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aii(Pulsar *self) {
    float ph, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, x, x1, tmp, oneOnSr;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iai(Pulsar *self) {
    float fr, frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, x, x1, tmp;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aai(Pulsar *self) {
    float frac, invfrac, pos, scl_pos, t_pos, e_pos, fpart, x, x1, tmp, oneOnSr;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    frac = _clip(PyFloat_AS_DOUBLE(self->frac));
    invfrac = 1.0 / frac;
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < frac) {
            scl_pos = pos * invfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iia(Pulsar *self) {
    float fr, ph, pos, curfrac, scl_pos, t_pos, e_pos, fpart, x, x1, tmp;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    ph = PyFloat_AS_DOUBLE(self->phase);
    float *frac = Stream_getData((Stream *)self->frac_stream);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aia(Pulsar *self) {
    float ph, pos, curfrac, scl_pos, t_pos, e_pos, fpart, x, x1, tmp, oneOnSr;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    ph = PyFloat_AS_DOUBLE(self->phase);
    float *frac = Stream_getData((Stream *)self->frac_stream);
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph;
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_iaa(Pulsar *self) {
    float fr, pos, curfrac, scl_pos, t_pos, e_pos, fpart, x, x1, tmp;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    float *frac = Stream_getData((Stream *)self->frac_stream);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void
Pulsar_readframes_aaa(Pulsar *self) {
    float pos, curfrac, scl_pos, t_pos, e_pos, fpart, x, x1, tmp, oneOnSr;
    double inc;
    int i, ipart;
    float *tablelist = TableStream_getData(self->table);
    float *envlist = TableStream_getData(self->env);
    int size = TableStream_getSize(self->table);
    int envsize = TableStream_getSize(self->env);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *ph = Stream_getData((Stream *)self->phase_stream);
    float *frac = Stream_getData((Stream *)self->frac_stream);
    
    oneOnSr = 1.0 / self->sr;
    for (i=0; i<self->bufsize; i++) {
        curfrac = frac[i];
        inc = fr[i] * oneOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = 1.0 + self->pointerPos;
        else if (self->pointerPos >= 1.0)
            self->pointerPos -= 1.0;
        pos = self->pointerPos + ph[i];
        if (pos >= 1.0)
            pos -= 1.0;
        if (pos < curfrac) {
            scl_pos = pos / curfrac;
            t_pos = scl_pos * size;
            ipart = (int)t_pos;
            fpart = t_pos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            tmp = x + (x1 - x) * fpart;
            
            e_pos = scl_pos * envsize;
            ipart = (int)e_pos;
            fpart = e_pos - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];            
            self->data[i] = tmp * (x + (x1 - x) * fpart);
        }    
        else {
            self->data[i] = 0.0;
        }    
    }
}

static void Pulsar_postprocessing_ii(Pulsar *self) { POST_PROCESSING_II };
static void Pulsar_postprocessing_ai(Pulsar *self) { POST_PROCESSING_AI };
static void Pulsar_postprocessing_ia(Pulsar *self) { POST_PROCESSING_IA };
static void Pulsar_postprocessing_aa(Pulsar *self) { POST_PROCESSING_AA };
static void Pulsar_postprocessing_ireva(Pulsar *self) { POST_PROCESSING_IREVA };
static void Pulsar_postprocessing_areva(Pulsar *self) { POST_PROCESSING_AREVA };
static void Pulsar_postprocessing_revai(Pulsar *self) { POST_PROCESSING_REVAI };
static void Pulsar_postprocessing_revaa(Pulsar *self) { POST_PROCESSING_REVAA };
static void Pulsar_postprocessing_revareva(Pulsar *self) { POST_PROCESSING_REVAREVA };

static void
Pulsar_setProcMode(Pulsar *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = Pulsar_readframes_iii;
            break;
        case 1:    
            self->proc_func_ptr = Pulsar_readframes_aii;
            break;
        case 10:        
            self->proc_func_ptr = Pulsar_readframes_iai;
            break;
        case 11:    
            self->proc_func_ptr = Pulsar_readframes_aai;
            break;
        case 100:        
            self->proc_func_ptr = Pulsar_readframes_iia;
            break;
        case 101:    
            self->proc_func_ptr = Pulsar_readframes_aia;
            break;
        case 110:        
            self->proc_func_ptr = Pulsar_readframes_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Pulsar_readframes_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Pulsar_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Pulsar_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Pulsar_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Pulsar_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Pulsar_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Pulsar_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Pulsar_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Pulsar_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Pulsar_postprocessing_revareva;
            break;
    } 
}

static void
Pulsar_compute_next_data_frame(Pulsar *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Pulsar_traverse(Pulsar *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->env);
    Py_VISIT(self->phase);    
    Py_VISIT(self->phase_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->frac);    
    Py_VISIT(self->frac_stream);
    return 0;
}

static int 
Pulsar_clear(Pulsar *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->env);
    Py_CLEAR(self->phase);    
    Py_CLEAR(self->phase_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->frac);    
    Py_CLEAR(self->frac_stream);
    return 0;
}

static void
Pulsar_dealloc(Pulsar* self)
{
    free(self->data);
    Pulsar_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pulsar_deleteStream(Pulsar *self) { DELETE_STREAM };

static PyObject *
Pulsar_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Pulsar *self;
    self = (Pulsar *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->phase = PyFloat_FromDouble(0);
    self->frac = PyFloat_FromDouble(0.5);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pulsar_compute_next_data_frame);
    self->mode_func_ptr = Pulsar_setProcMode;
    
    return (PyObject *)self;
}

static int
Pulsar_init(Pulsar *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *envtmp, *freqtmp=NULL, *phasetmp=NULL, *fractmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "env", "freq", "frac", "phase", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOOO", kwlist, &tabletmp, &envtmp, &freqtmp, &fractmp, &phasetmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    Py_XDECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)envtmp, "getTableStream", "");

    if (phasetmp) {
        PyObject_CallMethod((PyObject *)self, "setPhase", "O", phasetmp);
    }
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (fractmp) {
        PyObject_CallMethod((PyObject *)self, "setFrac", "O", fractmp);
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
    
    Pulsar_compute_next_data_frame((Pulsar *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Pulsar_getServer(Pulsar* self) { GET_SERVER };
static PyObject * Pulsar_getStream(Pulsar* self) { GET_STREAM };
static PyObject * Pulsar_setMul(Pulsar *self, PyObject *arg) { SET_MUL };	
static PyObject * Pulsar_setAdd(Pulsar *self, PyObject *arg) { SET_ADD };	
static PyObject * Pulsar_setSub(Pulsar *self, PyObject *arg) { SET_SUB };	
static PyObject * Pulsar_setDiv(Pulsar *self, PyObject *arg) { SET_DIV };	

static PyObject * Pulsar_play(Pulsar *self) { PLAY };
static PyObject * Pulsar_out(Pulsar *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Pulsar_stop(Pulsar *self) { STOP };

static PyObject * Pulsar_multiply(Pulsar *self, PyObject *arg) { MULTIPLY };
static PyObject * Pulsar_inplace_multiply(Pulsar *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Pulsar_add(Pulsar *self, PyObject *arg) { ADD };
static PyObject * Pulsar_inplace_add(Pulsar *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Pulsar_sub(Pulsar *self, PyObject *arg) { SUB };
static PyObject * Pulsar_inplace_sub(Pulsar *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Pulsar_div(Pulsar *self, PyObject *arg) { DIV };
static PyObject * Pulsar_inplace_div(Pulsar *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Pulsar_getTable(Pulsar* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Pulsar_getEnv(Pulsar* self)
{
    Py_INCREF(self->env);
    return self->env;
};

static PyObject *
Pulsar_setTable(Pulsar *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setEnv(Pulsar *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Pulsar_setFreq(Pulsar *self, PyObject *arg)
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
Pulsar_setPhase(Pulsar *self, PyObject *arg)
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

static PyObject *
Pulsar_setFrac(Pulsar *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->frac);
	if (isNumber == 1) {
		self->frac = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->frac = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->frac, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->frac_stream);
        self->frac_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Pulsar_members[] = {
{"server", T_OBJECT_EX, offsetof(Pulsar, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pulsar, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(Pulsar, table), 0, "Waveform table."},
{"freq", T_OBJECT_EX, offsetof(Pulsar, freq), 0, "Frequency in cycle per second."},
{"phase", T_OBJECT_EX, offsetof(Pulsar, phase), 0, "Oscillator phase."},
{"frac", T_OBJECT_EX, offsetof(Pulsar, frac), 0, "Table width inside whole length."},
{"mul", T_OBJECT_EX, offsetof(Pulsar, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Pulsar, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pulsar_methods[] = {
{"getTable", (PyCFunction)Pulsar_getTable, METH_NOARGS, "Returns waveform table object."},
{"getEnv", (PyCFunction)Pulsar_getEnv, METH_NOARGS, "Returns object envelope."},
{"getServer", (PyCFunction)Pulsar_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pulsar_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pulsar_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pulsar_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Pulsar_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Pulsar_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)Pulsar_setTable, METH_O, "Sets oscillator table."},
{"setFreq", (PyCFunction)Pulsar_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setPhase", (PyCFunction)Pulsar_setPhase, METH_O, "Sets oscillator phase."},
{"setFrac", (PyCFunction)Pulsar_setFrac, METH_O, "Sets waveform width inside whole period length."},
{"setMul", (PyCFunction)Pulsar_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Pulsar_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Pulsar_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)Pulsar_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Pulsar_as_number = {
(binaryfunc)Pulsar_add,                      /*nb_add*/
(binaryfunc)Pulsar_sub,                 /*nb_subtract*/
(binaryfunc)Pulsar_multiply,                 /*nb_multiply*/
(binaryfunc)Pulsar_div,                   /*nb_divide*/
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
(binaryfunc)Pulsar_inplace_add,              /*inplace_add*/
(binaryfunc)Pulsar_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Pulsar_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Pulsar_inplace_div,           /*inplace_divide*/
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

PyTypeObject PulsarType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pulsar_base",         /*tp_name*/
sizeof(Pulsar),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pulsar_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Pulsar_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Pulsar objects. Generates pulsar synthesis oscillator.",           /* tp_doc */
(traverseproc)Pulsar_traverse,   /* tp_traverse */
(inquiry)Pulsar_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pulsar_methods,             /* tp_methods */
Pulsar_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pulsar_init,      /* tp_init */
0,                         /* tp_alloc */
Pulsar_new,                 /* tp_new */
};

/**************/
/* TableRead object */
/**************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    int loop;
    int modebuffer[3];
    float pointerPos;
} TableRead;

static void
TableRead_readframes_i(TableRead *self) {
    float fr, inc, fpart, x, x1;
    int i, ipart;
    int go = 1;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr * size / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size) {
            if (self->loop == 1)
                self->pointerPos -= size;
            else
                go = 0;
        }
        if (go == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
        }
        else
            self->data[i] = 0.0;
    }
    if (go == 0) {
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    }    
}

static void
TableRead_readframes_a(TableRead *self) {
    float inc, fpart, x, x1, sizeOnSr;
    int i, ipart;
    int go = 1;
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    sizeOnSr = size / self->sr;
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] * sizeOnSr;
        self->pointerPos += inc;
        if (self->pointerPos < 0)
            self->pointerPos = size + self->pointerPos;
        else if (self->pointerPos >= size) {
            if (self->loop == 1)
                self->pointerPos -= size;
            else
                go = 0;
        }
        if (go == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
        }    
        else
            self->data[i] = 0.0;
    }
    if (go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
}

static void TableRead_postprocessing_ii(TableRead *self) { POST_PROCESSING_II };
static void TableRead_postprocessing_ai(TableRead *self) { POST_PROCESSING_AI };
static void TableRead_postprocessing_ia(TableRead *self) { POST_PROCESSING_IA };
static void TableRead_postprocessing_aa(TableRead *self) { POST_PROCESSING_AA };
static void TableRead_postprocessing_ireva(TableRead *self) { POST_PROCESSING_IREVA };
static void TableRead_postprocessing_areva(TableRead *self) { POST_PROCESSING_AREVA };
static void TableRead_postprocessing_revai(TableRead *self) { POST_PROCESSING_REVAI };
static void TableRead_postprocessing_revaa(TableRead *self) { POST_PROCESSING_REVAA };
static void TableRead_postprocessing_revareva(TableRead *self) { POST_PROCESSING_REVAREVA };

static void
TableRead_setProcMode(TableRead *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = TableRead_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = TableRead_readframes_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TableRead_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TableRead_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TableRead_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TableRead_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TableRead_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TableRead_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TableRead_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TableRead_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TableRead_postprocessing_revareva;
            break;
    } 
}

static void
TableRead_compute_next_data_frame(TableRead *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TableRead_traverse(TableRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
TableRead_clear(TableRead *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
TableRead_dealloc(TableRead* self)
{
    free(self->data);
    TableRead_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableRead_deleteStream(TableRead *self) { DELETE_STREAM };

static PyObject *
TableRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TableRead *self;
    self = (TableRead *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1);
    self->loop = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    self->pointerPos = 0.;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TableRead_compute_next_data_frame);
    self->mode_func_ptr = TableRead_setProcMode;
    
    return (PyObject *)self;
}

static int
TableRead_init(TableRead *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "freq", "loop", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OiOO", kwlist, &tabletmp, &freqtmp, &self->loop, &multmp, &addtmp))
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
    
    TableRead_compute_next_data_frame((TableRead *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TableRead_getServer(TableRead* self) { GET_SERVER };
static PyObject * TableRead_getStream(TableRead* self) { GET_STREAM };
static PyObject * TableRead_setMul(TableRead *self, PyObject *arg) { SET_MUL };	
static PyObject * TableRead_setAdd(TableRead *self, PyObject *arg) { SET_ADD };	
static PyObject * TableRead_setSub(TableRead *self, PyObject *arg) { SET_SUB };	
static PyObject * TableRead_setDiv(TableRead *self, PyObject *arg) { SET_DIV };	

static PyObject * TableRead_play(TableRead *self) 
{ 
    self->pointerPos = 0.0;
    PLAY 
};

static PyObject * TableRead_out(TableRead *self, PyObject *args, PyObject *kwds) 
{ 
    self->pointerPos = 0.0;
    OUT 
};
static PyObject * TableRead_stop(TableRead *self) { STOP };

static PyObject * TableRead_multiply(TableRead *self, PyObject *arg) { MULTIPLY };
static PyObject * TableRead_inplace_multiply(TableRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TableRead_add(TableRead *self, PyObject *arg) { ADD };
static PyObject * TableRead_inplace_add(TableRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TableRead_sub(TableRead *self, PyObject *arg) { SUB };
static PyObject * TableRead_inplace_sub(TableRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TableRead_div(TableRead *self, PyObject *arg) { DIV };
static PyObject * TableRead_inplace_div(TableRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TableRead_getTable(TableRead* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
TableRead_setTable(TableRead *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
TableRead_setFreq(TableRead *self, PyObject *arg)
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
TableRead_setLoop(TableRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->loop = PyInt_AsLong(arg);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef TableRead_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRead, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRead, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(TableRead, table), 0, "Waveform table."},
{"freq", T_OBJECT_EX, offsetof(TableRead, freq), 0, "Frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(TableRead, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TableRead, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRead_methods[] = {
{"getTable", (PyCFunction)TableRead_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)TableRead_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRead_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableRead_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TableRead_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TableRead_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TableRead_stop, METH_NOARGS, "Stops computing."},
{"setTable", (PyCFunction)TableRead_setTable, METH_O, "Sets oscillator table."},
{"setFreq", (PyCFunction)TableRead_setFreq, METH_O, "Sets oscillator frequency in cycle per second."},
{"setLoop", (PyCFunction)TableRead_setLoop, METH_O, "Sets the looping mode."},
{"setMul", (PyCFunction)TableRead_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)TableRead_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)TableRead_setSub, METH_O, "Sets oscillator inverse add factor."},
{"setDiv", (PyCFunction)TableRead_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TableRead_as_number = {
(binaryfunc)TableRead_add,                      /*nb_add*/
(binaryfunc)TableRead_sub,                 /*nb_subtract*/
(binaryfunc)TableRead_multiply,                 /*nb_multiply*/
(binaryfunc)TableRead_div,                   /*nb_divide*/
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
(binaryfunc)TableRead_inplace_add,              /*inplace_add*/
(binaryfunc)TableRead_inplace_sub,         /*inplace_subtract*/
(binaryfunc)TableRead_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)TableRead_inplace_div,           /*inplace_divide*/
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

PyTypeObject TableReadType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableRead_base",         /*tp_name*/
sizeof(TableRead),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableRead_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&TableRead_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TableRead objects. Generates an oscillatory waveform.",           /* tp_doc */
(traverseproc)TableRead_traverse,   /* tp_traverse */
(inquiry)TableRead_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableRead_methods,             /* tp_methods */
TableRead_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableRead_init,      /* tp_init */
0,                         /* tp_alloc */
TableRead_new,                 /* tp_new */
};
