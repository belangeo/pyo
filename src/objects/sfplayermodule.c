#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"
#include "interpolation.h"

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
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    float srScale;
    float startPos;
    float pointerPos;
    float *samplesBuffer;
    float (*interp_func_ptr)(float *, int, float);
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

float min_arr(float *a,int n)
{
    int i;
    float m;
    m=*a;
    for (i=1; i<n; i++) {
        if (m > *(a+i)) 
            m = *(a+i);
    }    
    return m;
}

float *reverseArray(float *orig, int b)
{
    int a=0;
    float swap;

    for (a; a<--b; a++) { //increment a and decrement b until they meet eachother
        swap = orig[a];       //put what's in a into swap space
        orig[a] = orig[b];    //put what's in b into a
        orig[b] = swap;       //put what's in the swap (a) into b
    }

    return orig;    //return the new (reversed) array (a pointer to it)
}

static void
SfPlayer_readframes_i(SfPlayer *self) {
    float sp, frac, bufpos, delta;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;

    sp = PyFloat_AS_DOUBLE(self->speed);

    delta = fabsf(sp) * self->srScale;
    
    buflen = (int)(self->bufsize * delta) + 10;
    totlen = self->sndChnls*buflen;
    float buffer[totlen];
    float buffer2[self->sndChnls][buflen];

    if (sp > 0) {
        if (self->startPos >= self->sndSize)
            self->startPos = 0;
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
                int padlen = pad*self->sndChnls;
                float buftemp[padlen];
                sf_seek(self->sf, (int)self->startPos, SEEK_SET);
                sf_read_float(self->sf, buftemp, padlen);
                for (i=0; i<(padlen); i++) {
                    buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
                }
            }    
        }
        else /* without zero padding */
            sf_read_float(self->sf, buffer, totlen);
    
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
    
        /* fill samplesBuffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = self->pointerPos - index;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }    
            self->pointerPos += delta;
        }
        if (self->pointerPos >= self->sndSize) {
            self->pointerPos -= self->sndSize - self->startPos;
            if (self->loop == 0)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        }
    }
    else {
        if (self->startPos == 0.)
            self->startPos = self->sndSize;
        index = (int)self->pointerPos + 1;
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index-buflen) < 0) {
            shortbuflen = index;
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;

            if (self->loop == 0) { /* with zero padding if noloop */
                for (i=0; i<padlen; i++) {
                    buffer[i] = 0.;
                }
            }
            else { /* wrap around and read new samples if loop */
                float buftemp[padlen];
                sf_seek(self->sf, (int)self->startPos-pad, SEEK_SET);
                sf_read_float(self->sf, buftemp, padlen);
                for (i=0; i<padlen; i++) {
                    buffer[i] = buftemp[i];
                }
            }
            
            float buftemp2[shortbuflen*self->sndChnls];
            sf_seek(self->sf, 0, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buftemp2, shortbuflen*self->sndChnls);
            for (i=0; i<(shortbuflen*self->sndChnls); i++) {
                buffer[i+padlen] = buftemp2[i];
            }    
        }
        else /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buffer, totlen);
        
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
        
        /* reverse arrays */
        float swap;
        for (i=0; i<self->sndChnls; i++) {
            int a = 0;
            int b = buflen; 
            for (a; a<--b; a++) { //increment a and decrement b until they meet eachother
                swap = buffer2[i][a];       //put what's in a into swap space
                buffer2[i][a] = buffer2[i][b];    //put what's in b into a
                buffer2[i][b] = swap;       //put what's in the swap (a) into b
            }
        }
        
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = index - self->pointerPos;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos -= delta;
        }
        if (self->pointerPos <= 0) {
            self->pointerPos += self->startPos;
            if (self->loop == 0)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        }
    }
}    

static void
SfPlayer_readframes_a(SfPlayer *self) {
    float frac, bufpos, delta;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;
    
    float *spobj = Stream_getData((Stream *)self->speed_stream);

    float mini = min_arr(spobj, self->bufsize);
    float maxi = max_arr(spobj, self->bufsize);
    if (fabsf(mini) > fabsf(maxi))
        maxi = mini;
    delta = fabsf(maxi) * self->srScale;

    buflen = (int)(self->bufsize * delta) + 10;
    totlen = self->sndChnls*buflen;
    float buffer[totlen];
    float buffer2[self->sndChnls][buflen];

    if (maxi > 0) {
        if (self->startPos >= self->sndSize)
            self->startPos = 0;
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
                int padlen = pad*self->sndChnls;
                float buftemp[padlen];
                sf_seek(self->sf, (int)self->startPos, SEEK_SET);
                sf_read_float(self->sf, buftemp, padlen);
                for (i=0; i<padlen; i++) {
                    buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
                }
            }
        }
        else /* without zero padding */
            sf_read_float(self->sf, buffer, totlen);
    
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
    
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = self->pointerPos - index;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos += spobj[i] * self->srScale;
        }
        if (self->pointerPos >= self->sndSize) {
            self->pointerPos -= self->sndSize - self->startPos;
            if (self->loop == 0)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        } 
    } 
    else {
        if (self->startPos == 0.)
            self->startPos = self->sndSize;
        index = (int)self->pointerPos + 1;
        //sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index-buflen) < 0) {
            shortbuflen = index;
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            //sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);
            if (self->loop == 0) { /* with zero padding if noloop */
                for (i=0; i<padlen; i++) {
                    buffer[i] = 0.;
                }
            }
            else { /* wrap around and read new samples if loop */
                float buftemp[padlen];
                sf_seek(self->sf, (int)self->startPos-pad, SEEK_SET);
                sf_read_float(self->sf, buftemp, padlen);
                for (i=0; i<padlen; i++) {
                    buffer[i] = buftemp[i];
                }
            }
        }
        else { /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buffer, totlen);
        }
        
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }

        /* reverse arrays */
        float swap;
        for (i=0; i<self->sndChnls; i++) {
            int a = 0;
            int b = buflen; 
            for (a; a<--b; a++) { //increment a and decrement b until they meet eachother
                swap = buffer2[i][a];       //put what's in a into swap space
                buffer2[i][a] = buffer2[i][b];    //put what's in b into a
                buffer2[i][b] = swap;       //put what's in the swap (a) into b
            }
        }
        
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = index - self->pointerPos;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos += spobj[i] * self->srScale;
        }
        if (self->pointerPos <= 0) {
            self->pointerPos += self->startPos;
            if (self->loop == 0)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        } 
    }    
}

static void
SfPlayer_setProcMode(SfPlayer *self)
{
    int procmode;
    procmode = self->modebuffer[2];

	switch (procmode) {
        case 0:        
            self->proc_func_ptr = SfPlayer_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = SfPlayer_readframes_a;
            break;
    }     
}

static void
SfPlayer_compute_next_data_frame(SfPlayer *self)
{
    (*self->proc_func_ptr)(self); 
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
    free(self->samplesBuffer);
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
    self->interp = 2;
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
    float offset = 0.;
    PyObject *speedtmp=NULL;
    
    static char *kwlist[] = {"path", "speed", "loop", "offset", "interp", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|Oifi", kwlist, &self->path, &speedtmp, &self->loop, &offset, &self->interp))
        return -1; 
    
    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);

    if (self->interp == 0)
        self->interp = 2;
    if (self->interp == 1)
        self->interp_func_ptr = nointerp;
    else if (self->interp == 2)
        self->interp_func_ptr = linear;
    else if (self->interp == 3)
        self->interp_func_ptr = cosine;
    else if (self->interp == 4)
        self->interp_func_ptr = cubic;
    
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

    self->samplesBuffer = (float *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(float));

    self->startPos = offset * self->sr * self->srScale;
    self->pointerPos = self->startPos;
    
    SfPlayer_compute_next_data_frame((SfPlayer *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * SfPlayer_getServer(SfPlayer* self) { GET_SERVER };
static PyObject * SfPlayer_getStream(SfPlayer* self) { GET_STREAM };

static PyObject * SfPlayer_play(SfPlayer *self) { PLAY };
static PyObject * SfPlayer_out(SfPlayer *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfPlayer_stop(SfPlayer *self) { STOP };

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
SfPlayer_setSound(SfPlayer *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->path = PyString_AsString(arg);

    sf_close(self->sf);

    /* Open the sound file. */
    self->info.format = 0;
    self->sf = sf_open(self->path, SFM_READ, &self->info);
    if (self->sf == NULL)
    {
        printf("Failed to open the file.\n");
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    //self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;
    
    //self->samplesBuffer = (float *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(float));
    
    self->startPos = 0.0;
    self->pointerPos = self->startPos;
    
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

static PyObject *
SfPlayer_setOffset(SfPlayer *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->startPos = PyFloat_AsDouble(PyNumber_Float(arg)) * self->sr * self->srScale;
    }  

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SfPlayer_setInterp(SfPlayer *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  

    if (self->interp == 0)
        self->interp = 2;
    if (self->interp == 1)
        self->interp_func_ptr = nointerp;
    else if (self->interp == 2)
        self->interp_func_ptr = linear;
    else if (self->interp == 3)
        self->interp_func_ptr = cosine;
    else if (self->interp == 4)
        self->interp_func_ptr = cubic;

    Py_INCREF(Py_None);
    return Py_None;
}

float *
SfPlayer_getSamplesBuffer(SfPlayer *self)
{
    return (float *)self->samplesBuffer;
}    

static PyMemberDef SfPlayer_members[] = {
{"server", T_OBJECT_EX, offsetof(SfPlayer, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfPlayer, stream), 0, "Stream object."},
{"speed", T_OBJECT_EX, offsetof(SfPlayer, speed), 0, "Frequency in cycle per second."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfPlayer_methods[] = {
{"getServer", (PyCFunction)SfPlayer_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfPlayer_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SfPlayer_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SfPlayer_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfPlayer_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfPlayer_stop, METH_NOARGS, "Stops computing."},
{"setSound", (PyCFunction)SfPlayer_setSound, METH_O, "Sets sfplayer sound path."},
{"setSpeed", (PyCFunction)SfPlayer_setSpeed, METH_O, "Sets sfplayer reading speed."},
{"setLoop", (PyCFunction)SfPlayer_setLoop, METH_O, "Sets sfplayer loop mode (0 = no loop, 1 = loop)."},
{"setOffset", (PyCFunction)SfPlayer_setOffset, METH_O, "Sets sfplayer start position."},
{"setInterp", (PyCFunction)SfPlayer_setInterp, METH_O, "Sets sfplayer interpolation mode."},
{NULL}  /* Sentinel */
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
0,             /*tp_as_number*/
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

/************************************************************************************************/
/* Sfplay streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    SfPlayer *mainPlayer;
    int modebuffer[2];
    int chnl; 
} SfPlay;

static void SfPlay_postprocessing_ii(SfPlay *self) { POST_PROCESSING_II };
static void SfPlay_postprocessing_ai(SfPlay *self) { POST_PROCESSING_AI };
static void SfPlay_postprocessing_ia(SfPlay *self) { POST_PROCESSING_IA };
static void SfPlay_postprocessing_aa(SfPlay *self) { POST_PROCESSING_AA };
static void SfPlay_postprocessing_ireva(SfPlay *self) { POST_PROCESSING_IREVA };
static void SfPlay_postprocessing_areva(SfPlay *self) { POST_PROCESSING_AREVA };
static void SfPlay_postprocessing_revai(SfPlay *self) { POST_PROCESSING_REVAI };
static void SfPlay_postprocessing_revaa(SfPlay *self) { POST_PROCESSING_REVAA };
static void SfPlay_postprocessing_revareva(SfPlay *self) { POST_PROCESSING_REVAREVA };

static void
SfPlay_setProcMode(SfPlay *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SfPlay_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SfPlay_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SfPlay_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SfPlay_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SfPlay_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SfPlay_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SfPlay_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SfPlay_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SfPlay_postprocessing_revareva;
            break;
    }
}

static void
SfPlay_compute_next_data_frame(SfPlay *self)
{
    int i;
    float *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SfPlayer_getSamplesBuffer((SfPlayer *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SfPlay_traverse(SfPlay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int 
SfPlay_clear(SfPlay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);    
    return 0;
}

static void
SfPlay_dealloc(SfPlay* self)
{
    free(self->data);
    SfPlay_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SfPlay_deleteStream(SfPlay *self) { DELETE_STREAM };

static PyObject *
SfPlay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SfPlay *self;
    self = (SfPlay *)type->tp_alloc(type, 0);
    
    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfPlay_compute_next_data_frame);
    self->mode_func_ptr = SfPlay_setProcMode;
    
    return (PyObject *)self;
}

static int
SfPlay_init(SfPlay *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainPlayer", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (SfPlayer *)maintmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    SfPlay_compute_next_data_frame((SfPlay *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * SfPlay_getServer(SfPlay* self) { GET_SERVER };
static PyObject * SfPlay_getStream(SfPlay* self) { GET_STREAM };
static PyObject * SfPlay_setMul(SfPlay *self, PyObject *arg) { SET_MUL };	
static PyObject * SfPlay_setAdd(SfPlay *self, PyObject *arg) { SET_ADD };	
static PyObject * SfPlay_setSub(SfPlay *self, PyObject *arg) { SET_SUB };	
static PyObject * SfPlay_setDiv(SfPlay *self, PyObject *arg) { SET_DIV };	

static PyObject * SfPlay_play(SfPlay *self) { PLAY };
static PyObject * SfPlay_out(SfPlay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfPlay_stop(SfPlay *self) { STOP };

static PyObject * SfPlay_multiply(SfPlay *self, PyObject *arg) { MULTIPLY };
static PyObject * SfPlay_inplace_multiply(SfPlay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SfPlay_add(SfPlay *self, PyObject *arg) { ADD };
static PyObject * SfPlay_inplace_add(SfPlay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SfPlay_sub(SfPlay *self, PyObject *arg) { SUB };
static PyObject * SfPlay_inplace_sub(SfPlay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SfPlay_div(SfPlay *self, PyObject *arg) { DIV };
static PyObject * SfPlay_inplace_div(SfPlay *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SfPlay_members[] = {
{"server", T_OBJECT_EX, offsetof(SfPlay, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfPlay, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(SfPlay, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SfPlay, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfPlay_methods[] = {
{"getServer", (PyCFunction)SfPlay_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfPlay_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SfPlay_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SfPlay_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfPlay_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfPlay_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)SfPlay_setMul, METH_O, "Sets SfPlay mul factor."},
{"setAdd", (PyCFunction)SfPlay_setAdd, METH_O, "Sets SfPlay add factor."},
{"setSub", (PyCFunction)SfPlay_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SfPlay_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SfPlay_as_number = {
(binaryfunc)SfPlay_add,                      /*nb_add*/
(binaryfunc)SfPlay_sub,                 /*nb_subtract*/
(binaryfunc)SfPlay_multiply,                 /*nb_multiply*/
(binaryfunc)SfPlay_div,                   /*nb_divide*/
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
(binaryfunc)SfPlay_inplace_add,              /*inplace_add*/
(binaryfunc)SfPlay_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SfPlay_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)SfPlay_inplace_div,           /*inplace_divide*/
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

PyTypeObject SfPlayType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SfPlay_base",         /*tp_name*/
sizeof(SfPlay),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfPlay_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&SfPlay_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SfPlay objects. Reads a channel from a soundfile directly from disk.",           /* tp_doc */
(traverseproc)SfPlay_traverse,   /* tp_traverse */
(inquiry)SfPlay_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SfPlay_methods,             /* tp_methods */
SfPlay_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SfPlay_init,      /* tp_init */
0,                         /* tp_alloc */
SfPlay_new,                 /* tp_new */
};



/************************************************************************************************/
/* SfMarkerShuffler object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *speed;
    Stream *speed_stream;
    int modebuffer[3];
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    float srScale;
    float startPos;
    float endPos;
    float nextStartPos;
    float nextEndPos;
    float pointerPos;
    float *samplesBuffer;
    float *markers;
    int markers_size;
    float (*interp_func_ptr)(float *, int, float);
} SfMarkerShuffler;

/*** PROTOTYPES ***/
static void SfMarkerShuffler_chooseNewMark(SfMarkerShuffler *self, int dir);
/******************/

static void
SfMarkerShuffler_readframes_i(SfMarkerShuffler *self) {
    float sp, frac, bufpos, delta;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;
    
    sp = PyFloat_AS_DOUBLE(self->speed);
    
    delta = fabsf(sp) * self->srScale;
    
    buflen = (int)(self->bufsize * delta) + 10;
    totlen = self->sndChnls*buflen;
    float buffer[totlen];
    float buffer2[self->sndChnls][buflen];

    //printf("startPos: %f\nendPos: %f\npointerPos: %f\n", self->startPos, self->endPos, self->pointerPos);
    if (sp > 0) {
        if (self->startPos == -1) {
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos;
        }    
        index = (int)self->pointerPos;
        sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index+buflen) > self->endPos) {
            shortbuflen = self->endPos - index;
            sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);

            /* wrap around and read new samples if loop */
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            float buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos, SEEK_SET);
            sf_read_float(self->sf, buftemp, padlen);
            for (i=0; i<(padlen); i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }
        else /* without zero padding */
            sf_read_float(self->sf, buffer, totlen);
        
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
        
        /* fill data with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = self->pointerPos - index;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }    
            self->pointerPos += delta;
        }
        if (self->pointerPos >= self->endPos) {
            float off = self->pointerPos - self->endPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos + off;
        }
    }
    else {
        if (self->startPos == -1) {
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 0);
            self->pointerPos = self->startPos;
        }    
        index = (int)self->pointerPos + 1;
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index-buflen) < self->endPos) {
            shortbuflen = index - self->endPos;
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;

            /* wrap around and read new samples if loop */
            float buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos-pad, SEEK_SET);
            sf_read_float(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i] = buftemp[i];
            }
            
            float buftemp2[shortbuflen*self->sndChnls];
            sf_seek(self->sf, self->endPos, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buftemp2, shortbuflen*self->sndChnls);
            for (i=0; i<(shortbuflen*self->sndChnls); i++) {
                buffer[i+padlen] = buftemp2[i];
            }    
        }
        else { /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buffer, totlen);
        }
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
        
        /* reverse arrays */
        float swap;
        for (i=0; i<self->sndChnls; i++) {
            int a = 0;
            int b = buflen; 
            for (a; a<--b; a++) { //increment a and decrement b until they meet eachother
                swap = buffer2[i][a];       //put what's in a into swap space
                buffer2[i][a] = buffer2[i][b];    //put what's in b into a
                buffer2[i][b] = swap;       //put what's in the swap (a) into b
            }
        }
        
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = index - self->pointerPos;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos -= delta;
        }
        if (self->pointerPos <= self->endPos) {
            float off = self->endPos - self->pointerPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 0);
            self->pointerPos = self->startPos - off;
        }
    }
}    

