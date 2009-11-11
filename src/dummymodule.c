#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

static void Dummy_postprocessing_ii(Dummy *self) { POST_PROCESSING_II };
static void Dummy_postprocessing_ai(Dummy *self) { POST_PROCESSING_AI };
static void Dummy_postprocessing_ia(Dummy *self) { POST_PROCESSING_IA };
static void Dummy_postprocessing_aa(Dummy *self) { POST_PROCESSING_AA };

static void
_setProcMode(Dummy *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Dummy_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Dummy_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Dummy_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Dummy_postprocessing_aa;
            break;
    }  
}

static void
_compute_next_data_frame(Dummy *self)
{
    int i;
    float *in = Stream_getData((Stream *)self->input_stream);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Dummy_traverse(Dummy *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int 
Dummy_clear(Dummy *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
Dummy_dealloc(Dummy* self)
{
    free(self->data);
    Dummy_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

PyObject *
Dummy_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Dummy *self;
    self = (Dummy *)type->tp_alloc(type, 0);
        
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);    

    return (PyObject *)self;
}

PyObject * 
Dummy_initialize(Dummy *self)
{
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    Stream_setStreamActive(self->stream, 1);
    
    Py_INCREF(Py_None);
    return Py_None;
}

int
Dummy_init(Dummy *self, PyObject *args, PyObject *kwds)
{
    
    Py_INCREF(self);
    return 0;
}

static PyObject *
Dummy_setInput(Dummy *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    tmp = arg;
    Py_INCREF(tmp);
    Py_XDECREF(self->input);
    self->input = tmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)streamtmp;

    _setProcMode(self);
    _compute_next_data_frame((Dummy *)self);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * Dummy_getServer(Dummy* self) { GET_SERVER };
static PyObject * Dummy_getStream(Dummy* self) { GET_STREAM };
static PyObject * Dummy_setMul(Dummy *self, PyObject *arg) { SET_MUL };	
static PyObject * Dummy_setAdd(Dummy *self, PyObject *arg) { SET_ADD };	

static PyObject * Dummy_play(Dummy *self) { PLAY };
static PyObject * Dummy_out(Dummy *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Dummy_stop(Dummy *self) { STOP };

static PyObject * Dummy_multiply(Dummy *self, PyObject *arg) { MULTIPLY };
static PyObject * Dummy_inplace_multiply(Dummy *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Dummy_add(Dummy *self, PyObject *arg) { ADD };
static PyObject * Dummy_inplace_add(Dummy *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef Dummy_members[] = {
    {"server", T_OBJECT_EX, offsetof(Dummy, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Dummy, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Dummy, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(Dummy, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Dummy, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Dummy_methods[] = {
    //{"getInput", (PyCFunction)Dummy_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Dummy_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Dummy_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Dummy_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Dummy_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Dummy_stop, METH_NOARGS, "Stops computing."},
	{"setInput", (PyCFunction)Dummy_setInput, METH_O, "Sets the input sound object."},
	{"setMul", (PyCFunction)Dummy_setMul, METH_O, "Sets mul factor."},
	{"setAdd", (PyCFunction)Dummy_setAdd, METH_O, "Sets add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Dummy_as_number = {
    (binaryfunc)Dummy_add,                         /*nb_add*/
    0,                                              /*nb_subtract*/
    (binaryfunc)Dummy_multiply,                    /*nb_multiply*/
    0,                                              /*nb_divide*/
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
    (binaryfunc)Dummy_inplace_add,                 /*inplace_add*/
    0,                                              /*inplace_subtract*/
    (binaryfunc)Dummy_inplace_multiply,            /*inplace_multiply*/
    0,                                              /*inplace_divide*/
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

PyTypeObject DummyType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "pyo.Dummy",                                   /*tp_name*/
    sizeof(Dummy),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)Dummy_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    &Dummy_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Dummy objects. Generates a biquadratic filter.",           /* tp_doc */
    (traverseproc)Dummy_traverse,                  /* tp_traverse */
    (inquiry)Dummy_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    Dummy_methods,                                 /* tp_methods */
    Dummy_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)Dummy_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    Dummy_new,                                     /* tp_new */
};

