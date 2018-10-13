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
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#include "py2to3.h"
#include "structmember.h"
#include "sndfile.h"
#include "streammodule.h"
#include "pyomodule.h"
#include "servermodule.h"

#ifdef USE_PORTAUDIO
#include "ad_portaudio.h"
#else
int Server_pa_init(Server *self) { return -10; };
int Server_pa_deinit(Server *self) { return 0; };
int Server_pa_start(Server *self) { return 0; };
int Server_pa_stop(Server *self) { return 0; };
#endif

#ifdef USE_JACK
#include "ad_jack.h"
#else
int Server_jack_init(Server *self) { return -10; };
int Server_jack_deinit(Server *self) { return 0; };
int Server_jack_start(Server *self) { return 0; };
int Server_jack_stop(Server *self) { return 0; };
int jack_input_port_set_names(Server *self) { return 0; };
int jack_output_port_set_names(Server *self) { return 0; };
int jack_midi_input_port_set_name(Server *self) { return 0; };
int jack_midi_output_port_set_name(Server *self) { return 0; };
void jack_noteout(Server *self, int pit, int vel, int chan, long timestamp) {};
void jack_afterout(Server *self, int pit, int vel, int chan, long timestamp) {};
void jack_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp) {};
void jack_programout(Server *self, int value, int chan, long timestamp) {};
void jack_pressout(Server *self, int value, int chan, long timestamp) {};
void jack_bendout(Server *self, int value, int chan, long timestamp) {};
void jack_makenote(Server *self, int pit, int vel, int dur, int chan) {};

#endif

#ifdef USE_COREAUDIO
#include "ad_coreaudio.h"
#else
int Server_coreaudio_init(Server *self) { return -10; };
int Server_coreaudio_deinit(Server *self) { return 0; };
int Server_coreaudio_start(Server *self) { return 0; };
int Server_coreaudio_stop(Server *self) { return 0; };
#endif

#ifdef USE_PORTMIDI
#include "md_portmidi.h"
#else
void portmidiGetEvents(Server *self) {};
int Server_pm_init(Server *self) { return -10; };
int Server_pm_deinit(Server *self) { return 0; };
void pm_noteout(Server *self, int pit, int vel, int chan, long timestamp) {};
void pm_afterout(Server *self, int pit, int vel, int chan, long timestamp) {};
void pm_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp) {};
void pm_programout(Server *self, int value, int chan, long timestamp) {};
void pm_pressout(Server *self, int value, int chan, long timestamp) {};
void pm_bendout(Server *self, int value, int chan, long timestamp) {};
void pm_sysexout(Server *self, unsigned char *msg, long timestamp) {};
void pm_makenote(Server *self, int pit, int vel, int dur, int chan) {};
long pm_get_current_time() { return 0; };
#endif

/** Array of Server objects. **/
/******************************/

#define MAX_NBR_SERVER 256

static Server *my_server[MAX_NBR_SERVER];
static int serverID = 0;

/* Function called by any new pyo object to get a pointer to the current server. */
PyObject * PyServer_get_server() { return (PyObject *)my_server[serverID]; };

/** Random generator and object seeds. **/
/****************************************/

#define num_rnd_objs 29

int rnd_objs_count[num_rnd_objs] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int rnd_objs_mult[num_rnd_objs] = {1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,2063,2069,
                                   2081,2083,2087,2089,2099,2111,2113,2129,2131,2137,2141,2143,
                                   2153,2161,2179,2203,2207};

/* Linear congruential pseudo-random generator. */
static unsigned int PYO_RAND_SEED = 1u;
unsigned int pyorand() {
    PYO_RAND_SEED = (PYO_RAND_SEED * 1664525 + 1013904223) % PYO_RAND_MAX;
    return PYO_RAND_SEED;
}

/** Logging levels. **/
/*********************/

/* Errors should indicate failure to execute a request. */
void
Server_error(Server *self, char * format, ...)
{
    if (self->verbosity & 1) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        PySys_WriteStdout("Pyo error: %s", buffer);
    }
}

/* Messages should print useful or relevant information, or 
   information requested by the user. */
void
Server_message(Server *self, char * format, ...)
{
    if (self->verbosity & 2) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        PySys_WriteStdout("Pyo message: %s", buffer);
    }
}

/* Warnings should be used when an unexpected or unusual 
   choice was made by pyo. */
void
Server_warning(Server *self, char * format, ...)
{
#ifndef NO_MESSAGES
    if (self->verbosity & 4) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);
        PySys_WriteStdout("Pyo warning: %s", buffer);
    }
#endif
}

/* Debug messages should print internal information which 
   might be necessary for debugging internal conditions. */
void
Server_debug(Server *self, char * format, ...)
{
    if (self->verbosity & 8) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        PySys_WriteStdout("Pyo debug: %s", buffer);
    }
}

/** Offline server. **/
/*********************/

static int
offline_process_block(Server *arg)
{
    Server *server = (Server *) arg;
    Server_process_buffers(server);
    return 0;
}

int Server_offline_init(Server *self) { return 0; };
int Server_offline_deinit(Server *self) { return 0; };

void
*Server_offline_thread(void *arg)
{
    int numBlocks;
    Server *self;
    self = (Server *)arg;

    PyGILState_STATE s = PyGILState_Ensure();
    if (self->recdur < 0) {
        Server_error(self, "Duration must be specified for Offline Server (see Server.recordOptions).");
    }
    else {
        Server_message(self, "Offline Server rendering file %s dur=%f\n", self->recpath, self->recdur);
        numBlocks = ceil(self->recdur * self->samplingRate/self->bufferSize);
        Server_debug(self,"Offline Server rendering, number of blocks = %i\n", numBlocks);
        Server_start_rec_internal(self, self->recpath);
        while (numBlocks-- > 0 && self->server_stopped == 0) {
            offline_process_block((Server *) self);
        }
        self->server_started = 0;
        self->record = 0;
        sf_close(self->recfile);
        Server_message(self, "Offline Server rendering finished.\n");
    }
    PyGILState_Release(s);

    return NULL;
}

int
Server_offline_nb_start(Server *self)
{
    pthread_t offthread;
    pthread_create(&offthread, NULL, Server_offline_thread, self);
    return 0;
}

int
Server_offline_start(Server *self)
{
    int numBlocks;

    if (self->recdur < 0) {
        Server_error(self, "Duration must be specified for Offline Server (see Server.recordOptions).");
        return -1;
    }
    Server_message(self, "Offline Server rendering file %s dur=%f\n", self->recpath, self->recdur);
    numBlocks = ceil(self->recdur * self->samplingRate/self->bufferSize);
    Server_debug(self,"Offline Server rendering, number of blocks = %i\n", numBlocks);
    Server_start_rec_internal(self, self->recpath);
    while (numBlocks-- > 0 && self->server_stopped == 0) {
        offline_process_block((Server *) self);
    }
    self->server_started = 0;
    self->server_stopped = 1;
    self->record = 0;
    sf_close(self->recfile);
    Server_message(self, "Offline Server rendering finished.\n");
    return 0;
}

int
Server_offline_stop(Server *self)
{
    self->server_stopped = 1;
    return 0;
}

/** Embedded server. **/
/**********************/

int Server_embedded_init(Server *self) { return 0; };
int Server_embedded_deinit(Server *self) { return 0; };

/* interleaved embedded callback */
int
Server_embedded_i_start(Server *self)
{
    Server_process_buffers(self);
    self->midi_count = 0;
    return 0;
}

int
Server_embedded_i_startIdx(int idx)
{
    Server_embedded_i_start(my_server[idx]);
    return 0;
}

