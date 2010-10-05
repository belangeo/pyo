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
static void Server_process_gui(Server *server);
static inline void Server_process_buffers(Server *server);
static int Server_start_rec_internal(Server *self, char *filename);

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
    if (self->verbosity & 4) {
        char buffer[256];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
        va_end (args);
        printf("%s",buffer);
    }
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
        printf("portaudio error in %s: %s\n", cmdName, eText);
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
            server->input_buffer[i] = (MYFLT)in[i];
        }
    }

    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server);

    if (server->withGUI == 1 && nchnls <= 8) {
        Server_process_gui(server);
    }
    PyGILState_Release(s);
    for (i=0; i<server->bufferSize*server->nchnls; i++) {
        out[i] = (float) server->output_buffer[i];
    }
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
                server->input_buffer[(i*server->nchnls)+j] = (MYFLT) in_buffers[j][i];
            }
        }
    }
    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server);
    if (server->withGUI == 1 && server->nchnls <= 8) {
        Server_process_gui(server);
    }
    PyGILState_Release(s);
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
    s->samplingRate = (float) nframes;
    Server_debug(s, "the sample rate is now %lu/sec\n", (unsigned long) nframes);
    return 0;
}

static int
jack_bufsize_cb (jack_nframes_t nframes, void *arg)
{
    Server *s = (Server *) arg;
    s->bufferSize = (int) nframes;
    Server_debug(s, "the buffer size is now %lu/sec\n", (unsigned long) nframes);
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
    int i;
    Server *server = (Server *) defptr;
    (void) outOutputData;
    const AudioBuffer* inputBuf = inInputData->mBuffers;
    float *bufdata = (float*)inputBuf->mData;
    for (i=0; i<server->bufferSize*server->nchnls; i++) {
        server->input_buffer[i] = (MYFLT)bufdata[i];
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
    int i, j;
    Server *server = (Server *) defptr;

    (void) inInputData;
    
    //float outputBuffer[server->nchnls*server->bufferSize];
    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server);
    if (server->withGUI == 1 && server->nchnls <= 8) {
        Server_process_gui(server);
    }
    PyGILState_Release(s);
    AudioBuffer* outputBuf = outOutputData->mBuffers;
    float *bufdata = (float*)outputBuf->mData;
    for (i=0; i<server->bufferSize*server->nchnls; i++) {
        bufdata[i] = server->output_buffer[i];
    }
    server->midi_count = 0;

    if (server->server_started == 1) {
        if (server->server_stopped == 1 && server->currentAmp < 0.0001)
            coreaudio_stop_callback(server);
    }        

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
    PyGILState_STATE s = PyGILState_Ensure();
    Server_process_buffers(server);
    PyGILState_Release(s);
    return 0;
}

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
    PyoPaBackendData *be_data = (PyoPaBackendData *) malloc(sizeof(PyoPaBackendData *));
    self->audio_be_data = (void *) be_data;
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

    if (self->input == -1 && self->output == -1) {
        if (self->duplex == 1)
            err = Pa_OpenDefaultStream(&be_data->stream, self->nchnls, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, pa_callback, (void *) self);
        else
            err = Pa_OpenDefaultStream(&be_data->stream, 0, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, pa_callback, (void *) self);
    }
    else
        err = Pa_OpenStream(&be_data->stream, &inputParameters, &outputParameters, self->samplingRate, self->bufferSize, paNoFlag, pa_callback,  (void *) self);
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
        err = Pa_StopStream(be_data->stream);
        portaudio_assert(err, "Pa_StopStream");
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
        err = Pa_StopStream(be_data->stream);
        portaudio_assert(err, "Pa_StopStream");
    }
    err = Pa_StartStream(be_data->stream);
    portaudio_assert(err, "Pa_StartStream");
    return err;
}

