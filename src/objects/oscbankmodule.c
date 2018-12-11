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
#include "tablemodule.h"

/*******************/
/***** OscBank ******/
/*******************/

static MYFLT
OscBank_clip(MYFLT x, int size) {
    if (x >= size) {
        x -= (int)(x / size) * size;
    }
    else if (x < 0) {
        x += ((int)(-x / size) + 1) * size;
    }
    return x;
}

typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *spread;
    Stream *spread_stream;
    PyObject *slope;
    Stream *slope_stream;
    PyObject *frndf;
    Stream *frndf_stream;
    PyObject *frnda;
    Stream *frnda_stream;
    PyObject *arndf;
    Stream *arndf_stream;
    PyObject *arnda;
    Stream *arnda_stream;
    int stages;
    int fjit;
    int modebuffer[9];
    MYFLT *pointerPos;
    MYFLT *frequencies;
    MYFLT lastFreq;
    MYFLT lastSpread;
    int lastFjit;
    MYFLT amplitude;
    /* frequency randoms */
    MYFLT ftime;
    MYFLT finc;
    MYFLT *fOldValues;
    MYFLT *fValues;
    MYFLT *fDiffs;
    /* amplitude randoms */
    MYFLT atime;
    MYFLT ainc;
    MYFLT *aOldValues;
    MYFLT *aValues;
    MYFLT *aDiffs;
} OscBank;

static void
OscBank_setFrequencies(OscBank *self, MYFLT freq, MYFLT spread) {
    int i, seed;
    MYFLT rnd;
    MYFLT scl = freq * spread;

    if (self->fjit == 1) {
        seed = pyorand();
        for (i=0; i<self->stages; i++) {
            seed = (seed * 15625 + 1) & 0xFFFF;
            rnd = seed * 1.52587890625e-07 - 0.005 + 1.0;
            self->frequencies[i] = (freq + scl * i) * rnd;
        }
    }
    else {
        for (i=0; i<self->stages; i++) {
            self->frequencies[i] = freq + scl * i;
        }
    }
}

static void
OscBank_pickNewFrnds(OscBank *self, MYFLT frndf, MYFLT frnda) {
    int i, seed;
    self->ftime -= 1.0;
    self->finc = frndf / self->sr * self->bufsize;
    if (frnda < 0)
        frnda = 0.0;
    else if (frnda > 1.0)
        frnda = 1.0;

    seed = pyorand();
    for (i=0; i<self->stages; i++) {
        self->fOldValues[i] = self->fValues[i];
        seed = (seed * 15625 + 1) & 0xFFFF;
        self->fValues[i] = (seed - 0x8000) * 3.0517578125e-05 * frnda * self->frequencies[i];
        self->fDiffs[i] = self->fValues[i] - self->fOldValues[i];
    }
}

static void
OscBank_pickNewArnds(OscBank *self, MYFLT arndf, MYFLT arnda) {
    int i, seed;
    self->atime -= 1.0;
    self->ainc = arndf / self->sr * self->bufsize;
    if (arnda < 0)
        arnda = 0.0;
    else if (arnda > 1.0)
        arnda = 1.0;

    seed = pyorand();
    for (i=0; i<self->stages; i++) {
        self->aOldValues[i] = self->aValues[i];
        seed = (seed * 15625 + 1) & 0xFFFF;
        self->aValues[i] = seed * 1.52587890625e-05 * arnda;
        self->aDiffs[i] = self->aValues[i] - self->aOldValues[i];
    }
}

