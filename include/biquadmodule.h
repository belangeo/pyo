#include <Python.h>

/* Object initialisation */
static int Biquad_traverse(PyObject *self, visitproc visit, void *arg);
static int Biquad_clear(PyObject *self);
static void Biquad_dealloc(PyObject* self);
static PyObject * Biquad_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Biquad_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject BiquadType;

/* Functions exposed to Python interpreter */
//static PyObject * Biquad_getInput(PyObject *self);
static PyObject * Biquad_getServer(PyObject *self);
static PyObject * Biquad_getStream(PyObject *self);
static PyObject * Biquad_play(PyObject *self);
static PyObject * Biquad_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Biquad_stop(PyObject *self);
static PyObject * Biquad_setMul(PyObject *self, PyObject *arg);
static PyObject * Biquad_multiply(PyObject *self, PyObject *arg);
static PyObject * Biquad_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Biquad_setAdd(PyObject *self, PyObject *arg);
static PyObject * Biquad_add(PyObject *self, PyObject *arg);
static PyObject * Biquad_inplace_add(PyObject *self, PyObject *arg);
static PyObject * Biquad_setFreq(PyObject *self, PyObject *arg);
static PyObject * Biquad_setQ(PyObject *self, PyObject *arg);
static PyObject * Biquad_setType(PyObject *self, PyObject *arg);
