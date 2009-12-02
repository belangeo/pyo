#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *time;
    Stream *time_stream;
    int modebuffer[1];
    float sampleToSec;
    float currentTime;
} Metro;

static void
Metro_generate_i(Metro *self) {
    float val, tm;
    int i;
    
    tm = PyFloat_AS_DOUBLE(self->time);

    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm) {
            val = 1;
            self->currentTime = 0.;
        }    
        else
            val = 0;
        
        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}

static void
Metro_generate_a(Metro *self) {
    float val;
    int i;
    
    float *tm = Stream_getData((Stream *)self->time_stream);
    
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm[i]) {
            val = 1;
            self->currentTime = 0.;
        }    
        else
            val = 0;
        
        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}


static void
Metro_setProcMode(Metro *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:        
            self->proc_func_ptr = Metro_generate_i;
            break;
        case 1:    
            self->proc_func_ptr = Metro_generate_a;
            break;
    }
}

static void
Metro_compute_next_data_frame(Metro *self)
{
    (*self->proc_func_ptr)(self); 
    Stream_setData(self->stream, self->data);
}

static int
Metro_traverse(Metro *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->time);    
    Py_VISIT(self->time_stream);    
    return 0;
}

static int 
Metro_clear(Metro *self)
{
    pyo_CLEAR
    Py_CLEAR(self->time);    
    Py_CLEAR(self->time_stream);
    return 0;
}

static void
Metro_dealloc(Metro* self)
{
    free(self->data);
    Metro_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Metro_deleteStream(Metro *self) { DELETE_STREAM };

static PyObject *
Metro_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Metro *self;
    self = (Metro *)type->tp_alloc(type, 0);
    
    self->time = PyFloat_FromDouble(1.);
	self->modebuffer[0] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Metro_compute_next_data_frame);
    self->mode_func_ptr = Metro_setProcMode;

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;
    
    return (PyObject *)self;
}

static int
Metro_init(Metro *self, PyObject *args, PyObject *kwds)
{
    PyObject *timetmp=NULL;
    
    static char *kwlist[] = {"time", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &timetmp))
        return -1; 
 
    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    Metro_compute_next_data_frame((Metro *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Metro_getServer(Metro* self) { GET_SERVER };
static PyObject * Metro_getStream(Metro* self) { GET_STREAM };

static PyObject * Metro_play(Metro *self) { PLAY };
static PyObject * Metro_stop(Metro *self) { STOP };

static PyObject *
Metro_setTime(Metro *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Metro_members[] = {
{"server", T_OBJECT_EX, offsetof(Metro, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Metro, stream), 0, "Stream object."},
{"time", T_OBJECT_EX, offsetof(Metro, time), 0, "Metro time factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Metro_methods[] = {
{"getServer", (PyCFunction)Metro_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Metro_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Metro_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Metro_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Metro_stop, METH_NOARGS, "Stops computing."},
{"setTime", (PyCFunction)Metro_setTime, METH_O, "Sets time factor."},
{NULL}  /* Sentinel */
};

PyTypeObject MetroType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Metro_base",         /*tp_name*/
sizeof(Metro),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Metro_dealloc, /*tp_dealloc*/
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
"Metro objects. Create a metronome.",           /* tp_doc */
(traverseproc)Metro_traverse,   /* tp_traverse */
(inquiry)Metro_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Metro_methods,             /* tp_methods */
Metro_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Metro_init,      /* tp_init */
0,                         /* tp_alloc */
Metro_new,                 /* tp_new */
};

