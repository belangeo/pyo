#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    PyObject *max;
    Stream *min_stream;
    Stream *max_stream;
    float value;
    int modebuffer[4]; // need at least 2 slots for mul & add 
} TrigRand;

static void
TrigRand_generate_ii(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float mi = PyFloat_AS_DOUBLE(self->min);
    float ma = PyFloat_AS_DOUBLE(self->max);
    float range = ma - mi;
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->data[i] = self->value;
        }
        else
            self->data[i] = self->value;
    }
}

static void
TrigRand_generate_ai(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *mi = Stream_getData((Stream *)self->min_stream);
    float ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma - mi[i];
        if (in[i] == 1) {
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->data[i] = self->value;
        }
        else
            self->data[i] = self->value;
    }
}

static void
TrigRand_generate_ia(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float mi = PyFloat_AS_DOUBLE(self->min);
    float *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma[i] - mi;
        if (in[i] == 1) {
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi;
            self->data[i] = self->value;
        }
        else
            self->data[i] = self->value;
    }
}

static void
TrigRand_generate_aa(TrigRand *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *mi = Stream_getData((Stream *)self->min_stream);
    float *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        float range = ma[i] - mi[i];
        if (in[i] == 1) {
            self->value = range * (rand()/((float)(RAND_MAX)+1)) + mi[i];
            self->data[i] = self->value;
        }
        else
            self->data[i] = self->value;
    }
}

static void TrigRand_postprocessing_ii(TrigRand *self) { POST_PROCESSING_II };
static void TrigRand_postprocessing_ai(TrigRand *self) { POST_PROCESSING_AI };
static void TrigRand_postprocessing_ia(TrigRand *self) { POST_PROCESSING_IA };
static void TrigRand_postprocessing_aa(TrigRand *self) { POST_PROCESSING_AA };
static void TrigRand_postprocessing_ireva(TrigRand *self) { POST_PROCESSING_IREVA };
static void TrigRand_postprocessing_areva(TrigRand *self) { POST_PROCESSING_AREVA };
static void TrigRand_postprocessing_revai(TrigRand *self) { POST_PROCESSING_REVAI };
static void TrigRand_postprocessing_revaa(TrigRand *self) { POST_PROCESSING_REVAA };
static void TrigRand_postprocessing_revareva(TrigRand *self) { POST_PROCESSING_REVAREVA };

static void
TrigRand_setProcMode(TrigRand *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = TrigRand_generate_ii;
            break;
        case 1:    
            self->proc_func_ptr = TrigRand_generate_ai;
            break;
        case 10:    
            self->proc_func_ptr = TrigRand_generate_ia;
            break;
        case 11:    
            self->proc_func_ptr = TrigRand_generate_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigRand_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigRand_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigRand_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigRand_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigRand_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigRand_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigRand_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigRand_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigRand_postprocessing_revareva;
            break;
    }  
}

