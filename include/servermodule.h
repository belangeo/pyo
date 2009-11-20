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
    PmEvent midibuf[1];
    float samplingRate;
    int nchnls;
    int bufferSize;
    int duplex;
    int withPortMidi;
    int server_started;
    int stream_count;
    int record;
    float *input_buffer;
    SNDFILE *recfile;
    SF_INFO recinfo;
} Server;

PyObject * PyServer_get_server();
static int Server_traverse(Server *self, visitproc visit, void *arg);
static int Server_clear(Server *self);
static void Server_dealloc(Server* self);    
static PyObject * Server_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Server_init(Server *self, PyObject *args, PyObject *kwds);
static PyObject * Server_start(Server *self);
static PyObject * Server_stop(Server *self);    
static PyObject * Server_addStream(Server *self, PyObject *args);
extern PyObject * Server_removeStream(Server *self, int sid);
extern float * Server_getInputBuffer(Server *self);    
extern PmEvent * Server_getMidiEventBuffer(Server *self);    
static PyObject * Server_getSamplingRate(Server *self);
static PyObject * Server_getBufferSize(Server *self);
static PyObject * Server_getStreams(Server *self);
extern PyTypeObject ServerType;    
    

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SERVERMODULE_H) */