static void
SfMarkerShuffler_readframes_a(SfMarkerShuffler *self) {
    float frac, bufpos, delta;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;
    
    float *spobj = Stream_getData((Stream *)self->speed_stream);
    
    float mini = min_arr(spobj, self->bufsize);
    float maxi = max_arr(spobj, self->bufsize);
    if (fabsf(mini) > fabsf(maxi))
        maxi = mini;
    delta = fabsf(maxi) * self->srScale;
    
    buflen = (int)(self->bufsize * delta) + 10;
    totlen = self->sndChnls*buflen;
    float buffer[totlen];
    float buffer2[self->sndChnls][buflen];
    
    if (maxi > 0) {
        if (self->startPos == -1) {
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos;
        }  
        index = (int)self->pointerPos;
        sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index+buflen) > self->endPos) {
            shortbuflen = self->endPos - index;
            sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);
            
            /* wrap around and read new samples if loop */
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            float buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos, SEEK_SET);
            sf_read_float(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }
        else /* without zero padding */
            sf_read_float(self->sf, buffer, totlen);
        
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
        
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = self->pointerPos - index;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos += spobj[i] * self->srScale;
        }
        if (self->pointerPos >= self->endPos) {
            float off = self->pointerPos - self->endPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos + off;
        } 
    } 
    else {
        if (self->startPos == -1) {
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 0);
            self->pointerPos = self->startPos;
        }
        index = (int)self->pointerPos + 1;
        
        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index-buflen) < self->endPos) {
            shortbuflen = index - self->endPos;
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            //sf_read_float(self->sf, buffer, shortbuflen*self->sndChnls);

            /* wrap around and read new samples if loop */
            float buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos-pad, SEEK_SET);
            sf_read_float(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i] = buftemp[i];
            }
        }
        else { /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            sf_read_float(self->sf, buffer, totlen);
        }
        
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }
        
        /* reverse arrays */
        float swap;
        for (i=0; i<self->sndChnls; i++) {
            int a = 0;
            int b = buflen; 
            for (a; a<--b; a++) { //increment a and decrement b until they meet eachother
                swap = buffer2[i][a];       //put what's in a into swap space
                buffer2[i][a] = buffer2[i][b];    //put what's in b into a
                buffer2[i][b] = swap;       //put what's in the swap (a) into b
            }
        }
        
        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            bufpos = index - self->pointerPos;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac);
            }
            self->pointerPos += spobj[i] * self->srScale;
        }
        if (self->pointerPos <= self->endPos) {
            float off = self->endPos - self->pointerPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 0);
            self->pointerPos = self->startPos - off;
        }
    }    
}

