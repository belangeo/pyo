#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    long value;
    float last_value;
} Select;

static void
Select_selector(Select *self) {
    float val, selval, inval;
    int i;

    float *in = Stream_getData((Stream *)self->input_stream);
    
    selval = (float)self->value;
    for (i=0; i<self->bufsize; i++) {
        inval = in[i];
        if (inval == self->value && inval != self->last_value)
            val = 1;
        else
            val = 0;
        
        self->last_value = inval;
        self->data[i] = val;
    }
}

static void
Select_setProcMode(Select *self)
{
    self->proc_func_ptr = Select_selector;
}

static void
Select_compute_next_data_frame(Select *self)
{
    (*self->proc_func_ptr)(self);    
    Stream_setData(self->stream, self->data);
}

static int
Select_traverse(Select *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);    
    Py_VISIT(self->input_stream);    
    return 0;
}

static int 
Select_clear(Select *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);    
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Select_dealloc(Select* self)
{
    free(self->data);
    Select_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Select_deleteStream(Select *self) { DELETE_STREAM };

static PyObject *
Select_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Select *self;
    self = (Select *)type->tp_alloc(type, 0);
    
    self->value = 0;
    self->last_value = -99.0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Select_compute_next_data_frame);
    self->mode_func_ptr = Select_setProcMode;
    
    return (PyObject *)self;
}

static int
Select_init(Select *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "value", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &inputtmp, &self->value))
        return -1; 

    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    Select_compute_next_data_frame((Select *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Select_getServer(Select* self) { GET_SERVER };
static PyObject * Select_getStream(Select* self) { GET_STREAM };

static PyObject * Select_play(Select *self) { PLAY };
static PyObject * Select_stop(Select *self) { STOP };

static PyObject *
Select_setValue(Select *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	if (PyLong_Check(arg) || PyInt_Check(arg)) {	
		self->value = PyLong_AsLong(arg);
	}
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Select_members[] = {
{"server", T_OBJECT_EX, offsetof(Select, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Select, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Select_methods[] = {
{"getServer", (PyCFunction)Select_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Select_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Select_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Select_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Select_stop, METH_NOARGS, "Stops computing."},
{"setValue", (PyCFunction)Select_setValue, METH_O, "Sets value to select."},
{NULL}  /* Sentinel */
};

PyTypeObject SelectType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Select_base",         /*tp_name*/
sizeof(Select),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Select_dealloc, /*tp_dealloc*/
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
"Select objects. Watch input and send a trig on a selected value.",           /* tp_doc */
(traverseproc)Select_traverse,   /* tp_traverse */
(inquiry)Select_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Select_methods,             /* tp_methods */
Select_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Select_init,      /* tp_init */
0,                         /* tp_alloc */
Select_new,                 /* tp_new */
};
