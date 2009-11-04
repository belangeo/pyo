#include <Python.h>

/* Object initialisation */
static int Disto_traverse(PyObject *self, visitproc visit, void *arg);
static int Disto_clear(PyObject *self);
static void Disto_dealloc(PyObject* self);
static PyObject * Disto_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Disto_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject DistoType;

/* Functions exposed to Python interpreter */
//static PyObject * Disto_getInput(PyObject *self);
static PyObject * Disto_getServer(PyObject *self);
static PyObject * Disto_getStream(PyObject *self);
static PyObject * Disto_play(PyObject *self);
static PyObject * Disto_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Disto_stop(PyObject *self);
static PyObject * Disto_setMul(PyObject *self, PyObject *arg);
static PyObject * Disto_multiply(PyObject *self, PyObject *arg);
static PyObject * Disto_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Disto_setAdd(PyObject *self, PyObject *arg);
static PyObject * Disto_add(PyObject *self, PyObject *arg);
static PyObject * Disto_inplace_add(PyObject *self, PyObject *arg);
static PyObject * Disto_setDrive(PyObject *self, PyObject *arg);
static PyObject * Disto_setSlope(PyObject *self, PyObject *arg);
