#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"

/* SfPlayer object */
typedef struct {
    pyo_audio_HEAD
    PyObject *speed;
    Stream *speed_stream;
    int modebuffer[3];
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    int loop;
    int chnl;
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    float srScale;
    float startPos;
    float pointerPos;
} SfPlayer;

float max_arr(float *a,int n)
{
    int i;
    float m;
    m=*a;
    for (i=1; i<n; i++) {
        if (m < *(a+i)) 
            m = *(a+i);
    }    
    return m;
}


float *reverseArray(float *orig, unsigned short int b)
{
    unsigned short int a=0;
    float swap;

    for(a; a<--b; a++) { //increment a and decrement b until they meet eachother
        swap = orig[a];       //put what's in a into swap space
        orig[a] = orig[b];    //put what's in b into a
        orig[b] = swap;       //put what's in the swap (a) into b
    }

    return orig;    //return the new (reversed) string (a pointer to it)
}

static void
SfPlayer_readframes_i(SfPlayer *self) {
    float sp, frac, bufpos, delta, x, x1;
    int i, buflen, shortbuflen, bufindex;
    sf_count_t index;

    sp = PyFloat_AS_DOUBLE(self->speed);

    delta = sp * self->srScale;

    buflen = (int)(self->bufsize * delta) + 2;
    float buffer[self->sndChnls*buflen];
    float buffer2[self->sndChnls][buflen];

    index = (int)self->pointerPos;
    sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */

    /* fill a buffer with enough samples to satisfy speed reading */
    /* if not enough samples to read in the file */
    if ((index+buflen) > self->sndSize) {
        shortbuflen = self->sndSize - index;
        sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);
        if (self->loop == 0) { /* with zero padding if noloop */
            int pad = (buflen-shortbuflen)*self->sndChnls;
            for (i=0; i<pad; i++) {
                buffer[i+shortbuflen*self->sndChnls] = 0.;
            }
        }
        else { /* wrap around and read new samples if loop */
            int pad = buflen - shortbuflen;
            float buftemp[pad*self->sndChnls];
            sf_seek(self->sf, (int)self->startPos, SEEK_SET);
            sf_read_float(self->sf, buftemp, pad*self->sndChnls);
            for (i=0; i<(pad*self->sndChnls); i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }    
    }
    else /* without zero padding */
        sf_read_float(self->sf, buffer, buflen*self->sndChnls);
    
    /* de-interleave samples */
    for (i=0; i<(self->sndChnls*buflen); i++) {
        buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
    }
    
    /* fill stream buffer with samples */
    for (i=0; i<self->bufsize; i++) {
        bufpos = self->pointerPos - index;
        bufindex = (int)bufpos;
        frac = bufpos - bufindex;
        x = buffer2[self->chnl][bufindex];
        x1 = buffer2[self->chnl][bufindex+1];
        self->data[i] = x + (x1 - x) * frac;
        self->pointerPos += delta;
    }
    if (self->pointerPos >= self->sndSize) {
        self->pointerPos -= self->sndSize - self->startPos;
        if (self->loop == 0)
            PyObject_CallMethod((PyObject *)self, "stop", NULL);
    }
}    

static void
SfPlayer_readframes_a(SfPlayer *self) {
    float frac, bufpos, delta, x, x1;
    int i, buflen, shortbuflen, bufindex;
    sf_count_t index;
    
    float *spobj = Stream_getData((Stream *)self->speed_stream);

    delta = max_arr(spobj, self->bufsize) * self->srScale;

    buflen = (int)(self->bufsize * delta) + 10;
    float buffer[self->sndChnls*buflen];
    float buffer2[self->sndChnls][buflen];
    
    index = (int)self->pointerPos;
    sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */
    
    /* fill a buffer with enough samples to satisfy speed reading */
    /* if not enough samples to read in the file */
    if ((index+buflen) > self->sndSize) {
        shortbuflen = self->sndSize - index;
        sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);
        if (self->loop == 0) { /* with zero padding if noloop */
            int pad = (buflen-shortbuflen)*self->sndChnls;
            for (i=0; i<pad; i++) {
                buffer[i+shortbuflen*self->sndChnls] = 0.;
            }
        }
        else { /* wrap around and read new samples if loop */
            int pad = buflen - shortbuflen;
            float buftemp[pad*self->sndChnls];
            sf_seek(self->sf, (int)self->startPos, SEEK_SET);
            sf_read_float(self->sf, buftemp, pad*self->sndChnls);
            for (i=0; i<(pad*self->sndChnls); i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }
    }
    else /* without zero padding */
        sf_read_float(self->sf, buffer, buflen*self->sndChnls);
    
    /* de-interleave samples */
    for (i=0; i<(self->sndChnls*buflen); i++) {
        buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
    }
    
    /* fill stream buffer with samples */
    for (i=0; i<self->bufsize; i++) {
        bufpos = self->pointerPos - index;
        bufindex = (int)bufpos;
        frac = bufpos - bufindex;
        x = buffer2[self->chnl][bufindex];
        x1 = buffer2[self->chnl][bufindex+1];
        self->data[i] = x + (x1 - x) * frac;
        self->pointerPos += spobj[i] * self->srScale;
    }
    if (self->pointerPos >= self->sndSize) {
        self->pointerPos -= self->sndSize - self->startPos;
        if (self->loop == 0)
            PyObject_CallMethod((PyObject *)self, "stop", NULL);
    }    
}

