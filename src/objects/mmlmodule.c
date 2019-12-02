/**************************************************************************
 * Copyright 2009-2019 Olivier Belanger                                   *
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
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

static MYFLT MML_DURATIONS[10] = {0.03125, 0.0625, 0.09375, 0.125, 0.1875, 0.25, 0.375, 0.5, 0.75, 1.0};
static MYFLT MML_VOLUMES[101] = {0.0, 1.0000000000000002e-06, 8.000000000000001e-06, 2.6999999999999996e-05, 6.400000000000001e-05, 0.00012500000000000003, 0.00021599999999999996, 0.0003430000000000001, 0.0005120000000000001, 0.0007289999999999999, 0.0010000000000000002, 0.001331, 0.0017279999999999997, 0.002197, 0.0027440000000000008, 0.0033749999999999995, 0.004096000000000001, 0.004913000000000001, 0.0058319999999999995, 0.0068590000000000005, 0.008000000000000002, 0.009260999999999998, 0.010648, 0.012167, 0.013823999999999998, 0.015625, 0.017576, 0.019683000000000003, 0.021952000000000006, 0.024388999999999994, 0.026999999999999996, 0.029790999999999998, 0.032768000000000005, 0.035937000000000004, 0.039304000000000006, 0.04287499999999999, 0.046655999999999996, 0.050653, 0.054872000000000004, 0.059319000000000004, 0.06400000000000002, 0.06892099999999998, 0.07408799999999999, 0.079507, 0.085184, 0.09112500000000001, 0.097336, 0.10382299999999998, 0.11059199999999998, 0.11764899999999999, 0.125, 0.13265100000000002, 0.140608, 0.148877, 0.15746400000000002, 0.16637500000000005, 0.17561600000000005, 0.18519299999999994, 0.19511199999999995, 0.20537899999999998, 0.21599999999999997, 0.226981, 0.23832799999999998, 0.250047, 0.26214400000000004, 0.274625, 0.28749600000000003, 0.30076300000000006, 0.31443200000000004, 0.32850899999999994, 0.3429999999999999, 0.3579109999999999, 0.37324799999999997, 0.38901699999999995, 0.405224, 0.421875, 0.43897600000000003, 0.456533, 0.47455200000000003, 0.49303900000000006, 0.5120000000000001, 0.531441, 0.5513679999999999, 0.5717869999999999, 0.5927039999999999, 0.6141249999999999, 0.636056, 0.658503, 0.681472, 0.7049690000000001, 0.7290000000000001, 0.7535710000000001, 0.778688, 0.8043570000000001, 0.8305839999999999, 0.8573749999999999, 0.8847359999999999, 0.912673, 0.9411919999999999, 0.970299, 1.0};

/****************/
/**** MMLMain *****/
/****************/
typedef struct {
    pyo_audio_HEAD
    PyObject *sequence;
    PyObject *pending;
    PyObject *pitches;
    int to_stop;
    int loop;
    int poly;
    int updateAtEnd;
    Py_ssize_t num_events;
    Py_ssize_t count;
    int voiceCount;
    double sampleToSec;
    double currentTime;
    double currentDuration;
    int currentVolume;
    double currentAmplitude;
    double currentTempo;
    MYFLT currentDivider;
    MYFLT currentX;
    MYFLT currentY;
    MYFLT currentZ;
    double durOfWhole;
    MYFLT *buffer_streams;
    MYFLT *fre_buffer_streams;
    MYFLT *amp_buffer_streams;
    MYFLT *dur_buffer_streams;
    MYFLT *end_buffer_streams;
    MYFLT *x_buffer_streams;
    MYFLT *y_buffer_streams;
    MYFLT *z_buffer_streams;
    MYFLT *curFrequencies;
    MYFLT *curAmplitudes;
    MYFLT *curDurations;
    MYFLT *curXs;
    MYFLT *curYs;
    MYFLT *curZs;
    int octave;
    Py_ssize_t loopStart[128];
    Py_ssize_t loopCount[128];
    Py_ssize_t currentLoop;
} MMLMain;

