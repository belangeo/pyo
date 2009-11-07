#include <Python.h>

/* Object initialisation */
static int Input_traverse(PyObject *self, visitproc visit, void *arg);
static int Input_clear(PyObject *self);
static void Input_dealloc(PyObject* self);
static PyObject * Input_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Input_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject InputType;

/* Functions exposed to Python interpreter */
//static PyObject * Input_getInput(PyObject *self);
static PyObject * Input_getServer(PyObject *self);
static PyObject * Input_getStream(PyObject *self);
static PyObject * Input_play(PyObject *self);
static PyObject * Input_out(PyObject *self, PyObject *args, PyObject *kwds);
static PyObject * Input_stop(PyObject *self);
static PyObject * Input_setMul(PyObject *self, PyObject *arg);
static PyObject * Input_multiply(PyObject *self, PyObject *arg);
static PyObject * Input_inplace_multiply(PyObject *self, PyObject *arg);
static PyObject * Input_setAdd(PyObject *self, PyObject *arg);
static PyObject * Input_add(PyObject *self, PyObject *arg);
static PyObject * Input_inplace_add(PyObject *self, PyObject *arg);
