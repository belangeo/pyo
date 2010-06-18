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

#include "Python.h"
#include <math.h>

extern PyTypeObject SineType;
extern PyTypeObject SineLoopType;
extern PyTypeObject FmType;
extern PyTypeObject PhasorType;
extern PyTypeObject PointerType;
extern PyTypeObject LookupType;
extern PyTypeObject TableReadType;
extern PyTypeObject TableReadTrigType;
extern PyTypeObject OscType;
extern PyTypeObject OscLoopType;
extern PyTypeObject PulsarType;
extern PyTypeObject NoiseType;
extern PyTypeObject InputType;
extern PyTypeObject SfPlayerType;
extern PyTypeObject SfPlayType;
extern PyTypeObject SfPlayTrigType;
extern PyTypeObject SfMarkerShufflerType;
extern PyTypeObject SfMarkerShuffleType;

extern PyTypeObject TrigType;
extern PyTypeObject MetroType;
extern PyTypeObject ClouderType;
extern PyTypeObject CloudType;
extern PyTypeObject CounterType;
extern PyTypeObject SelectType;
extern PyTypeObject ChangeType;
extern PyTypeObject ThreshType;

extern PyTypeObject ScoreType;

extern PyTypeObject FaderType;
extern PyTypeObject AdsrType;
extern PyTypeObject LinsegType;
extern PyTypeObject ExpsegType;

extern PyTypeObject RandiType;
extern PyTypeObject RandhType;
extern PyTypeObject ChoiceType;
extern PyTypeObject RandIntType;
extern PyTypeObject XnoiseType;
extern PyTypeObject XnoiseMidiType;

extern PyTypeObject BiquadType;
extern PyTypeObject EQType;
extern PyTypeObject ToneType;
extern PyTypeObject DCBlockType;
extern PyTypeObject PortType;
extern PyTypeObject AllpassType;
extern PyTypeObject Allpass2Type;
extern PyTypeObject PhaserType;
extern PyTypeObject DistoType;
extern PyTypeObject ClipType;
extern PyTypeObject DegradeType;
extern PyTypeObject CompressType;
extern PyTypeObject DelayType;
extern PyTypeObject WaveguideType;
extern PyTypeObject FreeverbType;
extern PyTypeObject WGVerbType;
extern PyTypeObject ConvolveType;

extern PyTypeObject GranulatorType;
extern PyTypeObject HarmonizerType;

extern PyTypeObject MidictlType;
extern PyTypeObject MidiNoteType;
extern PyTypeObject NoteinType;

extern PyTypeObject DummyType;
extern PyTypeObject RecordType;
extern PyTypeObject CompareType;
extern PyTypeObject MixType;
extern PyTypeObject SigType;
extern PyTypeObject SigToType;
extern PyTypeObject VarPortType;
extern PyTypeObject InputFaderType;

extern PyTypeObject HarmTableType;
extern PyTypeObject ChebyTableType;
extern PyTypeObject HannTableType;
extern PyTypeObject LinTableType;
extern PyTypeObject CosTableType;
extern PyTypeObject CurveTableType;
extern PyTypeObject ExpTableType;
extern PyTypeObject SndTableType;
extern PyTypeObject NewTableType;
extern PyTypeObject TableRecType;
extern PyTypeObject TableRecTrigType;
extern PyTypeObject TableMorphType;

extern PyTypeObject NewMatrixType;
extern PyTypeObject MatrixPointerType;
extern PyTypeObject MatrixRecType;
extern PyTypeObject MatrixRecTrigType;

extern PyTypeObject OscSendType;
extern PyTypeObject OscReceiveType;
extern PyTypeObject OscReceiverType;

extern PyTypeObject TrigRandType;
extern PyTypeObject TrigChoiceType;
extern PyTypeObject TrigEnvType;
extern PyTypeObject TrigEnvTrigType;
extern PyTypeObject TrigLinsegType;
extern PyTypeObject TrigLinsegTrigType;
extern PyTypeObject TrigExpsegType;
extern PyTypeObject TrigExpsegTrigType;
extern PyTypeObject TrigFuncType;
extern PyTypeObject TrigXnoiseType;
extern PyTypeObject TrigXnoiseMidiType;

