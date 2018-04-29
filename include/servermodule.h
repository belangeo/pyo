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
 * Octobre 2013 : Multiple servers facility and embedded backend added    *
 * by Guillaume Barrette.                                                 *
 * 2014-2016 : Several improvements by Olivier Belanger.                  *
 *************************************************************************/

#ifndef Py_SERVERMODULE_H
#define Py_SERVERMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "sndfile.h"
#include "pyomodule.h"

typedef enum {
    PyoPortaudio = 0,
    PyoCoreaudio = 1,
    PyoJack,
    PyoOffline,
    PyoOfflineNB,
    PyoEmbedded
} PyoAudioBackendType;

typedef enum {
    PyoPortmidi = 0,
    PyoJackMidi = 1
} PyoMidiBackendType;

/* Portmidi midi message and event type clones. */

typedef long PyoMidiTimestamp;

#define PyoMidi_Message(status, data1, data2) \
         ((((data2) << 16) & 0xFF0000) | \
          (((data1) << 8) & 0xFF00) | \
          ((status) & 0xFF))
#define PyoMidi_MessageStatus(msg) ((msg) & 0xFF)
#define PyoMidi_MessageData1(msg) (((msg) >> 8) & 0xFF)
#define PyoMidi_MessageData2(msg) (((msg) >> 16) & 0xFF)

typedef long PyoMidiMessage; 
typedef struct {
    PyoMidiMessage      message;
    PyoMidiTimestamp    timestamp;
} PyoMidiEvent;

/************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *streams;
    PyoAudioBackendType audio_be_type;
    PyoMidiBackendType midi_be_type;
    void *audio_be_data;
    void *midi_be_data;
    char *serverName; /* Only used for jack client name */
    int jackautoin; /* jack port auto-connection (on by default) */
    int jackautoout; /* jack port auto-connection (on by default) */
    PyObject *jackAutoConnectInputPorts; /* list of lists of jack auto-connection ports to pyo inputs */
    PyObject *jackAutoConnectOutputPorts; /* list of lists of jack auto-connection ports from pyo outputs */
    PyObject *jackInputPortNames; /* string or list of strings (input port short names for jack server) */
    PyObject *jackOutputPortNames; /* string or list of strings (output port short names for jack server) */
    PyObject *jackAutoConnectMidiInputPort; /* lists of jack auto-connection ports to pyo midi input */
    PyObject *jackAutoConnectMidiOutputPort; /* lists of jack auto-connection ports from pyo midi output */
    PyObject *jackMidiInputPortName; /* string (midi input port short names for jack server) */
    PyObject *jackMidiOutputPortName; /* string (midi output port short names for jack server) */
    int isJackTransportSlave;
    int jack_transport_state; /* 0 = stopped, 1 = started */
    PyoMidiEvent midiEvents[200];
    int midiin_count;
    int midiout_count;
    int midi_count;
    long midi_time_offset;
    double samplingRate;
    int nchnls;
    int ichnls;
    int bufferSize;
    int currentResampling;
    int lastResampling;
    int duplex;
    int input;
    int output;
    int input_offset;
    int output_offset;
    int midi_input;
    int midi_output;
    int withPortMidi;
    int withPortMidiOut;
    int withJackMidi;
    int midiActive;
    int allowMMMapper;
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
    double recquality;
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

    /* custom callback */
    PyObject *CALLBACK;

    /* Globals dur and del times. */
    float globalDur;
    float globalDel;

    /* Properties */
    int verbosity; /* a sum of values to display different levels: 1 = error */
                   /* 2 = message, 4 = warning , 8 = debug. Default 7.*/
    int globalSeed; /* initial seed for random objects. If <= 0, objects are seeded with the clock. */
    int autoStartChildren; /* if true, calls to play, out and stop propagate to children objects. */
} Server;

PyObject * PyServer_get_server();
extern unsigned int pyorand();
extern PyObject * Server_removeStream(Server *self, int sid);
extern MYFLT * Server_getInputBuffer(Server *self);
extern PyoMidiEvent * Server_getMidiEventBuffer(Server *self);
extern int Server_getMidiEventCount(Server *self);
extern long Server_getMidiTimeOffset(Server *self);
extern unsigned long Server_getElapsedTime(Server *self);
extern int Server_getCurrentResamplingFactor(Server *self);
extern int Server_getLastResamplingFactor(Server *self);
extern int Server_generateSeed(Server *self, int oid);
extern PyTypeObject ServerType;
void pyoGetMidiEvents(Server *self);
void Server_process_buffers(Server *server);
void Server_error(Server *self, char * format, ...);
void Server_message(Server *self, char * format, ...);
void Server_warning(Server *self, char * format, ...);
void Server_debug(Server *self, char * format, ...);
PyObject * Server_shutdown(Server *self);
PyObject * Server_stop(Server *self);
PyObject * Server_start(Server *self);
PyObject * Server_boot(Server *self, PyObject *arg);
void Server_process_gui(Server *server);
void Server_process_time(Server *server);
int Server_start_rec_internal(Server *self, char *filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* Py_SERVERMODULE_H */
