#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"

/************/
/* Convolve */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
    float *impulse;
    float *input_tmp;
    char *path;
    int size;
    int sndSr;
    int chnl;
    int count;
} Convolve;

static void
Convolve_loadSound(Convolve *self) {
    SNDFILE *sf;
    SF_INFO info;
    unsigned int i, num, num_items, num_chnls;
    float val;
    float *tmp;
    
    /* Open the WAV file. */
    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("Failed to open the file.\n");
    }
    /* Print some of the info, and figure out how much data to read. */
    self->size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;
    printf("load impulse response...\n");
    printf("samples = %d\n", self->size);
    printf("samplingrate = %d\n", self->sndSr);
    printf("channels = %d\n", num_chnls);
    num_items = self->size * num_chnls;
    //printf("num_items=%d\n",num_items);
    /* Allocate space for the data to be read, then read it. */
    self->impulse = (float *)realloc(self->impulse, self->size * sizeof(float));
    tmp = (float *)malloc(num_items * sizeof(float));
    num = sf_read_float(sf, tmp, num_items);
    sf_close(sf);
    for (i=0; i<num_items; i++) {
        if ((i % num_chnls) == self->chnl) {
            self->impulse[(int)(i/num_chnls)] = tmp[i];
        }    
    }
    
    free(tmp);
    self->input_tmp = (float *)realloc(self->input_tmp, self->size * sizeof(float));
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = 0.0;
    }
}

static void
Convolve_filters(Convolve *self) {
    int i,j,tmp_count;
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * self->impulse[j];
        }
        
        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void Convolve_postprocessing_ii(Convolve *self) { POST_PROCESSING_II };
static void Convolve_postprocessing_ai(Convolve *self) { POST_PROCESSING_AI };
static void Convolve_postprocessing_ia(Convolve *self) { POST_PROCESSING_IA };
static void Convolve_postprocessing_aa(Convolve *self) { POST_PROCESSING_AA };
static void Convolve_postprocessing_ireva(Convolve *self) { POST_PROCESSING_IREVA };
static void Convolve_postprocessing_areva(Convolve *self) { POST_PROCESSING_AREVA };
static void Convolve_postprocessing_revai(Convolve *self) { POST_PROCESSING_REVAI };
static void Convolve_postprocessing_revaa(Convolve *self) { POST_PROCESSING_REVAA };
static void Convolve_postprocessing_revareva(Convolve *self) { POST_PROCESSING_REVAREVA };

static void
Convolve_setProcMode(Convolve *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = Convolve_filters;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Convolve_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Convolve_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Convolve_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Convolve_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Convolve_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Convolve_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Convolve_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Convolve_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Convolve_postprocessing_revareva;
            break;
    }   
}

static void
Convolve_compute_next_data_frame(Convolve *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Convolve_traverse(Convolve *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
Convolve_clear(Convolve *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Convolve_dealloc(Convolve* self)
{
    free(self->data);
    Convolve_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Convolve_deleteStream(Convolve *self) { DELETE_STREAM };

static PyObject *
Convolve_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Convolve *self;
    self = (Convolve *)type->tp_alloc(type, 0);
    
    self->chnl = 0;
    self->count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Convolve_compute_next_data_frame);
    self->mode_func_ptr = Convolve_setProcMode;
    return (PyObject *)self;
}

static int
Convolve_init(Convolve *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "path", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os|iOO", kwlist, &inputtmp, &self->path, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    Convolve_loadSound(self);

    Convolve_compute_next_data_frame((Convolve *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Convolve_getServer(Convolve* self) { GET_SERVER };
static PyObject * Convolve_getStream(Convolve* self) { GET_STREAM };
static PyObject * Convolve_setMul(Convolve *self, PyObject *arg) { SET_MUL };	
static PyObject * Convolve_setAdd(Convolve *self, PyObject *arg) { SET_ADD };	
static PyObject * Convolve_setSub(Convolve *self, PyObject *arg) { SET_SUB };	
static PyObject * Convolve_setDiv(Convolve *self, PyObject *arg) { SET_DIV };	

static PyObject * Convolve_play(Convolve *self) { PLAY };
static PyObject * Convolve_out(Convolve *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Convolve_stop(Convolve *self) { STOP };

static PyObject * Convolve_multiply(Convolve *self, PyObject *arg) { MULTIPLY };
static PyObject * Convolve_inplace_multiply(Convolve *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Convolve_add(Convolve *self, PyObject *arg) { ADD };
static PyObject * Convolve_inplace_add(Convolve *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Convolve_sub(Convolve *self, PyObject *arg) { SUB };
static PyObject * Convolve_inplace_sub(Convolve *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Convolve_div(Convolve *self, PyObject *arg) { DIV };
static PyObject * Convolve_inplace_div(Convolve *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Convolve_members[] = {
{"server", T_OBJECT_EX, offsetof(Convolve, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Convolve, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Convolve, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Convolve, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Convolve, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Convolve_methods[] = {
{"getServer", (PyCFunction)Convolve_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Convolve_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Convolve_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Convolve_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Convolve_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Convolve_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Convolve_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Convolve_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Convolve_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Convolve_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Convolve_as_number = {
(binaryfunc)Convolve_add,                         /*nb_add*/
(binaryfunc)Convolve_sub,                         /*nb_subtract*/
(binaryfunc)Convolve_multiply,                    /*nb_multiply*/
(binaryfunc)Convolve_div,                                              /*nb_divide*/
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
(binaryfunc)Convolve_inplace_add,                 /*inplace_add*/
(binaryfunc)Convolve_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Convolve_inplace_multiply,            /*inplace_multiply*/
(binaryfunc)Convolve_inplace_div,                                              /*inplace_divide*/
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

PyTypeObject ConvolveType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Convolve_base",                                   /*tp_name*/
sizeof(Convolve),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Convolve_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
&Convolve_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Convolve objects. Implements a circular convolution.",           /* tp_doc */
(traverseproc)Convolve_traverse,                  /* tp_traverse */
(inquiry)Convolve_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Convolve_methods,                                 /* tp_methods */
Convolve_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Convolve_init,                          /* tp_init */
0,                                              /* tp_alloc */
Convolve_new,                                     /* tp_new */
};
