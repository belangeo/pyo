#include <Python.h>
#include "structmember.h"
#include "portaudio.h"
#include "portmidi.h"
#include "sndfile.h"
#include "streammodule.h"

#define SERVER_MODULE
#include "servermodule.h"
#undef SERVER_MODULE

static Server *my_server = NULL;

/* Portmidi get input events */
static void portmidiGetEvents(Server *self) 
{
    PmError status, length;	
    while (status = Pm_Poll(self->in))		// For all messages in queue
    {
        if (status == TRUE) {	// If no error
            length = Pm_Read(self->in, self->midibuf,1);	// Read the next message
        }
    }
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
static int callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *server )
{
 
    float *out = (float*)outputBuffer;
    
    int i, j;
    int todac;
    int count = my_server->stream_count;
    int nchnls = my_server->nchnls;
    Stream *stream_tmp;
    float *data;
    float old;
    
    /* avoid unused variable warnings */
    (void) timeInfo;
    (void) statusFlags;

    if (my_server->withPortMidi == 1) {
        portmidiGetEvents((Server *)my_server);
    }
    
    if (my_server->duplex == 1) {
        float *in = (float*)inputBuffer;
        for (i=0; i<framesPerBuffer*nchnls; i++) {
            my_server->input_buffer[i] = in[i];
        }
    }    
    else 
        (void) inputBuffer;
    
    float buffer[nchnls][framesPerBuffer];
    memset(&buffer, 0, sizeof(buffer));
    
    PyGILState_STATE s = PyGILState_Ensure();
    for (i=0; i<count; i++) {

        stream_tmp = (Stream *)PyList_GET_ITEM(my_server->streams, i);
        if (Stream_getStreamActive(stream_tmp) == 0) {
            continue;
        }
        
        if (Stream_getStreamToDac(stream_tmp) != 0) {
            data = Stream_getData(stream_tmp);
            int chnl = Stream_getStreamChnl(stream_tmp);
            for (j=0; j<framesPerBuffer; j++) {
                old = buffer[chnl][j];
                buffer[chnl][j] = *data + old;
                *data++;
            }
        } 
        Stream_callFunction(stream_tmp);
    }

    PyGILState_Release(s);
    
    for (i=0; i<framesPerBuffer; i++){
        for (j=0; j<nchnls; j++) {
            out[i*nchnls+j] = buffer[j][i];
        }
    }
    
    if (my_server->record == 1)
        sf_write_float(my_server->recfile, out, framesPerBuffer * nchnls);
    
    return paContinue;
}

/***************************************************/

/* Global function called by any new audio object to 
 get a pointer to the server */
PyObject *
PyServer_get_server()
{
    return (PyObject *)my_server;
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
    PaError err;
    err = Pa_CloseStream(self->stream);
    portaudio_assert(err, "Pa_CloseStream");
            
    err = Pa_Terminate();
    portaudio_assert(err, "Pa_Terminate");
    
    Py_CLEAR(self->stream);
    Py_CLEAR(self->streams);
    return 0;
}

static void
Server_dealloc(Server* self)
{
    if (self->withPortMidi == 1) {
        Pm_Close(self->in);
        Pm_Terminate();
    }    
    free(self->input_buffer);
    Server_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Server_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Server *self;
    self = (Server *)type->tp_alloc(type, 0);
    self->samplingRate = 44100.0;
    self->nchnls = 1;
    self->record = 0;
    self->bufferSize = 64;
    self->duplex = 0;
    Py_XDECREF(my_server);
    Py_XINCREF(self);
    my_server = (Server *)self;
    return (PyObject *)self;
}

static int
Server_init(Server *self, PyObject *args, PyObject *kwds)
{
    PaError err;
    PmError pmerr;
    int i;

    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|fiii", kwlist, &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex))
        return -1;

    self->server_started = 0;
    self->stream_count = 0;
    self->streams = PyList_New(0);
    err = Pa_Initialize();
    portaudio_assert(err, "Pa_Initialize");

    int n = Pa_GetDeviceCount();
    if (n < 0) {
        portaudio_assert(n, "Pa_GetDeviceCount");
    }

    // -- setup input and output -- 
    PaStreamParameters inputParameters;
    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.device = Pa_GetDefaultInputDevice(); // default input device 
    inputParameters.channelCount = self->nchnls;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaStreamParameters outputParameters;
    memset(&outputParameters, 0, sizeof(outputParameters));
    outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device 
    outputParameters.channelCount = self->nchnls;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_IsFormatSupported(&inputParameters, &outputParameters, self->samplingRate);
    portaudio_assert(err, "Pa_IsFormatSupported");

    self->input_buffer = (float *)realloc(self->input_buffer, self->bufferSize * self->nchnls * sizeof(float));
    for (i=0; i<self->bufferSize*self->nchnls; i++) {
        self->input_buffer[i] = 0.;
    }    

    if (self->duplex == 1)
        err = Pa_OpenDefaultStream(&self->stream, self->nchnls, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, callback, NULL);
    else
        err = Pa_OpenDefaultStream(&self->stream, 0, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, callback, NULL);
    //err = Pa_OpenStream(&self->stream, &inputParameters, &outputParameters, self->samplingRate, self->bufferSize, paClipOff, callback, NULL);
    portaudio_assert(err, "Pa_OpenStream");

    pmerr = Pm_Initialize();
    if (pmerr) {
        printf("could not initialize PortMidi: %s\n", Pm_GetErrorText(pmerr));
        self->withPortMidi = 0;
    }    
    else {
        printf("PortMidi initialized.\n");
        self->withPortMidi = 1;
    }    

    if (self->withPortMidi == 1) {
        int num_devices = Pm_CountDevices();
        if (num_devices > 0) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(0);
            if (info->input) {
                pmerr = Pm_OpenInput(&self->in, 0, NULL, 100, NULL, NULL);
                if (pmerr) {
                    printf("could not open midi input %d (%s): %s\nPortmidi closed\n", 0, info->name, Pm_GetErrorText(pmerr));
                    self->withPortMidi = 0;
                    Pm_Terminate();
                }    
                else
                    printf("Midi Input (%s) opened.\n", info->name);
            }
            else {
                printf("Something wrong with midi device!\nPortmidi closed\n");
                self->withPortMidi = 0;
                Pm_Terminate();
            }    
        }    
        else {
            printf("No midi device found!\nPortmidi closed\n");
            self->withPortMidi = 0;
            Pm_Terminate();
        }    
    }
    if (self->withPortMidi == 1) {
        Pm_SetFilter(self->in, PM_FILT_ACTIVE | PM_FILT_CLOCK);
        while (Pm_Poll(self->in)) {
            Pm_Read(self->in, self->midibuf, 1);
        }
    }
    
    return 0;
}


static PyObject *
Server_start(Server *self)
{
    PaError err;
    int i;
	/* Ensure Python is set up for threading */
	PyEval_InitThreads();
 
    err = Pa_StartStream(self->stream);
    portaudio_assert(err, "Pa_StartStream");

    self->server_started = 1;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_stop(Server *self)
{
    PaError err;

    err = Pa_StopStream(self->stream);
    portaudio_assert(err, "Pa_StopStream");

    self->server_started = 0;
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start_rec(Server *self, PyObject *args)
{
    char *path = getenv("HOME");
    strncat(path, "/pyo_rec.aif", strlen("/pyo_rec.aif"));
    
    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->samplingRate;
    self->recinfo.channels = self->nchnls;
    self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_FLOAT;
    
    /* Open the output file. */
    if (! (self->recfile = sf_open(path, SFM_WRITE, &self->recinfo))) {   
        printf ("Not able to open output file %s.\n", path) ;
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

    self->stream_count += 1;
    
    Py_INCREF(Py_None);
    return Py_None;    
}

float *
Server_getInputBuffer(Server *self) {
    return (float *)self->input_buffer;
}

PmEvent *
Server_getMidiEventBuffer(Server *self) {
    return (PmEvent *)self->midibuf;
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
Server_getStreams(Server *self)
{
    Py_INCREF(self->streams);
    return self->streams;
}

static PyMethodDef Server_methods[] = {
	{"start", (PyCFunction)Server_start, METH_NOARGS, "Starts the server's callback loop."},
    {"stop", (PyCFunction)Server_stop, METH_NOARGS, "Stops the server's callback loop."},
    {"recstart", (PyCFunction)Server_start_rec, METH_NOARGS, "Start automatic output recording."},
    {"recstop", (PyCFunction)Server_stop_rec, METH_NOARGS, "Stop automatic output recording."},
    {"addStream", (PyCFunction)Server_addStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called the user."},
    {"getStreams", (PyCFunction)Server_getStreams, METH_NOARGS, "Returns the list of streams added to the server."},
    {"getSamplingRate", (PyCFunction)Server_getSamplingRate, METH_NOARGS, "Returns the server's sampling rate."},
    {"getNchnls", (PyCFunction)Server_getNchnls, METH_NOARGS, "Returns the server's current number of channels."},
    {"getBufferSize", (PyCFunction)Server_getBufferSize, METH_NOARGS, "Returns the server's buffer size."},
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
