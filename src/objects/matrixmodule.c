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
#include "servermodule.h"
#include "streammodule.h"
#include "dummymodule.h"

#define __MATRIX_MODULE
#include "matrixmodule.h"
#undef __MATRIX_MODULE

/*************************/
/* MatrixStream structure */
/*************************/
static void
MatrixStream_dealloc(MatrixStream* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
MatrixStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MatrixStream *self;
    MAKE_NEW_MATRIXSTREAM(self, type, NULL);
    return (PyObject *)self;
}

int
MatrixStream_getWidth(MatrixStream *self)
{
    return self->width;
}

int
MatrixStream_getHeight(MatrixStream *self)
{
    return self->height;
}

/* width and height position normalized between 0 and 1 */
MYFLT
MatrixStream_getInterpPointFromPos(MatrixStream *self, MYFLT x, MYFLT y)
{
    MYFLT xpos, ypos, xfpart, yfpart, x1, x2, x3, x4;
    int xipart, yipart;

    xpos = x * self->width;
    if (xpos < 0)
        xpos += self->width;
    else if (xpos >= self->width) {
        while (xpos >= self->width) {
            xpos -= self->width;
        }
    }    

    ypos = y * self->height;
    if (ypos < 0)
        ypos += self->height;
    else if (ypos >= self->height) {
        while (ypos >= self->height) {
            ypos -= self->height;
        }
    }    

    xipart = (int)xpos;
    xfpart = xpos - xipart;
        
    yipart = (int)ypos;
    yfpart = ypos - yipart;

    x1 = self->data[yipart][xipart]; // (0, 0)
    x2 = self->data[yipart+1][xipart]; // (0, 1)
    x3 = self->data[yipart][xipart+1]; // (1, 0)
    x4 = self->data[yipart+1][xipart+1]; // (1, 1)
        
    return (x1*(1-yfpart)*(1-xfpart) + x2*yfpart*(1-xfpart) + x3*(1-yfpart)*xfpart + x4*yfpart*xfpart);
}

MYFLT 
MatrixStream_getPointFromPos(MatrixStream *self, long x, long y)
{
    return self->data[y][x];
}    

void
MatrixStream_setData(MatrixStream *self, MYFLT **data)
{
    self->data = data;
}    

void
MatrixStream_setWidth(MatrixStream *self, int size)
{
    self->width = size;
}    

void
MatrixStream_setHeight(MatrixStream *self, int size)
{
    self->height = size;
}    

PyTypeObject MatrixStreamType = {
PyObject_HEAD_INIT(NULL)
0, /*ob_size*/
"_pyo.MatrixStream", /*tp_name*/
sizeof(MatrixStream), /*tp_basicsize*/
0, /*tp_itemsize*/
(destructor)MatrixStream_dealloc, /*tp_dealloc*/
0, /*tp_print*/
0, /*tp_getattr*/
0, /*tp_setattr*/
0, /*tp_compare*/
0, /*tp_repr*/
0, /*tp_as_number*/
0, /*tp_as_sequence*/
0, /*tp_as_mapping*/
0, /*tp_hash */
0, /*tp_call*/
0, /*tp_str*/
0, /*tp_getattro*/
0, /*tp_setattro*/
0, /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
"MatrixStream objects. For internal use only. Must never be instantiated by the user.", /* tp_doc */
0, /* tp_traverse */
0, /* tp_clear */
0, /* tp_richcompare */
0, /* tp_weaklistoffset */
0, /* tp_iter */
0, /* tp_iternext */
0, /* tp_methods */
0, /* tp_members */
0, /* tp_getset */
0, /* tp_base */
0, /* tp_dict */
0, /* tp_descr_get */
0, /* tp_descr_set */
0, /* tp_dictoffset */
0, /* tp_init */
0, /* tp_alloc */
MatrixStream_new, /* tp_new */
};

/***********************/
/* NewMatrix structure */
/***********************/
typedef struct {
    pyo_matrix_HEAD
    int x_pointer;
    int y_pointer;
} NewMatrix;

