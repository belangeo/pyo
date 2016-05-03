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

#ifndef _AD_PORTAUDIO_H
#define _AD_PORTAUDIO_H

#include <Python.h>
#include "portaudio.h"
#include "servermodule.h"

typedef struct {
    PaStream *stream;
} PyoPaBackendData;

int pa_callback_interleaved(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *arg);

int pa_callback_nonInterleaved(const void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *arg);
int Server_pa_init(Server *self);
int Server_pa_deinit(Server *self);
int Server_pa_start(Server *self);
int Server_pa_stop(Server *self);

/* Queries. */
PyObject * portaudio_get_version();
PyObject * portaudio_get_version_text();
PyObject * portaudio_count_host_apis();
PyObject * portaudio_list_host_apis();
PyObject * portaudio_get_default_host_api();
PyObject * portaudio_count_devices();
PyObject * portaudio_list_devices();
PyObject * portaudio_get_devices_infos();
PyObject * portaudio_get_output_devices();
PyObject * portaudio_get_output_max_channels(PyObject *self, PyObject *arg);
PyObject * portaudio_get_input_max_channels(PyObject *self, PyObject *arg);
PyObject * portaudio_get_input_devices();
PyObject * portaudio_get_default_input();
PyObject * portaudio_get_default_output();

#endif /* _AD_PORTAUDIO_H */