int 
Server_pa_stop(Server *self)
{
    self->timeStep = (int)(0.1 * self->samplingRate);
    self->amp = 0.;
    self->server_stopped = 1;
    return 0;
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
        Server_error(self, "Jack: Cannot find any physical capture ports\n");
        ret = -1;
    }
    int i=0;
    while(ports[i]!=NULL && be_data->jack_in_ports[i] != NULL){
        if (jack_connect (be_data->jack_client, ports[i], jack_port_name(be_data->jack_in_ports[i]))) {
            Server_warning(self, "Jack: cannot connect input ports\n");
            ret = -1;
        }
        i++;
    }
    free (ports);
    
    if ((ports = jack_get_ports (be_data->jack_client, NULL, NULL, 
        JackPortIsPhysical|JackPortIsInput)) == NULL) {
        Server_error(self, "Jack: Cannot find any physical playback ports\n");
        ret = -1;
    }
    
    i=0;
    while(ports[i]!=NULL && be_data->jack_out_ports[i] != NULL){
        if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
            Server_warning(self, "Jack: cannot connect output ports\n");
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
    char client_name[32];
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
        Server_message(self, "JACK server started.\n");
    }
    if (strcmp(self->serverName, jack_get_client_name(be_data->jack_client)) ) {
        strcpy(self->serverName, jack_get_client_name(be_data->jack_client));
        Server_warning(self, "Jack name `%s' assigned\n", self->serverName);
    }
    
    sampleRate = jack_get_sample_rate (be_data->jack_client);
    if (sampleRate != self->samplingRate) {
        self->samplingRate = sampleRate;
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
            Server_error(self, "Jack: no more JACK ports available\n");
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
    AudioDeviceID mOutputDevice = kAudioDeviceUnknown;
    AudioDeviceID mInputDevice = kAudioDeviceUnknown;
    AudioTimeStamp	now;
    Boolean writable;
    
    // List all coreaudio devices
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &count, 0);
    AudioDeviceID *devices = (AudioDeviceID*) malloc(count);    
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &count, devices);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "get kAudioHardwarePropertyDevices error %s\n", (char*)&err);
        free(devices);
    }

    numdevices = count / sizeof(AudioDeviceID);
    Server_debug(self, "Coreaudio : Number of devices: %i\n", numdevices);

    for (i=0; i<numdevices; ++i) {
        err = AudioDeviceGetPropertyInfo(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "info kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            break;
        }
        
        char *name = (char*)malloc(count);
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "get kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            free(name);
            break;
        }
        Server_message(self, "   %d : \"%s\"\n", i, name);
        free(name);
    }    
    /////////////////////////////

    // Acquire audio devices    
    if (self->output != -1)
        mOutputDevice = devices[self->output];
        
    if (mOutputDevice==kAudioDeviceUnknown) {
        count = sizeof(mOutputDevice);
        //get the output device:
        err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &count, (void *) &mOutputDevice);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "get kAudioHardwarePropertyDefaultOutputDevice error %s\n", (char*)&err);
            return -1;
        }
    }

    err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }    
    char *name = (char*)malloc(namelen);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    Server_message(self, "Coreaudio : Uses output device : \"%s\"\n", name);
    self->output = mOutputDevice;
    free(name);

    if (self->duplex == 1) {
        if (self->input != -1)
            mInputDevice = devices[self->input];
    
        if (mInputDevice==kAudioDeviceUnknown) {
            count = sizeof(mInputDevice);
            err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &count, (void *) &mInputDevice);
            //get the input device:
            if (err != kAudioHardwareNoError) {
                Server_error(self, "get kAudioHardwarePropertyDefaultInputDevice error %s\n", (char*)&err);
                return -1;
            }
        }     

        err = AudioDeviceGetPropertyInfo(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }    
        name = (char*)malloc(namelen);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }
        Server_message(self, "Coreaudio : Uses input device : \"%s\"\n", name);
        self->input = mInputDevice;
        free(name);
    }    
    //////////////////////////
    
    now.mFlags = kAudioTimeStampHostTimeValid;
    now.mHostTime = AudioGetCurrentHostTime();
    
    // set/get the buffersize for the devices
    count = sizeof(UInt32);
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
    }    

    if (self->duplex == 1) {
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        }    
    }    
    
    err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, 0);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "info kAudioDevicePropertyBufferFrameSize error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    long bufferSize;
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, &bufferSize);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);
        return -1;
    }
    Server_message(self, "Coreaudio : Current buffer size = %ld\n", bufferSize);

    
    // set/get the sampling rate for the devices
    count = sizeof(double);
    double pyoSamplingRate = (double)self->samplingRate;
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, 0);
        if (err != kAudioHardwareNoError) {
            Server_debug(self, "info kAudioDevicePropertyNominalSampleRate error %s A %08X\n", (char*)&err, mOutputDevice);
        }
        double sampleRate;
        err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, &sampleRate);
        if (err != kAudioHardwareNoError) {
            Server_debug(self, "get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
            return -1;
        }
        self->samplingRate = (int)sampleRate;
        Server_message(self, "Coreaudio : Current sampling rate = %i\n", self->samplingRate);
    }
    if (self->duplex ==1) {
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        }
    }    

    // set IOprocs (callbacks)
        
    if (self->duplex == 1) {
        err = AudioDeviceAddIOProc(self->input, coreaudio_input_callback, (void *) self);	// setup our device with an IO proc
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceAddIOProc failed %d\n", (int)err);
            return -1;
        }        
        err = AudioDeviceGetPropertyInfo(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
        AudioHardwareIOProcStreamUsage *su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
        su->mIOProc = (void*)coreaudio_input_callback;
        err = AudioDeviceGetProperty(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, su);
        for (i=0; i<self->nchnls; ++i) {
            su->mStreamIsOn[i] = 1;
        }
        err = AudioDeviceSetProperty(self->input, &now, 0, true, kAudioDevicePropertyIOProcStreamUsage, propertySize, su);
    }
    
    err = AudioDeviceAddIOProc(self->output, coreaudio_output_callback, (void *) self);	// setup our device with an IO proc
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceAddIOProc failed %d\n", (int)err);
        return -1;
    }
    err = AudioDeviceGetPropertyInfo(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
    AudioHardwareIOProcStreamUsage *su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
    su->mIOProc = (void*)coreaudio_output_callback;
    err = AudioDeviceGetProperty(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, su);
    for (i=0; i<self->nchnls; ++i) {
        su->mStreamIsOn[i] = 1;
    }
    err = AudioDeviceSetProperty(self->output, &now, 0, false, kAudioDevicePropertyIOProcStreamUsage, propertySize, su);
    
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
    self->timeStep = (int)(0.1 * self->samplingRate);
    self->amp = 0.;
    self->server_stopped = 1;
    return 0;
}