/* non-interleaved embedded callback */
int
Server_embedded_ni_start(Server *self)
{
    int i, j;
    float out[self->bufferSize * self->nchnls];

    Server_process_buffers(self);

    for (i=0; i<(self->bufferSize*self->nchnls); i++){
        out[i] = self->output_buffer[i];
    }

    for (i=0; i<self->bufferSize; i++) {
        for (j=0; j<self->nchnls; j++) {
            self->output_buffer[(j*self->bufferSize)+i] = out[(i*self->nchnls)+j];
        }
    }
    self->midi_count = 0;
    return 0;
}

int
Server_embedded_ni_startIdx(int idx)
{
    Server_embedded_ni_start(my_server[idx]);
    return 0;
}

void
*Server_embedded_thread(void *arg)
{
    Server *self;
    self = (Server *)arg;

    Server_process_buffers(self);
    self->midi_count = 0;

    return NULL;
}

int
Server_embedded_nb_start(Server *self)
{
    pthread_t offthread;
    pthread_create(&offthread, NULL, Server_embedded_thread, self);
    return 0;
}

int
Server_embedded_stop(Server *self)
{
    self->server_started = 0;
    self->server_stopped = 1;
    return 0;
}

/** Main Processing functions. **/
/********************************/

void
Server_process_buffers(Server *server)
{
    //clock_t begin = clock();

    float *out = server->output_buffer;
    MYFLT buffer[server->nchnls][server->bufferSize];
    int i, j, chnl, nchnls = server->nchnls;
    MYFLT amp = server->amp;
    Stream *stream_tmp;
    MYFLT *data;

    memset(&buffer, 0, sizeof(buffer));

    /* This is the biggest bottle-neck of the callback. Don't know
       how (or if possible) to improve GIL acquire/release.
    */
    PyGILState_STATE s = PyGILState_Ensure();

    if (server->elapsedSamples == 0)
        server->midi_time_offset = pm_get_current_time();

    if (server->CALLBACK != NULL)
        PyObject_Call((PyObject *)server->CALLBACK, PyTuple_New(0), NULL);

    for (i=0; i<server->stream_count; i++) {
        stream_tmp = (Stream *)PyList_GET_ITEM(server->streams, i);
        if (Stream_getStreamActive(stream_tmp) == 1) {
            Stream_callFunction(stream_tmp);
            if (Stream_getStreamToDac(stream_tmp) != 0) {
                data = Stream_getData(stream_tmp);
                chnl = Stream_getStreamChnl(stream_tmp);
                for (j=0; j < server->bufferSize; j++) {
                    buffer[chnl][j] += *data++;
                }
            }
            if (Stream_getDuration(stream_tmp) != 0) {
                Stream_IncrementDurationCount(stream_tmp);
            }
        }
        else if (Stream_getBufferCountWait(stream_tmp) != 0)
            Stream_IncrementBufferCount(stream_tmp);
    }
    if (server->withGUI == 1 && nchnls <= 16) {
        Server_process_gui(server);
    }
    if (server->withTIME == 1) {
        Server_process_time(server);
    }
    server->elapsedSamples += server->bufferSize;

    PyGILState_Release(s);

    if (amp != server->lastAmp) {
        server->timeCount = 0;
        server->stepVal = (amp - server->currentAmp) / server->timeStep;
        server->lastAmp = amp;
    }

    for (i=0; i < server->bufferSize; i++){
        if (server->timeCount < server->timeStep) {
            server->currentAmp += server->stepVal;
            server->timeCount++;
        }
        for (j=0; j<server->nchnls; j++) {
            out[(i*server->nchnls)+j] = (float)buffer[j][i] * server->currentAmp;
        }
    }

    /* Writing to disk is not real time safe. */
    if (server->record == 1)
        sf_write_float(server->recfile, out, server->bufferSize * server->nchnls);

    //clock_t end = clock();
    //double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    //printf("%f\n", time_spent);
}

void
Server_process_gui(Server *server)
{
    float rms[server->nchnls];
    float *out = server->output_buffer;
    float outAmp;
    int i,j;
    for (j=0; j<server->nchnls; j++) {
        rms[j] = 0.0;
        for (i=0; i<server->bufferSize; i++) {
            outAmp = out[(i*server->nchnls)+j];
            outAmp *= outAmp;
            if (outAmp > rms[j])
                rms[j] = outAmp;
        }
    }
    if (server->gcount <= server->numPass) {
        for (j=0; j<server->nchnls; j++) {
            server->lastRms[j] = (rms[j] + server->lastRms[j]) * 0.5;
        }
        server->gcount++;
    }
    else {
        for (j=0; j<server->nchnls; j++) {
            server->lastRms[j] = (rms[j] + server->lastRms[j]) * 0.5;
        }
        switch (server->nchnls) {
            case 1:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "f", server->lastRms[0]);
                break;
            case 2:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ff", server->lastRms[0], server->lastRms[1]);
                break;
            case 3:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fff", server->lastRms[0], server->lastRms[1], server->lastRms[2]);
                break;
            case 4:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3]);
                break;
            case 5:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4]);
                break;
            case 6:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5]);
                break;
            case 7:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6]);
                break;
            case 8:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7]);
                break;
            case 9:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8]);
                break;
            case 10:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9]);
                break;
            case 11:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10]);
                break;
            case 12:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10], server->lastRms[11]);
                break;
            case 13:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10], server->lastRms[11], server->lastRms[12]);
                break;
            case 14:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10], server->lastRms[11], server->lastRms[12], server->lastRms[13]);
                break;
            case 15:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "fffffffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10], server->lastRms[11], server->lastRms[12], server->lastRms[13], server->lastRms[14]);
                break;
            case 16:
                PyObject_CallMethod((PyObject *)server->GUI, "setRms", "ffffffffffffffff", server->lastRms[0], server->lastRms[1], server->lastRms[2], server->lastRms[3], server->lastRms[4], server->lastRms[5], server->lastRms[6], server->lastRms[7], server->lastRms[8], server->lastRms[9], server->lastRms[10], server->lastRms[11], server->lastRms[12], server->lastRms[13], server->lastRms[14], server->lastRms[15]);
                break;
        }
        server->gcount = 0;
    }
}

void
Server_process_time(Server *server)
{
    int hours, minutes, seconds, milliseconds;
    float sr = server->samplingRate;
    double sampsToSecs;

    if (server->tcount <= server->timePass) {
        server->tcount++;
    }
    else {
        sampsToSecs = (double)(server->elapsedSamples / sr);
        seconds = (int)sampsToSecs;
        milliseconds = (int)((sampsToSecs - seconds) * 1000);
        minutes = seconds / 60;
        hours = minutes / 60;
        minutes = minutes % 60;
        seconds = seconds % 60;
        PyObject_CallMethod((PyObject *)server->TIME, "setTime", "iiii", hours, minutes, seconds, milliseconds);
        server->tcount = 0;
    }
}

static int
Server_traverse(Server *self, visitproc visit, void *arg)
{
    Py_VISIT(self->GUI);
    Py_VISIT(self->TIME);
    if (self->CALLBACK != NULL)
        Py_VISIT(self->CALLBACK);
    Py_VISIT(self->streams);
    Py_VISIT(self->jackInputPortNames);
    Py_VISIT(self->jackOutputPortNames);
    Py_VISIT(self->jackMidiInputPortName);
    Py_VISIT(self->jackMidiOutputPortName);
    Py_VISIT(self->jackAutoConnectInputPorts);
    Py_VISIT(self->jackAutoConnectOutputPorts);
    Py_VISIT(self->jackAutoConnectMidiInputPort);
    Py_VISIT(self->jackAutoConnectMidiOutputPort);
    return 0;
}