static void SfPlayer_postprocessing_ii(SfPlayer *self) { POST_PROCESSING_II };
static void SfPlayer_postprocessing_ai(SfPlayer *self) { POST_PROCESSING_AI };
static void SfPlayer_postprocessing_ia(SfPlayer *self) { POST_PROCESSING_IA };
static void SfPlayer_postprocessing_aa(SfPlayer *self) { POST_PROCESSING_AA };

static void
SfPlayer_setProcMode(SfPlayer *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = SfPlayer_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = SfPlayer_readframes_a;
            break;
    } 
    
	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SfPlayer_postprocessing_ii;
            break;
        case 1:   
            self->muladd_func_ptr = SfPlayer_postprocessing_ai;
            break;
        case 10:        
            self->muladd_func_ptr = SfPlayer_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SfPlayer_postprocessing_aa;
            break;
    }    
}

static void
SfPlayer_compute_next_data_frame(SfPlayer *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SfPlayer_traverse(SfPlayer *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->speed);    
    Py_VISIT(self->speed_stream);    
    return 0;
}

static int 
SfPlayer_clear(SfPlayer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->speed);    
    Py_CLEAR(self->speed_stream);    
    return 0;
}

static void
SfPlayer_dealloc(SfPlayer* self)
{
    sf_close(self->sf);
    free(self->data);
    SfPlayer_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SfPlayer_deleteStream(SfPlayer *self) { DELETE_STREAM };

static PyObject *
SfPlayer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SfPlayer *self;
    self = (SfPlayer *)type->tp_alloc(type, 0);
    
    self->speed = PyFloat_FromDouble(1);
    self->loop = 0;
    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfPlayer_compute_next_data_frame);
    self->mode_func_ptr = SfPlayer_setProcMode;
    
    return (PyObject *)self;
}

static int
SfPlayer_init(SfPlayer *self, PyObject *args, PyObject *kwds)
{
    int i;
    float offset = 0.;
    PyObject *speedtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"path", "speed", "loop", "offset", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|OifiOO", kwlist, &self->path, &speedtmp, &self->loop, &offset, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
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
        
    /* Open the sound file. */
    self->info.format = 0;
    self->sf = sf_open(self->path, SFM_READ, &self->info);
    if (self->sf == NULL)
    {
        printf("Failed to open the file.\n");
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;

    self->startPos = offset * self->sr * self->srScale;
    self->pointerPos = self->startPos;
    
    SfPlayer_compute_next_data_frame((SfPlayer *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * SfPlayer_getServer(SfPlayer* self) { GET_SERVER };
static PyObject * SfPlayer_getStream(SfPlayer* self) { GET_STREAM };
static PyObject * SfPlayer_setMul(SfPlayer *self, PyObject *arg) { SET_MUL };	
static PyObject * SfPlayer_setAdd(SfPlayer *self, PyObject *arg) { SET_ADD };	

static PyObject * SfPlayer_play(SfPlayer *self) { PLAY };
static PyObject * SfPlayer_out(SfPlayer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfPlayer_stop(SfPlayer *self) { STOP };

static PyObject * SfPlayer_multiply(SfPlayer *self, PyObject *arg) { MULTIPLY };
static PyObject * SfPlayer_inplace_multiply(SfPlayer *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SfPlayer_add(SfPlayer *self, PyObject *arg) { ADD };
static PyObject * SfPlayer_inplace_add(SfPlayer *self, PyObject *arg) { INPLACE_ADD };

static PyObject *
SfPlayer_setSpeed(SfPlayer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->speed);
	if (isNumber == 1) {
		self->speed = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->speed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->speed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->speed_stream);
        self->speed_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
SfPlayer_setLoop(SfPlayer *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

    self->loop = PyInt_AsLong(arg);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef SfPlayer_members[] = {
{"server", T_OBJECT_EX, offsetof(SfPlayer, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfPlayer, stream), 0, "Stream object."},
{"speed", T_OBJECT_EX, offsetof(SfPlayer, speed), 0, "Frequency in cycle per second."},
{"mul", T_OBJECT_EX, offsetof(SfPlayer, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SfPlayer, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfPlayer_methods[] = {
{"getServer", (PyCFunction)SfPlayer_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfPlayer_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SfPlayer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SfPlayer_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfPlayer_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfPlayer_stop, METH_NOARGS, "Stops computing."},
{"setSpeed", (PyCFunction)SfPlayer_setSpeed, METH_O, "Sets sfplayer reading speed."},
{"setLoop", (PyCFunction)SfPlayer_setLoop, METH_O, "Sets sfplayer loop mode (0 = no loop, 1 = loop)."},
{"setMul", (PyCFunction)SfPlayer_setMul, METH_O, "Sets SfPlayer mul factor."},
{"setAdd", (PyCFunction)SfPlayer_setAdd, METH_O, "Sets SfPlayer add factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SfPlayer_as_number = {
(binaryfunc)SfPlayer_add,                      /*nb_add*/
0,                 /*nb_subtract*/
(binaryfunc)SfPlayer_multiply,                 /*nb_multiply*/
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
(binaryfunc)SfPlayer_inplace_add,              /*inplace_add*/
0,         /*inplace_subtract*/
(binaryfunc)SfPlayer_inplace_multiply,         /*inplace_multiply*/
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

PyTypeObject SfPlayerType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SfPlayer_base",         /*tp_name*/
sizeof(SfPlayer),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfPlayer_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&SfPlayer_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SfPlayer objects. Reads a soundfile directly from disk.",           /* tp_doc */
(traverseproc)SfPlayer_traverse,   /* tp_traverse */
(inquiry)SfPlayer_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SfPlayer_methods,             /* tp_methods */
SfPlayer_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SfPlayer_init,      /* tp_init */
0,                         /* tp_alloc */
SfPlayer_new,                 /* tp_new */
};

