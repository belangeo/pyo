#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"

#define __TABLE_MODULE
#include "tablemodule.h"
#undef __TABLE_MODULE

/*************************/
/* TableStream structure */
/*************************/
static void
TableStream_dealloc(TableStream* self)
{
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
TableStream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TableStream *self;
    MAKE_NEW_TABLESTREAM(self, type, NULL);
    return (PyObject *)self;
}

int
TableStream_getSize(TableStream *self)
{
    return self->size;
}

float *
TableStream_getData(TableStream *self)
{
    return (float *)self->data;
}    

void
TableStream_setData(TableStream *self, float *data)
{
    self->data = data;
}    

void
TableStream_setSize(TableStream *self, int size)
{
    self->size = size;
}    

PyTypeObject TableStreamType = {
PyObject_HEAD_INIT(NULL)
0, /*ob_size*/
"_pyo.TableStream", /*tp_name*/
sizeof(TableStream), /*tp_basicsize*/
0, /*tp_itemsize*/
(destructor)TableStream_dealloc, /*tp_dealloc*/
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
"TableStream objects. For internal use only. Must never be instantiated by the user.", /* tp_doc */
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
TableStream_new, /* tp_new */
};



/***********************/
/* HarmTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *amplist;
} HarmTable;

static void
HarmTable_generate(HarmTable *self) {
    int i, j, ampsize;
    float factor, amplitude, val;
    
    ampsize = PyList_Size(self->amplist);
    factor = 1. / (self->size * 0.5) * PI;
    
    for(i=0; i<self->size; i++) {
        val = 0;
        for(j=0; j<ampsize; j++) {
            amplitude = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(self->amplist, j)));
            if (amplitude != 0.0) {
                val += sinf((j+1) * i * factor) * amplitude;
            }
        }
        self->data[i] = val;
    }
    val = self->data[0];
    self->data[self->size+1] = val;  
    TableStream_setData(self->tablestream, self->data);
}

static int
HarmTable_traverse(HarmTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->amplist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
HarmTable_clear(HarmTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->amplist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
HarmTable_dealloc(HarmTable* self)
{
    free(self->data);
    HarmTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
HarmTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HarmTable *self;
    
    self = (HarmTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->amplist = PyList_New(0);
    PyList_Append(self->amplist, PyFloat_FromDouble(1.));
    self->size = 8192;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);

    return (PyObject *)self;
}

static int
HarmTable_init(HarmTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *amplist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &amplist, &self->size))
        return -1; 
    
    if (amplist) {
        Py_INCREF(amplist);
        Py_DECREF(self->amplist);
        self->amplist = amplist;
    }

    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);
    HarmTable_generate(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * HarmTable_getServer(HarmTable* self) { GET_SERVER };
static PyObject * HarmTable_getTableStream(HarmTable* self) { GET_TABLE_STREAM };

static PyObject *
HarmTable_setSize(HarmTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 

    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);
    
    HarmTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
HarmTable_getSize(HarmTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
HarmTable_replace(HarmTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->amplist);
    self->amplist = value; 
    
    HarmTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef HarmTable_members[] = {
{"server", T_OBJECT_EX, offsetof(HarmTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(HarmTable, tablestream), 0, "Table stream object."},
{"amplist", T_OBJECT_EX, offsetof(HarmTable, amplist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef HarmTable_methods[] = {
{"getServer", (PyCFunction)HarmTable_getServer, METH_NOARGS, "Returns server object."},
{"getTableStream", (PyCFunction)HarmTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setSize", (PyCFunction)HarmTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HarmTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"replace", (PyCFunction)HarmTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject HarmTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.HarmTable_base",         /*tp_name*/
sizeof(HarmTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HarmTable_dealloc, /*tp_dealloc*/
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
"HarmTable objects. Generates a table filled with a waveform whose harmonic content correspond to a given amplitude list values.",  /* tp_doc */
(traverseproc)HarmTable_traverse,   /* tp_traverse */
(inquiry)HarmTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HarmTable_methods,             /* tp_methods */
HarmTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)HarmTable_init,      /* tp_init */
0,                         /* tp_alloc */
HarmTable_new,                 /* tp_new */
};

