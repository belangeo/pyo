/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
 *                                                                        *
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *
 *                                                                        *
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU Lesser General Public       *
 * License along with pyo.  If not, see <http://www.gnu.org/licenses/>.   *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "portmidi.h"
#include "porttime.h"

typedef struct {
    PyObject_HEAD
    PyObject *midicallable;
    PmStream *midiin[64];
    int mididev;
    int midicount;
    int active;
} MidiListener;

void process_midi(PtTimestamp timestamp, void *userData)
{
    PmError result;
    PmEvent buffer; /* just one message at a time */
    int i, status, data1, data2;
    PyObject *tup = NULL;
    MidiListener *server = (MidiListener *)userData;

    if (server->active == 0) return;

    PyGILState_STATE s = PyGILState_Ensure();
    do {
        for (i=0; i<server->midicount; i++) {
            result = Pm_Poll(server->midiin[i]);
            if (result) {
                if (Pm_Read(server->midiin[i], &buffer, 1) == pmBufferOverflow) 
                    continue;
                status = Pm_MessageStatus(buffer.message);
                data1 = Pm_MessageData1(buffer.message);
                data2 = Pm_MessageData2(buffer.message);
                tup = PyTuple_New(3);
                PyTuple_SetItem(tup, 0, PyInt_FromLong(status));
                PyTuple_SetItem(tup, 1, PyInt_FromLong(data1));
                PyTuple_SetItem(tup, 2, PyInt_FromLong(data2));
                PyObject_Call((PyObject *)server->midicallable, tup, NULL);
            }
        }
    } while (result);

    PyGILState_Release(s);
    Py_XDECREF(tup);
}

static int
MidiListener_traverse(MidiListener *self, visitproc visit, void *arg)
{
    Py_VISIT(self->midicallable);
    return 0;
}

static int
MidiListener_clear(MidiListener *self)
{
    Py_CLEAR(self->midicallable);
    return 0;
}

static void
MidiListener_dealloc(MidiListener* self)
{
    if (self->active == 1)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    MidiListener_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiListener_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *midicalltmp=NULL;
    MidiListener *self;

    self = (MidiListener *)type->tp_alloc(type, 0);

    self->active = self->midicount = 0;
    self->mididev = -1;

    static char *kwlist[] = {"midicallable", "mididevice", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi", kwlist, &midicalltmp, &self->mididev))
        Py_RETURN_NONE;

    if (midicalltmp) {
        PyObject_CallMethod((PyObject *)self, "setMidiFunction", "O", midicalltmp);
    }

    return (PyObject *)self;
}

static PyObject * MidiListener_play(MidiListener *self) {
    int i, num_devices;
    PmError pmerr;

    /* always start the timer before you start midi */
    Pt_Start(1, &process_midi, (void *)self);
    
    pmerr = Pm_Initialize();
    if (pmerr) {
        PySys_WriteStdout("Portmidi warning: could not initialize Portmidi: %s\n", Pm_GetErrorText(pmerr));
    }

    num_devices = Pm_CountDevices();
    if (num_devices > 0) {
        if (self->mididev < num_devices) {
            if (self->mididev == -1)
                self->mididev = Pm_GetDefaultInputDeviceID();
            const PmDeviceInfo *info = Pm_GetDeviceInfo(self->mididev);
            if (info != NULL) {
                if (info->input) {
                    pmerr = Pm_OpenInput(&self->midiin[0], self->mididev, NULL, 100, NULL, NULL);
                    if (pmerr) {
                        PySys_WriteStdout("Portmidi warning: could not open midi input %d (%s): %s\n",
                             self->mididev, info->name, Pm_GetErrorText(pmerr));
                    }
                    else {
                        self->midicount = 1;
                    }
                }
            }
        }
        else if (self->mididev >= num_devices) {
            self->midicount = 0;
            for (i=0; i<num_devices; i++) {
                const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                if (info != NULL) {
                    if (info->input) {
                        pmerr = Pm_OpenInput(&self->midiin[self->midicount], i, NULL, 100, NULL, NULL);
                        if (pmerr) {
                            PySys_WriteStdout("Portmidi warning: could not open midi input %d (%s): %s\n",
                                    i, info->name, Pm_GetErrorText(pmerr));
                        }
                        else {
                            self->midicount++;
                        }
                    }
                }
            }
        }
    }

    for (i=0; i<self->midicount; i++) {
        Pm_SetFilter(self->midiin[i], PM_FILT_ACTIVE | PM_FILT_CLOCK);
    }

    if (self->midicount > 0)
        self->active = 1;

	Py_INCREF(Py_None);
	return Py_None;
};

static PyObject * MidiListener_stop(MidiListener *self) { 
    int i;
    Pt_Stop();
    for (i=0; i<self->midicount; i++) {
        Pm_Close(self->midiin[i]);
    }
    Pm_Terminate();    
    self->active = 0;
	Py_INCREF(Py_None);
	return Py_None;
};

static PyObject *
MidiListener_setMidiFunction(MidiListener *self, PyObject *arg)
{
	PyObject *tmp;

	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The callable attribute must be a valid Python function.");
		Py_INCREF(Py_None);
		return Py_None;
	}

    tmp = arg;
    Py_XDECREF(self->midicallable);
    Py_INCREF(tmp);
    self->midicallable = tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef MidiListener_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiListener_methods[] = {
    {"play", (PyCFunction)MidiListener_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiListener_stop, METH_NOARGS, "Stops computing."},
    {"setMidiFunction", (PyCFunction)MidiListener_setMidiFunction, METH_O, "Sets the function to be called."},
    {NULL}  /* Sentinel */
};

PyTypeObject MidiListenerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    0,                         /*ob_size*/
    "_pyo.MidiListener_base",         /*tp_name*/
    sizeof(MidiListener),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MidiListener_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "MidiListener objects. Calls a function with midi data as arguments.",           /* tp_doc */
    (traverseproc)MidiListener_traverse,   /* tp_traverse */
    (inquiry)MidiListener_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MidiListener_methods,             /* tp_methods */
    MidiListener_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MidiListener_new,                 /* tp_new */
};
