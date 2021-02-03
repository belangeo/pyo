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

#include "structmember.h"
#include "streammodule.h"
#include "pyomodule.h"
#include "servermodule.h"

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

int rnd_objs_count[num_rnd_objs] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int rnd_objs_mult[num_rnd_objs] = {1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069,
                                   2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143,
                                   2153, 2161, 2179, 2203, 2207
                                  };

/* Linear congruential pseudo-random generator. */
static unsigned int PYO_RAND_SEED = 1u;
unsigned int pyorand()
{
    PYO_RAND_SEED = (PYO_RAND_SEED * 1664525 + 1013904223) % PYO_RAND_MAX;
    return PYO_RAND_SEED;
}

/** Logging levels. **/
/*********************/

/* Errors should indicate failure to execute a request. */
void
Server_error(Server *self, char * format, ...)
{
    if (self->verbosity & 1)
    {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer, format, args);
        va_end (args);

        PySys_WriteStdout("Pyo error: %s", buffer);
    }
}

/* Messages should print useful or relevant information, or
   information requested by the user. */
void
Server_message(Server *self, char * format, ...)
{
    if (self->verbosity & 2)
    {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer, format, args);
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

    if (self->verbosity & 4)
    {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer, format, args);
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
    if (self->verbosity & 8)
    {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer, format, args);
        va_end (args);

        PySys_WriteStdout("Pyo debug: %s", buffer);
    }
}

/** Manual server. **/
/*********************/
int Server_manual_init(Server *self) { return 0; };
int Server_manual_deinit(Server *self) { return 0; };
int Server_manual_start(Server *self) { return 0; }
int Server_manual_stop(Server *self) { return 0; }

void
Server_manual_process(Server *self)
{
    if (self->audio_be_type == PyoManual && self->server_started == 1)
        Server_process_buffers(self);
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

    for (i = 0; i < (self->bufferSize * self->nchnls); i++)
    {
        out[i] = self->output_buffer[i];
    }

    for (i = 0; i < self->bufferSize; i++)
    {
        for (j = 0; j < self->nchnls; j++)
        {
            self->output_buffer[(j * self->bufferSize) + i] = out[(i * self->nchnls) + j];
        }
    }

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
    float *out = server->output_buffer;
    MYFLT buffer[server->nchnls][server->bufferSize];
    int i, j, chnl;
    MYFLT amp = server->amp;
    Stream *stream_tmp;
    MYFLT *data;

    memset(&buffer, 0, sizeof(buffer));

    PyGILState_STATE s = PyGILState_Ensure();

    if (server->CALLBACK != NULL)
        PyObject_Call((PyObject *)server->CALLBACK, PyTuple_New(0), NULL);

    for (i = 0; i < server->stream_count; i++)
    {
        stream_tmp = (Stream *)PyList_GET_ITEM(server->streams, i);

        if (Stream_getStreamActive(stream_tmp) == 1)
        {
            Stream_callFunction(stream_tmp);

            if (Stream_getStreamToDac(stream_tmp) != 0)
            {
                data = Stream_getData(stream_tmp);
                chnl = Stream_getStreamChnl(stream_tmp);

                for (j = 0; j < server->bufferSize; j++)
                {
                    buffer[chnl][j] += *data++;
                }
            }

            if (Stream_getDuration(stream_tmp) != 0)
            {
                Stream_IncrementDurationCount(stream_tmp);
            }
        }
        else if (Stream_getBufferCountWait(stream_tmp) != 0)
            Stream_IncrementBufferCount(stream_tmp);
    }

    PyGILState_Release(s);

    // OPtimization: Server's amp could be removed.
    if (amp != server->lastAmp)
    {
        server->timeCount = 0;
        server->stepVal = (amp - server->currentAmp) / server->timeStep;
        server->lastAmp = amp;
    }

    for (i = 0; i < server->bufferSize; i++)
    {
        if (server->timeCount < server->timeStep)
        {
            server->currentAmp += server->stepVal;
            server->timeCount++;
        }

        for (j = 0; j < server->nchnls; j++)
        {
            out[(i * server->nchnls) + j] = (float)buffer[j][i] * server->currentAmp;
        }
    }
}

static int
Server_traverse(Server *self, visitproc visit, void *arg)
{
    if (self->CALLBACK != NULL)
        Py_VISIT(self->CALLBACK);

    Py_VISIT(self->streams);
    return 0;
}

