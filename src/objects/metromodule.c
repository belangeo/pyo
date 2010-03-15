#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

/****************/
/**** Metro *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *time;
    Stream *time_stream;
    int modebuffer[1];
    float sampleToSec;
    float currentTime;
    float offset;
    int flag;
} Metro;

static void
Metro_generate_i(Metro *self) {
    float val, tm, off;
    int i;
    
    tm = PyFloat_AS_DOUBLE(self->time);
    off = tm * self->offset;
    
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm) {
            val = 0;
            self->currentTime = 0.;
            self->flag = 1;
        }    
        else if (self->currentTime >= off && self->flag == 1) {
            val = 1;
            self->flag = 0;
        }    
        else
            val = 0;
        
        self->data[i] = val;
        self->currentTime += self->sampleToSec;
    }
}

static void
Metro_generate_a(Metro *self) {
    float val, off;
    int i;
    
    float *tm = Stream_getData((Stream *)self->time_stream);
    
    for (i=0; i<self->bufsize; i++) {
        off = tm[i] * self->offset;
        if (self->currentTime >= tm[i]) {
            val = 0;
            self->currentTime = 0.;
            self->flag = 1;
        }
        else if (self->currentTime >= off && self->flag == 1) {
            val = 1;
            self->flag = 0;
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
    self->flag = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Metro_compute_next_data_frame);
    self->mode_func_ptr = Metro_setProcMode;

    Stream_setStreamActive(self->stream, 0);

    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;
    
    return (PyObject *)self;
}

static int
Metro_init(Metro *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *timetmp=NULL;
    
    static char *kwlist[] = {"time", "offset", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Of", kwlist, &timetmp, &self->offset))
        return -1; 

    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }    
    Stream_setData(self->stream, self->data);
    
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

/****************/
/**** Cloud *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *density;
    Stream *density_stream;
    int modebuffer[1];
    int poly;
    int wait;
    unsigned int wait_count;
} Cloud;

static void
Cloud_generate_i(Cloud *self) {
    float maxrnd, val;
    int i, rnd;
    
    float dens = PyFloat_AS_DOUBLE(self->density);
    if (dens <= 0.0) {
        dens = 0.0;
        maxrnd = 32768;
    }    
    else
        maxrnd = (1.0 / dens) * 1500 * self->poly;
    
    for (i=0; i<self->bufsize; i++) {
        val = 0;
        if (self->wait_count >= self->wait && dens > 0.0) {
            rnd = (int)(rand()/(float)(RAND_MAX)*maxrnd);
            if (rnd == 0) {
                val = 1.0;
                self->wait_count = 0;
            }
        }
        
        self->data[i] = val;
        self->wait_count++;
    }
}

static void
Cloud_generate_a(Cloud *self) {
    float maxrnd, val, dens;
    int i, rnd;
    
    float *density = Stream_getData((Stream *)self->density_stream);
    
    for (i=0; i<self->bufsize; i++) {
        dens = density[i];
        if (dens <= 0.0) {
            dens = 0.0;
            maxrnd = 32768;
        }    
        else
            maxrnd = (1.0 / (dens*dens)) * 1000 * self->poly;
        
        val = 0;
        if (self->wait_count >= self->wait && dens > 0.0) {
            rnd = (int)(rand()/(float)(RAND_MAX)*maxrnd);
            if (rnd == 0) {
                val = 1.0;
                self->wait_count = 0;
            }
        }
        
        self->data[i] = val;
        self->wait_count++;
    }
}


static void
Cloud_setProcMode(Cloud *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:        
            self->proc_func_ptr = Cloud_generate_i;
            break;
        case 1:    
            self->proc_func_ptr = Cloud_generate_a;
            break;
    }
}

static void
Cloud_compute_next_data_frame(Cloud *self)
{
    (*self->proc_func_ptr)(self);    
    Stream_setData(self->stream, self->data);
}

static int
Cloud_traverse(Cloud *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->density);    
    Py_VISIT(self->density_stream);    
    return 0;
}

static int 
Cloud_clear(Cloud *self)
{
    pyo_CLEAR
    Py_CLEAR(self->density);    
    Py_CLEAR(self->density_stream);
    return 0;
}

static void
Cloud_dealloc(Cloud* self)
{
    free(self->data);
    Cloud_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Cloud_deleteStream(Cloud *self) { DELETE_STREAM };

static PyObject *
Cloud_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Cloud *self;
    self = (Cloud *)type->tp_alloc(type, 0);
    
    self->density = PyFloat_FromDouble(0.5);
    self->poly = 1;
	self->modebuffer[0] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Cloud_compute_next_data_frame);
    self->mode_func_ptr = Cloud_setProcMode;
    
    Stream_setStreamActive(self->stream, 0);

    self->wait = (int)(0.001 * self->sr);
    self->wait_count = self->wait;
    
    return (PyObject *)self;
}

static int
Cloud_init(Cloud *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *densitytmp=NULL;
    
    static char *kwlist[] = {"density", "poly", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &densitytmp, &self->poly))
        return -1; 
    
    if (densitytmp) {
        PyObject_CallMethod((PyObject *)self, "setDensity", "O", densitytmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    srand((unsigned)(time(0)));

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }    
    Stream_setData(self->stream, self->data);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Cloud_getServer(Cloud* self) { GET_SERVER };
static PyObject * Cloud_getStream(Cloud* self) { GET_STREAM };

static PyObject * Cloud_play(Cloud *self) { PLAY };
static PyObject * Cloud_stop(Cloud *self) { STOP };

static PyObject *
Cloud_setDensity(Cloud *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->density);
	if (isNumber == 1) {
		self->density = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->density = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->density, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->density_stream);
        self->density_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Cloud_members[] = {
{"server", T_OBJECT_EX, offsetof(Cloud, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Cloud, stream), 0, "Stream object."},
{"density", T_OBJECT_EX, offsetof(Cloud, density), 0, "Cloud density factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Cloud_methods[] = {
{"getServer", (PyCFunction)Cloud_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Cloud_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Cloud_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Cloud_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Cloud_stop, METH_NOARGS, "Stops computing."},
{"setDensity", (PyCFunction)Cloud_setDensity, METH_O, "Sets density factor."},
{NULL}  /* Sentinel */
};

