#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

/* Compressor */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *risetime;
    PyObject *falltime;
    PyObject *thresh;
    PyObject *ratio;
    Stream *risetime_stream;
    Stream *falltime_stream;
    Stream *thresh_stream;
    Stream *ratio_stream;
    int modebuffer[6]; // need at least 2 slots for mul & add 
    void (*comp_func_ptr)();
    float y1; // sample memory
    float x1;
    int dir;
    float *follow;
} Compress;

static float
_clip(float x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

static void 
direction(Compress *self, float val)
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
Compress_filters_ii(Compress *self) {
    float absin, val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float risetime = PyFloat_AS_DOUBLE(self->risetime);
    float falltime = PyFloat_AS_DOUBLE(self->falltime);
    float risefactor = 1. / (risetime * self->sr);
    float fallfactor = 1. / (falltime * self->sr);
    float factors[2] = {fallfactor, risefactor};
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        direction(self, absin);
        val = self->y1 + (absin - self->y1) * factors[self->dir];
        self->follow[i] = self->y1 = val;
    }
}

static void
Compress_filters_ai(Compress *self) {
    float absin, val, risefactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *risetime = Stream_getData((Stream *)self->risetime_stream);
    float falltime = PyFloat_AS_DOUBLE(self->falltime);
    float fallfactor = 1. / (falltime * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        direction(self, absin);
        risefactor = *risetime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (absin - self->y1) / risefactor;
        else
            val = self->y1 + (absin - self->y1) * fallfactor;
        self->follow[i] = self->y1 = val;
    }
}

static void
Compress_filters_ia(Compress *self) {
    float absin, val, fallfactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *falltime = Stream_getData((Stream *)self->falltime_stream);
    float risetime = PyFloat_AS_DOUBLE(self->risetime);
    float risefactor = 1. / (risetime * self->sr);
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        direction(self, absin);
        fallfactor = *falltime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (absin - self->y1) * risefactor;
        else
            val = self->y1 + (absin - self->y1) / fallfactor;
        self->follow[i] = self->y1 = val;
    }
}

static void
Compress_filters_aa(Compress *self) {
    float absin, val, risefactor, fallfactor;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *risetime = Stream_getData((Stream *)self->risetime_stream);
    float *falltime = Stream_getData((Stream *)self->falltime_stream);
    
    for (i=0; i<self->bufsize; i++) {
        absin = in[i] * in[i];
        direction(self, absin);
        risefactor = *risetime++ * self->sr;  
        fallfactor = *falltime++ * self->sr;  
        if (self->dir == 1)
            val = self->y1 + (absin - self->y1) / risefactor;
        else
            val = self->y1 + (absin - self->y1) / fallfactor;
        self->follow[i] = self->y1 = val;
    }
}

static void
Compress_compress_ii(Compress *self) {
    float val, indb, diff, outdb, outa;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float thresh = PyFloat_AS_DOUBLE(self->thresh);
    float ratio = PyFloat_AS_DOUBLE(self->ratio);
    ratio = 1.0 / ratio;
    
    for (i=0; i<self->bufsize; i++) {
        indb = 20.0 * log10f(self->follow[i]);
        diff = indb - thresh;
        outdb = diff - diff * ratio;
        outa = powf(10.0, (0.0 - outdb) * 0.05);
        self->data[i] = in[i] * _clip(outa);
    }
}

static void
Compress_compress_ai(Compress *self) {
    float val, indb, diff, outdb, outa;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *thresh = Stream_getData((Stream *)self->thresh_stream);
    float ratio = PyFloat_AS_DOUBLE(self->ratio);
    ratio = 1.0 / ratio;
    
    for (i=0; i<self->bufsize; i++) {
        indb = 20.0 * log10f(self->follow[i]);
        diff = indb - thresh[i];
        outdb = diff - diff * ratio;
        outa = powf(10.0, (0.0 - outdb) * 0.05);
        self->data[i] = in[i] * _clip(outa);
    }
}

static void
Compress_compress_ia(Compress *self) {
    float val, indb, diff, outdb, outa;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float thresh = PyFloat_AS_DOUBLE(self->thresh);
    float *ratio = Stream_getData((Stream *)self->ratio_stream);
    
    for (i=0; i<self->bufsize; i++) {
        indb = 20.0 * log10f(self->follow[i]);
        diff = indb - thresh;
        outdb = diff - diff / ratio[i];
        outa = powf(10.0, (0.0 - outdb) * 0.05);
        self->data[i] = in[i] * _clip(outa);
    }
}

