#include "Python.h"

#ifdef __TABLE_MODULE

typedef struct {
    PyObject_HEAD
    int size;
    float *data;
} TableStream;


#define MAKE_NEW_TABLESTREAM(self, type, rt_error)	\
(self) = (TableStream *)(type)->tp_alloc((type), 0);	\
if ((self) == rt_error) { return rt_error; }	\
\
(self)->size = 0

#else

int TableStream_getSize(PyObject *self);
float * TableStream_getData(PyObject *self);
extern PyTypeObject TableStreamType;

/* HarmTable */
/* Function exposed to other modules */
static PyObject * HarmTable_getTableStream(PyObject *self);

/* Object initialisation */
static int HarmTable_traverse(PyObject *self, visitproc visit, void *arg);
static void HarmTable_dealloc(PyObject* self);
static PyObject * HarmTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int HarmTable_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject HarmTableType;

/* Functions exposed to Python interpreter */
static PyObject * HarmTable_getSize(PyObject *self);
static PyObject * HarmTable_getServer(PyObject *self);
static PyObject * HarmTable_setSize(PyObject *self, PyObject *value);
static PyObject * HarmTable_replace(PyObject *self, PyObject *value);


/* HannTable */
/* Function exposed to other modules */
static PyObject * HannTable_getTableStream(PyObject *self);

/* Object initialisation */
static int HannTable_traverse(PyObject *self, visitproc visit, void *arg);
static void HannTable_dealloc(PyObject* self);
static PyObject * HannTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int HannTable_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject HannTableType;

/* Functions exposed to Python interpreter */
static PyObject * HannTable_getSize(PyObject *self);
static PyObject * HannTable_getServer(PyObject *self);
static PyObject * HannTable_setSize(PyObject *self, PyObject *value);


/* SndTable */
/* Function exposed to other modules */
static PyObject * SndTable_getTableStream(PyObject *self);

/* Object initialisation */
static int SndTable_traverse(PyObject *self, visitproc visit, void *arg);
static void SndTable_dealloc(PyObject* self);
static PyObject * SndTable_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int SndTable_init(PyObject *self, PyObject *args, PyObject *kwds);
extern PyTypeObject SndTableType;

/* Functions exposed to Python interpreter */
static PyObject * SndTable_getSize(PyObject *self);
static PyObject * SndTable_getRate(PyObject *self);
static PyObject * SndTable_getServer(PyObject *self);

#endif