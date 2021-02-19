/**************************************************************************
 * Copyright 2009-2021 Olivier Belanger                                   *
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
 *                                                                        *
 * Octobre 2013 : Multiple servers facility and embedded backend added    *
 * by Guillaume Barrette.                                                 *
 * 2014-2016 : Several improvements by Olivier Belanger.                  *
 *************************************************************************/

#ifndef Py_SERVERMODULE_H
#define Py_SERVERMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pyomodule.h"

typedef struct
{
    PyObject_HEAD
    PyObject *streams;
    double samplingRate;
    int nchnls;
    int ichnls;
    int bufferSize;
    int currentResampling;
    int lastResampling;
    int duplex;
    int server_started;
    int server_booted;
    int stream_count;
    int thisServerID;       /* To keep the reference index in the array of servers */

    MYFLT *input_buffer;
    float *output_buffer;   /* Has to be float since audio callbacks must use floats */

    PyObject *CALLBACK;     /* Custom callback */

    float globalDur;        /* Global duration */
    float globalDel;        /* Global delay */

    int verbosity;          /* Logging levels: 1 = error, 2 = message, 4 = warning , 8 = debug. Default 7.*/
    int globalSeed;         /* initial seed for random objects. If <= 0, objects are seeded with the clock. */
    int autoStartChildren;  /* if true, calls to play, out and stop propagate to children objects. */
} Server;

PyObject * PyServer_get_server();
extern unsigned int pyorand();
extern PyObject * Server_removeStream(Server *self, int sid);
extern MYFLT * Server_getInputBuffer(Server *self);
extern int Server_getCurrentResamplingFactor(Server *self);
extern int Server_getLastResamplingFactor(Server *self);
extern int Server_generateSeed(Server *self, int oid);
extern PyTypeObject ServerType;
void Server_process_buffers(Server *server);
void Server_error(Server *self, char * format, ...);
void Server_message(Server *self, char * format, ...);
void Server_warning(Server *self, char * format, ...);
void Server_debug(Server *self, char * format, ...);
PyObject * Server_shutdown(Server *self);
PyObject * Server_stop(Server *self);
PyObject * Server_start(Server *self);
PyObject * Server_boot(Server *self, PyObject *arg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* Py_SERVERMODULE_H */