/***********************/
/* HannTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
} HannTable;

static void
HannTable_generate(HannTable *self) {
    int i, halfSize;
    float val;
    
    halfSize = self->size / 2 + 1;
    
    for(i=0; i<self->size; i++) {
        val = 0.5 + (cosf(TWOPI * (i - halfSize) / self->size) * 0.5);
        self->data[i] = val;
    }
    val = self->data[0];
    self->data[self->size+1] = val;  
    TableStream_setData(self->tablestream, self->data);
}

static int
HannTable_traverse(HannTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
HannTable_clear(HannTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
HannTable_dealloc(HannTable* self)
{
    free(self->data);
    HannTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
HannTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    HannTable *self;
    
    self = (HannTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
HannTable_init(HannTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &self->size))
        return -1; 
    
    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);
    HannTable_generate(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * HannTable_getServer(HannTable* self) { GET_SERVER };
static PyObject * HannTable_getTableStream(HannTable* self) { GET_TABLE_STREAM };

static PyObject *
HannTable_setSize(HannTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }
    
    self->size = PyInt_AsLong(value); 
    
    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);
    
    HannTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
HannTable_getSize(HannTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyMemberDef HannTable_members[] = {
{"server", T_OBJECT_EX, offsetof(HannTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(HannTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef HannTable_methods[] = {
{"getServer", (PyCFunction)HannTable_getServer, METH_NOARGS, "Returns server object."},
{"getTableStream", (PyCFunction)HannTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setSize", (PyCFunction)HannTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)HannTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{NULL}  /* Sentinel */
};

PyTypeObject HannTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.HannTable_base",         /*tp_name*/
sizeof(HannTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)HannTable_dealloc, /*tp_dealloc*/
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
"HannTable objects. Generates a table filled with a waveform whose harmonic content correspond to a given amplitude list values.",  /* tp_doc */
(traverseproc)HannTable_traverse,   /* tp_traverse */
(inquiry)HannTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
HannTable_methods,             /* tp_methods */
HannTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)HannTable_init,      /* tp_init */
0,                         /* tp_alloc */
HannTable_new,                 /* tp_new */
};