static int
Server_clear(Server *self)
{
    if (self->CALLBACK != NULL)
        Py_CLEAR(self->CALLBACK);

    Py_CLEAR(self->streams);
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
                                      &samplingRate, &nchnls, &bufferSize, &duplex, &audioType, &serverName, &ichnls, &midiType))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }

    /* find the first free serverID */
    for (serverID = 0; serverID < MAX_NBR_SERVER; serverID++)
    {
        if (my_server[serverID] == NULL)
        {
            break;
        }
    }

    if(serverID == MAX_NBR_SERVER)
    {
        PyErr_SetString(PyExc_RuntimeError, "You are already using the maximum number of server allowed!\n");
        Py_RETURN_NONE;
    }

    Server *self;
    self = (Server *)type->tp_alloc(type, 0);
    self->server_booted = 0;
    self->audio_be_data = NULL;
    self->serverName = (char *) calloc(32, sizeof(char));
    self->streams = PyList_New(0);
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
    self->amp = self->resetAmp = 1.;
    self->currentAmp = self->lastAmp = 0.; // If set to 0, there is a 5ms fadein at server start.
    self->verbosity = 7;
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

    if (strcmp(audioType, "jack") == 0)
    {
        self->audio_be_type = PyoJack;
    }
    else if (strcmp(audioType, "portaudio") == 0 || strcmp(audioType, "pa") == 0 )
    {
        self->audio_be_type = PyoPortaudio;
    }
    else if (strcmp(audioType, "coreaudio") == 0)
    {
        self->audio_be_type = PyoCoreaudio;
    }
    else if (strcmp(audioType, "offline") == 0)
    {
        self->audio_be_type = PyoOffline;
    }
    else if (strcmp(audioType, "offline_nb") == 0)
    {
        self->audio_be_type = PyoOfflineNB;
    }
    else if (strcmp(audioType, "embedded") == 0)
    {
        self->audio_be_type = PyoEmbedded;
    }
    else if (strcmp(audioType, "manual") == 0)
    {
        self->audio_be_type = PyoManual;
    }
    else
    {
        Server_warning(self, "Unknown audio type. Using Portaudio\n");
        self->audio_be_type = PyoPortaudio;
    }

    strncpy(self->serverName, serverName, 32);

    if (strlen(serverName) > 31)
    {
        self->serverName[31] = '\0';
    }

    return 0;
}

/** Server's setters. **/
/***********************/

