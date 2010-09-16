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

#ifndef Py_SERVERMODULE_H
#define Py_SERVERMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "portaudio.h"
#include "portmidi.h"
#include "sndfile.h"
#include "pyomodule.h"

typedef struct {
    PyObject_HEAD
    PyObject *streams;
    PaStream *stream;
    PmStream *in;
    PmEvent midiEvents[200];
    int midi_count;
    MYFLT samplingRate;
    int nchnls;
    int bufferSize;
    int duplex;
    int input;
    int output;
    int midi_input;
    int withPortMidi;
    int server_started;
    int server_stopped; // for fadeout
    int server_booted;
    int stream_count;
    int record;
    /* global amplitude */
    MYFLT amp;
    MYFLT resetAmp;
    MYFLT lastAmp;
    MYFLT currentAmp;
    MYFLT stepVal;
    int timeStep;
    int timeCount;
    
    MYFLT *input_buffer;
    char *recpath;
    SNDFILE *recfile;
    SF_INFO recinfo;
    /* GUI VUMETER */
    int withGUI;
    int numPass;
    int gcount;
    float *lastRms;
    PyObject *GUI;
} Server;

PyObject * PyServer_get_server();
extern PyObject * Server_removeStream(Server *self, int sid);
extern MYFLT * Server_getInputBuffer(Server *self);    
extern PmEvent * Server_getMidiEventBuffer(Server *self);    
extern int Server_getMidiEventCount(Server *self);    
extern PyTypeObject ServerType;    
    

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_SERVERMODULE_H) */