#endif

int
Server_offline_init (Server *self)
{
    return 0;
}

int
Server_offline_deinit (Server *self)
{
    return 0;
}

int
Server_offline_start (Server *self)
{
    if (self->recdur < 0) {
        Server_error(self,"Duration must be specified for Offline Server (see Server.recordOptions).");
        return -1;
    }
    Server_message(self,"Offline Server rendering file %s dur=%f\n", self->recpath, self->recdur);
    int numBlocks = ceil(self->recdur * self->samplingRate/self->bufferSize);
    Server_debug(self,"Number of blocks: %i\n", numBlocks);
    Server_start_rec_internal(self, self->recpath);
    self->server_stopped = 0;
    self->server_started = 1;
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
Server_offline_stop (Server *self)
{
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
    int count = server->stream_count;
    MYFLT amp = server->amp;
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
    int ret = -1;
    if (self->server_booted == 0) {
        Server_error(self, "The Server must be booted!");
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
    }
    self->server_booted = 0;
    if (ret < 0) {
        Server_error(self, "Error closing audio backend.\n");
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
    Py_VISIT(self->streams);
    return 0;
}

static int 
Server_clear(Server *self)
{    
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
        Server_warning((Server *) PyServer_get_server(), "Warning: A Server is already created!"
            "If you put this Server in a new variable, please delete it!");
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
    self->verbosity = 7;
    self->recdur = -1;
    self->recformat = 0;
    self->rectype = 0;
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

    Server_debug(self, "Server_init. Compiled " TIMESTAMP "\n");  // Only for debugging purposes
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FIIISS, kwlist, 
            &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex, &audioType, &serverName))
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
    else {
        Server_warning(self, "Unknown audio type. Using Portaudio\n");
        self->audio_be_type = PyoPortaudio;
    }
    strncpy(self->serverName, serverName, 32);
    if (strlen(serverName) > 31) {
        self->serverName[31] = '\0';
    }
    self->recpath = getenv("HOME");
    if (self->recpath != NULL)
        strncat(self->recpath, "/pyo_rec.wav", strlen("/pyo_rec.wav")); 
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
        Server_warning(self, "Can't change sampling rate for booted server.\n");
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (arg != NULL && PyInt_Check(arg)) {
        self->samplingRate = PyInt_AsLong(arg);
    }
    else {
        Server_error(self, "Sampling rate must be an integer.\n");
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

int
Server_pm_init(Server *self)
{
   /* Initializing MIDI */    
    PmError pmerr;
    pmerr = Pm_Initialize();
    if (pmerr) {
        Server_warning(self, "PortMidi warning: could not initialize PortMidi: %s\n", Pm_GetErrorText(pmerr));
        self->withPortMidi = 0;
        return -1;
    }    
    else {
        Server_debug(self, "PortMidi initialized.\n");
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
                    Server_warning(self, 
                                 "PortMidi warning: could not open midi input %d (%s): %s\nPortmidi closed\n",
                                 0, info->name, Pm_GetErrorText(pmerr));
                    self->withPortMidi = 0;
                    Pm_Terminate();
                }    
                else
                    Server_debug(self, "Midi Input (%s) opened.\n", info->name);
            }
            else {
                Server_warning(self, "PortMidi warning: Something wrong with midi device!\nPortmidi closed\n");
                self->withPortMidi = 0;
                Pm_Terminate();
            }    
        }    
        else {
            Server_warning(self, "PortMidi warning: No midi device found!\nPortmidi closed\n");
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
        Server_error(self, "Server already booted!\n");
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
    }
    // Must allocate buffer after initializing the audio backend in case parameters change there
    if (self->input_buffer) {
        free(self->input_buffer);
    }
    self->input_buffer = (MYFLT *)calloc(self->bufferSize * self->nchnls, sizeof(MYFLT));
    if (self->output_buffer) {
        free(self->output_buffer);
    }
    self->output_buffer = (float *)calloc(self->bufferSize * self->nchnls, sizeof(float));
    for (i=0; i<self->bufferSize*self->nchnls; i++) {
        self->input_buffer[i] = 0.0;
        self->output_buffer[i] = 0.0;
    }
    if (audioerr == 0 && midierr == 0) {
        self->server_booted = 1;
    }
    else {
        self->server_booted = 0;
        Server_error(self, "Server not booted.\n");
    }    
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start(Server *self)
{
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
    int err = -1;
    
    /* Ensure Python is set up for threading */
    PyEval_InitThreads();

    self->amp = self->resetAmp;
    self->server_stopped = 0;
    self->server_started = 1;
    self->timeStep = (int)(0.01 * self->samplingRate);
    
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
    }
    if (err) {
        Server_error(self, "Error starting server.");
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_stop(Server *self)
{
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
    }

    if (err < 0) {
        Server_error(self, "Error stopping server.\n");
    }
    else {
        self->server_stopped = 1;
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

    switch (self->recformat) {
        case 0:
            self->recinfo.format = SF_FORMAT_WAV;
            break;
        case 1:
            self->recinfo.format = SF_FORMAT_AIFF;
            break;
    }
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
    }
    
    /* Open the output file. */
    if (filename == NULL) {
        if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {   
            Server_error(self, "Not able to open output file %s.\n", self->recpath);
            return -1;
        }
    }
    else {
        if (! (self->recfile = sf_open(filename, SFM_WRITE, &self->recinfo))) {   
            Server_error(self, "Not able to open output file %s.\n", filename);
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
        Server_error(self, "Need a pyo object as argument\n");
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
    {"setVerbosity", (PyCFunction)Server_setVerbosity, METH_O, "Sets the verbosity."},
    {"boot", (PyCFunction)Server_boot, METH_NOARGS, "Setup and boot the server."},
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
