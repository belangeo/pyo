#include <Python.h>
#include "pyomodule.h"
//#include "streammodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Dummy;

#define MAKE_NEW_DUMMY(self, type, rt_error)	\
(self) = (Dummy *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }

/* Object initialisation */
static int Dummy_traverse(Dummy *self, visitproc visit, void *arg);
static int Dummy_clear(Dummy *self);
static void Dummy_dealloc(Dummy* self);
extern PyObject * Dummy_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
extern int Dummy_init(Dummy *self, PyObject *args, PyObject *kwds);
extern PyObject * Dummy_initialize(Dummy * self);
extern PyTypeObject DummyType;

/* Functions exposed to Python interpreter */
//static PyObject * Dummy_getInput(PyObject *self);
static PyObject * Dummy_getServer(Dummy *self);
static PyObject * Dummy_getStream(Dummy *self);
static PyObject * Dummy_play(Dummy *self);
static PyObject * Dummy_out(Dummy *self, PyObject *args, PyObject *kwds);
static PyObject * Dummy_stop(Dummy *self);
static PyObject * Dummy_setMul(Dummy *self, PyObject *arg);
static PyObject * Dummy_multiply(Dummy *self, PyObject *arg);
static PyObject * Dummy_inplace_multiply(Dummy *self, PyObject *arg);
static PyObject * Dummy_setAdd(Dummy *self, PyObject *arg);
static PyObject * Dummy_add(Dummy *self, PyObject *arg);
static PyObject * Dummy_inplace_add(Dummy *self, PyObject *arg);
static PyObject * Dummy_setInput(Dummy *self, PyObject *arg);