static void
SfMarkerShuffler_setProcMode(SfMarkerShuffler *self)
{
    int procmode;
    procmode = self->modebuffer[2];
    
	switch (procmode) {
        case 0:        
            self->proc_func_ptr = SfMarkerShuffler_readframes_i;
            break;
        case 1:    
            self->proc_func_ptr = SfMarkerShuffler_readframes_a;
            break;
    }     
}

static void
SfMarkerShuffler_compute_next_data_frame(SfMarkerShuffler *self)
{
    (*self->proc_func_ptr)(self); 
}

static void
SfMarkerShuffler_chooseNewMark(SfMarkerShuffler *self, int dir) 
{
    int mark;
    if (dir == 1) {
        if (self->startPos == -1) {
            mark = (int)(self->markers_size * (rand()/((float)(RAND_MAX)+1)));
            self->startPos = self->markers[mark] * self->srScale;
            self->endPos = self->markers[mark+1] * self->srScale;
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }
        
        mark = (int)(self->markers_size * (rand()/((float)(RAND_MAX)+1)));
        self->nextStartPos = self->markers[mark] * self->srScale;
        self->nextEndPos = self->markers[mark+1] * self->srScale;
    }
    else {
        if (self->startPos == -1) {
            mark = self->markers_size - (int)(self->markers_size * (rand()/((float)(RAND_MAX)+1)));
            self->startPos = self->markers[mark] * self->srScale;
            self->endPos = self->markers[mark-1] * self->srScale;
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }
        
        mark = self->markers_size - (int)(self->markers_size * (rand()/((float)(RAND_MAX)+1)));
        self->nextStartPos = self->markers[mark] * self->srScale;
        self->nextEndPos = self->markers[mark-1] * self->srScale;
    }
}