static void
OscBank_readframes(OscBank *self) {
    MYFLT freq, spread, slope, frndf, frnda, arndf, arnda, amp, modamp, pos, inc, x, y, fpart;
    int i, j, ipart;
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    MYFLT tabscl = size / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }

    if (self->modebuffer[2] == 0)
        freq = PyFloat_AS_DOUBLE(self->freq);
    else
        freq = Stream_getData((Stream *)self->freq_stream)[0];
    if (self->modebuffer[3] == 0)
        spread = PyFloat_AS_DOUBLE(self->spread);
    else
        spread = Stream_getData((Stream *)self->spread_stream)[0];
    if (self->modebuffer[4] == 0)
        slope = PyFloat_AS_DOUBLE(self->slope);
    else
        slope = Stream_getData((Stream *)self->slope_stream)[0];
    if (self->modebuffer[5] == 0)
        frndf = PyFloat_AS_DOUBLE(self->frndf);
    else
        frndf = Stream_getData((Stream *)self->frndf_stream)[0];
    if (self->modebuffer[6] == 0)
        frnda = PyFloat_AS_DOUBLE(self->frnda);
    else
        frnda = Stream_getData((Stream *)self->frnda_stream)[0];
    if (self->modebuffer[7] == 0)
        arndf = PyFloat_AS_DOUBLE(self->arndf);
    else
        arndf = Stream_getData((Stream *)self->arndf_stream)[0];
    if (self->modebuffer[8] == 0)
        arnda = PyFloat_AS_DOUBLE(self->arnda);
    else
        arnda = Stream_getData((Stream *)self->arnda_stream)[0];

    if (freq != self->lastFreq || spread != self->lastSpread) {
        self->lastFreq = freq;
        self->lastSpread = spread;
        OscBank_setFrequencies(self, freq, spread);
    }
    if (self->fjit != self->lastFjit) {
        self->lastFjit = self->fjit;
        OscBank_setFrequencies(self, freq, spread);
        if (self->fjit == 0) {
            for (i=0; i<self->stages; i++) {
                self->pointerPos[i] = 0.0;
            }
        }
    }

    if (frnda == 0.0 && arnda == 0.0) {
        amp = self->amplitude;
        for (j=0; j<self->stages; j++) {
            inc = self->frequencies[j] * tabscl;
            pos = self->pointerPos[j];
            for (i=0; i<self->bufsize; i++) {
                pos = OscBank_clip(pos, size);
                ipart = (int)pos;
                fpart = pos - ipart;
                x = tablelist[ipart];
                y = tablelist[ipart+1];
                self->data[i] += (x + (y - x) * fpart) * amp;
                pos += inc;
            }
            self->pointerPos[j] = pos;
            amp *= slope;
        }
    }
    else if (frnda != 0.0 && arnda != 0.0) {
        if (self->ftime >= 1.0) {
            OscBank_pickNewFrnds(self, frndf, frnda);
        }
        if (self->atime >= 1.0) {
            OscBank_pickNewArnds(self, arndf, arnda);
        }
        amp = self->amplitude;
        for (j=0; j<self->stages; j++) {
            inc = (self->frequencies[j] + (self->fOldValues[j] + self->fDiffs[j] * self->ftime)) * tabscl;
            pos = self->pointerPos[j];
            modamp = (1.0 - arnda) + (self->aOldValues[j] + self->aDiffs[j] * self->atime);
            for (i=0; i<self->bufsize; i++) {
                pos = OscBank_clip(pos, size);
                ipart = (int)pos;
                fpart = pos - ipart;
                x = tablelist[ipart];
                y = tablelist[ipart+1];
                self->data[i] += (x + (y - x) * fpart) * amp * modamp;
                pos += inc;
            }
            self->pointerPos[j] = pos;
            amp *= slope;
        }
        self->ftime += self->finc;
        self->atime += self->ainc;
    }
    else if (frnda != 0.0 && arnda == 0.0) {
        if (self->ftime >= 1.0) {
            OscBank_pickNewFrnds(self, frndf, frnda);
        }
        amp = self->amplitude;
        for (j=0; j<self->stages; j++) {
            inc = (self->frequencies[j] + (self->fOldValues[j] + self->fDiffs[j] * self->ftime)) * tabscl;
            pos = self->pointerPos[j];
            for (i=0; i<self->bufsize; i++) {
                pos = OscBank_clip(pos, size);
                ipart = (int)pos;
                fpart = pos - ipart;
                x = tablelist[ipart];
                y = tablelist[ipart+1];
                self->data[i] += (x + (y - x) * fpart) * amp;
                pos += inc;
            }
            self->pointerPos[j] = pos;
            amp *= slope;
        }
        self->ftime += self->finc;
    }
    else if (frnda == 0.0 && arnda != 0.0) {
        if (self->atime >= 1.0) {
            OscBank_pickNewArnds(self, arndf, arnda);
        }
        amp = self->amplitude;
        for (j=0; j<self->stages; j++) {
            inc = self->frequencies[j] * tabscl;
            pos = self->pointerPos[j];
            modamp = (1.0 - arnda) + (self->aOldValues[j] + self->aDiffs[j] * self->atime);
            for (i=0; i<self->bufsize; i++) {
                pos = OscBank_clip(pos, size);
                ipart = (int)pos;
                fpart = pos - ipart;
                x = tablelist[ipart];
                y = tablelist[ipart+1];
                self->data[i] += (x + (y - x) * fpart) * amp * modamp;
                pos += inc;
            }
            self->pointerPos[j] = pos;
            amp *= slope;
        }
        self->atime += self->ainc;
    }

}

