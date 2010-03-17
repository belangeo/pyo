#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
//#include "dummymodule.h"

/************/
/* Print */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int method; // 0 -> interval, 1 -> change
    float lastValue;
    float time;
    float currentTime;
    float sampleToSec;
} Print;

static void
Print_process_time(Print *self) {
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime > = self->time) {
            self->currentTime = 0.0;
            printf("%f\n", in[i]);
        }
        self->currentTime += self->sampleToSec;
    }
}

static void
Print_process_change(Print *self) {
    int i;
    float inval;
    float *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval != self->lastValue) {
            printf("%f\n", inval);
            self->lastValue = inval;
        }
    }    
}

static void
Print_setProcMode(Print *self)
{    
    if (self->method < 0 || self->method > 1)
        self->method = 0;
        
    switch (self->method) {
        case 0:
            self->proc_func_ptr = Print_process_time;
            break;
        case 1:
            self->proc_func_ptr = Print_process_change;
            break;
    }        
}

static void
Print_compute_next_data_frame(Print *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
Print_traverse(Print *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Print_clear(Print *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Print_dealloc(Print* self)
{
    free(self->data);
    Print_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Print_deleteStream(Print *self) { DELETE_STREAM };

static PyObject *
Print_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Print *self;
    self = (Print *)type->tp_alloc(type, 0);

    self->method = 0;
    self->time = 1.0;
    self->lastValue = -99999.0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Print_compute_next_data_frame);
    self->mode_func_ptr = Print_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;
    
    return (PyObject *)self;
}

static int
Print_init(Print *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "method", "interval", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|if", kwlist, &inputtmp, &self->method, &self->time))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }
    
    Stream_setData(self->stream, self->data);

    (*self->mode_func_ptr)(self);
    
    Print_compute_next_data_frame((Print *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Print_getServer(Print* self) { GET_SERVER };
static PyObject * Print_getStream(Print* self) { GET_STREAM };

static PyObject * Print_play(Print *self) { PLAY };
static PyObject * Print_stop(Print *self) { STOP };

static PyObject *
Print_setMethod(Print *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->method = PyInt_AsLong(arg);
        (*self->mode_func_ptr)(self);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Print_setInterval(Print *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	if (isNumber == 1) {
		self->time = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Print_members[] = {
{"server", T_OBJECT_EX, offsetof(Print, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Print, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Print, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Print_methods[] = {
{"getServer", (PyCFunction)Print_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Print_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Print_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Print_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Print_stop, METH_NOARGS, "Stops computing."},
{"setInterval", (PyCFunction)Print_setInterval, METH_O, "Sets the time interval."},
{NULL}  /* Sentinel */
};

PyTypeObject PrintType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Print_base",                                   /*tp_name*/
sizeof(Print),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Print_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,                                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Print objects. Print the current value of the input object.",           /* tp_doc */
(traverseproc)Print_traverse,                  /* tp_traverse */
(inquiry)Print_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Print_methods,                                 /* tp_methods */
Print_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Print_init,                          /* tp_init */
0,                                              /* tp_alloc */
Print_new,                                     /* tp_new */
};
