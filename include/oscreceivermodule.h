#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "lo/lo.h"

typedef struct {
    pyo_audio_HEAD
    lo_server osc_server;
    int port;
    PyObject *dict;
    PyObject *address_path;
} OscReceiver;

extern float OscReceiver_getValue(OscReceiver *self, PyObject *path);