static void
Compress_compress_aa(Compress *self) {
    float val, indb, diff, outdb, outa;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *thresh = Stream_getData((Stream *)self->thresh_stream);
    float *ratio = Stream_getData((Stream *)self->ratio_stream);
    
    for (i=0; i<self->bufsize; i++) {
        indb = 20.0 * log10f(self->follow[i]);
        diff = indb - thresh[i];
        outdb = diff - diff / ratio[i];
        outa = powf(10.0, (0.0 - outdb) * 0.05);
        self->data[i] = in[i] * _clip(outa);
    }
}

static void Compress_postprocessing_ii(Compress *self) { POST_PROCESSING_II };
static void Compress_postprocessing_ai(Compress *self) { POST_PROCESSING_AI };
static void Compress_postprocessing_ia(Compress *self) { POST_PROCESSING_IA };
static void Compress_postprocessing_aa(Compress *self) { POST_PROCESSING_AA };
static void Compress_postprocessing_ireva(Compress *self) { POST_PROCESSING_IREVA };
static void Compress_postprocessing_areva(Compress *self) { POST_PROCESSING_AREVA };
static void Compress_postprocessing_revai(Compress *self) { POST_PROCESSING_REVAI };
static void Compress_postprocessing_revaa(Compress *self) { POST_PROCESSING_REVAA };
static void Compress_postprocessing_revareva(Compress *self) { POST_PROCESSING_REVAREVA };

static void
Compress_setProcMode(Compress *self)
{
    int compmode, procmode, muladdmode;
    compmode = self->modebuffer[4] + self->modebuffer[5] * 10;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (compmode) {
        case 0:    
            self->comp_func_ptr = Compress_compress_ii;
            break;
        case 1:    
            self->comp_func_ptr = Compress_compress_ai;
            break;
        case 10:    
            self->comp_func_ptr = Compress_compress_ia;
            break;
        case 11:    
            self->comp_func_ptr = Compress_compress_aa;
            break;
    }
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Compress_filters_ii;
            break;
        case 1:    
            self->proc_func_ptr = Compress_filters_ai;
            break;
        case 10:    
            self->proc_func_ptr = Compress_filters_ia;
            break;
        case 11:    
            self->proc_func_ptr = Compress_filters_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Compress_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Compress_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Compress_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Compress_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Compress_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Compress_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Compress_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Compress_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Compress_postprocessing_revareva;
            break;
    }  
}

static void
Compress_compute_next_data_frame(Compress *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->comp_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Compress_traverse(Compress *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->risetime);    
    Py_VISIT(self->risetime_stream);    
    Py_VISIT(self->falltime);    
    Py_VISIT(self->falltime_stream);    
    Py_VISIT(self->thresh);    
    Py_VISIT(self->thresh_stream);    
    Py_VISIT(self->ratio);    
    Py_VISIT(self->ratio_stream);    
    return 0;
}

static int 
Compress_clear(Compress *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->risetime);    
    Py_CLEAR(self->risetime_stream);    
    Py_CLEAR(self->falltime);    
    Py_CLEAR(self->falltime_stream);    
    Py_CLEAR(self->thresh);    
    Py_CLEAR(self->thresh_stream);    
    Py_CLEAR(self->ratio);    
    Py_CLEAR(self->ratio_stream);    
    return 0;
}

static void
Compress_dealloc(Compress* self)
{
    free(self->data);
    free(self->follow);
    Compress_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Compress_deleteStream(Compress *self) { DELETE_STREAM };

static PyObject *
Compress_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Compress *self;
    self = (Compress *)type->tp_alloc(type, 0);
    
    self->thresh = PyFloat_FromDouble(-20.0);
    self->ratio = PyFloat_FromDouble(2.0);
    self->risetime = PyFloat_FromDouble(0.005);
    self->falltime = PyFloat_FromDouble(0.05);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
	self->modebuffer[5] = 0;
    self->y1 = 0.0;
    self->x1 = 0.0;
    self->dir = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Compress_compute_next_data_frame);
    self->mode_func_ptr = Compress_setProcMode;
    return (PyObject *)self;
}

