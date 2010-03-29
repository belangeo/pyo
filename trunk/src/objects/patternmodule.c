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

typedef struct {
    pyo_audio_HEAD
    PyObject *callable;
    PyObject *time;
    Stream *time_stream;
    int modebuffer[1];
    float sampleToSec;
    float currentTime;
    int init;
} Pattern;

static void
Pattern_generate_i(Pattern *self) {
    float tm;
    int i, flag;
    
    flag = 0;
    tm = PyFloat_AS_DOUBLE(self->time);

    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm) {
            flag = 1;
            self->currentTime = 0.;
        }    

        self->currentTime += self->sampleToSec;
    }
    if (flag == 1 || self->init == 1) {
        PyObject_CallFunctionObjArgs(self->callable, NULL);
        self->init = 0;
    }
}

static void
Pattern_generate_a(Pattern *self) {
    int i, flag;
    
    float *tm = Stream_getData((Stream *)self->time_stream);
    
    flag = 0;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime >= tm[i]) {
            flag = 1;
            self->currentTime = 0.;
        }    
        
        self->currentTime += self->sampleToSec;
    }
    if (flag == 1 || self->init == 1) {
        PyObject_CallFunctionObjArgs(self->callable, NULL);
        self->init = 0;
    }    
}


static void
Pattern_setProcMode(Pattern *self)
{
    int procmode = self->modebuffer[0];
    switch (procmode) {
        case 0:        
            self->proc_func_ptr = Pattern_generate_i;
            break;
        case 1:    
            self->proc_func_ptr = Pattern_generate_a;
            break;
    }
}

static void
Pattern_compute_next_data_frame(Pattern *self)
{
    (*self->proc_func_ptr)(self);     
}

static int
Pattern_traverse(Pattern *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->callable);
    Py_VISIT(self->time);    
    Py_VISIT(self->time_stream);    
    return 0;
}

static int 
Pattern_clear(Pattern *self)
{
    pyo_CLEAR
    Py_CLEAR(self->callable);
    Py_CLEAR(self->time);    
    Py_CLEAR(self->time_stream);
    return 0;
}

static void
Pattern_dealloc(Pattern* self)
{
    free(self->data);
    Pattern_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Pattern_deleteStream(Pattern *self) { DELETE_STREAM };

static PyObject *
Pattern_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Pattern *self;
    self = (Pattern *)type->tp_alloc(type, 0);
    
    self->time = PyFloat_FromDouble(1.);
	self->modebuffer[0] = 0;
    self->init = 1;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Pattern_compute_next_data_frame);
    self->mode_func_ptr = Pattern_setProcMode;

    Stream_setStreamActive(self->stream, 0);
    
    self->sampleToSec = 1. / self->sr;
    self->currentTime = 0.;
    
    return (PyObject *)self;
}

static int
Pattern_init(Pattern *self, PyObject *args, PyObject *kwds)
{
    PyObject *timetmp=NULL, *calltmp=NULL;
    
    static char *kwlist[] = {"callable", "time", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &calltmp, &timetmp))
        return -1; 
 
    if (! PyFunction_Check(calltmp))
        return -1;
    
    Py_XDECREF(self->callable);
    self->callable = calltmp;
    
    if (timetmp) {
        PyObject_CallMethod((PyObject *)self, "setTime", "O", timetmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    Pattern_compute_next_data_frame((Pattern *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Pattern_getServer(Pattern* self) { GET_SERVER };
static PyObject * Pattern_getStream(Pattern* self) { GET_STREAM };

static PyObject * 
Pattern_play(Pattern *self) 
{ 
    self->init = 1;
    PLAY 
};

static PyObject * Pattern_stop(Pattern *self) { STOP };

static PyObject *
Pattern_setTime(Pattern *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->time);
	if (isNumber == 1) {
		self->time = PyNumber_Float(tmp);
        self->modebuffer[0] = 0;
	}
	else {
		self->time = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->time, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->time_stream);
        self->time_stream = (Stream *)streamtmp;
		self->modebuffer[0] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Pattern_members[] = {
{"server", T_OBJECT_EX, offsetof(Pattern, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Pattern, stream), 0, "Stream object."},
{"time", T_OBJECT_EX, offsetof(Pattern, time), 0, "Pattern time factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Pattern_methods[] = {
{"getServer", (PyCFunction)Pattern_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Pattern_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Pattern_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Pattern_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Pattern_stop, METH_NOARGS, "Stops computing."},
{"setTime", (PyCFunction)Pattern_setTime, METH_O, "Sets time factor."},
{NULL}  /* Sentinel */
};

PyTypeObject PatternType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Pattern_base",         /*tp_name*/
sizeof(Pattern),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Pattern_dealloc, /*tp_dealloc*/
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
"Pattern objects. Create a metronome.",           /* tp_doc */
(traverseproc)Pattern_traverse,   /* tp_traverse */
(inquiry)Pattern_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Pattern_methods,             /* tp_methods */
Pattern_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Pattern_init,      /* tp_init */
0,                         /* tp_alloc */
Pattern_new,                 /* tp_new */
};

/***************/
/**** Score ****/
/***************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    char *fname;
    char *curfname;
    int last_value;
} Score;

static void
Score_selector(Score *self) {
    int i, inval, state, res;
    
    float *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        inval = (int)in[i];
        if (inval != self->last_value) {
            res = asprintf(&self->curfname, "%s%i()\n", self->fname, inval);
            state = PyRun_SimpleString(self->curfname);
            self->last_value = inval;
        }    
    }
}

static void
Score_setProcMode(Score *self)
{
    self->proc_func_ptr = Score_selector;
}

static void
Score_compute_next_data_frame(Score *self)
{
    (*self->proc_func_ptr)(self);    
}

static int
Score_traverse(Score *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);    
    Py_VISIT(self->input_stream);    
    return 0;
}

static int 
Score_clear(Score *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);    
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
Score_dealloc(Score* self)
{
    free(self->data);
	free(self->curfname);
    Score_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Score_deleteStream(Score *self) { DELETE_STREAM };

static PyObject *
Score_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Score *self;
    self = (Score *)type->tp_alloc(type, 0);
    
    self->last_value = -99;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Score_compute_next_data_frame);
    self->mode_func_ptr = Score_setProcMode;
    
    return (PyObject *)self;
}

static int
Score_init(Score *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "fname", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|s", kwlist, &inputtmp, &self->fname))
        return -1; 
    
    INIT_INPUT_STREAM
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
    }
    Stream_setData(self->stream, self->data);
    
    (*self->mode_func_ptr)(self);
    
    Score_compute_next_data_frame((Score *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Score_getServer(Score* self) { GET_SERVER };
static PyObject * Score_getStream(Score* self) { GET_STREAM };

static PyObject * Score_play(Score *self) { PLAY };
static PyObject * Score_stop(Score *self) { STOP };

static PyMemberDef Score_members[] = {
{"server", T_OBJECT_EX, offsetof(Score, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Score, stream), 0, "Stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef Score_methods[] = {
{"getServer", (PyCFunction)Score_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Score_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Score_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Score_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Score_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject ScoreType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Score_base",         /*tp_name*/
sizeof(Score),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Score_dealloc, /*tp_dealloc*/
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
"Score objects. Calls numbered function from an integer count.",           /* tp_doc */
(traverseproc)Score_traverse,   /* tp_traverse */
(inquiry)Score_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Score_methods,             /* tp_methods */
Score_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Score_init,      /* tp_init */
0,                         /* tp_alloc */
Score_new,                 /* tp_new */
};

