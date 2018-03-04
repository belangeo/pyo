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
#include "py2to3.h"
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "portmidi.h"
#include "porttime.h"

typedef struct {
    PyObject_HEAD
    PyObject *midicallable;
    PmStream *midiin[64];
    PyObject *mididev;
    int ids[64];
    int midicount;
    int active;
    int reportdevice;
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
                if (server->reportdevice) {
                    tup = PyTuple_New(4);
                    PyTuple_SetItem(tup, 0, PyInt_FromLong(status));
                    PyTuple_SetItem(tup, 1, PyInt_FromLong(data1));
                    PyTuple_SetItem(tup, 2, PyInt_FromLong(data2));
                    PyTuple_SetItem(tup, 3, PyInt_FromLong(server->ids[i]));
                    PyObject_Call((PyObject *)server->midicallable, tup, NULL);
                }
                else {
                    tup = PyTuple_New(3);
                    PyTuple_SetItem(tup, 0, PyInt_FromLong(status));
                    PyTuple_SetItem(tup, 1, PyInt_FromLong(data1));
                    PyTuple_SetItem(tup, 2, PyInt_FromLong(data2));
                    PyObject_Call((PyObject *)server->midicallable, tup, NULL);
                }
            }
        }
    } while (result);

    PyGILState_Release(s);
}

static int
MidiListener_traverse(MidiListener *self, visitproc visit, void *arg)
{
    Py_VISIT(self->midicallable);
    Py_VISIT(self->mididev);
    return 0;
}

