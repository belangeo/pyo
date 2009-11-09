#include <Python.h>

/* Object initialisation */
static int Fader_traverse(PyObject *self, visitproc visit, void *arg);
static int Fader_clear(PyObject *self);
static void Fader_dealloc(PyObject* self);
static PyObject * Fader_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Fader_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject FaderType;

/* Functions exposed to Python interpreter */
static PyObject * Fader_getServer(PyObject *self);
static PyObject * Fader_getStream(PyObject *self);
static PyObject * Fader_play(PyObject *self);
static PyObject * Fader_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Fader_stop(PyObject *self);
static PyObject * Fader_setMul(PyObject *self, PyObject *arg);
static PyObject * Fader_multiply(PyObject *self, PyObject *arg);
static PyObject * Fader_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Fader_setAdd(PyObject *self, PyObject *arg);
static PyObject * Fader_add(PyObject *self, PyObject *arg);
static PyObject * Fader_inplace_add(PyObject *self, PyObject *arg);
