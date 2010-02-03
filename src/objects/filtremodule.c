#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *q;
    Stream *q_stream;
    void (*coeffs_func_ptr)();
    int init;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    int filtertype;
    // sample memories
    float x1;
    float x2;
    float y1;
    float y2;
    // variables
    float c;
    float w0;
    float alpha;
    // coefficients
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;
} Biquad;

static void 
Biquad_compute_coeffs_lp(Biquad *self)
{
    self->b0 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_hp(Biquad *self)
{
    self->b0 = (1 + self->c) / 2;
    self->b1 = -(1 + self->c);
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_bp(Biquad *self)
{
    self->b0 = self->alpha;
    self->b1 = 0;
    self->b2 = -self->alpha;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void 
Biquad_compute_coeffs_bs(Biquad *self)
{
    self->b0 = 1;
    self->b1 = -2 * self->c;
    self->b2 = 1;
    self->a0 = 1 + self->alpha;
    self->a1 = self->b1;
    self->a2 = 1 - self->alpha;
}

static void
Biquad_compute_variables(Biquad *self, float freq, float q)
{
    //float w0, c, alpha;
    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->w0 = TWOPI * freq / self->sr;
    self->c = cosf(self->w0);
    self->alpha = sinf(self->w0) / (2 * q);
    (*self->coeffs_func_ptr)(self);
}

static void
Biquad_filters_ii(Biquad *self) {
    float val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ai(Biquad *self) {
    float val, q;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    float *fr = Stream_getData((Stream *)self->freq_stream);
    q = PyFloat_AS_DOUBLE(self->q);
    
    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_ia(Biquad *self) {
    float val, fr;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }
    
    fr = PyFloat_AS_DOUBLE(self->freq);
    float *q = Stream_getData((Stream *)self->q_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr, q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void
Biquad_filters_aa(Biquad *self) {
    float val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->init == 1) {
        self->x1 = self->x2 = self->y1 = self->y2 = in[0];
        self->init = 0;
    }

    float *fr = Stream_getData((Stream *)self->freq_stream);
    float *q = Stream_getData((Stream *)self->q_stream);

    for (i=0; i<self->bufsize; i++) {
        Biquad_compute_variables(self, fr[i], q[i]);
        val = ( (self->b0 * in[i]) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = in[i];
        self->data[i] = val;
    }
}

static void Biquad_postprocessing_ii(Biquad *self) { POST_PROCESSING_II };
static void Biquad_postprocessing_ai(Biquad *self) { POST_PROCESSING_AI };
static void Biquad_postprocessing_ia(Biquad *self) { POST_PROCESSING_IA };
static void Biquad_postprocessing_aa(Biquad *self) { POST_PROCESSING_AA };
static void Biquad_postprocessing_ireva(Biquad *self) { POST_PROCESSING_IREVA };
static void Biquad_postprocessing_areva(Biquad *self) { POST_PROCESSING_AREVA };
static void Biquad_postprocessing_revai(Biquad *self) { POST_PROCESSING_REVAI };
static void Biquad_postprocessing_revaa(Biquad *self) { POST_PROCESSING_REVAA };
static void Biquad_postprocessing_revareva(Biquad *self) { POST_PROCESSING_REVAREVA };

static void
Biquad_setProcMode(Biquad *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (self->filtertype) {
        case 0:
            self->coeffs_func_ptr = Biquad_compute_coeffs_lp;
            break;
        case 1:
            self->coeffs_func_ptr = Biquad_compute_coeffs_hp;
            break;
        case 2:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bp;
            break;
        case 3:
            self->coeffs_func_ptr = Biquad_compute_coeffs_bs;
            break;
    }
    
	switch (procmode) {
        case 0:    
            Biquad_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), PyFloat_AS_DOUBLE(self->q));
            self->proc_func_ptr = Biquad_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Biquad_filters_ai;
            break;
        case 10:        
            self->proc_func_ptr = Biquad_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Biquad_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Biquad_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Biquad_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Biquad_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Biquad_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Biquad_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Biquad_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Biquad_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Biquad_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Biquad_postprocessing_revareva;
            break;
    }   
}

static void
Biquad_compute_next_data_frame(Biquad *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Biquad_traverse(Biquad *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->q);    
    Py_VISIT(self->q_stream);    
    return 0;
}

static int 
Biquad_clear(Biquad *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->q);    
    Py_CLEAR(self->q_stream);    
    return 0;
}

static void
Biquad_dealloc(Biquad* self)
{
    free(self->data);
    Biquad_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Biquad_deleteStream(Biquad *self) { DELETE_STREAM };

static PyObject *
Biquad_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Biquad *self;
    self = (Biquad *)type->tp_alloc(type, 0);
        
    self->freq = PyFloat_FromDouble(1000);
    self->q = PyFloat_FromDouble(1);
    self->filtertype = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Biquad_compute_next_data_frame);
    self->mode_func_ptr = Biquad_setProcMode;
    return (PyObject *)self;
}

static int
Biquad_init(Biquad *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *qtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "freq", "q", "type", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiOO", kwlist, &inputtmp, &freqtmp, &qtmp, &self->filtertype, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (qtmp) {
        PyObject_CallMethod((PyObject *)self, "setQ", "O", qtmp);
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

    Biquad_compute_next_data_frame((Biquad *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Biquad_getServer(Biquad* self) { GET_SERVER };
static PyObject * Biquad_getStream(Biquad* self) { GET_STREAM };
static PyObject * Biquad_setMul(Biquad *self, PyObject *arg) { SET_MUL };	
static PyObject * Biquad_setAdd(Biquad *self, PyObject *arg) { SET_ADD };	
static PyObject * Biquad_setSub(Biquad *self, PyObject *arg) { SET_SUB };	
static PyObject * Biquad_setDiv(Biquad *self, PyObject *arg) { SET_DIV };	

static PyObject * Biquad_play(Biquad *self) { PLAY };
static PyObject * Biquad_out(Biquad *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquad_stop(Biquad *self) { STOP };

static PyObject * Biquad_multiply(Biquad *self, PyObject *arg) { MULTIPLY };
static PyObject * Biquad_inplace_multiply(Biquad *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Biquad_add(Biquad *self, PyObject *arg) { ADD };
static PyObject * Biquad_inplace_add(Biquad *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Biquad_sub(Biquad *self, PyObject *arg) { SUB };
static PyObject * Biquad_inplace_sub(Biquad *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Biquad_div(Biquad *self, PyObject *arg) { DIV };
static PyObject * Biquad_inplace_div(Biquad *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Biquad_setFreq(Biquad *self, PyObject *arg)
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
Biquad_setQ(Biquad *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->q);
	if (isNumber == 1) {
		self->q = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->q = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->q, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->q_stream);
        self->q_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Biquad_setType(Biquad *self, PyObject *arg)
{
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
	}

    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Biquad_members[] = {
    {"server", T_OBJECT_EX, offsetof(Biquad, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Biquad, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Biquad, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(Biquad, freq), 0, "Cutoff frequency in cycle per second."},
    {"q", T_OBJECT_EX, offsetof(Biquad, q), 0, "Q factor."},
    {"mul", T_OBJECT_EX, offsetof(Biquad, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Biquad, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Biquad_methods[] = {
    {"getServer", (PyCFunction)Biquad_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquad_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Biquad_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Biquad_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquad_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquad_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquad_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquad_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquad_setType, METH_O, "Sets filter type factor."},
	{"setMul", (PyCFunction)Biquad_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquad_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Biquad_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Biquad_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquad_as_number = {
    (binaryfunc)Biquad_add,                         /*nb_add*/
    (binaryfunc)Biquad_sub,                         /*nb_subtract*/
    (binaryfunc)Biquad_multiply,                    /*nb_multiply*/
    (binaryfunc)Biquad_div,                                              /*nb_divide*/
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
    (binaryfunc)Biquad_inplace_add,                 /*inplace_add*/
    (binaryfunc)Biquad_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)Biquad_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)Biquad_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject BiquadType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.Biquad_base",                                   /*tp_name*/
    sizeof(Biquad),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Biquad_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Biquad_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Biquad objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Biquad_traverse,                  /* tp_traverse */
    (inquiry)Biquad_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Biquad_methods,                                 /* tp_methods */
    Biquad_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Biquad_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Biquad_new,                                     /* tp_new */
};

/* Performs portamento on audio signal */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    Stream *risetime_stream;
    Stream *falltime_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add 
    float y1; // sample memory
    float x1;
    int dir;
} Port;

static void 
direction(Port *self, float val)
{
    if (val == self->x1)
        return;
    
    if (val > self->x1) {
        self->x1 = val;
        self->dir = 1;
    }    
    if (val < self->x1) {
        self->x1 = val;
        self->dir = 0;
    }
}    
    
static void
Port_filters_ii(Port *self) {
    float val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float risetime = PyFloat_AS_DOUBLE(self->risetime);
    float falltime = PyFloat_AS_DOUBLE(self->falltime);
    float risefactor = 1. / (risetime * self->sr);
    float fallfactor = 1. / (falltime * self->sr);
    float factors[2] = {fallfactor, risefactor};

    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        val = self->y1 + (in[i] - self->y1) * factors[self->dir];
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ai(Port *self) {
    float val, risefactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *risetime = Stream_getData((Stream *)self->risetime_stream);
    float falltime = PyFloat_AS_DOUBLE(self->falltime);
    float fallfactor = 1. / (falltime * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = *risetime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) * fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_ia(Port *self) {
    float val, fallfactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *falltime = Stream_getData((Stream *)self->falltime_stream);
    float risetime = PyFloat_AS_DOUBLE(self->risetime);
    float risefactor = 1. / (risetime * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        fallfactor = *falltime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) * risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Port_filters_aa(Port *self) {
    float val, risefactor, fallfactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *risetime = Stream_getData((Stream *)self->risetime_stream);
    float *falltime = Stream_getData((Stream *)self->falltime_stream);
    
    for (i=0; i<self->bufsize; i++) {
        direction(self, in[i]);
        risefactor = *risetime++ * self->sr;  
        fallfactor = *falltime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (*in++ - self->y1) / risefactor;
        else
            val = self->y1 + (*in++ - self->y1) / fallfactor;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void Port_postprocessing_ii(Port *self) { POST_PROCESSING_II };
static void Port_postprocessing_ai(Port *self) { POST_PROCESSING_AI };
static void Port_postprocessing_ia(Port *self) { POST_PROCESSING_IA };
static void Port_postprocessing_aa(Port *self) { POST_PROCESSING_AA };
static void Port_postprocessing_ireva(Port *self) { POST_PROCESSING_IREVA };
static void Port_postprocessing_areva(Port *self) { POST_PROCESSING_AREVA };
static void Port_postprocessing_revai(Port *self) { POST_PROCESSING_REVAI };
static void Port_postprocessing_revaa(Port *self) { POST_PROCESSING_REVAA };
static void Port_postprocessing_revareva(Port *self) { POST_PROCESSING_REVAREVA };

static void
Port_setProcMode(Port *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Port_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Port_filters_ai;
            break;
        case 10:    
            self->proc_func_ptr = Port_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Port_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Port_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Port_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Port_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Port_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Port_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Port_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Port_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Port_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Port_postprocessing_revareva;
            break;
    }  
}

static void
Port_compute_next_data_frame(Port *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Port_traverse(Port *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);    
    Py_VISIT(self->risetime_stream);    
    Py_VISIT(self->falltime);    
    Py_VISIT(self->falltime_stream);    
    return 0;
}

static int 
Port_clear(Port *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);    
    Py_CLEAR(self->risetime_stream);    
    Py_CLEAR(self->falltime);    
    Py_CLEAR(self->falltime_stream);    
    return 0;
}

static void
Port_dealloc(Port* self)
{
    free(self->data);
    Port_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Port_deleteStream(Port *self) { DELETE_STREAM };

static PyObject *
Port_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Port *self;
    self = (Port *)type->tp_alloc(type, 0);
    
    self->risetime = PyFloat_FromDouble(0.05);
    self->falltime = PyFloat_FromDouble(0.05);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->y1 = 0.0;
    self->x1 = 0.0;
    self->dir = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Port_compute_next_data_frame);
    self->mode_func_ptr = Port_setProcMode;
    return (PyObject *)self;
}

static int
Port_init(Port *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    float inittmp = 0.0;
    
    static char *kwlist[] = {"input", "risetime", "falltime", "init", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &risetimetmp, &falltimetmp, &inittmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (risetimetmp) {
        PyObject_CallMethod((PyObject *)self, "setRiseTime", "O", risetimetmp);
    }

    if (falltimetmp) {
        PyObject_CallMethod((PyObject *)self, "setFallTime", "O", falltimetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    if (inittmp != 0.0)
        self->x1 = self->y1 = inittmp;
        
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Port_compute_next_data_frame((Port *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Port_getServer(Port* self) { GET_SERVER };
static PyObject * Port_getStream(Port* self) { GET_STREAM };
static PyObject * Port_setMul(Port *self, PyObject *arg) { SET_MUL };	
static PyObject * Port_setAdd(Port *self, PyObject *arg) { SET_ADD };	
static PyObject * Port_setSub(Port *self, PyObject *arg) { SET_SUB };	
static PyObject * Port_setDiv(Port *self, PyObject *arg) { SET_DIV };	

static PyObject * Port_play(Port *self) { PLAY };
static PyObject * Port_out(Port *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Port_stop(Port *self) { STOP };

static PyObject * Port_multiply(Port *self, PyObject *arg) { MULTIPLY };
static PyObject * Port_inplace_multiply(Port *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Port_add(Port *self, PyObject *arg) { ADD };
static PyObject * Port_inplace_add(Port *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Port_sub(Port *self, PyObject *arg) { SUB };
static PyObject * Port_inplace_sub(Port *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Port_div(Port *self, PyObject *arg) { DIV };
static PyObject * Port_inplace_div(Port *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Port_setRiseTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->risetime);
	if (isNumber == 1) {
		self->risetime = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->risetime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->risetime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->risetime_stream);
        self->risetime_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Port_setFallTime(Port *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->falltime);
	if (isNumber == 1) {
		self->falltime = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->falltime = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->falltime, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->falltime_stream);
        self->falltime_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Port_members[] = {
{"server", T_OBJECT_EX, offsetof(Port, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Port, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Port, input), 0, "Input sound object."},
{"risetime", T_OBJECT_EX, offsetof(Port, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Port, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Port, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Port, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Port_methods[] = {
{"getServer", (PyCFunction)Port_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Port_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Port_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Port_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Port_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Port_stop, METH_NOARGS, "Stops computing."},
{"setRiseTime", (PyCFunction)Port_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Port_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setMul", (PyCFunction)Port_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Port_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Port_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Port_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Port_as_number = {
(binaryfunc)Port_add,                         /*nb_add*/
(binaryfunc)Port_sub,                         /*nb_subtract*/
(binaryfunc)Port_multiply,                    /*nb_multiply*/
(binaryfunc)Port_div,                                              /*nb_divide*/
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
(binaryfunc)Port_inplace_add,                 /*inplace_add*/
(binaryfunc)Port_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Port_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Port_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject PortType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Port_base",                                   /*tp_name*/
sizeof(Port),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Port_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Port_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Port objects. Generates a portamento filter.",           /* tp_doc */
(traverseproc)Port_traverse,                  /* tp_traverse */
(inquiry)Port_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Port_methods,                                 /* tp_methods */
Port_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Port_init,                          /* tp_init */
0,                                              /* tp_alloc */
Port_new,                                     /* tp_new */
};

/************/
/* Follower */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add 
    // sample memories
    float x1;
    float x2;
    float y1;
    float y2;
    // variables
    float c;
    float w0;
    float alpha;
    // coefficients
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;
} Follower;

static void 
Follower_compute_coeffs_lp(Follower *self)
{
    self->b0 = (1 - self->c) / 2;
    self->b1 = 1 - self->c;
    self->b2 = self->b0;
    self->a0 = 1 + self->alpha;
    self->a1 = -2 * self->c;
    self->a2 = 1 - self->alpha;
}

static void
Follower_compute_variables(Follower *self, float freq, float q)
{
    //float w0, c, alpha;
    
    if (freq <= 1) 
        freq = 1;
    else if (freq >= self->sr)
        freq = self->sr;
    
    self->w0 = TWOPI * freq / self->sr;
    self->c = cosf(self->w0);
    self->alpha = sinf(self->w0) / (2 * q);
    Follower_compute_coeffs_lp((Follower *)self);
}

static void
Follower_filters_i(Follower *self) {
    float absin, val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        val = ( (self->b0 * absin) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = absin;
        self->data[i] = val;
    }
}

static void
Follower_filters_a(Follower *self) {
    float val, absin;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        Follower_compute_variables(self, fr[i], 1.0);
        absin = in[i] * in[i];
        val = ( (self->b0 * absin) + (self->b1 * self->x1) + (self->b2 * self->x2) - (self->a1 * self->y1) - (self->a2 * self->y2) ) / self->a0;
        self->y2 = self->y1;
        self->y1 = val;
        self->x2 = self->x1;
        self->x1 = absin;
        self->data[i] = val;
    }
}

static void Follower_postprocessing_ii(Follower *self) { POST_PROCESSING_II };
static void Follower_postprocessing_ai(Follower *self) { POST_PROCESSING_AI };
static void Follower_postprocessing_ia(Follower *self) { POST_PROCESSING_IA };
static void Follower_postprocessing_aa(Follower *self) { POST_PROCESSING_AA };
static void Follower_postprocessing_ireva(Follower *self) { POST_PROCESSING_IREVA };
static void Follower_postprocessing_areva(Follower *self) { POST_PROCESSING_AREVA };
static void Follower_postprocessing_revai(Follower *self) { POST_PROCESSING_REVAI };
static void Follower_postprocessing_revaa(Follower *self) { POST_PROCESSING_REVAA };
static void Follower_postprocessing_revareva(Follower *self) { POST_PROCESSING_REVAREVA };

static void
Follower_setProcMode(Follower *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            Follower_compute_variables(self, PyFloat_AS_DOUBLE(self->freq), 1.0);
            self->proc_func_ptr = Follower_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = Follower_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Follower_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Follower_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Follower_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Follower_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Follower_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Follower_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Follower_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Follower_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Follower_postprocessing_revareva;
            break;
    }   
}

static void
Follower_compute_next_data_frame(Follower *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Follower_traverse(Follower *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Follower_clear(Follower *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Follower_dealloc(Follower* self)
{
    free(self->data);
    Follower_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Follower_deleteStream(Follower *self) { DELETE_STREAM };

static PyObject *
Follower_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Follower *self;
    self = (Follower *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->x1 = self->x2 = self->y1 = self->y2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Follower_compute_next_data_frame);
    self->mode_func_ptr = Follower_setProcMode;
    return (PyObject *)self;
}

static int
Follower_init(Follower *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
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
    
    Follower_compute_next_data_frame((Follower *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Follower_getServer(Follower* self) { GET_SERVER };
static PyObject * Follower_getStream(Follower* self) { GET_STREAM };
static PyObject * Follower_setMul(Follower *self, PyObject *arg) { SET_MUL };	
static PyObject * Follower_setAdd(Follower *self, PyObject *arg) { SET_ADD };	
static PyObject * Follower_setSub(Follower *self, PyObject *arg) { SET_SUB };	
static PyObject * Follower_setDiv(Follower *self, PyObject *arg) { SET_DIV };	

static PyObject * Follower_play(Follower *self) { PLAY };
static PyObject * Follower_stop(Follower *self) { STOP };

static PyObject * Follower_multiply(Follower *self, PyObject *arg) { MULTIPLY };
static PyObject * Follower_inplace_multiply(Follower *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Follower_add(Follower *self, PyObject *arg) { ADD };
static PyObject * Follower_inplace_add(Follower *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Follower_sub(Follower *self, PyObject *arg) { SUB };
static PyObject * Follower_inplace_sub(Follower *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Follower_div(Follower *self, PyObject *arg) { DIV };
static PyObject * Follower_inplace_div(Follower *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Follower_setFreq(Follower *self, PyObject *arg)
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

static PyMemberDef Follower_members[] = {
{"server", T_OBJECT_EX, offsetof(Follower, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Follower, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Follower, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Follower, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Follower, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Follower, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Follower_methods[] = {
{"getServer", (PyCFunction)Follower_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Follower_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Follower_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Follower_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Follower_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Follower_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Follower_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Follower_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Follower_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Follower_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Follower_as_number = {
(binaryfunc)Follower_add,                         /*nb_add*/
(binaryfunc)Follower_sub,                         /*nb_subtract*/
(binaryfunc)Follower_multiply,                    /*nb_multiply*/
(binaryfunc)Follower_div,                                              /*nb_divide*/
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
(binaryfunc)Follower_inplace_add,                 /*inplace_add*/
(binaryfunc)Follower_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Follower_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Follower_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject FollowerType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Follower_base",                                   /*tp_name*/
sizeof(Follower),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Follower_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Follower_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Follower objects. Envelope follower.",           /* tp_doc */
(traverseproc)Follower_traverse,                  /* tp_traverse */
(inquiry)Follower_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Follower_methods,                                 /* tp_methods */
Follower_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Follower_init,                          /* tp_init */
0,                                              /* tp_alloc */
Follower_new,                                     /* tp_new */
};

/************/
/* Tone */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    int modebuffer[3]; // need at least 2 slots for mul & add 
    float lastFreq;
    // sample memories
    float y1;
    // variables
    float c1;
    float c2;
} Tone;

static void
Tone_filters_i(Tone *self) {
    float val, b;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float fr = PyFloat_AS_DOUBLE(self->freq);
    
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        b = 2.0 - cosf(TWOPI * fr / self->sr);
        self->c2 = (b - sqrtf(b * b - 1.0));
        self->c1 = 1.0 - self->c2;
    }
    
    for (i=0; i<self->bufsize; i++) {
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void
Tone_filters_a(Tone *self) {
    float val, freq, b;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *fr = Stream_getData((Stream *)self->freq_stream);
        
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            b = 2.0 - cosf(TWOPI * freq / self->sr);
            self->c2 = (b - sqrtf(b * b - 1.0));
            self->c1 = 1.0 - self->c2;
        }
        val = self->c1 * in[i] + self->c2 * self->y1;
        self->data[i] = val;
        self->y1 = val;
    }
}

static void Tone_postprocessing_ii(Tone *self) { POST_PROCESSING_II };
static void Tone_postprocessing_ai(Tone *self) { POST_PROCESSING_AI };
static void Tone_postprocessing_ia(Tone *self) { POST_PROCESSING_IA };
static void Tone_postprocessing_aa(Tone *self) { POST_PROCESSING_AA };
static void Tone_postprocessing_ireva(Tone *self) { POST_PROCESSING_IREVA };
static void Tone_postprocessing_areva(Tone *self) { POST_PROCESSING_AREVA };
static void Tone_postprocessing_revai(Tone *self) { POST_PROCESSING_REVAI };
static void Tone_postprocessing_revaa(Tone *self) { POST_PROCESSING_REVAA };
static void Tone_postprocessing_revareva(Tone *self) { POST_PROCESSING_REVAREVA };

static void
Tone_setProcMode(Tone *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Tone_filters_i;
            break;
        case 1:    
            self->proc_func_ptr = Tone_filters_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Tone_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Tone_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Tone_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Tone_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Tone_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Tone_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Tone_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Tone_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Tone_postprocessing_revareva;
            break;
    }   
}

static void
Tone_compute_next_data_frame(Tone *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Tone_traverse(Tone *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    return 0;
}

static int 
Tone_clear(Tone *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    return 0;
}

static void
Tone_dealloc(Tone* self)
{
    free(self->data);
    Tone_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Tone_deleteStream(Tone *self) { DELETE_STREAM };

static PyObject *
Tone_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Tone *self;
    self = (Tone *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(1000);
    self->lastFreq = -1.0;
    self->y1 = self->c1 = self->c2 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Tone_compute_next_data_frame);
    self->mode_func_ptr = Tone_setProcMode;
    return (PyObject *)self;
}

static int
Tone_init(Tone *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "freq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOO", kwlist, &inputtmp, &freqtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
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
    
    Tone_compute_next_data_frame((Tone *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Tone_getServer(Tone* self) { GET_SERVER };
static PyObject * Tone_getStream(Tone* self) { GET_STREAM };
static PyObject * Tone_setMul(Tone *self, PyObject *arg) { SET_MUL };	
static PyObject * Tone_setAdd(Tone *self, PyObject *arg) { SET_ADD };	
static PyObject * Tone_setSub(Tone *self, PyObject *arg) { SET_SUB };	
static PyObject * Tone_setDiv(Tone *self, PyObject *arg) { SET_DIV };	

static PyObject * Tone_play(Tone *self) { PLAY };
static PyObject * Tone_out(Tone *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Tone_stop(Tone *self) { STOP };

static PyObject * Tone_multiply(Tone *self, PyObject *arg) { MULTIPLY };
static PyObject * Tone_inplace_multiply(Tone *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Tone_add(Tone *self, PyObject *arg) { ADD };
static PyObject * Tone_inplace_add(Tone *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Tone_sub(Tone *self, PyObject *arg) { SUB };
static PyObject * Tone_inplace_sub(Tone *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Tone_div(Tone *self, PyObject *arg) { DIV };
static PyObject * Tone_inplace_div(Tone *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Tone_setFreq(Tone *self, PyObject *arg)
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

static PyMemberDef Tone_members[] = {
{"server", T_OBJECT_EX, offsetof(Tone, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Tone, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Tone, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Tone, freq), 0, "Cutoff frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(Tone, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Tone, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Tone_methods[] = {
{"getServer", (PyCFunction)Tone_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Tone_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Tone_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Tone_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Tone_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Tone_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Tone_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
{"setMul", (PyCFunction)Tone_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Tone_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Tone_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Tone_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Tone_as_number = {
(binaryfunc)Tone_add,                         /*nb_add*/
(binaryfunc)Tone_sub,                         /*nb_subtract*/
(binaryfunc)Tone_multiply,                    /*nb_multiply*/
(binaryfunc)Tone_div,                                              /*nb_divide*/
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
(binaryfunc)Tone_inplace_add,                 /*inplace_add*/
(binaryfunc)Tone_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Tone_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Tone_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject ToneType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Tone_base",                                   /*tp_name*/
sizeof(Tone),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Tone_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Tone_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Tone objects. One-pole recursive lowpass filter.",           /* tp_doc */
(traverseproc)Tone_traverse,                  /* tp_traverse */
(inquiry)Tone_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Tone_methods,                                 /* tp_methods */
Tone_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Tone_init,                          /* tp_init */
0,                                              /* tp_alloc */
Tone_new,                                     /* tp_new */
};

/************/
/* DCBlock */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
    // sample memories
    float x1;
    float y1;
} DCBlock;

static void
DCBlock_filters(DCBlock *self) {
    float x, y;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        x = in[i];
        y = x - self->x1 + 0.995 * self->y1;
        self->x1 = x;
        self->data[i] = self->y1 = y;
    }
}

static void DCBlock_postprocessing_ii(DCBlock *self) { POST_PROCESSING_II };
static void DCBlock_postprocessing_ai(DCBlock *self) { POST_PROCESSING_AI };
static void DCBlock_postprocessing_ia(DCBlock *self) { POST_PROCESSING_IA };
static void DCBlock_postprocessing_aa(DCBlock *self) { POST_PROCESSING_AA };
static void DCBlock_postprocessing_ireva(DCBlock *self) { POST_PROCESSING_IREVA };
static void DCBlock_postprocessing_areva(DCBlock *self) { POST_PROCESSING_AREVA };
static void DCBlock_postprocessing_revai(DCBlock *self) { POST_PROCESSING_REVAI };
static void DCBlock_postprocessing_revaa(DCBlock *self) { POST_PROCESSING_REVAA };
static void DCBlock_postprocessing_revareva(DCBlock *self) { POST_PROCESSING_REVAREVA };

static void
DCBlock_setProcMode(DCBlock *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
       
    self->proc_func_ptr = DCBlock_filters;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = DCBlock_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = DCBlock_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = DCBlock_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = DCBlock_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = DCBlock_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = DCBlock_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = DCBlock_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = DCBlock_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = DCBlock_postprocessing_revareva;
            break;
    }   
}

static void
DCBlock_compute_next_data_frame(DCBlock *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
DCBlock_traverse(DCBlock *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
DCBlock_clear(DCBlock *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
DCBlock_dealloc(DCBlock* self)
{
    free(self->data);
    DCBlock_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * DCBlock_deleteStream(DCBlock *self) { DELETE_STREAM };

static PyObject *
DCBlock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    DCBlock *self;
    self = (DCBlock *)type->tp_alloc(type, 0);
    
    self->x1 = self->y1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, DCBlock_compute_next_data_frame);
    self->mode_func_ptr = DCBlock_setProcMode;
    return (PyObject *)self;
}

static int
DCBlock_init(DCBlock *self, PyObject *args, PyObject *kwds)
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
    
    DCBlock_compute_next_data_frame((DCBlock *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * DCBlock_getServer(DCBlock* self) { GET_SERVER };
static PyObject * DCBlock_getStream(DCBlock* self) { GET_STREAM };
static PyObject * DCBlock_setMul(DCBlock *self, PyObject *arg) { SET_MUL };	
static PyObject * DCBlock_setAdd(DCBlock *self, PyObject *arg) { SET_ADD };	
static PyObject * DCBlock_setSub(DCBlock *self, PyObject *arg) { SET_SUB };	
static PyObject * DCBlock_setDiv(DCBlock *self, PyObject *arg) { SET_DIV };	

static PyObject * DCBlock_play(DCBlock *self) { PLAY };
static PyObject * DCBlock_out(DCBlock *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * DCBlock_stop(DCBlock *self) { STOP };

static PyObject * DCBlock_multiply(DCBlock *self, PyObject *arg) { MULTIPLY };
static PyObject * DCBlock_inplace_multiply(DCBlock *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * DCBlock_add(DCBlock *self, PyObject *arg) { ADD };
static PyObject * DCBlock_inplace_add(DCBlock *self, PyObject *arg) { INPLACE_ADD };
static PyObject * DCBlock_sub(DCBlock *self, PyObject *arg) { SUB };
static PyObject * DCBlock_inplace_sub(DCBlock *self, PyObject *arg) { INPLACE_SUB };
static PyObject * DCBlock_div(DCBlock *self, PyObject *arg) { DIV };
static PyObject * DCBlock_inplace_div(DCBlock *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef DCBlock_members[] = {
{"server", T_OBJECT_EX, offsetof(DCBlock, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(DCBlock, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(DCBlock, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(DCBlock, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(DCBlock, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef DCBlock_methods[] = {
{"getServer", (PyCFunction)DCBlock_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)DCBlock_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)DCBlock_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)DCBlock_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)DCBlock_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)DCBlock_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)DCBlock_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)DCBlock_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)DCBlock_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)DCBlock_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods DCBlock_as_number = {
(binaryfunc)DCBlock_add,                         /*nb_add*/
(binaryfunc)DCBlock_sub,                         /*nb_subtract*/
(binaryfunc)DCBlock_multiply,                    /*nb_multiply*/
(binaryfunc)DCBlock_div,                                              /*nb_divide*/
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
(binaryfunc)DCBlock_inplace_add,                 /*inplace_add*/
(binaryfunc)DCBlock_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)DCBlock_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)DCBlock_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject DCBlockType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.DCBlock_base",                                   /*tp_name*/
sizeof(DCBlock),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)DCBlock_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&DCBlock_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"DCBlock objects. Implements the DC blocking filter.",           /* tp_doc */
(traverseproc)DCBlock_traverse,                  /* tp_traverse */
(inquiry)DCBlock_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
DCBlock_methods,                                 /* tp_methods */
DCBlock_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)DCBlock_init,                          /* tp_init */
0,                                              /* tp_alloc */
DCBlock_new,                                     /* tp_new */
};
