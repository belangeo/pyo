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

#include "md_portmidi.h"
#include "py2to3.h"

static PyoMidiEvent PmEventToPyoMidiEvent(PmEvent buffer)
{
    PyoMidiEvent newbuf;
    newbuf.message = buffer.message;
    newbuf.timestamp = buffer.timestamp;
    return newbuf;
}

void portmidiGetEvents(Server *self)
{
    int i;
    PmError result, length;
    PmEvent buffer;

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    for (i=0; i<self->midiin_count; i++) {
        do {
            result = Pm_Poll(be_data->midiin[i]);
            if (result) {
                length = Pm_Read(be_data->midiin[i], &buffer, 1);
                if (length > 0)
                    self->midiEvents[self->midi_count++] = PmEventToPyoMidiEvent(buffer);
            }
        } while (result);
    }
}

int
Server_pm_init(Server *self)
{
    int i = 0, ret = 0;
    PmError pmerr;

    if (self->midiActive == 0) {
        self->withPortMidi = 0;
        self->withPortMidiOut = 0;
        return 0;
    }

    Py_BEGIN_ALLOW_THREADS
    pmerr = Pm_Initialize();
    Py_END_ALLOW_THREADS

    if (pmerr) {
        Server_warning(self, "Portmidi warning: could not initialize Portmidi: %s\n", Pm_GetErrorText(pmerr));
        self->withPortMidi = 0;
        self->withPortMidiOut = 0;
        return -1;
    }
    else {
        Server_debug(self, "Portmidi initialized.\n");
        self->withPortMidi = 1;
        self->withPortMidiOut = 1;
    }

    PyoPmBackendData *be_data = (PyoPmBackendData *) malloc(sizeof(PyoPmBackendData));
    self->midi_be_data = (void *) be_data;

    if (self->withPortMidi == 1) {
        self->midiin_count = self->midiout_count = 0;
        int num_devices = Pm_CountDevices();
        Server_debug(self, "Portmidi number of devices: %d.\n", num_devices);
        if (num_devices > 0) {
            if (self->midi_input < num_devices) {
                if (self->midi_input == -1)
                    self->midi_input = Pm_GetDefaultInputDeviceID();
                Server_debug(self, "Midi input device : %d.\n", self->midi_input);
                const PmDeviceInfo *info = Pm_GetDeviceInfo(self->midi_input);
                if (info != NULL) {
                    if (info->input) {

                        Py_BEGIN_ALLOW_THREADS
                        Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                        pmerr = Pm_OpenInput(&be_data->midiin[0], self->midi_input, NULL, 100, NULL, NULL);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            Server_warning(self,
                                 "Portmidi warning: could not open midi input %d (%s): %s\n",
                                 self->midi_input, info->name, Pm_GetErrorText(pmerr));
                            self->withPortMidi = 0;
                        }
                        else {
                            Server_debug(self, "Midi input (%s) opened.\n", info->name);
                            self->midiin_count = 1;
                        }
                    }
                    else {
                        Server_warning(self, "Portmidi warning: Midi Device (%s), not an input device!\n", info->name);
                        self->withPortMidi = 0;
                    }
                }
                else {
                    Server_debug(self, "Can't get midi input device info : %d.\n", self->midi_input);
                    self->withPortMidi = 0;
                }
            }
            else if (self->midi_input >= num_devices) {
                Server_debug(self, "Midi input device : all!\n");
                self->midiin_count = 0;

                Py_BEGIN_ALLOW_THREADS
                Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                Py_END_ALLOW_THREADS

                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
                    if (info != NULL) {
                        if (info->input) {

                            Py_BEGIN_ALLOW_THREADS
                            pmerr = Pm_OpenInput(&be_data->midiin[self->midiin_count], i, NULL, 100, NULL, NULL);
                            Py_END_ALLOW_THREADS

                            if (pmerr) {
                                Server_warning(self,
                                     "Portmidi warning: could not open midi input %d (%s): %s\n",
                                     0, info->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                Server_debug(self, "Midi input (%s) opened.\n", info->name);
                                self->midiin_count++;
                            }
                        }
                    }
                }
                if (self->midiin_count == 0)
                    self->withPortMidi = 0;
            }
            else {
                    Server_warning(self, "Portmidi warning: no input device!\n");
                    self->withPortMidi = 0;
            }

            if (self->midi_output < num_devices) {
                if (self->midi_output == -1)
                    self->midi_output = Pm_GetDefaultOutputDeviceID();
                Server_debug(self, "Midi output device : %d.\n", self->midi_output);
                const PmDeviceInfo *outinfo = Pm_GetDeviceInfo(self->midi_output);
                if (outinfo != NULL) {
                    if ((strcmp(outinfo->name, "Microsoft MIDI Mapper") == 0 || 
                          strcmp(outinfo->name, "Microsoft GS Wavetable Synth") == 0) && 
                         self->allowMMMapper == 0) {
                        self->withPortMidiOut = 0;
                    }
                    else if (outinfo->output) {

                        Py_BEGIN_ALLOW_THREADS
                        if (!Pt_Started())
                            Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                        pmerr = Pm_OpenOutput(&be_data->midiout[0], self->midi_output, NULL, 100, NULL, NULL, 1);
                        Py_END_ALLOW_THREADS

                        if (pmerr) {
                            Server_warning(self,
                                     "Portmidi warning: could not open midi output %d (%s): %s\n",
                                     self->midi_output, outinfo->name, Pm_GetErrorText(pmerr));
                            self->withPortMidiOut = 0;

                            Py_BEGIN_ALLOW_THREADS
                            if (Pt_Started())
                                Pt_Stop();
                            Py_END_ALLOW_THREADS

                        }
                        else {
                            Server_debug(self, "Midi output (%s) opened.\n", outinfo->name);
                            self->midiout_count = 1;
                        }
                    }
                    else {
                        Server_warning(self, "Portmidi warning: Midi Device (%s), not an output device!\n", outinfo->name);
                        self->withPortMidiOut = 0;
                    }
                }
                else {
                    Server_debug(self, "Can't get midi output device info : %d.\n", self->midi_output);
                    self->withPortMidiOut = 0;
                }
            }
            else if (self->midi_output >= num_devices) {
                Server_debug(self, "Midi output device : all!\n");
                self->midiout_count = 0;

                Py_BEGIN_ALLOW_THREADS
                if (!Pt_Started())
                    Pt_Start(1, 0, 0); /* start a timer with millisecond accuracy */
                Py_END_ALLOW_THREADS

                for (i=0; i<num_devices; i++) {
                    const PmDeviceInfo *outinfo = Pm_GetDeviceInfo(i);
                    if (outinfo != NULL) {
                        if ((strcmp(outinfo->name, "Microsoft MIDI Mapper") == 0 || 
                              strcmp(outinfo->name, "Microsoft GS Wavetable Synth") == 0) && 
                             self->allowMMMapper == 0) {
                            continue;
                        }
                        else if (outinfo->output) {

                            Py_BEGIN_ALLOW_THREADS
                            pmerr = Pm_OpenOutput(&be_data->midiout[self->midiout_count], i, NULL, 100, NULL, NULL, 1);
                            Py_END_ALLOW_THREADS

                            if (pmerr) {
                                Server_warning(self,
                                     "Portmidi warning: could not open midi output %d (%s): %s\n",
                                     0, outinfo->name, Pm_GetErrorText(pmerr));
                            }
                            else {
                                Server_debug(self, "Midi output (%s) opened.\n", outinfo->name);
                                self->midiout_count++;
                            }
                        }
                    }
                }
                if (self->midiout_count == 0) {
                    self->withPortMidiOut = 0;
                }
            }
            else {
                    Server_warning(self, "Portmidi warning: no output device!\n");
                    self->withPortMidiOut = 0;
            }

            if (self->withPortMidi == 0 && self->withPortMidiOut == 0) {

                Py_BEGIN_ALLOW_THREADS
                if (Pt_Started())
                    Pt_Stop();
                Pm_Terminate();
                Py_END_ALLOW_THREADS

                Server_warning(self, "Portmidi closed.\n");
                ret = -1;
            }
        }
        else {
            Server_warning(self, "Portmidi warning: no midi device found!\nPortmidi closed.\n");
            self->withPortMidi = 0;
            self->withPortMidiOut = 0;

            Py_BEGIN_ALLOW_THREADS
            Pm_Terminate();
            Py_END_ALLOW_THREADS

            ret = -1;
        }
    }
    if (self->withPortMidi == 1) {
        self->midi_count = 0;
        for (i=0; i<self->midiin_count; i++) {
            Pm_SetFilter(be_data->midiin[i], PM_FILT_ACTIVE | PM_FILT_CLOCK);
        }
    }
    return ret;
}

