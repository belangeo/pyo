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
#include "py2to3.h"

static PyoMidiEvent
JackMidiEventToPyoMidiEvent(jack_midi_event_t event)
{
    PyoMidiEvent newbuf;
    newbuf.message = PyoMidi_Message(event.buffer[0], event.buffer[1], event.buffer[2]);
    newbuf.timestamp = (long)event.time;
    return newbuf;
}

static int
jackMidiEventCompare(const void *evt1, const void *evt2) {
    const PyoJackMidiEvent *event1 = evt1;
    const PyoJackMidiEvent *event2 = evt2;
    if (event1->timestamp > event2->timestamp) return 1;
    if (event1->timestamp < event2->timestamp) return -1;
    return 0;
}

static int
jackMidiEventSort(PyoJackMidiEvent *orig, PyoJackMidiEvent *new, Server *server) {
    int i, count = 0;
    jack_nframes_t time = 0;
    for (i=0; i<512; i++) {
        if (orig[i].timestamp != -1 && orig[i].timestamp < (server->elapsedSamples + server->bufferSize)) {
            time = orig[i].timestamp % server->bufferSize;
            new[count] = orig[i];
            new[count].timestamp = time;
            orig[i].timestamp = -1;
            count++;
        }
    }
    if (count > 1) {
        qsort(new, count, sizeof(PyoJackMidiEvent), jackMidiEventCompare);
    }

    return count;
}

