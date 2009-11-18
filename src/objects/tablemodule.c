#include <Python.h>
#include "structmember.h"
#include <math.h>
#include "pyomodule.h"
#include "servermodule.h"
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
    factor = 1. / (self->size * 0.5) * Py_MATH_PI;
    
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
    PyObject *amplist=NULL;
    
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
