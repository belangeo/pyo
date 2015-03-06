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
#include "portaudio.h"
#include "portmidi.h"
#include "porttime.h"
#include "sndfile.h"
#include "streammodule.h"
#include "pyomodule.h"
#include "servermodule.h"


#define MAX_NBR_SERVER 256

static Server *my_server[MAX_NBR_SERVER];
static int serverID = 0;

static PyObject *Server_shut_down(Server *self);
static PyObject *Server_stop(Server *self);
static void Server_process_gui(Server *server);
static void Server_process_time(Server *server);
static inline void Server_process_buffers(Server *server);
static int Server_start_rec_internal(Server *self, char *filename);

/* random objects count and multiplier to assign different seed to each instance. */
#define num_rnd_objs 29

int rnd_objs_count[num_rnd_objs] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int rnd_objs_mult[num_rnd_objs] = {1993,1997,1999,2003,2011,2017,2027,2029,2039,2053,2063,2069,
                         2081,2083,2087,2089,2099,2111,2113,2129,2131,2137,2141,2143,2153,2161,2179,2203,2207};

#ifdef USE_COREAUDIO
static int coreaudio_stop_callback(Server *self);
#endif

void
Server_error(Server *self, char * format, ...)
{
    // Errors should indicate failure to execute a request
    if (self->verbosity & 1) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        printf("%s",buffer);
    }
}

void
Server_message(Server *self, char * format, ...)
{
    // Messages should print useful or relevant information, or information requested by the user
    if (self->verbosity & 2) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        printf("%s",buffer);
    }
}

void
Server_warning(Server *self, char * format, ...)
{
    // Warnings should be used when an unexpected or unusual choice was made by pyo
#ifndef NO_MESSAGES
    if (self->verbosity & 4) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);
        printf("%s",buffer);
    }
#endif
}

void
Server_debug(Server *self, char * format, ...)
{
    // Debug messages should print internal information which might be necessary for debugging internal conditions.
    if (self->verbosity & 8) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);

        printf("%s",buffer);
    }
}

/* Portmidi get input events */
static void portmidiGetEvents(Server *self)
{
    int i;
    PmError result;
    PmEvent buffer;

    for (i=0; i<self->midiin_count; i++) {
        do {
            result = Pm_Poll(self->midiin[i]);
            if (result) {
                if (Pm_Read(self->midiin[i], &buffer, 1) == pmBufferOverflow)
                    continue;
                self->midiEvents[self->midi_count++] = buffer;
            }
        } while (result);
    }
}

/* Portaudio stuff */
static void portaudio_assert(PaError ecode, const char* cmdName) {
    if (ecode != paNoError) {
        const char* eText = Pa_GetErrorText(ecode);
        if (!eText) {
            eText = "???";
        }
        printf("portaudio error in %s: %s\n", cmdName, eText);
        Pa_Terminate();
    }
}

/* Portaudio callback function */
static int
pa_callback_interleaved( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *arg )
{
    float *out = (float *)outputBuffer;
    Server *server = (Server *) arg;

    assert(framesPerBuffer == server->bufferSize);
    int i, j, bufchnls, index1, index2;

    /* avoid unused variable warnings */
    (void) timeInfo;
    (void) statusFlags;

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }

    if (server->duplex == 1) {
        float *in = (float *)inputBuffer;
        bufchnls = server->ichnls + server->input_offset;
        for (i=0; i<server->bufferSize; i++) {
            index1 = i * server->ichnls;
            index2 = i * bufchnls + server->input_offset;
            for (j=0; j<server->ichnls; j++) {
                server->input_buffer[index1+j] = (MYFLT)in[index2+j];
            }
        }
    }

    Server_process_buffers(server);
    bufchnls = server->nchnls + server->output_offset;
    for (i=0; i<server->bufferSize; i++) {
        index1 = i * server->nchnls;
        index2 = i * bufchnls + server->output_offset;
        for (j=0; j<server->nchnls; j++) {
            out[index2+j] = (float) server->output_buffer[index1+j];
        }
    }
    server->midi_count = 0;

#ifdef _OSX_
    if (server->server_stopped == 1)
        return paComplete;
    else
#endif
        return paContinue;
}

static int
pa_callback_nonInterleaved( const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *arg )
{
    float **out = (float **)outputBuffer;
    Server *server = (Server *) arg;

    assert(framesPerBuffer == server->bufferSize);
    int i, j;

    /* avoid unused variable warnings */
    (void) timeInfo;
    (void) statusFlags;

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }

    if (server->duplex == 1) {
        float **in = (float **)inputBuffer;
        for (i=0; i<server->bufferSize; i++) {
            for (j=0; j<server->ichnls; j++) {
                server->input_buffer[(i*server->ichnls)+j] = (MYFLT)in[j+server->input_offset][i];
            }
        }
    }

    Server_process_buffers(server);
    for (i=0; i<server->bufferSize; i++) {
        for (j=0; j<server->nchnls; j++) {
            out[j+server->output_offset][i] = (float) server->output_buffer[(i*server->nchnls)+j];
        }
    }
    server->midi_count = 0;

#ifdef _OSX_
    if (server->server_stopped == 1)
        return paComplete;
    else
#endif
        return paContinue;
}

#ifdef USE_JACK
/* Jack callbacks */

static int
jack_callback (jack_nframes_t nframes, void *arg)
{
    int i, j;
    Server *server = (Server *) arg;
    assert(nframes == server->bufferSize);
    jack_default_audio_sample_t *in_buffers[server->ichnls], *out_buffers[server->nchnls];

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }
    PyoJackBackendData *be_data = (PyoJackBackendData *) server->audio_be_data;
    for (i = 0; i < server->ichnls; i++) {
        in_buffers[i] = jack_port_get_buffer (be_data->jack_in_ports[i+server->input_offset], server->bufferSize);
    }
    for (i = 0; i < server->nchnls; i++) {
        out_buffers[i] = jack_port_get_buffer (be_data->jack_out_ports[i+server->output_offset], server->bufferSize);

    }
    /* jack audio data is not interleaved */
    if (server->duplex == 1) {
        for (i=0; i<server->bufferSize; i++) {
            for (j=0; j<server->ichnls; j++) {
                server->input_buffer[(i*server->ichnls)+j] = (MYFLT) in_buffers[j][i];
            }
        }
    }
    Server_process_buffers(server);
    for (i=0; i<server->bufferSize; i++) {
        for (j=0; j<server->nchnls; j++) {
            out_buffers[j][i] = (jack_default_audio_sample_t) server->output_buffer[(i*server->nchnls)+j];
        }
    }
    server->midi_count = 0;
    return 0;
}

static int
jack_srate_cb (jack_nframes_t nframes, void *arg)
{
    Server *s = (Server *) arg;
    s->samplingRate = (double) nframes;
    Server_debug(s, "The sample rate is now %lu/sec\n", (unsigned long) nframes);
    return 0;
}

static int
jack_bufsize_cb (jack_nframes_t nframes, void *arg)
{
    Server *s = (Server *) arg;
    s->bufferSize = (int) nframes;
    Server_debug(s, "The buffer size is now %lu/sec\n", (unsigned long) nframes);
    return 0;
}

static void
jack_error_cb (const char *desc)
{
    printf("JACK error: %s\n", desc);
}

static void
jack_shutdown_cb (void *arg)
{
    Server *s = (Server *) arg;
    Server_shut_down(s);
    Server_warning(s, "JACK server shutdown. Pyo Server shut down.\n");
}

#endif

#ifdef USE_COREAUDIO
/* Coreaudio callbacks */

OSStatus coreaudio_input_callback(AudioDeviceID device, const AudioTimeStamp* inNow,
                                   const AudioBufferList* inInputData,
                                   const AudioTimeStamp* inInputTime,
                                   AudioBufferList* outOutputData,
                                   const AudioTimeStamp* inOutputTime,
                                   void* defptr)
{
    int i, j, bufchnls, servchnls, off1chnls, off2chnls;
    Server *server = (Server *) defptr;
    (void) outOutputData;
    const AudioBuffer* inputBuf = inInputData->mBuffers;
    float *bufdata = (float*)inputBuf->mData;
    bufchnls = inputBuf->mNumberChannels;
    servchnls = server->ichnls < bufchnls ? server->ichnls : bufchnls;
    for (i=0; i<server->bufferSize; i++) {
        off1chnls = i*bufchnls+server->input_offset;
        off2chnls = i*servchnls;
        for (j=0; j<servchnls; j++) {
            server->input_buffer[off2chnls+j] = (MYFLT)bufdata[off1chnls+j];
        }
    }
    return kAudioHardwareNoError;
}

OSStatus coreaudio_output_callback(AudioDeviceID device, const AudioTimeStamp* inNow,
                                   const AudioBufferList* inInputData,
                                   const AudioTimeStamp* inInputTime,
                                   AudioBufferList* outOutputData,
                                   const AudioTimeStamp* inOutputTime,
                                   void* defptr)
{
    int i, j, bufchnls, servchnls, off1chnls, off2chnls;
    Server *server = (Server *) defptr;

    (void) inInputData;

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }

    Server_process_buffers(server);
    AudioBuffer* outputBuf = outOutputData->mBuffers;
    bufchnls = outputBuf->mNumberChannels;
    servchnls = server->nchnls < bufchnls ? server->nchnls : bufchnls;
    float *bufdata = (float*)outputBuf->mData;
    for (i=0; i<server->bufferSize; i++) {
        off1chnls = i*bufchnls+server->output_offset;
        off2chnls = i*servchnls;
        for(j=0; j<servchnls; j++) {
            bufdata[off1chnls+j] = server->output_buffer[off2chnls+j];
        }
    }
    server->midi_count = 0;

    return kAudioHardwareNoError;
}

