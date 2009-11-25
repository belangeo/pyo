#include "Python.h"
#include <math.h>

extern PyTypeObject SineType;
extern PyTypeObject OscType;
extern PyTypeObject NoiseType;
extern PyTypeObject InputType;
extern PyTypeObject SfPlayerType;
extern PyTypeObject SfPlayType;

extern PyTypeObject FaderType;
extern PyTypeObject BiquadType;
extern PyTypeObject PortType;
extern PyTypeObject DistoType;
extern PyTypeObject DelayType;

extern PyTypeObject MidictlType;
extern PyTypeObject MidiNoteType;
extern PyTypeObject NoteinType;

extern PyTypeObject DummyType;
extern PyTypeObject MixType;
extern PyTypeObject InputFaderType;

extern PyTypeObject HarmTableType;
extern PyTypeObject HannTableType;
extern PyTypeObject SndTableType;
extern PyTypeObject NewTableType;
extern PyTypeObject TableRecType;

extern PyTypeObject OscSendType;
extern PyTypeObject OscReceiveType;
extern PyTypeObject OscReceiverType;

/* Constants */
#define PI M_PI
#define TWOPI (2 * M_PI)

/* object headers */
#define pyo_audio_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    Stream *stream; \
    void (*mode_func_ptr)(); \
    void (*proc_func_ptr)(); \
    void (*muladd_func_ptr)(); \
    PyObject *mul; \
    Stream *mul_stream; \
    PyObject *add; \
    Stream *add_stream; \
    int bufsize; \
    int nchnls; \
    float sr; \
    float *data; 

#define pyo_table_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    TableStream *tablestream; \
    int size; \
    float *data;

/* VISIT & CLEAR */
#define pyo_VISIT \
    Py_VISIT(self->stream); \
    Py_VISIT(self->server); \
    Py_VISIT(self->mul); \
    Py_VISIT(self->mul_stream); \
    Py_VISIT(self->add); \
    Py_VISIT(self->add_stream);    

#define pyo_CLEAR \
    Py_CLEAR(self->stream); \
    Py_CLEAR(self->server); \
    Py_CLEAR(self->mul); \
    Py_CLEAR(self->mul_stream); \
    Py_CLEAR(self->add); \
    Py_CLEAR(self->add_stream);    

#define DELETE_STREAM \
    Server_removeStream((Server *)self->server, Stream_getStreamId(self->stream)); \
    Py_INCREF(Py_None); \
    return Py_None;

/* Init Server & Stream */
#define INIT_OBJECT_COMMON \
    self->server = PyServer_get_server(); \
    self->mul = PyFloat_FromDouble(1); \
    self->add = PyFloat_FromDouble(0); \
    self->bufsize = PyInt_AsLong(PyObject_CallMethod(self->server, "getBufferSize", NULL)); \
    self->sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    self->nchnls = PyInt_AsLong(PyObject_CallMethod(self->server, "getNchnls", NULL)); \
    self->data = (float *)realloc(self->data, (self->bufsize) * sizeof(float)); \
    MAKE_NEW_STREAM(self->stream, &StreamType, NULL); \
    Stream_setStreamObject(self->stream, (PyObject *)self); \
    Stream_setStreamId(self->stream, Stream_getNewStreamId());


/* GETS & SETS */
#define GET_SERVER \
    if (self->server == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No server found!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->server); \
    return self->server;

#define GET_STREAM \
    if (self->stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No stream found!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->stream); \
    return (PyObject *)self->stream;

#define GET_TABLE_STREAM \
    if (self->tablestream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No table stream found!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->tablestream); \
    return (PyObject *)self->tablestream; \

#define SET_MUL \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->mul); \
    if (isNumber == 1) { \
        self->mul = PyNumber_Float(tmp); \
        self->modebuffer[0] = 0; \
    } \
    else { \
        self->mul = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_ADD \
    PyObject *tmp, *streamtmp; \
\
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
\
    int isNumber = PyNumber_Check(arg); \
\
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Float(tmp); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 1; \
    } \
\
    (*self->mode_func_ptr)(self); \
\
    Py_INCREF(Py_None); \
    return Py_None; 

/* Multiply, Add, inplace_multiply & inplace_add */
#define MULTIPLY \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setMul", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_MULTIPLY \
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define ADD \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setAdd", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_ADD \
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

/* PLAY, OUT, STOP */
#define PLAY \
    Stream_setStreamActive(self->stream, 1); \
    Stream_setStreamToDac(self->stream, 0); \
    Py_INCREF(self); \
    return (PyObject *)self;

# define OUT \
    int chnltmp = 0; \
 \
    static char *kwlist[] = {"chnl", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &chnltmp)) \
        return PyInt_FromLong(-1); \
 \
    Stream_setStreamChnl(self->stream, chnltmp % self->nchnls); \
    Stream_setStreamToDac(self->stream, 1); \
    Stream_setStreamActive(self->stream, 1); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define STOP \
    int i; \
    Stream_setStreamActive(self->stream, 0); \
    Stream_setStreamChnl(self->stream, 0); \
    Stream_setStreamToDac(self->stream, 0); \
    for (i=0; i<self->bufsize; i++) { \
        self->data[i] = 0; \
    } \
    Py_INCREF(Py_None); \
    return Py_None;    

/* Post processing (mul & add) macros */
#define POST_PROCESSING_II \
    float mul, add, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_AI \
    float add, old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IA \
    float mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old + add[i]; \
        self->data[i] = val; \
    } 

#define POST_PROCESSING_AA \
    float old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add[i]; \
        self->data[i] = val; \
    }