static int
Server_clear(Server *self)
{
    Py_CLEAR(self->GUI);
    Py_CLEAR(self->TIME);
    if (self->CALLBACK != NULL)
        Py_CLEAR(self->CALLBACK);
    Py_CLEAR(self->streams);
    Py_CLEAR(self->jackInputPortNames);
    Py_CLEAR(self->jackOutputPortNames);
    Py_CLEAR(self->jackMidiInputPortName);
    Py_CLEAR(self->jackMidiOutputPortName);
    Py_CLEAR(self->jackAutoConnectInputPorts);
    Py_CLEAR(self->jackAutoConnectOutputPorts);
    Py_CLEAR(self->jackAutoConnectMidiInputPort);
    Py_CLEAR(self->jackAutoConnectMidiOutputPort);
    return 0;
}

static void
Server_dealloc(Server* self)
{
    if (self->server_booted == 1)
        Server_shutdown(self);
    Server_clear(self);
    free(self->input_buffer);
    free(self->output_buffer);
    free(self->serverName);
    if (self->withGUI == 1)
        free(self->lastRms);
    my_server[self->thisServerID] = NULL;
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* Unused variables to allow the safety check of the embedded audio backend. */
    double samplingRate = 44100.0;
    int  nchnls = 2;
    int  ichnls = 2;
    int  bufferSize = 256;
    int  duplex = 0;
    char *audioType = "portaudio";
    char *midiType = "portmidi";
    char *serverName = "pyo";

    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", "audio", "jackname", "ichnls", "midi", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|diiissis", kwlist,
            &samplingRate, &nchnls, &bufferSize, &duplex, &audioType, &serverName, &ichnls, &midiType)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    /* find the first free serverID */
    for(serverID = 0; serverID < MAX_NBR_SERVER; serverID++){
        if (my_server[serverID] == NULL){
            break;
        }
    }
    if(serverID == MAX_NBR_SERVER){
        PyErr_SetString(PyExc_RuntimeError, "You are already using the maximum number of server allowed!\n");
        Py_RETURN_NONE;
    }

    Server *self;
    self = (Server *)type->tp_alloc(type, 0);
    self->server_booted = 0;
    self->audio_be_data = NULL;
    self->midi_be_data = NULL;
    self->serverName = (char *) calloc(32, sizeof(char));
    self->jackautoin = 1;
    self->jackautoout = 1;
    self->streams = PyList_New(0);
    self->jackInputPortNames = PY_BYTES_FROM_STRING("");
    self->jackOutputPortNames = PY_BYTES_FROM_STRING("");
    self->jackMidiInputPortName = PY_BYTES_FROM_STRING("");
    self->jackMidiOutputPortName = PY_BYTES_FROM_STRING("");
    self->jackAutoConnectInputPorts = PyList_New(0);
    self->jackAutoConnectOutputPorts = PyList_New(0);
    self->jackAutoConnectMidiInputPort = PyList_New(0);
    self->jackAutoConnectMidiOutputPort = PyList_New(0);
    self->isJackTransportSlave = 0;
    self->jack_transport_state = 0;
    self->withJackMidi = 0;
    self->samplingRate = 44100.0;
    self->nchnls = 2;
    self->ichnls = 2;
    self->record = 0;
    self->bufferSize = 256;
    self->currentResampling = 1;
    self->lastResampling = 1;
    self->duplex = 0;
    self->input = -1;
    self->output = -1;
    self->input_offset = 0;
    self->output_offset = 0;
    self->midiin_count = 0;
    self->midiout_count = 0;
    self->midi_input = -1;
    self->midi_output = -1;
    self->midiActive = 1;
    self->allowMMMapper = 0; // Disable Microsoft MIDI Mapper by default.
    self->midi_time_offset = 0;
    self->amp = self->resetAmp = 1.;
    self->currentAmp = self->lastAmp = 0.; // If set to 0, there is a 5ms fadein at server start.
    self->withGUI = 0;
    self->withTIME = 0;
    self->verbosity = 7;
    self->recdur = -1;
    self->recformat = 0;
    self->rectype = 0;
    self->recquality = 0.4;
    self->globalDur = 0.0;
    self->globalDel = 0.0;
    self->startoffset = 0.0;
    self->globalSeed = 0;
    self->autoStartChildren = 0;
    self->CALLBACK = NULL;
    self->thisServerID = serverID;
    Py_XDECREF(my_server[serverID]);
    my_server[serverID] = (Server *)self;
    return (PyObject *)self;
}

static int
Server_init(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", "audio", "jackname", "ichnls", "midi", NULL};

    char *audioType = "portaudio";
    char *midiType = "portmidi";
    char *serverName = "pyo";

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|diiissis", kwlist,
            &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex, &audioType, &serverName, &self->ichnls, &midiType))
        return -1;

    if (strcmp(audioType, "jack") == 0) {
        self->audio_be_type = PyoJack;
    }
    else if (strcmp(audioType, "portaudio") == 0 || strcmp(audioType, "pa") == 0 ) {
        self->audio_be_type = PyoPortaudio;
    }
    else if (strcmp(audioType, "coreaudio") == 0) {
        self->audio_be_type = PyoCoreaudio;
    }
    else if (strcmp(audioType, "offline") == 0) {
        self->audio_be_type = PyoOffline;
    }
    else if (strcmp(audioType, "offline_nb") == 0) {
        self->audio_be_type = PyoOfflineNB;
    }
    else if (strcmp(audioType, "embedded") == 0) {
        self->audio_be_type = PyoEmbedded;
    }
    else {
        Server_warning(self, "Unknown audio type. Using Portaudio\n");
        self->audio_be_type = PyoPortaudio;
    }

    self->withJackMidi = 0;
    if (strcmp(midiType, "portmidi") == 0 || strcmp(midiType, "pm") == 0 ) {
        self->midi_be_type = PyoPortmidi;
    }
    else if (strcmp(midiType, "jack") == 0) {
        self->midi_be_type = PyoJackMidi;
        self->withJackMidi = 1;
    }
    else {
        Server_warning(self, "Unknown midi type. Using Portmidi\n");
        self->midi_be_type = PyoPortmidi;
    }

    strncpy(self->serverName, serverName, 32);
    if (strlen(serverName) > 31) {
        self->serverName[31] = '\0';
    }

    return 0;
}

/** Server's setters. **/
/***********************/

static PyObject *
Server_setDefaultRecPath(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"path", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &self->recpath))
        return PyInt_FromLong(-1);

    Py_RETURN_NONE;
}