static void OscBank_postprocessing_ii(OscBank *self) { POST_PROCESSING_II };
static void OscBank_postprocessing_ai(OscBank *self) { POST_PROCESSING_AI };
static void OscBank_postprocessing_ia(OscBank *self) { POST_PROCESSING_IA };
static void OscBank_postprocessing_aa(OscBank *self) { POST_PROCESSING_AA };
static void OscBank_postprocessing_ireva(OscBank *self) { POST_PROCESSING_IREVA };
static void OscBank_postprocessing_areva(OscBank *self) { POST_PROCESSING_AREVA };
static void OscBank_postprocessing_revai(OscBank *self) { POST_PROCESSING_REVAI };
static void OscBank_postprocessing_revaa(OscBank *self) { POST_PROCESSING_REVAA };
static void OscBank_postprocessing_revareva(OscBank *self) { POST_PROCESSING_REVAREVA };

static void
OscBank_setProcMode(OscBank *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = OscBank_readframes;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = OscBank_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = OscBank_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = OscBank_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = OscBank_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = OscBank_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = OscBank_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = OscBank_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = OscBank_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = OscBank_postprocessing_revareva;
            break;
    }
}

static void
OscBank_compute_next_data_frame(OscBank *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
OscBank_traverse(OscBank *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->spread);
    Py_VISIT(self->spread_stream);
    Py_VISIT(self->slope);
    Py_VISIT(self->slope_stream);
    Py_VISIT(self->frndf);
    Py_VISIT(self->frndf_stream);
    Py_VISIT(self->frnda);
    Py_VISIT(self->frnda_stream);
    Py_VISIT(self->arndf);
    Py_VISIT(self->arndf_stream);
    Py_VISIT(self->arnda);
    Py_VISIT(self->arnda_stream);
    return 0;
}

static int
OscBank_clear(OscBank *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->spread);
    Py_CLEAR(self->spread_stream);
    Py_CLEAR(self->slope);
    Py_CLEAR(self->slope_stream);
    Py_CLEAR(self->frndf);
    Py_CLEAR(self->frndf_stream);
    Py_CLEAR(self->frnda);
    Py_CLEAR(self->frnda_stream);
    Py_CLEAR(self->arndf);
    Py_CLEAR(self->arndf_stream);
    Py_CLEAR(self->arnda);
    Py_CLEAR(self->arnda_stream);
    return 0;
}

