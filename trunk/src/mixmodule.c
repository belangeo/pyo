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
    int modebuffer[2];
} Mix;

static void Mix_postprocessing_ii(Mix *self) { POST_PROCESSING_II };
static void Mix_postprocessing_ai(Mix *self) { POST_PROCESSING_AI };
static void Mix_postprocessing_ia(Mix *self) { POST_PROCESSING_IA };
static void Mix_postprocessing_aa(Mix *self) { POST_PROCESSING_AA };

static void
_setProcMode(Mix *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Mix_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Mix_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = Mix_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Mix_postprocessing_aa;
            break;
    }    
}

static void
Mix_compute_next_data_frame(Mix *self)
{   
    int i, j;
    float old;
    PyObject *stream;
    Py_ssize_t lsize = PyList_Size(self->input);

    float buffer[self->bufsize];
    memset(&buffer, 0, sizeof(buffer));

    for (i=0; i<lsize; i++) {
        stream = PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, i), "_getStream", NULL);
        float *in = Stream_getData((Stream *)stream);
        for (j=0; j<self->bufsize; j++) {
            old = buffer[j];
            buffer[j] = in[j] + old;
        }    
    }    
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = buffer[i];
    }
    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Mix_traverse(Mix *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Mix_clear(Mix *self)
{
    pyo_CLEAR
    return 0;
}

static void
Mix_dealloc(Mix* self)
{
    free(self->data);
    Mix_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Mix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Mix *self;
    self = (Mix *)type->tp_alloc(type, 0);

	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Mix_compute_next_data_frame);

    return (PyObject *)self;
}

static int
Mix_init(Mix *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &inputtmp, &multmp, &addtmp))
        return -1; 

    Py_INCREF(inputtmp);
    Py_XDECREF(self->input);
    self->input = inputtmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    _setProcMode(self);

    Mix_compute_next_data_frame((Mix *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Mix_getServer(Mix* self) { GET_SERVER };
static PyObject * Mix_getStream(Mix* self) { GET_STREAM };
static PyObject * Mix_setMul(Mix *self, PyObject *arg) { SET_MUL };	
static PyObject * Mix_setAdd(Mix *self, PyObject *arg) { SET_ADD };	

static PyObject * Mix_play(Mix *self) { PLAY };
static PyObject * Mix_out(Mix *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Mix_stop(Mix *self) { STOP };

static PyObject * Mix_multiply(Mix *self, PyObject *arg) { MULTIPLY };
static PyObject * Mix_inplace_multiply(Mix *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Mix_add(Mix *self, PyObject *arg) { ADD };
static PyObject * Mix_inplace_add(Mix *self, PyObject *arg) { INPLACE_ADD };

static PyMemberDef Mix_members[] = {
    {"server", T_OBJECT_EX, offsetof(Mix, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Mix, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(Mix, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Mix, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Mix_methods[] = {
    //{"getMix", (PyCFunction)Mix_getTable, METH_NOARGS, "Returns input sound object."},
    {"getServer", (PyCFunction)Mix_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Mix_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)Mix_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Mix_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Mix_stop, METH_NOARGS, "Stops computing."},
	{"setMul", (PyCFunction)Mix_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Mix_setAdd, METH_O, "Sets oscillator add factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Mix_as_number = {
    (binaryfunc)Mix_add,                      /*nb_add*/
    0,                 /*nb_subtract*/
    (binaryfunc)Mix_multiply,                 /*nb_multiply*/
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
    (binaryfunc)Mix_inplace_add,              /*inplace_add*/
    0,         /*inplace_subtract*/
    (binaryfunc)Mix_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject MixType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Mix_base",         /*tp_name*/
    sizeof(Mix),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Mix_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Mix_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Mix objects. Retreive audio from an input channel.",           /* tp_doc */
    (traverseproc)Mix_traverse,   /* tp_traverse */
    (inquiry)Mix_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Mix_methods,             /* tp_methods */
    Mix_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Mix_init,      /* tp_init */
    0,                         /* tp_alloc */
    Mix_new,                 /* tp_new */
};

