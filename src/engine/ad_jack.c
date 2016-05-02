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

#include "ad_jack.h"

int
jack_callback(jack_nframes_t nframes, void *arg) {
    int i, j;
    Server *server = (Server *) arg;
    assert(nframes == server->bufferSize);
    jack_default_audio_sample_t *in_buffers[server->ichnls], *out_buffers[server->nchnls];

    if (server->withPortMidi == 1) {
        portmidiGetEvents(server);
    }
    PyoJackBackendData *be_data = (PyoJackBackendData *) server->audio_be_data;
    for (i = 0; i < server->ichnls; i++) {
        in_buffers[i] = jack_port_get_buffer (be_data->jack_in_ports[i+server->input_offset], server->bufferSize);
    }
    for (i = 0; i < server->nchnls; i++) {
        out_buffers[i] = jack_port_get_buffer (be_data->jack_out_ports[i+server->output_offset], server->bufferSize);

    }
    /* jack audio data is not interleaved */
    if (server->duplex == 1) {
        for (i=0; i<server->bufferSize; i++) {
            for (j=0; j<server->ichnls; j++) {
                server->input_buffer[(i*server->ichnls)+j] = (MYFLT) in_buffers[j][i];
            }
        }
    }
    Server_process_buffers(server);
    for (i=0; i<server->bufferSize; i++) {
        for (j=0; j<server->nchnls; j++) {
            out_buffers[j][i] = (jack_default_audio_sample_t) server->output_buffer[(i*server->nchnls)+j];
        }
    }
    server->midi_count = 0;
    return 0;
}

int
jack_srate_cb(jack_nframes_t nframes, void *arg) {
    Server *s = (Server *) arg;
    s->samplingRate = (double) nframes;
    Server_debug(s, "The sample rate is now %lu.\n", (unsigned long) nframes);
    return 0;
}

int
jack_bufsize_cb(jack_nframes_t nframes, void *arg) {
    Server *s = (Server *) arg;
    s->bufferSize = (int) nframes;
    Server_debug(s, "The buffer size is now %lu.\n", (unsigned long) nframes);
    return 0;
}

void
jack_error_cb(const char *desc) {
    printf("JACK error: %s\n", desc);
}

void
jack_shutdown_cb(void *arg) {
    Server *s = (Server *) arg;
    Server_shut_down(s);
    Server_warning(s, "JACK server shutdown. Pyo Server shut down.\n");
}

int
Server_jack_autoconnect(Server *self) {
    const char **ports;
    int i, j, num = 0, ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (self->jackautoin) {
        if ((ports = jack_get_ports (be_data->jack_client, "system", NULL, JackPortIsOutput)) == NULL) {
            Server_error(self, "Jack: Cannot find any physical capture ports called 'system'\n");
            ret = -1;
        }

        i=0;
        while(ports[i]!=NULL && be_data->jack_in_ports[i] != NULL){
            if (jack_connect (be_data->jack_client, ports[i], jack_port_name(be_data->jack_in_ports[i]))) {
                Server_error(self, "Jack: cannot connect input ports to 'system'\n");
                ret = -1;
            }
            i++;
        }
        free (ports);
    }

    if (self->jackautoout) {
        if ((ports = jack_get_ports (be_data->jack_client, "system", NULL, JackPortIsInput)) == NULL) {
            Server_error(self, "Jack: Cannot find any physical playback ports called 'system'\n");
            ret = -1;
        }

        i=0;
        while(ports[i]!=NULL && be_data->jack_out_ports[i] != NULL){
            if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
                Server_error(self, "Jack: cannot connect output ports to 'system'\n");
                ret = -1;
            }
            i++;
        }
        free (ports);
    }

    num = PyList_Size(self->jackAutoConnectInputPorts);
    if (num > 0) {
        for (j=0; j<num; j++) {
            if ((ports = jack_get_ports (be_data->jack_client, PyString_AsString(PyList_GetItem(self->jackAutoConnectInputPorts, j)), NULL, JackPortIsOutput)) == NULL) {
                Server_error(self, "Jack: cannot connect input ports to %s\n", PyString_AsString(PyList_GetItem(self->jackAutoConnectInputPorts, j)));
            }
            else {
                i = 0;
                while(ports[i] != NULL && be_data->jack_in_ports[i] != NULL){
                    if (jack_connect (be_data->jack_client, ports[i], jack_port_name (be_data->jack_in_ports[i]))) {
                        Server_error(self, "Jack: cannot connect input ports\n");
                        ret = -1;
                    }
                    i++;
                }
                free (ports);
            }
        }
    }

    num = PyList_Size(self->jackAutoConnectOutputPorts);
    if (num > 0) {
        for (j=0; j<num; j++) {
            if ((ports = jack_get_ports (be_data->jack_client, PyString_AsString(PyList_GetItem(self->jackAutoConnectOutputPorts, j)), NULL, JackPortIsInput)) == NULL) {
                Server_error(self, "Jack: cannot connect output ports to %s\n", PyString_AsString(PyList_GetItem(self->jackAutoConnectOutputPorts, j)));
            }
            else {
                i = 0;
                while(ports[i] != NULL && be_data->jack_out_ports[i] != NULL){
                    if (jack_connect (be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i])) {
                        Server_error(self, "Jack: cannot connect output ports\n");
                        ret = -1;
                    }
                    i++;
                }
                free (ports);
            }
        }
    }

    return ret;
}

