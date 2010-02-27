#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *min;
    PyObject *max;
    PyObject *freq;
    Stream *min_stream;
    Stream *max_stream;
    Stream *freq_stream;
    float value;
    float oldValue;
    float diff;
    float time;
    int modebuffer[5]; // need at least 2 slots for mul & add 
} Randi;

static void
Randi_generate_iii(Randi *self) {
    int i;
    float inc;
    float mi = PyFloat_AS_DOUBLE(self->min);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float fr = PyFloat_AS_DOUBLE(self->freq);
    float range = ma - mi;
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aii(Randi *self) {
    int i;
    float inc, range;
    float *mi = Stream_getData((Stream *)self->min_stream);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iai(Randi *self) {
    int i;
    float inc, range;
    float mi = PyFloat_AS_DOUBLE(self->min);
    float *ma = Stream_getData((Stream *)self->max_stream);
    float fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aai(Randi *self) {
    int i;
    float inc, range;
    float *mi = Stream_getData((Stream *)self->min_stream);
    float *ma = Stream_getData((Stream *)self->max_stream);
    float fr = PyFloat_AS_DOUBLE(self->freq);
    inc = fr / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iia(Randi *self) {
    int i;
    float inc;
    float mi = PyFloat_AS_DOUBLE(self->min);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float *fr = Stream_getData((Stream *)self->freq_stream);
    float range = ma - mi;
    
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aia(Randi *self) {
    int i;
    float inc, range;
    float *mi = Stream_getData((Stream *)self->min_stream);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_iaa(Randi *self) {
    int i;
    float inc, range;
    float mi = PyFloat_AS_DOUBLE(self->min);
    float *ma = Stream_getData((Stream *)self->max_stream);
    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi;
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void
Randi_generate_aaa(Randi *self) {
    int i;
    float inc, range;
    float *mi = Stream_getData((Stream *)self->min_stream);
    float *ma = Stream_getData((Stream *)self->max_stream);
    float *fr = Stream_getData((Stream *)self->freq_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inc = fr[i] / self->sr;
        range = ma[i] - mi[i];
        self->time += inc;
        if (self->time < 0.0)
            self->time += 1.0;
        else if (self->time >= 1.0) {
            self->time -= 1.0;
            self->oldValue = self->value;
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->diff = self->value - self->oldValue;
        }
        self->data[i] = self->oldValue + self->diff * self->time;
    }
}

static void Randi_postprocessing_ii(Randi *self) { POST_PROCESSING_II };
static void Randi_postprocessing_ai(Randi *self) { POST_PROCESSING_AI };
static void Randi_postprocessing_ia(Randi *self) { POST_PROCESSING_IA };
static void Randi_postprocessing_aa(Randi *self) { POST_PROCESSING_AA };
static void Randi_postprocessing_ireva(Randi *self) { POST_PROCESSING_IREVA };
static void Randi_postprocessing_areva(Randi *self) { POST_PROCESSING_AREVA };
static void Randi_postprocessing_revai(Randi *self) { POST_PROCESSING_REVAI };
static void Randi_postprocessing_revaa(Randi *self) { POST_PROCESSING_REVAA };
static void Randi_postprocessing_revareva(Randi *self) { POST_PROCESSING_REVAREVA };

static void
Randi_setProcMode(Randi *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Randi_generate_iii;
            break;
        case 1:    
            self->proc_func_ptr = Randi_generate_aii;
            break;
        case 10:    
            self->proc_func_ptr = Randi_generate_iai;
            break;
        case 11:    
            self->proc_func_ptr = Randi_generate_aai;
            break;
        case 100:    
            self->proc_func_ptr = Randi_generate_iia;
            break;
        case 101:    
            self->proc_func_ptr = Randi_generate_aia;
            break;
        case 110:    
            self->proc_func_ptr = Randi_generate_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Randi_generate_aaa;
            break;            
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Randi_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Randi_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Randi_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Randi_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Randi_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Randi_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Randi_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Randi_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Randi_postprocessing_revareva;
            break;
    }  
}

static void
Randi_compute_next_data_frame(Randi *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Randi_traverse(Randi *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
Randi_clear(Randi *self)
{
    pyo_CLEAR
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
Randi_dealloc(Randi* self)
{
    free(self->data);
    Randi_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Randi_deleteStream(Randi *self) { DELETE_STREAM };

static PyObject *
Randi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Randi *self;
    self = (Randi *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->freq = PyFloat_FromDouble(1.);
    self->value = self->oldValue = self->diff = 0.0;
    self->time = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Randi_compute_next_data_frame);
    self->mode_func_ptr = Randi_setProcMode;
    return (PyObject *)self;
}

static int
Randi_init(Randi *self, PyObject *args, PyObject *kwds)
{
    float mi, ma;
    PyObject *mintmp=NULL, *maxtmp=NULL, *freqtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"min", "max", "freq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OOOOO", kwlist, &mintmp, &maxtmp, &freqtmp, &multmp, &addtmp))
        return -1; 

    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
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

    srand((unsigned)(time(0)));
    if (self->modebuffer[2] == 0)
        mi = PyFloat_AS_DOUBLE(self->min);
    else
        mi = Stream_getData((Stream *)self->min_stream)[0];
    if (self->modebuffer[3] == 0)
        ma = PyFloat_AS_DOUBLE(self->max);
    else
        ma = Stream_getData((Stream *)self->max_stream)[0];
    
    self->value = self->oldValue = (mi + ma) * 0.5;

    (*self->mode_func_ptr)(self);
    
    Randi_compute_next_data_frame((Randi *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Randi_getServer(Randi* self) { GET_SERVER };
static PyObject * Randi_getStream(Randi* self) { GET_STREAM };
static PyObject * Randi_setMul(Randi *self, PyObject *arg) { SET_MUL };	
static PyObject * Randi_setAdd(Randi *self, PyObject *arg) { SET_ADD };	
static PyObject * Randi_setSub(Randi *self, PyObject *arg) { SET_SUB };	
static PyObject * Randi_setDiv(Randi *self, PyObject *arg) { SET_DIV };	

static PyObject * Randi_play(Randi *self) { PLAY };
static PyObject * Randi_out(Randi *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Randi_stop(Randi *self) { STOP };

static PyObject * Randi_multiply(Randi *self, PyObject *arg) { MULTIPLY };
static PyObject * Randi_inplace_multiply(Randi *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Randi_add(Randi *self, PyObject *arg) { ADD };
static PyObject * Randi_inplace_add(Randi *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Randi_sub(Randi *self, PyObject *arg) { SUB };
static PyObject * Randi_inplace_sub(Randi *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Randi_div(Randi *self, PyObject *arg) { DIV };
static PyObject * Randi_inplace_div(Randi *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Randi_setMin(Randi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Randi_setMax(Randi *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Randi_setFreq(Randi *self, PyObject *arg)
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
        self->modebuffer[4] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Randi_members[] = {
{"server", T_OBJECT_EX, offsetof(Randi, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Randi, stream), 0, "Stream object."},
{"min", T_OBJECT_EX, offsetof(Randi, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(Randi, max), 0, "Maximum possible value."},
{"freq", T_OBJECT_EX, offsetof(Randi, freq), 0, "Polling frequency."},
{"mul", T_OBJECT_EX, offsetof(Randi, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Randi, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Randi_methods[] = {
{"getServer", (PyCFunction)Randi_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Randi_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Randi_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Randi_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Randi_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Randi_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)Randi_setMin, METH_O, "Sets minimum possible value."},
{"setMax", (PyCFunction)Randi_setMax, METH_O, "Sets maximum possible value."},
{"setFreq", (PyCFunction)Randi_setFreq, METH_O, "Sets polling frequency."},
{"setMul", (PyCFunction)Randi_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Randi_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Randi_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Randi_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Randi_as_number = {
(binaryfunc)Randi_add,                         /*nb_add*/
(binaryfunc)Randi_sub,                         /*nb_subtract*/
(binaryfunc)Randi_multiply,                    /*nb_multiply*/
(binaryfunc)Randi_div,                                              /*nb_divide*/
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
(binaryfunc)Randi_inplace_add,                 /*inplace_add*/
(binaryfunc)Randi_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Randi_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Randi_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject RandiType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Randi_base",                                   /*tp_name*/
sizeof(Randi),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Randi_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Randi_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Randi objects. Periodically generates a new random value with interpolation.",           /* tp_doc */
(traverseproc)Randi_traverse,                  /* tp_traverse */
(inquiry)Randi_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Randi_methods,                                 /* tp_methods */
Randi_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Randi_init,                          /* tp_init */
0,                                              /* tp_alloc */
Randi_new,                                     /* tp_new */
};

