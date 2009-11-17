#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "lo/lo.h"

/* main OSC receiver */
typedef struct {
    pyo_audio_HEAD
    lo_server osc_server;
    int port;
    PyObject *dict;
    PyObject *address_path;
} OscReceiver;

void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

int OscReceiver_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        void *data, void *user_data)
{
    OscReceiver *self = user_data;
    PyDict_SetItem(self->dict, PyString_FromString(path), PyFloat_FromDouble(argv[0]->f));
    return 0;
}

float OscReceiver_getValue(OscReceiver *self, PyObject *path)
{
    PyObject *tmp;
    tmp = PyDict_GetItem(self->dict, path);
    return PyFloat_AsDouble(tmp);
}

static void
OscReceiver_compute_next_data_frame(OscReceiver *self)
{
    lo_server_recv_noblock(self->osc_server, 0);
}

static int
OscReceiver_traverse(OscReceiver *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
OscReceiver_clear(OscReceiver *self)
{
    pyo_CLEAR
    return 0;
}

static void
OscReceiver_dealloc(OscReceiver* self)
{
    lo_server_free(self->osc_server);
    free(self->data);
    OscReceiver_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscReceiver_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    OscReceiver *self;
    self = (OscReceiver *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscReceiver_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
OscReceiver_init(OscReceiver *self, PyObject *args, PyObject *kwds)
{
    PyObject *pathtmp;
    Py_ssize_t i;
    
    static char *kwlist[] = {"port", "address", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "iO|", kwlist, &self->port, &pathtmp))
        return -1; 
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->dict = PyDict_New();
    
    if (PyString_Check(pathtmp) || PyList_Check(pathtmp)) {
        Py_INCREF(pathtmp);    
        Py_XDECREF(self->address_path);
        self->address_path = pathtmp;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string or a list of strings.");
        return -1;
    }    
    
    if (PyString_Check(self->address_path)) {
        PyDict_SetItem(self->dict, self->address_path, PyFloat_FromDouble(0.));
    }    
    else if (PyList_Check(self->address_path)) {
        Py_ssize_t lsize = PyList_Size(self->address_path);
        for (i=0; i<lsize; i++) {
            PyDict_SetItem(self->dict, PyList_GET_ITEM(self->address_path, i), PyFloat_FromDouble(0.));
        }    
    }
    
    char buf[20];
    sprintf(buf, "%i", self->port);
    self->osc_server = lo_server_new(buf, error);
    
    lo_server_add_method(self->osc_server, NULL, "f", OscReceiver_handler, self);
    
    OscReceiver_compute_next_data_frame((OscReceiver *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * OscReceiver_getServer(OscReceiver* self) { GET_SERVER };
static PyObject * OscReceiver_getStream(OscReceiver* self) { GET_STREAM };

static PyMemberDef OscReceiver_members[] = {
{"server", T_OBJECT_EX, offsetof(OscReceiver, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(OscReceiver, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef OscReceiver_methods[] = {
{"getServer", (PyCFunction)OscReceiver_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)OscReceiver_getStream, METH_NOARGS, "Returns stream object."},
{NULL}  /* Sentinel */
};

PyTypeObject OscReceiverType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.OscReceiver_base",         /*tp_name*/
sizeof(OscReceiver),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)OscReceiver_dealloc, /*tp_dealloc*/
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
"OscReceiver objects. Receive values via Open Sound Control protocol.",           /* tp_doc */
(traverseproc)OscReceiver_traverse,   /* tp_traverse */
(inquiry)OscReceiver_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
OscReceiver_methods,             /* tp_methods */
OscReceiver_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)OscReceiver_init,      /* tp_init */
0,                         /* tp_alloc */
OscReceiver_new,                 /* tp_new */
};



/* OSC receiver stream object */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    PyObject *address_path;
    float oldValue;
    float value;
    int modebuffer[2];
} OscReceive;

static void OscReceive_postprocessing_ii(OscReceive *self) { POST_PROCESSING_II };
static void OscReceive_postprocessing_ai(OscReceive *self) { POST_PROCESSING_AI };
static void OscReceive_postprocessing_ia(OscReceive *self) { POST_PROCESSING_IA };
static void OscReceive_postprocessing_aa(OscReceive *self) { POST_PROCESSING_AA };

static void
OscReceive_setProcMode(OscReceive *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = OscReceive_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = OscReceive_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = OscReceive_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = OscReceive_postprocessing_aa;
            break;
    }    
}

static void
OscReceive_compute_next_data_frame(OscReceive *self)
{
    int i;
    self->value = OscReceiver_getValue((OscReceiver *)self->input, self->address_path);
    float step = (self->value - self->oldValue) / self->bufsize;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->oldValue + step;
    }  
    self->oldValue = self->value;
    
    Stream_setData(self->stream, self->data);    
    (*self->muladd_func_ptr)(self);
}

static int
OscReceive_traverse(OscReceive *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
OscReceive_clear(OscReceive *self)
{
    pyo_CLEAR
    return 0;
}

static void
OscReceive_dealloc(OscReceive* self)
{
    free(self->data);
    OscReceive_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscReceive_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    OscReceive *self;
    self = (OscReceive *)type->tp_alloc(type, 0);

    self->oldValue = 0.;
    self->value = 0.;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscReceive_compute_next_data_frame);
    self->mode_func_ptr = OscReceive_setProcMode;

    return (PyObject *)self;
}

static int
OscReceive_init(OscReceive *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp=NULL, *pathtmp=NULL, *multmp=NULL, *addtmp=NULL;;

    static char *kwlist[] = {"input", "address", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist, &inputtmp, &pathtmp, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->input);
    Py_INCREF(inputtmp);
    self->input = inputtmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
        return -1;
    }    
        
    Py_INCREF(pathtmp);    
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;
        
    (*self->mode_func_ptr)(self);

    OscReceive_compute_next_data_frame((OscReceive *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * OscReceive_getServer(OscReceive* self) { GET_SERVER };
static PyObject * OscReceive_getStream(OscReceive* self) { GET_STREAM };
static PyObject * OscReceive_setMul(OscReceive *self, PyObject *arg) { SET_MUL };	
static PyObject * OscReceive_setAdd(OscReceive *self, PyObject *arg) { SET_ADD };	

static PyObject * OscReceive_play(OscReceive *self) { PLAY };
static PyObject * OscReceive_stop(OscReceive *self) { STOP };

static PyObject * OscReceive_multiply(OscReceive *self, PyObject *arg) { MULTIPLY };
static PyObject * OscReceive_inplace_multiply(OscReceive *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscReceive_add(OscReceive *self, PyObject *arg) { ADD };
static PyObject * OscReceive_inplace_add(OscReceive *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef OscReceive_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscReceive, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscReceive, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(OscReceive, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscReceive, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscReceive_methods[] = {
    {"getServer", (PyCFunction)OscReceive_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscReceive_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscReceive_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)OscReceive_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)OscReceive_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscReceive_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods OscReceive_as_number = {
    (binaryfunc)OscReceive_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)OscReceive_multiply,                 /*nb_multiply*/
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
    (binaryfunc)OscReceive_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)OscReceive_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject OscReceiveType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.OscReceive_base",         /*tp_name*/
    sizeof(OscReceive),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)OscReceive_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &OscReceive_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscReceive objects. Receive values via Open Sound Control protocol.",           /* tp_doc */
    (traverseproc)OscReceive_traverse,   /* tp_traverse */
    (inquiry)OscReceive_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    OscReceive_methods,             /* tp_methods */
    OscReceive_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)OscReceive_init,      /* tp_init */
    0,                         /* tp_alloc */
    OscReceive_new,                 /* tp_new */
};

/* OSC send object */
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    lo_address address;
    char *host;
    int port;
    PyObject *address_path;
} OscSend;

static void
OscSend_compute_next_data_frame(OscSend *self)
{
    float *in = Stream_getData((Stream *)self->input_stream);
    float value = in[0];
    
    char *path  = PyString_AsString(self->address_path);
    
    if (lo_send(self->address, path, "f", value) == -1) {
        printf("OSC error %d: %s\n", lo_address_errno(self->address), lo_address_errstr(self->address));
    }    
}

static int
OscSend_traverse(OscSend *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int 
OscSend_clear(OscSend *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
OscSend_dealloc(OscSend* self)
{
    free(self->data);
    OscSend_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
OscSend_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    OscSend *self;
    self = (OscSend *)type->tp_alloc(type, 0);
    
    self->host = NULL;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, OscSend_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
OscSend_init(OscSend *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *pathtmp;
    
    static char *kwlist[] = {"input", "port", "address", "host", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OiO|s", kwlist, &inputtmp, &self->port, &pathtmp, &self->host))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    if (! PyString_Check(pathtmp)) {
        PyErr_SetString(PyExc_TypeError, "The address attributes must be a string.");
        return -1;
    }    
    
    Py_INCREF(pathtmp);    
    Py_XDECREF(self->address_path);
    self->address_path = pathtmp;
    
    char buf[20];
    sprintf(buf, "%i", self->port);
    self->address = lo_address_new(self->host, buf);
    
    OscSend_compute_next_data_frame((OscSend *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * OscSend_getServer(OscSend* self) { GET_SERVER };
static PyObject * OscSend_getStream(OscSend* self) { GET_STREAM };

static PyObject * OscSend_play(OscSend *self) { PLAY };
static PyObject * OscSend_stop(OscSend *self) { STOP };

static PyObject * OscSend_multiply(OscSend *self, PyObject *arg) { MULTIPLY };
static PyObject * OscSend_inplace_multiply(OscSend *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscSend_add(OscSend *self, PyObject *arg) { ADD };
static PyObject * OscSend_inplace_add(OscSend *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef OscSend_members[] = {
{"server", T_OBJECT_EX, offsetof(OscSend, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(OscSend, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(OscSend, input), 0, "Input sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef OscSend_methods[] = {
{"getServer", (PyCFunction)OscSend_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)OscSend_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)OscSend_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)OscSend_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject OscSendType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.OscSend_base",         /*tp_name*/
sizeof(OscSend),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)OscSend_dealloc, /*tp_dealloc*/
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
"OscSend objects. Send values via Open Sound Control protocol.",           /* tp_doc */
(traverseproc)OscSend_traverse,   /* tp_traverse */
(inquiry)OscSend_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
OscSend_methods,             /* tp_methods */
OscSend_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)OscSend_init,      /* tp_init */
0,                         /* tp_alloc */
OscSend_new,                 /* tp_new */
};

