#include <Python.h>

/* Object initialisation */
static int Midictl_traverse(PyObject *self, visitproc visit, void *arg);
static int Midictl_clear(PyObject *self);
static void Midictl_dealloc(PyObject* self);
static PyObject * Midictl_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Midictl_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject MidictlType;

/* Functions exposed to Python interpreter */
//static PyObject * Midictl_getMidictl(PyObject *self);
static PyObject * Midictl_getServer(PyObject *self);
static PyObject * Midictl_getStream(PyObject *self);
static PyObject * Midictl_play(PyObject *self);
static PyObject * Midictl_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Midictl_stop(PyObject *self);
static PyObject * Midictl_setMul(PyObject *self, PyObject *arg);
static PyObject * Midictl_multiply(PyObject *self, PyObject *arg);
static PyObject * Midictl_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Midictl_setAdd(PyObject *self, PyObject *arg);
static PyObject * Midictl_add(PyObject *self, PyObject *arg);
static PyObject * Midictl_inplace_add(PyObject *self, PyObject *arg);