/***********************/
/* LinTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    PyObject *pointslist;
} LinTable;

static void
LinTable_generate(LinTable *self) {
    Py_ssize_t i, j, steps;
    Py_ssize_t listsize;
    PyObject *tup, *tup2;
    int x1, y1;
    float x2, y2, diff;
    
    listsize = PyList_Size(self->pointslist);
    
    for(i=0; i<(listsize-1); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));
        tup2 = PyList_GET_ITEM(self->pointslist, i+1);
        y1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup2, 0)));
        y2 = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup2, 1)));
        steps = y1 - x1;
        diff = (y2 - x2) / steps;
        for(j=0; j<steps; j++) {
            self->data[x1+j] = x2 + diff * j;
        }
    }
    if (y1 < (self->size-1)) {
        self->data[y1] = y2;
        for (i=y1; i<self->size; i++) {
            self->data[i+1] = 0.0;
        }
        self->data[self->size+1] = 0.0;
    }
    else {
        self->data[self->size] = y2;
        self->data[self->size+1] = y2;
    }    
    TableStream_setData(self->tablestream, self->data);
}

static int
LinTable_traverse(LinTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->pointslist);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
LinTable_clear(LinTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->pointslist);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
LinTable_dealloc(LinTable* self)
{
    free(self->data);
    LinTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
LinTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LinTable *self;
    
    self = (LinTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointslist = PyList_New(0);
    self->size = 8192;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
LinTable_init(LinTable *self, PyObject *args, PyObject *kwds)
{
    PyObject *pointslist=NULL;
    
    static char *kwlist[] = {"list", "size", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Oi", kwlist, &pointslist, &self->size))
        return -1; 
    
    if (pointslist) {
        Py_INCREF(pointslist);
        Py_DECREF(self->pointslist);
        self->pointslist = pointslist;
    }
    else {
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(0), PyFloat_FromDouble(0.)));
        PyList_Append(self->pointslist, PyTuple_Pack(2, PyInt_FromLong(self->size), PyFloat_FromDouble(1.)));
    }
    
    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);
    LinTable_generate(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * LinTable_getServer(LinTable* self) { GET_SERVER };
static PyObject * LinTable_getTableStream(LinTable* self) { GET_TABLE_STREAM };

static PyObject *
LinTable_setSize(LinTable *self, PyObject *value)
{
    Py_ssize_t i;
    PyObject *tup, *x2;
    int old_size, x1;
    float factor;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the size attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyInt_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The size attribute value must be an integer.");
        return PyInt_FromLong(-1);
    }

    old_size = self->size;
    self->size = PyInt_AsLong(value); 
    
    factor = (float)(self->size) / old_size;
    
    self->data = (float *)realloc(self->data, (self->size+1) * sizeof(float));
    TableStream_setSize(self->tablestream, self->size);

    Py_ssize_t listsize = PyList_Size(self->pointslist);

    PyObject *listtemp = PyList_New(0);
    
    for(i=0; i<(listsize); i++) {
        tup = PyList_GET_ITEM(self->pointslist, i);
        x1 = PyInt_AsLong(PyNumber_Long(PyTuple_GET_ITEM(tup, 0)));
        x2 = PyNumber_Float(PyTuple_GET_ITEM(tup, 1));
        PyList_Append(listtemp, PyTuple_Pack(2, PyInt_FromLong((int)(x1*factor)), x2));
    }
    
    Py_INCREF(listtemp);
    Py_DECREF(self->pointslist);
    self->pointslist = listtemp;
    
    LinTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
LinTable_getSize(LinTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
LinTable_getPoints(LinTable *self)
{
    Py_INCREF(self->pointslist);
    return self->pointslist;
};

static PyObject *
LinTable_replace(LinTable *self, PyObject *value)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the list attribute.");
        return PyInt_FromLong(-1);
    }
    
    if (! PyList_Check(value)) {
        PyErr_SetString(PyExc_TypeError, "The amplitude list attribute value must be a list of tuples.");
        return PyInt_FromLong(-1);
    }
    
    Py_INCREF(value);
    Py_DECREF(self->pointslist);
    self->pointslist = value; 
    
    LinTable_generate(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef LinTable_members[] = {
{"server", T_OBJECT_EX, offsetof(LinTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(LinTable, tablestream), 0, "Table stream object."},
{"pointslist", T_OBJECT_EX, offsetof(LinTable, pointslist), 0, "Harmonics amplitude values."},
{NULL}  /* Sentinel */
};

static PyMethodDef LinTable_methods[] = {
{"getServer", (PyCFunction)LinTable_getServer, METH_NOARGS, "Returns server object."},
{"getTableStream", (PyCFunction)LinTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setSize", (PyCFunction)LinTable_setSize, METH_O, "Sets the size of the table in samples"},
{"getSize", (PyCFunction)LinTable_getSize, METH_NOARGS, "Return the size of the table in samples"},
{"getPoints", (PyCFunction)LinTable_getPoints, METH_NOARGS, "Return the list of points."},
{"replace", (PyCFunction)LinTable_replace, METH_O, "Sets the harmonics amplitude list and generates a new waveform table."},
{NULL}  /* Sentinel */
};

PyTypeObject LinTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.LinTable_base",         /*tp_name*/
sizeof(LinTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)LinTable_dealloc, /*tp_dealloc*/
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
"LinTable objects. Generates a table filled with one or more straight lines.",  /* tp_doc */
(traverseproc)LinTable_traverse,   /* tp_traverse */
(inquiry)LinTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
LinTable_methods,             /* tp_methods */
LinTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)LinTable_init,      /* tp_init */
0,                         /* tp_alloc */
LinTable_new,                 /* tp_new */
};

