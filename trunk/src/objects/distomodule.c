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
    PyObject *drive;
    Stream *drive_stream;
    PyObject *slope;
    Stream *slope_stream;
    int init;
    int modebuffer[4];
    float y1; // sample memory
} Disto;

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
Disto_transform_ii(Disto *self) {
    float val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    float drv = .4 - _clip(PyFloat_AS_DOUBLE(self->drive)) * .3999;
    float slp = _clip(PyFloat_AS_DOUBLE(self->slope));
    
    for (i=0; i<self->bufsize; i++) {
        val = atan2f(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_ai(Disto *self) {
    float val, drv;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);

    float *drive = Stream_getData((Stream *)self->drive_stream);
    float slp = _clip(PyFloat_AS_DOUBLE(self->slope));
    
    for (i=0; i<self->bufsize; i++) {
        drv = .4 - _clip(drive[i]) * .3999;
        val = atan2f(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_ia(Disto *self) {
    float val;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float drv = .4 - _clip(PyFloat_AS_DOUBLE(self->drive)) * .3999;
    float *slope = Stream_getData((Stream *)self->slope_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = atan2f(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] + self->y1 * _clip(slope[i]);
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_aa(Disto *self) {
    float val, drv;
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    float *drive = Stream_getData((Stream *)self->drive_stream);
    float *slope = Stream_getData((Stream *)self->slope_stream);
    
    for (i=0; i<self->bufsize; i++) {
        drv = .4 - _clip(drive[i]) * .3999;
        val = atan2f(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] + self->y1 * _clip(slope[i]);
        self->y1 = val;
        self->data[i] = val;
    }
}

static void Disto_postprocessing_ii(Disto *self) { POST_PROCESSING_II };
static void Disto_postprocessing_ai(Disto *self) { POST_PROCESSING_AI };
static void Disto_postprocessing_ia(Disto *self) { POST_PROCESSING_IA };
static void Disto_postprocessing_aa(Disto *self) { POST_PROCESSING_AA };

static void
Disto_setProcMode(Disto *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Disto_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Disto_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Disto_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Disto_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Disto_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Disto_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Disto_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Disto_postprocessing_aa;
            break;
    }    
}

static void
Disto_compute_next_data_frame(Disto *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Disto_traverse(Disto *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->drive);    
    Py_VISIT(self->drive_stream);    
    Py_VISIT(self->slope);    
    Py_VISIT(self->slope_stream);    
    return 0;
}

static int 
Disto_clear(Disto *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->drive);    
    Py_CLEAR(self->drive_stream);    
    Py_CLEAR(self->slope);    
    Py_CLEAR(self->slope_stream);    
    return 0;
}

static void
Disto_dealloc(Disto* self)
{
    free(self->data);
    Disto_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Disto_deleteStream(Disto *self) { DELETE_STREAM };

static PyObject *
Disto_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Disto *self;
    self = (Disto *)type->tp_alloc(type, 0);

    self->drive = PyFloat_FromDouble(.75);
    self->slope = PyFloat_FromDouble(.5);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->y1 = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Disto_compute_next_data_frame);
    self->mode_func_ptr = Disto_setProcMode;
    
    return (PyObject *)self;
}

static int
Disto_init(Disto *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *drivetmp=NULL, *slopetmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "drive", "slope", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &drivetmp, &slopetmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (drivetmp) {
        PyObject_CallMethod((PyObject *)self, "setDrive", "O", drivetmp);
    }

    if (slopetmp) {
        PyObject_CallMethod((PyObject *)self, "setSlope", "O", slopetmp);
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

    Disto_compute_next_data_frame((Disto *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Disto_getServer(Disto* self) { GET_SERVER };
static PyObject * Disto_getStream(Disto* self) { GET_STREAM };
static PyObject * Disto_setMul(Disto *self, PyObject *arg) { SET_MUL };	
static PyObject * Disto_setAdd(Disto *self, PyObject *arg) { SET_ADD };	

static PyObject * Disto_play(Disto *self) { PLAY };
static PyObject * Disto_out(Disto *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Disto_stop(Disto *self) { STOP };

static PyObject * Disto_multiply(Disto *self, PyObject *arg) { MULTIPLY };
static PyObject * Disto_inplace_multiply(Disto *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Disto_add(Disto *self, PyObject *arg) { ADD };
static PyObject * Disto_inplace_add(Disto *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
Disto_setDrive(Disto *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->drive);
	if (isNumber == 1) {
		self->drive = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->drive = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->drive, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->drive_stream);
        self->drive_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Disto_setSlope(Disto *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->slope);
	if (isNumber == 1) {
		self->slope = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->slope = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->slope, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->slope_stream);
        self->slope_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Disto_members[] = {
    {"server", T_OBJECT_EX, offsetof(Disto, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Disto, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Disto, input), 0, "Input sound object."},
    {"drive", T_OBJECT_EX, offsetof(Disto, drive), 0, "Cutoff driveuency in cycle per second."},
    {"slope", T_OBJECT_EX, offsetof(Disto, slope), 0, "Lowpass filter slope factor."},
    {"mul", T_OBJECT_EX, offsetof(Disto, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Disto, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Disto_methods[] = {
    //{"getInput", (PyCFunction)Disto_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Disto_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Disto_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Disto_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Disto_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Disto_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Disto_stop, METH_NOARGS, "Stops computing."},
	{"setDrive", (PyCFunction)Disto_setDrive, METH_O, "Sets distortion drive factor (0 -> 1)."},
    {"setSlope", (PyCFunction)Disto_setSlope, METH_O, "Sets lowpass filter slope factor."},
	{"setMul", (PyCFunction)Disto_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Disto_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Disto_as_number = {
    (binaryfunc)Disto_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Disto_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Disto_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Disto_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject DistoType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Disto_base",         /*tp_name*/
    sizeof(Disto),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Disto_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Disto_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Disto objects. Arctan distortion.",           /* tp_doc */
    (traverseproc)Disto_traverse,   /* tp_traverse */
    (inquiry)Disto_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Disto_methods,             /* tp_methods */
    Disto_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Disto_init,      /* tp_init */
    0,                         /* tp_alloc */
    Disto_new,                 /* tp_new */
};