static void
MMLMain_consume(MMLMain *self, int i) {
    long pos = 0;
    if (self->count == self->num_events) {
        self->count = 0;
        self->end_buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
        if (self->loop == 0) {
            self->buffer_streams[i + self->voiceCount * self->bufsize] = 0.0;
            self->to_stop = 1;
            return;
        }
        if (self->pending != NULL) {
            Py_XDECREF(self->sequence);
            self->sequence = self->pending;
            self->num_events = PyList_Size(self->sequence);
            self->pending = NULL;
        }
    }

    PyObject *first = NULL;
    PyObject *c = NULL;
    PyObject *event = PyList_GetItem(self->sequence, self->count);

    self->count++;

    double duration = self->currentDuration;

    first = PySequence_GetItem(event, 0);

    Py_ssize_t mpos = 0;
    Py_ssize_t pos1 = 0;
    Py_ssize_t pos2 = 0;
    // Handle random values from a set.
    while (PyUnicode_Contains(event, PyUnicode_FromString("?["))) {
        mpos = PyUnicode_Find(event, PyUnicode_FromString("?"), 0, PySequence_Size(event), 1);
        pos1 = PyUnicode_Find(event, PyUnicode_FromString("["), 0, PySequence_Size(event), 1);
        pos2 = PyUnicode_Find(event, PyUnicode_FromString("]"), 0, PySequence_Size(event), 1);
        PyObject *choices = PyUnicode_Split(PySequence_GetSlice(event, pos1+1, pos2), PyUnicode_FromString(","), -1);
        Py_ssize_t chsize = PyList_Size(choices);
        PyObject *chosen = PyList_GetItem(choices, (Py_ssize_t)(RANDOM_UNIFORM * chsize));
        PyObject *head = PySequence_GetSlice(event, 0, mpos);
        PyObject *tail = PySequence_GetSlice(event, pos2+1, PySequence_Size(event));
        event = PyUnicode_Concat(head, chosen);
        event = PyUnicode_Concat(event, tail);
    }

    // Handle random values from a range.
    while (PyUnicode_Contains(event, PyUnicode_FromString("?{"))) {
        mpos = PyUnicode_Find(event, PyUnicode_FromString("?"), 0, PySequence_Size(event), 1);
        pos1 = PyUnicode_Find(event, PyUnicode_FromString("{"), 0, PySequence_Size(event), 1);
        pos2 = PyUnicode_Find(event, PyUnicode_FromString("}"), 0, PySequence_Size(event), 1);
        PyObject *bounds = PyUnicode_Split(PySequence_GetSlice(event, pos1+1, pos2), PyUnicode_FromString(","), -1);
        Py_ssize_t bsize = PyList_Size(bounds);
        MYFLT value, low = 0.0, high = 1.0;
        if (bsize == 1) {
            if (PyUnicode_GET_LENGTH(PyList_GetItem(bounds, 0)) > 0) {
                high = (MYFLT)PyFloat_AsDouble(PyNumber_Float(PyList_GetItem(bounds, 0)));
                value = RANDOM_UNIFORM * high;
            } else {
                value = RANDOM_UNIFORM;
            }
        } else if (bsize >= 2) {
            low = PyFloat_AsDouble(PyNumber_Float(PyList_GetItem(bounds, 0)));
            high = PyFloat_AsDouble(PyNumber_Float(PyList_GetItem(bounds, 1)));
            value = RANDOM_UNIFORM * (high - low) + low;
        }
        char temp[128];
        sprintf(temp, "%f", value); 
        PyObject *val = PyUnicode_FromString(temp);
        PyObject *head = PySequence_GetSlice(event, 0, mpos);
        PyObject *tail = PySequence_GetSlice(event, pos2+1, PySequence_Size(event));
        event = PyUnicode_Concat(head, val);
        event = PyUnicode_Concat(event, tail);
    }

    Py_ssize_t eventSize = PySequence_Size(event);

    // Octave up.
    if (PyUnicode_Compare(first, PyUnicode_FromString("o")) == 0) {
        if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("+")) == 0) {
            if (eventSize > 2) {
                self->octave += 12 * PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 2), 10));
            } else {
                self->octave += 12;
            }
        } else if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("-")) == 0) {
            if (eventSize > 2) {
                self->octave -= 12 * PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 2), 10));
            } else {
                self->octave -= 12;
            }
        } else {
            self->octave = 12 * PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 1), 10));
        }
        MMLMain_consume(self, i);
    }
    // tempo
    else if (PyUnicode_Compare(first, PyUnicode_FromString("t")) == 0) {
        if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("+")) == 0) {
            if (eventSize > 2) {
                self->currentTempo += PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 2, eventSize)));
            } else {
                self->currentTempo += 1.0;
            }
        } else if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("-")) == 0) {
            if (eventSize > 2) {
                self->currentTempo -= PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 2, eventSize)));
            } else {
                self->currentTempo -= 1.0;
            }
        } else {
            self->currentTempo = PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 1, eventSize)));
        }
        self->durOfWhole = 4 * 60.0 / self->currentTempo;
        MMLMain_consume(self, i);
    }
    // volume
    else if (PyUnicode_Compare(first, PyUnicode_FromString("v")) == 0) {
        if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("+")) == 0) {
            if (eventSize > 2) {
                self->currentVolume += PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetSlice(event, 2, eventSize), 10));
            } else {
                self->currentVolume++;
            }
        } else if (PyUnicode_Compare(PySequence_GetItem(event, 1), PyUnicode_FromString("-")) == 0) {
            if (eventSize > 2) {
                self->currentVolume -= PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetSlice(event, 2, eventSize), 10));
            } else {
                self->currentVolume--;
            }
        } else {
            self->currentVolume = (int)PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 1, eventSize)));
        }
        self->currentVolume = self->currentVolume < 0 ? 0 : self->currentVolume > 100 ? 100 : self->currentVolume;
        self->currentAmplitude = MML_VOLUMES[self->currentVolume];
        MMLMain_consume(self, i);
    }
    // divider
    else if (PyUnicode_Compare(first, PyUnicode_FromString("/")) == 0) {
        self->currentDivider = (MYFLT)PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetSlice(event, 1, eventSize), 10));
        MMLMain_consume(self, i);
    }
    // x
    else if (PyUnicode_Compare(first, PyUnicode_FromString("x")) == 0) {
        self->currentX = (MYFLT)PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 1, eventSize)));
        MMLMain_consume(self, i);
    }
    // y
    else if (PyUnicode_Compare(first, PyUnicode_FromString("y")) == 0) {
        self->currentY = (MYFLT)PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 1, eventSize)));
        MMLMain_consume(self, i);
    }
    // z
    else if (PyUnicode_Compare(first, PyUnicode_FromString("z")) == 0) {
        self->currentZ = (MYFLT)PyFloat_AsDouble(PyNumber_Float(PySequence_GetSlice(event, 1, eventSize)));
        MMLMain_consume(self, i);
    }
    // Opening repeat bar
    else if (PyUnicode_Compare(event, PyUnicode_FromString("|:")) == 0) {
        self->currentLoop++;
        self->loopStart[self->currentLoop] = self->count;
        self->loopCount[self->currentLoop] = 0;
        MMLMain_consume(self, i);
    }
    // Closing repeat bar
    else if (eventSize >= 2 && PyUnicode_Compare(PySequence_GetSlice(event, 0, 2), PyUnicode_FromString(":|")) == 0) {
        int repeats;
        if (eventSize == 2) {
            repeats = 2;
        } else {
            repeats = PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetSlice(event, 2, eventSize), 10));
        }
        self->loopCount[self->currentLoop]++;
        if (self->loopCount[self->currentLoop] < repeats) {
            self->count = self->loopStart[self->currentLoop];
        } else {
            self->currentLoop--;
        }
        MMLMain_consume(self, i);
    }
    // rest
    else if (PyUnicode_Compare(first, PyUnicode_FromString("r")) == 0) {
        if (eventSize > 1) {
            pos = PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 1), 10));
            pos = pos < 0 ? 0 : pos > 9 ? 9 : pos;
            duration = self->durOfWhole * MML_DURATIONS[pos] / self->currentDivider;
        }
        self->currentDuration = duration;
        self->curAmplitudes[self->voiceCount] = 0.0;
        self->curDurations[self->voiceCount] = (MYFLT)duration;
        self->curXs[self->voiceCount] = self->currentX;
        self->curYs[self->voiceCount] = self->currentY;
        self->curZs[self->voiceCount] = self->currentZ;
        self->voiceCount++;
        if (self->voiceCount >= self->poly) {
            self->voiceCount = 0;
        }
    }
    // note event.
    else if (PyDict_Contains(self->pitches, PySequence_GetItem(event, 0))) {
        long pitch = PyLong_AsLong(PyDict_GetItem(self->pitches, PySequence_GetItem(event, 0))) + self->octave;
        if (eventSize > 2) {
            c = PySequence_GetItem(event, 1);
            if (PyUnicode_Compare(c, PyUnicode_FromString("+")) == 0) {
                pitch += 1;
            } else if (PyUnicode_Compare(c, PyUnicode_FromString("-")) == 0) {
                pitch -= 1;
            }
            pos = PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 2), 10));
            pos = pos < 0 ? 0 : pos > 9 ? 9 : pos;
            duration = self->durOfWhole * MML_DURATIONS[pos] / self->currentDivider;
        } else if (eventSize > 1) {
            c = PySequence_GetItem(event, 1);
            if (PyUnicode_Compare(c, PyUnicode_FromString("+")) == 0) {
                pitch += 1;
            } else if (PyUnicode_Compare(c, PyUnicode_FromString("-")) == 0) {
                pitch -= 1;
            } else {
                pos = PyLong_AsLong(PyLong_FromUnicodeObject(PySequence_GetItem(event, 1), 10));
                pos = pos < 0 ? 0 : pos > 9 ? 9 : pos;
                duration = self->durOfWhole * MML_DURATIONS[pos] / self->currentDivider;
            }
        }
        self->currentDuration = duration;
        self->curFrequencies[self->voiceCount] = 440.0 * MYPOW(2.0, (pitch - 69) / 12.0);
        self->curAmplitudes[self->voiceCount] = self->currentAmplitude;
        self->curDurations[self->voiceCount] = (MYFLT)duration;
        self->curXs[self->voiceCount] = self->currentX;
        self->curYs[self->voiceCount] = self->currentY;
        self->curZs[self->voiceCount] = self->currentZ;
        self->voiceCount++;
        if (self->voiceCount >= self->poly) {
            self->voiceCount = 0;
        }
    }
}

