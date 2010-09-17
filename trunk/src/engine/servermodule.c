/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>

#include "structmember.h"
#include "portaudio.h"
#include "portmidi.h"
#include "sndfile.h"
#include "streammodule.h"
#include "pyomodule.h"
#include "servermodule.h"

static Server *my_server = NULL;
static PyObject *Server_shut_down(Server *self);
static PyObject *Server_stop(Server *self);
static void Server_process_gui(Server *server, float *out);
static inline void Server_process_buffers(Server *server, const void *inputBuffer, void *outputBuffer);
void Server_debug(char * format, ...);
void Server_message(char * format, ...);
void Server_warning(char * format, ...);

/* Portmidi get input events */
static void portmidiGetEvents(Server *self) 
{
    PmError result;
    PmEvent buffer;

    do {
        result = Pm_Poll(self->in);
        if (result) {
            if (Pm_Read(self->in, &buffer, 1) == pmBufferOverflow) 
                continue;
            self->midiEvents[self->midi_count++] = buffer;
        }    
    } while (result);  
}

/* Portaudio stuff */
static void portaudio_assert(PaError ecode, const char* cmdName) {
    if (ecode != paNoError) {
        const char* eText = Pa_GetErrorText(ecode);
        if (!eText) {
            eText = "???";
        }
        fprintf(stderr, "portaudio error in %s: %s\n", cmdName, eText);
        Pa_Terminate();
    }
}

/* Portaudio callback function */
static int 
pa_callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *arg )
{
    float *out = (float *)outputBuffer;
    Server *server = (Server *) arg;

    assert(framesPerBuffer == server->bufferSize);
    int i;
    int nchnls = server->nchnls;
    
    /* avoid unused variable warnings */
    (void) timeInfo;
    (void) statusFlags;

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }
    
    if (server->duplex == 1) {
        float *in = (float *)inputBuffer;
        for (i=0; i<server->bufferSize*server->nchnls; i++) {
            my_server->input_buffer[i] = (MYFLT)in[i];
        }
    }    
    else 
        (void) inputBuffer;

    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server, inputBuffer, outputBuffer);

    if (server->withGUI == 1 && nchnls <= 8) {
        Server_process_gui(server, out);
    }    
        
    PyGILState_Release(s);
    server->midi_count = 0;
    
    if (server->server_started == 1) {
        if (server->server_stopped == 1 && server->currentAmp < 0.0001)
            server->server_started = 0;
        return paContinue;
    }    
    else {
        return paComplete;
    }
}

#ifdef USE_JACK
/* Jack callbacks */

static int
jack_callback (jack_nframes_t nframes, void *arg)
{
    int i, j;
    Server *server = (Server *) arg;
    assert(nframes == server->bufferSize);
    jack_default_audio_sample_t *in_buffers[server->nchnls], *out_buffers[server->nchnls];

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }
    PyoJackBackendData *be_data = (PyoJackBackendData *) server->audio_be_data;
    for (i = 0; i < server->nchnls; i++) {
        in_buffers[i] = jack_port_get_buffer (be_data->jack_in_ports[i], server->bufferSize);
    }
    for (i = 0; i < server->nchnls; i++) {
        out_buffers[i] = jack_port_get_buffer (be_data->jack_out_ports[i], server->bufferSize);
        
    }
    /* jack audio data is not interleaved */
    if (server->duplex == 1) {
        for (i=0; i<server->bufferSize; i++) {
            for (j=0; j<server->nchnls; j++) {
                server->input_buffer[(j*server->bufferSize) + i] = (MYFLT) in_buffers[j][i];
            }
        }
    }
    MYFLT outputBuffer[server->nchnls*server->bufferSize];
    memset(outputBuffer,0, sizeof(MYFLT)*server->nchnls*server->bufferSize);
    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server, server->input_buffer, outputBuffer);
    if (server->withGUI == 1 && server->nchnls <= 8) {
        Server_process_gui(server, outputBuffer);
    }
    PyGILState_Release(s);
    for (i=0; i<server->bufferSize; i++) {
        for (j=0; j<server->nchnls; j++) {
            out_buffers[j][i] = (jack_default_audio_sample_t) outputBuffer[(j*server->bufferSize) + i];
        }
    }
    server->midi_count = 0;
