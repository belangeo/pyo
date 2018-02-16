/**************************************************************************
 * Copyright 2009-2016 Olivier Belanger                                   *
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

#ifndef _MD_PORTMIDI_H
#define _MD_PORTMIDI_H

#include <Python.h>
#include "portmidi.h"
#include "porttime.h"
#include "servermodule.h"

typedef struct {
    PmStream *midiin[64];
    PmStream *midiout[64];
} PyoPmBackendData;

void portmidiGetEvents(Server *self);
int Server_pm_init(Server *self);
int Server_pm_deinit(Server *self);
void pm_noteout(Server *self, int pit, int vel, int chan, long timestamp);
void pm_afterout(Server *self, int pit, int vel, int chan, long timestamp);
void pm_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp);
void pm_programout(Server *self, int value, int chan, long timestamp);
void pm_pressout(Server *self, int value, int chan, long timestamp);
void pm_bendout(Server *self, int value, int chan, long timestamp);
void pm_sysexout(Server *self, unsigned char *msg, long timestamp);
void pm_makenote(Server *self, int pit, int vel, int dur, int chan);
long pm_get_current_time();

/* Queries. */
PyObject * portmidi_count_devices();
PyObject * portmidi_list_devices();
PyObject * portmidi_get_input_devices();
PyObject * portmidi_get_output_devices();
PyObject * portmidi_get_default_input();
PyObject * portmidi_get_default_output();

#endif /* _MD_PORTMIDI_H */