static void
SfMarkerShuffler_setMarkers(SfMarkerShuffler *self, PyObject *markerstmp)
{
    Py_ssize_t i;
    Py_ssize_t len = PyList_Size(markerstmp);
    self->markers = (float *)realloc(self->markers, (len + 2) * sizeof(float));
    self->markers[0] = 0.;
    for (i=0; i<len; i++) {
        self->markers[i+1] = PyFloat_AsDouble(PyList_GetItem(markerstmp, i));
    }  
    self->markers[len+1] = self->sndSize;
    self->markers_size = (int)len+1;
}

static int
SfMarkerShuffler_traverse(SfMarkerShuffler *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->speed);    
    Py_VISIT(self->speed_stream);    
    return 0;
}

static int 
SfMarkerShuffler_clear(SfMarkerShuffler *self)
{
    pyo_CLEAR
    Py_CLEAR(self->speed);    
    Py_CLEAR(self->speed_stream);    
    return 0;
}

static void
SfMarkerShuffler_dealloc(SfMarkerShuffler* self)
{
    sf_close(self->sf);
    free(self->samplesBuffer);
    free(self->markers);
    free(self->data);
    SfMarkerShuffler_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SfMarkerShuffler_deleteStream(SfMarkerShuffler *self) { DELETE_STREAM };

static PyObject *
SfMarkerShuffler_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SfMarkerShuffler *self;
    self = (SfMarkerShuffler *)type->tp_alloc(type, 0);
    
    self->speed = PyFloat_FromDouble(1);
    self->interp = 2;
    self->startPos = -1;
    self->endPos = -1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerShuffler_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerShuffler_setProcMode;
    
    return (PyObject *)self;
}

static int
SfMarkerShuffler_init(SfMarkerShuffler *self, PyObject *args, PyObject *kwds)
{
    PyObject *speedtmp=NULL, *markerstmp=NULL;
    
    static char *kwlist[] = {"path", "markers", "speed", "interp", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "sO|Oi", kwlist, &self->path, &markerstmp, &speedtmp, &self->interp))
        return -1; 

    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    if (self->interp == 0)
        self->interp = 2;
    if (self->interp == 1)
        self->interp_func_ptr = nointerp;
    else if (self->interp == 2)
        self->interp_func_ptr = linear;
    else if (self->interp == 3)
        self->interp_func_ptr = cosine;
    else if (self->interp == 4)
        self->interp_func_ptr = cubic;
    
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

    Py_INCREF(markerstmp);
    SfMarkerShuffler_setMarkers((SfMarkerShuffler *)self, markerstmp);

    self->samplesBuffer = (float *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(float));

    srand((unsigned)(time(0)));

    SfMarkerShuffler_compute_next_data_frame((SfMarkerShuffler *)self);
    Py_INCREF(self);
    return 0;
}

static PyObject * SfMarkerShuffler_getServer(SfMarkerShuffler* self) { GET_SERVER };
static PyObject * SfMarkerShuffler_getStream(SfMarkerShuffler* self) { GET_STREAM };

