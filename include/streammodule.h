#include <Python.h>

typedef struct {
    PyObject_HEAD
    PyObject *streamobject;
    void (*funcptr)();
    int chnl;
    int active;
    int todac;
    float *data;
} Stream;

static void Stream_dealloc(Stream* self);
static PyObject * Stream_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int Stream_init(Stream *self, PyObject *args, PyObject *kwds);
extern PyObject * Stream_getStreamObject(Stream *self);
extern int Stream_getStreamActive(Stream *self);
extern int Stream_getStreamChnl(Stream *self);
extern int Stream_getStreamToDac(Stream *self);
extern float * Stream_getData(Stream *self);
extern void Stream_setData(Stream * self, float *data);
extern void Stream_setFunctionPtr(Stream *self, void *ptr);
extern void Stream_callFunction(Stream *self);
extern PyTypeObject StreamType;

#define MAKE_NEW_STREAM(self, type, rt_error)	\
  (self) = (Stream *)(type)->tp_alloc((type), 0);	\
  if ((self) == rt_error) { return rt_error; }	\
						\
  (self)->chnl = (self)->active = (self)->todac = 0


#ifdef __STREAM_MODULE
/* include from stream.c */

#else
/* include from other modules to use API */

#define Stream_setStreamObject(op, v) (((Stream *)(op))->streamobject = (v))
#define Stream_setStreamChnl(op, v) (((Stream *)(op))->chnl = (v))
#define Stream_setStreamActive(op, v) (((Stream *)(op))->active = (v))
#define Stream_setStreamToDac(op, v) (((Stream *)(op))->todac = (v))
 
#endif
/* __STREAMMODULE */