static int
MidiListener_clear(MidiListener *self)
{
    Py_CLEAR(self->midicallable);
    Py_CLEAR(self->mididev);
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
    PyObject *midicalltmp=NULL, *mididevtmp=NULL;
    MidiListener *self;

    self = (MidiListener *)type->tp_alloc(type, 0);

    self->active = self->midicount = self->reportdevice = 0;

    static char *kwlist[] = {"midicallable", "mididevice", "reportdevice", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi", kwlist, &midicalltmp, &mididevtmp, &self->reportdevice))
        Py_RETURN_NONE;

    if (midicalltmp) {
        PyObject_CallMethod((PyObject *)self, "setMidiFunction", "O", midicalltmp);
    }

    if (mididevtmp) {
        Py_INCREF(mididevtmp);
        Py_XDECREF(self->mididev);
        self->mididev = mididevtmp;
    }

    return (PyObject *)self;
}

static PyObject * MidiListener_play(MidiListener *self) {
    int i, num_devices, lsize, mididev;
    PmError pmerr;

    Py_BEGIN_ALLOW_THREADS
    /* always start the timer before you start midi */
    Pt_Start(1, &process_midi, (void *)self);
    
    pmerr = Pm_Initialize();
    Py_END_ALLOW_THREADS

    if (pmerr) {
        PySys_WriteStdout("Portmidi warning: could not initialize Portmidi: %s\n", Pm_GetErrorText(pmerr));
        if (Pt_Started()) {
            Pt_Stop();
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    lsize = PyList_Size(self->mididev);

    num_devices = Pm_CountDevices();
    if (num_devices > 0) {
        if (lsize == 1) {
            mididev = PyLong_AsLong(PyList_GetItem(self->mididev, 0));
            if (mididev < num_devices) {
                if (mididev == -1)
                    mididev = Pm_GetDefaultInputDeviceID();
                const PmDeviceInfo *info = Pm_GetDeviceInfo(mididev);
                if (info != NULL) {
                    if (info->input) {

                        Py_BEGIN_ALLOW_THREADS
                        pmerr = Pm_OpenInput(&self->midiin[0], mididev, NULL, 100, NULL, NULL);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            PySys_WriteStdout("Portmidi warning: could not open midi input %d (%s): %s\n",
                                              mididev, info->name, Pm_GetErrorText(pmerr));
                        }
                        else {
                            self->midicount = 1;
                            self->ids[0] = mididev;
                        }
                    }
                }
            }
            else if (mididev >= num_devices) {
                self->midicount = 0;
                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                    if (info != NULL) {
                        if (info->input) {

                            Py_BEGIN_ALLOW_THREADS
                            pmerr = Pm_OpenInput(&self->midiin[self->midicount], i, NULL, 100, NULL, NULL);
                            Py_END_ALLOW_THREADS

                            if (pmerr) {
                                PySys_WriteStdout("Portmidi warning: could not open midi input %d (%s): %s\n",
                                                  i, info->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                self->ids[self->midicount] = i;
                                self->midicount++;
                            }
                        }
                    }
                }
            }
        }
        else {
            self->midicount = 0;
            for (i=0; i<num_devices; i++) {
                if (PySequence_Contains(self->mididev, PyLong_FromLong(i)) == 0)
                    continue;
                const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                if (info != NULL) {
                    if (info->input) {

                        Py_BEGIN_ALLOW_THREADS
                        pmerr = Pm_OpenInput(&self->midiin[self->midicount], i, NULL, 100, NULL, NULL);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            PySys_WriteStdout("Portmidi warning: could not open midi input %d (%s): %s\n",
                                              i, info->name, Pm_GetErrorText(pmerr));
                        }
                        else {
                            self->ids[self->midicount] = i;
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
    else {
        if (Pt_Started()) {
            Pt_Stop();
        }
    }

	Py_INCREF(Py_None);
	return Py_None;
};

static PyObject * MidiListener_stop(MidiListener *self) { 
    int i;

    Py_BEGIN_ALLOW_THREADS
    if (Pt_Started()) {
        Pt_Stop();
    }
    for (i=0; i<self->midicount; i++) {
        Pm_Close(self->midiin[i]);
    }
    Pm_Terminate();
    Py_END_ALLOW_THREADS

    self->active = 0;
	Py_INCREF(Py_None);
	return Py_None;
};

static PyObject *
MidiListener_setMidiFunction(MidiListener *self, PyObject *arg)
{
	PyObject *tmp;

	if (! PyCallable_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "Pyo error: MidiListener callable attribute must be a valid Python function.");
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

PyObject *
MidiListener_getDeviceInfos(MidiListener *self) {
    int i;
    PyObject *str;
    PyObject *lst = PyList_New(0);
    for (i = 0; i < self->midicount; i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(self->ids[i]);
        str = PyUnicode_FromFormat("id: %d, name: %s, interface: %s\n", self->ids[i], info->name, info->interf);
        PyList_Append(lst, str);
    }
    return lst;
}

static PyMemberDef MidiListener_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiListener_methods[] = {
    {"play", (PyCFunction)MidiListener_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiListener_stop, METH_NOARGS, "Stops computing."},
    {"setMidiFunction", (PyCFunction)MidiListener_setMidiFunction, METH_O, "Sets the function to be called."},
    {"getDeviceInfos", (PyCFunction)MidiListener_getDeviceInfos, METH_NOARGS, "Returns a list of device infos."},
    {NULL}  /* Sentinel */
};

PyTypeObject MidiListenerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MidiListener_base",         /*tp_name*/
    sizeof(MidiListener),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MidiListener_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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

typedef struct {
    PyObject_HEAD
    PmStream *midiout[64];
    PyObject *mididev;
    int ids[64];
    int midicount;
    int active;
} MidiDispatcher;

static int
MidiDispatcher_traverse(MidiDispatcher *self, visitproc visit, void *arg) {
    return 0;
}

static int
MidiDispatcher_clear(MidiDispatcher *self) {
    return 0;
}

static void
MidiDispatcher_dealloc(MidiDispatcher* self) {
    if (self->active == 1)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    MidiDispatcher_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MidiDispatcher_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *mididevtmp=NULL;
    MidiDispatcher *self;

    self = (MidiDispatcher *)type->tp_alloc(type, 0);

    self->active = self->midicount = 0;

    static char *kwlist[] = {"mididevice", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &mididevtmp))
        Py_RETURN_NONE;

    if (mididevtmp) {
        Py_INCREF(mididevtmp);
        Py_XDECREF(self->mididev);
        self->mididev = mididevtmp;
    }

    return (PyObject *)self;
}

static PyObject * MidiDispatcher_play(MidiDispatcher *self) {
    int i, num_devices, lsize, mididev;
    PmError pmerr;

    Py_BEGIN_ALLOW_THREADS
    /* always start the timer before you start midi */
    Pt_Start(1, 0, 0);
    
    pmerr = Pm_Initialize();
    Py_END_ALLOW_THREADS

    if (pmerr) {
        PySys_WriteStdout("Portmidi warning: could not initialize Portmidi: %s\n", Pm_GetErrorText(pmerr));
        if (Pt_Started()) {
            Pt_Stop();
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    lsize = PyList_Size(self->mididev);

    num_devices = Pm_CountDevices();
    if (num_devices > 0) {
        if (lsize == 1) {
            mididev = PyLong_AsLong(PyList_GetItem(self->mididev, 0));
            if (mididev < num_devices) {
                if (mididev == -1)
                    mididev = Pm_GetDefaultOutputDeviceID();
                const PmDeviceInfo *info = Pm_GetDeviceInfo(mididev);
                if (info != NULL) {
                    if (info->output) {

                        Py_BEGIN_ALLOW_THREADS
                        pmerr = Pm_OpenOutput(&self->midiout[0], mididev, NULL, 100, NULL, NULL, 1);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            PySys_WriteStdout("Portmidi warning: could not open midi output %d (%s): %s\n",
                                              mididev, info->name, Pm_GetErrorText(pmerr));
                        }
                        else {
                            self->midicount = 1;
                            self->ids[0] = mididev;
                        }
                    }
                }
            }
            else if (mididev >= num_devices) {
                self->midicount = 0;
                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                    if (info != NULL) {
                        if (info->output) {

                            Py_BEGIN_ALLOW_THREADS
                            pmerr = Pm_OpenOutput(&self->midiout[self->midicount], i, NULL, 100, NULL, NULL, 1);
                            Py_END_ALLOW_THREADS

                            if (pmerr) {
                                PySys_WriteStdout("Portmidi warning: could not open midi output %d (%s): %s\n",
                                                  i, info->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                self->ids[self->midicount] = i;
                                self->midicount++;
                            }
                        }
                    }
                }
            }
        }
        else {
            self->midicount = 0;
            for (i=0; i<num_devices; i++) {
                if (PySequence_Contains(self->mididev, PyLong_FromLong(i)) == 0)
                    continue;
                const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                if (info != NULL) {
                    if (info->output) {

                        Py_BEGIN_ALLOW_THREADS
                        pmerr = Pm_OpenOutput(&self->midiout[self->midicount], i, NULL, 100, NULL, NULL, 1);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            PySys_WriteStdout("Portmidi warning: could not open midi output %d (%s): %s\n",
                                              i, info->name, Pm_GetErrorText(pmerr));
                        }
                        else {
                            self->ids[self->midicount] = i;
                            self->midicount++;
                        }
                    }
                }
            }
        }
    }

    if (self->midicount > 0)
        self->active = 1;
    else {
        if (Pt_Started()) {
            Pt_Stop();
        }
    }

	Py_INCREF(Py_None);
	return Py_None;
};

static PyObject * MidiDispatcher_stop(MidiDispatcher *self) { 
    int i;

    Py_BEGIN_ALLOW_THREADS
    if (Pt_Started()) {
        Pt_Stop();
    }
    for (i=0; i<self->midicount; i++) {
        Pm_Close(self->midiout[i]);
    }
    Pm_Terminate();    
    Py_END_ALLOW_THREADS

    self->active = 0;
	Py_INCREF(Py_None);
	return Py_None;
};

PyObject *
MidiDispatcher_send(MidiDispatcher *self, PyObject *args)
{
    int i, status, data1, data2, device;
    long curtime;
    PmTimestamp timestamp;
    PmEvent buffer[1];

    if (! PyArg_ParseTuple(args, "iiili", &status, &data1, &data2, &timestamp, &device))
        return PyInt_FromLong(-1);

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    buffer[0].message = Pm_Message(status, data1, data2);
    if (device == -1 && self->midicount > 1) {
        for (i=0; i<self->midicount; i++) {
            Pm_Write(self->midiout[i], buffer, 1);
        }
    }
    else if (self->midicount == 1)
        Pm_Write(self->midiout[0], buffer, 1);
    else {
        for (i=0; i<self->midicount; i++) {
            if (self->ids[i] == device) {
                device = i;
                break;
            }
        }
        if (device < 0 || device >= self->midicount)
            device = 0;
        Pm_Write(self->midiout[device], buffer, 1);
    }

    Py_RETURN_NONE;
}

PyObject *
MidiDispatcher_sendx(MidiDispatcher *self, PyObject *args)
{
    unsigned char *msg;
    int i, size, device;
    long curtime;
    PmTimestamp timestamp;

    if (! PyArg_ParseTuple(args, "s#li", &msg, &size, &timestamp, &device))
        return PyInt_FromLong(-1);

    curtime = Pt_Time();
    if (device == -1 && self->midicount > 1) {
        for (i=0; i<self->midicount; i++) {
            Pm_WriteSysEx(self->midiout[i], curtime + timestamp, msg);
        }
    }
    else if (self->midicount == 1)
        Pm_WriteSysEx(self->midiout[0], curtime + timestamp, msg);
    else {
        for (i=0; i<self->midicount; i++) {
            if (self->ids[i] == device) {
                device = i;
                break;
            }
        }
        if (device < 0 || device >= self->midicount)
            device = 0;
        Pm_WriteSysEx(self->midiout[device], curtime + timestamp, msg);
    }

    Py_RETURN_NONE;
}

PyObject *
MidiDispatcher_getDeviceInfos(MidiDispatcher *self) {
    int i;
    PyObject *str;
    PyObject *lst = PyList_New(0);
    for (i = 0; i < self->midicount; i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(self->ids[i]);
        str = PyUnicode_FromFormat("id: %d, name: %s, interface: %s\n", self->ids[i], info->name, info->interf);
        PyList_Append(lst, str);
    }
    return lst;
}

static PyMemberDef MidiDispatcher_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef MidiDispatcher_methods[] = {
    {"play", (PyCFunction)MidiDispatcher_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MidiDispatcher_stop, METH_NOARGS, "Stops computing."},
    {"getDeviceInfos", (PyCFunction)MidiDispatcher_getDeviceInfos, METH_NOARGS, "Returns a list of device infos."},
    {"send", (PyCFunction)MidiDispatcher_send, METH_VARARGS, "Send a raw midi event."},
    {"sendx", (PyCFunction)MidiDispatcher_sendx, METH_VARARGS, "Send a sysex midi event."},
    {NULL}  /* Sentinel */
};

PyTypeObject MidiDispatcherType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MidiDispatcher_base",         /*tp_name*/
    sizeof(MidiDispatcher),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MidiDispatcher_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_as_async (tp_compare in Python 2)*/
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
    "MidiDispatcher objects. Calls a function with midi data as arguments.",           /* tp_doc */
    (traverseproc)MidiDispatcher_traverse,   /* tp_traverse */
    (inquiry)MidiDispatcher_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MidiDispatcher_methods,             /* tp_methods */
    MidiDispatcher_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MidiDispatcher_new,                 /* tp_new */
};
