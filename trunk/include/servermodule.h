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

#ifndef Py_SERVERMODULE_H
#define Py_SERVERMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "portaudio.h"
#include "portmidi.h"
#include "sndfile.h"
#include "pyomodule.h"

#ifdef USE_JACK
#include <jack/jack.h>
#endif

#ifdef USE_COREAUDIO
# include <CoreAudio/AudioHardware.h>
#endif

typedef enum {
    PyoPortaudio = 0,
    PyoCoreaudio = 1,
    PyoJack,
    PyoOffline
} PyoAudioBackendType;

typedef struct {
    PaStream *stream;
} PyoPaBackendData;

typedef struct {
#ifdef USE_JACK
    jack_client_t *jack_client;
    jack_port_t **jack_in_ports;
    jack_port_t **jack_out_ports;
#endif
} PyoJackBackendData;
    
typedef struct {
    PyObject_HEAD
    PyObject *streams;
    PyoAudioBackendType audio_be_type;
    void *audio_be_data;
    char *serverName; /* Only used for jack client name */
    PmStream *in;
    PmEvent midiEvents[200];
    int midi_count;
    MYFLT samplingRate;
    int nchnls;
    int bufferSize;
    int duplex;
    int input;
    int output;
    int midi_input;
    int withPortMidi;
    int server_started;
    int server_stopped; /* for fadeout */
    int server_booted;
    int stream_count;
    int record;
    /* global amplitude */
    MYFLT amp;
    MYFLT resetAmp;
    MYFLT lastAmp;
    MYFLT currentAmp;
    MYFLT stepVal;
    int timeStep;
    int timeCount;
    
    MYFLT *input_buffer;
    float *output_buffer; /* Has to be float since audio callbacks must use floats */
    /* rendering settings */
    double recdur;
    char *recpath;
    int recformat;
    int rectype;
    SNDFILE *recfile;
    SF_INFO recinfo;
    /* GUI VUMETER */
    int withGUI;
    int numPass;
    int gcount;
    float *lastRms;
    PyObject *GUI;

    unsigned long elapsedSamples; /* time since the server was started */
    int withTIME;
    int timePass;
    int tcount;
    PyObject *TIME;
    
    /* Properties */
    int verbosity; /* a sum of values to display different levels: 1 = error */
                   /* 2 = message, 4 = warning , 8 = debug. Default 7.*/
} Server;

PyObject * PyServer_get_server();
extern PyObject * Server_removeStream(Server *self, int sid);
extern MYFLT * Server_getInputBuffer(Server *self);    
extern PmEvent * Server_getMidiEventBuffer(Server *self);    
extern int Server_getMidiEventCount(Server *self);    
extern PyTypeObject ServerType;    
    

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SERVERMODULE_H) */

