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

#endif