extern PyTypeObject PatternType;
extern PyTypeObject CallAfterType;

extern PyTypeObject BandSplitterType;
extern PyTypeObject BandSplitType;

extern PyTypeObject HilbertMainType;
extern PyTypeObject HilbertType;

extern PyTypeObject FollowerType;
extern PyTypeObject ZCrossType;

extern PyTypeObject SPannerType;
extern PyTypeObject SPanType;
extern PyTypeObject PannerType;
extern PyTypeObject PanType;
extern PyTypeObject SwitcherType;
extern PyTypeObject SwitchType;
extern PyTypeObject SelectorType;

extern PyTypeObject PrintType;
extern PyTypeObject SnapType;
extern PyTypeObject InterpType;
extern PyTypeObject SampHoldType;

extern PyTypeObject M_SinType;
extern PyTypeObject M_CosType;
extern PyTypeObject M_TanType;
extern PyTypeObject M_AbsType;
extern PyTypeObject M_SqrtType;
extern PyTypeObject M_LogType;
extern PyTypeObject M_Log2Type;
extern PyTypeObject M_Log10Type;
extern PyTypeObject M_PowType;

/* Constants */
#define E M_E
#define PI M_PI
#define TWOPI (2 * M_PI)

/* random uniform (0.0 -> 1.0) */
#define RANDOM_UNIFORM rand()/((float)(RAND_MAX)+1)

/* object headers */
#define pyo_audio_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    Stream *stream; \
    void (*mode_func_ptr)(); \
    void (*proc_func_ptr)(); \
    void (*muladd_func_ptr)(); \
    PyObject *mul; \
    Stream *mul_stream; \
    PyObject *add; \
    Stream *add_stream; \
    int bufsize; \
    int nchnls; \
    float sr; \
    float *data; 

#define pyo_table_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    TableStream *tablestream; \
    int size; \
    float *data;

#define pyo_matrix_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    MatrixStream *matrixstream; \
    int rowsize; \
    int colsize; \
    float **data;

/* VISIT & CLEAR */
#define pyo_VISIT \
    Py_VISIT(self->stream); \
    Py_VISIT(self->server); \
    Py_VISIT(self->mul); \
    Py_VISIT(self->mul_stream); \
    Py_VISIT(self->add); \
    Py_VISIT(self->add_stream);    

#define pyo_CLEAR \
    Py_CLEAR(self->stream); \
    Py_CLEAR(self->server); \
    Py_CLEAR(self->mul); \
    Py_CLEAR(self->mul_stream); \
    Py_CLEAR(self->add); \
    Py_CLEAR(self->add_stream);    

#define DELETE_STREAM \
    Server_removeStream((Server *)self->server, Stream_getStreamId(self->stream)); \
    Py_INCREF(Py_None); \
    return Py_None;

/* INIT INPUT STREAM */
#define INIT_INPUT_STREAM \
    Py_XDECREF(self->input); \
    self->input = inputtmp; \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL); \
    Py_INCREF(input_streamtmp); \
    Py_XDECREF(self->input_stream); \
    self->input_stream = (Stream *)input_streamtmp;


