#include <Python.h>

/* Object initialisation */
static int Osc_traverse(PyObject *self, visitproc visit, void *arg);
static int Osc_clear(PyObject *self);
static void Osc_dealloc(PyObject* self);
static PyObject * Osc_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Osc_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject OscType;

/* Functions exposed to Python interpreter */
static PyObject * Osc_getTable(PyObject *self);
static PyObject * Osc_getServer(PyObject *self);
static PyObject * Osc_getStream(PyObject *self);
static PyObject * Osc_play(PyObject *self);
static PyObject * Osc_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Osc_stop(PyObject *self);
static PyObject * Osc_setMul(PyObject *self, PyObject *arg);
static PyObject * Osc_multiply(PyObject *self, PyObject *arg);
static PyObject * Osc_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Osc_setAdd(PyObject *self, PyObject *arg);
static PyObject * Osc_add(PyObject *self, PyObject *arg);
static PyObject * Osc_inplace_add(PyObject *self, PyObject *arg);
static PyObject * Osc_setFreq(PyObject *self, PyObject *arg);