/***********************/
/* SndTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    char *path;
    int sndSr;
    int chnl;
} SndTable;

static void
SndTable_loadSound(SndTable *self) {
    SNDFILE *sf;
    SF_INFO info;
    int i, num, num_items, num_chnls;
    float val;
        
    /* Open the WAV file. */
    info.format = 0;
    sf = sf_open(self->path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("Failed to open the file.\n");
    }
    /* Print some of the info, and figure out how much data to read. */
    self->size = info.frames;
    self->sndSr = info.samplerate;
    num_chnls = info.channels;
    printf("samples = %d\n", self->size);
    printf("samplingrate = %d\n", self->sndSr);
    printf("channels = %d\n", num_chnls);
    num_items = self->size * num_chnls;
    //printf("num_items=%d\n",num_items);
    /* Allocate space for the data to be read, then read it. */
    self->data = (float *)realloc(self->data, (self->size + 1) * sizeof(float));
    float tmp[num_items];
    num = sf_read_float(sf, tmp, num_items);
    sf_close(sf);
    for (i=0; i<num_items; i++) {
        if ((i % num_chnls) == self->chnl) {
            self->data[(int)(i/num_chnls)] = tmp[i];
        }    
    }
    //printf("Read %d items\n",num);
    val = self->data[0];
    self->data[self->size+1] = val;  

    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);
}