/* Set data */
#define SET_TABLE_DATA \
    int i; \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->size = PyList_Size(arg)-1; \
    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float)); \
    TableStream_setSize(self->tablestream, self->size+1); \
 \
    for (i=0; i<(self->size+1); i++) { \
        self->data[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(arg, i))); \
    } \
    TableStream_setData(self->tablestream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define SET_MATRIX_DATA \
    int i, j; \
    PyObject *innerlist; \
 \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->rowsize = PyList_Size(arg); \
    self->colsize = PyList_Size(PyList_GetItem(arg, 0)); \
    self->data = (float **)realloc(self->data, (self->rowsize + 1) * sizeof(float)); \
    for (i=0; i<(self->rowsize+1); i++) { \
        self->data[i] = (float *)realloc(self->data[i], (self->colsize + 1) * sizeof(float)); \
    } \
    MatrixStream_setRowSize(self->matrixstream, self->rowsize); \
    MatrixStream_setColSize(self->matrixstream, self->colsize); \
 \
    for(i=0; i<self->rowsize; i++) { \
        innerlist = PyList_GetItem(arg, i); \
        for (j=0; j<self->colsize; j++) { \
            self->data[i][j] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(innerlist, j))); \
        } \
    } \
 \
    MatrixStream_setData(self->matrixstream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define GET_TABLE \
    int i; \
    PyObject *samples; \
 \
    samples = PyList_New(self->size); \
    for(i=0; i<self->size; i++) { \
        PyList_SetItem(samples, i, PyFloat_FromDouble(self->data[i])); \
    } \
 \
    return samples;

#define GET_VIEW_TABLE \
    int i, y; \
    int w = 500; \
    int h = 200; \
    int h2 = h/2; \
    int amp = h2 - 2; \
    float step = (float)self->size / (float)(w - 1); \
    PyObject *samples; \
 \
    samples = PyList_New(w*4); \
    for(i=0; i<w; i++) { \
        y = self->data[(int)(i*step)-1] * amp + amp + 1; \
        PyList_SetItem(samples, i*4, PyInt_FromLong(i)); \
        PyList_SetItem(samples, i*4+1, PyInt_FromLong(h-y)); \
        PyList_SetItem(samples, i*4+2, PyInt_FromLong(i)); \
        PyList_SetItem(samples, i*4+3, PyInt_FromLong(h-y)); \
    } \
 \
    return samples;

/* Normalize */
#define NORMALIZE \
	int i; \
	float mi, ma, max, ratio; \
	mi = ma = *self->data; \
	for (i=1; i<self->size; i++) { \
		if (mi > *(self->data+i)) \
			mi = *(self->data+i); \
		if (ma < *(self->data+i)) \
			ma = *(self->data+i); \
	} \
	if ((mi*mi) > (ma*ma)) \
		max = fabsf(mi); \
	else \
		max = fabsf(ma); \
 \
	if (max > 0.0) { \
		ratio = 0.99 / max; \
		for (i=0; i<self->size+1; i++) { \
			self->data[i] *= ratio; \
		} \
	} \
	Py_INCREF(Py_None); \
	return Py_None; \

#define NORMALIZE_MATRIX \
    int i, j; \
    float mi, ma, max, ratio; \
    mi = ma = self->data[0][0]; \
    for (i=1; i<self->rowsize; i++) { \
        for (j=1; j<self->colsize; j++) { \
            if (mi > self->data[i][j]) \
                mi = self->data[i][j]; \
            if (ma < self->data[i][j]) \
                ma = self->data[i][j]; \
        } \
    } \
    if ((mi*mi) > (ma*ma)) \
        max = fabsf(mi); \
    else \
        max = fabsf(ma); \
 \
    if (max > 0.0) { \
        ratio = 0.99 / max; \
        for (i=0; i<self->rowsize+1; i++) { \
            for (j=0; j<self->colsize+1; j++) { \
                self->data[i][j] *= ratio; \
            } \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \


#define TABLE_PUT \
    float val; \
    int pos = 0; \
    static char *kwlist[] = {"value", "pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "f|i", kwlist, &val, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) { \
        PyErr_SetString(PyExc_TypeError, "position outside of table boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    self->data[pos] = val; \
 \
    Py_RETURN_NONE;

#define TABLE_GET \
    int pos; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) { \
        PyErr_SetString(PyExc_TypeError, "position outside of table boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[pos]);

/* Matrix macros */
#define MATRIX_BLUR \
    int i,j; \
    float tmp[self->rowsize][self->colsize]; \
 \
    int lc = self->colsize - 1; \
    int lr = self->rowsize - 1; \
    for (i=1; i<lc; i++) { \
        tmp[0][i] = (self->data[0][i-1] + self->data[0][i] + self->data[1][i] + self->data[0][i+1]) * 0.25; \
        tmp[lr][i] = (self->data[lr][i-1] + self->data[lr][i] + self->data[lr-1][i] + self->data[lr][i+1]) * 0.25; \
    } \
    for (i=1; i<lr; i++) { \
        tmp[i][0] = (self->data[i-1][0] + self->data[i][0] + self->data[i][1] + self->data[i+1][0]) * 0.25; \
        tmp[i][lc] = (self->data[i-1][lc] + self->data[i][lc] + self->data[i][lc-1] + self->data[i+1][lc]) * 0.25; \
    } \
 \
    for (i=1; i<lr; i++) { \
        for (j=1; j<lc; j++) { \
            tmp[i][j] = (self->data[i][j-1] + self->data[i][j] + self->data[i][j+1]) * 0.3333333; \
        } \
    } \
    for (j=1; j<lc; j++) { \
        for (i=1; i<lr; i++) { \
            self->data[i][j] = (tmp[i-1][j] + tmp[i][j] + tmp[i+1][j]) * 0.3333333; \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None;

#define MATRIX_BOOST \
    int i, j; \
    float min, max, boost, val; \
    min = -1.0; \
    max = 1.0; \
    boost = 0.01; \
    static char *kwlist[] = {"min", "max", "boost", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|fff", kwlist, &min, &max, &boost)) \
        return PyInt_FromLong(-1); \
 \
    float mid = (min + max) * 0.5; \
 \
    for (i=0; i<self->rowsize; i++) { \
        for (j=0; j<self->colsize; j++) { \
            val = self->data[i][j]; \
            self->data[i][j] = NewMatrix_clip(val + (val-mid) * boost, min, max); \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_PUT \
    float val; \
    int row, col; \
    row = col = 0; \
    static char *kwlist[] = {"value", "row", "col", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "f|ii", kwlist, &val, &row, &col)) \
        return PyInt_FromLong(-1); \
 \
    if (row >= self->rowsize) { \
        PyErr_SetString(PyExc_TypeError, "row position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (col >= self->colsize) { \
        PyErr_SetString(PyExc_TypeError, "column position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    self->data[row][col] = val; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_GET \
    int row, col; \
    static char *kwlist[] = {"row", "col", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &row, &col)) \
        return PyInt_FromLong(-1); \
 \
    if (row >= self->rowsize) { \
        PyErr_SetString(PyExc_TypeError, "row position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (col >= self->colsize) { \
        PyErr_SetString(PyExc_TypeError, "column position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[row][col]); \


/* Init Server & Stream */
#define INIT_OBJECT_COMMON \
    self->server = PyServer_get_server(); \
    self->mul = PyFloat_FromDouble(1); \
    self->add = PyFloat_FromDouble(0); \
    self->bufsize = PyInt_AsLong(PyObject_CallMethod(self->server, "getBufferSize", NULL)); \
    self->sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    self->nchnls = PyInt_AsLong(PyObject_CallMethod(self->server, "getNchnls", NULL)); \
    self->data = (float *)realloc(self->data, (self->bufsize) * sizeof(float)); \
    MAKE_NEW_STREAM(self->stream, &StreamType, NULL); \
    Stream_setStreamObject(self->stream, (PyObject *)self); \
    Stream_setStreamId(self->stream, Stream_getNewStreamId());


#define SET_INTERP_POINTER \
    if (self->interp == 0) \
        self->interp = 2; \
    if (self->interp == 1) \
        self->interp_func_ptr = nointerp; \
    else if (self->interp == 2) \
        self->interp_func_ptr = linear; \
    else if (self->interp == 3) \
        self->interp_func_ptr = cosine; \
    else if (self->interp == 4) \
        self->interp_func_ptr = cubic; \

/* GETS & SETS */
#define GET_SERVER \
    if (self->server == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No server found!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->server); \
    return self->server;

#define GET_STREAM \
    if (self->stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->stream); \
    return (PyObject *)self->stream;

#define GET_TABLE_STREAM \
    if (self->tablestream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No table stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->tablestream); \
    return (PyObject *)self->tablestream; \

#define GET_MATRIX_STREAM \
    if (self->matrixstream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No matrix stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->matrixstream); \
    return (PyObject *)self->matrixstream; \

#define SET_MUL \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->mul); \
    if (isNumber == 1) { \
        self->mul = PyNumber_Float(tmp); \
        self->modebuffer[0] = 0; \
    } \
    else { \
        self->mul = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_ADD \
    PyObject *tmp, *streamtmp; \
\
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
\
    int isNumber = PyNumber_Check(arg); \
\
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Float(tmp); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 1; \
    } \
\
    (*self->mode_func_ptr)(self); \
\
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_SUB \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Multiply(PyNumber_Float(tmp), PyFloat_FromDouble(-1)); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_DIV \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    if (isNumber == 1) { \
        if (PyFloat_AsDouble(PyNumber_Float(tmp)) != 0.) { \
            Py_DECREF(self->mul); \
            self->mul = PyNumber_Divide(PyFloat_FromDouble(1.), PyNumber_Float(tmp)); \
            self->modebuffer[0] = 0; \
        } \
    } \
    else { \
        Py_DECREF(self->mul); \
        self->mul = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

/* Multiply, Add, inplace_multiply & inplace_add */
#define MULTIPLY \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setMul", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_MULTIPLY \
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define ADD \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setAdd", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_ADD \
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define SUB \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setSub", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_SUB \
    PyObject_CallMethod((PyObject *)self, "setSub", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define DIV \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setDiv", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_DIV \
    PyObject_CallMethod((PyObject *)self, "setDiv", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

/* PLAY, OUT, STOP */
#define PLAY \
    Stream_setStreamActive(self->stream, 1); \
    Stream_setStreamToDac(self->stream, 0); \
    Py_INCREF(self); \
    return (PyObject *)self;

# define OUT \
    int chnltmp = 0; \
 \
    static char *kwlist[] = {"chnl", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &chnltmp)) \
        return PyInt_FromLong(-1); \
 \
    Stream_setStreamChnl(self->stream, chnltmp % self->nchnls); \
    Stream_setStreamToDac(self->stream, 1); \
    Stream_setStreamActive(self->stream, 1); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define STOP \
    int i; \
    Stream_setStreamActive(self->stream, 0); \
    Stream_setStreamChnl(self->stream, 0); \
    Stream_setStreamToDac(self->stream, 0); \
    for (i=0; i<self->bufsize; i++) { \
        self->data[i] = 0; \
    } \
    Py_INCREF(Py_None); \
    return Py_None;    

/* Post processing (mul & add) macros */
#define POST_PROCESSING_II \
    float mul, add, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    add = PyFloat_AS_DOUBLE(self->add); \
    if (mul != 1 || add != 0) { \
        for (i=0; i<self->bufsize; i++) { \
            old = self->data[i]; \
            val = mul * old + add; \
            self->data[i] = val; \
        } \
    }

#define POST_PROCESSING_AI \
    float add, old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IA \
    float mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old + add[i]; \
        self->data[i] = val; \
    } 

#define POST_PROCESSING_AA \
    float old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAI \
    float tmp, add, old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAA \
    float tmp, old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IREVA \
    float mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old - add[i]; \
        self->data[i] = val; \
    } 

#define POST_PROCESSING_AREVA \
    float old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old - add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAREVA \
    float tmp, old, val; \
    int i; \
    float *mul = Stream_getData((Stream *)self->mul_stream); \
    float *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp - add[i]; \
        self->data[i] = val; \
    }

