#include <Python.h>
#include "pyomodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add 
} Dummy;

extern PyObject * Dummy_initialize(Dummy *self);

#define MAKE_NEW_DUMMY(self, type, rt_error)	\
(self) = (Dummy *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }

