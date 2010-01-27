#include <Python.h>
#include <math.h>
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
static int callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *server )
{
 
    float *out = (float*)outputBuffer;
    
    int i, j;
    int count = my_server->stream_count;
    int nchnls = my_server->nchnls;
    float amp = my_server->amp;
    Stream *stream_tmp;
    float *data;
    
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
                buffer[chnl][j] += *data++;
            }
        } 
        Stream_callFunction(stream_tmp);
    }
    
    for (i=0; i<framesPerBuffer; i++){
        for (j=0; j<nchnls; j++) {
            out[i*nchnls+j] = buffer[j][i] * amp;
        }
    }
    
    if (my_server->record == 1)
        sf_write_float(my_server->recfile, out, framesPerBuffer * nchnls);

    if (my_server->withGUI == 1) {
        float rms[nchnls];
        float outAmp;
        for (j=0; j<nchnls; j++) {
            rms[j] = 0.0;
            for (i=0; i<framesPerBuffer; i++) {
                outAmp = out[i*nchnls+j];
                outAmp *= outAmp;
                if (outAmp > rms[j])
                    rms[j] = outAmp;
            }
            rms[j] = sqrtf(rms[j]);
        }    
        if (my_server->gcount <= my_server->numPass) {
            for (j=0; j<nchnls; j++) {            
                my_server->lastRms[j] = (rms[j] + my_server->lastRms[j]) * 0.5;
            }    
            my_server->gcount++;
        }
        else {
            for (j=0; j<nchnls; j++) {            
                my_server->lastRms[j] = (rms[j] + my_server->lastRms[j]) * 0.5;
            }  
            switch (nchnls) {
                case 1:
                    PyObject_CallMethod((PyObject *)my_server->GUI, "setRms", "f", my_server->lastRms[0]);
                    break;
                case 2:
                    PyObject_CallMethod((PyObject *)my_server->GUI, "setRms", "ff", my_server->lastRms[0], my_server->lastRms[1]);
                    break;
            }        
            my_server->gcount = 0;
        }
    }    
        
    my_server->midi_count = 0;

    PyGILState_Release(s);

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


static PyObject * Server_stop(Server *self);

static PyObject *
Server_shut_down(Server *self)
{
    if (self->server_booted == 0) {
        PyErr_Warn(NULL, "The Server must be booted!");
        Py_INCREF(Py_None);
        return Py_None;
    }    

    if (self->server_started == 1) {
        Server_stop((Server *)self);
        self->server_started = 0;
    }
    
    if (self->withPortMidi == 1) {
        Pm_Close(self->in);
        Pm_Terminate();
    } 
    
    self->server_booted = 0;
    
    PaError err;
    err = Pa_CloseStream(self->stream);
    portaudio_assert(err, "Pa_CloseStream");
    
    err = Pa_Terminate();
    portaudio_assert(err, "Pa_Terminate");
    
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
    self->samplingRate = 44100.0;
    self->nchnls = 2;
    self->record = 0;
    self->bufferSize = 256;
    self->duplex = 0;
    self->input = -1;
    self->output = -1;
    self->midi_input = -1;
    self->amp = 1.;
    self->withGUI = 0;
    Py_XDECREF(my_server);
    Py_XINCREF(self);
    my_server = (Server *)self;
    return (PyObject *)self;
}

static int
Server_init(Server *self, PyObject *args, PyObject *kwds)
{

    static char *kwlist[] = {"sr", "nchnls", "buffersize", "duplex", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|fiii", kwlist, &self->samplingRate, &self->nchnls, &self->bufferSize, &self->duplex))
        return -1;

    self->recpath = getenv("HOME");
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
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->samplingRate = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setNchnls(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->nchnls = PyInt_AsLong(arg);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_setBufferSize(Server *self, PyObject *arg)
{
	if (arg != NULL) {
        if (PyInt_Check(arg))
            self->bufferSize = PyInt_AsLong(arg);
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
        
        if (check)
            self->amp = PyFloat_AsDouble(PyNumber_Float(arg));
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

static PyObject *
Server_boot(Server *self)
{
    if (self->server_booted == 1) {
        PyErr_Warn(NULL, "Server already booted!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    
    PaError err;
    PmError pmerr;
    int i;

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

    self->input_buffer = (float *)realloc(self->input_buffer, self->bufferSize * self->nchnls * sizeof(float));
    for (i=0; i<self->bufferSize*self->nchnls; i++) {
        self->input_buffer[i] = 0.;
    }    

    if (self->input == -1 && self->output == -1) {
        if (self->duplex == 1)
            err = Pa_OpenDefaultStream(&self->stream, self->nchnls, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, callback, NULL);
        else
            err = Pa_OpenDefaultStream(&self->stream, 0, self->nchnls, paFloat32, self->samplingRate, self->bufferSize, callback, NULL);
    }
    else
        err = Pa_OpenStream(&self->stream, &inputParameters, &outputParameters, self->samplingRate, self->bufferSize, paNoFlag, callback, NULL);
    portaudio_assert(err, "Pa_OpenStream");

    /* Initializing MIDI */    
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
            if (self->midi_input == -1 || self->midi_input >= num_devices)
                self->midi_input = 0;
            const PmDeviceInfo *info = Pm_GetDeviceInfo(self->midi_input);
            if (info->input) {
                pmerr = Pm_OpenInput(&self->in, self->midi_input, NULL, 100, NULL, NULL);
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
        self->midi_count = 0;
        Pm_SetFilter(self->in, PM_FILT_ACTIVE | PM_FILT_CLOCK);
    }
    
    self->server_booted = 1;
    
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
    
    PaError err;

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
    if (self->server_started == 0) {
        PyErr_Warn(NULL, "The Server must be started!");
        Py_INCREF(Py_None);
        return Py_None;
    }
    
    PaError err;

    self->server_started = 0;

    err = Pa_StopStream(self->stream);
    portaudio_assert(err, "Pa_StopStream");

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Server_start_rec(Server *self, PyObject *args)
{    
    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->samplingRate;
    self->recinfo.channels = self->nchnls;
    self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_FLOAT;
    
    /* Open the output file. */
    if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {   
        printf ("Not able to open output file %s.\n", self->recpath) ;
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
        stream_tmp = (Stream *)PyList_GET_ITEM(my_server->streams, i);
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

float *
Server_getInputBuffer(Server *self) {
    return (float *)self->input_buffer;
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
Server_getStreams(Server *self)
{
    Py_INCREF(self->streams);
    return self->streams;
}

static PyMethodDef Server_methods[] = {
    {"setInputDevice", (PyCFunction)Server_setInputDevice, METH_O, "Sets audio input device."},
    {"setOutputDevice", (PyCFunction)Server_setOutputDevice, METH_O, "Sets audio output device."},
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
    {"recstart", (PyCFunction)Server_start_rec, METH_NOARGS, "Start automatic output recording."},
    {"recstop", (PyCFunction)Server_stop_rec, METH_NOARGS, "Stop automatic output recording."},
    {"addStream", (PyCFunction)Server_addStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
    {"removeStream", (PyCFunction)Server_removeStream, METH_VARARGS, "Adds an audio stream to the server. \
                                                                This is for internal use and must never be called by the user."},
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