static void
MMLMain_generate(MMLMain *self) {
    int i, j;

    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
    }

    if (self->num_events == 0) {
        return;
    }

    if (self->to_stop) {
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
        self->to_stop = 0;
        return;
    }

    if (self->currentDuration == -1.0) {
        self->buffer_streams[self->voiceCount * self->bufsize] = 1.0;
        MMLMain_consume(self, 0);
    }

    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= self->currentDuration && self->to_stop == 0) {
            self->currentTime -= self->currentDuration;
            self->buffer_streams[i + self->voiceCount * self->bufsize] = 1.0;
            MMLMain_consume(self, i);
        }

        if (self->to_stop == 0) {
            for (j=0; j<self->poly; j++) {
                self->fre_buffer_streams[i + j * self->bufsize] = self->curFrequencies[j];
                self->dur_buffer_streams[i + j * self->bufsize] = self->curDurations[j];
                self->amp_buffer_streams[i + j * self->bufsize] = self->curAmplitudes[j];
                self->x_buffer_streams[i + j * self->bufsize] = self->curXs[j];
                self->y_buffer_streams[i + j * self->bufsize] = self->curYs[j];
                self->z_buffer_streams[i + j * self->bufsize] = self->curZs[j];
            }
        }
        self->currentTime += self->sampleToSec;
    }
}

MYFLT *
MMLMain_getSamplesBuffer(MMLMain *self)
{
    return (MYFLT *)self->buffer_streams;
}

MYFLT *
MMLMain_getFreqBuffer(MMLMain *self)
{
    return (MYFLT *)self->fre_buffer_streams;
}

MYFLT *
MMLMain_getAmpBuffer(MMLMain *self)
{
    return (MYFLT *)self->amp_buffer_streams;
}

MYFLT *
MMLMain_getDurBuffer(MMLMain *self)
{
    return (MYFLT *)self->dur_buffer_streams;
}

MYFLT *
MMLMain_getEndBuffer(MMLMain *self)
{
    return (MYFLT *)self->end_buffer_streams;
}

MYFLT *
MMLMain_getXBuffer(MMLMain *self)
{
    return (MYFLT *)self->x_buffer_streams;
}

MYFLT *
MMLMain_getYBuffer(MMLMain *self)
{
    return (MYFLT *)self->y_buffer_streams;
}

MYFLT *
MMLMain_getZBuffer(MMLMain *self)
{
    return (MYFLT *)self->z_buffer_streams;
}

static void
MMLMain_setProcMode(MMLMain *self)
{
    self->proc_func_ptr = MMLMain_generate;
}

static void
MMLMain_compute_next_data_frame(MMLMain *self)
{
    (*self->proc_func_ptr)(self);
}

static int
MMLMain_traverse(MMLMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->sequence);
    Py_VISIT(self->pitches);
    if (self->pending != NULL) {
        Py_VISIT(self->pending);
    }
    return 0;
}

static int
MMLMain_clear(MMLMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->sequence);
    Py_CLEAR(self->pitches);
    if (self->pending != NULL) {
        Py_CLEAR(self->pending);
    }
    return 0;
}