//     if (server->server_started == 1) {
//         if (server->server_stopped == 1 && server->currentAmp < 0.0001)
//             server->server_started = 0;
//     }  
    return 0;    
}

static int
jack_srate_cb (jack_nframes_t nframes, void *arg)
{
    Server *s = (Server *) arg;
    s->samplingRate = (float) nframes;
    printf ("the sample rate is now %lu/sec\n", (unsigned long) nframes);
    return 0;
}

static int
jack_bufsize_cb (jack_nframes_t nframes, void *arg)
{
    Server *s = (Server *) arg;
    s->bufferSize = (int) nframes;
    printf ("the buffer size is now %lu/sec\n", (unsigned long) nframes);
    return 0;
}

static void
jack_error_cb (const char *desc)
{
    printf ( "JACK error: %s\n", desc);
}

static void
jack_shutdown_cb (void *arg)
{
    Server *s = (Server *) arg;
    Server_shut_down(s);
    printf ( "JACK server shutdown. Pyo Server shut down.\n");
}

#endif

/* Server audio backend init functions */

int
Server_pa_init(Server *self)
{
    PaError err;

    err = Pa_Initialize();
    portaudio_assert(err, "Pa_Initialize");

    int n = Pa_GetDeviceCount();
    if (n < 0) {
        portaudio_assert(n, "Pa_GetDeviceCount");
    }

    // -- setup input and output -- 
    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    if (self->input == -1)
        inputParameters.device = Pa_GetDefaultInputDevice(); // default input device
    else
        inputParameters.device = self->input; // selected input device
    inputParameters.channelCount = self->nchnls;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaStreamParameters outputParameters;
    memset(&outputParameters, 0, sizeof(outputParameters));
    if (self->output == -1)
        outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device 
    else
        outputParameters.device = self->output; // selected output device 
    outputParameters.channelCount = self->nchnls;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    //err = Pa_IsFormatSupported(&inputParameters, &outputParameters, self->samplingRate);
    //portaudio_assert(err, "Pa_IsFormatSupported");


    if (self->input == -1 && self->output == -1) {
        if (self->duplex == 1)
            err = Pa_OpenDefaultStream(&self->stream, self->nchnls, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, pa_callback, (void *) self);
        else
            err = Pa_OpenDefaultStream(&self->stream, 0, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, pa_callback, (void *) self);
    }
    else
        err = Pa_OpenStream(&self->stream, &inputParameters, &outputParameters, self->samplingRate, self->bufferSize, paNoFlag, pa_callback,  (void *) self);
    portaudio_assert(err, "Pa_OpenStream");
    if (err < 0) {
        printf("Portaudio error: %s", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

int
Server_pa_deinit(Server *self)
{
    PaError err;

    if (Pa_IsStreamActive(self->stream) || ! Pa_IsStreamStopped(self->stream)) {
        err = Pa_StopStream(self->stream);
        portaudio_assert(err, "Pa_StopStream");
    }
    
    err = Pa_CloseStream(self->stream);
    portaudio_assert(err, "Pa_CloseStream");
    
    err = Pa_Terminate();
    portaudio_assert(err, "Pa_Terminate");
    
    return err;
}

int 
Server_pa_start(Server *self)
{
    PaError err;

    if (Pa_IsStreamActive(self->stream) || ! Pa_IsStreamStopped(self->stream)) {
        err = Pa_StopStream(self->stream);
        portaudio_assert(err, "Pa_StopStream");
    }
    err = Pa_StartStream(self->stream);
    portaudio_assert(err, "Pa_StartStream");
    return err;
}

#ifdef USE_JACK

int
Server_jack_autoconnect (Server *self)
{
    const char **ports;
    int ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    if ((ports = jack_get_ports (be_data->jack_client, NULL, NULL, 
        JackPortIsPhysical|JackPortIsOutput)) == NULL) {
        printf("Jack error: Cannot find any physical capture ports\n");
        ret = -1;
    }
    int i=0;
    while(ports[i]!=NULL && be_data->jack_in_ports[i] != NULL){
        if (jack_connect (be_data->jack_client, ports[i], jack_port_name(be_data->jack_in_ports[i]))) {
            printf ("Jack warning: cannot connect input ports\n");
            ret = -1;
        }
        i++;
    }
    free (ports);
    
    if ((ports = jack_get_ports (be_data->jack_client, NULL, NULL, 
        JackPortIsPhysical|JackPortIsInput)) == NULL) {
        printf("Jack error: Cannot find any physical playback ports\n");
        ret = -1;
    }
    
    i=0;
    while(ports[i]!=NULL && be_data->jack_out_ports[i] != NULL){
        if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
            printf ("Jack warning: cannot connect output ports\n");
            ret = -1;
        }
        i++;
    }
    free (ports);
    return ret;
}

int
Server_jack_init (Server *self)
{   
    const char *client_name = NULL;
    const char *server_name = "server";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    int sampleRate = 0;
    int bufferSize = 0;
    assert(self->audio_be_data == NULL);
    PyoJackBackendData *be_data = (PyoJackBackendData *) malloc(sizeof(PyoJackBackendData *));
    self->audio_be_data = (void *) be_data;
    be_data->jack_in_ports = (jack_port_t **) calloc(self->nchnls, sizeof(jack_port_t *));
    be_data->jack_out_ports = (jack_port_t **) calloc(self->nchnls, sizeof(jack_port_t *));
    
    be_data->jack_client = jack_client_open (self->serverName, options, &status, server_name);
    if (be_data->jack_client == NULL) {
        Server_message("Jack error: jack_client_open() failed, "
        "status = 0x%2.0x\n", status);
        if (status & JackServerFailed) {
            Server_message ("Jack error: Unable to connect to JACK server\n");
        }
        return -1;
    }
    if (status & JackServerStarted) {
        Server_message("JACK server started\n");
    }
    if (status & JackNameNotUnique) {
        client_name = jack_get_client_name(be_data->jack_client);
        Server_message("Jack unique name `%s' assigned\n", client_name);
    }
    
    sampleRate = jack_get_sample_rate (be_data->jack_client);
    if (sampleRate != self->samplingRate) {
        self->samplingRate = sampleRate;
        Server_message("Sample rate set to Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    else {
        Server_message("Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    if (sampleRate <= 0) {
        Server_message("Invalid Jack engine sample rate.");
        jack_client_close (be_data->jack_client);
        return -1;
    }
    bufferSize = jack_get_buffer_size(be_data->jack_client);
    if (bufferSize != self->bufferSize) {
        self->bufferSize = bufferSize;
        Server_message("Buffer size set to Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }
    else {
        Server_message("Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }
    int nchnls = self->nchnls;
    
    while (nchnls-- > 0) {
        char name[16];
        int ret;
        int index = self->nchnls - nchnls - 1;
        ret = sprintf(name, "input_%i", index + 1);
        if (ret > 0) {
            be_data->jack_in_ports[index]
            = jack_port_register (be_data->jack_client, name, 
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }
        ret = sprintf(name, "output_%i", self->nchnls - nchnls);
        if (ret > 0) {
            be_data->jack_out_ports[index] 
            = jack_port_register (be_data->jack_client, name, 
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }
        if ((be_data->jack_in_ports[index] == NULL) || (be_data->jack_out_ports[index] == NULL)) {
            printf("Jack error: no more JACK ports available\n");
            return -1;
        }
    }
    jack_set_error_function (jack_error_cb);
    jack_set_sample_rate_callback(be_data->jack_client, jack_srate_cb, (void *) self);
    jack_on_shutdown (be_data->jack_client, jack_shutdown_cb, (void *) self);
    jack_set_buffer_size_callback (be_data->jack_client, jack_bufsize_cb, (void *) self);
    return 0;
}

int 
Server_jack_deinit (Server *self)
{
    int ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    ret = jack_client_close(be_data->jack_client);
    free(be_data->jack_in_ports);
    free(be_data->jack_out_ports);
    free(self->audio_be_data);
    return ret;
}

int 
Server_jack_start (Server *self)
{
#ifdef USE_JACK
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    jack_set_process_callback(be_data->jack_client, jack_callback, (void *) self);
    if (jack_activate (be_data->jack_client)) {
        printf ("Jack error: cannot activate jack client.\n");
        jack_client_close (be_data->jack_client);
        Server_shut_down(self);
        return -1;
    }
    Server_jack_autoconnect(self);
    return 0;
#else
    printf ("Pyo compiled without Jack support.\n");
    return -1;
#endif
}

#endif


/***************************************************/
/*  Main Processing functions                      */

static inline void
Server_process_buffers(Server *server, const void *inputBuffer, void *outputBuffer)
{
    float *out = (float*)outputBuffer;    
    MYFLT buffer[server->nchnls][server->bufferSize];
    int i, j, chnl;
    int count = server->stream_count;
    MYFLT amp = my_server->amp;
    Stream *stream_tmp;
    MYFLT *data;
    
    memset(&buffer, 0, sizeof(buffer));
    for (i=0; i<count; i++) {
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
    if (server->record == 1)
        sf_write_float(server->recfile, out, server->bufferSize * server->nchnls);
}

static void
Server_process_gui(Server *server, float *out)
{
    float rms[server->nchnls];
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
        }        
        server->gcount = 0;
    }
}

void
Server_debug(char * format, ...)
{    
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);

    printf("%s",buffer);
}

void
Server_message(char * format, ...)
{    
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);

    printf("%s",buffer);
}

void
Server_warning(char * format, ...)
{    
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);

    PyErr_Warn(NULL, buffer);
}

/***************************************************/

/* Global function called by any new audio object to 
 get a pointer to the server */
PyObject *
PyServer_get_server()
{
    return (PyObject *)my_server;
}

static PyObject *
Server_shut_down(Server *self)
{
    int ret = 0;
    if (self->server_booted == 0) {
        PyErr_Warn(NULL, "The Server must be booted!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (self->server_started == 1) {
        Server_stop((Server *)self);
    }
    switch (self->audio_be_type) {
        case PyoPortaudio:
            ret = Server_pa_deinit(self);
            break;
        case PyoJack:
            ret = Server_jack_deinit(self);
            break;
    }
    self->server_booted = 0;
    if (ret < 0) {
        printf("Error closing audio backend.\n");
    }
    
    if (self->withPortMidi == 1) {
        Pm_Close(self->in);
        Pm_Terminate();
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

static int
Server_traverse(Server *self, visitproc visit, void *arg)
{
    Py_VISIT(self->stream);
    Py_VISIT(self->streams);
    return 0;
}

static int 
Server_clear(Server *self)
{    
    Py_CLEAR(self->stream);
    Py_CLEAR(self->streams);
    return 0;
}

static void
Server_dealloc(Server* self)
{  
    Server_shut_down(self);
    Server_clear(self);
    free(self->input_buffer);
    free(self->serverName);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (PyServer_get_server() != NULL) {
        PyErr_Warn(NULL, "A Server is already created!\nIf you put this Server in a new variable, please delete it!");
        return PyServer_get_server();
    }    
    Server *self;
    self = (Server *)type->tp_alloc(type, 0);
    self->server_booted = 0;
    self->audio_be_data = NULL;
    self->serverName = (char *) calloc(32, sizeof(char));
    self->samplingRate = 44100.0;
    self->nchnls = 2;
    self->record = 0;
    self->bufferSize = 256;
    self->duplex = 0;
    self->input = -1;
    self->output = -1;
    self->midi_input = -1;
    self->amp = self->resetAmp = 1.;
    self->currentAmp = self->lastAmp = 0.;
    self->withGUI = 0;
    Py_XDECREF(my_server);
    Py_XINCREF(self);
    my_server = (Server *)self;
    return (PyObject *)self;
}

static int
Server_init(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", "audio", "jackname", NULL};
    
    char *audioType = "portaudio";
    char *serverName = "pyo";

    Server_debug("Server_init. Compiled " TIMESTAMP "\n");  // Only for debugging purposes

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FIIISS, kwlist, 
            &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex, &audioType, &serverName))
        return -1;
    if (strcmp(audioType, "jack") == 0) {
        self->audio_be_type = PyoJack;
    }
    else if (strcmp(audioType, "portaudio") == 0 || strcmp(audioType, "pa") == 0 ) {
        self->audio_be_type = PyoPortaudio;
    }
    else {
        printf("Unknown audio type. Using Portaudio\n");
        self->audio_be_type = PyoPortaudio;
    }
    strncpy(self->serverName, serverName, 32);
    if (strlen(serverName) > 31) {
        self->serverName[31] = '\0';
    }
    self->recpath = getenv("HOME");
    if (self->recpath != NULL)
        strncat(self->recpath, "/pyo_rec.aif", strlen("/pyo_rec.aif"));
    return 0;
}


static PyObject *
Server_setInputDevice(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->input = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setInOutDevice(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->input = PyInt_AsLong(arg);
            self->output = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setOutputDevice(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->output = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setMidiInputDevice(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->midi_input = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setSamplingRate(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        printf("Can't change sampling rate for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->samplingRate = PyInt_AsLong(arg);
    }
    else {
        printf("Error setting sampling rate.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setNchnls(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        printf("Can't change number of channels for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->nchnls = PyInt_AsLong(arg);
    }
    else {
        printf("Error setting number of channels.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setBufferSize(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        printf("Can't change buffer size for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->bufferSize = PyInt_AsLong(arg);
    }
    else {
        printf("Error setting buffer size.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setDuplex(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->duplex = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setAmp(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        int check = PyNumber_Check(arg);
        
        if (check) {
            self->amp = PyFloat_AsDouble(PyNumber_Float(arg));
            if (self->amp != 0.0)
                self->resetAmp = self->amp;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setAmpCallable(Server *self, PyObject *arg)
{
    int i;
    PyObject *tmp;
    
    if (arg == NULL) {
    PyErr_SetString(PyExc_TypeError, "The amplitude callable attribute must be a method.");
            Py_INCREF(Py_None);
            return Py_None;
    }
    
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
    
	Py_INCREF(Py_None);
	return Py_None;
}

int
Server_pm_init(Server *self)
{
   /* Initializing MIDI */    
    PmError pmerr;
    pmerr = Pm_Initialize();
    if (pmerr) {
        printf("PortMidi warning: could not initialize PortMidi: %s\n", Pm_GetErrorText(pmerr));
        self->withPortMidi = 0;
        return -1;
    }    
    else {
        printf("PortMidi initialized.\n");
        self->withPortMidi = 1;
    }    

    if (self->withPortMidi == 1) {
        int num_devices = Pm_CountDevices();
        if (num_devices > 0) {
            if (self->midi_input == -1 || self->midi_input >= num_devices)
                self->midi_input = 0;
            const PmDeviceInfo *info = Pm_GetDeviceInfo(self->midi_input);
            if (info->input) {
                pmerr = Pm_OpenInput(&self->in, self->midi_input, NULL, 100, NULL, NULL);
                if (pmerr) {
                    printf("PortMidi warning: could not open midi input %d (%s): %s\nPortmidi closed\n", 0, info->name, Pm_GetErrorText(pmerr));
                    self->withPortMidi = 0;
                    Pm_Terminate();
                }    
                else
                    printf("Midi Input (%s) opened.\n", info->name);
            }
            else {
                printf("PortMidi warning: Something wrong with midi device!\nPortmidi closed\n");
                self->withPortMidi = 0;
                Pm_Terminate();
            }    
        }    
        else {
            printf("PortMidi warning: No midi device found!\nPortmidi closed\n");
            self->withPortMidi = 0;
            Pm_Terminate();
        }    
    }
    if (self->withPortMidi == 1) {
        self->midi_count = 0;
        Pm_SetFilter(self->in, PM_FILT_ACTIVE | PM_FILT_CLOCK);
    } 
    return 0;
}


static PyObject *
Server_boot(Server *self)
{
    int audioerr = 0, midierr = 0;
    int i;
    if (self->server_booted == 1) {
        PyErr_Warn(NULL, "Server already booted!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    self->server_started = 0;
    self->stream_count = 0;
    midierr = Server_pm_init(self);

    self->streams = PyList_New(0);
    switch (self->audio_be_type) {
        case PyoPortaudio:
            audioerr = Server_pa_init(self);
            break;
        case PyoJack:
#ifdef USE_JACK
            audioerr = Server_jack_init(self); 
#else
            audioerr = -1;
            printf("Pyo built without Jack support\n");
#endif
            break;
    }
    // Must allocate buffer after initializing the audio backend in case parameters change there
    self->input_buffer = (MYFLT *)realloc(self->input_buffer, self->bufferSize * self->nchnls * sizeof(MYFLT));
    for (i=0; i<self->bufferSize*self->nchnls; i++) {
        self->input_buffer[i] = 0.0;
    }
    if (audioerr == 0 && midierr == 0) {
        self->server_booted = 1;
    }
    else {
        printf("Server not booted.\n");
    }    
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start(Server *self)
{
    if (self->server_started == 1) {
        PyErr_Warn(NULL, "Server already started!");
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (self->server_booted == 0) {
        PyErr_Warn(NULL, "The Server must be booted!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    int err = 0;
    
    /* Ensure Python is set up for threading */
    PyEval_InitThreads();
    
    switch (self->audio_be_type) {
        case PyoPortaudio:
            err = Server_pa_start(self);
            break;
        case PyoJack:
            err = Server_jack_start(self);
            break;
    }
    if (err) {
        printf("Error starting server.");
    }

    self->amp = self->resetAmp;
    self->server_stopped = 0;
    self->server_started = 1;
    self->timeStep = (int)(0.01 * self->samplingRate);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_stop(Server *self)
{
    if (self->server_started == 0) {
        PyErr_Warn(NULL, "The Server must be started!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    
    self->timeStep = (int)(0.1 * self->samplingRate);
    self->amp = 0.;
    self->server_stopped = 1;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start_rec(Server *self, PyObject *args, PyObject *kwds)
{    
	char *filename=NULL;
	
    static char *kwlist[] = {"filename", NULL};

	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &filename))
        return PyInt_FromLong(-1);

    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->samplingRate;
    self->recinfo.channels = self->nchnls;
    self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_FLOAT;
    
    /* Open the output file. */
	if (filename == NULL) {
		if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {   
			printf ("Not able to open output file %s.\n", self->recpath);
		}	
    }
	else {
		if (! (self->recfile = sf_open(filename, SFM_WRITE, &self->recinfo))) {   
			printf ("Not able to open output file %s.\n", filename);
		}	
    }
    
    self->record = 1;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_stop_rec(Server *self, PyObject *args)
{
    self->record = 0;
    sf_close(self->recfile);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_addStream(Server *self, PyObject *args)
{
    PyObject *tmp;
    
    if (! PyArg_ParseTuple(args, "O", &tmp))
        return PyInt_FromLong(-1); 
        
    if (tmp == NULL) {
        PyErr_SetString(PyExc_TypeError, "Need a pyo object as argument");
        return PyInt_FromLong(-1);
    }

    Py_INCREF(tmp);
    PyList_Append(self->streams, tmp);

    self->stream_count++;
    
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_removeStream(Server *self, int id)
{
    int i, sid;
    Stream *stream_tmp;
    
    for (i=0; i<self->stream_count; i++) {
        stream_tmp = (Stream *)PyList_GET_ITEM(self->streams, i);
        sid = Stream_getStreamId(stream_tmp);
        if (sid == id) {
            PySequence_DelItem(self->streams, i);
            self->stream_count--;
            break;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;    
}

MYFLT *
Server_getInputBuffer(Server *self) {
    return (MYFLT *)self->input_buffer;
}

PmEvent *
Server_getMidiEventBuffer(Server *self) {
    return (PmEvent *)self->midiEvents;
}

int
Server_getMidiEventCount(Server *self) {
    return self->midi_count;
}

static PyObject *
Server_getSamplingRate(Server *self)
{
    return PyFloat_FromDouble(self->samplingRate);
}

static PyObject *
Server_getNchnls(Server *self)
{
    return PyInt_FromLong(self->nchnls);
}

static PyObject *
Server_getBufferSize(Server *self)
{
    return PyInt_FromLong(self->bufferSize);
}

static PyObject *
Server_getIsStarted(Server *self)
{
    return PyInt_FromLong(self->server_started);
}

static PyObject *
Server_getStreams(Server *self)
{
    Py_INCREF(self->streams);
    return self->streams;
}


static PyMethodDef Server_methods[] = {
    {"setInputDevice", (PyCFunction)Server_setInputDevice, METH_O, "Sets audio input device."},
    {"setOutputDevice", (PyCFunction)Server_setOutputDevice, METH_O, "Sets audio output device."},
    {"setInOutDevice", (PyCFunction)Server_setInOutDevice, METH_O, "Sets both audio input and output device."},
    {"setMidiInputDevice", (PyCFunction)Server_setMidiInputDevice, METH_O, "Sets MIDI input device."},
    {"setSamplingRate", (PyCFunction)Server_setSamplingRate, METH_O, "Sets the server's sampling rate."},
    {"setBufferSize", (PyCFunction)Server_setBufferSize, METH_O, "Sets the server's buffer size."},
    {"setNchnls", (PyCFunction)Server_setNchnls, METH_O, "Sets the server's number of channels."},
    {"setDuplex", (PyCFunction)Server_setDuplex, METH_O, "Sets the server's duplex mode (0 = only out, 1 = in/out)."},
    {"setAmp", (PyCFunction)Server_setAmp, METH_O, "Sets the overall amplitude."},
    {"setAmpCallable", (PyCFunction)Server_setAmpCallable, METH_O, "Sets the Server's GUI object."},
    {"boot", (PyCFunction)Server_boot, METH_NOARGS, "Setup and boot the server."},
    {"shutdown", (PyCFunction)Server_shut_down, METH_NOARGS, "Shut down the server."},
	{"start", (PyCFunction)Server_start, METH_NOARGS, "Starts the server's callback loop."},
    {"stop", (PyCFunction)Server_stop, METH_NOARGS, "Stops the server's callback loop."},
    {"recstart", (PyCFunction)Server_start_rec, METH_VARARGS|METH_KEYWORDS, "Start automatic output recording."},
    {"recstop", (PyCFunction)Server_stop_rec, METH_NOARGS, "Stop automatic output recording."},
    {"addStream", (PyCFunction)Server_addStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
    {"removeStream", (PyCFunction)Server_removeStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
    {"getStreams", (PyCFunction)Server_getStreams, METH_NOARGS, "Returns the list of streams added to the server."},
    {"getSamplingRate", (PyCFunction)Server_getSamplingRate, METH_NOARGS, "Returns the server's sampling rate."},
    {"getNchnls", (PyCFunction)Server_getNchnls, METH_NOARGS, "Returns the server's current number of channels."},
    {"getBufferSize", (PyCFunction)Server_getBufferSize, METH_NOARGS, "Returns the server's buffer size."},
    {"getIsStarted", (PyCFunction)Server_getIsStarted, METH_NOARGS, "Returns 1 if the server is started, otherwise returns 0."},
    {NULL}  /* Sentinel */
};

static PyMemberDef Server_members[] = {
    {"streams", T_OBJECT_EX, offsetof(Server, streams), 0, "Server's streams list."},
    {NULL}  /* Sentinel */
};

PyTypeObject ServerType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Server",         /*tp_name*/
    sizeof(Server),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Server_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
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
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
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