static PyObject *
Server_setInputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change input offset when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL)
    {
        if (PyLong_Check(arg))
            self->input_offset = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setOutputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change output offset when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL)
    {
        if (PyLong_Check(arg))
            self->output_offset = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setInputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL)
    {
        if (PyLong_Check(arg))
            self->input = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setInOutDevice(Server *self, PyObject *arg)
{
    if (arg != NULL)
    {
        if (PyLong_Check(arg))
        {
            self->input = PyLong_AsLong(arg);
            self->output = PyLong_AsLong(arg);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setOutputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL)
    {
        if (PyLong_Check(arg))
            self->output = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setSamplingRate(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change sampling rate when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL && PyNumber_Check(arg))
    {
        self->samplingRate = PyFloat_AsDouble(arg);
    }
    else
    {
        Server_error(self, "Sampling rate must be a number.\n");
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setNchnls(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change number of channels when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL && PyLong_Check(arg))
    {
        self->nchnls = PyLong_AsLong(arg);
    }
    else
    {
        Server_error(self, "Number of channels must be an integer.\n");
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setIchnls(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change number of input channels when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL && PyLong_Check(arg))
    {
        self->ichnls = PyLong_AsLong(arg);
    }
    else
    {
        Server_error(self, "Number of input channels must be an integer.\n");
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setBufferSize(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change buffer size when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL && PyLong_Check(arg))
    {
        self->bufferSize = PyLong_AsLong(arg);
    }
    else
    {
        Server_error(self, "Buffer size must be an integer.\n");
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalDur(Server *self, PyObject *arg)
{
    if (arg != NULL && PyNumber_Check(arg))
    {
        self->globalDur = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalDel(Server *self, PyObject *arg)
{
    if (arg != NULL && PyNumber_Check(arg))
    {
        self->globalDel = PyFloat_AsDouble(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setDuplex(Server *self, PyObject *arg)
{
    if (self->server_booted)
    {
        Server_warning(self, "Can't change duplex mode when the Server is already booted.\n");
        Py_RETURN_NONE;
    }

    if (arg != NULL)
    {
        if (PyLong_Check(arg))
            self->duplex = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setGlobalSeed(Server *self, PyObject *arg)
{
    self->globalSeed = 0;

    if (arg != NULL && PyLong_Check(arg))
    {
        self->globalSeed = (int)PyLong_AsLong(arg);
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

    if (self->globalSeed > 0)
    {
        curseed = (self->globalSeed + count * mult) % PYO_RAND_MAX;
    }
    else
    {
        ltime = (unsigned int)time(NULL);
        curseed = (ltime * ltime + count * mult) % PYO_RAND_MAX;
    }

    PYO_RAND_SEED = curseed;

    return 0;
}

static PyObject *
Server_setAmp(Server *self, PyObject *arg)
{
    if (arg != NULL)
    {
        int check = PyNumber_Check(arg);

        if (check)
        {
            self->amp = PyFloat_AsDouble(arg);

            if (self->amp != 0.0)
                self->resetAmp = self->amp;
        }
    }

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
    if (arg != NULL)
    {
        int check = PyLong_Check(arg);

        if (check)
        {
            self->verbosity = PyLong_AsLong(arg);
        }
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_setStartOffset(Server *self, PyObject *arg)
{
    if (arg != NULL)
    {
        int check = PyNumber_Check(arg);

        if (check)
        {
            self->startoffset = PyFloat_AsDouble(arg);
        }
    }

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

    if (self->server_booted == 0)
    {
        Server_error(self, "The Server must be booted!\n");
        Py_RETURN_NONE;
    }

    if (self->server_started == 1)
    {
        Server_stop((Server *)self);
    }

    for (i = 0; i < num_rnd_objs; i++)
    {
        rnd_objs_count[i] = 0;
    }

    switch (self->audio_be_type)
    {
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

        case PyoManual:
            ret = Server_manual_deinit(self);
            break;
    }

    self->server_booted = 0;

    if (ret < 0)
    {
        Server_error(self, "Error closing audio backend.\n");
    }

    /* Cleaning list of audio streams.
       Note: Grabbing the GIL crashes embedded servers. */
    if (self->audio_be_type != PyoEmbedded)
    {
        s = PyGILState_Ensure();
    }

    if (PyList_Size(self->streams) > 0)
    {
        for (i = PyList_Size(self->streams); i > 0; i--)
        {
            PySequence_DelItem(self->streams, i - 1);
        }
    }

    self->stream_count = 0;

    if (self->audio_be_type != PyoEmbedded)
    {
        PyGILState_Release(s);
    }

    Py_RETURN_NONE;
}

PyObject *
Server_boot(Server *self, PyObject *arg)
{
    int i, audioerr = 0, midierr = 0;

    if (self->server_booted == 1)
    {
        Server_error(self, "Server already booted!\n");
        Py_RETURN_NONE;
    }

    self->server_started = 0;
    self->stream_count = 0;

    /* Ensure Python is set up for threading */
    if (!PyEval_ThreadsInitialized())
    {
        PyEval_InitThreads();
    }

    int needNewBuffer = 0;

    if (arg != NULL && PyBool_Check(arg))
    {
        needNewBuffer = PyObject_IsTrue(arg);
    }
    else
    {
        Server_error(self, "The argument to set for a new buffer must be a boolean.\n");
    }

    Server_debug(self, "Streams list size at Server boot (must always be 0) = %d\n",
                 PyList_Size(self->streams));

    switch (self->audio_be_type)
    {
        case PyoPortaudio:
            audioerr = Server_pa_init(self);

            if (audioerr < 0)
            {
                Server_pa_deinit(self);

                if (audioerr == -10)
                    Server_error(self, "Pyo built without Portaudio support\n");
            }

            break;

        case PyoJack:
            audioerr = Server_jack_init(self);

            if (audioerr < 0)
            {
                Server_jack_deinit(self);

                if (audioerr == -10)
                    Server_error(self, "Pyo built without Jack support\n");
            }

            break;

        case PyoCoreaudio:
            audioerr = Server_coreaudio_init(self);

            if (audioerr < 0)
            {
                Server_coreaudio_deinit(self);

                if (audioerr == -10)
                    Server_error(self, "Pyo built without Coreaudio support\n");
            }

            break;

        case PyoOffline:
            audioerr = Server_offline_init(self);

            if (audioerr < 0)
            {
                Server_offline_deinit(self);
            }

            break;

        case PyoOfflineNB:
            audioerr = Server_offline_init(self);

            if (audioerr < 0)
            {
                Server_offline_deinit(self);
            }

            break;

        case PyoEmbedded:
            audioerr = Server_embedded_init(self);

            if (audioerr < 0)
            {
                Server_embedded_deinit(self);
            }

            break;

        case PyoManual:
            audioerr = Server_manual_init(self);

            if (audioerr < 0)
            {
                Server_manual_deinit(self);
            }

            break;
    }

    if (needNewBuffer == 1)
    {
        /* Must allocate buffer after initializing the audio backend in case parameters change there */
        if (self->input_buffer)
        {
            free(self->input_buffer);
        }

        self->input_buffer = (MYFLT *)calloc(self->bufferSize * self->ichnls, sizeof(MYFLT));

        if (self->output_buffer)
        {
            free(self->output_buffer);
        }

        self->output_buffer = (float *)calloc(self->bufferSize * self->nchnls, sizeof(float));
    }

    for (i = 0; i < self->bufferSize * self->ichnls; i++)
    {
        self->input_buffer[i] = 0.0;
    }

    for (i = 0; i < self->bufferSize * self->nchnls; i++)
    {
        self->output_buffer[i] = 0.0;
    }

    if (audioerr == 0)
    {
        self->server_booted = 1;
    }
    else
    {
        self->server_booted = 0;
        Server_error(self, "\nServer not booted.\n");
    }

    Py_RETURN_NONE;
}

PyObject *
Server_start(Server *self)
{
    int err = -1;

    if (self->server_started == 1)
    {
        Server_warning(self, "Server already started!\n");
        Py_RETURN_NONE;
    }

    if (self->server_booted == 0)
    {
        Server_warning(self, "The Server must be booted before calling the start method!\n");
        Py_RETURN_NONE;
    }

    Server_debug(self, "Number of streams at Server start = %d\n", self->stream_count);

    self->server_stopped = 0;
    self->server_started = 1;
    self->timeStep = (int)(0.005 * self->samplingRate);

    if (self->startoffset > 0.0)
    {
        Server_message(self, "Rendering %.2f seconds offline...\n", self->startoffset);
        int numBlocks = ceil(self->startoffset * self->samplingRate / self->bufferSize);
        self->lastAmp = 1.0;
        self->amp = 0.0;

        while (numBlocks-- > 0)
        {
            offline_process_block((Server *) self);
        }

        Server_message(self, "Offline rendering completed. Start realtime processing.\n");
        self->startoffset = 0.0;
    }

    self->amp = self->resetAmp;

    switch (self->audio_be_type)
    {
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

        case PyoManual:
            err = Server_manual_start(self);
            break;
    }

    if (err)
    {
        Server_error(self, "Error starting server.\n");
    }

    Py_RETURN_NONE;
}

PyObject *
Server_stop(Server *self)
{
    int err = 0;

    if (self->server_started == 0)
    {
        Server_warning(self, "The Server must be started!\n");
        Py_RETURN_NONE;
    }

    switch (self->audio_be_type)
    {
        case PyoPortaudio:
            err = Server_pa_stop(self);
            break;

        case PyoCoreaudio:
            err = Server_coreaudio_stop(self);
            break;

        case PyoJack:
            err = Server_jack_stop(self);
            break;

        case PyoOffline:
            err = Server_offline_stop(self);
            break;

        case PyoOfflineNB:
            err = Server_offline_stop(self);
            break;

        case PyoEmbedded:
            err = Server_embedded_stop(self);
            break;

        case PyoManual:
            err = Server_manual_stop(self);
            break;
    }

    if (err != 0)
    {
        Server_error(self, "Error stopping server.\n");
    }
    else
    {
        self->server_stopped = 1;
        self->server_started = 0;
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_addStream(Server *self, PyObject *args)
{
    PyObject *tmp;

    if (! PyArg_ParseTuple(args, "O", &tmp))
        return PyLong_FromLong(-1);

    if (tmp == NULL)
    {
        Server_error(self, "Server_addStream function needs a PyoObject as argument.\n");
        return PyLong_FromLong(-1);
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

    if (self->audio_be_type != PyoEmbedded)
    {
        s = PyGILState_Ensure();
    }

    if (my_server[self->thisServerID] != NULL && PySequence_Size(self->streams) != -1)
    {
        for (i = 0; i < self->stream_count; i++)
        {
            stream_tmp = (Stream *)PyList_GetItem(self->streams, i);

            if (stream_tmp != NULL)
            {
                sid = Stream_getStreamId(stream_tmp);

                if (sid == id)
                {
                    Server_debug(self, "Removed stream id %d\n", id);
                    PySequence_DelItem(self->streams, i);
                    self->stream_count--;
                    break;
                }
            }
        }
    }

    if (self->audio_be_type != PyoEmbedded)
    {
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
        return PyLong_FromLong(-1);

    rsid = Stream_getStreamId(ref_stream_tmp);
    csid = Stream_getStreamId(cur_stream_tmp);

    for (i = 0; i < self->stream_count; i++)
    {
        stream_tmp = (Stream *)PyList_GET_ITEM(self->streams, i);
        sid = Stream_getStreamId(stream_tmp);

        if (sid == csid)
        {
            PySequence_DelItem(self->streams, i);
            self->stream_count--;
            break;
        }
    }

    for (i = 0; i < self->stream_count; i++)
    {
        stream_tmp = (Stream *)PyList_GET_ITEM(self->streams, i);
        sid = Stream_getStreamId(stream_tmp);

        if (sid == rsid)
        {
            break;
        }
    }

    Py_INCREF(cur_stream_tmp);
    PyList_Insert(self->streams, i, (PyObject *)cur_stream_tmp);
    self->stream_count++;

    Py_RETURN_NONE;
}

MYFLT *
Server_getInputBuffer(Server *self)
{
    return (MYFLT *)self->input_buffer;
}

int
Server_getCurrentResamplingFactor(Server *self)
{
    return self->currentResampling;
}

int
Server_getLastResamplingFactor(Server *self)
{
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
    return PyLong_FromLong(self->nchnls);
}

static PyObject *
Server_getIchnls(Server *self)
{
    return PyLong_FromLong(self->ichnls);
}

static PyObject *
Server_getGlobalSeed(Server *self)
{
    return PyLong_FromLong(self->globalSeed);
}

static PyObject *
Server_getBufferSize(Server *self)
{
    if (self->currentResampling < 0)
        return PyLong_FromLong(self->bufferSize / -self->currentResampling);
    else
        return PyLong_FromLong(self->bufferSize * self->currentResampling);
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
    if (PyLong_Check(arg))
    {
        self->lastResampling = self->currentResampling;
        self->currentResampling = PyLong_AsLong(arg);
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
    return PyLong_FromLong(self->server_started);
}

static PyObject *
Server_getIsBooted(Server *self)
{
    return PyLong_FromLong(self->server_booted);
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
    return PyLong_FromLong(serverID);
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
    return PyLong_FromLong(self->thisServerID);
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
Server_getCurrentAmp(Server *self)
{
    PyObject *amplist;
    float rms[self->nchnls];
    float *out = self->output_buffer;
    float outAmp;
    int i, j;

    for (j = 0; j < self->nchnls; j++)
    {
        rms[j] = 0.0;

        for (i = 0; i < self->bufferSize; i++)
        {
            outAmp = out[(i * self->nchnls) + j];
            outAmp *= outAmp;

            if (outAmp > rms[j])
                rms[j] = outAmp;
        }
    }

    amplist = PyTuple_New(self->nchnls);

    for (i = 0; i < self->nchnls; i++)
    {
        PyTuple_SET_ITEM(amplist, i, PyFloat_FromDouble(rms[i]));
    }

    return amplist;
}

static PyObject *
Server_setAutoStartChildren(Server *self, PyObject *arg)
{
    if (PyLong_Check(arg))
    {
        self->autoStartChildren = PyLong_AsLong(arg);
    }

    Py_RETURN_NONE;
}

static PyObject *
Server_getAutoStartChildren(Server *self)
{
    return PyLong_FromLong(self->autoStartChildren);
}

static PyObject *
Server_manualProcess(Server *self)
{
    Server_manual_process(self);

    Py_RETURN_NONE;
}

static PyMethodDef Server_methods[] =
{
    {"setInputDevice", (PyCFunction)Server_setInputDevice, METH_O, "Sets audio input device."},
    {"setOutputDevice", (PyCFunction)Server_setOutputDevice, METH_O, "Sets audio output device."},
    {"setInputOffset", (PyCFunction)Server_setInputOffset, METH_O, "Sets audio input channel offset."},
    {"setOutputOffset", (PyCFunction)Server_setOutputOffset, METH_O, "Sets audio output channel offset."},
    {"setInOutDevice", (PyCFunction)Server_setInOutDevice, METH_O, "Sets both audio input and output device."},
    {"setSamplingRate", (PyCFunction)Server_setSamplingRate, METH_O, "Sets the server's sampling rate."},
    {"setBufferSize", (PyCFunction)Server_setBufferSize, METH_O, "Sets the server's buffer size."},
    {"setGlobalDur", (PyCFunction)Server_setGlobalDur, METH_O, "Sets the server's globalDur attribute."},
    {"setGlobalDel", (PyCFunction)Server_setGlobalDel, METH_O, "Sets the server's globalDel attribute."},
    {"beginResamplingBlock", (PyCFunction)Server_beginResamplingBlock, METH_O, "Starts a resampling code block."},
    {"endResamplingBlock", (PyCFunction)Server_endResamplingBlock, METH_NOARGS, "Stops a resampling code block."},
    {"setNchnls", (PyCFunction)Server_setNchnls, METH_O, "Sets the server's number of output/input channels."},
    {"setIchnls", (PyCFunction)Server_setIchnls, METH_O, "Sets the server's number of input channels."},
    {"setDuplex", (PyCFunction)Server_setDuplex, METH_O, "Sets the server's duplex mode (0 = only out, 1 = in/out)."},
    {"setGlobalSeed", (PyCFunction)Server_setGlobalSeed, METH_O, "Sets the server's global seed for random objects."},
    {"setAmp", (PyCFunction)Server_setAmp, METH_O, "Sets the overall amplitude."},
    {"setCallback", (PyCFunction)Server_setCallback, METH_O, "Sets the Server's CALLBACK callable object."},
    {"setVerbosity", (PyCFunction)Server_setVerbosity, METH_O, "Sets the verbosity."},
    {"setStartOffset", (PyCFunction)Server_setStartOffset, METH_O, "Sets starting time offset."},
    {"boot", (PyCFunction)Server_boot, METH_O, "Setup and boot the server."},
    {"shutdown", (PyCFunction)Server_shutdown, METH_NOARGS, "Shut down the server."},
    {"start", (PyCFunction)Server_start, METH_NOARGS, "Starts the server's callback loop."},
    {"stop", (PyCFunction)Server_stop, METH_NOARGS, "Stops the server's callback loop."},
    {"addStream", (PyCFunction)Server_addStream, METH_VARARGS, "Adds an audio stream to the server."},
    {"removeStream", (PyCFunction)Server_removeStream, METH_VARARGS, "Removes an audio stream from the server."},
    {"changeStreamPosition", (PyCFunction)Server_changeStreamPosition, METH_VARARGS, "Puts an audio stream before another one in the stack."},
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
    {"setServer", (PyCFunction)Server_setServer, METH_NOARGS, "Sets this server as the one to use for new objects when using the embedded device"},
    {"getInputAddr", (PyCFunction)Server_getInputAddr, METH_NOARGS, "Get the embedded device input buffer memory address"},
    {"getOutputAddr", (PyCFunction)Server_getOutputAddr, METH_NOARGS, "Get the embedded device output buffer memory address"},
    {"getServerID", (PyCFunction)Server_getServerID, METH_NOARGS, "Get the embedded device server memory address"},
    {"getServerAddr", (PyCFunction)Server_getServerAddr, METH_NOARGS, "Get the embedded device server memory address"},
    {"getEmbedICallbackAddr", (PyCFunction)Server_getEmbedICallbackAddr, METH_NOARGS, "Get the embedded device interleaved callback method memory address"},
    {"getCurrentAmp", (PyCFunction)Server_getCurrentAmp, METH_NOARGS, "Get the current global amplitudes as a list of floats."},
    {"setAutoStartChildren", (PyCFunction)Server_setAutoStartChildren, METH_O, "Sets autoStartChildren attribute."},
    {"getAutoStartChildren", (PyCFunction)Server_getAutoStartChildren, METH_NOARGS, "Gets autoStartChildren attribute."},
    {"process", (PyCFunction)Server_manualProcess, METH_NOARGS, "Compute one buffer size of samples in manual mode."},
    {NULL}  /* Sentinel */
};

static PyMemberDef Server_members[] =
{
    {"streams", T_OBJECT_EX, offsetof(Server, streams), 0, "Server's streams list."},
    {NULL}  /* Sentinel */
};

PyTypeObject ServerType =
{
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
