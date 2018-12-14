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
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

static MYFLT BLACKMAN2[257] = {0.0, 0.00001355457612407795, 0.00005422714831165853, 0.00012204424012042525, 0.00021705003118953348, 0.00033930631782219667, 0.0004888924578373699, 0.00066590529972627988, 0.00087045909615995898, 0.0011026854019042381, 0.0013627329562085205, 0.0016507675497451635, 0.001966971876186191, 0.0023115453685137038, 0.0026847040201710415, 0.0030866801911709624, 0.0035177223992877149, 0.0039780950964686118, 0.004468078430611172, 0.0049879679928611226, 0.0055380745505964057, 0.0061187237662708727, 0.0067302559023018349, 0.0073730255121936678, 0.0080474011180991928, 0.0087537648750297542, 0.0094925122219332442, 0.010264051519867853, 0.011068803677508544, 0.011907201764231275, 0.012779690611027246, 0.013686726399509554, 0.014628776239280425, 0.015606317733936455, 0.016619838535996113, 0.017669835891040944, 0.018756816171369962, 0.019881294399473233, 0.021043793761637002, 0.022244845112000651, 0.023484986467390646, 0.024764762493264474, 0.026084723981101995, 0.027445427317589366, 0.028847433945943753, 0.030291309819735913, 0.031777624849569003, 0.033306952342980187, 0.034879868437934558, 0.036496951530286398, 0.038158781695585731, 0.039865940105613923, 0.041619008440034494, 0.043418568293550078, 0.045265200578957936, 0.047159484926502321, 0.04910199907992175, 0.051093318289594611, 0.053134014703186641, 0.055224656754207603, 0.05736580854888524, 0.059558029251766974, 0.061801872470459936, 0.064097885639923663, 0.066446609406726198, 0.068848577013680551, 0.071304313685273069, 0.073814336014300028, 0.076379151350125907, 0.078999257188976796, 0.081675140566682625, 0.08440727745428013, 0.08719613215688693, 0.090042156716257177, 0.092945790317425406, 0.095907458699845349, 0.09892757357342627, 0.10200653203986923, 0.10514471601969966, 0.10834249168539431, 0.11160020890099166, 0.11491820066857752, 0.11829678258202875, 0.12173625228839696, 0.12523688895730928, 0.12879895275875847, 0.13242268434965018, 0.1361083043694708, 0.13985601294543293, 0.14366598920745235, 0.14753839081330203, 0.15147335348428598, 0.15547099055176727, 0.15953139251487919, 0.16365462660974361, 0.16784073639051059, 0.17208974132253127, 0.17640163638796383, 0.18077639170410914, 0.18521395215476394, 0.18971423703487098, 0.19427713970874003, 0.19890252728210264, 0.2035902402882592, 0.20834009238856521, 0.21315187008749686, 0.218025332462529, 0.22296021090904578, 0.22795620890049961, 0.23301300176402318, 0.2381302364716896, 0.24330753144760825, 0.24854447639103289, 0.25384063211565033, 0.25919553040520765, 0.26460867388562637, 0.27007953591374234, 0.27560756048280166, 0.28119216214482828, 0.28683272594997611, 0.29252860740296116, 0.29827913243666476, 0.30408359740298374, 0.30994126908099884, 0.31585138470251517, 0.3218131519950253, 0.32782574924213004, 0.33388832536144369, 0.33999999999999991, 0.34615986364716356, 0.35236697776504228, 0.35862037493638421, 0.36491905902993321, 0.37126200538320747, 0.37764816100265119, 0.38407644478110459, 0.39054574773252188, 0.39705493324385926, 0.40360283734404451, 0.41018826898992783, 0.41681001036910403, 0.42346681721948765, 0.43015741916550887, 0.43688052007079137, 0.44363479840716119, 0.45041890763982673, 0.45723147662855934, 0.46407111004469437, 0.47093638880376354, 0.47782587051356035, 0.4847380899374274, 0.49167155947254987, 0.49862476964302743, 0.50559618960748731, 0.51258426768099419, 0.51958743187100298, 0.526604090427091, 0.53363263240419834, 0.54067142823909731, 0.5477188303398014, 0.55477317368762102, 0.5618327764515586, 0.56889594061473336, 0.57596095261251634, 0.58302608398204925, 0.59008959202281352, 0.5971497204679086, 0.60420470016569239, 0.61125274977143074, 0.61829207644859363, 0.62532087657943414, 0.63233733648447599, 0.63933963315053088, 0.64632593496686574, 0.65329440246912585, 0.66024318909062385, 0.66717044192059383, 0.67407430246900757, 0.68095290743754511, 0.68780438949630818, 0.69462687806585954, 0.70141850010417084, 0.70817738089805216, 0.71490164485864349, 0.72158941632053231, 0.7282388203440715, 0.73484798352045921, 0.74141503477914861, 0.74793810619714429, 0.75441533380975301, 0.76084485842234006, 0.76722482642265344, 0.77355339059327366, 0.77982871092374229, 0.78604895542192688, 0.7922123009241796, 0.79831693390384428, 0.80436105127766677, 0.81034286120967125, 0.81626058391205358, 0.82211245244265874, 0.82789671349859684, 0.83361162820556423, 0.83925547290243352, 0.84482653992067935, 0.85032313835820861, 0.85574359484716933, 0.86108625431531149, 0.86634948074047979, 0.87153165789781828, 0.87663119009927604, 0.88164650292500113, 0.88657604394621592, 0.89141828343917606, 0.89617171508981341, 0.90083485668867092, 0.90540625081574555, 0.90988446551485458, 0.91426809495715211, 0.9185557600934271, 0.92274610929481327, 0.92683781898156326, 0.93082959423952683, 0.93472016942399416, 0.93850830875056723, 0.94219280687272511, 0.94577248944576608, 0.94924621367680617, 0.9526128688605292, 0.95587137690038915, 0.95902069281497004, 0.96205980522922363, 0.96498773685030803, 0.96780354492775944, 0.97050632169774165, 0.97309519481112294, 0.97556932774514038, 0.97792792019842123, 0.9801702084691396, 0.98229546581609617, 0.98430300280251803, 0.98619216762238726, 0.98796234640911229, 0.98961296352637218, 0.99114348184096723, 0.99255340297752515, 0.99384226755491845, 0.99500965540426034, 0.99605518576835683, 0.99697851748250432, 0.99777934913652766, 0.99845741921797138, 0.99901250623636195, 0.99944442882846996, 0.99975304584451585, 0.99993825641526857, 1.0};

/************/
/* Convolve */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    MYFLT *input_tmp;
    int size;
    int count;
} Convolve;

static void
Convolve_filters(Convolve *self) {
    int i,j,tmp_count;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *impulse = TableStream_getData(self->table);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * impulse[j];
        }

        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void Convolve_postprocessing_ii(Convolve *self) { POST_PROCESSING_II };
