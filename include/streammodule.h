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
#include "pyomodule.h"

typedef struct {
    PyObject_HEAD
    PyObject *streamobject;
    void (*funcptr)();
    int sid;
    int chnl;
    int bufsize;
    int active;
    int todac;
    int duration;
    int bufferCountWait;
    int bufferCount;
    MYFLT *data;
} Stream;

extern int Stream_getNewStreamId();
extern PyObject * Stream_getStreamObject(Stream *self);
extern int Stream_getStreamId(Stream *self);
extern int Stream_getStreamActive(Stream *self);
extern int Stream_getBufferCountWait(Stream *self);
extern int Stream_getDuration(Stream *self);
extern int Stream_getStreamChnl(Stream *self);
extern int Stream_getStreamToDac(Stream *self);
extern MYFLT * Stream_getData(Stream *self);
extern void Stream_setData(Stream * self, MYFLT *data);
extern void Stream_setFunctionPtr(Stream *self, void *ptr);
extern void Stream_callFunction(Stream *self);
extern void Stream_IncrementBufferCount(Stream *self);
extern void Stream_IncrementDurationCount(Stream *self);
extern PyTypeObject StreamType;

#define MAKE_NEW_STREAM(self, type, rt_error) \
  (self) = (Stream *)(type)->tp_alloc((type), 0); \
  if ((self) == rt_error) { return rt_error; } \
 \
  (self)->sid = (self)->chnl = (self)->todac = (self)->bufferCountWait = 0; \
  (self)->bufferCount = (self)->bufsize = (self)->duration = (self)->active = 0;

typedef struct {
    PyObject_HEAD
    MYFLT *data;
} TriggerStream;

extern MYFLT * TriggerStream_getData(TriggerStream *self);
extern void TriggerStream_setData(TriggerStream * self, MYFLT *data);
extern PyTypeObject TriggerStreamType;

#define MAKE_NEW_TRIGGER_STREAM(self, type, rt_error) \
    (self) = (TriggerStream *)(type)->tp_alloc((type), 0); \


#ifdef __STREAM_MODULE
/* include from stream.c */

#else
/* include from other modules to use API */

#define Stream_setStreamObject(op, v) (((Stream *)(op))->streamobject = (v))
#define Stream_setStreamId(op, v) (((Stream *)(op))->sid = (v))
#define Stream_setStreamChnl(op, v) (((Stream *)(op))->chnl = (v))
#define Stream_setStreamActive(op, v) (((Stream *)(op))->active = (v))
#define Stream_setStreamToDac(op, v) (((Stream *)(op))->todac = (v))
#define Stream_setBufferCountWait(op, v) (((Stream *)(op))->bufferCountWait = (v))
#define Stream_setDuration(op, v) (((Stream *)(op))->duration = (v))
#define Stream_setBufferSize(op, v) (((Stream *)(op))->bufsize = (v))
#define Stream_resetBufferCount(op) (((Stream *)(op))->bufferCount = 0)

#endif
/* __STREAMMODULE */