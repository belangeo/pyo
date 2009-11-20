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
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    float maxdelay;
    long size;
    int in_count;
    int modebuffer[4];
    float *buffer; // samples memory
} Delay;

static void
Delay_process_ii(Delay *self) {
    float val, x, x1, xind, frac;
    int i, ind;

    float del = PyFloat_AS_DOUBLE(self->delay);
    float feed = PyFloat_AS_DOUBLE(self->feedback);
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    float sampdel = del * self->sr;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        x = self->buffer[ind];
        x1 = self->buffer[ind+1];
        val = x + (x1 - x) * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count++] = in[i] + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ai(Delay *self) {
    float val, x, x1, xind, frac, sampdel, del;
    int i, ind;

    float *delobj = Stream_getData((Stream *)self->delay_stream);    
    float feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        x = self->buffer[ind];
        x1 = self->buffer[ind+1];
        val = x + (x1 - x) * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count++] = in[i]  + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ia(Delay *self) {
    float val, x, x1, xind, frac, feed;
    int i, ind;
    
    float del = PyFloat_AS_DOUBLE(self->delay);
    float *fdb = Stream_getData((Stream *)self->feedback_stream);    
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    float sampdel = del * self->sr;
       
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        x = self->buffer[ind];
        x1 = self->buffer[ind+1];
        val = x + (x1 - x) * frac;
        self->data[i] = val;

        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count++] = in[i] + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_aa(Delay *self) {
    float val, x, x1, xind, frac, sampdel, feed, del;
    int i, ind;
    
    float *delobj = Stream_getData((Stream *)self->delay_stream);    
    float *fdb = Stream_getData((Stream *)self->feedback_stream);    
  
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (int)xind;
        frac = xind - ind;
        x = self->buffer[ind];
        x1 = self->buffer[ind+1];
        val = x + (x1 - x) * frac;
        self->data[i] = val;
        
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count++] = in[i] + (val * feed);
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Delay_postprocessing_ii(Delay *self) { POST_PROCESSING_II };
static void Delay_postprocessing_ai(Delay *self) { POST_PROCESSING_AI };
static void Delay_postprocessing_ia(Delay *self) { POST_PROCESSING_IA };
static void Delay_postprocessing_aa(Delay *self) { POST_PROCESSING_AA };

static void
Delay_setProcMode(Delay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Delay_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Delay_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Delay_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Delay_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Delay_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Delay_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Delay_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Delay_postprocessing_aa;
            break;
    }    
}

static void
Delay_compute_next_data_frame(Delay *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Delay_traverse(Delay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->delay);    
    Py_VISIT(self->delay_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Delay_clear(Delay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->delay);    
    Py_CLEAR(self->delay_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Delay_dealloc(Delay* self)
{
    free(self->data);
    free(self->buffer);
    Delay_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Delay_deleteStream(Delay *self) { DELETE_STREAM };

static PyObject *
Delay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Delay *self;
    self = (Delay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0);
    self->feedback = PyFloat_FromDouble(0);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Delay_compute_next_data_frame);
    self->mode_func_ptr = Delay_setProcMode;
    
    return (PyObject *)self;
}

static int
Delay_init(Delay *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "delay", "feedback", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxdelay, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = self->maxdelay * self->sr + 0.5;

    self->buffer = (float *)realloc(self->buffer, (self->size+1) * sizeof(float));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    

    (*self->mode_func_ptr)(self);

    Delay_compute_next_data_frame((Delay *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Delay_getServer(Delay* self) { GET_SERVER };
static PyObject * Delay_getStream(Delay* self) { GET_STREAM };
static PyObject * Delay_setMul(Delay *self, PyObject *arg) { SET_MUL };	
static PyObject * Delay_setAdd(Delay *self, PyObject *arg) { SET_ADD };	

static PyObject * Delay_play(Delay *self) { PLAY };
static PyObject * Delay_out(Delay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Delay_stop(Delay *self) { STOP };

static PyObject * Delay_multiply(Delay *self, PyObject *arg) { MULTIPLY };
static PyObject * Delay_inplace_multiply(Delay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Delay_add(Delay *self, PyObject *arg) { ADD };
static PyObject * Delay_inplace_add(Delay *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
Delay_setDelay(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Delay_setFeedback(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Delay_members[] = {
    {"server", T_OBJECT_EX, offsetof(Delay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Delay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Delay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Delay, delay), 0, "Delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Delay, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Delay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Delay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Delay_methods[] = {
    {"getServer", (PyCFunction)Delay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Delay_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Delay_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Delay_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Delay_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Delay_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)Delay_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Delay_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
	{"setMul", (PyCFunction)Delay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Delay_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Delay_as_number = {
    (binaryfunc)Delay_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Delay_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Delay_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Delay_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject DelayType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Delay_base",         /*tp_name*/
    sizeof(Delay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Delay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Delay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Delay objects. Delay signal by x samples.",           /* tp_doc */
    (traverseproc)Delay_traverse,   /* tp_traverse */
    (inquiry)Delay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Delay_methods,             /* tp_methods */
    Delay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Delay_init,      /* tp_init */
    0,                         /* tp_alloc */
    Delay_new,                 /* tp_new */
};