int
Server_pm_deinit(Server *self)
{
    int i = 0;

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    /* An opened stream should be properly closed 
       but Pm_Close segfaults now and then so...
       This hack need to be tested on Windows and OSX. */

    if (self->withPortMidi == 1) {
        for (i=0; i<self->midiin_count; i++) {
            if (be_data->midiin[i] != NULL) {
/*
                Py_BEGIN_ALLOW_THREADS
                Pm_Close(be_data->midiin[i]);
                Py_END_ALLOW_THREADS
*/
            }
        }
    }
    if (self->withPortMidiOut == 1) {
        for (i=0; i<self->midiout_count; i++) {
            if (be_data->midiout[i] != NULL) {
/*
                Py_BEGIN_ALLOW_THREADS
                Pm_Close(be_data->midiout[i]);
                Py_END_ALLOW_THREADS
*/
            }
        }
    }
    if (self->withPortMidi == 1 || self->withPortMidiOut == 1) {

        Py_BEGIN_ALLOW_THREADS
        if (Pt_Started())
            Pt_Stop();
        Pm_Terminate();
        Py_END_ALLOW_THREADS

    }
    self->withPortMidi = 0;
    self->withPortMidiOut = 0;
    
    free(self->midi_be_data);

    return 0;
}

void
pm_noteout(Server *self, int pit, int vel, int chan, long timestamp)
{
    int i, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    if (chan == 0)
        buffer[0].message = Pm_Message(0x90, pit, vel);
    else
        buffer[0].message = Pm_Message(0x90 | (chan - 1), pit, vel);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_afterout(Server *self, int pit, int vel, int chan, long timestamp)
{
    int i, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    if (chan == 0)
        buffer[0].message = Pm_Message(0xA0, pit, vel);
    else
        buffer[0].message = Pm_Message(0xA0 | (chan - 1), pit, vel);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_ctlout(Server *self, int ctlnum, int value, int chan, long timestamp)
{
    int i, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    if (chan == 0)
        buffer[0].message = Pm_Message(0xB0, ctlnum, value);
    else
        buffer[0].message = Pm_Message(0xB0 | (chan - 1), ctlnum, value);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_programout(Server *self, int value, int chan, long timestamp)
{
    int i, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    if (chan == 0)
        buffer[0].message = Pm_Message(0xC0, value, 0);
    else
        buffer[0].message = Pm_Message(0xC0 | (chan - 1), value, 0);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_pressout(Server *self, int value, int chan, long timestamp)
{
    int i, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    if (chan == 0)
        buffer[0].message = Pm_Message(0xD0, value, 0);
    else
        buffer[0].message = Pm_Message(0xD0 | (chan - 1), value, 0);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_bendout(Server *self, int value, int chan, long timestamp)
{
    int i, lsb, msb, curtime;
    PmEvent buffer[1];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    buffer[0].timestamp = curtime + timestamp;
    lsb = value & 0x007F;
    msb = (value & (0x007F << 7)) >> 7;
    if (chan == 0)
        buffer[0].message = Pm_Message(0xE0, lsb, msb);
    else
        buffer[0].message = Pm_Message(0xE0 | (chan - 1), lsb, msb);
    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 1);
    }
}

void
pm_sysexout(Server *self, unsigned char *msg, long timestamp)
{
    int i, curtime;

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    for (i=0; i<self->midiout_count; i++) {
        Pm_WriteSysEx(be_data->midiout[i], curtime + timestamp, msg);
    }
}

void
pm_makenote(Server *self, int pit, int vel, int dur, int chan)
{
    int i, curtime, channel;
    PmEvent buffer[2];

    PyoPmBackendData *be_data = (PyoPmBackendData *) self->midi_be_data;

    curtime = Pt_Time();
    channel = (chan == 0) ? 0x90 : 0x90 | (chan - 1);

    buffer[0].timestamp = curtime;
    buffer[0].message = Pm_Message(channel, pit, vel);

    buffer[1].timestamp = curtime + dur;
    buffer[1].message = Pm_Message(channel, pit, 0);

    for (i=0; i<self->midiout_count; i++) {
        Pm_Write(be_data->midiout[i], buffer, 2);
    }
}

long
pm_get_current_time()
{
    if (Pt_Started())
        return Pt_Time();
    else
        return 0;
}

/* Query functions. */

PyObject *
portmidi_count_devices() {
    int numDevices;
	numDevices = Pm_CountDevices();
    return PyInt_FromLong(numDevices);
}

PyObject *
portmidi_list_devices() {
    int i;
    PySys_WriteStdout("MIDI devices:\n");
    for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input && info->output)
            PySys_WriteStdout("%d: IN/OUT, name: %s, interface: %s\n", i, info->name, info->interf);
        else if (info->input)
            PySys_WriteStdout("%d: IN, name: %s, interface: %s\n", i, info->name, info->interf);
        else if (info->output)
            PySys_WriteStdout("%d: OUT, name: %s, interface: %s\n", i, info->name, info->interf);
    }
    PySys_WriteStdout("\n");
    Py_RETURN_NONE;
}

PyObject *
portmidi_get_input_devices() {
	int n, i;
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);
    n = Pm_CountDevices();
    if (n < 0){
        PySys_WriteStdout("Portmidi warning: No Midi interface found.\n\n");
    }
    else {
        for (i=0; i < n; i++){
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            if (info->input){
                PyList_Append(list, PyUnicode_FromString(info->name));
                PyList_Append(list_index, PyInt_FromLong(i));
            }
        }
        PySys_WriteStdout("\n");
    }
    return Py_BuildValue("OO", list, list_index);
}

PyObject *
portmidi_get_output_devices() {
	int n, i;
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);
    n = Pm_CountDevices();
    if (n < 0){
        PySys_WriteStdout("Portmidi warning: No Midi interface found\n\n");
    }
    else {
        for (i=0; i < n; i++){
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            if (info->output){
                PyList_Append(list, PyUnicode_FromString(info->name));
                PyList_Append(list_index, PyInt_FromLong(i));
            }
        }
        PySys_WriteStdout("\n");
    }
    return Py_BuildValue("OO", list, list_index);
}

PyObject *
portmidi_get_default_input() {
    PmDeviceID i;

    i = Pm_GetDefaultInputDeviceID();
    if (i < 0)
        PySys_WriteStdout("Portmidi warning in pm_get_default_input: no midi input device found.\n");
    return PyInt_FromLong(i);
}


PyObject *
portmidi_get_default_output() {
    PmDeviceID i;
    i = Pm_GetDefaultOutputDeviceID();
    if (i < 0)
        PySys_WriteStdout("Portmidi warning in pm_get_default_output: no midi output device found.\n");
    return PyInt_FromLong(i);
}
