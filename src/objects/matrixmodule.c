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
    Py_TYPE(self)->tp_free((PyObject*)self);
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
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.MatrixStream", /*tp_name*/
sizeof(MatrixStream), /*tp_basicsize*/
0, /*tp_itemsize*/
(destructor)MatrixStream_dealloc, /*tp_dealloc*/
0, /*tp_print*/
0, /*tp_getattr*/
0, /*tp_setattr*/
0, /*tp_as_async (tp_compare in Python 2)*/
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
    pyo_matrix_VISIT
    return 0;
}

static int
NewMatrix_clear(NewMatrix *self)
{
    pyo_matrix_CLEAR
    return 0;
}

static void
NewMatrix_dealloc(NewMatrix* self)
{
    int i;
    for (i=0; i<(self->height+1); i++) {
        free(self->data[i]);
    }
    free(self->data);
    NewMatrix_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
NewMatrix_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inittmp=NULL;
    NewMatrix *self;

    self = (NewMatrix *)type->tp_alloc(type, 0);

    self->server = PyServer_get_server();
    Py_INCREF(self->server);

    self->x_pointer = self->y_pointer = 0;

    MAKE_NEW_MATRIXSTREAM(self->matrixstream, &MatrixStreamType, NULL);

    static char *kwlist[] = {"width", "height", "init", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii|O", kwlist, &self->width, &self->height, &inittmp))
        Py_RETURN_NONE;

    self->data = (MYFLT **)realloc(self->data, (self->height + 1) * sizeof(MYFLT *));

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

    return (PyObject *)self;
}

static PyObject * NewMatrix_getServer(NewMatrix* self) { GET_SERVER };
static PyObject * NewMatrix_getMatrixStream(NewMatrix* self) { GET_MATRIX_STREAM };
static PyObject * NewMatrix_normalize(NewMatrix *self) { NORMALIZE_MATRIX };
static PyObject * NewMatrix_setData(NewMatrix *self, PyObject *arg) { SET_MATRIX_DATA };
static PyObject * NewMatrix_blur(NewMatrix *self) { MATRIX_BLUR };
static PyObject * NewMatrix_boost(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_BOOST };
static PyObject * NewMatrix_put(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_PUT };
static PyObject * NewMatrix_get(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_GET };
static PyObject * NewMatrix_getInterpolated(NewMatrix *self, PyObject *args, PyObject *kwds) { MATRIX_GET_INTERPOLATED };

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
NewMatrix_getImageData(NewMatrix *self)
{
    int i, j, w3, index;
    char value;
    char matrix[self->width*self->height*3];

    w3 = self->width * 3;
    for(i=0; i<self->height; i++) {
        for (j=0; j<self->width; j++) {
            value = (char)(self->data[i][j]*128+128);
            index = i * w3 + j * 3;
            matrix[index] = matrix[index+1] = matrix[index+2] = value;
        }
    }

    return PyByteArray_FromStringAndSize(matrix, self->width*self->height*3);
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
            self->data[i][j] = PyFloat_AsDouble(PyList_GET_ITEM(innerlist, j));
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
NewMatrix_genSineTerrain(NewMatrix *self, PyObject *args, PyObject *kwds)
{
    int i, j;
    MYFLT xfreq, xphase, xsize;
    MYFLT freq = 1;
    MYFLT phase = 0.0625;
    static char *kwlist[] = {"freq", "phase", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FF, kwlist, &freq, &phase))
        return PyInt_FromLong(-1);

    xfreq = TWOPI * freq;
    xsize = 1.0 / self->width;
    for (i=0; i<self->height; i++) {
        xphase = MYSIN(i * phase);
        for (j=0; j<self->width; j++) {
            self->data[i][j] = MYSIN(xfreq * j * xsize + xphase);
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
{"getImageData", (PyCFunction)NewMatrix_getImageData, METH_NOARGS, "Returns a list of matrix samples in tuple of 3 ints normalized between 0 and 256 ."},
{"getMatrixStream", (PyCFunction)NewMatrix_getMatrixStream, METH_NOARGS, "Returns matrixstream object created by this matrix."},
{"setMatrix", (PyCFunction)NewMatrix_setMatrix, METH_O, "Sets the matrix from a list of list of floats (must be the same size as the object size)."},
{"setData", (PyCFunction)NewMatrix_setData, METH_O, "Sets the matrix from a list of list of floats (resizes the matrix)."},
{"normalize", (PyCFunction)NewMatrix_normalize, METH_NOARGS, "Normalize table samples between -1 and 1"},
{"blur", (PyCFunction)NewMatrix_blur, METH_NOARGS, "Blur the matrix."},
{"genSineTerrain", (PyCFunction)NewMatrix_genSineTerrain, METH_VARARGS|METH_KEYWORDS, "Generate a modulated sinusoidal terrain."},
{"boost", (PyCFunction)NewMatrix_boost, METH_VARARGS|METH_KEYWORDS, "Boost the contrast of the matrix."},
{"put", (PyCFunction)NewMatrix_put, METH_VARARGS|METH_KEYWORDS, "Puts a value at specified position in the matrix."},
{"get", (PyCFunction)NewMatrix_get, METH_VARARGS|METH_KEYWORDS, "Gets the value at specified position in the matrix."},
{"getInterpolated", (PyCFunction)NewMatrix_getInterpolated, METH_VARARGS|METH_KEYWORDS, "Gets the value at normalized position in the matrix."},
{"getSize", (PyCFunction)NewMatrix_getSize, METH_NOARGS, "Return the size of the matrix in samples."},
{"getRate", (PyCFunction)NewMatrix_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject NewMatrixType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.NewMatrix_base",         /*tp_name*/
sizeof(NewMatrix),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)NewMatrix_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_as_async (tp_compare in Python 2)*/
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
0,      /* tp_init */
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
    TriggerStream *trig_stream;
} MatrixRec;

static void
MatrixRec_compute_next_data_frame(MatrixRec *self)
{
    int i, num, num2, upBound;
    MYFLT val;
    int width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    int height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    int size = width * height;

    int off = self->delay - self->delayCount;

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

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
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
MatrixRec_clear(MatrixRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->matrix);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
MatrixRec_dealloc(MatrixRec* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    MatrixRec_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MatrixRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *matrixtmp;
    MatrixRec *self;
    self = (MatrixRec *)type->tp_alloc(type, 0);

    self->pointer = 0;
    self->active = 1;
    self->fadetime = 0.;
    self->delay = self->delayCount = 0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, MatrixRec_compute_next_data_frame);

    static char *kwlist[] = {"input", "matrix", "fadetime", "delay", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_FI, kwlist, &inputtmp, &matrixtmp, &self->fadetime, &self->delay))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->matrix);
    Py_INCREF(matrixtmp);
    self->matrix = (NewMatrix *)matrixtmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    int width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    int height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    int size = width * height;
    if ((self->fadetime * self->sr) > (size * 0.5))
        self->fadetime = size * 0.5 / self->sr;
    self->fadeInSample = roundf(self->fadetime * self->sr + 0.5);

    return (PyObject *)self;
}

static PyObject * MatrixRec_getServer(MatrixRec* self) { GET_SERVER };
static PyObject * MatrixRec_getStream(MatrixRec* self) { GET_STREAM };
static PyObject * MatrixRec_getTriggerStream(MatrixRec* self) { GET_TRIGGER_STREAM };

static PyObject * MatrixRec_play(MatrixRec *self, PyObject *args, PyObject *kwds)
{
    self->pointer = 0;
    self->active = 1;
    self->delayCount = 0;
    PLAY
};

static PyObject * MatrixRec_stop(MatrixRec *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
MatrixRec_setMatrix(MatrixRec *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->matrix);
    self->matrix = (NewMatrix *)tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef MatrixRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(MatrixRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MatrixRec, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(MatrixRec, trig_stream), 0, "Trigger Stream object."},
    {"input", T_OBJECT_EX, offsetof(MatrixRec, input), 0, "Input sound object."},
    {"matrix", T_OBJECT_EX, offsetof(MatrixRec, matrix), 0, "matrix to record in."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MatrixRec_methods[] = {
    {"getServer", (PyCFunction)MatrixRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MatrixRec_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)MatrixRec_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"setMatrix", (PyCFunction)MatrixRec_setMatrix, METH_O, "Sets a new Matrix."},
    {"play", (PyCFunction)MatrixRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixRec_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixRecType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MatrixRec_base",         /*tp_name*/
    sizeof(MatrixRec),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixRec_dealloc, /*tp_dealloc*/
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
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixRec_new,                 /* tp_new */
};

/******************************/
/* MatrixRecLoop object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    NewMatrix *matrix;
    int pointer;
    MYFLT *trigsBuffer;
    TriggerStream *trig_stream;
} MatrixRecLoop;

static void
MatrixRecLoop_compute_next_data_frame(MatrixRecLoop *self)
{
    int i;
    int width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    int height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    int size = width * height;

    MYFLT buffer[self->bufsize];
    memset(&buffer, 0, sizeof(buffer));
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
        buffer[i] = in[i];
        if (self->pointer++ >= size) {
            self->pointer = 0;
            self->trigsBuffer[i] = 1.0;
        }
    }
    NewMatrix_recordChunkAllRow((NewMatrix *)self->matrix, buffer, self->bufsize);
}

static int
MatrixRecLoop_traverse(MatrixRecLoop *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->matrix);
    Py_VISIT(self->trig_stream);
    return 0;
}

static int
MatrixRecLoop_clear(MatrixRecLoop *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->matrix);
    Py_CLEAR(self->trig_stream);
    return 0;
}

static void
MatrixRecLoop_dealloc(MatrixRecLoop* self)
{
    pyo_DEALLOC
    free(self->trigsBuffer);
    MatrixRecLoop_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MatrixRecLoop_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *matrixtmp;
    MatrixRecLoop *self;
    self = (MatrixRecLoop *)type->tp_alloc(type, 0);

    self->pointer = 0;

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, MatrixRecLoop_compute_next_data_frame);

    static char *kwlist[] = {"input", "matrix", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputtmp, &matrixtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->matrix);
    Py_INCREF(matrixtmp);
    self->matrix = (NewMatrix *)matrixtmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));

    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }

    MAKE_NEW_TRIGGER_STREAM(self->trig_stream, &TriggerStreamType, NULL);
    TriggerStream_setData(self->trig_stream, self->trigsBuffer);

    return (PyObject *)self;
}

static PyObject * MatrixRecLoop_getServer(MatrixRecLoop* self) { GET_SERVER };
static PyObject * MatrixRecLoop_getStream(MatrixRecLoop* self) { GET_STREAM };
static PyObject * MatrixRecLoop_getTriggerStream(MatrixRecLoop* self) { GET_TRIGGER_STREAM };

static PyObject * MatrixRecLoop_play(MatrixRecLoop *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MatrixRecLoop_stop(MatrixRecLoop *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
MatrixRecLoop_setMatrix(MatrixRecLoop *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->matrix);
    self->matrix = (NewMatrix *)tmp;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef MatrixRecLoop_members[] = {
    {"server", T_OBJECT_EX, offsetof(MatrixRecLoop, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(MatrixRecLoop, stream), 0, "Stream object."},
    {"trig_stream", T_OBJECT_EX, offsetof(MatrixRecLoop, trig_stream), 0, "Trigger Stream object."},
    {"input", T_OBJECT_EX, offsetof(MatrixRecLoop, input), 0, "Input sound object."},
    {"matrix", T_OBJECT_EX, offsetof(MatrixRecLoop, matrix), 0, "matrix to record in."},
    {NULL}  /* Sentinel */
};

static PyMethodDef MatrixRecLoop_methods[] = {
    {"getServer", (PyCFunction)MatrixRecLoop_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)MatrixRecLoop_getStream, METH_NOARGS, "Returns stream object."},
    {"_getTriggerStream", (PyCFunction)MatrixRecLoop_getTriggerStream, METH_NOARGS, "Returns trigger stream object."},
    {"setMatrix", (PyCFunction)MatrixRecLoop_setMatrix, METH_O, "Sets a new Matrix."},
    {"play", (PyCFunction)MatrixRecLoop_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixRecLoop_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixRecLoopType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MatrixRecLoop_base",         /*tp_name*/
    sizeof(MatrixRecLoop),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixRecLoop_dealloc, /*tp_dealloc*/
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
    "MatrixRecLoop objects. Record circular audio input in a Matrix object.",           /* tp_doc */
    (traverseproc)MatrixRecLoop_traverse,   /* tp_traverse */
    (inquiry)MatrixRecLoop_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    MatrixRecLoop_methods,             /* tp_methods */
    MatrixRecLoop_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixRecLoop_new,                 /* tp_new */
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
    pyo_DEALLOC
    free(self->buffer);
    MatrixMorph_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
MatrixMorph_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    long width, height, numsamps;
    PyObject *inputtmp, *input_streamtmp, *matrixtmp, *sourcestmp;
    MatrixMorph *self;
    self = (MatrixMorph *)type->tp_alloc(type, 0);

    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, MatrixMorph_compute_next_data_frame);

    static char *kwlist[] = {"input", "matrix", "sources", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOO", kwlist, &inputtmp, &matrixtmp, &sourcestmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    Py_XDECREF(self->matrix);
    Py_INCREF(matrixtmp);
    self->matrix = (PyObject *)matrixtmp;

    width = NewMatrix_getWidth((NewMatrix *)self->matrix);
    height = NewMatrix_getHeight((NewMatrix *)self->matrix);
    numsamps = width * height;
    self->buffer = (MYFLT *)realloc(self->buffer, (numsamps) * sizeof(MYFLT));

    Py_XDECREF(self->sources);
    Py_INCREF(sourcestmp);
    self->sources = (PyObject *)sourcestmp;

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    return (PyObject *)self;
}

static PyObject * MatrixMorph_getServer(MatrixMorph* self) { GET_SERVER };
static PyObject * MatrixMorph_getStream(MatrixMorph* self) { GET_STREAM };

static PyObject * MatrixMorph_play(MatrixMorph *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * MatrixMorph_stop(MatrixMorph *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject *
MatrixMorph_setMatrix(MatrixMorph *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

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
    ASSERT_ARG_NOT_NULL

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
    {"setMatrix", (PyCFunction)MatrixMorph_setMatrix, METH_O, "Sets a new matrix."},
    {"setSources", (PyCFunction)MatrixMorph_setSources, METH_O, "Changes the sources matrixs."},
    {"play", (PyCFunction)MatrixMorph_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)MatrixMorph_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject MatrixMorphType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.MatrixMorph_base",         /*tp_name*/
    sizeof(MatrixMorph),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)MatrixMorph_dealloc, /*tp_dealloc*/
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
    0,      /* tp_init */
    0,                         /* tp_alloc */
    MatrixMorph_new,                 /* tp_new */
};
