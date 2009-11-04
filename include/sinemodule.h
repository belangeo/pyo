#include <Python.h>

/* Object initialisation */
static int Sine_traverse(PyObject *self, visitproc visit, void *arg);
static int Sine_clear(PyObject *self);
static void Sine_dealloc(PyObject* self);
static PyObject * Sine_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Sine_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject SineType;

/* Functions exposed to Python interpreter */
static PyObject * Sine_getServer(PyObject *self);
static PyObject * Sine_getStream(PyObject *self);
static PyObject * Sine_play(PyObject *self);
static PyObject * Sine_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Sine_stop(PyObject *self);
static PyObject * Sine_setMul(PyObject *self, PyObject *arg);
static PyObject * Sine_multiply(PyObject *self, PyObject *arg);
static PyObject * Sine_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Sine_setAdd(PyObject *self, PyObject *arg);
static PyObject * Sine_add(PyObject *self, PyObject *arg);
static PyObject * Sine_inplace_add(PyObject *self, PyObject *arg);
static PyObject * Sine_setFreq(PyObject *self, PyObject *arg);