static void
MMLMain_dealloc(MMLMain* self)
{
    pyo_DEALLOC
    free(self->buffer_streams);
    free(self->fre_buffer_streams);
    free(self->amp_buffer_streams);
    free(self->dur_buffer_streams);
    free(self->end_buffer_streams);
    free(self->x_buffer_streams);
    free(self->y_buffer_streams);
    free(self->z_buffer_streams);
    free(self->curFrequencies);
    free(self->curAmplitudes);
    free(self->curDurations);
    free(self->curXs);
    free(self->curYs);
    free(self->curZs);
    MMLMain_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MMLMain *self;
    self = (MMLMain *)type->tp_alloc(type, 0);

    self->pitches = PyDict_New();
    PyDict_SetItem(self->pitches, PyUnicode_FromString("c"), PyLong_FromLong(0));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("d"), PyLong_FromLong(2));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("e"), PyLong_FromLong(4));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("f"), PyLong_FromLong(5));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("g"), PyLong_FromLong(7));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("a"), PyLong_FromLong(9));
    PyDict_SetItem(self->pitches, PyUnicode_FromString("b"), PyLong_FromLong(11));

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLMain_compute_next_data_frame);
    self->mode_func_ptr = MMLMain_setProcMode;

    Stream_setStreamActive(self->stream, 0);

    self->pending = NULL;
    self->to_stop = 0;
    self->durOfWhole = 2.0;
    self->voiceCount = 0;
    self->octave = 60;
    self->loop = 0;
    self->updateAtEnd = 0;
    self->num_events = 0;
    self->count = 0;
    self->sampleToSec = 1.0 / self->sr;
    self->currentTime = 0.0;
    self->currentDuration = -1.0;
    self->currentTempo = 120.0;
    self->currentVolume = 50;
    self->currentAmplitude = 0.125;
    self->currentDivider = 1.0;

    static char *kwlist[] = {"loop", "poly", "updateAtEnd", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iii", kwlist, &self->loop, &self->poly, &self->updateAtEnd))
        Py_RETURN_NONE;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->fre_buffer_streams = (MYFLT *)realloc(self->fre_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->amp_buffer_streams = (MYFLT *)realloc(self->amp_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->dur_buffer_streams = (MYFLT *)realloc(self->dur_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->end_buffer_streams = (MYFLT *)realloc(self->end_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->x_buffer_streams = (MYFLT *)realloc(self->x_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->y_buffer_streams = (MYFLT *)realloc(self->y_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    self->z_buffer_streams = (MYFLT *)realloc(self->z_buffer_streams, self->poly * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->poly*self->bufsize); i++) {
        self->buffer_streams[i] = self->fre_buffer_streams[i] = self->amp_buffer_streams[i] = self->dur_buffer_streams[i] = self->end_buffer_streams[i] = 0.0;
        self->x_buffer_streams[i] = self->y_buffer_streams[i] = self->z_buffer_streams[i] = 0.0;
    }

    self->curFrequencies = (MYFLT *)realloc(self->curFrequencies, self->poly * sizeof(MYFLT));
    self->curAmplitudes = (MYFLT *)realloc(self->curAmplitudes, self->poly * sizeof(MYFLT));
    self->curDurations = (MYFLT *)realloc(self->curDurations, self->poly * sizeof(MYFLT));
    self->curXs = (MYFLT *)realloc(self->curXs, self->poly * sizeof(MYFLT));
    self->curYs = (MYFLT *)realloc(self->curYs, self->poly * sizeof(MYFLT));
    self->curZs = (MYFLT *)realloc(self->curZs, self->poly * sizeof(MYFLT));
    for (i=0; i<self->poly; i++) {
        self->curFrequencies[i] = self->curAmplitudes[i] = self->curDurations[i] = self->curXs[i] = self->curYs[i] = self->curZs[i] = 0.0;
    }

    self->currentLoop = 0;
    for (i=0; i<128; i++) {
        self->loopStart[i] = self->loopCount[i] = 0;
    }

    return (PyObject *)self;
}

static PyObject * MMLMain_getServer(MMLMain* self) { GET_SERVER };
static PyObject * MMLMain_getStream(MMLMain* self) { GET_STREAM };

static PyObject * MMLMain_play(MMLMain *self, PyObject *args, PyObject *kwds) {
    self->voiceCount = 0;
    PLAY
};
static PyObject * MMLMain_stop(MMLMain *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
MMLMain_setSequence(MMLMain *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	if (PyList_Check(arg)) {
        tmp = arg;
        Py_INCREF(tmp);
        Py_XDECREF(self->sequence);
        self->sequence = tmp;
        self->num_events = PyList_Size(self->sequence);
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MMLMain_setPending(MMLMain *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	if (PyList_Check(arg)) {
        tmp = arg;
        Py_INCREF(tmp);
        Py_XDECREF(self->pending);
        self->pending = tmp;
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MMLMain_update(MMLMain *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	if (PyList_Check(arg)) {
        tmp = arg;
        Py_INCREF(tmp);
        if (self->updateAtEnd && self->num_events != 0) {
            MMLMain_setPending(self, arg);
        } else {
            MMLMain_setSequence(self, arg);
            self->count = 0;
            self->currentDuration = -1.0;
        }
    }

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef MMLMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLMain, stream), 0, "Stream object."},
    {"sequence", T_OBJECT_EX, offsetof(MMLMain, sequence), 0, "Sequence of events."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLMain_methods[] = {
    {"getServer", (PyCFunction)MMLMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLMain_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MMLMain_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setPending", (PyCFunction)MMLMain_setPending, METH_O, "Sets a new sequence of events."},
    {"setSequence", (PyCFunction)MMLMain_setSequence, METH_O, "Sets a new sequence of events."},
    {"update", (PyCFunction)MMLMain_update, METH_O, "Sets a new sequence of events."},
    {NULL}  /* Sentinel */
};

PyTypeObject MMLMainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLMain_base",         /*tp_name*/
    sizeof(MMLMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLMain_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "MMLMain objects. Read a MML sequence.",           /* tp_doc */
    (traverseproc)MMLMain_traverse,   /* tp_traverse */
    (inquiry)MMLMain_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLMain_methods,             /* tp_methods */
    MMLMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* MML streamer object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MML;

static void MML_postprocessing_ii(MML *self) { POST_PROCESSING_II };
static void MML_postprocessing_ai(MML *self) { POST_PROCESSING_AI };
static void MML_postprocessing_ia(MML *self) { POST_PROCESSING_IA };
static void MML_postprocessing_aa(MML *self) { POST_PROCESSING_AA };
static void MML_postprocessing_ireva(MML *self) { POST_PROCESSING_IREVA };
static void MML_postprocessing_areva(MML *self) { POST_PROCESSING_AREVA };
static void MML_postprocessing_revai(MML *self) { POST_PROCESSING_REVAI };
static void MML_postprocessing_revaa(MML *self) { POST_PROCESSING_REVAA };
static void MML_postprocessing_revareva(MML *self) { POST_PROCESSING_REVAREVA };

static void
MML_setProcMode(MML *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MML_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MML_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MML_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MML_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MML_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MML_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MML_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MML_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MML_postprocessing_revareva;
            break;
    }
}

static void
MML_compute_next_data_frame(MML *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getSamplesBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MML_traverse(MML *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MML_clear(MML *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MML_dealloc(MML* self)
{
    pyo_DEALLOC
    MML_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MML_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MML *self;
    self = (MML *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MML_compute_next_data_frame);
    self->mode_func_ptr = MML_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MML_getServer(MML* self) { GET_SERVER };
static PyObject * MML_getStream(MML* self) { GET_STREAM };
static PyObject * MML_setMul(MML *self, PyObject *arg) { SET_MUL };
static PyObject * MML_setAdd(MML *self, PyObject *arg) { SET_ADD };
static PyObject * MML_setSub(MML *self, PyObject *arg) { SET_SUB };
static PyObject * MML_setDiv(MML *self, PyObject *arg) { SET_DIV };

static PyObject * MML_play(MML *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MML_out(MML *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MML_stop(MML *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MML_multiply(MML *self, PyObject *arg) { MULTIPLY };
static PyObject * MML_inplace_multiply(MML *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MML_add(MML *self, PyObject *arg) { ADD };
static PyObject * MML_inplace_add(MML *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MML_sub(MML *self, PyObject *arg) { SUB };
static PyObject * MML_inplace_sub(MML *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MML_div(MML *self, PyObject *arg) { DIV };
static PyObject * MML_inplace_div(MML *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MML_members[] = {
    {"server", T_OBJECT_EX, offsetof(MML, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MML, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MML, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MML, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MML_methods[] = {
    {"getServer", (PyCFunction)MML_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MML_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MML_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MML_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MML_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MML_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MML_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MML_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MML_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MML_as_number = {
    (binaryfunc)MML_add,                         /*nb_add*/
    (binaryfunc)MML_sub,                         /*nb_subtract*/
    (binaryfunc)MML_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MML_inplace_add,                 /*inplace_add*/
    (binaryfunc)MML_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MML_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MML_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MML_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MML_base",         /*tp_name*/
    sizeof(MML),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MML_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MML_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MML objects. Reads a channel from a MMLMain.",           /* tp_doc */
    (traverseproc)MML_traverse,   /* tp_traverse */
    (inquiry)MML_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MML_methods,             /* tp_methods */
    MML_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MML_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLFreqStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLFreqStream;

static void MMLFreqStream_postprocessing_ii(MMLFreqStream *self) { POST_PROCESSING_II };
static void MMLFreqStream_postprocessing_ai(MMLFreqStream *self) { POST_PROCESSING_AI };
static void MMLFreqStream_postprocessing_ia(MMLFreqStream *self) { POST_PROCESSING_IA };
static void MMLFreqStream_postprocessing_aa(MMLFreqStream *self) { POST_PROCESSING_AA };
static void MMLFreqStream_postprocessing_ireva(MMLFreqStream *self) { POST_PROCESSING_IREVA };
static void MMLFreqStream_postprocessing_areva(MMLFreqStream *self) { POST_PROCESSING_AREVA };
static void MMLFreqStream_postprocessing_revai(MMLFreqStream *self) { POST_PROCESSING_REVAI };
static void MMLFreqStream_postprocessing_revaa(MMLFreqStream *self) { POST_PROCESSING_REVAA };
static void MMLFreqStream_postprocessing_revareva(MMLFreqStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLFreqStream_setProcMode(MMLFreqStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLFreqStream_postprocessing_revareva;
            break;
    }
}

static void
MMLFreqStream_compute_next_data_frame(MMLFreqStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getFreqBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLFreqStream_traverse(MMLFreqStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLFreqStream_clear(MMLFreqStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLFreqStream_dealloc(MMLFreqStream* self)
{
    pyo_DEALLOC
    MMLFreqStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLFreqStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLFreqStream *self;
    self = (MMLFreqStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLFreqStream_compute_next_data_frame);
    self->mode_func_ptr = MMLFreqStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLFreqStream_getServer(MMLFreqStream* self) { GET_SERVER };
static PyObject * MMLFreqStream_getStream(MMLFreqStream* self) { GET_STREAM };
static PyObject * MMLFreqStream_setMul(MMLFreqStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLFreqStream_setAdd(MMLFreqStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLFreqStream_setSub(MMLFreqStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLFreqStream_setDiv(MMLFreqStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLFreqStream_play(MMLFreqStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLFreqStream_out(MMLFreqStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLFreqStream_stop(MMLFreqStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLFreqStream_multiply(MMLFreqStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLFreqStream_inplace_multiply(MMLFreqStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLFreqStream_add(MMLFreqStream *self, PyObject *arg) { ADD };
static PyObject * MMLFreqStream_inplace_add(MMLFreqStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLFreqStream_sub(MMLFreqStream *self, PyObject *arg) { SUB };
static PyObject * MMLFreqStream_inplace_sub(MMLFreqStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLFreqStream_div(MMLFreqStream *self, PyObject *arg) { DIV };
static PyObject * MMLFreqStream_inplace_div(MMLFreqStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLFreqStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLFreqStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLFreqStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLFreqStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLFreqStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLFreqStream_methods[] = {
    {"getServer", (PyCFunction)MMLFreqStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLFreqStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLFreqStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLFreqStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLFreqStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLFreqStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLFreqStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLFreqStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLFreqStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLFreqStream_as_number = {
    (binaryfunc)MMLFreqStream_add,                         /*nb_add*/
    (binaryfunc)MMLFreqStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLFreqStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLFreqStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLFreqStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLFreqStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLFreqStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLFreqStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLFreqStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLFreqStream_base",         /*tp_name*/
    sizeof(MMLFreqStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLFreqStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLFreqStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLFreqStream objects. Reads the current Freq from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLFreqStream_traverse,   /* tp_traverse */
    (inquiry)MMLFreqStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLFreqStream_methods,             /* tp_methods */
    MMLFreqStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLFreqStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLAmpStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLAmpStream;

static void MMLAmpStream_postprocessing_ii(MMLAmpStream *self) { POST_PROCESSING_II };
static void MMLAmpStream_postprocessing_ai(MMLAmpStream *self) { POST_PROCESSING_AI };
static void MMLAmpStream_postprocessing_ia(MMLAmpStream *self) { POST_PROCESSING_IA };
static void MMLAmpStream_postprocessing_aa(MMLAmpStream *self) { POST_PROCESSING_AA };
static void MMLAmpStream_postprocessing_ireva(MMLAmpStream *self) { POST_PROCESSING_IREVA };
static void MMLAmpStream_postprocessing_areva(MMLAmpStream *self) { POST_PROCESSING_AREVA };
static void MMLAmpStream_postprocessing_revai(MMLAmpStream *self) { POST_PROCESSING_REVAI };
static void MMLAmpStream_postprocessing_revaa(MMLAmpStream *self) { POST_PROCESSING_REVAA };
static void MMLAmpStream_postprocessing_revareva(MMLAmpStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLAmpStream_setProcMode(MMLAmpStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLAmpStream_postprocessing_revareva;
            break;
    }
}

static void
MMLAmpStream_compute_next_data_frame(MMLAmpStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getAmpBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLAmpStream_traverse(MMLAmpStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLAmpStream_clear(MMLAmpStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLAmpStream_dealloc(MMLAmpStream* self)
{
    pyo_DEALLOC
    MMLAmpStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLAmpStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLAmpStream *self;
    self = (MMLAmpStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLAmpStream_compute_next_data_frame);
    self->mode_func_ptr = MMLAmpStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLAmpStream_getServer(MMLAmpStream* self) { GET_SERVER };
static PyObject * MMLAmpStream_getStream(MMLAmpStream* self) { GET_STREAM };
static PyObject * MMLAmpStream_setMul(MMLAmpStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLAmpStream_setAdd(MMLAmpStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLAmpStream_setSub(MMLAmpStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLAmpStream_setDiv(MMLAmpStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLAmpStream_play(MMLAmpStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLAmpStream_out(MMLAmpStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLAmpStream_stop(MMLAmpStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLAmpStream_multiply(MMLAmpStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLAmpStream_inplace_multiply(MMLAmpStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLAmpStream_add(MMLAmpStream *self, PyObject *arg) { ADD };
static PyObject * MMLAmpStream_inplace_add(MMLAmpStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLAmpStream_sub(MMLAmpStream *self, PyObject *arg) { SUB };
static PyObject * MMLAmpStream_inplace_sub(MMLAmpStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLAmpStream_div(MMLAmpStream *self, PyObject *arg) { DIV };
static PyObject * MMLAmpStream_inplace_div(MMLAmpStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLAmpStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLAmpStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLAmpStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLAmpStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLAmpStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLAmpStream_methods[] = {
    {"getServer", (PyCFunction)MMLAmpStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLAmpStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLAmpStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLAmpStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLAmpStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLAmpStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLAmpStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLAmpStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLAmpStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLAmpStream_as_number = {
    (binaryfunc)MMLAmpStream_add,                         /*nb_add*/
    (binaryfunc)MMLAmpStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLAmpStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLAmpStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLAmpStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLAmpStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLAmpStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLAmpStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLAmpStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLAmpStream_base",         /*tp_name*/
    sizeof(MMLAmpStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLAmpStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLAmpStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLAmpStream objects. Reads a amplitude channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLAmpStream_traverse,   /* tp_traverse */
    (inquiry)MMLAmpStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLAmpStream_methods,             /* tp_methods */
    MMLAmpStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLAmpStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLDurStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLDurStream;

static void MMLDurStream_postprocessing_ii(MMLDurStream *self) { POST_PROCESSING_II };
static void MMLDurStream_postprocessing_ai(MMLDurStream *self) { POST_PROCESSING_AI };
static void MMLDurStream_postprocessing_ia(MMLDurStream *self) { POST_PROCESSING_IA };
static void MMLDurStream_postprocessing_aa(MMLDurStream *self) { POST_PROCESSING_AA };
static void MMLDurStream_postprocessing_ireva(MMLDurStream *self) { POST_PROCESSING_IREVA };
static void MMLDurStream_postprocessing_areva(MMLDurStream *self) { POST_PROCESSING_AREVA };
static void MMLDurStream_postprocessing_revai(MMLDurStream *self) { POST_PROCESSING_REVAI };
static void MMLDurStream_postprocessing_revaa(MMLDurStream *self) { POST_PROCESSING_REVAA };
static void MMLDurStream_postprocessing_revareva(MMLDurStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLDurStream_setProcMode(MMLDurStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLDurStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLDurStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLDurStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLDurStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLDurStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLDurStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLDurStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLDurStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLDurStream_postprocessing_revareva;
            break;
    }
}

static void
MMLDurStream_compute_next_data_frame(MMLDurStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getDurBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLDurStream_traverse(MMLDurStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLDurStream_clear(MMLDurStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLDurStream_dealloc(MMLDurStream* self)
{
    pyo_DEALLOC
    MMLDurStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLDurStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLDurStream *self;
    self = (MMLDurStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLDurStream_compute_next_data_frame);
    self->mode_func_ptr = MMLDurStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLDurStream_getServer(MMLDurStream* self) { GET_SERVER };
static PyObject * MMLDurStream_getStream(MMLDurStream* self) { GET_STREAM };
static PyObject * MMLDurStream_setMul(MMLDurStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLDurStream_setAdd(MMLDurStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLDurStream_setSub(MMLDurStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLDurStream_setDiv(MMLDurStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLDurStream_play(MMLDurStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLDurStream_out(MMLDurStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLDurStream_stop(MMLDurStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLDurStream_multiply(MMLDurStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLDurStream_inplace_multiply(MMLDurStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLDurStream_add(MMLDurStream *self, PyObject *arg) { ADD };
static PyObject * MMLDurStream_inplace_add(MMLDurStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLDurStream_sub(MMLDurStream *self, PyObject *arg) { SUB };
static PyObject * MMLDurStream_inplace_sub(MMLDurStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLDurStream_div(MMLDurStream *self, PyObject *arg) { DIV };
static PyObject * MMLDurStream_inplace_div(MMLDurStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLDurStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLDurStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLDurStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLDurStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLDurStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLDurStream_methods[] = {
    {"getServer", (PyCFunction)MMLDurStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLDurStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLDurStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLDurStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLDurStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLDurStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLDurStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLDurStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLDurStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLDurStream_as_number = {
    (binaryfunc)MMLDurStream_add,                         /*nb_add*/
    (binaryfunc)MMLDurStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLDurStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLDurStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLDurStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLDurStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLDurStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLDurStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLDurStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLDurStream_base",         /*tp_name*/
    sizeof(MMLDurStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLDurStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLDurStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLDurStream objects. Reads a duration channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLDurStream_traverse,   /* tp_traverse */
    (inquiry)MMLDurStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLDurStream_methods,             /* tp_methods */
    MMLDurStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLDurStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLEndStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLEndStream;

static void MMLEndStream_postprocessing_ii(MMLEndStream *self) { POST_PROCESSING_II };
static void MMLEndStream_postprocessing_ai(MMLEndStream *self) { POST_PROCESSING_AI };
static void MMLEndStream_postprocessing_ia(MMLEndStream *self) { POST_PROCESSING_IA };
static void MMLEndStream_postprocessing_aa(MMLEndStream *self) { POST_PROCESSING_AA };
static void MMLEndStream_postprocessing_ireva(MMLEndStream *self) { POST_PROCESSING_IREVA };
static void MMLEndStream_postprocessing_areva(MMLEndStream *self) { POST_PROCESSING_AREVA };
static void MMLEndStream_postprocessing_revai(MMLEndStream *self) { POST_PROCESSING_REVAI };
static void MMLEndStream_postprocessing_revaa(MMLEndStream *self) { POST_PROCESSING_REVAA };
static void MMLEndStream_postprocessing_revareva(MMLEndStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLEndStream_setProcMode(MMLEndStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLEndStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLEndStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLEndStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLEndStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLEndStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLEndStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLEndStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLEndStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLEndStream_postprocessing_revareva;
            break;
    }
}

static void
MMLEndStream_compute_next_data_frame(MMLEndStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getEndBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLEndStream_traverse(MMLEndStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLEndStream_clear(MMLEndStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLEndStream_dealloc(MMLEndStream* self)
{
    pyo_DEALLOC
    MMLEndStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLEndStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLEndStream *self;
    self = (MMLEndStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLEndStream_compute_next_data_frame);
    self->mode_func_ptr = MMLEndStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLEndStream_getServer(MMLEndStream* self) { GET_SERVER };
static PyObject * MMLEndStream_getStream(MMLEndStream* self) { GET_STREAM };
static PyObject * MMLEndStream_setMul(MMLEndStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLEndStream_setAdd(MMLEndStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLEndStream_setSub(MMLEndStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLEndStream_setDiv(MMLEndStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLEndStream_play(MMLEndStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLEndStream_out(MMLEndStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLEndStream_stop(MMLEndStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLEndStream_multiply(MMLEndStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLEndStream_inplace_multiply(MMLEndStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLEndStream_add(MMLEndStream *self, PyObject *arg) { ADD };
static PyObject * MMLEndStream_inplace_add(MMLEndStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLEndStream_sub(MMLEndStream *self, PyObject *arg) { SUB };
static PyObject * MMLEndStream_inplace_sub(MMLEndStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLEndStream_div(MMLEndStream *self, PyObject *arg) { DIV };
static PyObject * MMLEndStream_inplace_div(MMLEndStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLEndStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLEndStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLEndStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLEndStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLEndStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLEndStream_methods[] = {
    {"getServer", (PyCFunction)MMLEndStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLEndStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLEndStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLEndStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLEndStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLEndStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLEndStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLEndStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLEndStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLEndStream_as_number = {
    (binaryfunc)MMLEndStream_add,                         /*nb_add*/
    (binaryfunc)MMLEndStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLEndStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLEndStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLEndStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLEndStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLEndStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLEndStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLEndStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLEndStream_base",         /*tp_name*/
    sizeof(MMLEndStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLEndStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLEndStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLEndStream objects. Reads a duration channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLEndStream_traverse,   /* tp_traverse */
    (inquiry)MMLEndStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLEndStream_methods,             /* tp_methods */
    MMLEndStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLEndStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLXStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLXStream;

static void MMLXStream_postprocessing_ii(MMLXStream *self) { POST_PROCESSING_II };
static void MMLXStream_postprocessing_ai(MMLXStream *self) { POST_PROCESSING_AI };
static void MMLXStream_postprocessing_ia(MMLXStream *self) { POST_PROCESSING_IA };
static void MMLXStream_postprocessing_aa(MMLXStream *self) { POST_PROCESSING_AA };
static void MMLXStream_postprocessing_ireva(MMLXStream *self) { POST_PROCESSING_IREVA };
static void MMLXStream_postprocessing_areva(MMLXStream *self) { POST_PROCESSING_AREVA };
static void MMLXStream_postprocessing_revai(MMLXStream *self) { POST_PROCESSING_REVAI };
static void MMLXStream_postprocessing_revaa(MMLXStream *self) { POST_PROCESSING_REVAA };
static void MMLXStream_postprocessing_revareva(MMLXStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLXStream_setProcMode(MMLXStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLXStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLXStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLXStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLXStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLXStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLXStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLXStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLXStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLXStream_postprocessing_revareva;
            break;
    }
}

static void
MMLXStream_compute_next_data_frame(MMLXStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getXBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLXStream_traverse(MMLXStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLXStream_clear(MMLXStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLXStream_dealloc(MMLXStream* self)
{
    pyo_DEALLOC
    MMLXStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLXStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLXStream *self;
    self = (MMLXStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLXStream_compute_next_data_frame);
    self->mode_func_ptr = MMLXStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLXStream_getServer(MMLXStream* self) { GET_SERVER };
static PyObject * MMLXStream_getStream(MMLXStream* self) { GET_STREAM };
static PyObject * MMLXStream_setMul(MMLXStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLXStream_setAdd(MMLXStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLXStream_setSub(MMLXStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLXStream_setDiv(MMLXStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLXStream_play(MMLXStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLXStream_out(MMLXStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLXStream_stop(MMLXStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLXStream_multiply(MMLXStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLXStream_inplace_multiply(MMLXStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLXStream_add(MMLXStream *self, PyObject *arg) { ADD };
static PyObject * MMLXStream_inplace_add(MMLXStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLXStream_sub(MMLXStream *self, PyObject *arg) { SUB };
static PyObject * MMLXStream_inplace_sub(MMLXStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLXStream_div(MMLXStream *self, PyObject *arg) { DIV };
static PyObject * MMLXStream_inplace_div(MMLXStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLXStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLXStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLXStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLXStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLXStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLXStream_methods[] = {
    {"getServer", (PyCFunction)MMLXStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLXStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLXStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLXStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLXStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLXStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLXStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLXStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLXStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLXStream_as_number = {
    (binaryfunc)MMLXStream_add,                         /*nb_add*/
    (binaryfunc)MMLXStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLXStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLXStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLXStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLXStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLXStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLXStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLXStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLXStream_base",         /*tp_name*/
    sizeof(MMLXStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLXStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLXStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLXStream objects. Reads a Xation channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLXStream_traverse,   /* tp_traverse */
    (inquiry)MMLXStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLXStream_methods,             /* tp_methods */
    MMLXStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLXStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLYStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLYStream;

static void MMLYStream_postprocessing_ii(MMLYStream *self) { POST_PROCESSING_II };
static void MMLYStream_postprocessing_ai(MMLYStream *self) { POST_PROCESSING_AI };
static void MMLYStream_postprocessing_ia(MMLYStream *self) { POST_PROCESSING_IA };
static void MMLYStream_postprocessing_aa(MMLYStream *self) { POST_PROCESSING_AA };
static void MMLYStream_postprocessing_ireva(MMLYStream *self) { POST_PROCESSING_IREVA };
static void MMLYStream_postprocessing_areva(MMLYStream *self) { POST_PROCESSING_AREVA };
static void MMLYStream_postprocessing_revai(MMLYStream *self) { POST_PROCESSING_REVAI };
static void MMLYStream_postprocessing_revaa(MMLYStream *self) { POST_PROCESSING_REVAA };
static void MMLYStream_postprocessing_revareva(MMLYStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLYStream_setProcMode(MMLYStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLYStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLYStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLYStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLYStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLYStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLYStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLYStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLYStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLYStream_postprocessing_revareva;
            break;
    }
}

static void
MMLYStream_compute_next_data_frame(MMLYStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getYBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLYStream_traverse(MMLYStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLYStream_clear(MMLYStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLYStream_dealloc(MMLYStream* self)
{
    pyo_DEALLOC
    MMLYStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLYStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLYStream *self;
    self = (MMLYStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLYStream_compute_next_data_frame);
    self->mode_func_ptr = MMLYStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLYStream_getServer(MMLYStream* self) { GET_SERVER };
static PyObject * MMLYStream_getStream(MMLYStream* self) { GET_STREAM };
static PyObject * MMLYStream_setMul(MMLYStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLYStream_setAdd(MMLYStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLYStream_setSub(MMLYStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLYStream_setDiv(MMLYStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLYStream_play(MMLYStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLYStream_out(MMLYStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLYStream_stop(MMLYStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLYStream_multiply(MMLYStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLYStream_inplace_multiply(MMLYStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLYStream_add(MMLYStream *self, PyObject *arg) { ADD };
static PyObject * MMLYStream_inplace_add(MMLYStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLYStream_sub(MMLYStream *self, PyObject *arg) { SUB };
static PyObject * MMLYStream_inplace_sub(MMLYStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLYStream_div(MMLYStream *self, PyObject *arg) { DIV };
static PyObject * MMLYStream_inplace_div(MMLYStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLYStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLYStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLYStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLYStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLYStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLYStream_methods[] = {
    {"getServer", (PyCFunction)MMLYStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLYStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLYStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLYStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLYStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLYStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLYStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLYStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLYStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLYStream_as_number = {
    (binaryfunc)MMLYStream_add,                         /*nb_add*/
    (binaryfunc)MMLYStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLYStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLYStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLYStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLYStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLYStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLYStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLYStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLYStream_base",         /*tp_name*/
    sizeof(MMLYStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLYStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLYStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLYStream objects. Reads a Yation channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLYStream_traverse,   /* tp_traverse */
    (inquiry)MMLYStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLYStream_methods,             /* tp_methods */
    MMLYStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLYStream_new,                 /* tp_new */
};

/************************************************************************************************/
/* MMLZStream object per channel */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MMLMain *mainPlayer;
    int chnl;
    int modebuffer[2];
} MMLZStream;

static void MMLZStream_postprocessing_ii(MMLZStream *self) { POST_PROCESSING_II };
static void MMLZStream_postprocessing_ai(MMLZStream *self) { POST_PROCESSING_AI };
static void MMLZStream_postprocessing_ia(MMLZStream *self) { POST_PROCESSING_IA };
static void MMLZStream_postprocessing_aa(MMLZStream *self) { POST_PROCESSING_AA };
static void MMLZStream_postprocessing_ireva(MMLZStream *self) { POST_PROCESSING_IREVA };
static void MMLZStream_postprocessing_areva(MMLZStream *self) { POST_PROCESSING_AREVA };
static void MMLZStream_postprocessing_revai(MMLZStream *self) { POST_PROCESSING_REVAI };
static void MMLZStream_postprocessing_revaa(MMLZStream *self) { POST_PROCESSING_REVAA };
static void MMLZStream_postprocessing_revareva(MMLZStream *self) { POST_PROCESSING_REVAREVA };

static void
MMLZStream_setProcMode(MMLZStream *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = MMLZStream_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = MMLZStream_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = MMLZStream_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = MMLZStream_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = MMLZStream_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = MMLZStream_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = MMLZStream_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = MMLZStream_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = MMLZStream_postprocessing_revareva;
            break;
    }
}

static void
MMLZStream_compute_next_data_frame(MMLZStream *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = MMLMain_getZBuffer((MMLMain *)self->mainPlayer);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }
    (*self->muladd_func_ptr)(self);
}

static int
MMLZStream_traverse(MMLZStream *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainPlayer);
    return 0;
}

static int
MMLZStream_clear(MMLZStream *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainPlayer);
    return 0;
}

static void
MMLZStream_dealloc(MMLZStream* self)
{
    pyo_DEALLOC
    MMLZStream_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MMLZStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *maintmp=NULL;
    MMLZStream *self;
    self = (MMLZStream *)type->tp_alloc(type, 0);

    self->chnl = 0;
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MMLZStream_compute_next_data_frame);
    self->mode_func_ptr = MMLZStream_setProcMode;

    static char *kwlist[] = {"mainPlayer", "chnl", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &maintmp, &self->chnl))
        Py_RETURN_NONE;

    Py_XDECREF(self->mainPlayer);
    Py_INCREF(maintmp);
    self->mainPlayer = (MMLMain *)maintmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * MMLZStream_getServer(MMLZStream* self) { GET_SERVER };
static PyObject * MMLZStream_getStream(MMLZStream* self) { GET_STREAM };
static PyObject * MMLZStream_setMul(MMLZStream *self, PyObject *arg) { SET_MUL };
static PyObject * MMLZStream_setAdd(MMLZStream *self, PyObject *arg) { SET_ADD };
static PyObject * MMLZStream_setSub(MMLZStream *self, PyObject *arg) { SET_SUB };
static PyObject * MMLZStream_setDiv(MMLZStream *self, PyObject *arg) { SET_DIV };

static PyObject * MMLZStream_play(MMLZStream *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MMLZStream_out(MMLZStream *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * MMLZStream_stop(MMLZStream *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * MMLZStream_multiply(MMLZStream *self, PyObject *arg) { MULTIPLY };
static PyObject * MMLZStream_inplace_multiply(MMLZStream *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * MMLZStream_add(MMLZStream *self, PyObject *arg) { ADD };
static PyObject * MMLZStream_inplace_add(MMLZStream *self, PyObject *arg) { INPLACE_ADD };
static PyObject * MMLZStream_sub(MMLZStream *self, PyObject *arg) { SUB };
static PyObject * MMLZStream_inplace_sub(MMLZStream *self, PyObject *arg) { INPLACE_SUB };
static PyObject * MMLZStream_div(MMLZStream *self, PyObject *arg) { DIV };
static PyObject * MMLZStream_inplace_div(MMLZStream *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef MMLZStream_members[] = {
    {"server", T_OBJECT_EX, offsetof(MMLZStream, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MMLZStream, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(MMLZStream, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(MMLZStream, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MMLZStream_methods[] = {
    {"getServer", (PyCFunction)MMLZStream_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MMLZStream_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)MMLZStream_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)MMLZStream_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)MMLZStream_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)MMLZStream_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)MMLZStream_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)MMLZStream_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)MMLZStream_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods MMLZStream_as_number = {
    (binaryfunc)MMLZStream_add,                         /*nb_add*/
    (binaryfunc)MMLZStream_sub,                         /*nb_subtract*/
    (binaryfunc)MMLZStream_multiply,                    /*nb_multiply*/
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
    (binaryfunc)MMLZStream_inplace_add,                 /*inplace_add*/
    (binaryfunc)MMLZStream_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)MMLZStream_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)MMLZStream_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)MMLZStream_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject MMLZStreamType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MMLZStream_base",         /*tp_name*/
    sizeof(MMLZStream),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MMLZStream_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    &MMLZStream_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MMLZStream objects. Reads a Zation channel from a MMLMain object.",           /* tp_doc */
    (traverseproc)MMLZStream_traverse,   /* tp_traverse */
    (inquiry)MMLZStream_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MMLZStream_methods,             /* tp_methods */
    MMLZStream_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MMLZStream_new,                 /* tp_new */
};
