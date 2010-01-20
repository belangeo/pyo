#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *env;
    PyObject *pitch;
    Stream *pitch_stream;
    PyObject *pos;
    Stream *pos_stream;
    PyObject *dur;
    Stream *dur_stream;
    int ngrains;
    float basedur;
    float pointerPos;
    float *startPos;
    float *gsize;
    float *gphase;
    int modebuffer[5];
} Granulator;

static void
Granulator_transform_iii(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float pit = PyFloat_AS_DOUBLE(self->pitch);
    float pos = PyFloat_AS_DOUBLE(self->pos);
    float dur = PyFloat_AS_DOUBLE(self->dur);
    
    inc = pit * (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aii(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float *pit = Stream_getData((Stream *)self->pitch_stream);
    float pos = PyFloat_AS_DOUBLE(self->pos);
    float dur = PyFloat_AS_DOUBLE(self->dur);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_iai(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;

    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);

    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);

    float pit = PyFloat_AS_DOUBLE(self->pitch);
    float *pos = Stream_getData((Stream *)self->pos_stream);
    float dur = PyFloat_AS_DOUBLE(self->dur);

    inc = pit * (1.0 / self->basedur) / self->sr;
    
    float gsize = dur * self->sr;
    
    for (j=0; j<self->ngrains; j++) {
        self->gsize[j] = gsize;
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;

            if (amp < 0.0001)
                self->startPos[j] = pos[i];

            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;

            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aai(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float *pit = Stream_getData((Stream *)self->pitch_stream);
    float *pos = Stream_getData((Stream *)self->pos_stream);
    float dur = PyFloat_AS_DOUBLE(self->dur);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
} 

static void
Granulator_transform_iia(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float pit = PyFloat_AS_DOUBLE(self->pitch);
    float pos = PyFloat_AS_DOUBLE(self->pos);
    float *dur = Stream_getData((Stream *)self->dur_stream);
    
    inc = pit * (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }     
}

static void
Granulator_transform_aia(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float *pit = Stream_getData((Stream *)self->pitch_stream);
    float pos = PyFloat_AS_DOUBLE(self->pos);
    float *dur = Stream_getData((Stream *)self->dur_stream);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    } 
}

static void
Granulator_transform_iaa(Granulator *self) {
    float val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float pit = PyFloat_AS_DOUBLE(self->pitch);
    float *pos = Stream_getData((Stream *)self->pos_stream);
    float *dur = Stream_getData((Stream *)self->dur_stream);
    
    inc = pit * (1.0 / self->basedur) / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aaa(Granulator *self) { 
    float val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    float *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    float *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    float *pit = Stream_getData((Stream *)self->pitch_stream);
    float *pos = Stream_getData((Stream *)self->pos_stream);
    float *dur = Stream_getData((Stream *)self->dur_stream);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    } 
}

static void Granulator_postprocessing_ii(Granulator *self) { POST_PROCESSING_II };
static void Granulator_postprocessing_ai(Granulator *self) { POST_PROCESSING_AI };
static void Granulator_postprocessing_ia(Granulator *self) { POST_PROCESSING_IA };
static void Granulator_postprocessing_aa(Granulator *self) { POST_PROCESSING_AA };
static void Granulator_postprocessing_ireva(Granulator *self) { POST_PROCESSING_IREVA };
static void Granulator_postprocessing_areva(Granulator *self) { POST_PROCESSING_AREVA };
static void Granulator_postprocessing_revai(Granulator *self) { POST_PROCESSING_REVAI };
static void Granulator_postprocessing_revaa(Granulator *self) { POST_PROCESSING_REVAA };
static void Granulator_postprocessing_revareva(Granulator *self) { POST_PROCESSING_REVAREVA };

static void
Granulator_setProcMode(Granulator *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Granulator_transform_iii;
            break;
        case 1:    
            self->proc_func_ptr = Granulator_transform_aii;
            break;
        case 10:        
            self->proc_func_ptr = Granulator_transform_iai;
            break;
        case 11:    
            self->proc_func_ptr = Granulator_transform_aai;
            break;
        case 100:        
            self->proc_func_ptr = Granulator_transform_iia;
            break;
        case 101:    
            self->proc_func_ptr = Granulator_transform_aia;
            break;
        case 110:        
            self->proc_func_ptr = Granulator_transform_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Granulator_transform_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Granulator_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Granulator_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Granulator_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Granulator_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Granulator_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Granulator_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Granulator_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Granulator_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Granulator_postprocessing_revareva;
            break;
    }   
}

static void
Granulator_compute_next_data_frame(Granulator *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Granulator_traverse(Granulator *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->pitch);    
    Py_VISIT(self->pitch_stream);    
    Py_VISIT(self->pos);    
    Py_VISIT(self->pos_stream);    
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
Granulator_clear(Granulator *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->pitch);    
    Py_CLEAR(self->pitch_stream);    
    Py_CLEAR(self->pos);    
    Py_CLEAR(self->pos_stream);    
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
Granulator_dealloc(Granulator* self)
{
    int i;
    free(self->data);   
    free(self->startPos);   
    free(self->gphase);
    free(self->gsize);
    Granulator_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Granulator_deleteStream(Granulator *self) { DELETE_STREAM };

static PyObject *
Granulator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Granulator *self;
    self = (Granulator *)type->tp_alloc(type, 0);

    self->pitch = PyFloat_FromDouble(1);
    self->pos = PyFloat_FromDouble(0.0);
    self->dur = PyFloat_FromDouble(0.1);
    self->ngrains = 8;
    self->basedur = 0.1;
    self->pointerPos = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Granulator_compute_next_data_frame);
    self->mode_func_ptr = Granulator_setProcMode;
    
    return (PyObject *)self;
}

static int
Granulator_init(Granulator *self, PyObject *args, PyObject *kwds)
{
    int i;
    float phase;
    PyObject *tabletmp, *envtmp, *pitchtmp=NULL, *postmp=NULL, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"table", "env", "pitch", "pos", "dur", "grains", "basedur", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|OOOifOO", kwlist, &tabletmp, &envtmp, &pitchtmp, &postmp, &durtmp, &self->ngrains, &self->basedur, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    Py_XDECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)envtmp, "getTableStream", "");
    
    if (pitchtmp) {
        PyObject_CallMethod((PyObject *)self, "setPitch", "O", pitchtmp);
    }

    if (postmp) {
        PyObject_CallMethod((PyObject *)self, "setPos", "O", postmp);
    }

    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
 
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->startPos = (float *)realloc(self->startPos, self->ngrains * sizeof(float));
    self->gsize = (float *)realloc(self->gsize, self->ngrains * sizeof(float));
    self->gphase = (float *)realloc(self->gphase, self->ngrains * sizeof(float));

    srand((unsigned)(time(0)));
    for (i=0; i<self->ngrains; i++) {
        phase = ((float)i/self->ngrains) * (1.0 + ((rand()/((float)(RAND_MAX)+1)*2.0-1.0) * 0.015));
        if (phase < 0.0)
            phase = 0.0;
        self->gphase[i] = phase;
        self->startPos[i] = self->gsize[i] = 0.0;
    }
    
    (*self->mode_func_ptr)(self);

    Granulator_compute_next_data_frame((Granulator *)self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Granulator_getServer(Granulator* self) { GET_SERVER };
static PyObject * Granulator_getStream(Granulator* self) { GET_STREAM };
static PyObject * Granulator_setMul(Granulator *self, PyObject *arg) { SET_MUL };	
static PyObject * Granulator_setAdd(Granulator *self, PyObject *arg) { SET_ADD };	
static PyObject * Granulator_setSub(Granulator *self, PyObject *arg) { SET_SUB };	
static PyObject * Granulator_setDiv(Granulator *self, PyObject *arg) { SET_DIV };	

static PyObject * Granulator_play(Granulator *self) { PLAY };
static PyObject * Granulator_out(Granulator *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Granulator_stop(Granulator *self) { STOP };

static PyObject * Granulator_multiply(Granulator *self, PyObject *arg) { MULTIPLY };
static PyObject * Granulator_inplace_multiply(Granulator *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Granulator_add(Granulator *self, PyObject *arg) { ADD };
static PyObject * Granulator_inplace_add(Granulator *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Granulator_sub(Granulator *self, PyObject *arg) { SUB };
static PyObject * Granulator_inplace_sub(Granulator *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Granulator_div(Granulator *self, PyObject *arg) { DIV };
static PyObject * Granulator_inplace_div(Granulator *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Granulator_setPitch(Granulator *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pitch);
	if (isNumber == 1) {
		self->pitch = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->pitch = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pitch, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pitch_stream);
        self->pitch_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setPos(Granulator *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pos);
	if (isNumber == 1) {
		self->pos = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->pos = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pos, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pos_stream);
        self->pos_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setDur(Granulator *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Granulator_getTable(Granulator* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Granulator_setTable(Granulator *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_getEnv(Granulator* self)
{
    Py_INCREF(self->env);
    return self->env;
};

static PyObject *
Granulator_setEnv(Granulator *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setBaseDur(Granulator *self, PyObject *arg)
{	
	if (arg != NULL)
        self->basedur = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
        
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Granulator_members[] = {
    {"server", T_OBJECT_EX, offsetof(Granulator, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Granulator, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Granulator, table), 0, "Sound table."},
    {"env", T_OBJECT_EX, offsetof(Granulator, env), 0, "Envelope table."},
    {"pos", T_OBJECT_EX, offsetof(Granulator, pos), 0, "Position in the sound table."},
    {"dur", T_OBJECT_EX, offsetof(Granulator, dur), 0, "Duration of each grains."},
    {"mul", T_OBJECT_EX, offsetof(Granulator, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Granulator, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Granulator_methods[] = {
    {"getTable", (PyCFunction)Granulator_getTable, METH_NOARGS, "Returns sound table object."},
    {"setTable", (PyCFunction)Granulator_setTable, METH_O, "Sets sound table."},
    {"getEnv", (PyCFunction)Granulator_getEnv, METH_NOARGS, "Returns envelope table object."},
    {"setEnv", (PyCFunction)Granulator_setEnv, METH_O, "Sets envelope table."},
    {"getServer", (PyCFunction)Granulator_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Granulator_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Granulator_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Granulator_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Granulator_out, METH_VARARGS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Granulator_stop, METH_NOARGS, "Stops computing."},
	{"setPitch", (PyCFunction)Granulator_setPitch, METH_O, "Sets global pitch factor."},
    {"setPos", (PyCFunction)Granulator_setPos, METH_O, "Sets position in the sound table."},
    {"setDur", (PyCFunction)Granulator_setDur, METH_O, "Sets the grain duration."},
    {"setBaseDur", (PyCFunction)Granulator_setBaseDur, METH_O, "Sets the grain base duration."},
	{"setMul", (PyCFunction)Granulator_setMul, METH_O, "Sets granulator mul factor."},
	{"setAdd", (PyCFunction)Granulator_setAdd, METH_O, "Sets granulator add factor."},
    {"setSub", (PyCFunction)Granulator_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Granulator_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Granulator_as_number = {
    (binaryfunc)Granulator_add,                      /*nb_add*/
    (binaryfunc)Granulator_sub,                 /*nb_subtract*/
    (binaryfunc)Granulator_multiply,                 /*nb_multiply*/
    (binaryfunc)Granulator_div,                   /*nb_divide*/
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
    (binaryfunc)Granulator_inplace_add,              /*inplace_add*/
    (binaryfunc)Granulator_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Granulator_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Granulator_inplace_div,           /*inplace_divide*/
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

PyTypeObject GranulatorType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_pitch*/
    "_pyo.Granulator_base",         /*tp_name*/
    sizeof(Granulator),         /*tp_basicpitch*/
    0,                         /*tp_itempitch*/
    (destructor)Granulator_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Granulator_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Granulator objects. Accumulation of multiples grains of sound.",           /* tp_doc */
    (traverseproc)Granulator_traverse,   /* tp_traverse */
    (inquiry)Granulator_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Granulator_methods,             /* tp_methods */
    Granulator_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Granulator_init,      /* tp_init */
    0,                         /* tp_alloc */
    Granulator_new,                 /* tp_new */
};

