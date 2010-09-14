/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"

/************/
/* Record */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input_list;
    PyObject *input_stream_list;
    int chnls;
    int buffering;
    int count;
    int listlen;
    char *recpath;
    SNDFILE *recfile;
    SF_INFO recinfo;
    float *buffer;
} Record;

static void
Record_process(Record *self) {
    int i, j, chnl, offset, totlen;
    float *in;

    totlen = self->chnls*self->bufsize*self->buffering;
    
    if (self->count == self->buffering) {
        self->count = 0;
        for (i=0; i<totlen; i++) {
            self->buffer[i] = 0.0;
        }
    }    

    offset = self->bufsize * self->chnls * self->count;
    
    for (j=0; j<self->listlen; j++) {
        chnl = j % self->chnls;
        in = Stream_getData((Stream *)PyList_GET_ITEM(self->input_stream_list, j));
        for (i=0; i<self->bufsize; i++) {
            self->buffer[i*self->chnls+chnl+offset] += in[i];
        }
    }
    self->count++;
    
    if (self->count == self->buffering)
        sf_write_float(self->recfile, self->buffer, totlen);
}

static void
Record_setProcMode(Record *self)
{       
    self->proc_func_ptr = Record_process;   
}

static void
Record_compute_next_data_frame(Record *self)
{
    (*self->proc_func_ptr)(self); 
    Stream_setData(self->stream, self->data);
}

static int
Record_traverse(Record *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input_list);
    Py_VISIT(self->input_stream_list);
    return 0;
}

static int 
Record_clear(Record *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input_list);
    Py_CLEAR(self->input_stream_list);
    return 0;
}

static void
Record_dealloc(Record* self)
{
    free(self->data);
    free(self->buffer);
    Record_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Record_deleteStream(Record *self) { DELETE_STREAM };

static PyObject *
Record_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Record *self;
    self = (Record *)type->tp_alloc(type, 0);
    
    self->chnls = 2;
    self->buffering = 4;
    self->count = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Record_compute_next_data_frame);
    self->mode_func_ptr = Record_setProcMode;
    return (PyObject *)self;
}

static int
Record_init(Record *self, PyObject *args, PyObject *kwds)
{
    int i, buflen;
    int format = 0;
    PyObject *input_listtmp;
    
    static char *kwlist[] = {"input", "filename", "chnls", "format", "buffering", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os|iii", kwlist, &input_listtmp, &self->recpath, &self->chnls, &format, &self->buffering))
        return -1; 
    
    Py_XDECREF(self->input_list);
    self->input_list = input_listtmp;
    self->listlen = PyList_Size(self->input_list);
    self->input_stream_list = PyList_New(self->listlen);
    for (i=0; i<self->listlen; i++) {
        PyList_SET_ITEM(self->input_stream_list, i, PyObject_CallMethod(PyList_GET_ITEM(self->input_list, i), "_getStream", NULL));
    }

    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->sr;
    self->recinfo.channels = self->chnls;
    switch (format) {
        case 0:
            self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_FLOAT;
            break;
        case 1:    
            self->recinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            break;
        case 2:
            self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_16;
            break;
        case 3:    
            self->recinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            break;
        case 4:
            self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_24;
            break;
        case 5:    
            self->recinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
            break;
        case 6:
            self->recinfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_32;
            break;
        case 7:    
            self->recinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
            break;
    }
    
    /* Open the output file. */
    if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {   
        printf ("Not able to open output file %s.\n", self->recpath);
    }	

    buflen = self->bufsize * self->chnls * self->buffering;
    self->buffer = (float *)realloc(self->buffer, buflen * sizeof(float));
    for (i=0; i<buflen; i++) {
        self->buffer[i] = 0.;
    }    
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Record_getServer(Record* self) { GET_SERVER };
static PyObject * Record_getStream(Record* self) { GET_STREAM };

static PyObject * Record_play(Record *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Record_stop(Record *self) 
{ 
    sf_close(self->recfile);
    STOP
};

static PyMemberDef Record_members[] = {
{"server", T_OBJECT_EX, offsetof(Record, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Record, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Record, input_list), 0, "Input sound base object list."},
{NULL}  /* Sentinel */
};

static PyMethodDef Record_methods[] = {
{"getServer", (PyCFunction)Record_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Record_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Record_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Record_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Record_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject RecordType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Record_base",                                   /*tp_name*/
sizeof(Record),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Record_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,												/*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Record objects. Records its audio input in a file.",           /* tp_doc */
(traverseproc)Record_traverse,                  /* tp_traverse */
(inquiry)Record_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Record_methods,                                 /* tp_methods */
Record_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Record_init,                          /* tp_init */
0,                                              /* tp_alloc */
Record_new,                                     /* tp_new */
};