int
coreaudio_stop_callback(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceStop(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceStop failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceStop(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceStop failed %d\n", (int)err);
        return -1;
    }
    self->server_started = 0;
    return 0;
}

#endif

static int
offline_process_block(Server *arg)
{
    Server *server = (Server *) arg;
    Server_process_buffers(server);
    return 0;
}

/* Server audio backend init functions */

int
Server_pa_init(Server *self)
{
    PaError err;
    PaStreamParameters outputParameters;
    PaStreamParameters inputParameters;
    PaDeviceIndex n, inDevice, outDevice;
    const PaDeviceInfo *deviceInfo;
    PaHostApiIndex hostIndex;
    const PaHostApiInfo *hostInfo;
    PaHostApiTypeId hostId;
    PaSampleFormat sampleFormat;
    PaStreamCallback *streamCallback;

    err = Pa_Initialize();
    portaudio_assert(err, "Pa_Initialize");

    n = Pa_GetDeviceCount();
    if (n < 0) {
        portaudio_assert(n, "Pa_GetDeviceCount");
    }

    PyoPaBackendData *be_data = (PyoPaBackendData *) malloc(sizeof(PyoPaBackendData *));
    self->audio_be_data = (void *) be_data;

    if (self->output == -1)
        outDevice = Pa_GetDefaultOutputDevice(); /* default output device */
    else
        outDevice = (PaDeviceIndex) self->output; /* selected output device */
    if (self->input == -1)
        inDevice = Pa_GetDefaultInputDevice(); /* default input device */
    else
        inDevice = (PaDeviceIndex) self->input; /* selected input device */

    /* Retrieve host api id and define sample and callback format*/
    deviceInfo = Pa_GetDeviceInfo(outDevice);
    hostIndex = deviceInfo->hostApi;
    hostInfo = Pa_GetHostApiInfo(hostIndex);
    hostId = hostInfo->type;
    if (hostId == paASIO) {
        Server_debug(self, "Portaudio uses non-interleaved callback.\n");
        sampleFormat = paFloat32 | paNonInterleaved;
        streamCallback = pa_callback_nonInterleaved;
    }
    else if (hostId == paALSA) {
        Server_debug(self, "Portaudio uses interleaved callback.\n");
        Server_debug(self, "Using ALSA, if no input/output devices are specified, force to devices 0.\n");
        if (self->input == -1 && self->output == -1) {
            self->input = self->output = 0;
            inDevice = outDevice = (PaDeviceIndex) 0;
        }
        sampleFormat = paFloat32;
        streamCallback = pa_callback_interleaved;
    }
    else {
        Server_debug(self, "Portaudio uses interleaved callback.\n");
        sampleFormat = paFloat32;
        streamCallback = pa_callback_interleaved;
    }


    /* setup output and input streams */
    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.device = outDevice;
    outputParameters.channelCount = self->nchnls + self->output_offset;
    outputParameters.sampleFormat = sampleFormat;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    if (self->duplex == 1) {
        memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.device = inDevice;
        inputParameters.channelCount = self->ichnls + self->input_offset;
        inputParameters.sampleFormat = sampleFormat;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
        inputParameters.hostApiSpecificStreamInfo = NULL;
    }

    if (self->input == -1 && self->output == -1) {
        if (self->duplex == 1)
            err = Pa_OpenDefaultStream(&be_data->stream,
                                       self->ichnls + self->input_offset,
                                       self->nchnls + self->output_offset,
                                       sampleFormat,
                                       self->samplingRate,
                                       self->bufferSize,
                                       streamCallback,
                                       (void *) self);
        else
            err = Pa_OpenDefaultStream(&be_data->stream,
                                       0,
                                       self->nchnls + self->output_offset,
                                       sampleFormat,
                                       self->samplingRate,
                                       self->bufferSize,
                                       streamCallback,
                                       (void *) self);
    }
    else {
        if (self->duplex == 1)
            err = Pa_OpenStream(&be_data->stream,
                                &inputParameters,
                                &outputParameters,
                                self->samplingRate,
                                self->bufferSize,
                                paNoFlag,
                                streamCallback,
                                (void *) self);
        else
            err = Pa_OpenStream(&be_data->stream,
                                (PaStreamParameters *) NULL,
                                &outputParameters,
                                self->samplingRate,
                                self->bufferSize,
                                paNoFlag,
                                streamCallback,
                                (void *) self);
    }
    portaudio_assert(err, "Pa_OpenStream");
    if (err < 0) {
        Server_error(self, "Portaudio error: %s", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

int
Server_pa_deinit(Server *self)
{
    PaError err;
    PyoPaBackendData *be_data = (PyoPaBackendData *) self->audio_be_data;

    if (Pa_IsStreamActive(be_data->stream) || ! Pa_IsStreamStopped(be_data->stream)) {
        self->server_started = 0;
        err = Pa_AbortStream(be_data->stream);
        portaudio_assert(err, "Pa_AbortStream");
    }

    err = Pa_CloseStream(be_data->stream);
    portaudio_assert(err, "Pa_CloseStream");

    err = Pa_Terminate();
    portaudio_assert(err, "Pa_Terminate");

    free(self->audio_be_data);
    return err;
}

int
Server_pa_start(Server *self)
{
    PaError err;
    PyoPaBackendData *be_data = (PyoPaBackendData *) self->audio_be_data;

    if (Pa_IsStreamActive(be_data->stream) || ! Pa_IsStreamStopped(be_data->stream)) {
        err = Pa_AbortStream(be_data->stream);
        portaudio_assert(err, "Pa_AbortStream");
    }
    err = Pa_StartStream(be_data->stream);
    portaudio_assert(err, "Pa_StartStream");
    return err;
}

int
Server_pa_stop(Server *self)
{
    PaError err;
    PyoPaBackendData *be_data = (PyoPaBackendData *) self->audio_be_data;

    if (Pa_IsStreamActive(be_data->stream) || ! Pa_IsStreamStopped(be_data->stream)) {
#ifndef _OSX_
        err = Pa_AbortStream(be_data->stream);
        portaudio_assert(err, "Pa_AbortStream");
#endif
    }
    self->server_started = 0;
    self->server_stopped = 1;
    return 0;
}

#ifdef USE_JACK
int
Server_jack_autoconnect (Server *self)
{
    const char **ports;
    int i, j, num = 0, ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (self->jackautoin) {
        if ((ports = jack_get_ports (be_data->jack_client, "system", NULL, JackPortIsOutput)) == NULL) {
            Server_error(self, "Jack: Cannot find any physical capture ports called 'system'\n");
            ret = -1;
        }

        i=0;
        while(ports[i]!=NULL && be_data->jack_in_ports[i] != NULL){
            if (jack_connect (be_data->jack_client, ports[i], jack_port_name(be_data->jack_in_ports[i]))) {
                Server_error(self, "Jack: cannot connect input ports to 'system'\n");
                ret = -1;
            }
            i++;
        }
        free (ports);
    }

    if (self->jackautoout) {
        if ((ports = jack_get_ports (be_data->jack_client, "system", NULL, JackPortIsInput)) == NULL) {
            Server_error(self, "Jack: Cannot find any physical playback ports called 'system'\n");
            ret = -1;
        }

        i=0;
        while(ports[i]!=NULL && be_data->jack_out_ports[i] != NULL){
            if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
                Server_error(self, "Jack: cannot connect output ports to 'system'\n");
                ret = -1;
            }
            i++;
        }
        free (ports);
    }

    num = PyList_Size(self->jackAutoConnectInputPorts);
    if (num > 0) {
        for (j=0; j<num; j++) {
            if ((ports = jack_get_ports (be_data->jack_client, PyString_AsString(PyList_GetItem(self->jackAutoConnectInputPorts, j)), NULL, JackPortIsOutput)) == NULL) {
                Server_error(self, "Jack: cannot connect input ports to %s\n", PyString_AsString(PyList_GetItem(self->jackAutoConnectInputPorts, j)));
            }
            else {
                i = 0;
                while(ports[i] != NULL && be_data->jack_in_ports[i] != NULL){
                    if (jack_connect (be_data->jack_client, ports[i], jack_port_name (be_data->jack_in_ports[i]))) {
                        Server_error(self, "Jack: cannot connect input ports\n");
                        ret = -1;
                    }
                    i++;
                }
                free (ports);
            }
        }
    }

    num = PyList_Size(self->jackAutoConnectOutputPorts);
    if (num > 0) {
        for (j=0; j<num; j++) {
            if ((ports = jack_get_ports (be_data->jack_client, PyString_AsString(PyList_GetItem(self->jackAutoConnectOutputPorts, j)), NULL, JackPortIsInput)) == NULL) {
                Server_error(self, "Jack: cannot connect output ports to %s\n", PyString_AsString(PyList_GetItem(self->jackAutoConnectOutputPorts, j)));
            }
            else {
                i = 0;
                while(ports[i] != NULL && be_data->jack_out_ports[i] != NULL){
                    if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
                        Server_error(self, "Jack: cannot connect output ports\n");
                        ret = -1;
                    }
                    i++;
                }
                free (ports);
            }
        }
    }

    return ret;
}

int
Server_jack_init (Server *self)
{
    char client_name[32];
    char name[16];
    const char *server_name = "server";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    int sampleRate = 0;
    int bufferSize = 0;
    int nchnls = 0;
    int total_nchnls = 0;
    int index = 0;
    int ret = 0;
    assert(self->audio_be_data == NULL);
    PyoJackBackendData *be_data = (PyoJackBackendData *) malloc(sizeof(PyoJackBackendData *));
    self->audio_be_data = (void *) be_data;
    be_data->jack_in_ports = (jack_port_t **) calloc(self->ichnls + self->input_offset, sizeof(jack_port_t *));
    be_data->jack_out_ports = (jack_port_t **) calloc(self->nchnls + self->output_offset, sizeof(jack_port_t *));
    strncpy(client_name,self->serverName, 32);
    be_data->jack_client = jack_client_open (client_name, options, &status, server_name);
    if (be_data->jack_client == NULL) {
        Server_error(self, "Jack error: Unable to create JACK client\n");
        if (status & JackServerFailed) {
            Server_debug(self, "Jack error: jack_client_open() failed, "
            "status = 0x%2.0x\n", status);
        }
        return -1;
    }
    if (status & JackServerStarted) {
        Server_warning(self, "JACK server started.\n");
    }
    if (strcmp(self->serverName, jack_get_client_name(be_data->jack_client)) ) {
        strcpy(self->serverName, jack_get_client_name(be_data->jack_client));
        Server_warning(self, "Jack name `%s' assigned\n", self->serverName);
    }

    sampleRate = jack_get_sample_rate (be_data->jack_client);
    if (sampleRate != self->samplingRate) {
        self->samplingRate = (double)sampleRate;
        Server_warning(self, "Sample rate set to Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    else {
        Server_debug(self, "Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    if (sampleRate <= 0) {
        Server_error(self, "Invalid Jack engine sample rate.");
        jack_client_close (be_data->jack_client);
        return -1;
    }
    bufferSize = jack_get_buffer_size(be_data->jack_client);
    if (bufferSize != self->bufferSize) {
        self->bufferSize = bufferSize;
        Server_warning(self, "Buffer size set to Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }
    else {
        Server_debug(self, "Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }

    nchnls = total_nchnls = self->ichnls + self->input_offset;
    while (nchnls-- > 0) {
        index = total_nchnls - nchnls - 1;
        ret = sprintf(name, "input_%i", index + 1);
        if (ret > 0) {
            be_data->jack_in_ports[index]
            = jack_port_register (be_data->jack_client, name,
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }

        if ((be_data->jack_in_ports[index] == NULL)) {
            Server_error(self, "Jack: no more JACK input ports available\n");
            return -1;
        }
    }

    nchnls = total_nchnls = self->nchnls + self->output_offset;
    while (nchnls-- > 0) {
        index = total_nchnls - nchnls - 1;
        ret = sprintf(name, "output_%i", index + 1);
        if (ret > 0) {
            be_data->jack_out_ports[index]
            = jack_port_register (be_data->jack_client, name,
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }
        if ((be_data->jack_out_ports[index] == NULL)) {
            Server_error(self, "Jack: no more JACK output ports available\n");
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
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    jack_set_process_callback(be_data->jack_client, jack_callback, (void *) self);
    if (jack_activate (be_data->jack_client)) {
        Server_error(self, "Jack error: cannot activate jack client.\n");
        jack_client_close (be_data->jack_client);
        Server_shut_down(self);
        return -1;
    }
    Server_jack_autoconnect(self);
    return 0;
}

int
Server_jack_stop (Server *self)
{
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    int ret = jack_deactivate(be_data->jack_client);
    self->server_started = 0;
    return ret;
}

#endif

#ifdef USE_COREAUDIO
int
Server_coreaudio_init(Server *self)
{
    OSStatus err = kAudioHardwareNoError;
    UInt32 count, namelen, propertySize;
    int i, numdevices;
    char *name;
    AudioDeviceID mOutputDevice = kAudioDeviceUnknown;
    AudioDeviceID mInputDevice = kAudioDeviceUnknown;
    Boolean writable;
    AudioTimeStamp now;

    now.mFlags = kAudioTimeStampHostTimeValid;
    now.mHostTime = AudioGetCurrentHostTime();

    /************************************/
    /* List Coreaudio available devices */
    /************************************/
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &count, 0);
    AudioDeviceID *devices = (AudioDeviceID*) malloc(count);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &count, devices);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Get kAudioHardwarePropertyDevices error %s\n", (char*)&err);
        free(devices);
    }

    numdevices = count / sizeof(AudioDeviceID);
    Server_debug(self, "Coreaudio : Number of devices: %i\n", numdevices);

    for (i=0; i<numdevices; ++i) {
        err = AudioDeviceGetPropertyInfo(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            break;
        }

        char *name = (char*)malloc(count);
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            free(name);
            break;
        }
        Server_debug(self, "   %d : \"%s\"\n", i, name);
        free(name);
    }

    /************************************/
    /* Acquire input and output devices */
    /************************************/
    /* Acquire input audio device */
    if (self->duplex == 1) {
        if (self->input != -1)
            mInputDevice = devices[self->input];

        if (mInputDevice==kAudioDeviceUnknown) {
            count = sizeof(mInputDevice);
            err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &count, (void *) &mInputDevice);
            if (err != kAudioHardwareNoError) {
                Server_error(self, "Get kAudioHardwarePropertyDefaultInputDevice error %s\n", (char*)&err);
                return -1;
            }
        }

        err = AudioDeviceGetPropertyInfo(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }
        name = (char*)malloc(namelen);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }
        Server_debug(self, "Coreaudio : Uses input device : \"%s\"\n", name);
        self->input = mInputDevice;
        free(name);
    }

    /* Acquire output audio device */
    if (self->output != -1)
        mOutputDevice = devices[self->output];

    if (mOutputDevice==kAudioDeviceUnknown) {
        count = sizeof(mOutputDevice);
        err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &count, (void *) &mOutputDevice);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioHardwarePropertyDefaultOutputDevice error %s\n", (char*)&err);
            return -1;
        }
    }

    err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    name = (char*)malloc(namelen);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    Server_debug(self, "Coreaudio : Uses output device : \"%s\"\n", name);
    self->output = mOutputDevice;
    free(name);

    /*************************************************/
    /* Get in/out buffer frame and buffer frame size */
    /*************************************************/
    UInt32 bufferSize;
    AudioValueRange range;
    Float64 sampleRate;

    /* Get input device buffer frame size and buffer frame size range */
    if (self->duplex == 1) {
        count = sizeof(UInt32);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, &bufferSize);
        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device buffer size = %ld\n", bufferSize);

        count = sizeof(AudioValueRange);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyBufferSizeRange, &count, &range);
        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device buffer size range = %f -> %f\n", range.mMinimum, range.mMaximum);

        /* Get input device sampling rate */
        count = sizeof(Float64);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, &sampleRate);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device sampling rate = %.2f\n", sampleRate);
    }

    /* Get output device buffer frame size and buffer frame size range */
    count = sizeof(UInt32);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, &bufferSize);
    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device buffer size = %ld\n", bufferSize);

    count = sizeof(AudioValueRange);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferSizeRange, &count, &range);
    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device buffer size range = %.2f -> %.2f\n", range.mMinimum, range.mMaximum);

    /* Get output device sampling rate */
    count = sizeof(Float64);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, &sampleRate);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device sampling rate = %.2f\n", sampleRate);


    /****************************************/
    /********* Set audio properties *********/
    /****************************************/
    /* set/get the buffersize for the devices */
    count = sizeof(UInt32);
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        self->bufferSize = bufferSize;
        err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        else
            Server_debug(self, "pyo buffer size set to output device buffer size : %i\n", self->bufferSize);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device buffer size successfully: %i\n", self->bufferSize);

    if (self->duplex == 1) {
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        }
    }

    /* set/get the sampling rate for the devices */
    count = sizeof(double);
    double pyoSamplingRate = self->samplingRate;
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        self->samplingRate = (double)sampleRate;
        err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &sampleRate);
        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        else
            Server_debug(self, "pyo sampling rate set to output device sampling rate : %i\n", self->samplingRate);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device sampling rate successfully: %.2f\n", self->samplingRate);

    if (self->duplex ==1) {
        pyoSamplingRate = self->samplingRate;
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        }
    }


    /****************************************/
    /* Input and output stream descriptions */
    /****************************************/
    AudioStreamBasicDescription outputStreamDescription;
    AudioStreamBasicDescription inputStreamDescription;

    // Get input device stream configuration
    if (self->duplex == 1) {
        count = sizeof(AudioStreamBasicDescription);
        err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

        /*
        inputStreamDescription.mSampleRate = (Float64)self->samplingRate;

        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyStreamFormat, count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "-- Set kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

        // Print new input stream description
        err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        */
        Server_debug(self, "Coreaudio : Coreaudio driver input stream sampling rate = %.2f\n", inputStreamDescription.mSampleRate);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream bytes per frame = %i\n", inputStreamDescription.mBytesPerFrame);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream number of channels = %i\n", inputStreamDescription.mChannelsPerFrame);
    }

    /* Get output device stream configuration */
    count = sizeof(AudioStreamBasicDescription);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyStreamFormat, &count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

    /*
    outputStreamDescription.mSampleRate = (Float64)self->samplingRate;

    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyStreamFormat, count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Set kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

    // Print new output stream description
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyStreamFormat, &count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);
    */
    Server_debug(self, "Coreaudio : Coreaudio driver output stream sampling rate = %.2f\n", outputStreamDescription.mSampleRate);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream bytes per frame = %i\n", outputStreamDescription.mBytesPerFrame);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream number of channels = %i\n", outputStreamDescription.mChannelsPerFrame);


    /**************************************************/
    /********* Set input and output callbacks *********/
    /**************************************************/
    if (self->duplex == 1) {
        err = AudioDeviceAddIOProc(self->input, coreaudio_input_callback, (void *) self);    // setup our device with an IO proc
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceAddIOProc failed %d\n", (int)err);
            return -1;
        }
        err = AudioDeviceGetPropertyInfo(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
        AudioHardwareIOProcStreamUsage *input_su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
        input_su->mIOProc = (void*)coreaudio_input_callback;
        err = AudioDeviceGetProperty(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, input_su);
        for (i=0; i<inputStreamDescription.mChannelsPerFrame; ++i) {
            input_su->mStreamIsOn[i] = 1;
        }
        err = AudioDeviceSetProperty(self->input, &now, 0, true, kAudioDevicePropertyIOProcStreamUsage, propertySize, input_su);
    }

    err = AudioDeviceAddIOProc(self->output, coreaudio_output_callback, (void *) self);    // setup our device with an IO proc
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceAddIOProc failed %d\n", (int)err);
        return -1;
    }
    err = AudioDeviceGetPropertyInfo(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
    AudioHardwareIOProcStreamUsage *output_su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
    output_su->mIOProc = (void*)coreaudio_output_callback;
    err = AudioDeviceGetProperty(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, output_su);
    for (i=0; i<outputStreamDescription.mChannelsPerFrame; ++i) {
        output_su->mStreamIsOn[i] = 1;
    }
    err = AudioDeviceSetProperty(self->output, &now, 0, false, kAudioDevicePropertyIOProcStreamUsage, propertySize, output_su);

    return 0;
}