MYFLT 
NewMatrix_clip(MYFLT val, MYFLT min, MYFLT max) {
    if (val < min) return min;
    else if (val > max) return max;
    else return val;
}

static PyObject *
NewMatrix_recordChunkAllRow(NewMatrix *self, MYFLT *data, long datasize)
{
    long i;

    for (i=0; i<datasize; i++) {
        self->data[self->y_pointer][self->x_pointer++] = data[i];
        if (self->x_pointer >= self->width) {
            self->x_pointer = 0;
            self->y_pointer++;
            if (self->y_pointer >= self->height)
                self->y_pointer = 0;
        }    
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static int
NewMatrix_traverse(NewMatrix *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->matrixstream);
    return 0;
}

static int 
NewMatrix_clear(NewMatrix *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->matrixstream);
    return 0;
}

static void
NewMatrix_dealloc(NewMatrix* self)
{
    int i;
    for (i=0; i<self->height; i++) {
        free(self->data[i]);
    }    
    free(self->data);
    NewMatrix_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
NewMatrix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NewMatrix *self;
    
    self = (NewMatrix *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->x_pointer = self->y_pointer = 0;
    
    MAKE_NEW_MATRIXSTREAM(self->matrixstream, &MatrixStreamType, NULL);
    
    return (PyObject *)self;
}

static int
NewMatrix_init(NewMatrix *self, PyObject *args, PyObject *kwds)
{    
    int i, j;
    PyObject *inittmp=NULL;
    static char *kwlist[] = {"width", "height", "init", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii|O", kwlist, &self->width, &self->height, &inittmp))
        return -1; 

    self->data = (MYFLT **)realloc(self->data, (self->height + 1) * sizeof(MYFLT));

    for (i=0; i<(self->height+1); i++) {
        self->data[i] = (MYFLT *)malloc((self->width + 1) * sizeof(MYFLT));
    }

    for(i=0; i<(self->height+1); i++) {
        for (j=0; j<(self->width+1); j++) {
            self->data[i][j] = 0.0;
        }    
    }

    MatrixStream_setWidth(self->matrixstream, self->width);
    MatrixStream_setHeight(self->matrixstream, self->height);

    if (inittmp) {
        PyObject_CallMethod((PyObject *)self, "setMatrix", "O", inittmp);
    }
    
    MatrixStream_setData(self->matrixstream, self->data);

    Py_INCREF(self);
    return 0;
}

static PyObject * NewMatrix_getServer(NewMatrix* self) { GET_SERVER };
static PyObject * NewMatrix_getMatrixStream(NewMatrix* self) { GET_MATRIX_STREAM };
static PyObject * NewMatrix_normalize(NewMatrix *self) { NORMALIZE_MATRIX };
static PyObject * NewMatrix_setData(NewMatrix *self, PyObject *arg) { SET_MATRIX_DATA };
static PyObject * NewMatrix_blur(NewMatrix *self) { MATRIX_BLUR };
static PyObject * NewMatrix_boost(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_BOOST };
static PyObject * NewMatrix_put(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_PUT };
static PyObject * NewMatrix_get(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_GET };

static PyObject *
NewMatrix_getSize(NewMatrix *self)
{
    return Py_BuildValue("(ii)", self->width, self->height);
};

static int
NewMatrix_getWidth(NewMatrix *self)
{
    return self->width;
};

static int
NewMatrix_getHeight(NewMatrix *self)
{
    return self->height;
};

static PyObject *
NewMatrix_getRate(NewMatrix *self)
{
    MYFLT sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr / self->width);
};

static PyObject *
NewMatrix_getData(NewMatrix *self)
{
    int i, j;
    PyObject *matrix, *samples;
    
    matrix = PyList_New(self->height);
    for(i=0; i<self->height; i++) {
        samples = PyList_New(self->width);
        for (j=0; j<self->width; j++) {
            PyList_SetItem(samples, j, PyFloat_FromDouble(self->data[i][j]));
        }    
        PyList_SetItem(matrix, i, samples);
    }
    
    return matrix;
};

static PyObject *
NewMatrix_getViewData(NewMatrix *self)
{
    int i, j;
    PyObject *matrix;
    
    matrix = PyList_New(self->width*self->height);
    for(i=0; i<self->height; i++) {
        for (j=0; j<self->width; j++) {
            PyList_SET_ITEM(matrix, i*self->width+j, PyFloat_FromDouble(self->data[i][j]*128+128));
        }    
    }
    
    return matrix;
};

static PyObject *
NewMatrix_setMatrix(NewMatrix *self, PyObject *value)
{
    int i, j;
    PyObject *innerlist;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The matrix value value must be a list.");
        return PyInt_FromLong(-1);
    }

    int height = PyList_Size(value);
    int width = PyList_Size(PyList_GetItem(value, 0));
    if (height != self->height || width != self->width) {
        PyErr_SetString(PyExc_TypeError, "New matrix must be of the same size as actual matrix.");
        return PyInt_FromLong(-1);
    }
    
    for(i=0; i<self->height; i++) {
        innerlist = PyList_GetItem(value, i);
        for (j=0; j<self->width; j++) {
            self->data[i][j] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(innerlist, j)));
        }    
    }

    Py_INCREF(Py_None);
    return Py_None;    
}