int
Server_jack_init(Server *self) {
    char client_name[32];
    char name[16];
    const char *server_name = "server";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    int sampleRate = 0;
    int bufferSize = 0;
    int nchnls = 0;
    int total_nchnls = 0;
    int index = 0;
    int ret = 0;
    assert(self->audio_be_data == NULL);
    PyoJackBackendData *be_data = (PyoJackBackendData *) malloc(sizeof(PyoJackBackendData *));
    self->audio_be_data = (void *) be_data;
    be_data->jack_in_ports = (jack_port_t **) calloc(self->ichnls + self->input_offset, sizeof(jack_port_t *));
    be_data->jack_out_ports = (jack_port_t **) calloc(self->nchnls + self->output_offset, sizeof(jack_port_t *));
    strncpy(client_name,self->serverName, 32);
    be_data->jack_client = jack_client_open (client_name, options, &status, server_name);
    if (be_data->jack_client == NULL) {
        Server_error(self, "Jack error: Unable to create JACK client\n");
        if (status & JackServerFailed) {
            Server_debug(self, "Jack error: jack_client_open() failed, "
            "status = 0x%2.0x\n", status);
        }
        return -1;
    }
    if (status & JackServerStarted) {
        Server_warning(self, "JACK server started.\n");
    }
    if (strcmp(self->serverName, jack_get_client_name(be_data->jack_client)) ) {
        strcpy(self->serverName, jack_get_client_name(be_data->jack_client));
        Server_warning(self, "Jack name `%s' assigned\n", self->serverName);
    }

    sampleRate = jack_get_sample_rate (be_data->jack_client);
    if (sampleRate != self->samplingRate) {
        self->samplingRate = (double)sampleRate;
        Server_warning(self, "Sample rate set to Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    else {
        Server_debug(self, "Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    if (sampleRate <= 0) {
        Server_error(self, "Invalid Jack engine sample rate.");
        jack_client_close (be_data->jack_client);
        return -1;
    }
    bufferSize = jack_get_buffer_size(be_data->jack_client);
    if (bufferSize != self->bufferSize) {
        self->bufferSize = bufferSize;
        Server_warning(self, "Buffer size set to Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }
    else {
        Server_debug(self, "Jack engine buffer size: %" PRIu32 "\n", bufferSize);
    }

    nchnls = total_nchnls = self->ichnls + self->input_offset;
    while (nchnls-- > 0) {
        index = total_nchnls - nchnls - 1;
        ret = sprintf(name, "input_%i", index + 1);
        if (ret > 0) {
            be_data->jack_in_ports[index]
            = jack_port_register (be_data->jack_client, name,
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        }

        if ((be_data->jack_in_ports[index] == NULL)) {
            Server_error(self, "Jack: no more JACK input ports available\n");
            return -1;
        }
    }

    nchnls = total_nchnls = self->nchnls + self->output_offset;
    while (nchnls-- > 0) {
        index = total_nchnls - nchnls - 1;
        ret = sprintf(name, "output_%i", index + 1);
        if (ret > 0) {
            be_data->jack_out_ports[index]
            = jack_port_register (be_data->jack_client, name,
                                  JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }
        if ((be_data->jack_out_ports[index] == NULL)) {
            Server_error(self, "Jack: no more JACK output ports available\n");
            return -1;
        }
    }
    jack_set_error_function (jack_error_cb);
    jack_set_sample_rate_callback(be_data->jack_client, jack_srate_cb, (void *) self);
    jack_on_shutdown (be_data->jack_client, jack_shutdown_cb, (void *) self);
    jack_set_buffer_size_callback (be_data->jack_client, jack_bufsize_cb, (void *) self);
    return 0;
}

int
Server_jack_deinit(Server *self) {
    int ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    ret = jack_client_close(be_data->jack_client);
    free(be_data->jack_in_ports);
    free(be_data->jack_out_ports);
    free(self->audio_be_data);
    return ret;
}

int
Server_jack_start(Server *self) {
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    jack_set_process_callback(be_data->jack_client, jack_callback, (void *) self);
    if (jack_activate (be_data->jack_client)) {
        Server_error(self, "Jack error: cannot activate jack client.\n");
        jack_client_close (be_data->jack_client);
        Server_shut_down(self);
        return -1;
    }
    Server_jack_autoconnect(self);
    return 0;
}

int
Server_jack_stop(Server *self) {
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;
    int ret = jack_deactivate(be_data->jack_client);
    self->server_started = 0;
    return ret;
}