PyTypeObject CloudType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Cloud_base",         /*tp_name*/
sizeof(Cloud),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Cloud_dealloc, /*tp_dealloc*/
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
"Cloud objects. Create a cloud of triggers.",           /* tp_doc */
(traverseproc)Cloud_traverse,   /* tp_traverse */
(inquiry)Cloud_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Cloud_methods,             /* tp_methods */
Cloud_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Cloud_init,      /* tp_init */
0,                         /* tp_alloc */
Cloud_new,                 /* tp_new */
};

/****************/
/**** Trig *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    int flag;
} Trig;

static void
Trig_compute_next_data_frame(Trig *self)
{
    if (self->flag == 1) {
        self->data[0] = 1.0;
        self->flag = 0;
    }    
    else
        self->data[0] = 0.0;
    Stream_setData(self->stream, self->data);
}

static int
Trig_traverse(Trig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Trig_clear(Trig *self)
{
    pyo_CLEAR
    return 0;
}

static void
Trig_dealloc(Trig* self)
{
    free(self->data);
    Trig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Trig_deleteStream(Trig *self) { DELETE_STREAM };

static PyObject *
Trig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Trig *self;
    self = (Trig *)type->tp_alloc(type, 0);
    
    self->flag = 1;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Trig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
Trig_init(Trig *self, PyObject *args, PyObject *kwds)
{
    int i;
    
    static char *kwlist[] = {NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist))
        return -1; 

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }    
    
    Trig_compute_next_data_frame((Trig *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Trig_getServer(Trig* self) { GET_SERVER };
static PyObject * Trig_getStream(Trig* self) { GET_STREAM };

static PyObject * Trig_play(Trig *self) 
{ 
    self->flag = 1;
    PLAY 
};

static PyObject * Trig_stop(Trig *self) { STOP };

static PyMemberDef Trig_members[] = {
{"server", T_OBJECT_EX, offsetof(Trig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Trig, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Trig_methods[] = {
{"getServer", (PyCFunction)Trig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Trig_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Trig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Trig_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Trig_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TrigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Trig_base",         /*tp_name*/
sizeof(Trig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Trig_dealloc, /*tp_dealloc*/
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
"Trig objects. Sneds one trig.",           /* tp_doc */
(traverseproc)Trig_traverse,   /* tp_traverse */
(inquiry)Trig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Trig_methods,             /* tp_methods */
Trig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Trig_init,      /* tp_init */
0,                         /* tp_alloc */
Trig_new,                 /* tp_new */
};