static int
jack_callback(jack_nframes_t nframes, void *arg) {
    int i, j;
    Server *server = (Server *) arg;

    assert(nframes == server->bufferSize);

    jack_default_audio_sample_t *in_buffers[server->ichnls], *out_buffers[server->nchnls];

    PyoJackBackendData *be_data = (PyoJackBackendData *) server->audio_be_data;

    if (be_data->jack_in_ports != NULL) {
        for (i = 0; i < server->ichnls; i++) {
            in_buffers[i] = jack_port_get_buffer(be_data->jack_in_ports[i+server->input_offset], server->bufferSize);
        }
    }

    for (i = 0; i < server->nchnls; i++) {
        out_buffers[i] = jack_port_get_buffer(be_data->jack_out_ports[i+server->output_offset], server->bufferSize);

    }

    /* Outputs zeros while the audio server is not started. */
    if (!server->server_started) {
        for (i=0; i<server->bufferSize; i++) {
            for (j=0; j<server->nchnls; j++) {
                out_buffers[j][i] = 0.0;
            }
        }
        return 0;
    }

    if (server->withJackMidi) {
        // Handle midiout events.
        if (be_data->midi_event_count > 0) {
            PyoJackMidiEvent ordlist[512];
            int numevents = jackMidiEventSort(be_data->midi_events, ordlist, server);
            be_data->midi_event_count -= numevents;

            unsigned char *buffer;
            void *port_buf_out = jack_port_get_buffer(be_data->jack_midiout_port, server->bufferSize);
            jack_midi_clear_buffer(port_buf_out);
            for (i=0; i<numevents; i++) {
                buffer = jack_midi_event_reserve(port_buf_out, ordlist[i].timestamp, 3);
                buffer[0] = ordlist[i].status;
                buffer[1] = ordlist[i].data1;
                buffer[2] = ordlist[i].data2;
            }
        }

        // Handle midiin events.
        void* port_buf_in = jack_port_get_buffer(be_data->jack_midiin_port, server->bufferSize);

        jack_midi_event_t in_event;
        jack_nframes_t event_count = jack_midi_get_event_count(port_buf_in);

        if (event_count > 0) {
            for (i=0; i<event_count; i++) {
                jack_midi_event_get(&in_event, port_buf_in, i);
                server->midiEvents[server->midi_count++] = JackMidiEventToPyoMidiEvent(in_event);
            }
        }
    } else {
        pyoGetMidiEvents(server);
    }

    /* jack audio data is not interleaved */
    if (be_data->jack_in_ports != NULL) {
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

static int
jack_transport_cb(jack_transport_state_t state, jack_position_t *pos, void *arg) {
    Server *server = (Server *) arg;

    if (server->jack_transport_state == state)
        return 0;

    switch (state) {
        case JackTransportStopped:
            if (server->server_started) {
                PyGILState_STATE s = PyGILState_Ensure();
                Server_stop(server);
                PyGILState_Release(s);
            }
            break;
        case JackTransportRolling:
            if (!server->server_started) {
                PyGILState_STATE s = PyGILState_Ensure();
                Server_start(server);
                PyGILState_Release(s);
            }
            break;
        case JackTransportStarting:
            break;
        case JackTransportLooping:
            break;
    }

    server->jack_transport_state = state;

    return 0;
}

static int
jack_srate_cb(jack_nframes_t nframes, void *arg) {
    Server *server = (Server *) arg;
    server->samplingRate = (double) nframes;

    PyGILState_STATE s = PyGILState_Ensure();
    Server_debug(server, "The sample rate is now %lu.\n", (unsigned long) nframes);
    PyGILState_Release(s);

    return 0;
}

static int
jack_bufsize_cb(jack_nframes_t nframes, void *arg) {
    Server *server = (Server *) arg;
    server->bufferSize = (int) nframes;

    PyGILState_STATE s = PyGILState_Ensure();
    Server_debug(server, "The buffer size is now %lu.\n", (unsigned long) nframes);
    PyGILState_Release(s);

    return 0;
}

static void
jack_error_cb(const char *desc) {
#ifndef NDEBUG
    PyGILState_STATE s = PyGILState_Ensure();
    PySys_WriteStdout("Jack error: %s\n", desc);
    PyGILState_Release(s);
#endif
}

static void
jack_shutdown_cb(void *arg) {
    Server *server = (Server *) arg;

    PyoJackBackendData *be_data = (PyoJackBackendData *) server->audio_be_data;

    be_data->activated = 0;

    PyGILState_STATE s = PyGILState_Ensure();
    Server_shutdown(server);
    Server_warning(server, "Jack server shutdown. Pyo Server also shutdown.\n");
    PyGILState_Release(s);
}

static void
Server_jack_autoconnect(Server *self) {
    const char **ports;
    char *portname;
    int i, j, num, err = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (self->jackautoin && be_data->jack_in_ports != NULL) {

        Py_BEGIN_ALLOW_THREADS
        ports = jack_get_ports(be_data->jack_client, "system", NULL, JackPortIsOutput);
        Py_END_ALLOW_THREADS

        if (ports == NULL) {
            Server_error(self, "Jack cannot find any physical capture ports called 'system'\n");
        }

        i = 0;
        while(ports[i] != NULL && be_data->jack_in_ports[i] != NULL){

            Py_BEGIN_ALLOW_THREADS
            err = jack_connect(be_data->jack_client, ports[i], jack_port_name(be_data->jack_in_ports[i]));
            Py_END_ALLOW_THREADS

            if (err) {
                Server_error(self, "Jack cannot connect 'system' to input ports\n");
            }
            i++;
        }
        free(ports);
    }

    if (self->jackautoout) {

        Py_BEGIN_ALLOW_THREADS
        ports = jack_get_ports(be_data->jack_client, "system", NULL, JackPortIsInput);
        Py_END_ALLOW_THREADS

        if (ports == NULL) {
            Server_error(self, "Jack cannot find any physical playback ports called 'system'\n");
        }

        i = 0;
        while(ports[i] != NULL && be_data->jack_out_ports[i] != NULL){

            Py_BEGIN_ALLOW_THREADS
            jack_connect(be_data->jack_client, jack_port_name (be_data->jack_out_ports[i]), ports[i]);
            Py_END_ALLOW_THREADS

            if (err) {
                Server_error(self, "Jack cannot connect output ports to 'system'\n");
            }
            i++;
        }
        free(ports);
    }

    if (be_data->jack_in_ports != NULL) {
        num = PyList_Size(self->jackAutoConnectInputPorts);
        if (num > 0) {
            if (num != self->ichnls || !PyList_Check(PyList_GetItem(self->jackAutoConnectInputPorts, 0))) {
                Server_error(self, "Jack auto-connect input ports list size does not match server.ichnls.\n");
            }
            else {
                for (j=0; j<self->ichnls; j++) {
                    num = PyList_Size(PyList_GetItem(self->jackAutoConnectInputPorts, j));
                    for (i=0; i<num; i++) {
                        portname = PY_STRING_AS_STRING(PyList_GetItem(PyList_GetItem(self->jackAutoConnectInputPorts, j), i));

                        if (jack_port_by_name(be_data->jack_client, portname) != NULL) {

                            Py_BEGIN_ALLOW_THREADS
                            err = jack_connect(be_data->jack_client, portname, jack_port_name(be_data->jack_in_ports[j]));
                            Py_END_ALLOW_THREADS

                            if (err) {
                                Server_error(self, "Jack cannot connect '%s' to input port %d\n", portname, j);
                            }
                        }
                        else {
                            Server_error(self, "Jack cannot find port '%s'\n", portname);
                        }
                    }
                }
            }
        }
    }

    num = PyList_Size(self->jackAutoConnectOutputPorts);
    if (num > 0) {
        if (num != self->nchnls || !PyList_Check(PyList_GetItem(self->jackAutoConnectOutputPorts, 0))) {
            Server_error(self, "Jack auto-connect output ports list size does not match server.nchnls.\n");
        } else {
            for (j=0; j<self->nchnls; j++) {
                num = PyList_Size(PyList_GetItem(self->jackAutoConnectOutputPorts, j));
                for (i=0; i<num; i++) {
                    portname = PY_STRING_AS_STRING(PyList_GetItem(PyList_GetItem(self->jackAutoConnectOutputPorts, j), i));
                    if (jack_port_by_name(be_data->jack_client, portname) != NULL) {

                        Py_BEGIN_ALLOW_THREADS
                        jack_connect(be_data->jack_client, jack_port_name(be_data->jack_out_ports[j]), portname);
                        Py_END_ALLOW_THREADS

                        if (err) {
                            Server_error(self, "Jack cannot connect output port %d to '%s'\n", j, portname);
                        }
                    }
                    else {
                        Server_error(self, "Jack cannot find port '%s'\n", portname);
                    }
                }
            }
        }
    }

    if (self->withJackMidi) {
        num = PyList_Size(self->jackAutoConnectMidiInputPort);
        if (num > 0) {
            for (i=0; i<num; i++) {
                portname = PY_STRING_AS_STRING(PyList_GetItem(self->jackAutoConnectMidiInputPort, i));

                if (jack_port_by_name(be_data->jack_client, portname) != NULL) {

                    Py_BEGIN_ALLOW_THREADS
                    err = jack_connect(be_data->jack_client, portname, jack_port_name(be_data->jack_midiin_port));
                    Py_END_ALLOW_THREADS

                    if (err) {
                        Server_error(self, "Jack cannot connect '%s' to midi input port\n", portname);
                    }
                }
                else {
                    Server_error(self, "Jack cannot find port '%s'\n", portname);
                }
            }
        }

        num = PyList_Size(self->jackAutoConnectMidiOutputPort);
        if (num > 0) {
            for (i=0; i<num; i++) {
                portname = PY_STRING_AS_STRING(PyList_GetItem(self->jackAutoConnectMidiOutputPort, i));

                if (jack_port_by_name(be_data->jack_client, portname) != NULL) {

                    Py_BEGIN_ALLOW_THREADS
                    err = jack_connect(be_data->jack_client, jack_port_name(be_data->jack_midiout_port), portname);
                    Py_END_ALLOW_THREADS

                    if (err) {
                        Server_error(self, "Jack cannot connect '%s' to midi output port\n", portname);
                    }
                }
                else {
                    Server_error(self, "Jack cannot find port '%s'\n", portname);
                }
            }
        }
    }
}

int
Server_jack_init(Server *self) {
    int i = 0;
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
    PyoJackBackendData *be_data = (PyoJackBackendData *) malloc(sizeof(PyoJackBackendData));
    self->audio_be_data = (void *) be_data;
    be_data->activated = 0;
    strncpy(client_name, self->serverName, 32);

    Py_BEGIN_ALLOW_THREADS
    be_data->midi_event_count = 0;
    if (self->duplex == 1) {
        be_data->jack_in_ports = (jack_port_t **) calloc(self->ichnls + self->input_offset, sizeof(jack_port_t *));
    } else {
        be_data->jack_in_ports = NULL;
    }
    be_data->jack_out_ports = (jack_port_t **) calloc(self->nchnls + self->output_offset, sizeof(jack_port_t *));
    be_data->jack_client = jack_client_open(client_name, options, &status, server_name);
    if (self->withJackMidi) {
        be_data->midi_events = (PyoJackMidiEvent *)malloc(512 * sizeof(PyoJackMidiEvent));
        for (i=0; i<512; i++) {
            be_data->midi_events[i].timestamp = -1;
        }
    }
    Py_END_ALLOW_THREADS

    if (be_data->jack_client == NULL) {
        Server_error(self, "Jack unable to create client\n");
        if (status & JackServerFailed) {
            Server_debug(self, "Jack jack_client_open() failed, "
                         "status = 0x%2.0x\n", status);
        }
        return -1;
    }
    if (status & JackServerStarted) {
        Server_warning(self, "Jack server started.\n");
    }
    if (strcmp(self->serverName, jack_get_client_name(be_data->jack_client)) ) {
        strcpy(self->serverName, jack_get_client_name(be_data->jack_client));
        Server_warning(self, "Jack name `%s' assigned.\n", self->serverName);
    }

    sampleRate = jack_get_sample_rate(be_data->jack_client);
    if (sampleRate != self->samplingRate) {
        self->samplingRate = (double)sampleRate;
        Server_warning(self, "Sample rate set to Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    else {
        Server_debug(self, "Jack engine sample rate: %" PRIu32 "\n", sampleRate);
    }
    if (sampleRate <= 0) {
        Server_error(self, "Jack invalid engine sample rate.");

        Py_BEGIN_ALLOW_THREADS
        jack_client_close(be_data->jack_client);
        Py_END_ALLOW_THREADS

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

    if (self->withJackMidi) {
        Py_BEGIN_ALLOW_THREADS
        be_data->jack_midiin_port = jack_port_register(be_data->jack_client, "input",
                                                       JACK_DEFAULT_MIDI_TYPE,
                                                       JackPortIsInput, 0);
        be_data->jack_midiout_port = jack_port_register(be_data->jack_client, "output",
                                                       JACK_DEFAULT_MIDI_TYPE,
                                                       JackPortIsOutput, 0);
        Py_END_ALLOW_THREADS
    }

    if (self->duplex == 1) {
        nchnls = total_nchnls = self->ichnls + self->input_offset;
        while (nchnls-- > 0) {
            index = total_nchnls - nchnls - 1;
            ret = sprintf(name, "input_%i", index + 1);
            if (ret > 0) {

                Py_BEGIN_ALLOW_THREADS
                be_data->jack_in_ports[index] = jack_port_register(be_data->jack_client,
                                                                   name,
                                                                   JACK_DEFAULT_AUDIO_TYPE,
                                                                   JackPortIsInput, 0);
                Py_END_ALLOW_THREADS

            }

            if ((be_data->jack_in_ports[index] == NULL)) {
                Server_error(self, "No more Jack input ports available\n");
                return -1;
            }
        }
    }

    nchnls = total_nchnls = self->nchnls + self->output_offset;
    while (nchnls-- > 0) {
        index = total_nchnls - nchnls - 1;
        ret = sprintf(name, "output_%i", index + 1);
        if (ret > 0) {

            Py_BEGIN_ALLOW_THREADS
            be_data->jack_out_ports[index] = jack_port_register(be_data->jack_client,
                                                                name,
                                                                JACK_DEFAULT_AUDIO_TYPE,
                                                                JackPortIsOutput, 0);
            Py_END_ALLOW_THREADS

        }
        if ((be_data->jack_out_ports[index] == NULL)) {
            Server_error(self, "No more Jack output ports available\n");
            return -1;
        }
    }
    jack_set_error_function(jack_error_cb);
    jack_set_sample_rate_callback(be_data->jack_client, jack_srate_cb, (void *) self);
    jack_on_shutdown(be_data->jack_client, jack_shutdown_cb, (void *) self);
    jack_set_buffer_size_callback(be_data->jack_client, jack_bufsize_cb, (void *) self);
    jack_set_process_callback(be_data->jack_client, jack_callback, (void *) self);

    if (self->isJackTransportSlave)
        jack_set_sync_callback(be_data->jack_client, jack_transport_cb, (void *) self);

    Py_BEGIN_ALLOW_THREADS
    ret = jack_activate(be_data->jack_client);
    Py_END_ALLOW_THREADS

    if (ret) {
        Server_error(self, "Jack cannot activate jack client.\n");
        return -1;
    }

    be_data->activated = 1;

    Server_jack_autoconnect(self);

    return 0;
}

int
Server_jack_deinit(Server *self) {
    int ret = 0;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (be_data->activated == 1) {

        Py_BEGIN_ALLOW_THREADS
        ret = jack_deactivate(be_data->jack_client);
        Py_END_ALLOW_THREADS

        if (ret)
            Server_error(self, "Jack cannot deactivate jack client.\n");


        Py_BEGIN_ALLOW_THREADS
        ret = jack_client_close(be_data->jack_client);
        Py_END_ALLOW_THREADS

        if (ret)
            Server_error(self, "Jack cannot close client.\n");
    }

    be_data->activated = 0;

    if (be_data->jack_in_ports != NULL) {
        free(be_data->jack_in_ports);
    }
    free(be_data->jack_out_ports);
    if (self->withJackMidi == 1) {
        free(be_data->midi_events);
    }

    free(self->audio_be_data);

    return ret;
}

int
jack_input_port_set_names(Server *self) {
    int i, err, lsize;
    char *name;
    char result[128];
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (be_data->jack_in_ports == NULL) {
        Server_error(self, "Can not change Jack input port name with duplex=0.\n");
        return 0;
    }

    if (PyList_Check(self->jackInputPortNames)) {
        lsize = PyList_Size(self->jackInputPortNames);
        for (i=0; i<self->ichnls && i<lsize; i++) {
            name = PY_STRING_AS_STRING(PyList_GetItem(self->jackInputPortNames, i));

            Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
            err = jack_port_rename(be_data->jack_client, be_data->jack_in_ports[i], name);
#else
            err = jack_port_set_name(be_data->jack_in_ports[i], name);
#endif
            Py_END_ALLOW_THREADS

            if (err)
                Server_error(self, "Jack cannot change port short name.\n");
        }
    }
    else if (PY_STRING_CHECK(self->jackInputPortNames)) {
        name = PY_STRING_AS_STRING(self->jackInputPortNames);
        for (i=0; i<self->ichnls; i++) {
            sprintf(result, "%s_%d", name, i);

            Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
            err = jack_port_rename(be_data->jack_client, be_data->jack_in_ports[i], result);
#else
            err = jack_port_set_name(be_data->jack_in_ports[i], result);
#endif
            Py_END_ALLOW_THREADS

            if (err)
                Server_error(self, "Jack cannot change port short name.\n");
        }
    }
    else
        Server_error(self, "Jack input port names must be a string or a list of strings.\n");

    return 0;
}

int
jack_output_port_set_names(Server *self) {
    int i, err, lsize;
    char *name;
    char result[128];
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (PyList_Check(self->jackOutputPortNames)) {
        lsize = PyList_Size(self->jackOutputPortNames);
        for (i=0; i<self->nchnls && i<lsize; i++) {
            name = PY_STRING_AS_STRING(PyList_GetItem(self->jackOutputPortNames, i));

            Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
            err = jack_port_rename(be_data->jack_client, be_data->jack_out_ports[i], name);
#else
            err = jack_port_set_name(be_data->jack_out_ports[i], name);
#endif
            Py_END_ALLOW_THREADS

            if (err)
                Server_error(self, "Jack cannot change port short name.\n");
        }
    }
    else if (PY_STRING_CHECK(self->jackOutputPortNames)) {
        name = PY_STRING_AS_STRING(self->jackOutputPortNames);
        for (i=0; i<self->nchnls; i++) {
            sprintf(result, "%s_%d", name, i);

            Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
            err = jack_port_rename(be_data->jack_client, be_data->jack_out_ports[i], result);
#else
            err = jack_port_set_name(be_data->jack_out_ports[i], result);
#endif
            Py_END_ALLOW_THREADS

            if (err)
                Server_error(self, "Jack cannot change port short name.\n");
        }
    }
    else
        Server_error(self, "Jack output port names must be a string or a list of strings.\n");

    return 0;
}

int
jack_midi_input_port_set_name(Server *self) {
    int err;
    char *name;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (PY_STRING_CHECK(self->jackMidiInputPortName)) {
        name = PY_STRING_AS_STRING(self->jackMidiInputPortName);

        Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
        err = jack_port_rename(be_data->jack_client, be_data->jack_midiin_port, name);
#else
        err = jack_port_set_name(be_data->jack_midiin_port, name);
#endif
        Py_END_ALLOW_THREADS

        if (err)
            Server_error(self, "Jack cannot change midi input port short name.\n");
    }
    else
        Server_error(self, "Jack midi input port name must be a string.\n");

    return 0;
}

int
jack_midi_output_port_set_name(Server *self) {
    int err;
    char *name;
    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    if (PY_STRING_CHECK(self->jackMidiOutputPortName)) {
        name = PY_STRING_AS_STRING(self->jackMidiOutputPortName);

        Py_BEGIN_ALLOW_THREADS
#ifdef JACK_NEW_API
        err = jack_port_rename(be_data->jack_client, be_data->jack_midiout_port, name);
#else
        err = jack_port_set_name(be_data->jack_midiout_port, name);
#endif
        Py_END_ALLOW_THREADS

        if (err)
            Server_error(self, "Jack cannot change midi output port short name.\n");
    }
    else
        Server_error(self, "Jack midi output port name must be a string.\n");

    return 0;
}

int
Server_jack_start(Server *self) {
    return 0;
}

int
Server_jack_stop(Server *self) {
    return 0;
}

void
jack_noteout(Server *self, int pit, int vel, int chan, long timestamp)
{
    int i;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0x90;
    else
        event.status = 0x90 | (chan - 1);
    event.data1 = pit;
    event.data2 = vel;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_afterout(Server *self, int pit, int vel, int chan, long timestamp)
{
    int i;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0xA0;
    else
        event.status = 0xA0 | (chan - 1);
    event.data1 = pit;
    event.data2 = vel;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp)
{
    int i;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0xB0;
    else
        event.status = 0xB0 | (chan - 1);
    event.data1 = ctlnum;
    event.data2 = value;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_programout(Server *self, int value, int chan, long timestamp)
{
    int i;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0xC0;
    else
        event.status = 0xC0 | (chan - 1);
    event.data1 = value;
    event.data2 = 0;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_pressout(Server *self, int value, int chan, long timestamp)
{
    int i;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0xD0;
    else
        event.status = 0xD0 | (chan - 1);
    event.data1 = value;
    event.data2 = 0;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_bendout(Server *self, int value, int chan, long timestamp)
{
    int i, lsb, msb;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent event;
    event.timestamp = elapsed + (unsigned long)(timestamp * 0.001 * self->samplingRate);
    if (chan == 0)
        event.status = 0xE0;
    else
        event.status = 0xE0 | (chan - 1);

    lsb = value & 0x007F;
    msb = (value & (0x007F << 7)) >> 7;

    event.data1 = lsb;
    event.data2 = msb;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = event;
            be_data->midi_event_count++;
            break;
        }
    }
}

void
jack_makenote(Server *self, int pit, int vel, int dur, int chan)
{
    int i, channel;
    unsigned long elapsed = Server_getElapsedTime(self);

    PyoJackBackendData *be_data = (PyoJackBackendData *) self->audio_be_data;

    PyoJackMidiEvent eventon, eventoff;

    channel = (chan == 0) ? 0x90 : 0x90 | (chan - 1);

    /* noteon */
    eventon.timestamp = elapsed;
    eventon.status = channel;
    eventon.data1 = pit;
    eventon.data2 = vel;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = eventon;
            be_data->midi_event_count++;
            break;
        }
    }

    /* noteoff */
    eventoff.timestamp = elapsed + (unsigned long)(dur * 0.001 * self->samplingRate);
    eventoff.status = channel;
    eventoff.data1 = pit;
    eventoff.data2 = 0;

    for (i=0; i<512; i++) {
        if (be_data->midi_events[i].timestamp == -1) {
            be_data->midi_events[i] = eventoff;
            be_data->midi_event_count++;
            break;
        }
    }
}