static PyMemberDef NewMatrix_members[] = {
{"server", T_OBJECT_EX, offsetof(NewMatrix, server), 0, "Pyo server."},
{"matrixstream", T_OBJECT_EX, offsetof(NewMatrix, matrixstream), 0, "Matrix stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef NewMatrix_methods[] = {
{"getServer", (PyCFunction)NewMatrix_getServer, METH_NOARGS, "Returns server object."},
{"getData", (PyCFunction)NewMatrix_getData, METH_NOARGS, "Returns a list of matrix samples."},
{"getViewData", (PyCFunction)NewMatrix_getViewData, METH_NOARGS, "Returns a list of matrix samples normalized between 0 and 256 ."},
{"getMatrixStream", (PyCFunction)NewMatrix_getMatrixStream, METH_NOARGS, "Returns matrixstream object created by this matrix."},
{"setMatrix", (PyCFunction)NewMatrix_setMatrix, METH_O, "Sets the matrix from a list of list of floats (must be the same size as the object size)."},
{"setData", (PyCFunction)NewMatrix_setData, METH_O, "Sets the matrix from a list of list of floats (resizes the matrix)."},
{"normalize", (PyCFunction)NewMatrix_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"blur", (PyCFunction)NewMatrix_blur, METH_NOARGS, "Blur the matrix."},
{"boost", (PyCFunction)NewMatrix_boost, METH_VARARGS|METH_KEYWORDS, "Boost the contrast of the matrix."},
{"put", (PyCFunction)NewMatrix_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the matrix."},
{"get", (PyCFunction)NewMatrix_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the matrix."},
{"getSize", (PyCFunction)NewMatrix_getSize, METH_NOARGS, "Return the size of the matrix in samples."},
{"getRate", (PyCFunction)NewMatrix_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject NewMatrixType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.NewMatrix_base",         /*tp_name*/
sizeof(NewMatrix),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)NewMatrix_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
0,                         /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
"NewMatrix objects. Generates an empty matrix.",  /* tp_doc */
(traverseproc)NewMatrix_traverse,   /* tp_traverse */
(inquiry)NewMatrix_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
NewMatrix_methods,             /* tp_methods */
NewMatrix_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)NewMatrix_init,      /* tp_init */
0,                         /* tp_alloc */
NewMatrix_new,                 /* tp_new */
};

/******************************/
/* MatrixRec object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    NewMatrix *matrix;
    int pointer;
    int active;
    int delay;
    int delayCount;
    MYFLT fadetime;
    MYFLT fadeInSample;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
} MatrixRec;

static void
MatrixRec_compute_next_data_frame(MatrixRec *self)
{
    int i, num, num2, upBound;
    MYFLT sclfade, val;
    int width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    int height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    int size = width * height;
   
    int off = self->delay - self->delayCount;
 
    if ((size - self->pointer) >= self->bufsize)
        num = self->bufsize;
    else {
        num = size - self->pointer;
        if (self->active == 1) {
            if (num <= 0)
                self->trigsBuffer[0] = 1.0;
            else
                self->trigsBuffer[num-1] = 1.0;
            self->active = 0;
        }    
    }
    
    if (self->pointer < size) {   
        sclfade = 1. / self->fadetime;
        upBound = size - self->fadeInSample;
       
        if (off == 0)
            num2 = num;
        else if ((num-off) <= 0)
            num2 = 0;
        else
            num2 = num-off;

        MYFLT buffer[num2];
        memset(&buffer, 0, sizeof(buffer));
        MYFLT *in = Stream_getData((Stream *)self->input_stream);
        
        for (i=0; i<num; i++) {
            if (self->delayCount < self->delay) {
                self->delayCount++;
            }
            else {
                if (self->pointer < self->fadeInSample)
                    val = self->pointer / self->fadeInSample;
                else if (self->pointer > upBound)
                    val = (size - self->pointer) / self->fadeInSample;
                else
                    val = 1.;
                buffer[i-off] = in[i] * val;
                self->pointer++;
            }
        }
        NewMatrix_recordChunkAllRow((NewMatrix *)self->matrix, buffer, num2);
    }    
}

static int
MatrixRec_traverse(MatrixRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->matrix);
    return 0;
}

static int 
MatrixRec_clear(MatrixRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->matrix);
    return 0;
}

static void
MatrixRec_dealloc(MatrixRec* self)
{
    free(self->data);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    MatrixRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * MatrixRec_deleteStream(MatrixRec *self) { DELETE_STREAM };

static PyObject *
MatrixRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MatrixRec *self;
    self = (MatrixRec *)type->tp_alloc(type, 0);
    
    self->pointer = 0;
    self->active = 1;
    self->fadetime = 0.;
    self->delay = self->delayCount = 0;
    
    INIT_OBJECT_COMMON
    
    Stream_setFunctionPtr(self->stream, MatrixRec_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 0);
    
    return (PyObject *)self;
}

static int
MatrixRec_init(MatrixRec *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *matrixtmp;
    
    static char *kwlist[] = {"input", "matrix", "fadetime", "delay", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_FI, kwlist, &inputtmp, &matrixtmp, &self->fadetime, &self->delay))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->matrix);
    self->matrix = (NewMatrix *)matrixtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    int width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    int height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    int size = width * height;
    if ((self->fadetime * self->sr) > (size * 0.5))
        self->fadetime = size * 0.5 / self->sr;
    self->fadeInSample = roundf(self->fadetime * self->sr + 0.5);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * MatrixRec_getServer(MatrixRec* self) { GET_SERVER };
static PyObject * MatrixRec_getStream(MatrixRec* self) { GET_STREAM };

static PyObject * MatrixRec_play(MatrixRec *self, PyObject *args, PyObject *kwds) 
{ 
    self->pointer = 0;
    self->active = 1;
    self->delayCount = 0;
    PLAY 
};

static PyObject * MatrixRec_stop(MatrixRec *self) { STOP };

static PyObject *
MatrixRec_setMatrix(MatrixRec *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->matrix);
    self->matrix = (NewMatrix *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

MYFLT *
MatrixRec_getTrigsBuffer(MatrixRec *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    


static PyMemberDef MatrixRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(MatrixRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MatrixRec, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MatrixRec, input), 0, "Input sound object."},
    {"matrix", T_OBJECT_EX, offsetof(MatrixRec, matrix), 0, "matrix to record in."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MatrixRec_methods[] = {
    {"getServer", (PyCFunction)MatrixRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MatrixRec_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)MatrixRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"setMatrix", (PyCFunction)MatrixRec_setMatrix, METH_O, "Sets a new Matrix."},
    {"play", (PyCFunction)MatrixRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixRec_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixRecType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.MatrixRec_base",         /*tp_name*/
    sizeof(MatrixRec),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixRec_dealloc, /*tp_dealloc*/
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
    "MatrixRec objects. Record audio input in a Matrix object.",           /* tp_doc */
    (traverseproc)MatrixRec_traverse,   /* tp_traverse */
    (inquiry)MatrixRec_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MatrixRec_methods,             /* tp_methods */
    MatrixRec_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MatrixRec_init,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixRec_new,                 /* tp_new */
};

/************************************************************************************************/
/* MatrixRecTrig trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    MatrixRec *mainReader;
} MatrixRecTrig;

static void
MatrixRecTrig_compute_next_data_frame(MatrixRecTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = MatrixRec_getTrigsBuffer((MatrixRec *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
    Stream_setData(self->stream, self->data);
}

static int
MatrixRecTrig_traverse(MatrixRecTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
MatrixRecTrig_clear(MatrixRecTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
MatrixRecTrig_dealloc(MatrixRecTrig* self)
{
    free(self->data);
    MatrixRecTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * MatrixRecTrig_deleteStream(MatrixRecTrig *self) { DELETE_STREAM };

static PyObject *
MatrixRecTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MatrixRecTrig *self;
    self = (MatrixRecTrig *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, MatrixRecTrig_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
MatrixRecTrig_init(MatrixRecTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (MatrixRec *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * MatrixRecTrig_getServer(MatrixRecTrig* self) { GET_SERVER };
static PyObject * MatrixRecTrig_getStream(MatrixRecTrig* self) { GET_STREAM };

static PyObject * MatrixRecTrig_play(MatrixRecTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MatrixRecTrig_stop(MatrixRecTrig *self) { STOP };

static PyMemberDef MatrixRecTrig_members[] = {
    {"server", T_OBJECT_EX, offsetof(MatrixRecTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MatrixRecTrig, stream), 0, "Stream object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MatrixRecTrig_methods[] = {
    {"getServer", (PyCFunction)MatrixRecTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MatrixRecTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)MatrixRecTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)MatrixRecTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixRecTrig_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixRecTrigType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.MatrixRecTrig_base",         /*tp_name*/
    sizeof(MatrixRecTrig),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixRecTrig_dealloc, /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "MatrixRecTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
    (traverseproc)MatrixRecTrig_traverse,   /* tp_traverse */
    (inquiry)MatrixRecTrig_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MatrixRecTrig_methods,             /* tp_methods */
    MatrixRecTrig_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MatrixRecTrig_init,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixRecTrig_new,                 /* tp_new */
};