static PyObject * SfMarkerShuffler_play(SfMarkerShuffler *self) { PLAY };
static PyObject * SfMarkerShuffler_out(SfMarkerShuffler *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerShuffler_stop(SfMarkerShuffler *self) { STOP };

static PyObject *
SfMarkerShuffler_setSpeed(SfMarkerShuffler *self, PyObject *arg)
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
SfMarkerShuffler_setInterp(SfMarkerShuffler *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  
    
    if (self->interp == 0)
        self->interp = 2;
    if (self->interp == 1)
        self->interp_func_ptr = nointerp;
    else if (self->interp == 2)
        self->interp_func_ptr = linear;
    else if (self->interp == 3)
        self->interp_func_ptr = cosine;
    else if (self->interp == 4)
        self->interp_func_ptr = cubic;
    
    Py_INCREF(Py_None);
    return Py_None;
}

float *
SfMarkerShuffler_getSamplesBuffer(SfMarkerShuffler *self)
{
    return (float *)self->samplesBuffer;
}    

static PyMemberDef SfMarkerShuffler_members[] = {
{"server", T_OBJECT_EX, offsetof(SfMarkerShuffler, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfMarkerShuffler, stream), 0, "Stream object."},
{"speed", T_OBJECT_EX, offsetof(SfMarkerShuffler, speed), 0, "Frequency in cycle per second."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfMarkerShuffler_methods[] = {
{"getServer", (PyCFunction)SfMarkerShuffler_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfMarkerShuffler_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SfMarkerShuffler_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SfMarkerShuffler_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfMarkerShuffler_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfMarkerShuffler_stop, METH_NOARGS, "Stops computing."},
{"setSpeed", (PyCFunction)SfMarkerShuffler_setSpeed, METH_O, "Sets sfplayer reading speed."},
{"setInterp", (PyCFunction)SfMarkerShuffler_setInterp, METH_O, "Sets sfplayer interpolation mode."},
{NULL}  /* Sentinel */
};

PyTypeObject SfMarkerShufflerType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SfMarkerShuffler_base",         /*tp_name*/
sizeof(SfMarkerShuffler),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfMarkerShuffler_dealloc, /*tp_dealloc*/
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
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SfMarkerShuffler objects. Shuffle an AIFF soundfile from markers points.",           /* tp_doc */
(traverseproc)SfMarkerShuffler_traverse,   /* tp_traverse */
(inquiry)SfMarkerShuffler_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SfMarkerShuffler_methods,             /* tp_methods */
SfMarkerShuffler_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SfMarkerShuffler_init,      /* tp_init */
0,                         /* tp_alloc */
SfMarkerShuffler_new,                 /* tp_new */
};

/************************************************************************************************/
/* SfMarkerShuffle streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    SfMarkerShuffler *mainPlayer;
    int modebuffer[2];
    int chnl; 
} SfMarkerShuffle;

static void SfMarkerShuffle_postprocessing_ii(SfMarkerShuffle *self) { POST_PROCESSING_II };
static void SfMarkerShuffle_postprocessing_ai(SfMarkerShuffle *self) { POST_PROCESSING_AI };
static void SfMarkerShuffle_postprocessing_ia(SfMarkerShuffle *self) { POST_PROCESSING_IA };
static void SfMarkerShuffle_postprocessing_aa(SfMarkerShuffle *self) { POST_PROCESSING_AA };
static void SfMarkerShuffle_postprocessing_ireva(SfMarkerShuffle *self) { POST_PROCESSING_IREVA };
static void SfMarkerShuffle_postprocessing_areva(SfMarkerShuffle *self) { POST_PROCESSING_AREVA };
static void SfMarkerShuffle_postprocessing_revai(SfMarkerShuffle *self) { POST_PROCESSING_REVAI };
static void SfMarkerShuffle_postprocessing_revaa(SfMarkerShuffle *self) { POST_PROCESSING_REVAA };
static void SfMarkerShuffle_postprocessing_revareva(SfMarkerShuffle *self) { POST_PROCESSING_REVAREVA };

static void
SfMarkerShuffle_setProcMode(SfMarkerShuffle *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SfMarkerShuffle_postprocessing_revareva;
            break;
    }
}

static void
SfMarkerShuffle_compute_next_data_frame(SfMarkerShuffle *self)
{
    int i;
    float *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SfMarkerShuffler_getSamplesBuffer((SfMarkerShuffler *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SfMarkerShuffle_traverse(SfMarkerShuffle *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int 
SfMarkerShuffle_clear(SfMarkerShuffle *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);    
    return 0;
}

static void
SfMarkerShuffle_dealloc(SfMarkerShuffle* self)
{
    free(self->data);
    SfMarkerShuffle_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SfMarkerShuffle_deleteStream(SfMarkerShuffle *self) { DELETE_STREAM };

static PyObject *
SfMarkerShuffle_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SfMarkerShuffle *self;
    self = (SfMarkerShuffle *)type->tp_alloc(type, 0);
    
    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerShuffle_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerShuffle_setProcMode;
    
    return (PyObject *)self;
}

static int
SfMarkerShuffle_init(SfMarkerShuffle *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainPlayer", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (SfMarkerShuffler *)maintmp;
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    SfMarkerShuffle_compute_next_data_frame((SfMarkerShuffle *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * SfMarkerShuffle_getServer(SfMarkerShuffle* self) { GET_SERVER };
static PyObject * SfMarkerShuffle_getStream(SfMarkerShuffle* self) { GET_STREAM };
static PyObject * SfMarkerShuffle_setMul(SfMarkerShuffle *self, PyObject *arg) { SET_MUL };	
static PyObject * SfMarkerShuffle_setAdd(SfMarkerShuffle *self, PyObject *arg) { SET_ADD };	
static PyObject * SfMarkerShuffle_setSub(SfMarkerShuffle *self, PyObject *arg) { SET_SUB };	
static PyObject * SfMarkerShuffle_setDiv(SfMarkerShuffle *self, PyObject *arg) { SET_DIV };	

static PyObject * SfMarkerShuffle_play(SfMarkerShuffle *self) { PLAY };
static PyObject * SfMarkerShuffle_out(SfMarkerShuffle *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerShuffle_stop(SfMarkerShuffle *self) { STOP };

static PyObject * SfMarkerShuffle_multiply(SfMarkerShuffle *self, PyObject *arg) { MULTIPLY };
static PyObject * SfMarkerShuffle_inplace_multiply(SfMarkerShuffle *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SfMarkerShuffle_add(SfMarkerShuffle *self, PyObject *arg) { ADD };
static PyObject * SfMarkerShuffle_inplace_add(SfMarkerShuffle *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SfMarkerShuffle_sub(SfMarkerShuffle *self, PyObject *arg) { SUB };
static PyObject * SfMarkerShuffle_inplace_sub(SfMarkerShuffle *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SfMarkerShuffle_div(SfMarkerShuffle *self, PyObject *arg) { DIV };
static PyObject * SfMarkerShuffle_inplace_div(SfMarkerShuffle *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SfMarkerShuffle_members[] = {
{"server", T_OBJECT_EX, offsetof(SfMarkerShuffle, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfMarkerShuffle, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(SfMarkerShuffle, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(SfMarkerShuffle, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfMarkerShuffle_methods[] = {
{"getServer", (PyCFunction)SfMarkerShuffle_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfMarkerShuffle_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)SfMarkerShuffle_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)SfMarkerShuffle_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfMarkerShuffle_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfMarkerShuffle_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)SfMarkerShuffle_setMul, METH_O, "Sets SfMarkerShuffle mul factor."},
{"setAdd", (PyCFunction)SfMarkerShuffle_setAdd, METH_O, "Sets SfMarkerShuffle add factor."},
{"setSub", (PyCFunction)SfMarkerShuffle_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)SfMarkerShuffle_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods SfMarkerShuffle_as_number = {
(binaryfunc)SfMarkerShuffle_add,                      /*nb_add*/
(binaryfunc)SfMarkerShuffle_sub,                 /*nb_subtract*/
(binaryfunc)SfMarkerShuffle_multiply,                 /*nb_multiply*/
(binaryfunc)SfMarkerShuffle_div,                   /*nb_divide*/
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
(binaryfunc)SfMarkerShuffle_inplace_add,              /*inplace_add*/
(binaryfunc)SfMarkerShuffle_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SfMarkerShuffle_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)SfMarkerShuffle_inplace_div,           /*inplace_divide*/
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

PyTypeObject SfMarkerShuffleType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SfMarkerShuffle_base",         /*tp_name*/
sizeof(SfMarkerShuffle),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfMarkerShuffle_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&SfMarkerShuffle_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"SfMarkerShuffle objects. Reads a channel from a soundfile directly from disk.",           /* tp_doc */
(traverseproc)SfMarkerShuffle_traverse,   /* tp_traverse */
(inquiry)SfMarkerShuffle_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SfMarkerShuffle_methods,             /* tp_methods */
SfMarkerShuffle_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SfMarkerShuffle_init,      /* tp_init */
0,                         /* tp_alloc */
SfMarkerShuffle_new,                 /* tp_new */
};

