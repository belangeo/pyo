#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input1;
    PyObject *input2;
    Stream *input1_stream;
    Stream *input2_stream;
    float fadetime;
    int switcher;
    float currentTime;
    float sampleToSec;
} InputFader;

static void InputFader_setProcMode(InputFader *self) {};

static void InputFader_process_first(InputFader *self) 
{
    int i;
    float amp;
    float *in = Stream_getData((Stream *)self->input1_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i];
    }
}

static void InputFader_process_one(InputFader *self) 
{
    int i;
    float sclfade, val;
    float *in1 = Stream_getData((Stream *)self->input1_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);
    
    sclfade = 1. / self->fadetime;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = sqrtf(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }    
        else
            val = 1.;

        self->data[i] = in1[i] * val + in2[i] * (1 - val);
    }
}

static void InputFader_process_two(InputFader *self) 
{
    int i;
    float sclfade, val;
    float *in1 = Stream_getData((Stream *)self->input1_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);

    sclfade = 1. / self->fadetime;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = sqrtf(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }    
        else
            val = 1.;
        
        self->data[i] = in2[i] * val + in1[i] * (1 - val);
    }
}

static void
InputFader_compute_next_data_frame(InputFader *self)
{   
    (*self->proc_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
InputFader_traverse(InputFader *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input1);
    Py_VISIT(self->input1_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int 
InputFader_clear(InputFader *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input1);
    Py_CLEAR(self->input1_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
InputFader_dealloc(InputFader* self)
{
    free(self->data);
    InputFader_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
InputFader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    InputFader *self;
    self = (InputFader *)type->tp_alloc(type, 0);
    
    self->switcher = 0;
    self->fadetime = 0.05;
    self->currentTime = 0.0;
    
    INIT_OBJECT_COMMON

    self->sampleToSec = 1. / self->sr;
    
    Stream_setFunctionPtr(self->stream, InputFader_compute_next_data_frame);
    self->mode_func_ptr = InputFader_setProcMode;
    self->proc_func_ptr = InputFader_process_first;
    
    return (PyObject *)self;
}

static int
InputFader_init(InputFader *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp=NULL, *streamtmp;
    
    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp))
        return -1; 

    Py_INCREF(inputtmp);
    Py_XDECREF(self->input1);
    self->input1 = inputtmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->input1_stream);
    self->input1_stream = (Stream *)streamtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    InputFader_compute_next_data_frame((InputFader *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject *
InputFader_setInput(InputFader *self, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *streamtmp;

    static char *kwlist[] = {"input", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|f", kwlist, &tmp, &self->fadetime))
        return PyInt_FromLong(-1);
    
    self->switcher = (self->switcher + 1) % 2;
    self->currentTime = 0.0;
    if (self->fadetime == 0)
        self->fadetime = 0.0001;
    
    Py_INCREF(tmp);

    if (self->switcher == 0) {
        Py_DECREF(self->input1);
        self->input1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input1_stream);
        self->input1_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_one;
	}
    else {
        Py_XDECREF(self->input2);
        self->input2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input2_stream);
        self->input2_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_two;
	}    
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject * InputFader_getServer(InputFader* self) { GET_SERVER };
static PyObject * InputFader_getStream(InputFader* self) { GET_STREAM };
//static PyObject * InputFader_setMul(InputFader *self, PyObject *arg) { SET_MUL };	
//static PyObject * InputFader_setAdd(InputFader *self, PyObject *arg) { SET_ADD };	

static PyObject * InputFader_play(InputFader *self) { PLAY };
//static PyObject * InputFader_out(InputFader *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * InputFader_stop(InputFader *self) { STOP };

//static PyObject * InputFader_multiply(InputFader *self, PyObject *arg) { MULTIPLY };
//static PyObject * InputFader_inplace_multiply(InputFader *self, PyObject *arg) { INPLACE_MULTIPLY };
//static PyObject * InputFader_add(InputFader *self, PyObject *arg) { ADD };
//static PyObject * InputFader_inplace_add(InputFader *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef InputFader_members[] = {
    {"server", T_OBJECT_EX, offsetof(InputFader, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(InputFader, stream), 0, "Stream object."},
    {"input1", T_OBJECT_EX, offsetof(InputFader, input1), 0, "First input."},
    {"input2", T_OBJECT_EX, offsetof(InputFader, input2), 0, "Second input."},
//    {"mul", T_OBJECT_EX, offsetof(InputFader, mul), 0, "Mul factor."},
//    {"add", T_OBJECT_EX, offsetof(InputFader, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef InputFader_methods[] = {
    //{"getInputFader", (PyCFunction)InputFader_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)InputFader_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)InputFader_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)InputFader_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"setInput", (PyCFunction)InputFader_setInput, METH_VARARGS, "Crossfade between current stream and given stream."},
//    {"out", (PyCFunction)InputFader_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)InputFader_stop, METH_NOARGS, "Stops computing."},
//	{"setMul", (PyCFunction)InputFader_setMul, METH_O, "Sets oscillator mul factor."},
//	{"setAdd", (PyCFunction)InputFader_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

PyTypeObject InputFaderType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.InputFader_base",         /*tp_name*/
    sizeof(InputFader),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)InputFader_dealloc, /*tp_dealloc*/
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
    "InputFader objects. Generates a crossfade between current input sound stream and new input sound stream.",  /* tp_doc */
    (traverseproc)InputFader_traverse,   /* tp_traverse */
    (inquiry)InputFader_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    InputFader_methods,             /* tp_methods */
    InputFader_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)InputFader_init,      /* tp_init */
    0,                         /* tp_alloc */
    InputFader_new,                 /* tp_new */
};