static int
SndTable_traverse(SndTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
SndTable_clear(SndTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
SndTable_dealloc(SndTable* self)
{
    free(self->data);
    SndTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
SndTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    SndTable *self;
    
    self = (SndTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->chnl = 0;

    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
SndTable_init(SndTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"path", "chnl", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, &self->path, &self->chnl))
        return -1; 
    
    SndTable_loadSound(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * SndTable_getServer(SndTable* self) { GET_SERVER };
static PyObject * SndTable_getTableStream(SndTable* self) { GET_TABLE_STREAM };

static PyObject *
SndTable_setSound(SndTable *self, PyObject *args, PyObject *kwds)
{    
    static char *kwlist[] = {"path", "chnl", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "si", kwlist, &self->path, &self->chnl)) {
        Py_INCREF(Py_None);
        return Py_None;
    }    
    
    SndTable_loadSound(self);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
SndTable_getSize(SndTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
SndTable_getRate(SndTable *self)
{
    float sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr * (self->sndSr/sr) / self->size);
};

static PyMemberDef SndTable_members[] = {
{"server", T_OBJECT_EX, offsetof(SndTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(SndTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef SndTable_methods[] = {
{"getServer", (PyCFunction)SndTable_getServer, METH_NOARGS, "Returns server object."},
{"getTableStream", (PyCFunction)SndTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"setSound", (PyCFunction)SndTable_setSound, METH_VARARGS, "Load a new sound in the table."},
{"getSize", (PyCFunction)SndTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getRate", (PyCFunction)SndTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject SndTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.SndTable_base",         /*tp_name*/
sizeof(SndTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)SndTable_dealloc, /*tp_dealloc*/
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
"SndTable objects. Generates a table filled with a soundfile.",  /* tp_doc */
(traverseproc)SndTable_traverse,   /* tp_traverse */
(inquiry)SndTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
SndTable_methods,             /* tp_methods */
SndTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)SndTable_init,      /* tp_init */
0,                         /* tp_alloc */
SndTable_new,                 /* tp_new */
};

/***********************/
/* NewTable structure */
/***********************/
typedef struct {
    pyo_table_HEAD
    float length;
    int pointer;
} NewTable;

static PyObject *
NewTable_recordChunk(NewTable *self, float *data, int datasize)
{
    int i;

    for (i=0; i<datasize; i++) {
        self->data[self->pointer++] = data[i];
        if (self->pointer == self->size)
            self->pointer = 0;
    }    
    Py_INCREF(Py_None);
    return Py_None;
}

static int
NewTable_traverse(NewTable *self, visitproc visit, void *arg)
{
    Py_VISIT(self->server);
    Py_VISIT(self->tablestream);
    return 0;
}

static int 
NewTable_clear(NewTable *self)
{
    Py_CLEAR(self->server);
    Py_CLEAR(self->tablestream);
    return 0;
}

static void
NewTable_dealloc(NewTable* self)
{
    free(self->data);
    NewTable_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
NewTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NewTable *self;
    
    self = (NewTable *)type->tp_alloc(type, 0);
    
    self->server = PyServer_get_server();
    
    self->pointer = 0;
    
    MAKE_NEW_TABLESTREAM(self->tablestream, &TableStreamType, NULL);
    
    return (PyObject *)self;
}

static int
NewTable_init(NewTable *self, PyObject *args, PyObject *kwds)
{    
    int i;
    static char *kwlist[] = {"length", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "f|i", kwlist, &self->length))
        return -1; 

    float sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    self->size = (int)(self->length * sr + 0.5);
    self->data = (float *)realloc(self->data, (self->size + 1) * sizeof(float));

    for (i=0; i<(self->size+1); i++) {
        self->data[i] = 0.;
    }
    
    TableStream_setSize(self->tablestream, self->size);
    TableStream_setData(self->tablestream, self->data);

    Py_INCREF(self);
    return 0;
}

static PyObject * NewTable_getServer(NewTable* self) { GET_SERVER };
static PyObject * NewTable_getTableStream(NewTable* self) { GET_TABLE_STREAM };

static PyObject *
NewTable_getSize(NewTable *self)
{
    return PyInt_FromLong(self->size);
};

static PyObject *
NewTable_getLength(NewTable *self)
{
    return PyFloat_FromDouble(self->length);
};

static PyObject *
NewTable_getRate(NewTable *self)
{
    float sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    return PyFloat_FromDouble(sr / self->size);
};

static PyMemberDef NewTable_members[] = {
{"server", T_OBJECT_EX, offsetof(NewTable, server), 0, "Pyo server."},
{"tablestream", T_OBJECT_EX, offsetof(NewTable, tablestream), 0, "Table stream object."},
{NULL}  /* Sentinel */
};

static PyMethodDef NewTable_methods[] = {
{"getServer", (PyCFunction)NewTable_getServer, METH_NOARGS, "Returns server object."},
{"getTableStream", (PyCFunction)NewTable_getTableStream, METH_NOARGS, "Returns table stream object created by this table."},
{"getSize", (PyCFunction)NewTable_getSize, METH_NOARGS, "Return the size of the table in samples."},
{"getLength", (PyCFunction)NewTable_getLength, METH_NOARGS, "Return the length of the table in seconds."},
{"getRate", (PyCFunction)NewTable_getRate, METH_NOARGS, "Return the frequency (in cps) that reads the sound without pitch transposition."},
{NULL}  /* Sentinel */
};

PyTypeObject NewTableType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.NewTable_base",         /*tp_name*/
sizeof(NewTable),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)NewTable_dealloc, /*tp_dealloc*/
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
"NewTable objects. Generates an empty table.",  /* tp_doc */
(traverseproc)NewTable_traverse,   /* tp_traverse */
(inquiry)NewTable_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
NewTable_methods,             /* tp_methods */
NewTable_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)NewTable_init,      /* tp_init */
0,                         /* tp_alloc */
NewTable_new,                 /* tp_new */
};

/******************************/
/* TableRec object definition */
/******************************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    NewTable *table;
    int pointer;
    float fadetime;
    float fadeInSample;
} TableRec;

static void
TableRec_compute_next_data_frame(TableRec *self)
{
    int i, num, upBound;
    float sclfade, val;
    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    
    if ((size - self->pointer) >= self->bufsize)
        num = self->bufsize;
    else
        num = size - self->pointer;
    
    if (self->pointer < size) {   
        sclfade = 1. / self->fadetime;
        upBound = size - self->fadeInSample;
        
        float buffer[num];
        memset(&buffer, 0, sizeof(buffer));
        float *in = Stream_getData((Stream *)self->input_stream);
        
        for (i=0; i<num; i++) {
            if (self->pointer < self->fadeInSample)
                val = self->pointer / self->fadeInSample;
            else if (self->pointer > upBound)
                val = (size - self->pointer) / self->fadeInSample;
            else
                val = 1.;
            buffer[i] = in[i] * val;
            self->pointer++;
        }
        
        NewTable_recordChunk((NewTable *)self->table, buffer, num);
    } 
}

static int
TableRec_traverse(TableRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->table);
    return 0;
}

static int 
TableRec_clear(TableRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->table);
    return 0;
}

static void
TableRec_dealloc(TableRec* self)
{
    free(self->data);
    TableRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * TableRec_deleteStream(TableRec *self) { DELETE_STREAM };

static PyObject *
TableRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    TableRec *self;
    self = (TableRec *)type->tp_alloc(type, 0);
    
    self->pointer = 0;
    self->fadetime = 0.;
    
    INIT_OBJECT_COMMON

    Stream_setFunctionPtr(self->stream, TableRec_compute_next_data_frame);
    Stream_setStreamActive(self->stream, 0);

    return (PyObject *)self;
}

static int
TableRec_init(TableRec *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *inputtmp, *input_streamtmp, *tabletmp;
    
    static char *kwlist[] = {"input", "table", "fadetime", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|f", kwlist, &inputtmp, &tabletmp, &self->fadetime))
        return -1; 
    
    Py_XDECREF(self->input);
    self->input = inputtmp;
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL);
    Py_INCREF(input_streamtmp);
    Py_XDECREF(self->input_stream);
    self->input_stream = (Stream *)input_streamtmp;
    
    Py_XDECREF(self->table);
    self->table = (NewTable *)tabletmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    int size = PyInt_AsLong(NewTable_getSize((NewTable *)self->table));
    if ((self->fadetime * self->sr) > (size * 0.5))
        self->fadetime = size * 0.5 / self->sr;
    self->fadeInSample = roundf(self->fadetime * self->sr + 0.5);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.;
    }    
    
    Py_INCREF(self);
    return 0;
}

static PyObject * TableRec_getServer(TableRec* self) { GET_SERVER };
static PyObject * TableRec_getStream(TableRec* self) { GET_STREAM };

static PyObject * TableRec_play(TableRec *self) 
{ 
    self->pointer = 0;
    PLAY 
};

static PyObject * TableRec_stop(TableRec *self) { STOP };

static PyObject *
TableRec_setTable(TableRec *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
    Py_INCREF(tmp);
	Py_DECREF(self->table);
    self->table = (NewTable *)tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef TableRec_members[] = {
{"server", T_OBJECT_EX, offsetof(TableRec, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(TableRec, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(TableRec, input), 0, "Input sound object."},
{"table", T_OBJECT_EX, offsetof(TableRec, table), 0, "Table to record in."},
{NULL}  /* Sentinel */
};

static PyMethodDef TableRec_methods[] = {
{"getServer", (PyCFunction)TableRec_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)TableRec_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)TableRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"setTable", (PyCFunction)TableRec_setTable, METH_O, "Sets a new table."},
{"play", (PyCFunction)TableRec_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)TableRec_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject TableRecType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.TableRec_base",         /*tp_name*/
sizeof(TableRec),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)TableRec_dealloc, /*tp_dealloc*/
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
"TableRec objects. Record audio input in a table object.",           /* tp_doc */
(traverseproc)TableRec_traverse,   /* tp_traverse */
(inquiry)TableRec_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
TableRec_methods,             /* tp_methods */
TableRec_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)TableRec_init,      /* tp_init */
0,                         /* tp_alloc */
TableRec_new,                 /* tp_new */
};