int
Server_coreaudio_deinit(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceRemoveIOProc(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceRemoveIOProc failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceRemoveIOProc(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceRemoveIOProc failed %d\n", (int)err);
        return -1;
    }

    return 0;
}

int
Server_coreaudio_start(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceStart(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceStart failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceStart(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceStart failed %d\n", (int)err);
        return -1;
    }
    return 0;
}

int
Server_coreaudio_stop(Server *self)
{
    coreaudio_stop_callback(self);
    self->server_stopped = 1;
    return 0;
}

#endif

int
Server_offline_init(Server *self)
{
    return 0;
}

int
Server_offline_deinit(Server *self)
{
    return 0;
}

void
*Server_offline_thread(void *arg)
{
    int numBlocks;
    Server *self;
    self = (Server *)arg;

    if (self->recdur < 0) {
        Server_error(self,"Duration must be specified for Offline Server (see Server.recordOptions).");
    }
    else {
        Server_message(self,"Offline Server rendering file %s dur=%f\n", self->recpath, self->recdur);
        numBlocks = ceil(self->recdur * self->samplingRate/self->bufferSize);
        Server_debug(self,"Number of blocks: %i\n", numBlocks);
        Server_start_rec_internal(self, self->recpath);
        while (numBlocks-- > 0 && self->server_stopped == 0) {
            offline_process_block((Server *) self);
        }
        self->server_started = 0;
        self->record = 0;
        sf_close(self->recfile);
        Server_message(self,"Offline Server rendering finished.\n");
    }
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
        Server_error(self,"Duration must be specified for Offline Server (see Server.recordOptions).");
        return -1;
    }
    Server_message(self,"Offline Server rendering file %s dur=%f\n", self->recpath, self->recdur);
    numBlocks = ceil(self->recdur * self->samplingRate/self->bufferSize);
    Server_debug(self,"Number of blocks: %i\n", numBlocks);
    Server_start_rec_internal(self, self->recpath);
    while (numBlocks-- > 0 && self->server_stopped == 0) {
        offline_process_block((Server *) self);
    }
    self->server_started = 0;
    self->record = 0;
    sf_close(self->recfile);
    Server_message(self,"Offline Server rendering finished.\n");
    return 0;
}

int
Server_offline_stop(Server *self)
{
    self->server_stopped = 1;
    return 0;
}

/******* Embedded Server *******/
int
Server_embedded_init(Server *self)
{
    return 0;
}

int
Server_embedded_deinit(Server *self)
{
    return 0;
}

/* interleaved embedded callback */
int
Server_embedded_i_start(Server *self)
{
    Server_process_buffers(self);
    return 0;
}

// To be easier to call without depending on the Server structure
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
    Server_process_buffers(self);
    float *out = (float *)calloc(self->bufferSize * self->nchnls, sizeof(float));
    for (i=0; i<(self->bufferSize*self->nchnls); i++){
        out[i] = self->output_buffer[i];
    }

    /* Non-Interleaved */
    for (i=0; i<self->bufferSize; i++) {
        for (j=0; j<=self->nchnls; j++) {
            /* TODO: This could probably be more efficient (ob) */
            self->output_buffer[i+(self->bufferSize*(j+1))-self->bufferSize] = out[(i*self->nchnls)+j];
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

/* this stop function is not very useful since the processing is stopped at the end
of every processing callback, but function put to not break pyo */
int
Server_embedded_stop(Server *self)
{
    self->server_started = 0;
    self->server_stopped = 1;
    return 0;
}

/***************************************************/
/*  Main Processing functions                      */

static inline void
Server_process_buffers(Server *server)
{
    float *out = server->output_buffer;
    MYFLT buffer[server->nchnls][server->bufferSize];
    int i, j, chnl;
    int nchnls = server->nchnls;
    MYFLT amp = server->amp;
    Stream *stream_tmp;
    MYFLT *data;

    memset(&buffer, 0, sizeof(buffer));
    PyGILState_STATE s = PyGILState_Ensure();
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
    if (server->withGUI == 1 && nchnls <= 8) {
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
    if (server->record == 1)
        sf_write_float(server->recfile, out, server->bufferSize * server->nchnls);

}

static void
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
        }
        server->gcount = 0;
    }
}

static void
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

/***************************************************/

/* Global function called by any new audio object to
 get a pointer to the server */
PyObject *
PyServer_get_server()
{
    return (PyObject *)my_server[serverID];
}

static PyObject *
Server_shut_down(Server *self)
{
    int i;
    int ret = -1;
    if (self->server_booted == 0) {
        Server_error(self, "The Server must be booted!\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (self->server_started == 1) {
        Server_stop((Server *)self);
    }

    for (i=0; i<num_rnd_objs; i++) {
        rnd_objs_count[i] = 0;
    }

    switch (self->audio_be_type) {
        case PyoPortaudio:
            ret = Server_pa_deinit(self);
            break;
        case PyoCoreaudio:
#ifdef USE_COREAUDIO
            ret = Server_coreaudio_deinit(self);
#endif
            break;
        case PyoJack:
#ifdef USE_JACK
            ret = Server_jack_deinit(self);
#endif
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

    Py_INCREF(Py_None);
    return Py_None;
}

/* handling of PyObjects */
static int
Server_traverse(Server *self, visitproc visit, void *arg)
{
    /* GUI and TIME ? */
    Py_VISIT(self->streams);
    Py_VISIT(self->jackAutoConnectInputPorts);
    Py_VISIT(self->jackAutoConnectOutputPorts);
    return 0;
}

static int
Server_clear(Server *self)
{
    Py_CLEAR(self->streams);
    Py_CLEAR(self->jackAutoConnectInputPorts);
    Py_CLEAR(self->jackAutoConnectOutputPorts);
    return 0;
}

static void
Server_dealloc(Server* self)
{
    if (self->server_booted == 1)
        Server_shut_down(self);
    Server_clear(self);
    free(self->input_buffer);
    free(self->output_buffer);
    free(self->serverName);
    my_server[self->thisServerID] = NULL;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* Unused variables to allow the safety check of the embeded audio backend. */
    double samplingRate = 44100.0;
    int  nchnls = 2;
    int  ichnls = 2;
    int  bufferSize = 256;
    int  duplex = 0;
    char *audioType = "portaudio";
    char *serverName = "pyo";

    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", "audio", "jackname", "ichnls", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|diiissi", kwlist,
            &samplingRate, &nchnls, &bufferSize, &duplex, &audioType, &serverName, &ichnls)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (strcmp(audioType, "embedded") != 0)
    {
        if (PyServer_get_server() != NULL) {
            PyErr_SetString(PyExc_RuntimeError, "Warning: Trying to create a new Server object while one is already created!\n");
            Py_RETURN_NONE;
        }
    }

    /* find the first free serverID */
    for(serverID = 0; serverID < MAX_NBR_SERVER; serverID++){
        if(my_server[serverID] == NULL){
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
    self->serverName = (char *) calloc(32, sizeof(char));
    self->jackautoin = 1;
    self->jackautoout = 1;
    self->jackAutoConnectInputPorts = PyList_New(0);
    self->jackAutoConnectOutputPorts = PyList_New(0);
    self->samplingRate = 44100.0;
    self->nchnls = 2;
    self->ichnls = 2;
    self->record = 0;
    self->bufferSize = 256;
    self->duplex = 0;
    self->input = -1;
    self->output = -1;
    self->input_offset = 0;
    self->output_offset = 0;
    self->midiin_count = 0;
    self->midiout_count = 0;
    self->midi_input = -1;
    self->midi_output = -1;
    self->amp = self->resetAmp = 1.;
    self->currentAmp = self->lastAmp = 0.;
    self->withGUI = 0;
    self->withTIME = 0;
    self->verbosity = 7;
    self->recdur = -1;
    self->recformat = 0;
    self->rectype = 0;
    self->startoffset = 0.0;
    self->globalSeed = 0;
    self->thisServerID = serverID;
    Py_XDECREF(my_server[serverID]);
    my_server[serverID] = (Server *)self;
    return (PyObject *)self;
}

static int
Server_init(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", "audio", "jackname", "ichnls", NULL};

    char *audioType = "portaudio";
    char *serverName = "pyo";

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|diiissi", kwlist,
            &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex, &audioType, &serverName, &self->ichnls))
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
    strncpy(self->serverName, serverName, 32);
    if (strlen(serverName) > 31) {
        self->serverName[31] = '\0';
    }

    return 0;
}

static PyObject *
Server_setDefaultRecPath(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"path", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &self->recpath))
        return PyInt_FromLong(-1);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setInputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change input offset for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->input_offset = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setOutputOffset(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change output offset for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->output_offset = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
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
Server_setMidiOutputDevice(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->midi_output = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setSamplingRate(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change sampling rate for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyNumber_Check(arg)) {
        self->samplingRate = PyFloat_AsDouble(PyNumber_Float(arg));
    }
    else {
        Server_error(self, "Sampling rate must be a number.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setNchnls(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change number of channels for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->nchnls = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Number of channels must be an integer.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setIchnls(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change number of input channels for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->ichnls = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Number of input channels must be an integer.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setBufferSize(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self, "Can't change buffer size for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->bufferSize = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Buffer size must be an integer.\n");
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setDuplex(Server *self, PyObject *arg)
{
    if (self->server_booted) {
        Server_warning(self,"Can't change duplex mode for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL) {
        if (PyInt_Check(arg))
            self->duplex = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setJackAuto(Server *self, PyObject *args)
{
    int in=1, out=1;

    if (! PyArg_ParseTuple(args, "ii", &in, &out)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    self->jackautoin = in;
    self->jackautoout = out;

    Py_INCREF(Py_None);
    return Py_None;
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

    Py_INCREF(Py_None);
    return Py_None;
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

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setGlobalSeed(Server *self, PyObject *arg)
{
    unsigned int tmp;

    if (arg != NULL) {
        if (PyInt_Check(arg)) {
            tmp = PyInt_AsLong(arg);
            if (tmp < 0)
                self->globalSeed = 0;
            else
                self->globalSeed = tmp;
        }
        else
            self->globalSeed = 0;
    }
    else
        self->globalSeed = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

int
Server_generateSeed(Server *self, int oid)
{
    int curseed, seed, count, mult;
    long ltime;

    count = ++rnd_objs_count[oid];
    mult = rnd_objs_mult[oid];

    if (self->globalSeed > 0) {
        curseed = self->globalSeed + ((count * mult) % 32768);
    }
    else {
        ltime = time(NULL);
        seed = (unsigned) (ltime / 2) % 32768;
        curseed = seed + ((count * mult) % 32768);
    }
    srand(curseed);

    return 0;
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
        Server_error(self,"The amplitude callable attribute must be a method.\n");
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

static PyObject *
Server_setTimeCallable(Server *self, PyObject *arg)
{
    int i;
    PyObject *tmp;

    if (arg == NULL) {
        Server_error(self,"The time callable attribute must be a method.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }

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

    Py_INCREF(Py_None);
    return Py_None;
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

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setStartOffset(Server *self, PyObject *arg)
{
    if (arg != NULL) {
        int check = PyNumber_Check(arg);

        if (check) {
            self->startoffset = PyFloat_AsDouble(PyNumber_Float(arg));
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

int
Server_pm_init(Server *self)
{
    int i = 0;
   /* Initializing MIDI */
    PmError pmerr;

    pmerr = Pm_Initialize();
    if (pmerr) {
        Server_warning(self, "Portmidi warning: could not initialize Portmidi: %s\n", Pm_GetErrorText(pmerr));
        self->withPortMidi = 0;
        self->withPortMidiOut = 0;
        return -1;
    }
    else {
        Server_debug(self, "Portmidi initialized.\n");
        self->withPortMidi = 1;
        self->withPortMidiOut = 1;
    }

    if (self->withPortMidi == 1) {
        self->midiin_count = self->midiout_count = 0;
        int num_devices = Pm_CountDevices();
        Server_debug(self, "Portmidi number of devices: %d.\n", num_devices);
        if (num_devices > 0) {
            if (self->midi_input < num_devices) {
                if (self->midi_input == -1)
                    self->midi_input = Pm_GetDefaultInputDeviceID();
                Server_debug(self, "Midi input device : %d.\n", self->midi_input);
                const PmDeviceInfo *info = Pm_GetDeviceInfo(self->midi_input);
                if (info != NULL) {
                    if (info->input) {
                        pmerr = Pm_OpenInput(&self->midiin[0], self->midi_input, NULL, 100, NULL, NULL);
                        if (pmerr) {
                            Server_warning(self,
                                 "Portmidi warning: could not open midi input %d (%s): %s\n",
                                 self->midi_input, info->name, Pm_GetErrorText(pmerr));
                            self->withPortMidi = 0;
                        }
                        else {
                            Server_debug(self, "Midi input (%s) opened.\n", info->name);
                            self->midiin_count = 1;
                        }
                    }
                    else {
                        Server_warning(self, "Portmidi warning: Midi Device (%s), not an input device!\n", info->name);
                        self->withPortMidi = 0;
                    }
                }
            }
            else if (self->midi_input >= num_devices) {
                Server_debug(self, "Midi input device : all!\n");
                self->midiin_count = 0;
                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                    if (info != NULL) {
                        if (info->input) {
                            pmerr = Pm_OpenInput(&self->midiin[self->midiin_count], i, NULL, 100, NULL, NULL);
                            if (pmerr) {
                                Server_warning(self,
                                     "Portmidi warning: could not open midi input %d (%s): %s\n",
                                     0, info->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                Server_debug(self, "Midi input (%s) opened.\n", info->name);
                                self->midiin_count++;
                            }
                        }
                    }
                }
                if (self->midiin_count == 0)
                    self->withPortMidi = 0;
            }
            else {
                    Server_warning(self, "Portmidi warning: no input device!\n");
                    self->withPortMidi = 0;
            }

            if (self->midi_output < num_devices) {
                if (self->midi_output == -1)
                    self->midi_output = Pm_GetDefaultOutputDeviceID();
                Server_debug(self, "Midi output device : %d.\n", self->midi_output);
                const PmDeviceInfo *outinfo = Pm_GetDeviceInfo(self->midi_output);
                if (outinfo != NULL) {
                    if (outinfo->output) {
                        Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                        pmerr = Pm_OpenOutput(&self->midiout[0], self->midi_output, NULL, 0, NULL, NULL, 1);
                        if (pmerr) {
                            Server_warning(self,
                                     "Portmidi warning: could not open midi output %d (%s): %s\n",
                                     self->midi_output, outinfo->name, Pm_GetErrorText(pmerr));
                            self->withPortMidiOut = 0;
                            if (Pt_Started())
                                Pt_Stop();
                        }
                        else {
                            Server_debug(self, "Midi output (%s) opened.\n", outinfo->name);
                            self->midiout_count = 1;
                        }
                    }
                    else {
                        Server_warning(self, "Portmidi warning: Midi Device (%s), not an output device!\n", outinfo->name);
                        self->withPortMidiOut = 0;
                    }
                }
            }
            else if (self->midi_output >= num_devices) {
                Server_debug(self, "Midi output device : all!\n");
                self->midiout_count = 0;
                Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *outinfo = Pm_GetDeviceInfo(i);
                    if (outinfo != NULL) {
                        if (outinfo->output) {
                            pmerr = Pm_OpenOutput(&self->midiout[self->midiout_count], i, NULL, 100, NULL, NULL, 1);
                            if (pmerr) {
                                Server_warning(self,
                                     "Portmidi warning: could not open midi output %d (%s): %s\n",
                                     0, outinfo->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                Server_debug(self, "Midi output (%s) opened.\n", outinfo->name);
                                self->midiout_count++;
                            }
                        }
                    }
                }
                if (self->midiout_count == 0) {
                    if (Pt_Started())
                        Pt_Stop();
                    self->withPortMidiOut = 0;
                }
            }
            else {
                    Server_warning(self, "Portmidi warning: no output device!\n");
                    self->withPortMidiOut = 0;
            }

            if (self->withPortMidi == 0 && self->withPortMidiOut == 0) {
                Pm_Terminate();
                Server_warning(self, "Portmidi closed.\n");
            }
        }
        else {
            Server_warning(self, "Portmidi warning: no midi device found!\nPortmidi closed.\n");
            self->withPortMidi = 0;
            self->withPortMidiOut = 0;
            Pm_Terminate();
        }
    }
    if (self->withPortMidi == 1) {
        self->midi_count = 0;
        for (i=0; i<self->midiin_count; i++) {
            Pm_SetFilter(self->midiin[i], PM_FILT_ACTIVE | PM_FILT_CLOCK);
        }
    }
    return 0;
}


static PyObject *
Server_boot(Server *self, PyObject *arg)
{
    int audioerr = 0;
    int i;
    if (self->server_booted == 1) {
        Server_error(self, "Server already booted!\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    self->server_started = 0;
    self->stream_count = 0;
    self->elapsedSamples = 0;

    int needNewBuffer = 0;
    if (arg != NULL && PyBool_Check(arg)) {
        needNewBuffer = PyObject_IsTrue(arg);
    }
    else {
        Server_error(self, "The argument to set for a new buffer must be a boolean.\n");
    }

    self->streams = PyList_New(0);
    switch (self->audio_be_type) {
        case PyoPortaudio:
            audioerr = Server_pa_init(self);
            break;
        case PyoJack:
#ifdef USE_JACK
            audioerr = Server_jack_init(self);
            if (audioerr < 0) {
                Server_jack_deinit(self);
            }
#else
            audioerr = -1;
            Server_error(self, "Pyo built without Jack support\n");
#endif
            break;
        case PyoCoreaudio:
#ifdef USE_COREAUDIO
            audioerr = Server_coreaudio_init(self);
            if (audioerr < 0) {
                Server_coreaudio_deinit(self);
            }
#else
            audioerr = -1;
            Server_error(self, "Pyo built without Coreaudio support\n");
#endif
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

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start(Server *self)
{
    int err = -1, midierr = 0;
    if (self->server_started == 1) {
        Server_warning(self, "Server already started!\n");
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (self->server_booted == 0) {
        Server_warning(self, "The Server must be booted!\n");
        Py_INCREF(Py_None);
        return Py_None;
    }

    Server_debug(self, "Server_start: number of streams %d\n", self->stream_count);

    /* Ensure Python is set up for threading */
    PyEval_InitThreads();

    self->server_stopped = 0;
    self->server_started = 1;
    self->timeStep = (int)(0.01 * self->samplingRate);

    if (self->audio_be_type != PyoOffline && self->audio_be_type != PyoOfflineNB && self->audio_be_type != PyoEmbedded) {
        midierr = Server_pm_init(self);
        Server_debug(self, "PortMidi initialization return code : %d.\n", midierr);
    }

    if (self->startoffset > 0.0) {
        Server_message(self,"Rendering %.2f seconds offline...\n", self->startoffset);
        int numBlocks = ceil(self->startoffset * self->samplingRate/self->bufferSize);
        self->lastAmp = 1.0; self->amp = 0.0;
        while (numBlocks-- > 0) {
            offline_process_block((Server *) self);
        }
        Server_message(self,"Offline rendering completed. Start realtime processing.\n");
        self->startoffset = 0.0;
    }

    self->amp = self->resetAmp;

    switch (self->audio_be_type) {
        case PyoPortaudio:
            err = Server_pa_start(self);
            break;
        case PyoCoreaudio:
#ifdef USE_COREAUDIO
            err = Server_coreaudio_start(self);
#endif
            break;
        case PyoJack:
#ifdef USE_JACK
            err = Server_jack_start(self);
#endif
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

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_stop(Server *self)
{
    int i;
    int err = -1;
    if (self->server_started == 0) {
        Server_warning(self, "The Server must be started!\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    switch (self->audio_be_type) {
        case PyoPortaudio:
            err = Server_pa_stop(self);
            break;
        case PyoCoreaudio:
#ifdef USE_COREAUDIO
            err = Server_coreaudio_stop(self);
#endif
            break;
        case PyoJack:
#ifdef USE_JACK
            err = Server_jack_stop(self);
#endif
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
    }

    if (err < 0) {
        Server_error(self, "Error stopping server.\n");
    }
    else {
        self->server_stopped = 1;
        if (self->withPortMidi == 1) {
            for (i=0; i<self->midiin_count; i++) {
                Pm_Close(self->midiin[i]);
            }
        }
        if (self->withPortMidiOut == 1) {
            for (i=0; i<self->midiout_count; i++) {
                Pm_Close(self->midiout[i]);
            }
        }
        if (self->withPortMidi == 1 || self->withPortMidiOut == 1) {
            if (Pt_Started())
                Pt_Stop();
            Pm_Terminate();
        }
        self->withPortMidi = 0;
        self->withPortMidiOut = 0;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_recordOptions(Server *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"dur", "filename", "fileformat", "sampletype", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "d|sii", kwlist, &self->recdur, &self->recpath, &self->recformat, &self->rectype)) {
        return PyInt_FromLong(-1);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start_rec(Server *self, PyObject *args, PyObject *kwds)
{
    char *filename=NULL;

    static char *kwlist[] = {"filename", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &filename)) {
        return PyInt_FromLong(-1);
    }
    Server_start_rec_internal(self, filename);

    Py_INCREF(Py_None);
    return Py_None;
}

static int
Server_start_rec_internal(Server *self, char *filename)
{
    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->samplingRate;
    self->recinfo.channels = self->nchnls;

    Server_debug(self, "recinfo.samplerate : %i\n", self->recinfo.samplerate);
    Server_debug(self, "recinfo.channels : %i\n", self->recinfo.channels);

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
    Server_debug(self, "recinfo.format : %i\n", self->recinfo.format);

    /* Open the output file. */
    if (filename == NULL) {
        Server_debug(self, "recpath : %s\n", self->recpath);
        if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {
            Server_error(self, "Not able to open output file %s.\n", self->recpath);
            Server_debug(self, "%s\n", sf_strerror(self->recfile));
            return -1;
        }
    }
    else {
        Server_debug(self, "filename : %s\n", filename);
        if (! (self->recfile = sf_open(filename, SFM_WRITE, &self->recinfo))) {
            Server_error(self, "Not able to open output file %s.\n", filename);
            Server_debug(self, "%s\n", sf_strerror(self->recfile));
            return -1;
        }
    }

    self->record = 1;
    return 0;
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
        Server_error(self, "Server_addStream needs a pyo object as argument.\n");
        return PyInt_FromLong(-1);
    }

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
            Server_debug(self, "Removed stream id %d\n", id);
            PySequence_DelItem(self->streams, i);
            self->stream_count--;
            break;
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
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

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_noteout(Server *self, PyObject *args)
{
    int i, pit, vel, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiii", &pit, &vel, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        if (chan == 0)
            buffer[0].message = Pm_Message(0x90, pit, vel);
        else
            buffer[0].message = Pm_Message(0x90 | (chan - 1), pit, vel);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_afterout(Server *self, PyObject *args)
{
    int i, pit, vel, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiii", &pit, &vel, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        if (chan == 0)
            buffer[0].message = Pm_Message(0xA0, pit, vel);
        else
            buffer[0].message = Pm_Message(0xA0 | (chan - 1), pit, vel);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_ctlout(Server *self, PyObject *args)
{
    int i, ctlnum, value, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iiii", &ctlnum, &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        if (chan == 0)
            buffer[0].message = Pm_Message(0xB0, ctlnum, value);
        else
            buffer[0].message = Pm_Message(0xB0 | (chan - 1), ctlnum, value);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_programout(Server *self, PyObject *args)
{
    int i, value, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iii", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        if (chan == 0)
            buffer[0].message = Pm_Message(0xC0, value, 0);
        else
            buffer[0].message = Pm_Message(0xC0 | (chan - 1), value, 0);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_pressout(Server *self, PyObject *args)
{
    int i, value, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iii", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        if (chan == 0)
            buffer[0].message = Pm_Message(0xD0, value, 0);
        else
            buffer[0].message = Pm_Message(0xD0 | (chan - 1), value, 0);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *
Server_bendout(Server *self, PyObject *args)
{
    int i, lsb, msb, value, chan, curtime;
    PmEvent buffer[1];
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "iii", &value, &chan, &timestamp))
        return PyInt_FromLong(-1);

    if (self->withPortMidiOut) {
        curtime = Pt_Time();
        buffer[0].timestamp = curtime + timestamp;
        lsb = value & 0x007F;
        msb = (value & (0x007F << 7)) >> 7;
        if (chan == 0)
            buffer[0].message = Pm_Message(0xE0, lsb, msb);
        else
            buffer[0].message = Pm_Message(0xE0 | (chan - 1), lsb, msb);
        for (i=0; i<self->midiout_count; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
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
    return PyInt_FromLong(self->bufferSize);
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
    /* Should return a more conventional signal, like True or False */
    return PyString_FromString("Server set");
}


static PyObject *
Server_getInputAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &self->input_buffer[0]);
    return PyString_FromString(address);
}


static PyObject *
Server_getOutputAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &self->output_buffer[0]);
    return PyString_FromString(address);
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
    return PyString_FromString(address);
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
    return PyString_FromString(address);
}
*/

static PyObject *
Server_getEmbedICallbackAddr(Server *self)
{
    char address[32];
    sprintf(address, "%p", &Server_embedded_i_startIdx);
    return PyString_FromString(address);
}

static PyMethodDef Server_methods[] = {
    {"setInputDevice", (PyCFunction)Server_setInputDevice, METH_O, "Sets audio input device."},
    {"setOutputDevice", (PyCFunction)Server_setOutputDevice, METH_O, "Sets audio output device."},
    {"setInputOffset", (PyCFunction)Server_setInputOffset, METH_O, "Sets audio input channel offset."},
    {"setOutputOffset", (PyCFunction)Server_setOutputOffset, METH_O, "Sets audio output channel offset."},
    {"setInOutDevice", (PyCFunction)Server_setInOutDevice, METH_O, "Sets both audio input and output device."},
    {"setMidiInputDevice", (PyCFunction)Server_setMidiInputDevice, METH_O, "Sets MIDI input device."},
    {"setMidiOutputDevice", (PyCFunction)Server_setMidiOutputDevice, METH_O, "Sets MIDI output device."},
    {"setSamplingRate", (PyCFunction)Server_setSamplingRate, METH_O, "Sets the server's sampling rate."},
    {"setBufferSize", (PyCFunction)Server_setBufferSize, METH_O, "Sets the server's buffer size."},
    {"setNchnls", (PyCFunction)Server_setNchnls, METH_O, "Sets the server's number of output/input channels."},
    {"setIchnls", (PyCFunction)Server_setIchnls, METH_O, "Sets the server's number of input channels."},
    {"setDuplex", (PyCFunction)Server_setDuplex, METH_O, "Sets the server's duplex mode (0 = only out, 1 = in/out)."},
    {"setJackAuto", (PyCFunction)Server_setJackAuto, METH_VARARGS, "Tells the server to auto-connect Jack ports (0 = disable, 1 = enable)."},
    {"setJackAutoConnectInputPorts", (PyCFunction)Server_setJackAutoConnectInputPorts, METH_O, "Sets a list of ports to auto-connect inputs when using Jack."},
    {"setJackAutoConnectOutputPorts", (PyCFunction)Server_setJackAutoConnectOutputPorts, METH_O, "Sets a list of ports to auto-connect outputs when using Jack."},
    {"setGlobalSeed", (PyCFunction)Server_setGlobalSeed, METH_O, "Sets the server's global seed for random objects."},
    {"setAmp", (PyCFunction)Server_setAmp, METH_O, "Sets the overall amplitude."},
    {"setAmpCallable", (PyCFunction)Server_setAmpCallable, METH_O, "Sets the Server's GUI callable object."},
    {"setTimeCallable", (PyCFunction)Server_setTimeCallable, METH_O, "Sets the Server's TIME callable object."},
    {"setVerbosity", (PyCFunction)Server_setVerbosity, METH_O, "Sets the verbosity."},
    {"setStartOffset", (PyCFunction)Server_setStartOffset, METH_O, "Sets starting time offset."},
    {"boot", (PyCFunction)Server_boot, METH_O, "Setup and boot the server."},
    {"shutdown", (PyCFunction)Server_shut_down, METH_NOARGS, "Shut down the server."},
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
    {"getStreams", (PyCFunction)Server_getStreams, METH_NOARGS, "Returns the list of streams added to the server."},
    {"getSamplingRate", (PyCFunction)Server_getSamplingRate, METH_NOARGS, "Returns the server's sampling rate."},
    {"getNchnls", (PyCFunction)Server_getNchnls, METH_NOARGS, "Returns the server's current number of output channels."},
    {"getIchnls", (PyCFunction)Server_getIchnls, METH_NOARGS, "Returns the server's current number of input channels."},
    {"getGlobalSeed", (PyCFunction)Server_getGlobalSeed, METH_NOARGS, "Returns the server's global seed."},
    {"getBufferSize", (PyCFunction)Server_getBufferSize, METH_NOARGS, "Returns the server's buffer size."},
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