static int
Compress_init(Compress *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *threshtmp=NULL, *ratiotmp=NULL, *risetimetmp=NULL, *falltimetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "thresh", "ratio", "risetime", "falltime", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOO", kwlist, &inputtmp, &threshtmp, &ratiotmp, &risetimetmp, &falltimetmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (threshtmp) {
        PyObject_CallMethod((PyObject *)self, "setThresh", "O", threshtmp);
    }
   
    if (ratiotmp) {
        PyObject_CallMethod((PyObject *)self, "setRatio", "O", ratiotmp);
    }
   
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

    self->follow = (float *)realloc(self->follow, self->bufsize * sizeof(float));

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Compress_compute_next_data_frame((Compress *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Compress_getServer(Compress* self) { GET_SERVER };
static PyObject * Compress_getStream(Compress* self) { GET_STREAM };
static PyObject * Compress_setMul(Compress *self, PyObject *arg) { SET_MUL };	
static PyObject * Compress_setAdd(Compress *self, PyObject *arg) { SET_ADD };	
static PyObject * Compress_setSub(Compress *self, PyObject *arg) { SET_SUB };	
static PyObject * Compress_setDiv(Compress *self, PyObject *arg) { SET_DIV };	

static PyObject * Compress_play(Compress *self) { PLAY };
static PyObject * Compress_out(Compress *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Compress_stop(Compress *self) { STOP };

static PyObject * Compress_multiply(Compress *self, PyObject *arg) { MULTIPLY };
static PyObject * Compress_inplace_multiply(Compress *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Compress_add(Compress *self, PyObject *arg) { ADD };
static PyObject * Compress_inplace_add(Compress *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Compress_sub(Compress *self, PyObject *arg) { SUB };
static PyObject * Compress_inplace_sub(Compress *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Compress_div(Compress *self, PyObject *arg) { DIV };
static PyObject * Compress_inplace_div(Compress *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Compress_setThresh(Compress *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->thresh);
	if (isNumber == 1) {
		self->thresh = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->thresh = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->thresh, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->thresh_stream);
        self->thresh_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Compress_setRatio(Compress *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ratio);
	if (isNumber == 1) {
		self->ratio = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->ratio = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ratio, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ratio_stream);
        self->ratio_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Compress_setRiseTime(Compress *self, PyObject *arg)
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
Compress_setFallTime(Compress *self, PyObject *arg)
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

static PyMemberDef Compress_members[] = {
{"server", T_OBJECT_EX, offsetof(Compress, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Compress, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Compress, input), 0, "Input sound object."},
{"thresh", T_OBJECT_EX, offsetof(Compress, thresh), 0, "Compressor threshold."},
{"ratio", T_OBJECT_EX, offsetof(Compress, ratio), 0, "Compressor ratio."},
{"risetime", T_OBJECT_EX, offsetof(Compress, risetime), 0, "Rising portamento time in seconds."},
{"falltime", T_OBJECT_EX, offsetof(Compress, falltime), 0, "Falling portamento time in seconds."},
{"mul", T_OBJECT_EX, offsetof(Compress, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Compress, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Compress_methods[] = {
{"getServer", (PyCFunction)Compress_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Compress_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Compress_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Compress_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Compress_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Compress_stop, METH_NOARGS, "Stops computing."},
{"setThresh", (PyCFunction)Compress_setThresh, METH_O, "Sets compressor threshold."},
{"setRatio", (PyCFunction)Compress_setRatio, METH_O, "Sets compressor ratio."},
{"setRiseTime", (PyCFunction)Compress_setRiseTime, METH_O, "Sets rising portamento time in seconds."},
{"setFallTime", (PyCFunction)Compress_setFallTime, METH_O, "Sets falling portamento time in seconds."},
{"setMul", (PyCFunction)Compress_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Compress_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Compress_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Compress_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Compress_as_number = {
(binaryfunc)Compress_add,                         /*nb_add*/
(binaryfunc)Compress_sub,                         /*nb_subtract*/
(binaryfunc)Compress_multiply,                    /*nb_multiply*/
(binaryfunc)Compress_div,                                              /*nb_divide*/
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
(binaryfunc)Compress_inplace_add,                 /*inplace_add*/
(binaryfunc)Compress_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Compress_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Compress_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject CompressType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Compress_base",                                   /*tp_name*/
sizeof(Compress),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Compress_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Compress_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Compress objects. Compress audio signal by a certain ratio above a threshold.",           /* tp_doc */
(traverseproc)Compress_traverse,                  /* tp_traverse */
(inquiry)Compress_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Compress_methods,                                 /* tp_methods */
Compress_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Compress_init,                          /* tp_init */
0,                                              /* tp_alloc */
Compress_new,                                     /* tp_new */
};
