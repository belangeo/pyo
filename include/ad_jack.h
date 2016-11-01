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
#include "servermodule.h"

typedef struct {
    jack_client_t *jack_client;
    jack_port_t **jack_in_ports;
    jack_port_t **jack_out_ports;
} PyoJackBackendData;

int jack_callback(jack_nframes_t nframes, void *arg);
int jack_srate_cb(jack_nframes_t nframes, void *arg);
int jack_bufsize_cb(jack_nframes_t nframes, void *arg);
void jack_error_cb(const char *desc);
void jack_shutdown_cb(void *arg);
void Server_jack_autoconnect(Server *self);
int Server_jack_init(Server *self);
int Server_jack_deinit(Server *self);
int Server_jack_start(Server *self);
int Server_jack_stop(Server *self);

#endif 
/* _AD_JACK_H */