static void
OscBank_dealloc(OscBank* self)
{
    pyo_DEALLOC
    free(self->pointerPos);
    free(self->frequencies);
    free(self->fOldValues);
    free(self->fValues);
    free(self->fDiffs);
    free(self->aOldValues);
    free(self->aValues);
    free(self->aDiffs);
    OscBank_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
OscBank_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *tabletmp, *freqtmp=NULL, *spreadtmp=NULL, *slopetmp=NULL, *frndftmp=NULL, *frndatmp=NULL, *arndftmp=NULL, *arndatmp=NULL, *multmp=NULL, *addtmp=NULL;
    OscBank *self;
    self = (OscBank *)type->tp_alloc(type, 0);

    self->freq = PyFloat_FromDouble(100.0);
    self->spread = PyFloat_FromDouble(1.0);
    self->slope = PyFloat_FromDouble(0.9);
    self->frndf = PyFloat_FromDouble(1.0);
    self->frnda = PyFloat_FromDouble(0.0);
    self->arndf = PyFloat_FromDouble(1.0);
    self->arnda = PyFloat_FromDouble(0.0);
    self->stages = 24;
    self->fjit = 0;
    self->lastFreq = self->lastSpread = -1.0;
    self->lastFjit = -1;
    self->ftime = 1.0;
    self->finc = 0.0;
    self->atime = 1.0;
    self->ainc = 0.0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    self->modebuffer[2] = 0;
    self->modebuffer[3] = 0;
    self->modebuffer[4] = 0;
    self->modebuffer[5] = 0;
    self->modebuffer[6] = 0;
    self->modebuffer[7] = 0;
    self->modebuffer[8] = 0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, OscBank_compute_next_data_frame);
    self->mode_func_ptr = OscBank_setProcMode;

    static char *kwlist[] = {"table", "freq", "spread", "slope", "frndf", "frnda", "arndf", "arnda", "num", "fjit", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOOOiiOO", kwlist, &tabletmp, &freqtmp, &spreadtmp, &slopetmp, &frndftmp, &frndatmp, &arndftmp, &arndatmp, &self->stages, &self->fjit, &multmp, &addtmp))
        Py_RETURN_NONE;

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of OscBank must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (spreadtmp) {
        PyObject_CallMethod((PyObject *)self, "setSpread", "O", spreadtmp);
    }

    if (slopetmp) {
        PyObject_CallMethod((PyObject *)self, "setSlope", "O", slopetmp);
    }

    if (frndftmp) {
        PyObject_CallMethod((PyObject *)self, "setFrndf", "O", frndftmp);
    }
    if (frndatmp) {
        PyObject_CallMethod((PyObject *)self, "setFrnda", "O", frndatmp);
    }

    if (arndftmp) {
        PyObject_CallMethod((PyObject *)self, "setArndf", "O", arndftmp);
    }
    if (arndatmp) {
        PyObject_CallMethod((PyObject *)self, "setArnda", "O", arndatmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    self->pointerPos = (MYFLT *)realloc(self->pointerPos, self->stages * sizeof(MYFLT));
    self->frequencies = (MYFLT *)realloc(self->frequencies, self->stages * sizeof(MYFLT));
    self->fOldValues = (MYFLT *)realloc(self->fOldValues, self->stages * sizeof(MYFLT));
    self->fValues = (MYFLT *)realloc(self->fValues, self->stages * sizeof(MYFLT));
    self->fDiffs = (MYFLT *)realloc(self->fDiffs, self->stages * sizeof(MYFLT));
    self->aOldValues = (MYFLT *)realloc(self->aOldValues, self->stages * sizeof(MYFLT));
    self->aValues = (MYFLT *)realloc(self->aValues, self->stages * sizeof(MYFLT));
    self->aDiffs = (MYFLT *)realloc(self->aDiffs, self->stages * sizeof(MYFLT));

    for (i=0; i<self->stages; i++) {
        self->pointerPos[i] = self->frequencies[i] = self->fOldValues[i] = self->fValues[i] = self->fDiffs[i] = self->aOldValues[i] = self->aValues[i] = self->aDiffs[i] = 0.0;
    }

    self->amplitude = 1. / self->stages;

    Server_generateSeed((Server *)self->server, OSCBANK_ID);

    return (PyObject *)self;
}

static PyObject * OscBank_getServer(OscBank* self) { GET_SERVER };
static PyObject * OscBank_getStream(OscBank* self) { GET_STREAM };
static PyObject * OscBank_setMul(OscBank *self, PyObject *arg) { SET_MUL };
static PyObject * OscBank_setAdd(OscBank *self, PyObject *arg) { SET_ADD };
static PyObject * OscBank_setSub(OscBank *self, PyObject *arg) { SET_SUB };
static PyObject * OscBank_setDiv(OscBank *self, PyObject *arg) { SET_DIV };

static PyObject * OscBank_play(OscBank *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * OscBank_out(OscBank *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * OscBank_stop(OscBank *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * OscBank_multiply(OscBank *self, PyObject *arg) { MULTIPLY };
static PyObject * OscBank_inplace_multiply(OscBank *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * OscBank_add(OscBank *self, PyObject *arg) { ADD };
static PyObject * OscBank_inplace_add(OscBank *self, PyObject *arg) { INPLACE_ADD };
static PyObject * OscBank_sub(OscBank *self, PyObject *arg) { SUB };
static PyObject * OscBank_inplace_sub(OscBank *self, PyObject *arg) { INPLACE_SUB };
static PyObject * OscBank_div(OscBank *self, PyObject *arg) { DIV };
static PyObject * OscBank_inplace_div(OscBank *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
OscBank_getTable(OscBank *self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
OscBank_setTable(OscBank *self, PyObject *arg)
{
    PyObject *tmp;

    ASSERT_ARG_NOT_NULL

    tmp = arg;
    Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setFreq(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->freq);
    if (isNumber == 1) {
        self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
    }
    else {
        self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
        self->modebuffer[2] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setSpread(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->spread);
    if (isNumber == 1) {
        self->spread = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
    }
    else {
        self->spread = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->spread, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->spread_stream);
        self->spread_stream = (Stream *)streamtmp;
        self->modebuffer[3] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setSlope(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->slope);
    if (isNumber == 1) {
        self->slope = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
    }
    else {
        self->slope = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->slope, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->slope_stream);
        self->slope_stream = (Stream *)streamtmp;
        self->modebuffer[4] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setFrndf(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->frndf);
    if (isNumber == 1) {
        self->frndf = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
    }
    else {
        self->frndf = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->frndf, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->frndf_stream);
        self->frndf_stream = (Stream *)streamtmp;
        self->modebuffer[5] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setFrnda(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->frnda);
    if (isNumber == 1) {
        self->frnda = PyNumber_Float(tmp);
        self->modebuffer[6] = 0;
    }
    else {
        self->frnda = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->frnda, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->frnda_stream);
        self->frnda_stream = (Stream *)streamtmp;
        self->modebuffer[6] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setArndf(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->arndf);
    if (isNumber == 1) {
        self->arndf = PyNumber_Float(tmp);
        self->modebuffer[7] = 0;
    }
    else {
        self->arndf = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->arndf, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->arndf_stream);
        self->arndf_stream = (Stream *)streamtmp;
        self->modebuffer[7] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setArnda(OscBank *self, PyObject *arg)
{
    PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

    int isNumber = PyNumber_Check(arg);

    tmp = arg;
    Py_INCREF(tmp);
    Py_DECREF(self->arnda);
    if (isNumber == 1) {
        self->arnda = PyNumber_Float(tmp);
        self->modebuffer[8] = 0;
    }
    else {
        self->arnda = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->arnda, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->arnda_stream);
        self->arnda_stream = (Stream *)streamtmp;
        self->modebuffer[8] = 1;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
OscBank_setFjit(OscBank *self, PyObject *arg)
{
    int isInt = PyInt_Check(arg);

    if (isInt) {
        self->fjit = PyInt_AS_LONG(arg);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef OscBank_members[] = {
    {"server", T_OBJECT_EX, offsetof(OscBank, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(OscBank, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(OscBank, table), 0, "Waveform table."},
    {"freq", T_OBJECT_EX, offsetof(OscBank, freq), 0, "Base frequency in Hertz."},
    {"spread", T_OBJECT_EX, offsetof(OscBank, spread), 0, "Frequencies spreading factor."},
    {"slope", T_OBJECT_EX, offsetof(OscBank, slope), 0, "Amplitude slope factor."},
    {"frndf", T_OBJECT_EX, offsetof(OscBank, frndf), 0, "Frequency of random frequency changes."},
    {"frnda", T_OBJECT_EX, offsetof(OscBank, frnda), 0, "Amplitude of random frequency changes."},
    {"mul", T_OBJECT_EX, offsetof(OscBank, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(OscBank, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef OscBank_methods[] = {
    {"getTable", (PyCFunction)OscBank_getTable, METH_NOARGS, "Returns waveform table object."},
    {"getServer", (PyCFunction)OscBank_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)OscBank_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)OscBank_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)OscBank_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)OscBank_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setTable", (PyCFunction)OscBank_setTable, METH_O, "Sets oscillator table."},
    {"setFreq", (PyCFunction)OscBank_setFreq, METH_O, "Sets base frequency in Hertz."},
    {"setSpread", (PyCFunction)OscBank_setSpread, METH_O, "Sets spreading factor."},
    {"setSlope", (PyCFunction)OscBank_setSlope, METH_O, "Sets oscillators amplitude slope factor."},
    {"setFrndf", (PyCFunction)OscBank_setFrndf, METH_O, "Sets frequency of random frequency changes."},
    {"setFrnda", (PyCFunction)OscBank_setFrnda, METH_O, "Sets amplitude of random frequency changes."},
    {"setArndf", (PyCFunction)OscBank_setArndf, METH_O, "Sets frequency of random amplitude changes."},
    {"setArnda", (PyCFunction)OscBank_setArnda, METH_O, "Sets amplitude of random amplitude changes."},
    {"setFjit", (PyCFunction)OscBank_setFjit, METH_O, "Sets frequencies jitter on/off switch."},
    {"setMul", (PyCFunction)OscBank_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)OscBank_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)OscBank_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)OscBank_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods OscBank_as_number = {
    (binaryfunc)OscBank_add,                         /*nb_add*/
    (binaryfunc)OscBank_sub,                         /*nb_subtract*/
    (binaryfunc)OscBank_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
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
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)OscBank_inplace_add,                 /*inplace_add*/
    (binaryfunc)OscBank_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)OscBank_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO               /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)OscBank_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)OscBank_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject OscBankType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.OscBank_base",                                   /*tp_name*/
    sizeof(OscBank),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)OscBank_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &OscBank_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "OscBank objects. Oscillator bank.",           /* tp_doc */
    (traverseproc)OscBank_traverse,                  /* tp_traverse */
    (inquiry)OscBank_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    OscBank_methods,                                 /* tp_methods */
    OscBank_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    OscBank_new,                                     /* tp_new */
};