static void
TrigRand_compute_next_data_frame(TrigRand *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigRand_traverse(TrigRand *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
TrigRand_clear(TrigRand *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
TrigRand_dealloc(TrigRand* self)
{
    free(self->data);
    TrigRand_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigRand_deleteStream(TrigRand *self) { DELETE_STREAM };

static PyObject *
TrigRand_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigRand *self;
    self = (TrigRand *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.);
    self->max = PyFloat_FromDouble(1.);
    self->value = 0.;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigRand_compute_next_data_frame);
    self->mode_func_ptr = TrigRand_setProcMode;
    return (PyObject *)self;
}

static int
TrigRand_init(TrigRand *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
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

    (*self->mode_func_ptr)(self);
    
    TrigRand_compute_next_data_frame((TrigRand *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigRand_getServer(TrigRand* self) { GET_SERVER };
static PyObject * TrigRand_getStream(TrigRand* self) { GET_STREAM };
static PyObject * TrigRand_setMul(TrigRand *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigRand_setAdd(TrigRand *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigRand_setSub(TrigRand *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigRand_setDiv(TrigRand *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigRand_play(TrigRand *self) { PLAY };
static PyObject * TrigRand_out(TrigRand *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigRand_stop(TrigRand *self) { STOP };

static PyObject * TrigRand_multiply(TrigRand *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigRand_inplace_multiply(TrigRand *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigRand_add(TrigRand *self, PyObject *arg) { ADD };
static PyObject * TrigRand_inplace_add(TrigRand *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigRand_sub(TrigRand *self, PyObject *arg) { SUB };
static PyObject * TrigRand_inplace_sub(TrigRand *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigRand_div(TrigRand *self, PyObject *arg) { DIV };
static PyObject * TrigRand_inplace_div(TrigRand *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigRand_setMin(TrigRand *self, PyObject *arg)
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
TrigRand_setMax(TrigRand *self, PyObject *arg)
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

static PyMemberDef TrigRand_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigRand, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigRand, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TrigRand, input), 0, "Input sound object."},
{"min", T_OBJECT_EX, offsetof(TrigRand, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(TrigRand, max), 0, "Maximum possible value."},
{"mul", T_OBJECT_EX, offsetof(TrigRand, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TrigRand, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigRand_methods[] = {
{"getServer", (PyCFunction)TrigRand_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigRand_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigRand_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigRand_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TrigRand_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TrigRand_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)TrigRand_setMin, METH_O, "Sets minimum possible value."},
{"setMax", (PyCFunction)TrigRand_setMax, METH_O, "Sets maximum possible value."},
{"setMul", (PyCFunction)TrigRand_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)TrigRand_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)TrigRand_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)TrigRand_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TrigRand_as_number = {
(binaryfunc)TrigRand_add,                         /*nb_add*/
(binaryfunc)TrigRand_sub,                         /*nb_subtract*/
(binaryfunc)TrigRand_multiply,                    /*nb_multiply*/
(binaryfunc)TrigRand_div,                                              /*nb_divide*/
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
(binaryfunc)TrigRand_inplace_add,                 /*inplace_add*/
(binaryfunc)TrigRand_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)TrigRand_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)TrigRand_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject TrigRandType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.TrigRand_base",                                   /*tp_name*/
sizeof(TrigRand),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)TrigRand_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&TrigRand_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigRand objects. Generates a new random value on a trigger signal.",           /* tp_doc */
(traverseproc)TrigRand_traverse,                  /* tp_traverse */
(inquiry)TrigRand_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
TrigRand_methods,                                 /* tp_methods */
TrigRand_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)TrigRand_init,                          /* tp_init */
0,                                              /* tp_alloc */
TrigRand_new,                                     /* tp_new */
};


/*********************************************************************************************/
/* TrigEnv *********************************************************************************/
/*********************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *input;
    Stream *input_stream;
    PyObject *dur;
    Stream *dur_stream;
    int modebuffer[3];
    int active;
    float current_dur; // duration in samples
    float inc; // table size / current_dur
    float pointerPos; // reading position in sample
} TrigEnv;

static void
TrigEnv_readframes_i(TrigEnv *self) {
    float fpart, x, x1;
    int i, ipart;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            float dur = PyFloat_AS_DOUBLE(self->dur);
            self->current_dur = self->sr * dur;
            self->inc = (float)size / self->current_dur;
            self->active = 1;
            self->pointerPos = 0.;
        }
        if (self->active == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;
        
        if (self->pointerPos > size)
            self->active = 0;
    }
}

static void
TrigEnv_readframes_a(TrigEnv *self) {
    float fpart, x, x1;
    int i, ipart;
    float *in = Stream_getData((Stream *)self->input_stream);
    float *dur_st = Stream_getData((Stream *)self->dur_stream);
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    for (i=0; i<self->bufsize; i++) {
        if (in[i] == 1) {
            float dur = dur_st[i];
            self->current_dur = self->sr * dur;
            self->inc = (float)size / self->current_dur;
            self->active = 1;
            self->pointerPos = 0.;
        }
        if (self->active == 1) {
            ipart = (int)self->pointerPos;
            fpart = self->pointerPos - ipart;
            x = tablelist[ipart];
            x1 = tablelist[ipart+1];
            self->data[i] = x + (x1 - x) * fpart;
            self->pointerPos += self->inc;
        }
        else
            self->data[i] = 0.;
        
        if (self->pointerPos > size)
            self->active = 0;
    }
}

static void TrigEnv_postprocessing_ii(TrigEnv *self) { POST_PROCESSING_II };
static void TrigEnv_postprocessing_ai(TrigEnv *self) { POST_PROCESSING_AI };
static void TrigEnv_postprocessing_ia(TrigEnv *self) { POST_PROCESSING_IA };
static void TrigEnv_postprocessing_aa(TrigEnv *self) { POST_PROCESSING_AA };
static void TrigEnv_postprocessing_ireva(TrigEnv *self) { POST_PROCESSING_IREVA };
static void TrigEnv_postprocessing_areva(TrigEnv *self) { POST_PROCESSING_AREVA };
static void TrigEnv_postprocessing_revai(TrigEnv *self) { POST_PROCESSING_REVAI };
static void TrigEnv_postprocessing_revaa(TrigEnv *self) { POST_PROCESSING_REVAA };
static void TrigEnv_postprocessing_revareva(TrigEnv *self) { POST_PROCESSING_REVAREVA };

static void
TrigEnv_setProcMode(TrigEnv *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = TrigEnv_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = TrigEnv_readframes_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = TrigEnv_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = TrigEnv_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = TrigEnv_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = TrigEnv_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = TrigEnv_postprocessing_revareva;
            break;
    } 
}

static void
TrigEnv_compute_next_data_frame(TrigEnv *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
TrigEnv_traverse(TrigEnv *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
TrigEnv_clear(TrigEnv *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
TrigEnv_dealloc(TrigEnv* self)
{
    free(self->data);
    TrigEnv_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TrigEnv_deleteStream(TrigEnv *self) { DELETE_STREAM };

static PyObject *
TrigEnv_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TrigEnv *self;
    self = (TrigEnv *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    self->pointerPos = 0.;
    self->active = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, TrigEnv_compute_next_data_frame);
    self->mode_func_ptr = TrigEnv_setProcMode;

    self->dur = PyFloat_FromDouble(1.);
    self->current_dur = self->sr;

    return (PyObject *)self;
}

static int
TrigEnv_init(TrigEnv *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "table", "dur", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOO", kwlist, &inputtmp, &tabletmp, &durtmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");
    
    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
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
    
    TrigEnv_compute_next_data_frame((TrigEnv *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TrigEnv_getServer(TrigEnv* self) { GET_SERVER };
static PyObject * TrigEnv_getStream(TrigEnv* self) { GET_STREAM };
static PyObject * TrigEnv_setMul(TrigEnv *self, PyObject *arg) { SET_MUL };	
static PyObject * TrigEnv_setAdd(TrigEnv *self, PyObject *arg) { SET_ADD };	
static PyObject * TrigEnv_setSub(TrigEnv *self, PyObject *arg) { SET_SUB };	
static PyObject * TrigEnv_setDiv(TrigEnv *self, PyObject *arg) { SET_DIV };	

static PyObject * TrigEnv_play(TrigEnv *self) { PLAY };
static PyObject * TrigEnv_out(TrigEnv *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * TrigEnv_stop(TrigEnv *self) { STOP };

static PyObject * TrigEnv_multiply(TrigEnv *self, PyObject *arg) { MULTIPLY };
static PyObject * TrigEnv_inplace_multiply(TrigEnv *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * TrigEnv_add(TrigEnv *self, PyObject *arg) { ADD };
static PyObject * TrigEnv_inplace_add(TrigEnv *self, PyObject *arg) { INPLACE_ADD };
static PyObject * TrigEnv_sub(TrigEnv *self, PyObject *arg) { SUB };
static PyObject * TrigEnv_inplace_sub(TrigEnv *self, PyObject *arg) { INPLACE_SUB };
static PyObject * TrigEnv_div(TrigEnv *self, PyObject *arg) { DIV };
static PyObject * TrigEnv_inplace_div(TrigEnv *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
TrigEnv_getTable(TrigEnv* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
TrigEnv_setDur(TrigEnv *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TrigEnv_members[] = {
{"server", T_OBJECT_EX, offsetof(TrigEnv, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TrigEnv, stream), 0, "Stream object."},
{"table", T_OBJECT_EX, offsetof(TrigEnv, table), 0, "Envelope table."},
{"dur", T_OBJECT_EX, offsetof(TrigEnv, dur), 0, "Envelope duration in seconds."},
{"mul", T_OBJECT_EX, offsetof(TrigEnv, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(TrigEnv, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef TrigEnv_methods[] = {
{"getTable", (PyCFunction)TrigEnv_getTable, METH_NOARGS, "Returns waveform table object."},
{"getServer", (PyCFunction)TrigEnv_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TrigEnv_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TrigEnv_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)TrigEnv_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)TrigEnv_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)TrigEnv_stop, METH_NOARGS, "Stops computing."},
{"setDur", (PyCFunction)TrigEnv_setDur, METH_O, "Sets envelope duration in second."},
{"setMul", (PyCFunction)TrigEnv_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)TrigEnv_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)TrigEnv_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)TrigEnv_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods TrigEnv_as_number = {
(binaryfunc)TrigEnv_add,                      /*nb_add*/
(binaryfunc)TrigEnv_sub,                 /*nb_subtract*/
(binaryfunc)TrigEnv_multiply,                 /*nb_multiply*/
(binaryfunc)TrigEnv_div,                   /*nb_divide*/
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
(binaryfunc)TrigEnv_inplace_add,              /*inplace_add*/
(binaryfunc)TrigEnv_inplace_sub,         /*inplace_subtract*/
(binaryfunc)TrigEnv_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)TrigEnv_inplace_div,           /*inplace_divide*/
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

PyTypeObject TrigEnvType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TrigEnv_base",         /*tp_name*/
sizeof(TrigEnv),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TrigEnv_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&TrigEnv_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"TrigEnv objects. Starts an envelope on a trigger signal.",           /* tp_doc */
(traverseproc)TrigEnv_traverse,   /* tp_traverse */
(inquiry)TrigEnv_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TrigEnv_methods,             /* tp_methods */
TrigEnv_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TrigEnv_init,      /* tp_init */
0,                         /* tp_alloc */
TrigEnv_new,                 /* tp_new */
};

