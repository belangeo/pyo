#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    float value;
    int modebuffer[2];
} Sig;

static void Sig_postprocessing_ii(Sig *self) { POST_PROCESSING_II };
static void Sig_postprocessing_ai(Sig *self) { POST_PROCESSING_AI };
static void Sig_postprocessing_ia(Sig *self) { POST_PROCESSING_IA };
static void Sig_postprocessing_aa(Sig *self) { POST_PROCESSING_AA };
static void Sig_postprocessing_ireva(Sig *self) { POST_PROCESSING_IREVA };
static void Sig_postprocessing_areva(Sig *self) { POST_PROCESSING_AREVA };
static void Sig_postprocessing_revai(Sig *self) { POST_PROCESSING_REVAI };
static void Sig_postprocessing_revaa(Sig *self) { POST_PROCESSING_REVAA };
static void Sig_postprocessing_revareva(Sig *self) { POST_PROCESSING_REVAREVA };

static void
Sig_setProcMode(Sig *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Sig_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Sig_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Sig_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Sig_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Sig_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Sig_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Sig_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Sig_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Sig_postprocessing_revareva;
            break;
    }
}

static void
Sig_compute_next_data_frame(Sig *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = self->value;
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Sig_traverse(Sig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Sig_clear(Sig *self)
{
    pyo_CLEAR
    return 0;
}

static void
Sig_dealloc(Sig* self)
{
    free(self->data);
    Sig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Sig_deleteStream(Sig *self) { DELETE_STREAM };

static PyObject *
Sig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Sig *self;
    self = (Sig *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Sig_compute_next_data_frame);
    self->mode_func_ptr = Sig_setProcMode;
    
    return (PyObject *)self;
}

static int
Sig_init(Sig *self, PyObject *args, PyObject *kwds)
{
    PyObject *valuetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"value", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OO", kwlist, &valuetmp, &multmp, &addtmp))
        return -1; 

    if (valuetmp) {
        PyObject_CallMethod((PyObject *)self, "setValue", "O", valuetmp);
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
    
    srand((unsigned)(time(0)));
    Sig_compute_next_data_frame((Sig *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject *
Sig_setValue(Sig *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	if (isNumber == 1)
		self->value = PyFloat_AsDouble(PyNumber_Float(tmp));
    else
        self->value = 1.;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject * Sig_getServer(Sig* self) { GET_SERVER };
static PyObject * Sig_getStream(Sig* self) { GET_STREAM };
static PyObject * Sig_setMul(Sig *self, PyObject *arg) { SET_MUL };	
static PyObject * Sig_setAdd(Sig *self, PyObject *arg) { SET_ADD };	
static PyObject * Sig_setSub(Sig *self, PyObject *arg) { SET_SUB };	
static PyObject * Sig_setDiv(Sig *self, PyObject *arg) { SET_DIV };	

static PyObject * Sig_play(Sig *self) { PLAY };
static PyObject * Sig_stop(Sig *self) { STOP };

static PyObject * Sig_multiply(Sig *self, PyObject *arg) { MULTIPLY };
static PyObject * Sig_inplace_multiply(Sig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Sig_add(Sig *self, PyObject *arg) { ADD };
static PyObject * Sig_inplace_add(Sig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Sig_sub(Sig *self, PyObject *arg) { SUB };
static PyObject * Sig_inplace_sub(Sig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Sig_div(Sig *self, PyObject *arg) { DIV };
static PyObject * Sig_inplace_div(Sig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Sig_members[] = {
{"server", T_OBJECT_EX, offsetof(Sig, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Sig, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Sig, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Sig, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Sig_methods[] = {
{"getServer", (PyCFunction)Sig_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Sig_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Sig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Sig_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Sig_stop, METH_NOARGS, "Stops computing."},
{"setValue", (PyCFunction)Sig_setValue, METH_O, "Sets Sig value."},
{"setMul", (PyCFunction)Sig_setMul, METH_O, "Sets Sig mul factor."},
{"setAdd", (PyCFunction)Sig_setAdd, METH_O, "Sets Sig add factor."},
{"setSub", (PyCFunction)Sig_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Sig_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Sig_as_number = {
(binaryfunc)Sig_add,                      /*nb_add*/
(binaryfunc)Sig_sub,                 /*nb_subtract*/
(binaryfunc)Sig_multiply,                 /*nb_multiply*/
(binaryfunc)Sig_div,                   /*nb_divide*/
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
(binaryfunc)Sig_inplace_add,              /*inplace_add*/
(binaryfunc)Sig_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Sig_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Sig_inplace_div,           /*inplace_divide*/
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

PyTypeObject SigType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Sig_base",         /*tp_name*/
sizeof(Sig),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Sig_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Sig_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Sig objects. Converts number into a signal stream.",           /* tp_doc */
(traverseproc)Sig_traverse,   /* tp_traverse */
(inquiry)Sig_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Sig_methods,             /* tp_methods */
Sig_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Sig_init,      /* tp_init */
0,                         /* tp_alloc */
Sig_new,                 /* tp_new */
};

