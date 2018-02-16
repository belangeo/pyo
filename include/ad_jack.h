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

#ifndef _AD_JACK_H
#define _AD_JACK_H

#include <jack/jack.h>
#include <jack/midiport.h>
#include "servermodule.h"

typedef struct {
    unsigned long timestamp;
    int status;
    int data1;
    int data2;
} PyoJackMidiEvent;

typedef struct {
    int activated;
    jack_client_t *jack_client;
    jack_port_t **jack_in_ports;
    jack_port_t **jack_out_ports;
    unsigned int midi_event_count;
    PyoJackMidiEvent *midi_events;
    jack_port_t *jack_midiin_port;
    jack_port_t *jack_midiout_port;
} PyoJackBackendData;

int jack_input_port_set_names(Server *self);
int jack_output_port_set_names(Server *self);
int jack_midi_input_port_set_name(Server *self);
int jack_midi_output_port_set_name(Server *self);
int Server_jack_init(Server *self);
int Server_jack_deinit(Server *self);
int Server_jack_start(Server *self);
int Server_jack_stop(Server *self);

void jack_noteout(Server *self, int pit, int vel, int chan, long timestamp);
void jack_afterout(Server *self, int pit, int vel, int chan, long timestamp);
void jack_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp);
void jack_programout(Server *self, int value, int chan, long timestamp);
void jack_pressout(Server *self, int value, int chan, long timestamp);
void jack_bendout(Server *self, int value, int chan, long timestamp);
void jack_makenote(Server *self, int pit, int vel, int dur, int chan);

#endif 
/* _AD_JACK_H */