/******************************/
/* MatrixMorph object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *matrix;
    PyObject *sources;
    MYFLT *buffer;
} MatrixMorph;

static MYFLT
MatrixMorph_clip(MYFLT x) {
    if (x < 0.0)
        return 0.0;
    else if (x >= 0.999999)
        return 0.999999;
    else
        return x;
}

static void
MatrixMorph_compute_next_data_frame(MatrixMorph *self)
{
    int x, y;
    long i, j, width, height, numsamps, index;
    MYFLT input, interp, interp1;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    numsamps = width * height;
    int len = PyList_Size(self->sources);
    
    input = MatrixMorph_clip(in[0]);
    
    interp = input * (len - 1);
    x = (int)(interp);   
    y = x + 1;
    
    MatrixStream *tab1 = (MatrixStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, x), "getMatrixStream", "");
    MatrixStream *tab2 = (MatrixStream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->sources, y), "getMatrixStream", "");
    
    interp = MYFMOD(interp, 1.0);
    interp1 = 1. - interp;
    
    for (i=0; i<height; i++) {
        for (j=0; j<width; j++) {
            index = i*width+j;
            self->buffer[index] = MatrixStream_getPointFromPos(tab1, j, i) * interp1 + MatrixStream_getPointFromPos(tab2, j, i) * interp;
        }    
    }    
    
    NewMatrix_recordChunkAllRow((NewMatrix *)self->matrix, self->buffer, numsamps);
}

static int
MatrixMorph_traverse(MatrixMorph *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->matrix);
    Py_VISIT(self->sources);
    return 0;
}

static int 
MatrixMorph_clear(MatrixMorph *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->matrix);
    Py_CLEAR(self->sources);
    return 0;
}

static void
MatrixMorph_dealloc(MatrixMorph* self)
{
    free(self->data);
    free(self->buffer);
    MatrixMorph_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * MatrixMorph_deleteStream(MatrixMorph *self) { DELETE_STREAM };

static PyObject *
MatrixMorph_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    MatrixMorph *self;
    self = (MatrixMorph *)type->tp_alloc(type, 0);
    
    INIT_OBJECT_COMMON
    
    Stream_setFunctionPtr(self->stream, MatrixMorph_compute_next_data_frame);
    
    return (PyObject *)self;
}

static int
MatrixMorph_init(MatrixMorph *self, PyObject *args, PyObject *kwds)
{
    long width, height, numsamps;
    PyObject *inputtmp, *input_streamtmp, *matrixtmp, *sourcestmp;
    
    static char *kwlist[] = {"input", "matrix", "sources", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &matrixtmp, &sourcestmp))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->matrix);
    self->matrix = (PyObject *)matrixtmp;

    width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    numsamps = width * height;
    self->buffer = (MYFLT *)realloc(self->buffer, (numsamps) * sizeof(MYFLT));

    Py_XDECREF(self->sources);
    self->sources = (PyObject *)sourcestmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * MatrixMorph_getServer(MatrixMorph* self) { GET_SERVER };
static PyObject * MatrixMorph_getStream(MatrixMorph* self) { GET_STREAM };

static PyObject * MatrixMorph_play(MatrixMorph *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MatrixMorph_stop(MatrixMorph *self) { STOP };

static PyObject *
MatrixMorph_setMatrix(MatrixMorph *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->matrix);
    self->matrix = (PyObject *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
MatrixMorph_setSources(MatrixMorph *self, PyObject *arg)
{	
    if (arg == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(arg);
    Py_DECREF(self->sources);
    self->sources = arg;
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef MatrixMorph_members[] = {
    {"server", T_OBJECT_EX, offsetof(MatrixMorph, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MatrixMorph, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(MatrixMorph, input), 0, "Input sound object."},
    {"matrix", T_OBJECT_EX, offsetof(MatrixMorph, matrix), 0, "Matrix to record in."},
    {"sources", T_OBJECT_EX, offsetof(MatrixMorph, sources), 0, "list of matrixes to interpolate from."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MatrixMorph_methods[] = {
    {"getServer", (PyCFunction)MatrixMorph_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MatrixMorph_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)MatrixMorph_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"setMatrix", (PyCFunction)MatrixMorph_setMatrix, METH_O, "Sets a new matrix."},
    {"setSources", (PyCFunction)MatrixMorph_setSources, METH_O, "Changes the sources matrixs."},
    {"play", (PyCFunction)MatrixMorph_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixMorph_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixMorphType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.MatrixMorph_base",         /*tp_name*/
    sizeof(MatrixMorph),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixMorph_dealloc, /*tp_dealloc*/
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
    "MatrixMorph objects. Interpolation contents of different matrix objects.",           /* tp_doc */
    (traverseproc)MatrixMorph_traverse,   /* tp_traverse */
    (inquiry)MatrixMorph_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MatrixMorph_methods,             /* tp_methods */
    MatrixMorph_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MatrixMorph_init,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixMorph_new,                 /* tp_new */
};

