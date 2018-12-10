/**************************************************************************
 * Copyright 2018 Olivier Belanger                                        *
 *                                                                        *
 * This file is part of pyo-plug, an audio plugin using the python        *
 * module pyo to create the dsp.                                          *
 *                                                                        *
 * pyo-plug is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo-plug is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU LGPL along with pyo-plug.   *
 * If not, see <http://www.gnu.org/licenses/>.                            *
 *                                                                        *
 *************************************************************************/
#include "PyoClass.h"

Pyo::Pyo() {
    interpreter = nullptr;
}

Pyo::~Pyo() {
    if (interpreter != NULL) {
        pyo_end_interpreter(interpreter);
    }
}

void Pyo::setup(int _nChannels, int _bufferSize, int _sampleRate) {
    nChannels = _nChannels;
    bufferSize = _bufferSize;
    sampleRate = _sampleRate;
    interpreter = pyo_new_interpreter(sampleRate, bufferSize, nChannels);
    pyoInBuffer = reinterpret_cast<float*>(pyo_get_input_buffer_address(interpreter));
    pyoOutBuffer = reinterpret_cast<float*>(pyo_get_output_buffer_address(interpreter));
    pyoCallback = reinterpret_cast<callPtr*>(pyo_get_embedded_callback_address(interpreter));
    pyoId = pyo_get_server_id(interpreter);
    pyo_set_server_params(interpreter, sampleRate, bufferSize);
}

void Pyo::setbpm(double bpm) {
    pyo_set_bpm(interpreter, bpm);
}

void Pyo::process(AudioBuffer<float>& buffer) {
    for (int channel = 0; channel < nChannels; ++channel) {
        float *channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < bufferSize; sample++) {
            pyoInBuffer[sample*nChannels+channel] = channelData[sample];
        }
    }
    pyoCallback(pyoId);
    for (int channel = 0; channel < nChannels; ++channel) {
        float *channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < bufferSize; sample++) {
            channelData[sample] = pyoOutBuffer[sample*nChannels+channel];
        }
    }
}

int Pyo::loadfile(const char *file, int add) {
    return pyo_exec_file(interpreter, file, pyoMsg, add);
}

int Pyo::loadfile(const String &file, int add) {
    return pyo_exec_file(interpreter, file.getCharPointer(), pyoMsg, add);
}

int Pyo::exec(const char *_msg) {
    strcpy(pyoMsg, _msg);
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::exec(const String &_msg) {
    strcpy(pyoMsg, _msg.getCharPointer());
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::value(const char *name, float value) {
    sprintf(pyoMsg, "%s.value=%f", name, value);
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::value(const String &name, float value) {
    const char * _name = name.getCharPointer();
    sprintf(pyoMsg, "%s.value=%f", _name, value);
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::value(const char *name, float *value, int len) {
    char fchar[32];
    sprintf(pyoMsg, "%s.value=[", name);
    for (int i=0; i<len; i++) {
        sprintf(fchar, "%f,", value[i]);
        strcat(pyoMsg, fchar);
    }
    strcat(pyoMsg, "]");
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::value(const String &name, float *value, int len) {
    char fchar[32];
    const char * _name = name.getCharPointer();
    sprintf(pyoMsg, "%s.value=[", _name);
    for (int i=0; i<len; i++) {
        sprintf(fchar, "%f,", value[i]);
        strcat(pyoMsg, fchar);
    }
    strcat(pyoMsg, "]");
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::set(const char *name, float value) {
    sprintf(pyoMsg, "%s=%f", name, value);
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::set(const String &name, float value) {
    const char * _name = name.getCharPointer();
    sprintf(pyoMsg, "%s=%f", _name, value);
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::set(const char *name, float *value, int len) {
    char fchar[32];
    sprintf(pyoMsg, "%s=[", name);
    for (int i=0; i<len; i++) {
        sprintf(fchar, "%f,", value[i]);
        strcat(pyoMsg, fchar);
    }
    strcat(pyoMsg, "]");
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

int Pyo::set(const String &name, float *value, int len) {
    char fchar[32];
    const char * _name = name.getCharPointer();
    sprintf(pyoMsg, "%s=[", _name);
    for (int i=0; i<len; i++) {
        sprintf(fchar, "%f,", value[i]);
        strcat(pyoMsg, fchar);
    }
    strcat(pyoMsg, "]");
    return pyo_exec_statement(interpreter, pyoMsg, 0);
}

void Pyo::midi(int status, int data1, int data2) {
    pyo_add_midi_event(interpreter, status, data1, data2);
}

void Pyo::clear() {
    pyo_server_reboot(interpreter);
}
