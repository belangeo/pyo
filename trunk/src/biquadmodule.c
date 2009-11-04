#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"

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
        self->x1 = self->x2 = self->y1 = self->y2 = in[i];
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
        self->x1 = self->x2 = self->y1 = self->y2 = in[i];
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
        self->x1 = self->x2 = self->y1 = self->y2 = in[i];
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
        self->x1 = self->x2 = self->y1 = self->y2 = in[i];
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

static void
_setProcMode(Biquad *self)
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
        case 10:        
            self->muladd_func_ptr = Biquad_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Biquad_postprocessing_aa;
            break;
    }    
}

static void
_compute_next_data_frame(Biquad *self)
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

    _setProcMode(self);

    _compute_next_data_frame((Biquad *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Biquad_getServer(Biquad* self) { GET_SERVER };
static PyObject * Biquad_getStream(Biquad* self) { GET_STREAM };
static PyObject * Biquad_setMul(Biquad *self, PyObject *arg) { SET_MUL };	
static PyObject * Biquad_setAdd(Biquad *self, PyObject *arg) { SET_ADD };	

static PyObject * Biquad_play(Biquad *self) { PLAY };
static PyObject * Biquad_out(Biquad *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Biquad_stop(Biquad *self) { STOP };

static PyObject *
Biquad_multiply(Biquad *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Biquad_inplace_multiply(Biquad *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}


static PyObject *
Biquad_add(Biquad *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Biquad_inplace_add(Biquad *self, PyObject *arg)
{
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg);
    Py_INCREF(self);
    return (PyObject *)self;
}

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
    
    _setProcMode(self);
    
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
    
    _setProcMode(self);
    
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

    _setProcMode(self);
    
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
    //{"getInput", (PyCFunction)Biquad_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Biquad_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Biquad_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Biquad_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Biquad_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Biquad_stop, METH_NOARGS, "Stops computing."},
	{"setFreq", (PyCFunction)Biquad_setFreq, METH_O, "Sets filter cutoff frequency in cycle per second."},
    {"setQ", (PyCFunction)Biquad_setQ, METH_O, "Sets filter Q factor."},
    {"setType", (PyCFunction)Biquad_setType, METH_O, "Sets filter type factor."},
	{"setMul", (PyCFunction)Biquad_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Biquad_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Biquad_as_number = {
    (binaryfunc)Biquad_add,                         /*nb_add*/
    0,                                              /*nb_subtract*/
    (binaryfunc)Biquad_multiply,                    /*nb_multiply*/
    0,                                              /*nb_divide*/
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
    0,                                              /*inplace_subtract*/
    (binaryfunc)Biquad_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_divide*/
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
    "pyo.Biquad",                                   /*tp_name*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
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

