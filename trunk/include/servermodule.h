#ifndef Py_SERVERMODULE_H
#define Py_SERVERMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "portaudio.h"
#include "portmidi.h"
#include "sndfile.h"    

typedef struct {
    PyObject_HEAD
    PyObject *streams;
    PaStream *stream;
    PmStream *in;
    PmEvent midiEvents[200];
    int midi_count;
    float samplingRate;
    int nchnls;
    int bufferSize;
    int duplex;
    int input;
    int output;
    int midi_input;
    int withPortMidi;
    int server_started;
    int server_booted;
    int stream_count;
    int record;
    /* global amplitude */
    float amp;
    float lastAmp;
    float currentAmp;
    float stepVal;
    int timeStep;
    int timeCount;
    
    float *input_buffer;
    char *recpath;
    SNDFILE *recfile;
    SF_INFO recinfo;
    /* GUI VUMETER */
    int withGUI;
    int numPass;
    int gcount;
    float *lastRms;
    PyObject *GUI;
} Server;

PyObject * PyServer_get_server();
extern PyObject * Server_removeStream(Server *self, int sid);
extern float * Server_getInputBuffer(Server *self);    
extern PmEvent * Server_getMidiEventBuffer(Server *self);    
extern int Server_getMidiEventCount(Server *self);    
extern PyTypeObject ServerType;    
    

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SERVERMODULE_H) */

