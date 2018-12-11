/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
 *                                                                        *
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *
 *                                                                        *
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with pyo.  If not, see <http://www.gnu.org/licenses/>.   *
 *************************************************************************/

#include <Python.h>
#include "py2to3.h"
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
    int modebuffer[1];
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    int loop;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    MYFLT srScale;
    MYFLT startPos;
    double pointerPos;
    MYFLT *samplesBuffer;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
    int init;
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} SfPlayer;

static void
SfPlayer_readframes_i(SfPlayer *self) {
    MYFLT sp, frac, bufpos, delta, startPos;
    int i, j, totlen, buflen, shortbuflen, pad, bufindex;
    sf_count_t index;

    if (self->modebuffer[0] == 0)
        sp = PyFloat_AS_DOUBLE(self->speed);
    else
        sp = Stream_getData((Stream *)self->speed_stream)[0];
    delta = MYFABS(sp) * self->srScale;

    buflen = (int)(self->bufsize * delta + 0.5) + 64;
    totlen = self->sndChnls*buflen;
    MYFLT buffer[totlen];
    MYFLT buffer2[self->sndChnls][buflen];

    if (sp > 0) { /* forward reading */
        if (self->pointerPos >= self->sndSize) {
            self->pointerPos -= self->sndSize - self->startPos;
            if (self->loop == 0) {
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
                for (i=0; i<(self->bufsize * self->sndChnls); i++) {
                    self->samplesBuffer[i] = 0.0;
                }
                for (i=0; i<self->bufsize; i++) {
                    self->trigsBuffer[i] = 0.0;
                }
                return;
            }
        }
        index = (int)self->pointerPos;
        sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */

        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples left in the file */
        if ((index+buflen) > self->sndSize) {
            shortbuflen = self->sndSize - index;
            pad = (buflen-shortbuflen)*self->sndChnls;
            SF_READ(self->sf, buffer, shortbuflen*self->sndChnls);
            if (self->loop == 0) { /* with zero padding if noloop */
                for (i=0; i<pad; i++) {
                    buffer[i+shortbuflen*self->sndChnls] = 0.;
                }
            }
            else { /* wrap around and read new samples if loop */
                MYFLT buftemp[pad];
                sf_seek(self->sf, (int)self->startPos, SEEK_SET);
                SF_READ(self->sf, buftemp, pad);
                for (i=0; i<(pad); i++) {
                    buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
                }
            }
        }
        else /* without zero padding */
            SF_READ(self->sf, buffer, totlen);

        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }

        /* fill samplesBuffer with samples */
        for (i=0; i<self->bufsize; i++) {
            self->trigsBuffer[i] = 0.0;
            bufpos = self->pointerPos - index;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos += delta;
        }
        if (self->pointerPos >= self->sndSize)
            self->trigsBuffer[0] = 1.0;
    }
    else if (sp < 0){ /* backward reading */
        startPos = self->startPos;
        if (startPos == 0.)
            startPos = self->sndSize - 1;
        if (self->pointerPos == 0.0)
            self->pointerPos = self->sndSize - 1;

        if (self->pointerPos <= 0) {
            self->pointerPos += startPos;
            if (self->loop == 0) {
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
                for (i=0; i<(self->bufsize * self->sndChnls); i++) {
                    self->samplesBuffer[i] = 0.0;
                }
                for (i=0; i<self->bufsize; i++) {
                    self->trigsBuffer[i] = 0.0;
                }
                return;
            }
        }

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
                MYFLT buftemp[padlen];
                sf_seek(self->sf, (int)startPos-pad, SEEK_SET);
                SF_READ(self->sf, buftemp, padlen);
                for (i=0; i<padlen; i++) {
                    buffer[i] = buftemp[i];
                }
            }

            MYFLT buftemp2[shortbuflen*self->sndChnls];
            sf_seek(self->sf, 0, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buftemp2, shortbuflen*self->sndChnls);
            for (i=0; i<(shortbuflen*self->sndChnls); i++) {
                buffer[i+padlen] = buftemp2[i];
            }
        }
        else /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buffer, totlen);

        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }

        /* reverse arrays */
        MYFLT swap;
        for (i=0; i<self->sndChnls; i++) {
            int a;
            int b = buflen;
            for (a=0; a<--b; a++) { //increment a and decrement b until they meet eachother
                swap = buffer2[i][a];       //put what's in a into swap space
                buffer2[i][a] = buffer2[i][b];    //put what's in b into a
                buffer2[i][b] = swap;       //put what's in the swap (a) into b
            }
        }

        /* fill stream buffer with samples */
        for (i=0; i<self->bufsize; i++) {
            self->trigsBuffer[i] = 0.0;
            bufpos = index - self->pointerPos;
            bufindex = (int)bufpos;
            frac = bufpos - bufindex;
            for (j=0; j<self->sndChnls; j++) {
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos -= delta;
        }
        if (self->pointerPos <= 0) {
            if (self->init == 0)
                self->trigsBuffer[0] = 1.0;
            else
                self->init = 0;
        }
    }
    else { /* speed == 0.0 */
        for (i = 0; i < (self->bufsize*self->sndChnls); i++) {
            self->samplesBuffer[i] = 0.0;
        }
    }
}