static void Convolve_postprocessing_ai(Convolve *self) { POST_PROCESSING_AI };
static void Convolve_postprocessing_ia(Convolve *self) { POST_PROCESSING_IA };
static void Convolve_postprocessing_aa(Convolve *self) { POST_PROCESSING_AA };
static void Convolve_postprocessing_ireva(Convolve *self) { POST_PROCESSING_IREVA };
static void Convolve_postprocessing_areva(Convolve *self) { POST_PROCESSING_AREVA };
static void Convolve_postprocessing_revai(Convolve *self) { POST_PROCESSING_REVAI };
static void Convolve_postprocessing_revaa(Convolve *self) { POST_PROCESSING_REVAA };
static void Convolve_postprocessing_revareva(Convolve *self) { POST_PROCESSING_REVAREVA };

static void
Convolve_setProcMode(Convolve *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = Convolve_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = Convolve_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = Convolve_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = Convolve_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = Convolve_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = Convolve_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = Convolve_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = Convolve_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = Convolve_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = Convolve_postprocessing_revareva;
            break;
    }
}

static void
Convolve_compute_next_data_frame(Convolve *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
Convolve_traverse(Convolve *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    return 0;
}

static int
Convolve_clear(Convolve *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    return 0;
}

static void
Convolve_dealloc(Convolve* self)
{
    pyo_DEALLOC
    free(self->input_tmp);
    Convolve_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Convolve_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp, *multmp=NULL, *addtmp=NULL;
    Convolve *self;
    self = (Convolve *)type->tp_alloc(type, 0);

    self->count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Convolve_compute_next_data_frame);
    self->mode_func_ptr = Convolve_setProcMode;

    static char *kwlist[] = {"input", "table", "size", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &tabletmp, &self->size, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if ( PyObject_HasAttrString((PyObject *)tabletmp, "getTableStream") == 0 ) {
        PyErr_SetString(PyExc_TypeError, "\"table\" argument of Convolve must be a PyoTableObject.\n");
        Py_RETURN_NONE;
    }
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    self->input_tmp = (MYFLT *)realloc(self->input_tmp, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = 0.0;
    }

    return (PyObject *)self;
}

static PyObject * Convolve_getServer(Convolve* self) { GET_SERVER };
static PyObject * Convolve_getStream(Convolve* self) { GET_STREAM };
static PyObject * Convolve_setMul(Convolve *self, PyObject *arg) { SET_MUL };
static PyObject * Convolve_setAdd(Convolve *self, PyObject *arg) { SET_ADD };
static PyObject * Convolve_setSub(Convolve *self, PyObject *arg) { SET_SUB };
static PyObject * Convolve_setDiv(Convolve *self, PyObject *arg) { SET_DIV };

static PyObject * Convolve_play(Convolve *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Convolve_out(Convolve *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Convolve_stop(Convolve *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * Convolve_multiply(Convolve *self, PyObject *arg) { MULTIPLY };
static PyObject * Convolve_inplace_multiply(Convolve *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Convolve_add(Convolve *self, PyObject *arg) { ADD };
static PyObject * Convolve_inplace_add(Convolve *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Convolve_sub(Convolve *self, PyObject *arg) { SUB };
static PyObject * Convolve_inplace_sub(Convolve *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Convolve_div(Convolve *self, PyObject *arg) { DIV };
static PyObject * Convolve_inplace_div(Convolve *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Convolve_getTable(Convolve* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Convolve_setTable(Convolve *self, PyObject *arg)
{
	PyObject *tmp;

    ASSERT_ARG_NOT_NULL

	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Convolve_members[] = {
{"server", T_OBJECT_EX, offsetof(Convolve, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Convolve, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Convolve, input), 0, "Input sound object."},
{"mul", T_OBJECT_EX, offsetof(Convolve, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Convolve, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Convolve_methods[] = {
{"getTable", (PyCFunction)Convolve_getTable, METH_NOARGS, "Returns impulse response table object."},
{"getServer", (PyCFunction)Convolve_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Convolve_getStream, METH_NOARGS, "Returns stream object."},
{"play", (PyCFunction)Convolve_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Convolve_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Convolve_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
{"setTable", (PyCFunction)Convolve_setTable, METH_O, "Sets inpulse response table."},
{"setMul", (PyCFunction)Convolve_setMul, METH_O, "Sets mul factor."},
{"setAdd", (PyCFunction)Convolve_setAdd, METH_O, "Sets add factor."},
{"setSub", (PyCFunction)Convolve_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Convolve_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Convolve_as_number = {
(binaryfunc)Convolve_add,                         /*nb_add*/
(binaryfunc)Convolve_sub,                         /*nb_subtract*/
(binaryfunc)Convolve_multiply,                    /*nb_multiply*/
INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
0,                                              /*nb_remainder*/
0,                                              /*nb_divmod*/
0,                                              /*nb_power*/
0,                                              /*nb_neg*/
0,                                              /*nb_pos*/
0,                                              /*(unaryfunc)array_abs,*/
0,                                              /*nb_nonzero*/
0,                                              /*nb_invert*/
0,                                              /*nb_lshift*/
0,                                              /*nb_rshift*/
0,                                              /*nb_and*/
0,                                              /*nb_xor*/
0,                                              /*nb_or*/
INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
0,                                              /*nb_int*/
0,                                              /*nb_long*/
0,                                              /*nb_float*/
INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
(binaryfunc)Convolve_inplace_add,                 /*inplace_add*/
(binaryfunc)Convolve_inplace_sub,                 /*inplace_subtract*/
(binaryfunc)Convolve_inplace_multiply,            /*inplace_multiply*/
INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
0,                                              /*inplace_remainder*/
0,                                              /*inplace_power*/
0,                                              /*inplace_lshift*/
0,                                              /*inplace_rshift*/
0,                                              /*inplace_and*/
0,                                              /*inplace_xor*/
0,                                              /*inplace_or*/
0,                                              /*nb_floor_divide*/
(binaryfunc)Convolve_div,                       /*nb_true_divide*/
0,                                              /*nb_inplace_floor_divide*/
(binaryfunc)Convolve_inplace_div,                       /*nb_inplace_true_divide*/
0,                                              /* nb_index */
};

PyTypeObject ConvolveType = {
PyVarObject_HEAD_INIT(NULL, 0)
"_pyo.Convolve_base",                                   /*tp_name*/
sizeof(Convolve),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Convolve_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_as_async (tp_compare in Python 2)*/
0,                                              /*tp_repr*/
&Convolve_as_number,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Convolve objects. Implements a circular convolution.",           /* tp_doc */
(traverseproc)Convolve_traverse,                  /* tp_traverse */
(inquiry)Convolve_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Convolve_methods,                                 /* tp_methods */
Convolve_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
0,                          /* tp_init */
0,                                              /* tp_alloc */
Convolve_new,                                     /* tp_new */
};

/************/
/* IRWinSinc */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *bandwidth;
    Stream *bandwidth_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT *impulse;
    MYFLT *impulse_tmp;
    MYFLT *input_tmp;
    int count;
    int filtertype;
    int order;
    int size;
    int changed;
    MYFLT last_freq;
    MYFLT last_bandwidth;
} IRWinSinc;

static void
IRWinSinc_alloc_memory(IRWinSinc *self) {
    int i;
    if ((self->order % 2) != 0)
        self->order += 1;

    self->size = self->order + 1;

    self->input_tmp = (MYFLT *)realloc(self->input_tmp, self->size * sizeof(MYFLT));
    self->impulse = (MYFLT *)realloc(self->impulse, self->size * sizeof(MYFLT));
    self->impulse_tmp = (MYFLT *)realloc(self->impulse_tmp, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = self->impulse[i] = self->impulse_tmp[i] = 0.0;
    }
}

static void
IRWinSinc_create_impulse(IRWinSinc *self, MYFLT freq, MYFLT bandwidth) {
    int i, half, ipart;
    MYFLT val, fpart, sum, invSum, env, envPointer, envPointerScaling, sincScaling, w;

    half = self->order / 2;
    sum = 0.0;
    envPointerScaling = 1.0 / self->size * 512.0;
    sincScaling = self->order / 2.0;

    if (freq < 1)
        freq = 1.0;
    else if (freq > (self->sr*0.5))
        freq = self->sr*0.5;

    if (bandwidth < 1)
        bandwidth = 1.0;
    else if (bandwidth > (self->sr*0.5))
        bandwidth = self->sr*0.5;

    if (self->filtertype <= 1)
        w = TWOPI * freq / self->sr;
    else
        w = TWOPI * (freq - bandwidth / 2.0) / self->sr;

    /* LOWPASS */
    for (i=0; i<half; i++) {
        envPointer = i * envPointerScaling;
        ipart = (int)envPointer;
        fpart = envPointer - ipart;
        env = BLACKMAN2[ipart] * (1.0 - fpart) + BLACKMAN2[ipart+1] * fpart;
        val = MYSIN(w * (i - sincScaling)) / (i - sincScaling) * env;
        sum += val;
        self->impulse[i] = val;
    }
    sum *= 2.0;
    sum += w;
    invSum = 1.0 / sum;
    self->impulse[half] = w * invSum;
    for (i=0; i<half; i++) {
        self->impulse[i] *= invSum;
    }
    for (i=half+1; i<self->size; i++) {
        self->impulse[i] = self->impulse[self->order-i];
    }

    /* HIGHPASS */
    if (self->filtertype == 1) {
        for (i=0; i<self->size; i++) {
            self->impulse[i] = -self->impulse[i];
        }
        self->impulse[half] = self->impulse[half] + 1.0;
    }

    /* BANDREJECT */
    if (self->filtertype >= 2) {
        sum = 0.0;
        w = TWOPI * (freq + bandwidth / 2.0) / self->sr;
        for (i=0; i<half; i++) {
            envPointer = i * envPointerScaling;
            ipart = (int)envPointer;
            fpart = envPointer - ipart;
            env = BLACKMAN2[ipart] * (1.0 - fpart) + BLACKMAN2[ipart+1] * fpart;
            val = MYSIN(w * (i - sincScaling)) / (i - sincScaling) * env;
            sum += val;
            self->impulse_tmp[i] = val;
        }
        sum *= 2.0;
        sum += w;
        invSum = 1.0 / sum;
        self->impulse_tmp[half] = w * invSum;
        for (i=0; i<half; i++) {
            self->impulse_tmp[i] *= invSum;
        }
        for (i=half+1; i<self->size; i++) {
            self->impulse_tmp[i] = self->impulse_tmp[self->order-i];
        }
        for (i=0; i<self->size; i++) {
            self->impulse_tmp[i] = -self->impulse_tmp[i];
        }
        self->impulse_tmp[half] = self->impulse_tmp[half] + 1.0;
        for (i=0; i<self->size; i++) {
            self->impulse[i] = self->impulse[i] + self->impulse_tmp[i];
        }
        /* BANDPASS */
        if (self->filtertype == 3) {
            for (i=0; i<self->size; i++) {
                self->impulse[i] = -self->impulse[i];
            }
            self->impulse[half] = self->impulse[half] + 1.0;
        }
    }
}

static void
IRWinSinc_filters(IRWinSinc *self) {
    int i,j,tmp_count;
    MYFLT freq, bw;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        freq = PyFloat_AS_DOUBLE(self->freq);
    else {
        MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
        freq = fr[0];
    }
    if (self->modebuffer[3] == 0)
        bw = PyFloat_AS_DOUBLE(self->bandwidth);
    else {
        MYFLT *band = Stream_getData((Stream *)self->bandwidth_stream);
        bw = band[0];
    }

    if (freq != self->last_freq || bw != self->last_bandwidth || self->changed == 1) {
        IRWinSinc_create_impulse(self, freq, bw);
        self->last_freq = freq;
        self->last_bandwidth = bw;
        self->changed = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * self->impulse[j];
        }

        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void IRWinSinc_postprocessing_ii(IRWinSinc *self) { POST_PROCESSING_II };
static void IRWinSinc_postprocessing_ai(IRWinSinc *self) { POST_PROCESSING_AI };
static void IRWinSinc_postprocessing_ia(IRWinSinc *self) { POST_PROCESSING_IA };
static void IRWinSinc_postprocessing_aa(IRWinSinc *self) { POST_PROCESSING_AA };
static void IRWinSinc_postprocessing_ireva(IRWinSinc *self) { POST_PROCESSING_IREVA };
static void IRWinSinc_postprocessing_areva(IRWinSinc *self) { POST_PROCESSING_AREVA };
static void IRWinSinc_postprocessing_revai(IRWinSinc *self) { POST_PROCESSING_REVAI };
static void IRWinSinc_postprocessing_revaa(IRWinSinc *self) { POST_PROCESSING_REVAA };
static void IRWinSinc_postprocessing_revareva(IRWinSinc *self) { POST_PROCESSING_REVAREVA };

static void
IRWinSinc_setProcMode(IRWinSinc *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IRWinSinc_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = IRWinSinc_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = IRWinSinc_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = IRWinSinc_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = IRWinSinc_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = IRWinSinc_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = IRWinSinc_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = IRWinSinc_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = IRWinSinc_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = IRWinSinc_postprocessing_revareva;
            break;
    }
}

static void
IRWinSinc_compute_next_data_frame(IRWinSinc *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
IRWinSinc_traverse(IRWinSinc *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->bandwidth);
    Py_VISIT(self->bandwidth_stream);
    return 0;
}

static int
IRWinSinc_clear(IRWinSinc *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->bandwidth);
    Py_CLEAR(self->bandwidth_stream);
    return 0;
}

static void
IRWinSinc_dealloc(IRWinSinc* self)
{
    pyo_DEALLOC
    free(self->input_tmp);
    free(self->impulse);
    free(self->impulse_tmp);
    IRWinSinc_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
IRWinSinc_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *bandwidthtmp=NULL, *multmp=NULL, *addtmp=NULL;
    IRWinSinc *self;
    self = (IRWinSinc *)type->tp_alloc(type, 0);

    self->last_freq = -1.0;
    self->last_bandwidth = -1.0;
    self->freq = PyFloat_FromDouble(1000.0);
    self->bandwidth = PyFloat_FromDouble(500.0);
    self->filtertype = 0;
    self->order = 256;
    self->count = 0;
    self->changed = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IRWinSinc_compute_next_data_frame);
    self->mode_func_ptr = IRWinSinc_setProcMode;

    static char *kwlist[] = {"input", "freq", "bw", "type", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiiOO", kwlist, &inputtmp, &freqtmp, &bandwidthtmp, &self->filtertype, &self->order, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (bandwidthtmp) {
        PyObject_CallMethod((PyObject *)self, "setBandwidth", "O", bandwidthtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IRWinSinc_alloc_memory(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * IRWinSinc_getServer(IRWinSinc* self) { GET_SERVER };
static PyObject * IRWinSinc_getStream(IRWinSinc* self) { GET_STREAM };
static PyObject * IRWinSinc_setMul(IRWinSinc *self, PyObject *arg) { SET_MUL };
static PyObject * IRWinSinc_setAdd(IRWinSinc *self, PyObject *arg) { SET_ADD };
static PyObject * IRWinSinc_setSub(IRWinSinc *self, PyObject *arg) { SET_SUB };
static PyObject * IRWinSinc_setDiv(IRWinSinc *self, PyObject *arg) { SET_DIV };

static PyObject * IRWinSinc_play(IRWinSinc *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IRWinSinc_out(IRWinSinc *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IRWinSinc_stop(IRWinSinc *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * IRWinSinc_multiply(IRWinSinc *self, PyObject *arg) { MULTIPLY };
static PyObject * IRWinSinc_inplace_multiply(IRWinSinc *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IRWinSinc_add(IRWinSinc *self, PyObject *arg) { ADD };
static PyObject * IRWinSinc_inplace_add(IRWinSinc *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IRWinSinc_sub(IRWinSinc *self, PyObject *arg) { SUB };
static PyObject * IRWinSinc_inplace_sub(IRWinSinc *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IRWinSinc_div(IRWinSinc *self, PyObject *arg) { DIV };
static PyObject * IRWinSinc_inplace_div(IRWinSinc *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
IRWinSinc_setFreq(IRWinSinc *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRWinSinc_setBandwidth(IRWinSinc *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->bandwidth);
	if (isNumber == 1) {
		self->bandwidth = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->bandwidth = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bandwidth, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bandwidth_stream);
        self->bandwidth_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRWinSinc_setType(IRWinSinc *self, PyObject *arg)
{
    ASSERT_ARG_NOT_NULL

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
        self->changed = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef IRWinSinc_members[] = {
    {"server", T_OBJECT_EX, offsetof(IRWinSinc, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IRWinSinc, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(IRWinSinc, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(IRWinSinc, freq), 0, "Cutoff or center frequency."},
    {"bandwidth", T_OBJECT_EX, offsetof(IRWinSinc, bandwidth), 0, "Bandwidth, in Hz, for bandreject and bandpass filters."},
    {"mul", T_OBJECT_EX, offsetof(IRWinSinc, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IRWinSinc, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IRWinSinc_methods[] = {
    {"getServer", (PyCFunction)IRWinSinc_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IRWinSinc_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)IRWinSinc_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IRWinSinc_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IRWinSinc_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setFreq", (PyCFunction)IRWinSinc_setFreq, METH_O, "Sets center/cutoff frequency."},
    {"setBandwidth", (PyCFunction)IRWinSinc_setBandwidth, METH_O, "Sets bandwidth in Hz."},
    {"setType", (PyCFunction)IRWinSinc_setType, METH_O, "Sets filter type factor."},
    {"setMul", (PyCFunction)IRWinSinc_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)IRWinSinc_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)IRWinSinc_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IRWinSinc_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IRWinSinc_as_number = {
    (binaryfunc)IRWinSinc_add,                         /*nb_add*/
    (binaryfunc)IRWinSinc_sub,                         /*nb_subtract*/
    (binaryfunc)IRWinSinc_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)IRWinSinc_inplace_add,                 /*inplace_add*/
    (binaryfunc)IRWinSinc_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)IRWinSinc_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)IRWinSinc_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)IRWinSinc_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject IRWinSincType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.IRWinSinc_base",                                   /*tp_name*/
    sizeof(IRWinSinc),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)IRWinSinc_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &IRWinSinc_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IRWinSinc objects. Windowed-sinc filter.",           /* tp_doc */
    (traverseproc)IRWinSinc_traverse,                  /* tp_traverse */
    (inquiry)IRWinSinc_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    IRWinSinc_methods,                                 /* tp_methods */
    IRWinSinc_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    IRWinSinc_new,                                     /* tp_new */
};

/************/
/* IRAverage */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int modebuffer[2]; // need at least 2 slots for mul & add
    MYFLT *impulse;
    MYFLT *input_tmp;
    int count;
    int order;
    int size;
} IRAverage;

static void
IRAverage_alloc_memory(IRAverage *self) {
    int i;
    MYFLT val, sum;
    if ((self->order % 2) != 0)
        self->order += 1;

    self->size = self->order + 1;

    self->input_tmp = (MYFLT *)realloc(self->input_tmp, self->size * sizeof(MYFLT));
    self->impulse = (MYFLT *)realloc(self->impulse, self->size * sizeof(MYFLT));

    sum = 0.0;
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = 0.0;
        val = 0.42 - 0.5 * MYCOS(TWOPI*i/self->order) + 0.08 * MYCOS(2.0*TWOPI*i/self->order);
        self->impulse[i] = val;
        sum += val;
    }
    for (i=0; i<self->size; i++) {
        self->impulse[i] /= sum;
    }
}

static void
IRAverage_filters(IRAverage *self) {
    int i,j,tmp_count;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * self->impulse[j];
        }

        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void IRAverage_postprocessing_ii(IRAverage *self) { POST_PROCESSING_II };
static void IRAverage_postprocessing_ai(IRAverage *self) { POST_PROCESSING_AI };
static void IRAverage_postprocessing_ia(IRAverage *self) { POST_PROCESSING_IA };
static void IRAverage_postprocessing_aa(IRAverage *self) { POST_PROCESSING_AA };
static void IRAverage_postprocessing_ireva(IRAverage *self) { POST_PROCESSING_IREVA };
static void IRAverage_postprocessing_areva(IRAverage *self) { POST_PROCESSING_AREVA };
static void IRAverage_postprocessing_revai(IRAverage *self) { POST_PROCESSING_REVAI };
static void IRAverage_postprocessing_revaa(IRAverage *self) { POST_PROCESSING_REVAA };
static void IRAverage_postprocessing_revareva(IRAverage *self) { POST_PROCESSING_REVAREVA };

static void
IRAverage_setProcMode(IRAverage *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IRAverage_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = IRAverage_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = IRAverage_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = IRAverage_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = IRAverage_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = IRAverage_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = IRAverage_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = IRAverage_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = IRAverage_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = IRAverage_postprocessing_revareva;
            break;
    }
}

static void
IRAverage_compute_next_data_frame(IRAverage *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
IRAverage_traverse(IRAverage *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int
IRAverage_clear(IRAverage *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
IRAverage_dealloc(IRAverage* self)
{
    pyo_DEALLOC
    free(self->input_tmp);
    free(self->impulse);
    IRAverage_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
IRAverage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *multmp=NULL, *addtmp=NULL;
    IRAverage *self;
    self = (IRAverage *)type->tp_alloc(type, 0);

    self->order = 32;
    self->count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IRAverage_compute_next_data_frame);
    self->mode_func_ptr = IRAverage_setProcMode;

    static char *kwlist[] = {"input", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iOO", kwlist, &inputtmp, &self->order, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IRAverage_alloc_memory(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * IRAverage_getServer(IRAverage* self) { GET_SERVER };
static PyObject * IRAverage_getStream(IRAverage* self) { GET_STREAM };
static PyObject * IRAverage_setMul(IRAverage *self, PyObject *arg) { SET_MUL };
static PyObject * IRAverage_setAdd(IRAverage *self, PyObject *arg) { SET_ADD };
static PyObject * IRAverage_setSub(IRAverage *self, PyObject *arg) { SET_SUB };
static PyObject * IRAverage_setDiv(IRAverage *self, PyObject *arg) { SET_DIV };

static PyObject * IRAverage_play(IRAverage *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IRAverage_out(IRAverage *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IRAverage_stop(IRAverage *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * IRAverage_multiply(IRAverage *self, PyObject *arg) { MULTIPLY };
static PyObject * IRAverage_inplace_multiply(IRAverage *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IRAverage_add(IRAverage *self, PyObject *arg) { ADD };
static PyObject * IRAverage_inplace_add(IRAverage *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IRAverage_sub(IRAverage *self, PyObject *arg) { SUB };
static PyObject * IRAverage_inplace_sub(IRAverage *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IRAverage_div(IRAverage *self, PyObject *arg) { DIV };
static PyObject * IRAverage_inplace_div(IRAverage *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef IRAverage_members[] = {
    {"server", T_OBJECT_EX, offsetof(IRAverage, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IRAverage, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(IRAverage, input), 0, "Input sound object."},
    {"mul", T_OBJECT_EX, offsetof(IRAverage, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IRAverage, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IRAverage_methods[] = {
    {"getServer", (PyCFunction)IRAverage_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IRAverage_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)IRAverage_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IRAverage_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IRAverage_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setMul", (PyCFunction)IRAverage_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)IRAverage_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)IRAverage_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IRAverage_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IRAverage_as_number = {
    (binaryfunc)IRAverage_add,                         /*nb_add*/
    (binaryfunc)IRAverage_sub,                         /*nb_subtract*/
    (binaryfunc)IRAverage_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)IRAverage_inplace_add,                 /*inplace_add*/
    (binaryfunc)IRAverage_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)IRAverage_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)IRAverage_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)IRAverage_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject IRAverageType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.IRAverage_base",                                   /*tp_name*/
    sizeof(IRAverage),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)IRAverage_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &IRAverage_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IRAverage objects. Moving average FIR filter.",           /* tp_doc */
    (traverseproc)IRAverage_traverse,                  /* tp_traverse */
    (inquiry)IRAverage_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    IRAverage_methods,                                 /* tp_methods */
    IRAverage_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    IRAverage_new,                                     /* tp_new */
};

/************/
/* IRPulse */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *bandwidth;
    Stream *bandwidth_stream;
    int modebuffer[4]; // need at least 2 slots for mul & add
    MYFLT *impulse;
    MYFLT *input_tmp;
    int count;
    int filtertype;
    int order;
    int size;
    int changed;
    MYFLT last_freq;
    MYFLT last_bandwidth;
} IRPulse;

static void
IRPulse_alloc_memory(IRPulse *self) {
    int i;
    if ((self->order % 2) != 0)
        self->order += 1;

    self->size = self->order + 1;

    self->input_tmp = (MYFLT *)realloc(self->input_tmp, self->size * sizeof(MYFLT));
    self->impulse = (MYFLT *)realloc(self->impulse, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = self->impulse[i] = 0.0;
    }
}

static void
IRPulse_create_impulse(IRPulse *self, MYFLT freq, MYFLT bandwidth) {
    int i, n, w, bw, dir, gate;
    MYFLT val, sum;

    sum = 0.0;

    if (freq < 1)
        freq = 1.0;
    else if (freq > (self->sr*0.5))
        freq = self->sr*0.5;

    if (bandwidth < 1)
        bandwidth = 1.0;
    else if (bandwidth > (self->sr*0.5))
        bandwidth = self->sr*0.5;

    switch (self->filtertype) {
        case 0:
            /* PULSE */
            w = (int)(self->sr / freq);
            bw = (int)(self->sr / bandwidth);
            for (i=0; i<self->size; i++) {
                if ((i % w) <= bw) {
                    val = 1.0;
                    self->impulse[i] = val;
                    sum += val;
                }
                else {
                    self->impulse[i] = 0.0;
                }

            }
            for (i=0; i<self->size; i++) {
                self->impulse[i] /= sum;
            }
            break;
        case 1:
            /* PULSELP */
            w = (int)(self->sr / freq);
            bw = (int)(self->sr / bandwidth);
            for (i=0; i<self->size; i++) {
                if ((i % w) <= bw) {
                    n = i % w;
                    val = 0.5 * (1.0 - MYCOS(TWOPI*n/(bw-1)));
                    self->impulse[i] = val;
                    sum += val;
                }
                else {
                    self->impulse[i] = 0.0;
                }

            }
            for (i=0; i<self->size; i++) {
                self->impulse[i] /= sum;
            }
            break;
        case 2:
            /* PULSEODD */
            w = (int)(self->sr / (freq*2));
            bw = (int)(self->sr / bandwidth);
            dir = 0;
            gate = 0;
            for (i=0; i<self->size; i++) {
                if ((i % w) <= bw) {
                    if (gate == 1) {
                        dir++;
                        gate = 0;
                    }
                    if ((dir % 2) == 0)
                        val = 1.0;
                    else
                        val = -1.0;
                    self->impulse[i] = val;
                    sum += MYFABS(val);
                }
                else {
                    gate = 1;
                    self->impulse[i] = 0.0;
                }
            }
            for (i=0; i<self->size; i++) {
                self->impulse[i] /= sum;
            }
            break;
        case 3:
            /* PULSEODDLP */
            w = (int)(self->sr / (freq*2));
            bw = (int)(self->sr / bandwidth);
            dir = 0;
            gate = 0;
            for (i=0; i<self->size; i++) {
                if ((i % w) <= bw) {
                    n = i % w;
                    val = 0.5 * (1.0 - MYCOS(TWOPI*n/(bw-1)));
                    if (gate == 1) {
                        dir++;
                        gate = 0;
                    }
                    if ((dir % 2) == 1)
                        val = -val;
                    self->impulse[i] = val;
                    sum += MYFABS(val);
                }
                else {
                    gate = 1;
                    self->impulse[i] = 0.0;
                }
            }
            for (i=0; i<self->size; i++) {
                self->impulse[i] /= sum;
            }
            break;
    }
}

static void
IRPulse_filters(IRPulse *self) {
    int i,j,tmp_count;
    MYFLT freq, bw;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        freq = PyFloat_AS_DOUBLE(self->freq);
    else {
        MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
        freq = fr[0];
    }
    if (self->modebuffer[3] == 0)
        bw = PyFloat_AS_DOUBLE(self->bandwidth);
    else {
        MYFLT *band = Stream_getData((Stream *)self->bandwidth_stream);
        bw = band[0];
    }

    if (freq != self->last_freq || bw != self->last_bandwidth || self->changed == 1) {
        IRPulse_create_impulse(self, freq, bw);
        self->last_freq = freq;
        self->last_bandwidth = bw;
        self->changed = 0;
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * self->impulse[j];
        }

        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void IRPulse_postprocessing_ii(IRPulse *self) { POST_PROCESSING_II };
static void IRPulse_postprocessing_ai(IRPulse *self) { POST_PROCESSING_AI };
static void IRPulse_postprocessing_ia(IRPulse *self) { POST_PROCESSING_IA };
static void IRPulse_postprocessing_aa(IRPulse *self) { POST_PROCESSING_AA };
static void IRPulse_postprocessing_ireva(IRPulse *self) { POST_PROCESSING_IREVA };
static void IRPulse_postprocessing_areva(IRPulse *self) { POST_PROCESSING_AREVA };
static void IRPulse_postprocessing_revai(IRPulse *self) { POST_PROCESSING_REVAI };
static void IRPulse_postprocessing_revaa(IRPulse *self) { POST_PROCESSING_REVAA };
static void IRPulse_postprocessing_revareva(IRPulse *self) { POST_PROCESSING_REVAREVA };

static void
IRPulse_setProcMode(IRPulse *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IRPulse_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = IRPulse_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = IRPulse_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = IRPulse_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = IRPulse_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = IRPulse_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = IRPulse_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = IRPulse_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = IRPulse_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = IRPulse_postprocessing_revareva;
            break;
    }
}

static void
IRPulse_compute_next_data_frame(IRPulse *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
IRPulse_traverse(IRPulse *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->freq);
    Py_VISIT(self->freq_stream);
    Py_VISIT(self->bandwidth);
    Py_VISIT(self->bandwidth_stream);
    return 0;
}

static int
IRPulse_clear(IRPulse *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->freq);
    Py_CLEAR(self->freq_stream);
    Py_CLEAR(self->bandwidth);
    Py_CLEAR(self->bandwidth_stream);
    return 0;
}

static void
IRPulse_dealloc(IRPulse* self)
{
    pyo_DEALLOC
    free(self->input_tmp);
    free(self->impulse);
    IRPulse_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
IRPulse_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *bandwidthtmp=NULL, *multmp=NULL, *addtmp=NULL;
    IRPulse *self;
    self = (IRPulse *)type->tp_alloc(type, 0);

    self->last_freq = -1.0;
    self->last_bandwidth = -1.0;
    self->freq = PyFloat_FromDouble(500.0);
    self->bandwidth = PyFloat_FromDouble(2500.0);
    self->filtertype = 0;
    self->order = 256;
    self->count = 0;
    self->changed = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IRPulse_compute_next_data_frame);
    self->mode_func_ptr = IRPulse_setProcMode;

    static char *kwlist[] = {"input", "freq", "bw", "type", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOiiOO", kwlist, &inputtmp, &freqtmp, &bandwidthtmp, &self->filtertype, &self->order, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }

    if (bandwidthtmp) {
        PyObject_CallMethod((PyObject *)self, "setBandwidth", "O", bandwidthtmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IRPulse_alloc_memory(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * IRPulse_getServer(IRPulse* self) { GET_SERVER };
static PyObject * IRPulse_getStream(IRPulse* self) { GET_STREAM };
static PyObject * IRPulse_setMul(IRPulse *self, PyObject *arg) { SET_MUL };
static PyObject * IRPulse_setAdd(IRPulse *self, PyObject *arg) { SET_ADD };
static PyObject * IRPulse_setSub(IRPulse *self, PyObject *arg) { SET_SUB };
static PyObject * IRPulse_setDiv(IRPulse *self, PyObject *arg) { SET_DIV };

static PyObject * IRPulse_play(IRPulse *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IRPulse_out(IRPulse *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IRPulse_stop(IRPulse *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * IRPulse_multiply(IRPulse *self, PyObject *arg) { MULTIPLY };
static PyObject * IRPulse_inplace_multiply(IRPulse *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IRPulse_add(IRPulse *self, PyObject *arg) { ADD };
static PyObject * IRPulse_inplace_add(IRPulse *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IRPulse_sub(IRPulse *self, PyObject *arg) { SUB };
static PyObject * IRPulse_inplace_sub(IRPulse *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IRPulse_div(IRPulse *self, PyObject *arg) { DIV };
static PyObject * IRPulse_inplace_div(IRPulse *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
IRPulse_setFreq(IRPulse *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRPulse_setBandwidth(IRPulse *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->bandwidth);
	if (isNumber == 1) {
		self->bandwidth = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->bandwidth = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bandwidth, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bandwidth_stream);
        self->bandwidth_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRPulse_setType(IRPulse *self, PyObject *arg)
{

    ASSERT_ARG_NOT_NULL

	int isInt = PyInt_Check(arg);

	if (isInt == 1) {
		self->filtertype = PyInt_AsLong(arg);
        self->changed = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef IRPulse_members[] = {
    {"server", T_OBJECT_EX, offsetof(IRPulse, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IRPulse, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(IRPulse, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(IRPulse, freq), 0, "Cutoff or center frequency."},
    {"bandwidth", T_OBJECT_EX, offsetof(IRPulse, bandwidth), 0, "Bandwidth, in Hz, for bandreject and bandpass filters."},
    {"mul", T_OBJECT_EX, offsetof(IRPulse, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IRPulse, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IRPulse_methods[] = {
    {"getServer", (PyCFunction)IRPulse_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IRPulse_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)IRPulse_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IRPulse_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IRPulse_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setFreq", (PyCFunction)IRPulse_setFreq, METH_O, "Sets center/cutoff frequency."},
    {"setBandwidth", (PyCFunction)IRPulse_setBandwidth, METH_O, "Sets bandwidth in Hz."},
    {"setType", (PyCFunction)IRPulse_setType, METH_O, "Sets filter type factor."},
    {"setMul", (PyCFunction)IRPulse_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)IRPulse_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)IRPulse_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IRPulse_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IRPulse_as_number = {
    (binaryfunc)IRPulse_add,                         /*nb_add*/
    (binaryfunc)IRPulse_sub,                         /*nb_subtract*/
    (binaryfunc)IRPulse_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)IRPulse_inplace_add,                 /*inplace_add*/
    (binaryfunc)IRPulse_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)IRPulse_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)IRPulse_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)IRPulse_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject IRPulseType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.IRPulse_base",                                   /*tp_name*/
    sizeof(IRPulse),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)IRPulse_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &IRPulse_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IRPulse objects. Windowed-sinc filter.",           /* tp_doc */
    (traverseproc)IRPulse_traverse,                  /* tp_traverse */
    (inquiry)IRPulse_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    IRPulse_methods,                                 /* tp_methods */
    IRPulse_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    IRPulse_new,                                     /* tp_new */
};

/************/
/* IRFM */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *carrier;
    Stream *carrier_stream;
    PyObject *ratio;
    Stream *ratio_stream;
    PyObject *index;
    Stream *index_stream;
    int modebuffer[5]; // need at least 2 slots for mul & add
    MYFLT *impulse;
    MYFLT *input_tmp;
    int count;
    int order;
    int size;
    MYFLT last_carrier;
    MYFLT last_ratio;
    MYFLT last_index;
} IRFM;

static void
IRFM_alloc_memory(IRFM *self) {
    int i;
    if ((self->order % 2) != 0)
        self->order += 1;

    self->size = self->order + 1;

    self->input_tmp = (MYFLT *)realloc(self->input_tmp, self->size * sizeof(MYFLT));
    self->impulse = (MYFLT *)realloc(self->impulse, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->input_tmp[i] = self->impulse[i] = 0.0;
    }
}

static void
IRFM_create_impulse(IRFM *self, MYFLT carrier, MYFLT ratio, MYFLT index) {
    int i;
    MYFLT val, sum, invSum, env, mod, fc, bw, modamp;

    if (carrier < 1)
        carrier = 1.0;
    else if (carrier > (self->sr*0.5))
        carrier = self->sr*0.5;

    if (ratio < 0.0001)
        ratio = 0.0001;
    else if (ratio > (self->sr*0.5))
        ratio = self->sr*0.5;

    if (index < 0)
        index = 0.0;

    fc = carrier / self->sr * self->order;
    bw = carrier * ratio / self->sr * self->order;
    modamp = index*TWOPI*bw/self->order;
    sum = 0.0;

    for (i=0; i<self->size; i++) {
        env = 0.5 * (1.0 - MYCOS(TWOPI*i/self->order));
        mod = modamp * MYSIN(TWOPI*bw*i/self->order);
        val = MYSIN(TWOPI*(fc+mod)*i/self->order) * env;
        sum += MYFABS(val);
        self->impulse[i] = val;
    }
    invSum = 1.0 / sum;
    for (i=0; i<self->size; i++) {
        self->impulse[i] *= invSum;
    }
}

static void
IRFM_filters(IRFM *self) {
    int i,j,tmp_count;
    MYFLT carrier, ratio, index;

    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    if (self->modebuffer[2] == 0)
        carrier = PyFloat_AS_DOUBLE(self->carrier);
    else {
        MYFLT *car = Stream_getData((Stream *)self->carrier_stream);
        carrier = car[0];
    }
    if (self->modebuffer[3] == 0)
        ratio = PyFloat_AS_DOUBLE(self->ratio);
    else {
        MYFLT *rat = Stream_getData((Stream *)self->ratio_stream);
        ratio = rat[0];
    }
    if (self->modebuffer[4] == 0)
        index = PyFloat_AS_DOUBLE(self->index);
    else {
        MYFLT *ind = Stream_getData((Stream *)self->index_stream);
        index = ind[0];
    }

    if (carrier != self->last_carrier || ratio != self->last_ratio || index != self->last_index) {
        IRFM_create_impulse(self, carrier, ratio, index);
        self->last_carrier = carrier;
        self->last_ratio = ratio;
        self->last_index = index;
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        tmp_count = self->count;
        for(j=0; j<self->size; j++) {
            if (tmp_count < 0)
                tmp_count += self->size;
            self->data[i] += self->input_tmp[tmp_count--] * self->impulse[j];
        }

        self->count++;
        if (self->count == self->size)
            self->count = 0;
        self->input_tmp[self->count] = in[i];
    }
}

static void IRFM_postprocessing_ii(IRFM *self) { POST_PROCESSING_II };
static void IRFM_postprocessing_ai(IRFM *self) { POST_PROCESSING_AI };
static void IRFM_postprocessing_ia(IRFM *self) { POST_PROCESSING_IA };
static void IRFM_postprocessing_aa(IRFM *self) { POST_PROCESSING_AA };
static void IRFM_postprocessing_ireva(IRFM *self) { POST_PROCESSING_IREVA };
static void IRFM_postprocessing_areva(IRFM *self) { POST_PROCESSING_AREVA };
static void IRFM_postprocessing_revai(IRFM *self) { POST_PROCESSING_REVAI };
static void IRFM_postprocessing_revaa(IRFM *self) { POST_PROCESSING_REVAA };
static void IRFM_postprocessing_revareva(IRFM *self) { POST_PROCESSING_REVAREVA };

static void
IRFM_setProcMode(IRFM *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IRFM_filters;

	switch (muladdmode) {
        case 0:
            self->muladd_func_ptr = IRFM_postprocessing_ii;
            break;
        case 1:
            self->muladd_func_ptr = IRFM_postprocessing_ai;
            break;
        case 2:
            self->muladd_func_ptr = IRFM_postprocessing_revai;
            break;
        case 10:
            self->muladd_func_ptr = IRFM_postprocessing_ia;
            break;
        case 11:
            self->muladd_func_ptr = IRFM_postprocessing_aa;
            break;
        case 12:
            self->muladd_func_ptr = IRFM_postprocessing_revaa;
            break;
        case 20:
            self->muladd_func_ptr = IRFM_postprocessing_ireva;
            break;
        case 21:
            self->muladd_func_ptr = IRFM_postprocessing_areva;
            break;
        case 22:
            self->muladd_func_ptr = IRFM_postprocessing_revareva;
            break;
    }
}

static void
IRFM_compute_next_data_frame(IRFM *self)
{
    (*self->proc_func_ptr)(self);
    (*self->muladd_func_ptr)(self);
}

static int
IRFM_traverse(IRFM *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->carrier);
    Py_VISIT(self->carrier_stream);
    Py_VISIT(self->ratio);
    Py_VISIT(self->ratio_stream);
    Py_VISIT(self->index);
    Py_VISIT(self->index_stream);
    return 0;
}

static int
IRFM_clear(IRFM *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->carrier);
    Py_CLEAR(self->carrier_stream);
    Py_CLEAR(self->ratio);
    Py_CLEAR(self->ratio_stream);
    Py_CLEAR(self->index);
    Py_CLEAR(self->index_stream);
    return 0;
}

static void
IRFM_dealloc(IRFM* self)
{
    pyo_DEALLOC
    free(self->input_tmp);
    free(self->impulse);
    IRFM_clear(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
IRFM_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *carriertmp=NULL, *ratiotmp=NULL, *indextmp=NULL, *multmp=NULL, *addtmp=NULL;
    IRFM *self;
    self = (IRFM *)type->tp_alloc(type, 0);

    self->last_carrier = -1.0;
    self->last_ratio = -1.0;
    self->last_index = -1.0;
    self->carrier = PyFloat_FromDouble(1000.0);
    self->ratio = PyFloat_FromDouble(0.5);
    self->index = PyFloat_FromDouble(3.0);
    self->order = 256;
    self->count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IRFM_compute_next_data_frame);
    self->mode_func_ptr = IRFM_setProcMode;

    static char *kwlist[] = {"input", "carrier", "ratio", "index", "order", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOiOO", kwlist, &inputtmp, &carriertmp, &ratiotmp, &indextmp, &self->order, &multmp, &addtmp))
        Py_RETURN_NONE;

    INIT_INPUT_STREAM

    if (carriertmp) {
        PyObject_CallMethod((PyObject *)self, "setCarrier", "O", carriertmp);
    }

    if (ratiotmp) {
        PyObject_CallMethod((PyObject *)self, "setRatio", "O", ratiotmp);
    }

    if (indextmp) {
        PyObject_CallMethod((PyObject *)self, "setIndex", "O", indextmp);
    }

    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }

    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IRFM_alloc_memory(self);

    (*self->mode_func_ptr)(self);

    return (PyObject *)self;
}

static PyObject * IRFM_getServer(IRFM* self) { GET_SERVER };
static PyObject * IRFM_getStream(IRFM* self) { GET_STREAM };
static PyObject * IRFM_setMul(IRFM *self, PyObject *arg) { SET_MUL };
static PyObject * IRFM_setAdd(IRFM *self, PyObject *arg) { SET_ADD };
static PyObject * IRFM_setSub(IRFM *self, PyObject *arg) { SET_SUB };
static PyObject * IRFM_setDiv(IRFM *self, PyObject *arg) { SET_DIV };

static PyObject * IRFM_play(IRFM *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IRFM_out(IRFM *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IRFM_stop(IRFM *self, PyObject *args, PyObject *kwds) { STOP };

static PyObject * IRFM_multiply(IRFM *self, PyObject *arg) { MULTIPLY };
static PyObject * IRFM_inplace_multiply(IRFM *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IRFM_add(IRFM *self, PyObject *arg) { ADD };
static PyObject * IRFM_inplace_add(IRFM *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IRFM_sub(IRFM *self, PyObject *arg) { SUB };
static PyObject * IRFM_inplace_sub(IRFM *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IRFM_div(IRFM *self, PyObject *arg) { DIV };
static PyObject * IRFM_inplace_div(IRFM *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
IRFM_setCarrier(IRFM *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->carrier);
	if (isNumber == 1) {
		self->carrier = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->carrier = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->carrier, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->carrier_stream);
        self->carrier_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRFM_setRatio(IRFM *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->ratio);
	if (isNumber == 1) {
		self->ratio = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->ratio = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->ratio, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->ratio_stream);
        self->ratio_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IRFM_setIndex(IRFM *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;

    ASSERT_ARG_NOT_NULL

	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->index);
	if (isNumber == 1) {
		self->index = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->index = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->index, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->index_stream);
        self->index_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef IRFM_members[] = {
    {"server", T_OBJECT_EX, offsetof(IRFM, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IRFM, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(IRFM, input), 0, "Input sound object."},
    {"carrier", T_OBJECT_EX, offsetof(IRFM, carrier), 0, "Carrier frequency."},
    {"ratio", T_OBJECT_EX, offsetof(IRFM, ratio), 0, "Modulator / carrier ratio."},
    {"index", T_OBJECT_EX, offsetof(IRFM, index), 0, "Modulation index."},
    {"mul", T_OBJECT_EX, offsetof(IRFM, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IRFM, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IRFM_methods[] = {
    {"getServer", (PyCFunction)IRFM_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IRFM_getStream, METH_NOARGS, "Returns stream object."},
    {"play", (PyCFunction)IRFM_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IRFM_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IRFM_stop, METH_VARARGS|METH_KEYWORDS, "Stops computing."},
    {"setCarrier", (PyCFunction)IRFM_setCarrier, METH_O, "Sets carrier frequency."},
    {"setRatio", (PyCFunction)IRFM_setRatio, METH_O, "Sets ratio."},
    {"setIndex", (PyCFunction)IRFM_setIndex, METH_O, "Sets the modulation index."},
    {"setMul", (PyCFunction)IRFM_setMul, METH_O, "Sets mul factor."},
    {"setAdd", (PyCFunction)IRFM_setAdd, METH_O, "Sets add factor."},
    {"setSub", (PyCFunction)IRFM_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IRFM_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IRFM_as_number = {
    (binaryfunc)IRFM_add,                         /*nb_add*/
    (binaryfunc)IRFM_sub,                         /*nb_subtract*/
    (binaryfunc)IRFM_multiply,                    /*nb_multiply*/
    INITIALIZE_NB_DIVIDE_ZERO                       /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    INITIALIZE_NB_COERCE_ZERO                       /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    INITIALIZE_NB_OCT_ZERO                          /*nb_oct*/
    INITIALIZE_NB_HEX_ZERO                          /*nb_hex*/
    (binaryfunc)IRFM_inplace_add,                 /*inplace_add*/
    (binaryfunc)IRFM_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)IRFM_inplace_multiply,            /*inplace_multiply*/
    INITIALIZE_NB_IN_PLACE_DIVIDE_ZERO                                           /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    (binaryfunc)IRFM_div,                       /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    (binaryfunc)IRFM_inplace_div,                       /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject IRFMType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_pyo.IRFM_base",                                   /*tp_name*/
    sizeof(IRFM),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)IRFM_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_as_async (tp_compare in Python 2)*/
    0,                                              /*tp_repr*/
    &IRFM_as_number,                              /*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IRFM objects. Convolve an input signal with a frequency modulation spectrum.",           /* tp_doc */
    (traverseproc)IRFM_traverse,                  /* tp_traverse */
    (inquiry)IRFM_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    IRFM_methods,                                 /* tp_methods */
    IRFM_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    0,                          /* tp_init */
    0,                                              /* tp_alloc */
    IRFM_new,                                     /* tp_new */
};