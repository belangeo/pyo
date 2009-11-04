#include <Python.h>

/* Object initialisation */
static int Noise_traverse(PyObject *self, visitproc visit, void *arg);
static int Noise_clear(PyObject *self);
static void Noise_dealloc(PyObject* self);
static PyObject * Noise_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Noise_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject NoiseType;

/* Functions exposed to Python interpreter */
static PyObject * Noise_getServer(PyObject *self);
static PyObject * Noise_getStream(PyObject *self);
static PyObject * Noise_play(PyObject *self);
static PyObject * Noise_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Noise_stop(PyObject *self);
static PyObject * Noise_setMul(PyObject *self, PyObject *arg);
static PyObject * Noise_multiply(PyObject *self, PyObject *arg);
static PyObject * Noise_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Noise_setAdd(PyObject *self, PyObject *arg);
static PyObject * Noise_add(PyObject *self, PyObject *arg);
static PyObject * Noise_inplace_add(PyObject *self, PyObject *arg);