static void
SfPlayer_setProcMode(SfPlayer *self)
{
    self->proc_func_ptr = SfPlayer_readframes_i;
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
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
SfPlayer_clear(SfPlayer *self)
{
    pyo_CLEAR
    Py_CLEAR(self->speed);
    Py_CLEAR(self->speed_stream);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
SfPlayer_dealloc(SfPlayer* self)
{
    pyo_DEALLOC
    if (self->sf != NULL)
        sf_close(self->sf);
    free(self->trigsBuffer);
    free(self->samplesBuffer);
    SfPlayer_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfPlayer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Py_ssize_t psize;
    MYFLT offset = 0.;
    PyObject *speedtmp=NULL;
    SfPlayer *self;
    self = (SfPlayer *)type->tp_alloc(type, 0);

    self->speed = PyFloat_FromDouble(1);
    self->loop = 0;
    self->interp = 2;
    self->init = 1;
	self->modebuffer[0] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfPlayer_compute_next_data_frame);
    self->mode_func_ptr = SfPlayer_setProcMode;

    static char *kwlist[] = {"path", "speed", "loop", "offset", "interp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_P__OIFI, kwlist, &self->path, &psize, &speedtmp, &self->loop, &offset, &self->interp))
        Py_RETURN_NONE;

    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    SET_INTERP_POINTER

    /* Open the sound file. */
    self->info.format = 0;
    self->sf = sf_open(self->path, SFM_READ, &self->info);
    if (self->sf == NULL)
    {
        PySys_WriteStdout("SfPlayer: failed to open the file.\n");
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;

    self->samplesBuffer = (MYFLT *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(MYFLT));
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    self->startPos = offset * self->sr * self->srScale;
    if (self->startPos < 0.0 || self->startPos >= self->sndSize)
        self->startPos = 0.0;

    self->pointerPos = self->startPos;

    return (PyObject *)self;
}

static PyObject * SfPlayer_getServer(SfPlayer* self) { GET_SERVER };
static PyObject * SfPlayer_getStream(SfPlayer* self) { GET_STREAM };
static PyObject * SfPlayer_getTriggerStream(SfPlayer* self) { GET_TRIGGER_STREAM };

static PyObject * SfPlayer_play(SfPlayer *self, PyObject *args, PyObject *kwds)
{
    self->init = 1;
    self->pointerPos = self->startPos;
    PLAY
};

static PyObject * SfPlayer_out(SfPlayer *self, PyObject *args, PyObject *kwds)
{
    self->init = 1;
    self->pointerPos = self->startPos;
    OUT
};

static PyObject * SfPlayer_stop(SfPlayer *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
SfPlayer_setSpeed(SfPlayer *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->speed);
	if (isNumber == 1) {
		self->speed = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->speed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->speed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->speed_stream);
        self->speed_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_RETURN_NONE;
}

static PyObject *
SfPlayer_setSound(SfPlayer *self, PyObject *args)
{
    /* Need to perform a check to be sure that the new 
       sound is of the same number of channels. */
    Py_ssize_t psize;

    //ASSERT_ARG_NOT_NULL

    if (! PyArg_ParseTuple(args, "s#", &self->path, &psize))
        Py_RETURN_NONE;

    //self->path = PY_STRING_AS_STRING(arg);

    sf_close(self->sf);

    /* Open the sound file. */
    self->info.format = 0;
    self->sf = sf_open(self->path, SFM_READ, &self->info);
    if (self->sf == NULL)
    {
        PySys_WriteStdout("SfPlayer: failed to open the file.\n");
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    //self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;

    //self->samplesBuffer = (MYFLT *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(MYFLT));

    self->startPos = 0.0;
    self->pointerPos = self->startPos;

    Py_RETURN_NONE;
}

static PyObject *
SfPlayer_setLoop(SfPlayer *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    self->loop = PyInt_AsLong(arg);

    Py_RETURN_NONE;
}

static PyObject *
SfPlayer_setOffset(SfPlayer *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->startPos = PyFloat_AsDouble(arg) * self->sr * self->srScale;
        if (self->startPos < 0.0 || self->startPos >= self->sndSize)
            self->startPos = 0.0;
    }

    Py_RETURN_NONE;
}

static PyObject *
SfPlayer_setInterp(SfPlayer *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }

    SET_INTERP_POINTER

    Py_RETURN_NONE;
}

MYFLT *
SfPlayer_getSamplesBuffer(SfPlayer *self)
{
    return (MYFLT *)self->samplesBuffer;
}

static PyMemberDef SfPlayer_members[] = {
{"server", T_OBJECT_EX, offsetof(SfPlayer, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(SfPlayer, stream), 0, "Stream object."},
{"trig_stream", T_OBJECT_EX, offsetof(SfPlayer, trig_stream), 0, "Trigger Stream object."},
{"speed", T_OBJECT_EX, offsetof(SfPlayer, speed), 0, "Frequency in cycle per second."},
{NULL}  /* Sentinel */
};

static PyMethodDef SfPlayer_methods[] = {
{"getServer", (PyCFunction)SfPlayer_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)SfPlayer_getStream, METH_NOARGS, "Returns stream object."},
{"_getTriggerStream", (PyCFunction)SfPlayer_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
{"play", (PyCFunction)SfPlayer_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfPlayer_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfPlayer_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setSound", (PyCFunction)SfPlayer_setSound, METH_VARARGS, "Sets sfplayer sound path."},
{"setSpeed", (PyCFunction)SfPlayer_setSpeed, METH_O, "Sets sfplayer reading speed."},
{"setLoop", (PyCFunction)SfPlayer_setLoop, METH_O, "Sets sfplayer loop mode (0 = no loop, 1 = loop)."},
{"setOffset", (PyCFunction)SfPlayer_setOffset, METH_O, "Sets sfplayer start position."},
{"setInterp", (PyCFunction)SfPlayer_setInterp, METH_O, "Sets sfplayer interpolation mode."},
{NULL}  /* Sentinel */
};

PyTypeObject SfPlayerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SfPlayer_base",         /*tp_name*/
sizeof(SfPlayer),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfPlayer_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
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
0,      /* tp_init */
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
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SfPlayer_getSamplesBuffer((SfPlayer *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
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
    pyo_DEALLOC
    SfPlay_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfPlay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    SfPlay *self;
    self = (SfPlay *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfPlay_compute_next_data_frame);
    self->mode_func_ptr = SfPlay_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (SfPlayer *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SfPlay_getServer(SfPlay* self) { GET_SERVER };
static PyObject * SfPlay_getStream(SfPlay* self) { GET_STREAM };
static PyObject * SfPlay_setMul(SfPlay *self, PyObject *arg) { SET_MUL };
static PyObject * SfPlay_setAdd(SfPlay *self, PyObject *arg) { SET_ADD };
static PyObject * SfPlay_setSub(SfPlay *self, PyObject *arg) { SET_SUB };
static PyObject * SfPlay_setDiv(SfPlay *self, PyObject *arg) { SET_DIV };

static PyObject * SfPlay_play(SfPlay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SfPlay_out(SfPlay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfPlay_stop(SfPlay *self, PyObject *args, PyObject *kwds) { STOP };

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
{"play", (PyCFunction)SfPlay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfPlay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfPlay_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
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
INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
(binaryfunc)SfPlay_inplace_add,              /*inplace_add*/
(binaryfunc)SfPlay_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SfPlay_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)SfPlay_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)SfPlay_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SfPlayType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SfPlay_base",         /*tp_name*/
sizeof(SfPlay),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfPlay_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
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
0,      /* tp_init */
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
    int modebuffer[1];
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    int lastDir;
    MYFLT srScale;
    MYFLT startPos;
    MYFLT endPos;
    MYFLT nextStartPos;
    MYFLT nextEndPos;
    double pointerPos;
    MYFLT *samplesBuffer;
    MYFLT *markers;
    int markers_size;
    MYFLT x;
    MYFLT (*type_func_ptr)();
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} SfMarkerShuffler;

/*** PROTOTYPES ***/
static void SfMarkerShuffler_chooseNewMark(SfMarkerShuffler *self, int dir);
/******************/

// no parameter
static MYFLT
SfMarkerShuffler_uniform(SfMarkerShuffler *self) {
    return RANDOM_UNIFORM;
}

static MYFLT
SfMarkerShuffler_linear_min(SfMarkerShuffler *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a < b) return a;
    else return b;
}

static MYFLT
SfMarkerShuffler_linear_max(SfMarkerShuffler *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    if (a > b) return a;
    else return b;
}

static MYFLT
SfMarkerShuffler_triangle(SfMarkerShuffler *self) {
    MYFLT a = RANDOM_UNIFORM;
    MYFLT b = RANDOM_UNIFORM;
    return ((a + b) * 0.5);
}

// x1 = slope
static MYFLT
SfMarkerShuffler_expon_min(SfMarkerShuffler *self) {
    if (self->x <= 0.0) self->x = 0.00001;
    MYFLT val = -MYLOG(RANDOM_UNIFORM) / self->x;
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
SfMarkerShuffler_expon_max(SfMarkerShuffler *self) {
    if (self->x <= 0.0) self->x = 0.00001;
    MYFLT val = 1.0 - (-MYLOG(RANDOM_UNIFORM) / self->x);
    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x1 = bandwidth
static MYFLT
SfMarkerShuffler_biexpon(SfMarkerShuffler *self) {
    MYFLT polar, val;
    if (self->x <= 0.0) self->x = 0.00001;
    MYFLT sum = RANDOM_UNIFORM * 2.0;

    if (sum > 1.0) {
        polar = -1;
        sum = 2.0 - sum;
    }
    else
        polar = 1;

    val = 0.5 * (polar * MYLOG(sum) / self->x) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static MYFLT
SfMarkerShuffler_cauchy(SfMarkerShuffler *self) {
    MYFLT rnd, val, dir;
    do {
        rnd = RANDOM_UNIFORM;
    }
    while (rnd == 0.5);

    if (pyorand() < (PYO_RAND_MAX / 2))
        dir = -1;
    else
        dir = 1;

    val = 0.5 * (MYTAN(rnd) * self->x * dir) + 0.5;

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x = shape
static MYFLT
SfMarkerShuffler_weibull(SfMarkerShuffler *self) {
    MYFLT rnd, val;
    if (self->x <= 0.0) self->x = 0.00001;

    rnd = 1.0 / (1.0 - RANDOM_UNIFORM);
    val = 0.5 * MYPOW(MYLOG(rnd), (1.0 / self->x));

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

// x = bandwidth
static MYFLT
SfMarkerShuffler_gaussian(SfMarkerShuffler *self) {
    MYFLT rnd, val;

    rnd = (RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM + RANDOM_UNIFORM);
    val = (self->x * (rnd - 3.0) * 0.33 + 0.5);

    if (val < 0.0) return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

static void
SfMarkerShuffler_readframes_i(SfMarkerShuffler *self) {
    MYFLT sp, frac, bufpos, delta, tmp;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;

    if (self->modebuffer[0] == 0)
        sp = PyFloat_AS_DOUBLE(self->speed);
    else
        sp = Stream_getData((Stream *)self->speed_stream)[0];

    delta = MYFABS(sp) * self->srScale;

    buflen = (int)(self->bufsize * delta + 0.5) + 64;
    totlen = self->sndChnls*buflen;
    MYFLT buffer[totlen];
    MYFLT buffer2[self->sndChnls][buflen];

    if (sp > 0) { /* reading forward */
        if (self->startPos == -1 || self->lastDir == 0) {
            self->lastDir = 1;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos;
        }
        if (self->lastDir != 1) {
            tmp = self->startPos;
            self->startPos = self->endPos;
            self->endPos = tmp;
            self->lastDir = 1;
        }
        index = (int)self->pointerPos;
        sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */

        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index+buflen) > self->endPos) {
            shortbuflen = self->endPos - index;
            SF_READ(self->sf, buffer, shortbuflen*self->sndChnls);

            /* wrap around and read new samples from new marker */
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            MYFLT buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos, SEEK_SET);
            SF_READ(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }
        else /* without wraparound */
            SF_READ(self->sf, buffer, totlen);

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
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos += delta;
        }
        if (self->pointerPos >= self->endPos) {
            MYFLT off = self->pointerPos - self->endPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 1);
            self->pointerPos = self->startPos + off;
        }
    }
    else if (sp < 0) { /* reading backward */
        if (self->startPos == -1 || self->lastDir != -1) {
            self->lastDir = self->startPos = -1;
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

            /* wrap around and read new samples from new marker */
            MYFLT buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos-pad, SEEK_SET);
            SF_READ(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i] = buftemp[i];
            }

            MYFLT buftemp2[shortbuflen*self->sndChnls];
            sf_seek(self->sf, self->endPos, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buftemp2, shortbuflen*self->sndChnls);
            for (i=0; i<(shortbuflen*self->sndChnls); i++) {
                buffer[i+padlen] = buftemp2[i];
            }
        }
        else { /* without wraparound */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buffer, totlen);
        }
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }

        /* reverse arrays */
        MYFLT swap;
        for (i=0; i<self->sndChnls; i++) {
            int a;
            int b = buflen;
            for (a=0; a<--b; a++) { //increment a and decrement b until they meet eachother
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
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos -= delta;
        }
        if (self->pointerPos <= self->endPos) {
            MYFLT off = self->endPos - self->pointerPos;
            SfMarkerShuffler_chooseNewMark((SfMarkerShuffler *)self, 0);
            self->pointerPos = self->startPos - off;
        }
    }
    else { /* speed == 0 */
        self->lastDir = 0;
        for (i = 0; i < (self->bufsize*self->sndChnls); i++) {
            self->samplesBuffer[i] = 0.0;
        }
    }
}

static void
SfMarkerShuffler_setProcMode(SfMarkerShuffler *self)
{
    self->proc_func_ptr = SfMarkerShuffler_readframes_i;
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
            mark = (int)(self->markers_size * (*self->type_func_ptr)(self) * 0.99);
            self->startPos = self->markers[mark];
            self->endPos = self->markers[mark+1];
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }

        mark = (int)(self->markers_size * (*self->type_func_ptr)(self) * 0.99);
        self->nextStartPos = self->markers[mark];
        self->nextEndPos = self->markers[mark+1];
    }
    else {
        if (self->startPos == -1) {
            mark = self->markers_size - (int)(self->markers_size * (*self->type_func_ptr)(self) * 0.99);
            self->startPos = self->markers[mark];
            self->endPos = self->markers[mark-1];
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }

        mark = self->markers_size - (int)(self->markers_size * (*self->type_func_ptr)(self) * 0.99);
        self->nextStartPos = self->markers[mark];
        self->nextEndPos = self->markers[mark-1];
    }
}

static void
SfMarkerShuffler_setMarkers(SfMarkerShuffler *self, PyObject *markerstmp)
{
    Py_ssize_t i;
    Py_ssize_t len = PyList_Size(markerstmp);
    self->markers = (MYFLT *)realloc(self->markers, (len + 2) * sizeof(MYFLT));
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
    pyo_DEALLOC
    if (self->sf != NULL)
        sf_close(self->sf);
    free(self->samplesBuffer);
    free(self->markers);
    SfMarkerShuffler_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfMarkerShuffler_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Py_ssize_t psize;
    PyObject *speedtmp=NULL, *markerstmp=NULL;
    SfMarkerShuffler *self;
    self = (SfMarkerShuffler *)type->tp_alloc(type, 0);

    self->speed = PyFloat_FromDouble(1);
    self->interp = 2;
    self->startPos = -1;
    self->endPos = -1;
	self->modebuffer[0] = 0;
    self->lastDir = 1;
    self->x = 0.5;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerShuffler_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerShuffler_setProcMode;

    static char *kwlist[] = {"path", "markers", "speed", "interp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#O|Oi", kwlist, &self->path, &psize, &markerstmp, &speedtmp, &self->interp))
        Py_RETURN_NONE;

    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    self->type_func_ptr = SfMarkerShuffler_uniform;

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
        PySys_WriteStdout("SfMarkerShuffler: failed to open the file.\n");
        Py_RETURN_NONE;
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;

    Py_INCREF(markerstmp);
    SfMarkerShuffler_setMarkers((SfMarkerShuffler *)self, markerstmp);

    self->samplesBuffer = (MYFLT *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(MYFLT));

    Server_generateSeed((Server *)self->server, SFMARKERSHUFFLER_ID);

    return (PyObject *)self;
}

static PyObject * SfMarkerShuffler_getServer(SfMarkerShuffler* self) { GET_SERVER };
static PyObject * SfMarkerShuffler_getStream(SfMarkerShuffler* self) { GET_STREAM };

static PyObject * SfMarkerShuffler_play(SfMarkerShuffler *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SfMarkerShuffler_out(SfMarkerShuffler *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerShuffler_stop(SfMarkerShuffler *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
SfMarkerShuffler_setSpeed(SfMarkerShuffler *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->speed);
	if (isNumber == 1) {
		self->speed = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->speed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->speed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->speed_stream);
        self->speed_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

    (*self->mode_func_ptr)(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SfMarkerShuffler_setInterp(SfMarkerShuffler *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

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

    Py_RETURN_NONE;
}

static PyObject *
SfMarkerShuffler_setRandomType(SfMarkerShuffler *self, PyObject *args, PyObject *kwds)
{
    int dist = 0;
    MYFLT x = 0.5;

    static char *kwlist[] = {"dist", "x", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__IF, kwlist, &dist, &x))
        Py_RETURN_NONE;

    if (dist < 0 || dist > 9) {
        Py_RETURN_NONE;
    }

    if (x < 0.0)
        self->x = 0.0;
    else if (x >= 1.0)
        self->x = 1.0;
    else
        self->x = x;

    switch (dist) {
        case 0:
            self->type_func_ptr = SfMarkerShuffler_uniform;
            break;
        case 1:
            self->type_func_ptr = SfMarkerShuffler_linear_min;
            break;
        case 2:
            self->type_func_ptr = SfMarkerShuffler_linear_max;
            break;
        case 3:
            self->type_func_ptr = SfMarkerShuffler_triangle;
            break;
        case 4:
            self->type_func_ptr = SfMarkerShuffler_expon_min;
            self->x *= 10.0;
            break;
        case 5:
            self->type_func_ptr = SfMarkerShuffler_expon_max;
            self->x *= 10.0;
            break;
        case 6:
            self->type_func_ptr = SfMarkerShuffler_biexpon;
            self->x *= 10.0;
            break;
        case 7:
            self->type_func_ptr = SfMarkerShuffler_cauchy;
            self->x *= 10.0;
            self->x = 10.0 - self->x;
            break;
        case 8:
            self->type_func_ptr = SfMarkerShuffler_weibull;
            self->x = self->x * 5.0 + 0.1;
            break;
        case 9:
            self->type_func_ptr = SfMarkerShuffler_gaussian;
            self->x *= 10.0;
            self->x = 10.0 - self->x;
            break;
    }

    Py_RETURN_NONE;
}

MYFLT *
SfMarkerShuffler_getSamplesBuffer(SfMarkerShuffler *self)
{
    return (MYFLT *)self->samplesBuffer;
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
{"play", (PyCFunction)SfMarkerShuffler_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfMarkerShuffler_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfMarkerShuffler_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setSpeed", (PyCFunction)SfMarkerShuffler_setSpeed, METH_O, "Sets sfplayer reading speed."},
{"setInterp", (PyCFunction)SfMarkerShuffler_setInterp, METH_O, "Sets sfplayer interpolation mode."},
{"setRandomType", (PyCFunction)SfMarkerShuffler_setRandomType, METH_VARARGS|METH_KEYWORDS, "Sets sfplayer random type."},
{NULL}  /* Sentinel */
};

PyTypeObject SfMarkerShufflerType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SfMarkerShuffler_base",         /*tp_name*/
sizeof(SfMarkerShuffler),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfMarkerShuffler_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
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
0,      /* tp_init */
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
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SfMarkerShuffler_getSamplesBuffer((SfMarkerShuffler *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
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
    pyo_DEALLOC
    SfMarkerShuffle_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfMarkerShuffle_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    SfMarkerShuffle *self;
    self = (SfMarkerShuffle *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerShuffle_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerShuffle_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (SfMarkerShuffler *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SfMarkerShuffle_getServer(SfMarkerShuffle* self) { GET_SERVER };
static PyObject * SfMarkerShuffle_getStream(SfMarkerShuffle* self) { GET_STREAM };
static PyObject * SfMarkerShuffle_setMul(SfMarkerShuffle *self, PyObject *arg) { SET_MUL };
static PyObject * SfMarkerShuffle_setAdd(SfMarkerShuffle *self, PyObject *arg) { SET_ADD };
static PyObject * SfMarkerShuffle_setSub(SfMarkerShuffle *self, PyObject *arg) { SET_SUB };
static PyObject * SfMarkerShuffle_setDiv(SfMarkerShuffle *self, PyObject *arg) { SET_DIV };

static PyObject * SfMarkerShuffle_play(SfMarkerShuffle *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SfMarkerShuffle_out(SfMarkerShuffle *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerShuffle_stop(SfMarkerShuffle *self, PyObject *args, PyObject *kwds) { STOP };

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
{"play", (PyCFunction)SfMarkerShuffle_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)SfMarkerShuffle_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)SfMarkerShuffle_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
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
INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
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
INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
0,                       /*nb_int*/
0,                      /*nb_long*/
0,                     /*nb_float*/
INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
(binaryfunc)SfMarkerShuffle_inplace_add,              /*inplace_add*/
(binaryfunc)SfMarkerShuffle_inplace_sub,         /*inplace_subtract*/
(binaryfunc)SfMarkerShuffle_inplace_multiply,         /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
0,        /*inplace_remainder*/
0,           /*inplace_power*/
0,       /*inplace_lshift*/
0,      /*inplace_rshift*/
0,      /*inplace_and*/
0,      /*inplace_xor*/
0,       /*inplace_or*/
0,             /*nb_floor_divide*/
(binaryfunc)SfMarkerShuffle_div,                       /*nb_true_divide*/
0,     /*nb_inplace_floor_divide*/
(binaryfunc)SfMarkerShuffle_inplace_div,                       /*nb_inplace_true_divide*/
0,                     /* nb_index */
};

PyTypeObject SfMarkerShuffleType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.SfMarkerShuffle_base",         /*tp_name*/
sizeof(SfMarkerShuffle),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SfMarkerShuffle_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
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
0,      /* tp_init */
0,                         /* tp_alloc */
SfMarkerShuffle_new,                 /* tp_new */
};

/************************************************************************************************/
/* SfMarkerLooper object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *speed;
    Stream *speed_stream;
    PyObject *mark;
    Stream *mark_stream;
    int modebuffer[2];
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    int sndSize; /* number of frames */
    int sndChnls;
    int sndSr;
    MYFLT srScale;
    MYFLT startPos;
    MYFLT endPos;
    MYFLT nextStartPos;
    MYFLT nextEndPos;
    double pointerPos;
    MYFLT *samplesBuffer;
    MYFLT *markers;
    int markers_size;
    int old_mark;
    int lastDir;
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} SfMarkerLooper;

/*** PROTOTYPES ***/
static void SfMarkerLooper_chooseNewMark(SfMarkerLooper *self, int dir);
/******************/

static void
SfMarkerLooper_readframes_i(SfMarkerLooper *self) {
    MYFLT sp, frac, bufpos, delta, tmp;
    int i, j, totlen, buflen, shortbuflen, bufindex;
    sf_count_t index;

    if (self->modebuffer[0] == 0)
        sp = PyFloat_AS_DOUBLE(self->speed);
    else
        sp = Stream_getData((Stream *)self->speed_stream)[0];

    delta = MYFABS(sp) * self->srScale;

    buflen = (int)(self->bufsize * delta + 0.5) + 64;
    totlen = self->sndChnls*buflen;
    MYFLT buffer[totlen];
    MYFLT buffer2[self->sndChnls][buflen];

    if (sp > 0) { /* reading forward */
        if (self->startPos == -1 || self->lastDir == 0) {
            self->lastDir = 1;
            SfMarkerLooper_chooseNewMark((SfMarkerLooper *)self, 1);
            self->pointerPos = self->startPos;
        }
        if (self->lastDir != 1) {
            tmp = self->startPos;
            self->startPos = self->endPos;
            self->endPos = tmp;
            self->lastDir = 1;
        }
        index = (int)self->pointerPos;
        sf_seek(self->sf, index, SEEK_SET); /* sets position pointer in the file */

        /* fill a buffer with enough samples to satisfy speed reading */
        /* if not enough samples to read in the file */
        if ((index+buflen) > self->endPos) {
            shortbuflen = self->endPos - index;
            SF_READ(self->sf, buffer, shortbuflen*self->sndChnls);

            /* wrap around and read new samples if loop */
            int pad = buflen - shortbuflen;
            int padlen = pad*self->sndChnls;
            MYFLT buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos, SEEK_SET);
            SF_READ(self->sf, buftemp, padlen);
            for (i=0; i<(padlen); i++) {
                buffer[i+shortbuflen*self->sndChnls] = buftemp[i];
            }
        }
        else /* without zero padding */
            SF_READ(self->sf, buffer, totlen);

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
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos += delta;
        }
        if (self->pointerPos >= self->endPos) {
            MYFLT off = self->pointerPos - self->endPos;
            SfMarkerLooper_chooseNewMark((SfMarkerLooper *)self, 1);
            self->pointerPos = self->startPos + off;
        }
    }
    else if (sp < 0) { /* reading backward */
        if (self->startPos == -1 || self->lastDir != -1) {
            self->lastDir = self->startPos = -1;
            SfMarkerLooper_chooseNewMark((SfMarkerLooper *)self, 0);
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
            MYFLT buftemp[padlen];
            sf_seek(self->sf, (int)self->nextStartPos-pad, SEEK_SET);
            SF_READ(self->sf, buftemp, padlen);
            for (i=0; i<padlen; i++) {
                buffer[i] = buftemp[i];
            }

            MYFLT buftemp2[shortbuflen*self->sndChnls];
            sf_seek(self->sf, self->endPos, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buftemp2, shortbuflen*self->sndChnls);
            for (i=0; i<(shortbuflen*self->sndChnls); i++) {
                buffer[i+padlen] = buftemp2[i];
            }
        }
        else { /* without zero padding */
            sf_seek(self->sf, index-buflen, SEEK_SET); /* sets position pointer in the file */
            SF_READ(self->sf, buffer, totlen);
        }
        /* de-interleave samples */
        for (i=0; i<totlen; i++) {
            buffer2[i%self->sndChnls][(int)(i/self->sndChnls)] = buffer[i];
        }

        /* reverse arrays */
        MYFLT swap;
        for (i=0; i<self->sndChnls; i++) {
            int a;
            int b = buflen;
            for (a=0; a<--b; a++) { //increment a and decrement b until they meet eachother
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
                self->samplesBuffer[i+(j*self->bufsize)] = (*self->interp_func_ptr)(buffer2[j], bufindex, frac, buflen);
            }
            self->pointerPos -= delta;
        }
        if (self->pointerPos <= self->endPos) {
            MYFLT off = self->endPos - self->pointerPos;
            SfMarkerLooper_chooseNewMark((SfMarkerLooper *)self, 0);
            self->pointerPos = self->startPos - off;
        }
    }
    else { /* speed == 0 */
        self->lastDir = 0;
        for (i = 0; i < (self->bufsize*self->sndChnls); i++) {
            self->samplesBuffer[i] = 0.0;
        }
    }
}

static void
SfMarkerLooper_setProcMode(SfMarkerLooper *self)
{
    self->proc_func_ptr = SfMarkerLooper_readframes_i;
}

static void
SfMarkerLooper_compute_next_data_frame(SfMarkerLooper *self)
{
    (*self->proc_func_ptr)(self);
}

static void
SfMarkerLooper_chooseNewMark(SfMarkerLooper *self, int dir)
{
    int mark;

    if (self->modebuffer[1] == 0)
        mark = (int)(PyFloat_AS_DOUBLE(self->mark));
    else
        mark = (int)(Stream_getData((Stream *)self->mark_stream)[0]);

    if (mark < 0 || mark >= self->markers_size) {
        mark = self->markers_size / 2;
    }

    self->old_mark = mark;

    if (dir == 1) {
        if (self->startPos == -1) {
            self->startPos = self->markers[mark];
            self->endPos = self->markers[mark+1];
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }
        self->nextStartPos = self->markers[mark];
        self->nextEndPos = self->markers[mark+1];
    }
    else {
        mark = self->markers_size - mark;
        if (self->startPos == -1) {
            self->startPos = self->markers[mark];
            self->endPos = self->markers[mark-1];
        }
        else {
            self->startPos = self->nextStartPos;
            self->endPos = self->nextEndPos;
        }
        self->nextStartPos = self->markers[mark];
        self->nextEndPos = self->markers[mark-1];
    }
}

static void
SfMarkerLooper_setMarkers(SfMarkerLooper *self, PyObject *markerstmp)
{
    Py_ssize_t i;
    Py_ssize_t len = PyList_Size(markerstmp);
    self->markers = (MYFLT *)realloc(self->markers, (len + 2) * sizeof(MYFLT));
    self->markers[0] = 0.;
    for (i=0; i<len; i++) {
        self->markers[i+1] = PyFloat_AsDouble(PyList_GetItem(markerstmp, i));
    }
    self->markers[len+1] = self->sndSize;
    self->markers_size = (int)len+1;
}

static int
SfMarkerLooper_traverse(SfMarkerLooper *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->speed);
    Py_VISIT(self->speed_stream);
    Py_VISIT(self->mark);
    Py_VISIT(self->mark_stream);
     return 0;
}

static int
SfMarkerLooper_clear(SfMarkerLooper *self)
{
    pyo_CLEAR
    Py_CLEAR(self->speed);
    Py_CLEAR(self->speed_stream);
    Py_CLEAR(self->mark);
    Py_CLEAR(self->mark_stream);
    return 0;
}

static void
SfMarkerLooper_dealloc(SfMarkerLooper* self)
{
    pyo_DEALLOC
    if (self->sf != NULL)
        sf_close(self->sf);
    free(self->samplesBuffer);
    free(self->markers);
    SfMarkerLooper_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfMarkerLooper_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Py_ssize_t psize;
    PyObject *speedtmp=NULL, *marktmp=NULL, *markerstmp=NULL;
    SfMarkerLooper *self;
    self = (SfMarkerLooper *)type->tp_alloc(type, 0);

    self->speed = PyFloat_FromDouble(1);
    self->mark = PyFloat_FromDouble(0);
    self->interp = 2;
    self->startPos = -1;
    self->endPos = -1;
    self->old_mark = -1;
    self->lastDir = 1;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerLooper_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerLooper_setProcMode;

    static char *kwlist[] = {"path", "markers", "speed", "mark", "interp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#O|OOi", kwlist, &self->path, &psize, &markerstmp, &speedtmp, &marktmp, &self->interp))
        Py_RETURN_NONE;

    if (speedtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpeed", "O", speedtmp);
    }

    if (marktmp) {
        PyObject_CallMethod((PyObject *)self, "setMark", "O", marktmp);
    }

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
        PySys_WriteStdout("SfMarkerLooper: failed to open the file.\n");
        Py_RETURN_NONE;
    }
    self->sndSize = self->info.frames;
    self->sndSr = self->info.samplerate;
    self->sndChnls = self->info.channels;
    self->srScale = self->sndSr / self->sr;

    Py_INCREF(markerstmp);
    SfMarkerLooper_setMarkers((SfMarkerLooper *)self, markerstmp);

    self->samplesBuffer = (MYFLT *)realloc(self->samplesBuffer, self->bufsize * self->sndChnls * sizeof(MYFLT));

    Server_generateSeed((Server *)self->server, SFMARKERLOOPER_ID);

    return (PyObject *)self;
}

static PyObject * SfMarkerLooper_getServer(SfMarkerLooper* self) { GET_SERVER };
static PyObject * SfMarkerLooper_getStream(SfMarkerLooper* self) { GET_STREAM };

static PyObject * SfMarkerLooper_play(SfMarkerLooper *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SfMarkerLooper_out(SfMarkerLooper *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerLooper_stop(SfMarkerLooper *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
SfMarkerLooper_setSpeed(SfMarkerLooper *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->speed);
	if (isNumber == 1) {
		self->speed = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->speed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->speed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->speed_stream);
        self->speed_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SfMarkerLooper_setMark(SfMarkerLooper *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->mark);
	if (isNumber == 1) {
		self->mark = PyNumber_Float(tmp);
        self->modebuffer[1] = 0;
	}
	else {
		self->mark = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->mark, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->mark_stream);
        self->mark_stream = (Stream *)streamtmp;
		self->modebuffer[1] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
SfMarkerLooper_setInterp(SfMarkerLooper *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

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

    Py_RETURN_NONE;
}

MYFLT *
SfMarkerLooper_getSamplesBuffer(SfMarkerLooper *self)
{
    return (MYFLT *)self->samplesBuffer;
}

static PyMemberDef SfMarkerLooper_members[] = {
    {"server", T_OBJECT_EX, offsetof(SfMarkerLooper, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SfMarkerLooper, stream), 0, "Stream object."},
    {"speed", T_OBJECT_EX, offsetof(SfMarkerLooper, speed), 0, "Frequency in cycle per second."},
    {"mark", T_OBJECT_EX, offsetof(SfMarkerLooper, mark), 0, "Marker to loop."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SfMarkerLooper_methods[] = {
    {"getServer", (PyCFunction)SfMarkerLooper_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SfMarkerLooper_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)SfMarkerLooper_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SfMarkerLooper_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SfMarkerLooper_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setSpeed", (PyCFunction)SfMarkerLooper_setSpeed, METH_O, "Sets sfplayer reading speed."},
    {"setMark", (PyCFunction)SfMarkerLooper_setMark, METH_O, "Sets marker to loop."},
    {"setInterp", (PyCFunction)SfMarkerLooper_setInterp, METH_O, "Sets sfplayer interpolation mode."},
    {NULL}  /* Sentinel */
};

PyTypeObject SfMarkerLooperType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SfMarkerLooper_base",         /*tp_name*/
    sizeof(SfMarkerLooper),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SfMarkerLooper_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    "SfMarkerLooper objects. Shuffle an AIFF soundfile from markers points.",           /* tp_doc */
    (traverseproc)SfMarkerLooper_traverse,   /* tp_traverse */
    (inquiry)SfMarkerLooper_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SfMarkerLooper_methods,             /* tp_methods */
    SfMarkerLooper_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SfMarkerLooper_new,                 /* tp_new */
};

/************************************************************************************************/
/* SfMarkerLoop streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    SfMarkerLooper *mainPlayer;
    int modebuffer[2];
    int chnl;
} SfMarkerLoop;

static void SfMarkerLoop_postprocessing_ii(SfMarkerLoop *self) { POST_PROCESSING_II };
static void SfMarkerLoop_postprocessing_ai(SfMarkerLoop *self) { POST_PROCESSING_AI };
static void SfMarkerLoop_postprocessing_ia(SfMarkerLoop *self) { POST_PROCESSING_IA };
static void SfMarkerLoop_postprocessing_aa(SfMarkerLoop *self) { POST_PROCESSING_AA };
static void SfMarkerLoop_postprocessing_ireva(SfMarkerLoop *self) { POST_PROCESSING_IREVA };
static void SfMarkerLoop_postprocessing_areva(SfMarkerLoop *self) { POST_PROCESSING_AREVA };
static void SfMarkerLoop_postprocessing_revai(SfMarkerLoop *self) { POST_PROCESSING_REVAI };
static void SfMarkerLoop_postprocessing_revaa(SfMarkerLoop *self) { POST_PROCESSING_REVAA };
static void SfMarkerLoop_postprocessing_revareva(SfMarkerLoop *self) { POST_PROCESSING_REVAREVA };

static void
SfMarkerLoop_setProcMode(SfMarkerLoop *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = SfMarkerLoop_postprocessing_revareva;
            break;
    }
}

static void
SfMarkerLoop_compute_next_data_frame(SfMarkerLoop *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = SfMarkerLooper_getSamplesBuffer((SfMarkerLooper *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
SfMarkerLoop_traverse(SfMarkerLoop *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
SfMarkerLoop_clear(SfMarkerLoop *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
SfMarkerLoop_dealloc(SfMarkerLoop* self)
{
    pyo_DEALLOC
    SfMarkerLoop_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
SfMarkerLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    SfMarkerLoop *self;
    self = (SfMarkerLoop *)type->tp_alloc(type, 0);

    self->chnl = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SfMarkerLoop_compute_next_data_frame);
    self->mode_func_ptr = SfMarkerLoop_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (SfMarkerLooper *)maintmp;

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * SfMarkerLoop_getServer(SfMarkerLoop* self) { GET_SERVER };
static PyObject * SfMarkerLoop_getStream(SfMarkerLoop* self) { GET_STREAM };
static PyObject * SfMarkerLoop_setMul(SfMarkerLoop *self, PyObject *arg) { SET_MUL };
static PyObject * SfMarkerLoop_setAdd(SfMarkerLoop *self, PyObject *arg) { SET_ADD };
static PyObject * SfMarkerLoop_setSub(SfMarkerLoop *self, PyObject *arg) { SET_SUB };
static PyObject * SfMarkerLoop_setDiv(SfMarkerLoop *self, PyObject *arg) { SET_DIV };

static PyObject * SfMarkerLoop_play(SfMarkerLoop *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SfMarkerLoop_out(SfMarkerLoop *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SfMarkerLoop_stop(SfMarkerLoop *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * SfMarkerLoop_multiply(SfMarkerLoop *self, PyObject *arg) { MULTIPLY };
static PyObject * SfMarkerLoop_inplace_multiply(SfMarkerLoop *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SfMarkerLoop_add(SfMarkerLoop *self, PyObject *arg) { ADD };
static PyObject * SfMarkerLoop_inplace_add(SfMarkerLoop *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SfMarkerLoop_sub(SfMarkerLoop *self, PyObject *arg) { SUB };
static PyObject * SfMarkerLoop_inplace_sub(SfMarkerLoop *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SfMarkerLoop_div(SfMarkerLoop *self, PyObject *arg) { DIV };
static PyObject * SfMarkerLoop_inplace_div(SfMarkerLoop *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef SfMarkerLoop_members[] = {
    {"server", T_OBJECT_EX, offsetof(SfMarkerLoop, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SfMarkerLoop, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(SfMarkerLoop, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(SfMarkerLoop, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SfMarkerLoop_methods[] = {
    {"getServer", (PyCFunction)SfMarkerLoop_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SfMarkerLoop_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)SfMarkerLoop_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SfMarkerLoop_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SfMarkerLoop_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)SfMarkerLoop_setMul, METH_O, "Sets SfMarkerLoop mul factor."},
    {"setAdd", (PyCFunction)SfMarkerLoop_setAdd, METH_O, "Sets SfMarkerLoop add factor."},
    {"setSub", (PyCFunction)SfMarkerLoop_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)SfMarkerLoop_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SfMarkerLoop_as_number = {
    (binaryfunc)SfMarkerLoop_add,                      /*nb_add*/
    (binaryfunc)SfMarkerLoop_sub,                 /*nb_subtract*/
    (binaryfunc)SfMarkerLoop_multiply,                 /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO               /*nb_divide*/
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
    INITIALIZE_NB_COERCE_ZERO                   /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    INITIALIZE_NB_OCT_ZERO   /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO   /*nb_hex*/
    (binaryfunc)SfMarkerLoop_inplace_add,              /*inplace_add*/
    (binaryfunc)SfMarkerLoop_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)SfMarkerLoop_inplace_multiply,         /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO        /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    (binaryfunc)SfMarkerLoop_div,                       /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    (binaryfunc)SfMarkerLoop_inplace_div,                       /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject SfMarkerLoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.SfMarkerLoop_base",         /*tp_name*/
    sizeof(SfMarkerLoop),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SfMarkerLoop_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &SfMarkerLoop_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "SfMarkerLoop objects. Reads a channel from a soundfile directly from disk.",           /* tp_doc */
    (traverseproc)SfMarkerLoop_traverse,   /* tp_traverse */
    (inquiry)SfMarkerLoop_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SfMarkerLoop_methods,             /* tp_methods */
    SfMarkerLoop_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    SfMarkerLoop_new,                 /* tp_new */
};