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
 *                                                                        *
 * Octobre 2013 : Multiple servers facility and embeded backend added by  *
 * Guillaume Barrette. See embeded possibilities at:                      *
 * https://github.com/guibarrette/PyoPlug                                 *
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
    PyoOffline,
    PyoOfflineNB,
    PyoEmbedded
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
    int jackautoin; /* jack port auto-connection (on by default) */
    int jackautoout; /* jack port auto-connection (on by default) */
    PyObject *jackAutoConnectInputPorts; /* list of regex to match for jack auto-connection */
    PyObject *jackAutoConnectOutputPorts; /* list of regex to match for jack auto-connection */
    PmStream *midiin[64];
    PmStream *midiout[64];
    int midiin_count;
    int midiout_count;
    PmEvent midiEvents[200];
    int midi_count;
    double samplingRate;
    int nchnls;
    int ichnls;
    int bufferSize;
    int duplex;
    int input;
    int output;
    int input_offset;
    int output_offset;
    int midi_input;
    int midi_output;
    int withPortMidi;
    int withPortMidiOut;
    int server_started;
    int server_stopped; /* for fadeout */
    int server_booted;
    int stream_count;
    int record;
    int thisServerID;       /* To keep the reference index in the array of servers */

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

    /* rendering offline of the first "startoffset" seconds */
    double startoffset;

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

    /* Current time */
    unsigned long elapsedSamples; /* time since the server was started */
    int withTIME;
    int timePass;
    int tcount;
    PyObject *TIME;

    /* Properties */
    int verbosity; /* a sum of values to display different levels: 1 = error */
                   /* 2 = message, 4 = warning , 8 = debug. Default 7.*/
    int globalSeed; /* initial seed for random objects. If -1, objects are seeded with the clock. */
} Server;

PyObject * PyServer_get_server();
extern PyObject * Server_removeStream(Server *self, int sid);
extern MYFLT * Server_getInputBuffer(Server *self);
extern PmEvent * Server_getMidiEventBuffer(Server *self);
extern int Server_getMidiEventCount(Server *self);
extern int Server_generateSeed(Server *self, int oid);
extern PyTypeObject ServerType;

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SERVERMODULE_H) */