static PyObject *
Server_setInputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change input offset when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->input_offset = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setOutputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change output offset when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->output_offset = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setInputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->input = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setInOutDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg)) {
            self->input = PyInt_AsLong(arg);
            self->output = PyInt_AsLong(arg);
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setOutputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->output = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setMidiInputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->midi_input = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setMidiOutputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->midi_output = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_deactivateMidi(Server *self)
{
    self->midiActive = 0;
    Py_RETURN_NONE;
}

static PyObject *
Server_setSamplingRate(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change sampling rate when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL && PyNumber_Check(arg)) {
        self->samplingRate = PyFloat_AsDouble(arg);
    }
    else {
        Server_error(self, "Sampling rate must be a number.\n");
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setNchnls(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change number of channels when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->nchnls = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Number of channels must be an integer.\n");
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setIchnls(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change number of input channels when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->ichnls = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Number of input channels must be an integer.\n");
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setBufferSize(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change buffer size when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->bufferSize = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Buffer size must be an integer.\n");
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalDur(Server *self, PyObject *arg)
{
    if (arg != NULL && PyNumber_Check(arg)) {
        self->globalDur = PyFloat_AsDouble(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalDel(Server *self, PyObject *arg)
{
    if (arg != NULL && PyNumber_Check(arg)) {
        self->globalDel = PyFloat_AsDouble(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setDuplex(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self,"Can't change duplex mode when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->duplex = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setJackAuto(Server *self, PyObject *args)
{
    int in=1, out=1;

    if (! PyArg_ParseTuple(args, "ii", &in, &out)) {
        Py_RETURN_NONE;
    }

    self->jackautoin = in;
    self->jackautoout = out;

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackAutoConnectInputPorts(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackAutoConnectInputPorts);
            Py_INCREF(tmp);
            self->jackAutoConnectInputPorts = tmp;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackAutoConnectOutputPorts(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackAutoConnectOutputPorts);
            Py_INCREF(tmp);
            self->jackAutoConnectOutputPorts = tmp;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackAutoConnectMidiInputPort(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackAutoConnectMidiInputPort);
            Py_INCREF(tmp);
            self->jackAutoConnectMidiInputPort = tmp;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackAutoConnectMidiOutputPort(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackAutoConnectMidiOutputPort);
            Py_INCREF(tmp);
            self->jackAutoConnectMidiOutputPort = tmp;
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackInputPortNames(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg) || PY_STRING_CHECK(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackInputPortNames);
            Py_INCREF(tmp);
            self->jackInputPortNames = tmp;

            jack_input_port_set_names(self);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackOutputPortNames(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PyList_Check(arg) || PY_STRING_CHECK(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackOutputPortNames);
            Py_INCREF(tmp);
            self->jackOutputPortNames = tmp;

            jack_output_port_set_names(self);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackMidiInputPortName(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PY_STRING_CHECK(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackMidiInputPortName);
            Py_INCREF(tmp);
            self->jackMidiInputPortName = tmp;

            jack_midi_input_port_set_name(self);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setJackMidiOutputPortName(Server *self, PyObject *arg)
{
    PyObject *tmp;

    if (arg != NULL) {
        if (PY_STRING_CHECK(arg)) {
            tmp = arg;
            Py_XDECREF(self->jackMidiOutputPortName);
            Py_INCREF(tmp);
            self->jackMidiOutputPortName = tmp;

            jack_midi_output_port_set_name(self);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setIsJackTransportSlave(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self,"Can't change isJackTransportSlave mode when the Server is already booted.\n");
        Py_RETURN_NONE;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->isJackTransportSlave = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalSeed(Server *self, PyObject *arg)
{
    self->globalSeed = 0;

    if (arg != NULL && PyLong_Check(arg)) {
        self->globalSeed = (int)PyInt_AsLong(arg);
        self->globalSeed = self->globalSeed > 0 ? self->globalSeed : 0;
    }

    Py_RETURN_NONE;
}

int
Server_generateSeed(Server *self, int oid)
{
    unsigned int curseed, count, mult, ltime;

    count = ++rnd_objs_count[oid];
    mult = rnd_objs_mult[oid];

    if (self->globalSeed > 0) {
        curseed = (self->globalSeed + count * mult) % PYO_RAND_MAX;
    }
    else {
        ltime = (unsigned int)time(NULL);
        curseed = (ltime * ltime + count * mult) % PYO_RAND_MAX;
    }

    PYO_RAND_SEED = curseed;

    return 0;
}

static PyObject *
Server_setAmp(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        int check = PyNumber_Check(arg);

        if (check) {
            self->amp = PyFloat_AsDouble(arg);
            if (self->amp != 0.0)
                self->resetAmp = self->amp;
        }
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_setAmpCallable(Server *self, PyObject *arg)
{
    int i;
    PyObject *tmp;

    ASSERT_ARG_NOT_NULL

    tmp = arg;
    Py_XDECREF(self->GUI);
    Py_INCREF(tmp);
    self->GUI = tmp;

    self->lastRms = (float *)realloc(self->lastRms, self->nchnls * sizeof(float));
    for (i=0; i<self->nchnls; i++) {
        self->lastRms[i] = 0.0;
    }

    for (i=1; i<100; i++) {
        if ((self->bufferSize * i / self->samplingRate) > 0.045) {
            self->numPass = i;
            break;
        }
    }
    self->gcount = 0;
    self->withGUI = 1;

    Py_RETURN_NONE;
}

static PyObject *
Server_setTimeCallable(Server *self, PyObject *arg)
{
    int i;
    PyObject *tmp;

    ASSERT_ARG_NOT_NULL

    tmp = arg;
    Py_XDECREF(self->TIME);
    Py_INCREF(tmp);
    self->TIME = tmp;

    for (i=1; i<100; i++) {
        if ((self->bufferSize * i / self->samplingRate) > 0.06) {
            self->timePass = i;
            break;
        }
    }
    self->tcount = 0;
    self->withTIME = 1;

    Py_RETURN_NONE;
}

static PyObject *
Server_setCallback(Server *self, PyObject *arg)
{
    PyObject *tmp;

    ASSERT_ARG_NOT_NULL

    tmp = arg;
    Py_XDECREF(self->CALLBACK);
    Py_INCREF(tmp);
    self->CALLBACK = tmp;

    Py_RETURN_NONE;
}

static PyObject *
Server_setVerbosity(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        int check = PyInt_Check(arg);

        if (check) {
            self->verbosity = PyInt_AsLong(arg);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setStartOffset(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        int check = PyNumber_Check(arg);

        if (check) {
            self->startoffset = PyFloat_AsDouble(arg);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_allowMicrosoftMidiDevices(Server *self)
{
    self->allowMMMapper = 1;
    Py_RETURN_NONE;
}

/*******************************************/
/** Server shutdown / boot / start / stop **/
/*******************************************/

PyObject *
Server_shutdown(Server *self)
{
    int i, ret = -1;
    PyGILState_STATE s = 0;

    if (self->server_booted == 0) {
        Server_error(self, "The Server must be booted!\n");
        Py_RETURN_NONE;
    }
    if (self->server_started == 1) {
        Server_stop((Server *)self);
    }

    for (i=0; i<num_rnd_objs; i++) {
        rnd_objs_count[i] = 0;
    }

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidi == 1 || self->withPortMidiOut == 1)
                ret = Server_pm_deinit(self);
            break;
        default:
            break;
    }

    switch (self->audio_be_type) {
        case PyoPortaudio:
            ret = Server_pa_deinit(self);
            break;
        case PyoCoreaudio:
            ret = Server_coreaudio_deinit(self);
            break;
        case PyoJack:
            ret = Server_jack_deinit(self);
            break;
        case PyoOffline:
            ret = Server_offline_deinit(self);
            break;
        case PyoOfflineNB:
            ret = Server_offline_deinit(self);
            break;
        case PyoEmbedded:
            ret = Server_embedded_deinit(self);
            break;
    }
    self->server_booted = 0;
    if (ret < 0) {
        Server_error(self, "Error closing audio backend.\n");
    }

    /* Cleaning list of audio streams. 
       Note: Grabbing the GIL crashes embedded servers. */
    if (self->audio_be_type != PyoEmbedded) {
        s = PyGILState_Ensure();
    }
    if (PyList_Size(self->streams) > 0) {
        for (i=PyList_Size(self->streams); i>0; i--) {
            PySequence_DelItem(self->streams, i-1);
        }
    }
    self->stream_count = 0;
    if (self->audio_be_type != PyoEmbedded) {
        PyGILState_Release(s);
    }

    Py_RETURN_NONE;
}

PyObject *
Server_boot(Server *self, PyObject *arg)
{
    int i, audioerr = 0, midierr = 0;
    if (self->server_booted == 1) {
        Server_error(self, "Server already booted!\n");
        Py_RETURN_NONE;
    }
    self->server_started = 0;
    self->stream_count = 0;
    self->elapsedSamples = 0;

    /* Ensure Python is set up for threading */
    if (!PyEval_ThreadsInitialized()) {
        PyEval_InitThreads();
    }

    int needNewBuffer = 0;
    if (arg != NULL && PyBool_Check(arg)) {
        needNewBuffer = PyObject_IsTrue(arg);
    }
    else {
        Server_error(self, "The argument to set for a new buffer must be a boolean.\n");
    }

    Server_debug(self, "Streams list size at Server boot (must always be 0) = %d\n",
                 PyList_Size(self->streams));
    switch (self->audio_be_type) {
        case PyoPortaudio:
            audioerr = Server_pa_init(self);
            if (audioerr < 0) {
                Server_pa_deinit(self);
                if (audioerr == -10)
                    Server_error(self, "Pyo built without Portaudio support\n");
            }
            break;
        case PyoJack:
            audioerr = Server_jack_init(self);
            if (audioerr < 0) {
                Server_jack_deinit(self);
                if (audioerr == -10)
                    Server_error(self, "Pyo built without Jack support\n");
            }
            break;
        case PyoCoreaudio:
            audioerr = Server_coreaudio_init(self);
            if (audioerr < 0) {
                Server_coreaudio_deinit(self);
                if (audioerr == -10)
                    Server_error(self, "Pyo built without Coreaudio support\n");
            }
            break;
        case PyoOffline:
            audioerr = Server_offline_init(self);
            if (audioerr < 0) {
                Server_offline_deinit(self);
            }
            break;
        case PyoOfflineNB:
            audioerr = Server_offline_init(self);
            if (audioerr < 0) {
                Server_offline_deinit(self);
            }
            break;
        case PyoEmbedded:
            audioerr = Server_embedded_init(self);
            if (audioerr < 0) {
                Server_embedded_deinit(self);
            }
            break;
    }
    if (needNewBuffer == 1){
        /* Must allocate buffer after initializing the audio backend in case parameters change there */
        if (self->input_buffer) {
            free(self->input_buffer);
        }
        self->input_buffer = (MYFLT *)calloc(self->bufferSize * self->ichnls, sizeof(MYFLT));
        if (self->output_buffer) {
            free(self->output_buffer);
        }
        self->output_buffer = (float *)calloc(self->bufferSize * self->nchnls, sizeof(float));
    }
    for (i=0; i<self->bufferSize*self->ichnls; i++) {
        self->input_buffer[i] = 0.0;
    }
    for (i=0; i<self->bufferSize*self->nchnls; i++) {
        self->output_buffer[i] = 0.0;
    }
    if (audioerr == 0) {
        self->server_booted = 1;
    }
    else {
        self->server_booted = 0;
        Server_error(self, "\nServer not booted.\n");
    }

    if (self->audio_be_type != PyoOffline && self->audio_be_type != PyoOfflineNB && self->audio_be_type != PyoEmbedded) {
        switch (self->midi_be_type) {
            case PyoPortmidi:
                midierr = Server_pm_init(self);
                if (midierr < 0) {
                    Server_pm_deinit(self);
                    if (midierr == -10)
                        Server_error(self, "Pyo built without Portmidi support\n");
                }
                break;
            case PyoJackMidi:
                /* Initialized inside the jack audio backend. */
                if (self->audio_be_type != PyoJack) {
                    Server_error(self, "To use jack midi, you must also use jack as the audio backend.\n"); 
                }
                break;
        }
    }

    Py_RETURN_NONE;
}

PyObject *
Server_start(Server *self)
{
    int err = -1;
    if (self->server_started == 1) {
        Server_warning(self, "Server already started!\n");
        Py_RETURN_NONE;
    }

    if (self->server_booted == 0) {
        Server_warning(self, "The Server must be booted before calling the start method!\n");
        Py_RETURN_NONE;
    }

    Server_debug(self, "Number of streams at Server start = %d\n", self->stream_count);

    self->server_stopped = 0;
    self->server_started = 1;
    self->timeStep = (int)(0.005 * self->samplingRate);

    if (self->startoffset > 0.0) {
        Server_message(self, "Rendering %.2f seconds offline...\n", self->startoffset);
        int numBlocks = ceil(self->startoffset * self->samplingRate/self->bufferSize);
        self->lastAmp = 1.0; self->amp = 0.0;
        while (numBlocks-- > 0) {
            offline_process_block((Server *) self);
        }
        Server_message(self, "Offline rendering completed. Start realtime processing.\n");
        self->startoffset = 0.0;
    }

    self->amp = self->resetAmp;

    switch (self->audio_be_type) {
        case PyoPortaudio:
            err = Server_pa_start(self);
            break;
        case PyoCoreaudio:
            err = Server_coreaudio_start(self);
            break;
        case PyoJack:
            err = Server_jack_start(self);
            break;
        case PyoOffline:
            err = Server_offline_start(self);
            break;
        case PyoOfflineNB:
            err = Server_offline_nb_start(self);
            break;
        case PyoEmbedded:
            err = Server_embedded_nb_start(self);
            break;
    }
    if (err) {
        Server_error(self, "Error starting server.\n");
    }

    if (self->withGUI && PyObject_HasAttrString((PyObject *)self->GUI, "setStartButtonState"))
        PyObject_CallMethod((PyObject *)self->GUI, "setStartButtonState", "i", 1);

    Py_RETURN_NONE;
}

PyObject *
Server_stop(Server *self)
{
    int err = 0;
    if (self->server_started == 0) {
        Server_warning(self, "The Server must be started!\n");
        Py_RETURN_NONE;
    }
    switch (self->audio_be_type) {
        case PyoPortaudio:
            err = Server_pa_stop(self); break;
        case PyoCoreaudio:
            err = Server_coreaudio_stop(self); break;
        case PyoJack:
            err = Server_jack_stop(self); break;
        case PyoOffline:
            err = Server_offline_stop(self); break;
        case PyoOfflineNB:
            err = Server_offline_stop(self); break;
        case PyoEmbedded:
            err = Server_embedded_stop(self); break;
    }

    if (err != 0) {
        Server_error(self, "Error stopping server.\n");
    }
    else {
        self->server_stopped = 1;
        self->server_started = 0;
    }

    if (self->withGUI && PyObject_HasAttrString((PyObject *)self->GUI, "setStartButtonState"))
        PyObject_CallMethod((PyObject *)self->GUI, "setStartButtonState", "i", 0);

    Py_RETURN_NONE;
}

static PyObject *
Server_recordOptions(Server *self, PyObject *args, PyObject *kwds)
{
    //Py_ssize_t psize;
    static char *kwlist[] = {"dur", "filename", "fileformat", "sampletype", "quality", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "d|siid", kwlist, &self->recdur, &self->recpath, &self->recformat, &self->rectype, &self->recquality)) {
        return PyInt_FromLong(-1);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_start_rec(Server *self, PyObject *args, PyObject *kwds)
{
    Py_ssize_t psize;
    char *filename=NULL;

    static char *kwlist[] = {"filename", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|s#", kwlist, &filename, &psize)) {
        return PyInt_FromLong(-1);
    }
    Server_start_rec_internal(self, filename);

    Py_RETURN_NONE;
}

int
Server_start_rec_internal(Server *self, char *filename)
{
    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->samplingRate;
    self->recinfo.channels = self->nchnls;

    Server_debug(self, "Recording samplerate = %i\n", self->recinfo.samplerate);
    Server_debug(self, "Recording number of channels = %i\n", self->recinfo.channels);

    switch (self->recformat) {
        case 0:
            self->recinfo.format = SF_FORMAT_WAV;
            break;
        case 1:
            self->recinfo.format = SF_FORMAT_AIFF;
            break;
        case 2:
            self->recinfo.format = SF_FORMAT_AU;
            break;
        case 3:
            self->recinfo.format = SF_FORMAT_RAW;
            break;
        case 4:
            self->recinfo.format = SF_FORMAT_SD2;
            break;
        case 5:
            self->recinfo.format = SF_FORMAT_FLAC;
            break;
        case 6:
            self->recinfo.format = SF_FORMAT_CAF;
            break;
        case 7:
            self->recinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            break;
    }
    if (self->recformat != 7) {
        switch (self->rectype) {
            case 0:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_16;
                break;
            case 1:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_24;
                break;
            case 2:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_32;
                break;
            case 3:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_FLOAT;
                break;
            case 4:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_DOUBLE;
                break;
            case 5:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_ULAW;
                break;
            case 6:
                self->recinfo.format = self->recinfo.format | SF_FORMAT_ALAW;
                break;
        }
    }
    Server_debug(self, "Recording format = %i\n", self->recinfo.format);

    /* Open the output file. */
    if (filename == NULL) {
        Server_debug(self, "Recording path = %s\n", self->recpath);
        if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {
            Server_error(self, "Not able to open output file %s.\n", self->recpath);
            Server_debug(self, "%s\n", sf_strerror(self->recfile));
            return -1;
        }
    }
    else {
        Server_debug(self, "Recording filename path = %s\n", filename);
        if (! (self->recfile = sf_open(filename, SFM_WRITE, &self->recinfo))) {
            Server_error(self, "Not able to open output file %s.\n", filename);
            Server_debug(self, "%s\n", sf_strerror(self->recfile));
            return -1;
        }
    }

    /* Sets the encoding quality for FLAC and OGG compressed formats. */
    if (self->recformat == 5 || self->recformat == 7) {
        sf_command(self->recfile, SFC_SET_VBR_ENCODING_QUALITY, &self->recquality, sizeof(double));
    }

    self->record = 1;
    return 0;
}

static PyObject *
Server_stop_rec(Server *self, PyObject *args)
{
    self->record = 0;
    sf_close(self->recfile);

    Py_RETURN_NONE;
}

static PyObject *
Server_addStream(Server *self, PyObject *args)
{
    PyObject *tmp;

    if (! PyArg_ParseTuple(args, "O", &tmp))
        return PyInt_FromLong(-1);

    if (tmp == NULL) {
        Server_error(self, "Server_addStream function needs a PyoObject as argument.\n");
        return PyInt_FromLong(-1);
    }

    PyList_Append(self->streams, tmp);

    self->stream_count++;

    Py_RETURN_NONE;
}

PyObject *
Server_removeStream(Server *self, int id)
{
    int i, sid;
    Stream *stream_tmp;
    PyGILState_STATE s = 0;

    if (self->audio_be_type != PyoEmbedded) {
        s = PyGILState_Ensure();
    }
    if (my_server[self->thisServerID] != NULL && PySequence_Size(self->streams) != -1) {
        for (i=0; i<self->stream_count; i++) {
            stream_tmp = (Stream *)PyList_GetItem(self->streams, i);
            if (stream_tmp != NULL) {
                sid = Stream_getStreamId(stream_tmp);
                if (sid == id) {
                    Server_debug(self, "Removed stream id %d\n", id);
                    PySequence_DelItem(self->streams, i);
                    self->stream_count--;
                    break;
                }
            }
        }
    }
    if (self->audio_be_type != PyoEmbedded) {
        PyGILState_Release(s);
    }

    Py_RETURN_NONE;
}

PyObject *
Server_changeStreamPosition(Server *self, PyObject *args)
{
    int i, rsid, csid, sid;
    Stream *ref_stream_tmp, *cur_stream_tmp, *stream_tmp;

    if (! PyArg_ParseTuple(args, "OO", &ref_stream_tmp, &cur_stream_tmp))
        return PyInt_FromLong(-1);

    rsid = Stream_getStreamId(ref_stream_tmp);
    csid = Stream_getStreamId(cur_stream_tmp);

    for (i=0; i<self->stream_count; i++) {
        stream_tmp = (Stream *)PyList_GET_ITEM(self->streams, i);
        sid = Stream_getStreamId(stream_tmp);
        if (sid == csid) {
            PySequence_DelItem(self->streams, i);
            self->stream_count--;
            break;
        }
    }

    for (i=0; i<self->stream_count; i++) {
        stream_tmp = (Stream *)PyList_GET_ITEM(self->streams, i);
        sid = Stream_getStreamId(stream_tmp);
        if (sid == rsid) {
            break;
        }
    }

    Py_INCREF(cur_stream_tmp);
    PyList_Insert(self->streams, i, (PyObject *)cur_stream_tmp);
    self->stream_count++;

    Py_RETURN_NONE;
}

void pyoGetMidiEvents(Server *self) {
    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidi == 1) {
                portmidiGetEvents(self);
            }
            break;
        case PyoJackMidi:
            /* Handled inside jack audio callback! */
        default:
            break;
    }
}

PyObject *
Server_noteout(Server *self, PyObject *args)
{
    int pit, vel, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiil", &pit, &vel, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_noteout(self, pit, vel, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_noteout(self, pit, vel, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_afterout(Server *self, PyObject *args)
{
    int pit, vel, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiil", &pit, &vel, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_afterout(self, pit, vel, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_afterout(self, pit, vel, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_ctlout(Server *self, PyObject *args)
{
    int ctlnum, value, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiil", &ctlnum, &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_ctlout(self, ctlnum, value, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_ctlout(self, ctlnum, value, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_programout(Server *self, PyObject *args)
{
    int value, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iil", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_programout(self, value, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_programout(self, value, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_pressout(Server *self, PyObject *args)
{
    int value, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iil", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_pressout(self, value, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_pressout(self, value, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_bendout(Server *self, PyObject *args)
{
    int value, chan;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iil", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_bendout(self, value, chan, timestamp);
            }
            break;
        case PyoJackMidi:
            jack_bendout(self, value, chan, timestamp);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

PyObject *
Server_sysexout(Server *self, PyObject *args)
{
    unsigned char *msg;
    int size;
    PyoMidiTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "s#l", &msg, &size, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        switch (self->midi_be_type) {
            case PyoPortmidi:
                pm_sysexout(self, msg, timestamp);
                break;
            default:
                break;
        }
    }
    Py_RETURN_NONE;
}

PyObject *
Server_makenote(Server *self, PyObject *args)
{
    int pit, vel, dur, chan;

    if (! PyArg_ParseTuple(args, "iiii", &pit, &vel, &dur, &chan))
        return PyInt_FromLong(-1);

    switch (self->midi_be_type) {
        case PyoPortmidi:
            if (self->withPortMidiOut) {
                pm_makenote(self, pit, vel, dur, chan);
            }
            break;
        case PyoJackMidi:
            jack_makenote(self, pit, vel, dur, chan);
            break;
        default:
            break;
    }
    Py_RETURN_NONE;
}

MYFLT *
Server_getInputBuffer(Server *self) {
    return (MYFLT *)self->input_buffer;
}

PyoMidiEvent *
Server_getMidiEventBuffer(Server *self) {
    return (PyoMidiEvent *)self->midiEvents;
}

int
Server_getMidiEventCount(Server *self) {
    return self->midi_count;
}

long
Server_getMidiTimeOffset(Server *self) {
    return self->midi_time_offset;
}

unsigned long Server_getElapsedTime(Server *self) {
    return self->elapsedSamples;
}

static PyObject *
Server_addMidiEvent(Server *self, PyObject *args)
{
    int status, data1, data2;
    PyoMidiEvent buffer;
    
    if (! PyArg_ParseTuple(args, "iii", &status, &data1, &data2))
        return PyInt_FromLong(-1);

    buffer.timestamp = 0;
    buffer.message = PyoMidi_Message(status, data1, data2);
    self->midiEvents[self->midi_count++] = buffer;
    Py_RETURN_NONE;
}

int
Server_getCurrentResamplingFactor(Server *self) {
    return self->currentResampling;
}

int
Server_getLastResamplingFactor(Server *self) {
    return self->lastResampling;
}

static PyObject *
Server_getSamplingRate(Server *self)
{
    if (self->currentResampling < 0)
        return PyFloat_FromDouble(self->samplingRate / -self->currentResampling);
    else
        return PyFloat_FromDouble(self->samplingRate * self->currentResampling);
}

static PyObject *
Server_getNchnls(Server *self)
{
    return PyInt_FromLong(self->nchnls);
}

static PyObject *
Server_getIchnls(Server *self)
{
    return PyInt_FromLong(self->ichnls);
}

static PyObject *
Server_getGlobalSeed(Server *self)
{
    return PyInt_FromLong(self->globalSeed);
}

static PyObject *
Server_getBufferSize(Server *self)
{
    if (self->currentResampling < 0)
        return PyInt_FromLong(self->bufferSize / -self->currentResampling);
    else
        return PyInt_FromLong(self->bufferSize * self->currentResampling);
}

static PyObject *
Server_getGlobalDur(Server *self)
{
    return PyFloat_FromDouble(self->globalDur);
}

static PyObject *
Server_getGlobalDel(Server *self)
{
    return PyFloat_FromDouble(self->globalDel);
}

static PyObject *
Server_beginResamplingBlock(Server *self, PyObject *arg)
{
    if (PyInt_Check(arg)) {
        self->lastResampling = self->currentResampling;
        self->currentResampling = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_endResamplingBlock(Server *self)
{
    self->lastResampling = self->currentResampling;
    self->currentResampling = 1;
    Py_RETURN_NONE;
}

static PyObject *
Server_getIsStarted(Server *self)
{
    return PyInt_FromLong(self->server_started);
}

static PyObject *
Server_getIsBooted(Server *self)
{
    return PyInt_FromLong(self->server_booted);
}

static PyObject *
Server_getMidiActive(Server *self)
{
    return PyInt_FromLong(self->withPortMidi);
}

static PyObject *
Server_getStreams(Server *self)
{
    Py_INCREF(self->streams);
    return self->streams;
}

static PyObject *
Server_setServer(Server *self)
{
    serverID = self->thisServerID;
    return PyInt_FromLong(serverID);
}


static PyObject *
Server_getInputAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &self->input_buffer[0]);
    return PyUnicode_FromString(address);
}


static PyObject *
Server_getOutputAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &self->output_buffer[0]);
    return PyUnicode_FromString(address);
}

static PyObject *
Server_getServerID(Server *self)
{
    return PyInt_FromLong(self->thisServerID);
}

static PyObject *
Server_getServerAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &my_server[self->thisServerID]);
    return PyUnicode_FromString(address);
}

void
Server_getThisServer(int id, Server *server)
{
    server = my_server[id];
}

/*
static PyObject *
Server_getThisServerFunc(Server *self)
{
    char address[32];
    sprintf(address, "%p", &Server_getThisServer);
    return PyUnicode_FromString(address);
}
*/

static PyObject *
Server_getEmbedICallbackAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &Server_embedded_i_startIdx);
    return PyUnicode_FromString(address);
}

static PyObject *
Server_getCurrentTime(Server *self)
{
    int hours, minutes, seconds, milliseconds;
    float sr = self->samplingRate;
    double sampsToSecs;
    char curtime[20];

    sampsToSecs = (double)(self->elapsedSamples / sr);
    seconds = (int)sampsToSecs;
    milliseconds = (int)((sampsToSecs - seconds) * 1000);
    minutes = seconds / 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    seconds = seconds % 60;
    sprintf(curtime, "%02d : %02d : %02d : %03d", hours, minutes, seconds, milliseconds);
    return PyUnicode_FromString(curtime);
}

static PyObject *
Server_getCurrentAmp(Server *self)
{
    PyObject *amplist;
    float rms[self->nchnls];
    float *out = self->output_buffer;
    float outAmp;
    int i,j;
    for (j=0; j<self->nchnls; j++) {
        rms[j] = 0.0;
        for (i=0; i<self->bufferSize; i++) {
            outAmp = out[(i*self->nchnls)+j];
            outAmp *= outAmp;
            if (outAmp > rms[j])
                rms[j] = outAmp;
        }
    }
    amplist = PyTuple_New(self->nchnls);
    for (i=0; i<self->nchnls; i++) {
        PyTuple_SET_ITEM(amplist, i, PyFloat_FromDouble(rms[i]));
    }
    return amplist;
}

static PyObject *
Server_setAutoStartChildren(Server *self, PyObject *arg)
{
    if (PyInt_Check(arg)) {
        self->autoStartChildren = PyInt_AsLong(arg);
    }
    Py_RETURN_NONE;
}

static PyObject *
Server_getAutoStartChildren(Server *self)
{
    return PyInt_FromLong(self->autoStartChildren);
}

static PyMethodDef Server_methods[] = {
    {"setInputDevice", (PyCFunction)Server_setInputDevice, METH_O, "Sets audio input device."},
    {"setOutputDevice", (PyCFunction)Server_setOutputDevice, METH_O, "Sets audio output device."},
    {"setInputOffset", (PyCFunction)Server_setInputOffset, METH_O, "Sets audio input channel offset."},
    {"setOutputOffset", (PyCFunction)Server_setOutputOffset, METH_O, "Sets audio output channel offset."},
    {"setInOutDevice", (PyCFunction)Server_setInOutDevice, METH_O, "Sets both audio input and output device."},
    {"setMidiInputDevice", (PyCFunction)Server_setMidiInputDevice, METH_O, "Sets MIDI input device."},
    {"setMidiOutputDevice", (PyCFunction)Server_setMidiOutputDevice, METH_O, "Sets MIDI output device."},
    {"deactivateMidi", (PyCFunction)Server_deactivateMidi, METH_NOARGS, "Deactivates midi callback."},
    {"setSamplingRate", (PyCFunction)Server_setSamplingRate, METH_O, "Sets the server's sampling rate."},
    {"setBufferSize", (PyCFunction)Server_setBufferSize, METH_O, "Sets the server's buffer size."},
    {"setGlobalDur", (PyCFunction)Server_setGlobalDur, METH_O, "Sets the server's globalDur attribute."},
    {"setGlobalDel", (PyCFunction)Server_setGlobalDel, METH_O, "Sets the server's globalDel attribute."},
    {"beginResamplingBlock", (PyCFunction)Server_beginResamplingBlock, METH_O, "Starts a resampling code block."},
    {"endResamplingBlock", (PyCFunction)Server_endResamplingBlock, METH_NOARGS, "Stops a resampling code block."},
    {"setNchnls", (PyCFunction)Server_setNchnls, METH_O, "Sets the server's number of output/input channels."},
    {"setIchnls", (PyCFunction)Server_setIchnls, METH_O, "Sets the server's number of input channels."},
    {"setDuplex", (PyCFunction)Server_setDuplex, METH_O, "Sets the server's duplex mode (0 = only out, 1 = in/out)."},
    {"setJackAuto", (PyCFunction)Server_setJackAuto, METH_VARARGS, "Tells the server to auto-connect Jack ports (0 = disable, 1 = enable)."},
    {"setJackAutoConnectInputPorts", (PyCFunction)Server_setJackAutoConnectInputPorts, METH_O, "Sets a list of ports to auto-connect inputs when using Jack."},
    {"setJackAutoConnectOutputPorts", (PyCFunction)Server_setJackAutoConnectOutputPorts, METH_O, "Sets a list of ports to auto-connect outputs when using Jack."},
    {"setJackAutoConnectMidiInputPort", (PyCFunction)Server_setJackAutoConnectMidiInputPort, METH_O, "Sets a list of ports to auto-connect midi inputs when using JackMidi."},
    {"setJackAutoConnectMidiOutputPort", (PyCFunction)Server_setJackAutoConnectMidiOutputPort, METH_O, "Sets a list of ports to auto-connect midi outputs when using JackMidi."},
    {"setJackInputPortNames", (PyCFunction)Server_setJackInputPortNames, METH_O, "Sets the short name of input ports for jack server."},
    {"setJackOutputPortNames", (PyCFunction)Server_setJackOutputPortNames, METH_O, "Sets the short name of output ports for jack server."},
    {"setJackMidiInputPortName", (PyCFunction)Server_setJackMidiInputPortName, METH_O, "Sets the short name of midi input port for jack server."},
    {"setJackMidiOutputPortName", (PyCFunction)Server_setJackMidiOutputPortName, METH_O, "Sets the short name of midi output port for jack server."},
    {"setIsJackTransportSlave", (PyCFunction)Server_setIsJackTransportSlave, METH_O, "Sets if the server's start/stop is slave of jack transport."},
    {"setGlobalSeed", (PyCFunction)Server_setGlobalSeed, METH_O, "Sets the server's global seed for random objects."},
    {"setAmp", (PyCFunction)Server_setAmp, METH_O, "Sets the overall amplitude."},
    {"setAmpCallable", (PyCFunction)Server_setAmpCallable, METH_O, "Sets the Server's GUI callable object."},
    {"setTimeCallable", (PyCFunction)Server_setTimeCallable, METH_O, "Sets the Server's TIME callable object."},
    {"setCallback", (PyCFunction)Server_setCallback, METH_O, "Sets the Server's CALLBACK callable object."},
    {"setVerbosity", (PyCFunction)Server_setVerbosity, METH_O, "Sets the verbosity."},
    {"allowMicrosoftMidiDevices", (PyCFunction)Server_allowMicrosoftMidiDevices, METH_NOARGS, "Allow Microsoft Midi Mapper or GS Wavetable Synth devices."},
    {"setStartOffset", (PyCFunction)Server_setStartOffset, METH_O, "Sets starting time offset."},
    {"boot", (PyCFunction)Server_boot, METH_O, "Setup and boot the server."},
    {"shutdown", (PyCFunction)Server_shutdown, METH_NOARGS, "Shut down the server."},
    {"start", (PyCFunction)Server_start, METH_NOARGS, "Starts the server's callback loop."},
    {"stop", (PyCFunction)Server_stop, METH_NOARGS, "Stops the server's callback loop."},
    {"recordOptions", (PyCFunction)Server_recordOptions, METH_VARARGS|METH_KEYWORDS, "Sets format settings for offline rendering and global recording."},
    {"recstart", (PyCFunction)Server_start_rec, METH_VARARGS|METH_KEYWORDS, "Start automatic output recording."},
    {"recstop", (PyCFunction)Server_stop_rec, METH_NOARGS, "Stop automatic output recording."},
    {"addStream", (PyCFunction)Server_addStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
    {"removeStream", (PyCFunction)Server_removeStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
    {"changeStreamPosition", (PyCFunction)Server_changeStreamPosition, METH_VARARGS, "Puts an audio stream before another in the stack. \
                                                                This is for internal use and must never be called by the user."},
    {"noteout", (PyCFunction)Server_noteout, METH_VARARGS, "Send a Midi note event to Portmidi output stream."},
    {"afterout", (PyCFunction)Server_afterout, METH_VARARGS, "Send an aftertouch event to Portmidi output stream."},
    {"ctlout", (PyCFunction)Server_ctlout, METH_VARARGS, "Send a control change event to Portmidi output stream."},
    {"programout", (PyCFunction)Server_programout, METH_VARARGS, "Send a program change event to Portmidi output stream."},
    {"pressout", (PyCFunction)Server_pressout, METH_VARARGS, "Send a channel pressure event to Portmidi output stream."},
    {"bendout", (PyCFunction)Server_bendout, METH_VARARGS, "Send a pitch bend event to Portmidi output stream."},
    {"sysexout", (PyCFunction)Server_sysexout, METH_VARARGS, "Send a system exclusive message to midi output stream."},
    {"makenote", (PyCFunction)Server_makenote, METH_VARARGS, "Send a Midi noteon event followed by its corresponding noteoff event."},
    {"addMidiEvent", (PyCFunction)Server_addMidiEvent, METH_VARARGS, "Add a midi event manually (without using portmidi callback)."},
    {"getStreams", (PyCFunction)Server_getStreams, METH_NOARGS, "Returns the list of streams added to the server."},
    {"getSamplingRate", (PyCFunction)Server_getSamplingRate, METH_NOARGS, "Returns the server's sampling rate."},
    {"getNchnls", (PyCFunction)Server_getNchnls, METH_NOARGS, "Returns the server's current number of output channels."},
    {"getIchnls", (PyCFunction)Server_getIchnls, METH_NOARGS, "Returns the server's current number of input channels."},
    {"getGlobalSeed", (PyCFunction)Server_getGlobalSeed, METH_NOARGS, "Returns the server's global seed."},
    {"getBufferSize", (PyCFunction)Server_getBufferSize, METH_NOARGS, "Returns the server's buffer size."},
    {"getGlobalDur", (PyCFunction)Server_getGlobalDur, METH_NOARGS, "Returns the server's globalDur attribute."},
    {"getGlobalDel", (PyCFunction)Server_getGlobalDel, METH_NOARGS, "Returns the server's globalDel attribute."},
    {"getIsBooted", (PyCFunction)Server_getIsBooted, METH_NOARGS, "Returns 1 if the server is booted, otherwise returns 0."},
    {"getIsStarted", (PyCFunction)Server_getIsStarted, METH_NOARGS, "Returns 1 if the server is started, otherwise returns 0."},
    {"getMidiActive", (PyCFunction)Server_getMidiActive, METH_NOARGS, "Returns 1 if midi callback is active, otherwise returns 0."},
    {"_setDefaultRecPath", (PyCFunction)Server_setDefaultRecPath, METH_VARARGS|METH_KEYWORDS, "Sets the default recording path."},
    {"setServer", (PyCFunction)Server_setServer, METH_NOARGS, "Sets this server as the one to use for new objects when using the embedded device"},
    {"getInputAddr", (PyCFunction)Server_getInputAddr, METH_NOARGS, "Get the embedded device input buffer memory address"},
    {"getOutputAddr", (PyCFunction)Server_getOutputAddr, METH_NOARGS, "Get the embedded device output buffer memory address"},
    {"getServerID", (PyCFunction)Server_getServerID, METH_NOARGS, "Get the embedded device server memory address"},
    {"getServerAddr", (PyCFunction)Server_getServerAddr, METH_NOARGS, "Get the embedded device server memory address"},
    {"getEmbedICallbackAddr", (PyCFunction)Server_getEmbedICallbackAddr, METH_NOARGS, "Get the embedded device interleaved callback method memory address"},
    {"getCurrentTime", (PyCFunction)Server_getCurrentTime, METH_NOARGS, "Get the current time as a formatted string."},
    {"getCurrentAmp", (PyCFunction)Server_getCurrentAmp, METH_NOARGS, "Get the current global amplitudes as a list of floats."},
    {"setAutoStartChildren", (PyCFunction)Server_setAutoStartChildren, METH_O, "Sets autoStartChildren attribute."},
    {"getAutoStartChildren", (PyCFunction)Server_getAutoStartChildren, METH_NOARGS, "Gets autoStartChildren attribute."},
    {NULL}  /* Sentinel */
};

static PyMemberDef Server_members[] = {
    {"streams", T_OBJECT_EX, offsetof(Server, streams), 0, "Server's streams list."},
    {NULL}  /* Sentinel */
};

PyTypeObject ServerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.Server",         /*tp_name*/
    sizeof(Server),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Server_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT, /*tp_flags*/
    "Pyo Server object. Handles communication with Portaudio and processing callback loop.",           /* tp_doc */
    (traverseproc)Server_traverse,   /* tp_traverse */
    (inquiry)Server_clear,           /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Server_methods,             /* tp_methods */
    Server_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Server_init,      /* tp_init */
    0,                         /* tp_alloc */
    Server_new,                 /* tp_new */
};
