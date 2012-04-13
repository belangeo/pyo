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

#include <Python.h>
#include <math.h>
#include "portaudio.h"
#include "sndfile.h"
#include "pyomodule.h"
#include "servermodule.h"
#include "streammodule.h"
#include "dummymodule.h"
#include "tablemodule.h"
#include "matrixmodule.h"

/****** Portaudio utilities ******/
static void portaudio_assert(PaError ecode, const char* cmdName) {
    if (ecode != paNoError) {
        const char* eText = Pa_GetErrorText(ecode);
        if (!eText) {
            eText = "???";
        }
        fprintf(stderr, "portaudio error in %s: %s\n", cmdName, eText);
        Pa_Terminate();
    }
}

#define portaudio_count_host_apis_info \
"\nReturns the number of host apis found by Portaudio.\n\npa_count_host_apis()\n\nExamples:\n\n    \
>>> c = pa_count_host_apis()\n    \
>>> print c\n    \
>>> 1\n\n"

static PyObject *
portaudio_count_host_apis(){
    PaError err;
    PaHostApiIndex numApis; 

    err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
	else {
        numApis = Pa_GetHostApiCount();
        if( numApis < 0 )
            portaudio_assert(numApis, "Pa_GetHostApiCount");
        return PyInt_FromLong(numApis);
    }
}

#define portaudio_list_host_apis_info \
"\nPrints a list of all host apis found by Portaudio.\n\npa_list_host_apis()\n\nExamples:\n\n    \
>>> pa_list_host_apis()\n    \
>>> index: 0, id: 5, name: Core Audio, num devices: 6, default in: 0, default out: 2\n\n"

static PyObject*
portaudio_list_host_apis(){
    PaError err;
    PaHostApiIndex n, i;
	
    err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
	}
    else {
        n = Pa_GetHostApiCount();
        if (n < 0){
            portaudio_assert(n, "Pa_GetHostApiCount");
        }
        else {
            for (i=0; i < n; ++i){
                const PaHostApiInfo *info = Pa_GetHostApiInfo(i);
                assert(info);
                
                fprintf(stdout, "index: %i, id: %i, name: %s, num devices: %i, default in: %i, default out: %i\n", i, (int)info->type, info->name, (int)info->deviceCount, (int)info->defaultInputDevice, (int)info->defaultOutputDevice);
            }
        }        
    }
    Py_RETURN_NONE;
}

#define portaudio_get_default_host_api_info \
"\nReturns the index number of Portaudio's default host api.\n\npa_get_default_host_api()\n\nExamples:\n\n    \
>>> h = pa_get_default_host_api()\n    \
>>> print h\n    \
>>> 0\n\n"

static PyObject*
portaudio_get_default_host_api(){
    PaError err;
    PaHostApiIndex i;
	
    err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        i = Pa_GetDefaultHostApi();
        const PaHostApiInfo *info = Pa_GetHostApiInfo(i);
        assert(info);
        
        fprintf(stdout, "index: %i, id: %i, name: %s, num devices: %i, default in: %i, default out: %i\n", i, (int)info->type, info->name, (int)info->deviceCount, (int)info->defaultInputDevice, (int)info->defaultOutputDevice);
        
        return PyInt_FromLong(i);
    }
}

#define portaudio_count_devices_info \
"\nReturns the number of devices found by Portaudio.\n\npa_count_devices()\n\nExamples:\n\n    \
>>> c = pa_count_devices()\n    \
>>> print c\n    \
>>> 6\n\n"

static PyObject*
portaudio_count_devices(){
    PaError err;
    PaDeviceIndex numDevices;
	
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
	else {
        numDevices = Pa_GetDeviceCount();
        if( numDevices < 0 )
            portaudio_assert(numDevices, "Pa_GetDeviceCount");
        return PyInt_FromLong(numDevices);        
    }

}

#define portaudio_list_devices_info \
"\nPrints a list of all devices found by Portaudio.\n\npa_list_devices()\n\nExamples:\n\n    \
>>> pa_list_devices()\n    \
>>> AUDIO devices:\n    \
>>> 0: IN, name: Built-in Microphone, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n    \
>>> 1: IN, name: Built-in Input, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n    \
>>> 2: OUT, name: Built-in Output, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n    \
>>> 3: IN, name: UA-4FX, host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n    \
>>> 3: OUT, name: UA-4FX, host api index: 0, default sr: 44100 Hz, latency: 0.003061 s\n    \
>>> 4: IN, name: Soundflower (2ch), host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n    \
>>> 4: OUT, name: Soundflower (2ch), host api index: 0, default sr: 44100 Hz, latency: 0.000000 s\n    \
>>> 5: IN, name: Soundflower (16ch), host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n    \
>>> 5: OUT, name: Soundflower (16ch), host api index: 0, default sr: 44100 Hz, latency: 0.000000 s\n\n"

static PyObject*
portaudio_list_devices(){
    PaError err;
    PaDeviceIndex n, i;
	
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        n = Pa_GetDeviceCount();
        if (n < 0){
            portaudio_assert(n, "Pa_GetDeviceCount");
        }
        else {
            printf("AUDIO devices:\n");
            for (i=0; i < n; ++i){
                const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
                assert(info);
                
                if (info->maxInputChannels > 0)
                    fprintf(stdout, "%i: IN, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);
                
                if (info->maxOutputChannels > 0)
                    fprintf(stdout, "%i: OUT, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowOutputLatency);
            }
            printf("\n");
        }        
    }
    Py_RETURN_NONE;
}

#define portaudio_get_output_devices_info \
"\nReturns output devices (device names, device indexes) found by Portaudio.\n\npa_get_output_devices()\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortaudio index of each device.\n\nExamples:\n\n    \
>>> outs = pa_get_output_devices()\n    \
>>> print outs\n    \
>>> (['Built-in Output', 'UA-4FX', 'Soundflower (2ch)', 'Soundflower (16ch)'], [2, 3, 4, 5])\n\n"

static PyObject*
portaudio_get_output_devices(){
    PaError err;
    PaDeviceIndex n, i;

    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);
    	
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        n = Pa_GetDeviceCount();
        if (n < 0){
            portaudio_assert(n, "Pa_GetDeviceCount");
            Py_RETURN_NONE;
        }
        else {
            for (i=0; i < n; ++i){
                const PaDeviceInfo *info=Pa_GetDeviceInfo(i);
                assert(info);
                
                if (info->maxOutputChannels > 0){
                    fprintf(stdout, "%i: OUT, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowOutputLatency);
                    PyList_Append(list, PyString_FromString(info->name));
                    PyList_Append(list_index, PyInt_FromLong(i));
                }
            }
            return Py_BuildValue("OO", list, list_index);
        }        
    }
}

#define portaudio_get_input_devices_info \
"\nReturns input devices (device names, device indexes) found by Portaudio.\n\npa_get_input_devices()\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortaudio index of each device.\n\nExamples:\n\n    \
>>> ins = pa_get_input_devices()\n    \
>>> print ins\n    \
>>> (['Built-in Microphone', 'Built-in Input', 'UA-4FX', 'Soundflower (2ch)', 'Soundflower (16ch)'],\n    \
>>> [0, 1, 3, 4, 5])\n\n"

static PyObject*
portaudio_get_input_devices(){
    PaError err;
    PaDeviceIndex n, i;
    
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);
    
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        n = Pa_GetDeviceCount();
        if (n < 0){
            portaudio_assert(n, "Pa_GetDeviceCount");
            Py_RETURN_NONE;
        }
        else {
            for (i=0; i < n; ++i){
                const PaDeviceInfo *info=Pa_GetDeviceInfo(i);
                assert(info);
                
                if (info->maxInputChannels > 0){
                    fprintf(stdout, "%i: IN, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);
                    PyList_Append(list, PyString_FromString(info->name));
                    PyList_Append(list_index, PyInt_FromLong(i));
                }
            }
            return Py_BuildValue("OO", list, list_index);            
        }        
    }
}

#define portaudio_get_default_input_info \
"\nReturns the index number of Portaudio's default input device.\n\npa_get_default_input()\n\nExamples:\n\n    \
>>> names, indexes = pa_get_input_devices()\n    \
>>> name = names[indexes.index(pa_get_default_input())]\n    \
>>> print name\n    \
>>> 'Built-in Microphone'\n\n"

static PyObject*
portaudio_get_default_input(){
    PaError err;
    PaDeviceIndex i;
	
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        i = Pa_GetDefaultInputDevice();
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        assert(info);
        
        if (info->maxInputChannels > 0)
            fprintf(stdout, "%i: IN, name: %s, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);

        return PyInt_FromLong(i);        
    }

}

#define portaudio_get_default_output_info \
"\nReturns the index number of Portaudio's default output device.\n\npa_get_default_output()\n\nExamples:\n\n    \
>>> names, indexes = pa_get_output_devices()\n    \
>>> name = names[indexes.index(pa_get_default_output())]\n    \
>>> print name\n    \
>>> 'UA-4FX'\n\n"

static PyObject*
portaudio_get_default_output(){
    PaError err;
    PaDeviceIndex i;
	
	err = Pa_Initialize();
    if (err != paNoError) {
        portaudio_assert(err, "Pa_Initialize");
		Py_RETURN_NONE;
	}
    else {
        i = Pa_GetDefaultOutputDevice();
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        assert(info);
        
        if (info->maxOutputChannels > 0)
            fprintf(stdout, "%i: OUT, name: %s, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);
        
        return PyInt_FromLong(i);
        
    }
}

/****** Portmidi utilities ******/
#define portmidi_count_devices_info \
"\nReturns the number of devices found by Portmidi.\n\npm_count_devices()\n\nExamples:\n\n    \
>>> c = pm_count_devices()\n    \
>>> print c\n    \
>>> 6\n\n"

static PyObject *
portmidi_count_devices(){
    int numDevices;
	numDevices = Pm_CountDevices();
    return PyInt_FromLong(numDevices);
}

#define portmidi_list_devices_info \
"\nPrints a list of all devices found by Portmidi.\n\npm_list_devices()\n\nExamples:\n\n    \
>>> pm_list_devices()\n    \
>>> MIDI devices:\n    \
>>> 0: IN, name: IAC Driver Bus 1, interface: CoreMIDI\n    \
>>> 1: IN, name: from MaxMSP 1, interface: CoreMIDI\n    \
>>> 2: IN, name: from MaxMSP 2, interface: CoreMIDI\n    \
>>> 3: OUT, name: IAC Driver Bus 1, interface: CoreMIDI\n    \
>>> 4: OUT, name: to MaxMSP 1, interface: CoreMIDI\n    \
>>> 5: OUT, name: to MaxMSP 2, interface: CoreMIDI\n\n"

static PyObject *
portmidi_list_devices(){
    int i;
    printf("MIDI devices:\n");
    for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input && info->output) 
            printf("%d: IN/OUT, name: %s, interface: %s\n", i, info->name, info->interf);
        else if (info->input) 
            printf("%d: IN, name: %s, interface: %s\n", i, info->name, info->interf);
        else if (info->output) 
            printf("%d: OUT, name: %s, interface: %s\n", i, info->name, info->interf);
    }
    printf("\n");
    Py_RETURN_NONE;
}

#define portmidi_get_input_devices_info \
"\nReturns midi input devices (device names, device indexes) found by Portmidi.\n\npm_get_input_devices()\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortmidi index of each device.\n\nExamples:\n\n    \
>>> ins = pm_get_input_devices()\n    \
>>> print ins\n    \
>>> (['IAC Driver Bus 1', 'from MaxMSP 1', 'from MaxMSP 2'], [0, 1, 2])\n\n"

static PyObject*
portmidi_get_input_devices(){
	int n, i;
    
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);

    n = Pm_CountDevices();
    if (n < 0){
        printf("Portmidi warning: No Midi interface found\n\n");
    }
    else {
        for (i=0; i < n; i++){
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        
            if (info->input){
                printf("%d: IN, name: %s, interface: %s\n", i, info->name, info->interf);
                PyList_Append(list, PyString_FromString(info->name));
                PyList_Append(list_index, PyInt_FromLong(i));
            }
        }
        printf("\n");
    }
    return Py_BuildValue("OO", list, list_index);
}

#define portmidi_get_output_devices_info \
"\nReturns midi output devices (device names, device indexes) found by Portmidi.\n\npm_get_output_devices()\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortmidi index of each device.\n\nExamples:\n\n    \
>>> outs = pm_get_output_devices()\n    \
>>> print outs\n    \
>>> (['IAC Driver Bus 1', 'to MaxMSP 1', 'to MaxMSP 2'], [3, 4, 5])\n\n"

static PyObject*
portmidi_get_output_devices(){
	int n, i;
    
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);
    
    n = Pm_CountDevices();
    if (n < 0){
        printf("Portmidi warning: No Midi interface found\n\n");
    }
    else {
        for (i=0; i < n; i++){
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
            
            if (info->output){
                printf("%d: OUT, name: %s, interface: %s\n", i, info->name, info->interf);
                PyList_Append(list, PyString_FromString(info->name));
                PyList_Append(list_index, PyInt_FromLong(i));
            }
        }
        printf("\n");
    }
    return Py_BuildValue("OO", list, list_index);
}

#define portmidi_get_default_input_info \
"\nReturns the index number of Portmidi's default input device.\n\npm_get_default_input()\n\nExamples:\n\n    \
>>> names, indexes = pm_get_input_devices()\n    \
>>> name = names[indexes.index(pm_get_default_input())]\n    \
>>> print name\n    \
>>> 'IAC Driver Bus 1'\n\n"

static PyObject *
portmidi_get_default_input(){
    PmDeviceID i;
    
    i = Pm_GetDefaultInputDeviceID();
    if (i >= 0) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input) 
            printf("%d: IN, name: %s, interface: %s\n", i, info->name, info->interf);        
    }
    else {
        printf("pm_get_default_input: no midi input device found.\n");
    }

    return PyInt_FromLong(i);
}

#define portmidi_get_default_output_info \
"\nReturns the index number of Portmidi's default output device.\n\npm_get_default_output()\n\nExamples:\n\n    \
>>> names, indexes = pm_get_output_devices()\n    \
>>> name = names[indexes.index(pm_get_default_output())]\n    \
>>> print name\n    \
>>> 'IAC Driver Bus 1'\n\n"

static PyObject *
portmidi_get_default_output(){
    PmDeviceID i;
    
    i = Pm_GetDefaultOutputDeviceID();
    if (i >= 0) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->output) 
            printf("%d: OUT, name: %s, interface: %s\n", i, info->name, info->interf);        
    }
    else {
        printf("pm_get_default_output: no midi output device found.\n");
    }
    
    return PyInt_FromLong(i);
}

/****** Libsndfile utilities ******/
#define sndinfo_info \
"\nRetrieve informations about a soundfile.\n\nsndinfo(path, print=False)\n\n\
Prints the infos of the given soundfile to the console and returns a tuple containing:\n\n(number of frames, duration in seconds, sampling rate,\nnumber of channels, file format, sample type).\n\nParameters:\n\n    \
path : string\n        Path of a valid soundfile.\n    \
print : boolean, optional\n        If True, sndinfo will print sound infos to the console. Defaults to False.\n\nExamples:\n\n    \
>>> path = SNDS_PATH + '/transparent.aif'\n    \
>>> print path\n    \
>>> /usr/lib/python2.7/dist-packages/pyolib/snds/transparent.aif\n    \
>>> info = sndinfo(path)\n    \
>>> print info\n    \
>>> (29877, 0.6774829931972789, 44100.0, 1, 'AIFF', '16 bit int')\n\n"

static PyObject *
sndinfo(PyObject *self, PyObject *args, PyObject *kwds) {
    
    SNDFILE *sf;
    SF_INFO info;
    char *pathtmp;
    char *path;
    char fileformat[5];
    char *sampletype;
    int format;
    int subformat;
    int print = 0;

    static char *kwlist[] = {"path", "print", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s|i", kwlist, &pathtmp, &print))
        Py_RETURN_NONE;

    path = malloc(strlen(pathtmp)+1);
    strcpy(path, pathtmp);

    /* Open the sound file. */
    info.format = 0;
    sf = sf_open(path, SFM_READ, &info);
    if (sf == NULL) {
        printf("Failed to open the file.\n");
        Py_RETURN_NONE;
    }
    else {
        /* Retrieve file format */
        format = (int)info.format;
        if (format > SF_FORMAT_WAV && format < SF_FORMAT_AIFF) {
            strcpy(fileformat, "WAVE");
            subformat = format - SF_FORMAT_WAV;
        }
        else if (format > SF_FORMAT_AIFF && format < SF_FORMAT_AU) {
            strcpy(fileformat, "AIFF");
            subformat = format - SF_FORMAT_AIFF;
        }
        else {
            strcpy(fileformat, "????");
            subformat = -1;
        }
        /* Retrieve sample type */
        if (subformat != -1) {
            switch (subformat) {
                case SF_FORMAT_PCM_S8:
                    sampletype = malloc(strlen("s8 bit int") + 1);
                    strcpy(sampletype, "s8 bit int");
                    break;
                case SF_FORMAT_PCM_U8:
                    sampletype = malloc(strlen("u8 bit int") + 1);
                    strcpy(sampletype, "u8 bit int");
                    break;
                case SF_FORMAT_PCM_16:
                    sampletype = malloc(strlen("16 bit int") + 1);
                    strcpy(sampletype, "16 bit int");
                    break;
                case SF_FORMAT_PCM_24:
                    sampletype = malloc(strlen("24 bit int") + 1);
                    strcpy(sampletype, "24 bit int");
                    break;
                case SF_FORMAT_PCM_32:
                    sampletype = malloc(strlen("32 bit int") + 1);
                    strcpy(sampletype, "32 bit int");
                    break;
                case SF_FORMAT_FLOAT:
                    sampletype = malloc(strlen("32 bit float") + 1);
                    strcpy(sampletype, "32 bit float");
                    break;
                case SF_FORMAT_DOUBLE:
                    sampletype = malloc(strlen("64 bit float") + 1);
                    strcpy(sampletype, "64 bit float");
                    break;
                default:
                    sampletype = malloc(strlen("Unknown...") + 1);
                    strcpy(sampletype, "Unknown...");
                    break;
            }
        }
        else {
            sampletype = malloc(strlen("Unknown...") + 1);
            strcpy(sampletype, "Unknown...");
        }
    
        if (print)
            fprintf(stdout, "name: %s\nnumber of frames: %i\nduration: %.4f sec\nsr: %.2f\nchannels: %i\nformat: %s\nsample type: %s\n", 
                    path, (int)info.frames, ((float)info.frames / info.samplerate), (float)info.samplerate, (int)info.channels, fileformat, sampletype);
        PyObject *sndinfo = PyTuple_Pack(6, PyInt_FromLong(info.frames), PyFloat_FromDouble((float)info.frames / info.samplerate), PyFloat_FromDouble(info.samplerate), 
                                         PyInt_FromLong(info.channels), PyString_FromString(fileformat), PyString_FromString(sampletype));
        sf_close(sf);
        free(path);
        free(sampletype);
        return sndinfo;
    }
}    

#define savefile_info \
"\nCreates an audio file from a list of floats.\n\nsavefile(samples, path, sr=44100, channels=1, fileformat=0, sampletype=0)\n\nParameters:\n\n    \
samples : list of floats\n        list of samples data, or list of list of samples data if more than 1 channels.\n    \
path : string\n        Full path (including extension) of the new file.\n    \
sr : int, optional\n        Sampling rate of the new file. Defaults to 44100.\n    \
channels : int, optional\n        number of channels of the new file. Defaults to 1.\n    \
fileformat : int, optional\n        Format type of the new file. Defaults to 0. Supported formats are:\n    \
        0 : WAVE - Microsoft WAV format (little endian) {.wav, .wave}\n    \
        1 : AIFF - Apple/SGI AIFF format (big endian) {.aif, .aiff}\n    \
sampletype ; int, optional\n        Bit depth encoding of the audio file. Defaults to 0. Supported types are:\n    \
        0 : 16 bit int\n    \
        1 : 24 bit int\n    \
        2 : 32 bit int\n    \
        3 : 32 bit float\n    \
        4 : 64 bit float\n\n\
Examples:\n\n    \
>>> from random import uniform\n    \
>>> import os\n    \
>>> home = os.path.expanduser('~')\n    \
>>> sr, dur, chnls, path = 44100, 5, 2, os.path.join(home, 'noise.aif')\n    \
>>> samples = [[uniform(-0.5,0.5) for i in range(sr*dur)] for i in range(chnls)]\n    \
>>> savefile(samples=samples, path=path, sr=sr, channels=chnls, fileformat=1, sampletype=1)\n\n"

static PyObject *
savefile(PyObject *self, PyObject *args, PyObject *kwds) {
    int i, j, size;
    char *recpath;
    PyObject *samples;
    MYFLT *sampsarray;
    int sr = 44100;
    int channels = 1;
    int fileformat = 0;
    int sampletype = 0;
    SNDFILE *recfile;
    SF_INFO recinfo;
    static char *kwlist[] = {"samples", "path", "sr", "channels", "fileformat", "sampletype", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os|iiii", kwlist, &samples, &recpath, &sr, &channels, &fileformat, &sampletype))
        return PyInt_FromLong(-1);
    
    recinfo.samplerate = sr;
    recinfo.channels = channels;
    switch (fileformat) {
        case 0:
            recinfo.format = SF_FORMAT_WAV;
            break;
        case 1:
            recinfo.format = SF_FORMAT_AIFF;
            break;
    }
    switch (sampletype) {
        case 0:
            recinfo.format = recinfo.format | SF_FORMAT_PCM_16;
            break;
        case 1:
            recinfo.format = recinfo.format | SF_FORMAT_PCM_24;
            break;
        case 2:
            recinfo.format = recinfo.format | SF_FORMAT_PCM_32;
            break;
        case 3:
            recinfo.format = recinfo.format | SF_FORMAT_FLOAT;
            break;
        case 4:
            recinfo.format = recinfo.format | SF_FORMAT_DOUBLE;
            break;
    }
    
    if (channels == 1) {
        size = PyList_Size(samples);
        sampsarray = (MYFLT *)malloc(size * sizeof(MYFLT));
        for (i=0; i<size; i++) {
            sampsarray[i] = PyFloat_AS_DOUBLE(PyList_GET_ITEM(samples, i));
        }
    }
    else {
        if (PyList_Size(samples) != channels) {
            printf("Samples list size and channels must be the same!\n");
            return PyInt_FromLong(-1);
        }
        size = PyList_Size(PyList_GET_ITEM(samples, 0)) * channels;
        sampsarray = (MYFLT *)malloc(size * sizeof(MYFLT));
        for (i=0; i<(size/channels); i++) {
            for (j=0; j<channels; j++) {
                sampsarray[i*channels+j] = PyFloat_AS_DOUBLE(PyList_GET_ITEM(PyList_GET_ITEM(samples, j), i));
            }
        }    
    }    
    if (! (recfile = sf_open(recpath, SFM_WRITE, &recinfo))) {
        printf ("Not able to open output file %s.\n", recpath);
        return PyInt_FromLong(-1);
    }
    SF_WRITE(recfile, sampsarray, size);
    sf_close(recfile);
    free(sampsarray);
    
    Py_RETURN_NONE;    
}

/****** Sampling rate conversions ******/
#define upsamp_info \
"\nIncreases the sampling rate of an audio file.\n\nupsamp(path, outfile, up=4, order=128)\n\nParameters:\n\n    \
path : string\n        Full path (including extension) of the audio file to convert.\n    \
outfile : string\n        Full path (including extension) of the new file.\n    \
up : int, optional\n        Upsampling factor. Defaults to 4.\n    \
order : int, optional\n        Length, in samples, of the anti-aliasing lowpass filter. Defaults to 128.\n\n\
Examples:\n\n    \
>>> import os\n    \
>>> home = os.path.expanduser('~')\n    \
>>> f = SNDS_PATH+'/transparent.aif'\n    \
>>> # upsample a signal 3 times\n    \
>>> upfile = os.path.join(home, 'trans_upsamp_2.aif')\n    \
>>> upsamp(f, upfile, 2, 256)\n    \
>>> # downsample the upsampled signal 3 times\n    \
>>> downfile = os.path.join(home, 'trans_downsamp_3.aif')\n    \
>>> downsamp(upfile, downfile, 3, 256)\n\n"

#define downsamp_info \
"\nDecreases the sampling rate of an audio file.\n\ndownsamp(path, outfile, down=4, order=128)\n\nParameters:\n\n    \
path : string\n        Full path (including extension) of the audio file to convert.\n    \
outfile : string\n        Full path (including extension) of the new file.\n    \
down : int, optional\n        Downsampling factor. Defaults to 4.\n    \
order : int, optional\n        Length, in samples, of the anti-aliasing lowpass filter. Defaults to 128.\n\n\
Examples:\n\n    \
>>> import os\n    \
>>> home = os.path.expanduser('~')\n    \
>>> f = SNDS_PATH+'/transparent.aif'\n    \
>>> # upsample a signal 3 times\n    \
>>> upfile = os.path.join(home, 'trans_upsamp_2.aif')\n    \
>>> upsamp(f, upfile, 2, 256)\n    \
>>> # downsample the upsampled signal 3 times\n    \
>>> downfile = os.path.join(home, 'trans_downsamp_3.aif')\n    \
>>> downsamp(upfile, downfile, 3, 256)\n\n"

MYFLT HALF_BLACKMAN[513] = {5.999999848427251e-05, 6.0518785176100209e-05, 6.2141079979483038e-05, 6.4805892179720104e-05, 6.8557070335373282e-05, 7.335994450841099e-05, 7.9284000094048679e-05, 8.6251806351356208e-05, 9.4344803073909134e-05, 0.00010353395919082686, 0.0001138320003519766, 0.0001252776273759082, 0.00013784394832327962, 0.00015158756286837161, 0.00016646583389956504, 0.00018252100562676787, 0.00019978794443886727, 0.00021828405442647636, 0.00023800843337085098, 0.00025901006301864982, 0.0002812814200296998, 0.00030484798480756581, 0.00032972017652355134, 0.00035596732050180435, 0.00038358545862138271, 0.0004126313142478466, 0.00044307118514552712, 0.00047501336666755378, 0.00050844199722632766, 0.00054337596520781517, 0.00057988864136859775, 0.00061800965340808034, 0.00065775914117693901, 0.000699152413289994, 0.00074227934237569571, 0.00078715570271015167, 0.00083377416012808681, 0.00088227324886247516, 0.0009326221770606935, 0.00098489224910736084, 0.0010391034884378314, 0.0010953464079648256, 0.001153626712039113, 0.0012140328763052821, 0.0012765693245455623, 0.001341317780315876, 0.0014083425048738718, 0.0014776336029171944, 0.001549328095279634, 0.0016234172508120537, 0.0017000052612274885, 0.0017791179707273841, 0.0018608199898153543, 0.0019451823318377137, 0.0020322385244071484, 0.0021220885682851076, 0.0022147782146930695, 0.0023103870917111635, 0.0024089745711535215, 0.0025105655658990145, 0.0026152802165597677, 0.0027231767307966948, 0.0028343265876173973, 0.0029487889260053635, 0.0030666270758956671, 0.0031879479065537453, 0.0033128033392131329, 0.0034412886016070843, 0.0035734693519771099, 0.0037094042636454105, 0.0038491983432322741, 0.0039929361082613468, 0.0041406778618693352, 0.004292510449886322, 0.0044485158286988735, 0.004608803428709507, 0.0047734435647726059, 0.0049425391480326653, 0.005116121843457222, 0.0052943285554647446, 0.0054772454313933849, 0.0056649716570973396, 0.0058575910516083241, 0.0060551739297807217, 0.0062578483484685421, 0.0064656869508326054, 0.0066788033582270145, 0.0068972636945545673, 0.0071212123148143291, 0.0073507223278284073, 0.007585874292999506, 0.0078268209472298622, 0.0080736298114061356, 0.0083263935521245003, 0.0085852388292551041, 0.0088502718135714531, 0.0091215828433632851, 0.0093993041664361954, 0.0096835149452090263, 0.0099743194878101349, 0.010271874256432056, 0.010576239787042141, 0.010887577198445797, 0.01120593398809433, 0.01153149176388979, 0.011864298023283482, 0.012204526923596859, 0.012552268803119659, 0.012907638214528561, 0.013270745985209942, 0.013641729019582272, 0.014020670205354691, 0.014407743699848652, 0.014803030528128147, 0.015206646174192429, 0.015618747100234032, 0.016039434820413589, 0.0164688341319561, 0.01690707728266716, 0.017354268580675125, 0.017810540273785591, 0.018276045098900795, 0.018750874325633049, 0.019235162064433098, 0.01972905732691288, 0.020232660695910454, 0.020746102556586266, 0.021269544959068298, 0.021803082898259163, 0.022346852347254753, 0.022900991141796112, 0.023465657606720924, 0.024040926247835159, 0.024626968428492546, 0.025223886594176292, 0.025831848382949829, 0.026450937613844872, 0.02708134613931179, 0.027723187580704689, 0.02837657742202282, 0.029041649773716927, 0.029718579724431038, 0.030407454818487167, 0.03110840916633606, 0.03182162344455719, 0.032547183334827423, 0.033285260200500488, 0.034035947173833847, 0.034799445420503616, 0.035575807094573975, 0.036365248262882233, 0.037167854607105255, 0.037983741611242294, 0.038813117891550064, 0.039656046777963638, 0.040512733161449432, 0.041383236646652222, 0.042267743498086929, 0.043166369199752808, 0.044079229235649109, 0.045006513595581055, 0.045948274433612823, 0.046904727816581726, 0.047875978052616119, 0.048862140625715256, 0.049863360822200775, 0.050879742950201035, 0.051911454647779465, 0.052958611398935318, 0.054021358489990234, 0.055099856108427048, 0.056194130331277847, 0.057304393500089645, 0.0584307461977005, 0.059573329985141754, 0.060732249170541763, 0.061907690018415451, 0.063099689781665802, 0.064308419823646545, 0.065534010529518127, 0.066776573657989502, 0.068036213517189026, 0.069313108921051025, 0.070607319474220276, 0.071918979287147522, 0.073248207569122314, 0.074595145881175995, 0.075959883630275726, 0.07734256237745285, 0.078743241727352142, 0.080162093043327332, 0.08159918338060379, 0.083054669201374054, 0.084528610110282898, 0.086021184921264648, 0.087532415986061096, 0.089062459766864777, 0.090611375868320465, 0.092179328203201294, 0.093766368925571442, 0.095372647047042847, 0.096998192369937897, 0.098643146455287933, 0.10030759125947952, 0.10199161618947983, 0.10369531810283661, 0.10541882365942001, 0.10716214776039124, 0.10892540961503983, 0.11070869863033295, 0.11251209676265717, 0.11433566361665726, 0.11617954820394516, 0.11804373562335968, 0.11992833018302917, 0.12183342128992081, 0.12375906854867935, 0.12570534646511078, 0.12767235934734344, 0.12966008484363556, 0.13166864216327667, 0.13369807600975037, 0.13574843108654022, 0.13781978189945221, 0.13991223275661469, 0.14202572405338287, 0.14416038990020752, 0.14631621539592743, 0.14849328994750977, 0.15069162845611572, 0.15291133522987366, 0.15515235066413879, 0.15741473436355591, 0.15969853103160858, 0.1620037853717804, 0.16433051228523254, 0.16667875647544861, 0.16904847323894501, 0.17143970727920532, 0.17385250329971313, 0.17628686130046844, 0.17874275147914886, 0.18122029304504395, 0.18371935188770294, 0.18623997271060944, 0.18878217041492462, 0.1913459450006485, 0.19393126666545868, 0.19653819501399994, 0.19916661083698273, 0.20181652903556824, 0.20448794960975647, 0.20718084275722504, 0.20989517867565155, 0.2126309871673584, 0.21538813412189484, 0.21816661953926086, 0.2209663987159729, 0.22378745675086975, 0.22662979364395142, 0.22949324548244476, 0.23237781226634979, 0.23528343439102173, 0.23821006715297699, 0.24115763604640961, 0.24412614107131958, 0.24711540341377258, 0.25012537837028503, 0.25315603613853455, 0.25620725750923157, 0.25927898287773132, 0.26237118244171143, 0.26548364758491516, 0.26861634850502014, 0.27176916599273682, 0.27494201064109802, 0.2781347930431366, 0.28134745359420776, 0.28457978367805481, 0.28783169388771057, 0.29110309481620789, 0.29439383745193481, 0.29770383238792419, 0.30103299021720886, 0.30438104271888733, 0.30774796009063721, 0.31113356351852417, 0.31453773379325867, 0.31796032190322876, 0.3214012086391449, 0.32486018538475037, 0.32833707332611084, 0.33183175325393677, 0.33534407615661621, 0.33887386322021484, 0.34242099523544312, 0.34598517417907715, 0.34956631064414978, 0.35316416621208191, 0.35677862167358398, 0.3604094386100769, 0.36405652761459351, 0.36771953105926514, 0.37139829993247986, 0.37509268522262573, 0.37880244851112366, 0.38252738118171692, 0.38626736402511597, 0.39002197980880737, 0.39379113912582397, 0.39757457375526428, 0.40137210488319397, 0.40518343448638916, 0.40900847315788269, 0.41284680366516113, 0.41669824719429016, 0.42056256532669067, 0.42443951964378357, 0.42832884192466736, 0.4322303831577301, 0.43614372611045837, 0.44006863236427307, 0.44400492310523987, 0.4479522705078125, 0.45191043615341187, 0.45587921142578125, 0.45985805988311768, 0.46384698152542114, 0.46784573793411255, 0.47185373306274414, 0.47587084770202637, 0.47989678382873535, 0.48393124341964722, 0.48797392845153809, 0.49202454090118408, 0.49608278274536133, 0.50014835596084595, 0.50422090291976929, 0.50830012559890747, 0.5123857855796814, 0.51647758483886719, 0.52057504653930664, 0.52467787265777588, 0.5287858247756958, 0.53289848566055298, 0.53701561689376831, 0.54113680124282837, 0.54526180028915405, 0.54939013719558716, 0.55352163314819336, 0.55765581130981445, 0.56179243326187134, 0.56593120098114014, 0.57007157802581787, 0.57421320676803589, 0.57835590839385986, 0.58249920606613159, 0.58664274215698242, 0.59078621864318848, 0.59492921829223633, 0.5990714430809021, 0.60321247577667236, 0.60735195875167847, 0.61148953437805176, 0.61562496423721313, 0.61975759267807007, 0.62388718128204346, 0.62801331281661987, 0.63213574886322021, 0.6362539529800415, 0.64036762714385986, 0.64447635412216187, 0.64857983589172363, 0.65267753601074219, 0.65676921606063843, 0.66085445880889893, 0.6649329662322998, 0.66900408267974854, 0.67306756973266602, 0.67712306976318359, 0.68117010593414307, 0.68520838022232056, 0.68923747539520264, 0.69325697422027588, 0.69726645946502686, 0.70126563310623169, 0.70525401830673218, 0.70923143625259399, 0.71319711208343506, 0.71715086698532104, 0.72109222412109375, 0.7250208854675293, 0.72893643379211426, 0.73283845186233521, 0.73672652244567871, 0.74060028791427612, 0.74445939064025879, 0.74830329418182373, 0.75213176012039185, 0.75594443082809448, 0.75974071025848389, 0.76352030038833618, 0.76728278398513794, 0.77102780342102051, 0.77475500106811523, 0.77846395969390869, 0.78215426206588745, 0.78582549095153809, 0.78947734832763672, 0.79310941696166992, 0.79672133922576904, 0.80031275749206543, 0.80388307571411133, 0.80743205547332764, 0.8109593391418457, 0.81446456909179688, 0.81794726848602295, 0.82140713930130005, 0.82484376430511475, 0.82825678586959839, 0.83164584636688232, 0.8350105881690979, 0.83835059404373169, 0.84166562557220459, 0.84495508670806885, 0.84821879863739014, 0.85145628452301025, 0.8546673059463501, 0.85785144567489624, 0.86100828647613525, 0.86413758993148804, 0.86723899841308594, 0.87031209468841553, 0.87335652112960815, 0.87637203931808472, 0.87935841083526611, 0.88231492042541504, 0.88524156808853149, 0.88813787698745728, 0.89100354909896851, 0.89383822679519653, 0.89664167165756226, 0.89941352605819702, 0.90215343236923218, 0.90486115217208862, 0.90753632783889771, 0.91017866134643555, 0.91278791427612305, 0.91536372900009155, 0.91790568828582764, 0.92041373252868652, 0.92288732528686523, 0.92532640695571899, 0.92773056030273438, 0.93009954690933228, 0.93243312835693359, 0.93473094701766968, 0.93699288368225098, 0.93921846151351929, 0.94140768051147461, 0.94356006383895874, 0.94567543268203735, 0.94775348901748657, 0.94979411363601685, 0.95179694890975952, 0.95376187562942505, 0.95568859577178955, 0.95757681131362915, 0.95942646265029907, 0.96123719215393066, 0.9630088210105896, 0.96474123001098633, 0.96643412113189697, 0.96808725595474243, 0.96970051527023315, 0.97127372026443481, 0.97280663251876831, 0.97429907321929932, 0.97575092315673828, 0.9771619439125061, 0.97853195667266846, 0.97986090183258057, 0.98114854097366333, 0.98239481449127197, 0.98359936475753784, 0.98476219177246094, 0.98588317632675171, 0.98696213960647583, 0.98799896240234375, 0.98899352550506592, 0.98994570970535278, 0.99085539579391479, 0.9917224645614624, 0.99254685640335083, 0.99332839250564575, 0.99406707286834717, 0.99476277828216553, 0.99541538953781128, 0.99602478742599487, 0.99659103155136108, 0.99711394309997559, 0.99759352207183838, 0.99802964925765991, 0.99842232465744019, 0.9987715482711792, 0.99907714128494263, 0.99933922290802002, 0.99955761432647705, 0.9997323751449585, 0.99986344575881958, 0.9999508261680603, 0.99999451637268066, 0.99999451637268066};
/*
 gen_lp_impulse generates a sinc function to be used as a lowpass impulse response.
 array is the container where to save the impulse response function.
 size is the convolution impulse response length in samples.
 freq is the cutoff frequency in radians.
*/
void gen_lp_impulse(MYFLT *array, int size, float freq) {
    int i, ppi;
    MYFLT pp, ppf, env, scl, invSum, val;
    int half = size / 2;
    MYFLT vsum = 0.0;
    MYFLT envPointerScaling = 1.0 / (size + 1) * 1024.0;
    MYFLT sincScaling = (MYFLT)half;
    
    for (i=0; i<half; i++) {
        pp = i * envPointerScaling;
        ppi = (int)pp;
        ppf = pp - ppi;
        env = HALF_BLACKMAN[ppi] + (HALF_BLACKMAN[ppi+1] - HALF_BLACKMAN[ppi]) * ppf;
        scl = i - sincScaling;
        val = MYSIN(freq * scl) / scl * env;
        array[i] = val;
        vsum += val;
    }
    vsum *= 2.0;
    vsum += freq;
    invSum = 1.0 / vsum;
    val = freq * invSum;
    array[half] = val;
    for (i=0; i<half; i++) {
        array[i] *= invSum;
    }
    for (i=1; i<half; i++) {
        array[half+i] = array[half-i];
    }    
}

/*
 lp_conv -> convolution lowpass filter.
 samples is the samples array to filter.
 impulse is the impulse response array.
 num_samps is the number od samples to filter.
 size is the filter order. Minimum suggested = 16, ideal = 128 or higher.
 gain is the gain of the filter.
*/
void lp_conv(MYFLT *samples, MYFLT *impulse, int num_samps, int size, int gain) {
    int i, j, count, tmp_count;
    MYFLT val;
    MYFLT intmp[size];
    
    for (i=0; i<size; i++) {
        intmp[i] = 0.0;
    }
    count = 0;
    for (i=0; i<num_samps; i++) {
        val = 0.0;
        tmp_count = count;
        for (j=0; j<size; j++) {
            if (tmp_count < 0)
                tmp_count += size;
            val += intmp[tmp_count] * impulse[j] * gain;
            tmp_count--;
        }
        if (++count == size)
            count = 0;
        intmp[count] = samples[i];
        samples[i] = val;
    }
}

static PyObject *
upsamp(PyObject *self, PyObject *args, PyObject *kwds)
{
    int i, j, k;
    char *inpath;
    char *outpath;
    SNDFILE *sf;
    SF_INFO info;
    unsigned int num, snd_size, snd_sr, snd_chnls, num_items;
    MYFLT *sincfunc;
    MYFLT *tmp;
    MYFLT **samples;
    MYFLT **upsamples;
    int up = 4;
    int order = 128;
    static char *kwlist[] = {"path", "outfile", "up", "order", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ss|ii", kwlist, &inpath, &outpath, &up, &order))
        return PyInt_FromLong(-1);
    
    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);
    if (sf == NULL) {
        printf("Failed to open the file.\n");
        return PyInt_FromLong(-1);
    }
    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    num = SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT));
    for(i=0; i<snd_chnls; i++)
        samples[i] = (MYFLT *)malloc(snd_size * sizeof(MYFLT));
    
    for (i=0; i<num_items; i++)
        samples[i%snd_chnls][(int)(i/snd_chnls)] = tmp[i];
    free(tmp);
    
    /* upsampling */
    upsamples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT));
    for(i=0; i<snd_chnls; i++)
        upsamples[i] = (MYFLT *)malloc(snd_size * up * sizeof(MYFLT));
    
    for (i=0; i<snd_size; i++) {
        for (j=0; j<snd_chnls; j++) {
            upsamples[j][i*up] = samples[j][i];
            for (k=1; k<up; k++) {
                upsamples[j][i*up+k] = 0.0;
            }
        }
    }
    
    if (order > 2) {
        /* apply lowpass filter */
        sincfunc = (MYFLT *)malloc(order * sizeof(MYFLT));
        gen_lp_impulse(sincfunc, order, PI/up);
        for (i=0; i<snd_chnls; i++) {
            lp_conv(upsamples[i], sincfunc, snd_size*up, order, up);
        }
        free(sincfunc);
    }
    
    /* save upsampled file */
    info.samplerate = snd_sr * up;
    tmp = (MYFLT *)malloc(num_items * up * sizeof(MYFLT));
    for (i=0; i<(snd_size*up); i++) {
        for (j=0; j<snd_chnls; j++) {
            tmp[i*snd_chnls+j] = upsamples[j][i];
        }
    }    
    
    if (! (sf = sf_open(outpath, SFM_WRITE, &info))) {
        printf ("Not able to open output file %s.\n", outpath);
        free(tmp);
        for (i=0; i<snd_chnls; i++) {
            free(samples[i]);
            free(upsamples[i]);
        }
        free(samples);
        free(upsamples);
        return PyInt_FromLong(-1);
    }

    SF_WRITE(sf, tmp, num_items * up);
    sf_close(sf);
    
    /* clean-up */
    free(tmp);
    for (i=0; i<snd_chnls; i++) {
        free(samples[i]);
        free(upsamples[i]);
    }
    free(samples);
    free(upsamples);
    
    Py_RETURN_NONE;    
}

static PyObject *
downsamp(PyObject *self, PyObject *args, PyObject *kwds)
{
    int i, j;
    char *inpath;
    char *outpath;
    SNDFILE *sf;
    SF_INFO info;
    unsigned int num, snd_size, snd_sr, snd_chnls, num_items;
    MYFLT *sincfunc;
    MYFLT *tmp;
    MYFLT **samples;
    MYFLT **downsamples;
    int down = 4;
    int order = 128;
    static char *kwlist[] = {"path", "outfile", "down", "order", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ss|ii", kwlist, &inpath, &outpath, &down, &order))
        return PyInt_FromLong(-1);
    
    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);
    if (sf == NULL) {
        printf("Failed to open the file.\n");
        return PyInt_FromLong(-1);
    }
    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    num = SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT));
    for(i=0; i<snd_chnls; i++)
        samples[i] = (MYFLT *)malloc(snd_size * sizeof(MYFLT));
    
    for (i=0; i<num_items; i++)
        samples[i%snd_chnls][(int)(i/snd_chnls)] = tmp[i];
    free(tmp);

    if (order > 2) {
        /* apply lowpass filter */
        sincfunc = (MYFLT *)malloc(order * sizeof(MYFLT));
        gen_lp_impulse(sincfunc, order, PI/down);
        for (i=0; i<snd_chnls; i++) {
            lp_conv(samples[i], sincfunc, snd_size, order, 1);
        }
        free(sincfunc);
    }
    
    /* downsampling */
    downsamples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT));
    for(i=0; i<snd_chnls; i++)
        downsamples[i] = (MYFLT *)malloc(snd_size / down * sizeof(MYFLT));
    
    for (i=0; i<(snd_size / down); i++) {
        for (j=0; j<snd_chnls; j++) {
            downsamples[j][i] = samples[j][i*down];
        }
    }

    /* save downsampled file */
    info.samplerate = snd_sr / down;
    tmp = (MYFLT *)malloc(num_items / down * sizeof(MYFLT));
    for (i=0; i<(snd_size / down); i++) {
        for (j=0; j<snd_chnls; j++) {
            tmp[i*snd_chnls+j] = downsamples[j][i];
        }
    }    
    
    if (! (sf = sf_open(outpath, SFM_WRITE, &info))) {
        printf ("Not able to open output file %s.\n", outpath);
        free(tmp);
        for (i=0; i<snd_chnls; i++) {
            free(samples[i]);
            free(downsamples[i]);
        }
        free(samples);
        free(downsamples);
        return PyInt_FromLong(-1);
    }
    
    SF_WRITE(sf, tmp, num_items / down);
    sf_close(sf);
    
    /* clean-up */
    free(tmp);
    for (i=0; i<snd_chnls; i++) {
        free(samples[i]);
        free(downsamples[i]);
    }
    free(samples);
    free(downsamples);
    
    Py_RETURN_NONE;    
}

/****** Algorithm utilities ******/
#define reducePoints_info \
"\nDouglas-Peucker curve reduction algorithm.\n\n\
reducePoints(pointlist, tolerance=0.02)\n\nThis function receives a list of points as input and returns a simplified list by\neliminating redundancies.\n\n\
A point is a tuple (or a list) of two floats, time and value. A list of points looks like:\n\n        \
[(0, 0), (0.1, 0.7), (0.2, 0.5), ...] \n\n\
Parameters:\n\n    \
pointlist : list of lists or list of tuples\n        List of points (time, value) to filter.\n    \
tolerance : float, optional\n        Normalized distance threshold under which a point is\n        excluded from the list. Defaults to 0.02."

typedef struct STACK_RECORD {
    int nAnchorIndex;
    int nFloaterIndex;
    struct STACK_RECORD *precPrev;
} STACK_RECORD;

STACK_RECORD *m_pStack = NULL;

void StackPush( int nAnchorIndex, int nFloaterIndex )
{
    STACK_RECORD *precPrev = m_pStack;
    m_pStack = (STACK_RECORD *)malloc( sizeof(STACK_RECORD) );
    m_pStack->nAnchorIndex = nAnchorIndex;
    m_pStack->nFloaterIndex = nFloaterIndex;
    m_pStack->precPrev = precPrev;
}

int StackPop( int *pnAnchorIndex, int *pnFloaterIndex )
{
    STACK_RECORD *precStack = m_pStack;
    if ( precStack == NULL )
        return 0;
    *pnAnchorIndex = precStack->nAnchorIndex;
    *pnFloaterIndex = precStack->nFloaterIndex;
    m_pStack = precStack->precPrev;
    free( precStack );
    return 1;
}

static PyObject *
reducePoints(PyObject *self, PyObject *args, PyObject *kwds)
{
    int i, nPointsCount, nVertexIndex, nAnchorIndex, nFloaterIndex;
    MYFLT dSegmentVecLength;
    MYFLT dAnchorVecX, dAnchorVecY;
    MYFLT dAnchorUnitVecX, dAnchorUnitVecY;
    MYFLT dVertexVecLength;
    MYFLT dVertexVecX, dVertexVecY;
    MYFLT dProjScalar;
    MYFLT dVertexDistanceToSegment;
    MYFLT dMaxDistThisSegment;
    int nVertexIndexMaxDistance;
    PyObject *pointlist, *pPointsOut, *tup;
    MYFLT *pPointsX, *pPointsY;
    int *pnUseFlag;
    MYFLT dTolerance = .02;
    MYFLT xMax, yMin, yMax, yRange;;
    
    static char *kwlist[] = {"pointlist", "tolerance", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_F, kwlist, &pointlist, &dTolerance))
        return PyInt_FromLong(-1);
    
    nPointsCount = PyList_Size(pointlist);
    
    pPointsX = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
    pPointsY = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
    pnUseFlag = (int *)malloc(nPointsCount * sizeof(int));
    
    tup = PyList_GET_ITEM(pointlist, 0);
    if (PyTuple_Check(tup) == 1) {
        for (i=0; i<nPointsCount; i++) {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 0)));
            pPointsY[i] = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(tup, 1)));
            pnUseFlag[i] = 0;
        }
    }
    else {
        for (i=0; i<nPointsCount; i++) {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(tup, 0)));
            pPointsY[i] = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(tup, 1)));
            pnUseFlag[i] = 0;
        }
    }

    // rescale points between 0. and 1.
    xMax = pPointsX[nPointsCount-1];
    yMin = 9999999999.9; yMax = -999999.9;
    for (i=0; i<nPointsCount; i++) {
        if (pPointsY[i] < yMin)
            yMin = pPointsY[i];
        else if (pPointsY[i] > yMax)
            yMax = pPointsY[i];
    }    
    yRange = yMax - yMin;
    for (i=0; i<nPointsCount; i++) {
        pPointsX[i] = pPointsX[i] / xMax;
        pPointsY[i] = (pPointsY[i] - yMin) / yMax;
    }

    // filter...
    pnUseFlag[0] = pnUseFlag[nPointsCount-1] = 1;
    nAnchorIndex = 0;
    nFloaterIndex = nPointsCount - 1;
    StackPush( nAnchorIndex, nFloaterIndex );
    while ( StackPop( &nAnchorIndex, &nFloaterIndex ) ) {
        // initialize line segment
        dAnchorVecX = pPointsX[ nFloaterIndex ] - pPointsX[ nAnchorIndex ];
        dAnchorVecY = pPointsY[ nFloaterIndex ] - pPointsY[ nAnchorIndex ];
        dSegmentVecLength = sqrt( dAnchorVecX * dAnchorVecX
                                 + dAnchorVecY * dAnchorVecY );
        dAnchorUnitVecX = dAnchorVecX / dSegmentVecLength;
        dAnchorUnitVecY = dAnchorVecY / dSegmentVecLength;
        // inner loop:
        dMaxDistThisSegment = 0.0;
        nVertexIndexMaxDistance = nAnchorIndex + 1;
        for ( nVertexIndex = nAnchorIndex + 1; nVertexIndex < nFloaterIndex; nVertexIndex++ ) {
            //compare to anchor
            dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nAnchorIndex ];
            dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nAnchorIndex ];
            dVertexVecLength = sqrt( dVertexVecX * dVertexVecX
                                    + dVertexVecY * dVertexVecY );
            //dot product:
            dProjScalar = dVertexVecX * dAnchorUnitVecX + dVertexVecY * dAnchorUnitVecY;
            if ( dProjScalar < 0.0 )
                dVertexDistanceToSegment = dVertexVecLength;
            else {
                //compare to floater
                dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nFloaterIndex ];
                dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nFloaterIndex ];
                dVertexVecLength = sqrt( dVertexVecX * dVertexVecX
                                        + dVertexVecY * dVertexVecY );
                //dot product:
                dProjScalar = dVertexVecX * (-dAnchorUnitVecX) + dVertexVecY * (-dAnchorUnitVecY);
                if ( dProjScalar < 0.0 )
                    dVertexDistanceToSegment = dVertexVecLength;
                else //calculate perpendicular distance to line (pythagorean theorem):
                    dVertexDistanceToSegment =
                    sqrt( fabs( dVertexVecLength * dVertexVecLength - dProjScalar * dProjScalar ) );
            }
            if ( dMaxDistThisSegment < dVertexDistanceToSegment ) {
                dMaxDistThisSegment = dVertexDistanceToSegment;
                nVertexIndexMaxDistance = nVertexIndex;
            }
        }
        if ( dMaxDistThisSegment <= dTolerance ) { //use line segment
            pnUseFlag[ nAnchorIndex ] = 1;
            pnUseFlag[ nFloaterIndex ] = 1;
        }
        else {
            StackPush( nAnchorIndex, nVertexIndexMaxDistance );
            StackPush( nVertexIndexMaxDistance, nFloaterIndex );
        }
    }
    
    pPointsOut = PyList_New(0);    
    for (i=0; i<nPointsCount; i++) {
        if (pnUseFlag[i] == 1) {
            PyList_Append(pPointsOut, PyList_GET_ITEM(pointlist, i));
        }
    }        

    return pPointsOut;    
}

#define distanceToSegment_info \
"\nFind the distance from a point to a line or line segment.\n\n\
distanceToSegment(p, p1, p2, xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)\n\nThis function returns the shortest distance from a point to a line segment\nnormalized between 0 and 1.\n\n\
A point is a tuple (or a list) of two floats, time and value. `p` is the point for which\nto find the distance from line `p1` to `p2`.\n\n\
Parameters:\n\n    \
p : list or tuple\n        Point for which to find the distance.\n    \
p1 : list or tuple\n        First point of the segment.\n    \
p2 : list or tuple\n        Second point of the segment.\n    \
xmin : float, optional\n        Minimum value on the X axis.\n    \
xmax : float, optional\n        Maximum value on the X axis.\n    \
ymin : float, optional\n        Minimum value on the Y axis.\n    \
ymax : float, optional\n        Maximum value on the Y axis.\n    \
xlog : boolean, optional\n        Set this argument to True if X axis has a logarithmic scaling.\n    \
ylog : boolean, optional\n        Set this argument to True if Y axis has a logarithmic scaling."

static PyObject *
distanceToSegment(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *p, *p1, *p2, *pf, *pf1, *pf2;
    MYFLT xscale, yscale, xDelta, yDelta, u;
    MYFLT xmin = 0.0;
    MYFLT xmax = 1.0;
    MYFLT ymin = 0.0;
    MYFLT ymax = 1.0;
    int xlog = 0;
    int ylog = 0;
    MYFLT xp[2], xp1[2], xp2[2], closest[2];
    
    static char *kwlist[] = {"p", "p1", "p2", "xmin", "xmax", "ymin", "ymax", "xlog", "ylog", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OOO_FFFFII, kwlist, &p, &p1, &p2, &xmin, &xmax, &ymin, &ymax, &xlog, &ylog))
        return PyInt_FromLong(-1);

    pf = PySequence_Fast(p, NULL);
    pf1 = PySequence_Fast(p1, NULL);
    pf2 = PySequence_Fast(p2, NULL);
    if (xlog == 0) {
        xscale = xmax - xmin;
        xp[0] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf, 0))) / xscale;
        xp1[0] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf1, 0))) / xscale;
        xp2[0] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf2, 0))) / xscale;
    }
    else {
        xscale = MYLOG10(xmax / xmin);
        xp[0] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf, 0))) / xmin) / xscale;
        xp1[0] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf1, 0))) / xmin) / xscale;
        xp2[0] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf2, 0))) / xmin) / xscale;
    }
    if (ylog == 0) {
        yscale = ymax - ymin;
        xp[1] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf, 1))) / yscale;
        xp1[1] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf1, 1))) / yscale;
        xp2[1] = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf2, 1))) / yscale;        
    }
    else {
        yscale = MYLOG10(ymax / ymin);
        xp[1] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf, 1))) / ymin) / yscale;
        xp1[1] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf1, 1))) / ymin) / yscale;
        xp2[1] = MYLOG10(PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(pf2, 1))) / ymin) / yscale;        
    }

    xDelta = xp2[0] - xp1[0]; yDelta = xp2[1] - xp1[1];
    u = ((xp[0] - xp1[0]) * xDelta + (xp[1] - xp1[1]) * yDelta) / (xDelta * xDelta + yDelta * yDelta);

    if (u < 0.0) {
        closest[0] = xp1[0]; closest[1] = xp1[1];
    }
    else if (u > 1.0) {
        closest[0] = xp2[0]; closest[1] = xp2[1];
    }
    else {
        closest[0] = xp1[0] + u * xDelta; closest[1] = xp1[1] + u * yDelta;
    }

    return PyFloat_FromDouble(MYSQRT(MYPOW(xp[0] - closest[0], 2.0) + MYPOW(xp[1] - closest[1], 2.0)));
}

#define linToCosCurve_info \
"\nCreates a cosinus interpolated curve from a list of points.\n\n\
linToCosCurve(data, yrange=[0, 1], totaldur=1, points=1024, log=False)\n\n    \
A point is a tuple (or a list) of two floats, time and value.\n\nParameters:\n\n    \
data : list of points\n        Set of points between which will be inserted interpolated segments.\n    \
yrange : list of 2 floats, optional\n        Minimum and maximum values on the Y axis. Defaults to [0., 1.].\n    \
totaldur : float, optional\n        X axis duration. Defaults to 1.\n    \
points : int, optional\n        Number of points in the output list. Defaults to 1024.\n    \
log : boolean, optional\n        Set this value to True if the Y axis has a logarithmic scale. Defaults to False\n\n\
Examples:\n\n    \
>>> s = Server().boot()\n    \
>>> a = [(0,0), (0.25, 1), (0.33, 1), (1,0)]\n    \
>>> b = linToCosCurve(a, yrange=[0, 1], totaldur=1, points=8192)\n    \
>>> t = DataTable(size=len(b), init=[x[1] for x in b])\n    \
>>> t.view()\n\n"

static PyObject *
linToCosCurve(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *data, *fdata, *out, *inout, *ftup, *yrange=NULL, *fyrange=NULL;
    int i, j, datasize, steps; 
    double tmp, x1, x2, y1, y2, mu, ydiff, log10ymin, log10ymax;
    double *xdata, *ydata, *cxdata, *cydata;
    double totaldur = 1.0;
    double ymin = 0.0;
    double ymax = 1.0;
    int num = 1024;
    double inc = 1.0 / num;
    int log = 0;
    int count = 0;
    
    static char *kwlist[] = {"data", "yrange", "totaldur", "points", "log", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|Odii", kwlist, &data, &yrange, &totaldur, &num, &log))
        Py_RETURN_NONE;
    
    if (yrange) {
        fyrange = PySequence_Fast(yrange, NULL);
        ymin = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(fyrange, 0)));
        ymax = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(fyrange, 1)));
    }
    ydiff = ymax - ymin;
    log10ymin = log10(ymin);
    log10ymax = log10(ymax);
    
    fdata = PySequence_Fast(data, NULL);
    datasize = PySequence_Size(fdata);
    xdata = (double *)malloc(datasize * sizeof(double));
    ydata = (double *)malloc(datasize * sizeof(double));
    
    /* acquire data + normalization */
    if (log == 0) {
        for (i=0; i<datasize; i++) {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(ftup, 0)));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(ftup, 1)));
            ydata[i] = (tmp - ymin) / ydiff;
        }
    }
    else {
        for (i=0; i<datasize; i++) {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(ftup, 0)));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PyNumber_Float(PySequence_Fast_GET_ITEM(ftup, 1)));
            ydata[i] = log10(tmp / ymin) / log10(ymax / ymin);
        }        
    }

    cxdata = (double *)malloc((num+5) * sizeof(double));
    cydata = (double *)malloc((num+5) * sizeof(double));
    
    /* generates cos interpolation */
    for (i=0; i<(datasize-1); i++) {
        x1 = xdata[i];
        x2 = xdata[i+1];
        y1 = ydata[i];
        y2 = ydata[i+1];
        steps = (int)((x2 - x1) * num);
        if (steps <= 0)
            continue;
        for (j=0; j<steps; j++) {
            mu = (1.0 - cos(j / (float)steps * PI)) * 0.5;
            cxdata[count] = x1 + inc * j;
            cydata[count++] = y1 + (y2 - y1) * mu;
        }
    }
    cxdata[count] = xdata[datasize-1];
    cydata[count++] = ydata[datasize-1];
    
    /* denormalization */
    if (log == 0) {
        for (i=0; i<count; i++) {
            cxdata[i] *= totaldur;
            cydata[i] = cydata[i] * ydiff + ymin;
        }
    }
    else {
        for (i=0; i<count; i++) {
            cxdata[i] *= totaldur;
            cydata[i] = pow(10.0, cydata[i] * (log10ymax - log10ymin) + log10ymin);
        }        
    }

    /* output Python's list of lists */
    out = PyList_New(count);
    for (i=0; i<count; i++) {
        inout = PyList_New(2);
        PyList_SET_ITEM(inout, 0, PyFloat_FromDouble(cxdata[i]));
        PyList_SET_ITEM(inout, 1, PyFloat_FromDouble(cydata[i]));
        PyList_SET_ITEM(out, i, inout);
    }
    
    free(xdata);
    free(ydata);
    free(cxdata);
    free(cydata);
    return out;
}

#define rescale_info \
"\nConverts values from an input range to an output range.\n\n\
rescale(data, xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)\n\nThis function takes data in the range `xmin` - `xmax` and returns corresponding values\nin the range `ymin` - `ymax`.\n\n\
`data` can be either a number or a list. Return value is of the same type as `data`\nwith all values rescaled.\n\n\
Parameters:\n\n    \
data : float or list of floats\n        Values to convert.\n    \
xmin : float, optional\n        Minimum value of the input range.\n    \
xmax : float, optional\n        Maximum value of the input range.\n    \
ymin : float, optional\n        Minimum value of the output range.\n    \
ymax : float, optional\n        Maximum value of the output range.\n    \
xlog : boolean, optional\n        Set this argument to True if the input range has a logarithmic scaling.\n    \
ylog : boolean, optional\n        Set this argument to True if the output range has a logarithmic scaling.\n\n\
Examples:\n\n    \
>>> a = 0.5\n    \
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n    \
>>> print b\n    \
>>> 632.453369141\n    \
>>> a = [0, .2, .4, .6, .8]\n    \
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n    \
>>> print b\n    \
>>> [20.000001907348633, 79.621009826660156, 316.97738647460938, 1261.91162109375, 5023.7705078125]\n\n"

static PyObject *
rescale(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *data, *out;
    MYFLT datascl, curscl, val;
    MYFLT xmin = 0.0;
    MYFLT xmax = 1.0;
    MYFLT ymin = 0.0;
    MYFLT ymax = 1.0;
    int xlog = 0;
    int ylog = 0;
    int i, cnt;
    int type; // 0 = float, 1 = list of floats
    
    static char *kwlist[] = {"data", "xmin", "xmax", "ymin", "ymax", "xlog", "ylog", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_FFFFII, kwlist, &data, &xmin, &xmax, &ymin, &ymax, &xlog, &ylog))
        return PyInt_FromLong(-1);

    if (PyNumber_Check(data))
        type = 0;
    else if (PyList_Check(data))
        type = 1;
    else
        Py_RETURN_NONE;

    if (xlog == 0 && ylog == 0) {
        datascl = xmax - xmin;
        curscl = ymax - ymin;
        curscl /= datascl;
        if (type == 0) {
            val = PyFloat_AsDouble(PyNumber_Float(data));
            return Py_BuildValue("d", (val - xmin) * curscl + ymin);
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(data, i)));
                PyList_SET_ITEM(out, i, PyFloat_FromDouble((val - xmin) * curscl + ymin));
            }
            return out;
        }        
    }
    else if (xlog == 0 && ylog == 1) {
        if (xmin == 0)
            xmin = 0.000001;
        datascl = xmax - xmin;
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);
        if (type == 0) {
            val = PyFloat_AsDouble(PyNumber_Float(data));
            if (val == 0)
                val = 0.000001;
            val = (val - xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(data, i)));
                if (val == 0)
                    val = 0.000001;
                val = (val - xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }
            return out;
        }        
    }
    else if (xlog == 1 && ylog == 0) {
        datascl = MYLOG10(xmax / xmin);
        curscl = ymax - ymin;
        if (type == 0) {
            val = PyFloat_AsDouble(PyNumber_Float(data));
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", val * curscl + ymin);
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(data, i)));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(val * curscl + ymin));
            }
            return out;
        }        
    }
    else if (xlog == 1 && ylog == 1) {
        datascl = MYLOG10(xmax / xmin);
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);
        if (type == 0) {
            val = PyFloat_AsDouble(PyNumber_Float(data));
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(data, i)));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }
            return out;
        }        
    }
    else {
        Py_RETURN_NONE;
    }
    Py_RETURN_NONE;
}

/****** Conversion utilities ******/
#define midiToHz_info \
"\nConverts a midi note value to frequency in Hertz.\n\nmidiToHz(x)\n\nParameters:\n\n    \
x : int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\nExamples:\n\n    \
>>> a = (48, 60, 62, 67, 72)\n    \
>>> b = midiToHz(a)\n    \
>>> print b\n    \
>>> (130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656, 523.2511306013643)\n    \
>>> a = [48, 60, 62, 67, 72]\n    \
>>> b = midiToHz(a)\n    \
>>> print b\n    \
>>> [130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656, 523.2511306013643]\n    \
>>> b = midiToHz(60.0)\n    \
>>> print b\n    \
>>> 261.625565301\n\n"

static PyObject *
midiToHz(PyObject *self, PyObject *arg) {
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 8.1757989156437 * pow(1.0594630943593, PyFloat_AsDouble(PyNumber_Float(arg))));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(arg, i)));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(8.1757989156437 * pow(1.0594630943593, x)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(arg, i)));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(8.1757989156437 * pow(1.0594630943593, x)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}    

#define midiToTranspo_info \
"\nConverts a midi note value to transposition factor (central key = 60).\n\nmidiToTranspo(x)\n\nParameters:\n\n    \
x : int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\nExamples:\n\n    \
>>> a = (48, 60, 62, 67, 72)\n    \
>>> b = midiToTranspo(a)\n    \
>>> print b\n    \
>>> (0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281, 2.0000000000001066)\n    \
>>> a = [48, 60, 62, 67, 72]\n    \
>>> b = midiToTranspo(a)\n    \
>>> print b\n    \
>>> [0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281, 2.0000000000001066]\n    \
>>> b = midiToTranspo(60.0)\n    \
>>> print b\n    \
>>> 1.0\n\n"

static PyObject *
midiToTranspo(PyObject *self, PyObject *arg) {
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", pow(1.0594630943593, PyFloat_AsDouble(PyNumber_Float(arg))-60.0));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(arg, i)));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x-60.0)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(arg, i)));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x-60.0)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}    

#define sampsToSec_info \
"\nReturns the duration in seconds equivalent to the number of samples given as an argument.\n\nsampsToSec(x)\n\nParameters:\n\n    \
x : int or float\n        Duration in samples. `x` can be a number, a list or a tuple, otherwise function returns None.\n\nExamples:\n\n    \
>>> a = (64, 128, 256)\n    \
>>> b = sampsToSec(a)\n    \
>>> print b\n    \
>>> (0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194)\n    \
>>> a = [64, 128, 256]\n    \
>>> b = sampsToSec(a)\n    \
>>> print b\n    \
>>> [0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194]\n    \
>>> b = sampsToSec(8192)\n    \
>>> print b\n    \
>>> 0.185759637188\n\n"

static PyObject *
sampsToSec(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    if (server == NULL) {
        printf("Warning: A Server must be booted before calling `sampsToSec` function.\n");
        Py_RETURN_NONE;
    }
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", PyFloat_AsDouble(PyNumber_Float(arg)) / sr);
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(arg, i)));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(arg, i)));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}                         

#define secToSamps_info \
"\nReturns the number of samples equivalent to the duration in seconds given as an argument.\n\nsecToSamps(x)\n\nParameters:\n\n    \
x : int or float\n        Duration in seconds. `x` can be a number, a list or a tuple, otherwise function returns None.\n\nExamples:\n\n    \
>>> a = (0.1, 0.25, 0.5, 1)\n    \
>>> b = secToSamps(a)\n    \
>>> print b\n    \
>>> (4410, 11025, 22050, 44100)\n    \
>>> a = [0.1, 0.25, 0.5, 1]\n    \
>>> b = secToSamps(a)\n    \
>>> print b\n    \
>>> [4410, 11025, 22050, 44100]\n    \
>>> b = secToSamps(2.5)\n    \
>>> print b\n    \
>>> 110250\n\n"

static PyObject *
secToSamps(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    if (server == NULL) {
        printf("Warning: A Server must be booted before calling `secToSamps` function.\n");
        Py_RETURN_NONE;
    }
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("l", (long)(PyFloat_AsDouble(PyNumber_Float(arg)) * sr));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyList_GET_ITEM(arg, i)));
            PyList_SET_ITEM(newseq, i, PyInt_FromLong((long)(x * sr)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyNumber_Float(PyTuple_GET_ITEM(arg, i)));
            PyTuple_SET_ITEM(newseq, i, PyInt_FromLong((long)(x * sr)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}                         

/************* Server quieries *************/
#define serverCreated_info \
"\nReturns True if a Server object is already created, otherwise, returns False.\n\nserverCreated()\n\nExamples:\n\n    \
>>> print serverCreated()\n    \
>>> False\n    \
>>> s = Server()\n    \
>>> print serverCreated()\n    \
>>> True\n\n"

static PyObject *
serverCreated(PyObject *self) {
    if (PyServer_get_server() != NULL)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

#define serverBooted_info \
"\nReturns True if an already created Server is booted, otherwise, returns False.\n\nserverBooted()\n\nExamples:\n\n    \
>>> s = Server()\n    \
>>> print serverBooted()\n    \
>>> False\n    \
>>> s.boot()\n    \
>>> print serverBooted()\n    \
>>> True\n\n"

static PyObject *
serverBooted(PyObject *self) {
    int boot;
    PyObject *server;
    if (PyServer_get_server() != NULL) {
        server = PyServer_get_server();
        boot = PyInt_AsLong(PyObject_CallMethod(server, "getIsBooted", NULL));
        if (boot == 0)
            Py_RETURN_FALSE;
        else
            Py_RETURN_TRUE;
    }
    else {
        printf("'serverBooted' called but there is no server created.\n");
        Py_RETURN_FALSE;
    }
}

static PyMethodDef pyo_functions[] = {
{"pa_count_devices", (PyCFunction)portaudio_count_devices, METH_NOARGS, portaudio_count_devices_info},
{"pa_count_host_apis", (PyCFunction)portaudio_count_host_apis, METH_NOARGS, portaudio_count_host_apis_info},
{"pa_list_devices", (PyCFunction)portaudio_list_devices, METH_NOARGS, portaudio_list_devices_info},
{"pa_get_output_devices", (PyCFunction)portaudio_get_output_devices, METH_NOARGS, portaudio_get_output_devices_info},
{"pa_get_input_devices", (PyCFunction)portaudio_get_input_devices, METH_NOARGS, portaudio_get_input_devices_info},
{"pa_list_host_apis", (PyCFunction)portaudio_list_host_apis, METH_NOARGS, portaudio_list_host_apis_info},
{"pa_get_default_input", (PyCFunction)portaudio_get_default_input, METH_NOARGS, portaudio_get_default_input_info},
{"pa_get_default_host_api", (PyCFunction)portaudio_get_default_host_api, METH_NOARGS, portaudio_get_default_host_api_info},
{"pa_get_default_output", (PyCFunction)portaudio_get_default_output, METH_NOARGS, portaudio_get_default_output_info},
{"pm_count_devices", (PyCFunction)portmidi_count_devices, METH_NOARGS, portmidi_count_devices_info},
{"pm_list_devices", (PyCFunction)portmidi_list_devices, METH_NOARGS, portmidi_list_devices_info},
{"pm_get_input_devices", (PyCFunction)portmidi_get_input_devices, METH_NOARGS, portmidi_get_input_devices_info},
{"pm_get_default_input", (PyCFunction)portmidi_get_default_input, METH_NOARGS, portmidi_get_default_input_info},
{"pm_get_output_devices", (PyCFunction)portmidi_get_output_devices, METH_NOARGS, portmidi_get_output_devices_info},
{"pm_get_default_output", (PyCFunction)portmidi_get_default_output, METH_NOARGS, portmidi_get_default_output_info},
{"sndinfo", (PyCFunction)sndinfo, METH_VARARGS|METH_KEYWORDS, sndinfo_info},
{"savefile", (PyCFunction)savefile, METH_VARARGS|METH_KEYWORDS, savefile_info},
{"upsamp", (PyCFunction)upsamp, METH_VARARGS|METH_KEYWORDS, upsamp_info},
{"downsamp", (PyCFunction)downsamp, METH_VARARGS|METH_KEYWORDS, downsamp_info},
{"reducePoints", (PyCFunction)reducePoints, METH_VARARGS|METH_KEYWORDS, reducePoints_info},
{"distanceToSegment", (PyCFunction)distanceToSegment, METH_VARARGS|METH_KEYWORDS, distanceToSegment_info},
{"rescale", (PyCFunction)rescale, METH_VARARGS|METH_KEYWORDS, rescale_info},
{"linToCosCurve", (PyCFunction)linToCosCurve, METH_VARARGS|METH_KEYWORDS, linToCosCurve_info},
{"midiToHz", (PyCFunction)midiToHz, METH_O, midiToHz_info},
{"midiToTranspo", (PyCFunction)midiToTranspo, METH_O, midiToTranspo_info},
{"sampsToSec", (PyCFunction)sampsToSec, METH_O, sampsToSec_info},
{"secToSamps", (PyCFunction)secToSamps, METH_O, secToSamps_info},
{"serverCreated", (PyCFunction)serverCreated, METH_NOARGS, serverCreated_info},
{"serverBooted", (PyCFunction)serverBooted, METH_NOARGS, serverBooted_info},
{NULL, NULL, 0, NULL},
};

PyMODINIT_FUNC
#ifndef USE_DOUBLE
init_pyo(void)
#else
init_pyo64(void)
#endif
{
    PyObject *m;
    
    m = Py_InitModule3(LIB_BASE_NAME, pyo_functions, "Python digital signal processing module.");

    if (PyType_Ready(&ServerType) < 0)
        return;
    Py_INCREF(&ServerType);
    PyModule_AddObject(m, "Server_base", (PyObject *)&ServerType);

    if (PyType_Ready(&StreamType) < 0)
        return;
    Py_INCREF(&StreamType);
    PyModule_AddObject(m, "Stream", (PyObject *)&StreamType);

    if (PyType_Ready(&TriggerStreamType) < 0)
        return;
    Py_INCREF(&TriggerStreamType);
    PyModule_AddObject(m, "TriggerStream", (PyObject *)&TriggerStreamType);
    
    if (PyType_Ready(&DummyType) < 0)
        return;
    Py_INCREF(&DummyType);
    PyModule_AddObject(m, "Dummy_base", (PyObject *)&DummyType);

    if (PyType_Ready(&TriggerDummyType) < 0)
        return;
    Py_INCREF(&TriggerDummyType);
    PyModule_AddObject(m, "TriggerDummy_base", (PyObject *)&TriggerDummyType);
    
    if (PyType_Ready(&RecordType) < 0)
        return;
    Py_INCREF(&RecordType);
    PyModule_AddObject(m, "Record_base", (PyObject *)&RecordType);

    if (PyType_Ready(&ControlRecType) < 0)
        return;
    Py_INCREF(&ControlRecType);
    PyModule_AddObject(m, "ControlRec_base", (PyObject *)&ControlRecType);

    if (PyType_Ready(&ControlReadType) < 0)
        return;
    Py_INCREF(&ControlReadType);
    PyModule_AddObject(m, "ControlRead_base", (PyObject *)&ControlReadType);

    if (PyType_Ready(&NoteinRecType) < 0)
        return;
    Py_INCREF(&NoteinRecType);
    PyModule_AddObject(m, "NoteinRec_base", (PyObject *)&NoteinRecType);

    if (PyType_Ready(&NoteinReadType) < 0)
        return;
    Py_INCREF(&NoteinReadType);
    PyModule_AddObject(m, "NoteinRead_base", (PyObject *)&NoteinReadType);

    if (PyType_Ready(&CompareType) < 0)
        return;
    Py_INCREF(&CompareType);
    PyModule_AddObject(m, "Compare_base", (PyObject *)&CompareType);
    
    if (PyType_Ready(&MixType) < 0)
        return;
    Py_INCREF(&MixType);
    PyModule_AddObject(m, "Mix_base", (PyObject *)&MixType);

    if (PyType_Ready(&SigType) < 0)
        return;
    Py_INCREF(&SigType);
    PyModule_AddObject(m, "Sig_base", (PyObject *)&SigType);

    if (PyType_Ready(&SigToType) < 0)
        return;
    Py_INCREF(&SigToType);
    PyModule_AddObject(m, "SigTo_base", (PyObject *)&SigToType);

    if (PyType_Ready(&VarPortType) < 0)
        return;
    Py_INCREF(&VarPortType);
    PyModule_AddObject(m, "VarPort_base", (PyObject *)&VarPortType);
    
    if (PyType_Ready(&InputFaderType) < 0)
        return;
    Py_INCREF(&InputFaderType);
    PyModule_AddObject(m, "InputFader_base", (PyObject *)&InputFaderType);

    if (PyType_Ready(&AdsrType) < 0)
        return;
    Py_INCREF(&AdsrType);
    PyModule_AddObject(m, "Adsr_base", (PyObject *)&AdsrType);

    if (PyType_Ready(&LinsegType) < 0)
        return;
    Py_INCREF(&LinsegType);
    PyModule_AddObject(m, "Linseg_base", (PyObject *)&LinsegType);

    if (PyType_Ready(&ExpsegType) < 0)
        return;
    Py_INCREF(&ExpsegType);
    PyModule_AddObject(m, "Expseg_base", (PyObject *)&ExpsegType);
    
    if (PyType_Ready(&TableStreamType) < 0)
        return;
    Py_INCREF(&TableStreamType);
    PyModule_AddObject(m, "TableStream", (PyObject *)&TableStreamType);
    
    if (PyType_Ready(&HarmTableType) < 0)
        return;
    Py_INCREF(&HarmTableType);
    PyModule_AddObject(m, "HarmTable_base", (PyObject *)&HarmTableType);

    if (PyType_Ready(&ChebyTableType) < 0)
        return;
    Py_INCREF(&ChebyTableType);
    PyModule_AddObject(m, "ChebyTable_base", (PyObject *)&ChebyTableType);
    
    if (PyType_Ready(&HannTableType) < 0)
        return;
    Py_INCREF(&HannTableType);
    PyModule_AddObject(m, "HannTable_base", (PyObject *)&HannTableType);

    if (PyType_Ready(&SincTableType) < 0)
        return;
    Py_INCREF(&SincTableType);
    PyModule_AddObject(m, "SincTable_base", (PyObject *)&SincTableType);
    
    if (PyType_Ready(&WinTableType) < 0)
        return;
    Py_INCREF(&WinTableType);
    PyModule_AddObject(m, "WinTable_base", (PyObject *)&WinTableType);

    if (PyType_Ready(&ParaTableType) < 0)
        return;
    Py_INCREF(&ParaTableType);
    PyModule_AddObject(m, "ParaTable_base", (PyObject *)&ParaTableType);
    
    if (PyType_Ready(&LinTableType) < 0)
        return;
    Py_INCREF(&LinTableType);
    PyModule_AddObject(m, "LinTable_base", (PyObject *)&LinTableType);

    if (PyType_Ready(&CosTableType) < 0)
        return;
    Py_INCREF(&CosTableType);
    PyModule_AddObject(m, "CosTable_base", (PyObject *)&CosTableType);

    if (PyType_Ready(&CurveTableType) < 0)
        return;
    Py_INCREF(&CurveTableType);
    PyModule_AddObject(m, "CurveTable_base", (PyObject *)&CurveTableType);

    if (PyType_Ready(&ExpTableType) < 0)
        return;
    Py_INCREF(&ExpTableType);
    PyModule_AddObject(m, "ExpTable_base", (PyObject *)&ExpTableType);
    
    if (PyType_Ready(&SndTableType) < 0)
        return;
    Py_INCREF(&SndTableType);
    PyModule_AddObject(m, "SndTable_base", (PyObject *)&SndTableType);

    if (PyType_Ready(&DataTableType) < 0)
        return;
    Py_INCREF(&DataTableType);
    PyModule_AddObject(m, "DataTable_base", (PyObject *)&DataTableType);
    
    if (PyType_Ready(&NewTableType) < 0)
        return;
    Py_INCREF(&NewTableType);
    PyModule_AddObject(m, "NewTable_base", (PyObject *)&NewTableType);

    if (PyType_Ready(&TableRecType) < 0)
        return;
    Py_INCREF(&TableRecType);
    PyModule_AddObject(m, "TableRec_base", (PyObject *)&TableRecType);
   
    if (PyType_Ready(&TableMorphType) < 0)
        return;
    Py_INCREF(&TableMorphType);
    PyModule_AddObject(m, "TableMorph_base", (PyObject *)&TableMorphType);

    if (PyType_Ready(&TrigTableRecType) < 0)
        return;
    Py_INCREF(&TrigTableRecType);
    PyModule_AddObject(m, "TrigTableRec_base", (PyObject *)&TrigTableRecType);
   
    /* Matrix objects */
    if (PyType_Ready(&MatrixStreamType) < 0)
        return;
    Py_INCREF(&MatrixStreamType);
    PyModule_AddObject(m, "MatrixStream", (PyObject *)&MatrixStreamType);
    
    if (PyType_Ready(&NewMatrixType) < 0)
        return;
    Py_INCREF(&NewMatrixType);
    PyModule_AddObject(m, "NewMatrix_base", (PyObject *)&NewMatrixType);

    if (PyType_Ready(&MatrixPointerType) < 0)
        return;
    Py_INCREF(&MatrixPointerType);
    PyModule_AddObject(m, "MatrixPointer_base", (PyObject *)&MatrixPointerType);

    if (PyType_Ready(&MatrixRecType) < 0)
        return;
    Py_INCREF(&MatrixRecType);
    PyModule_AddObject(m, "MatrixRec_base", (PyObject *)&MatrixRecType);

    if (PyType_Ready(&MatrixMorphType) < 0)
        return;
    Py_INCREF(&MatrixMorphType);
    PyModule_AddObject(m, "MatrixMorph_base", (PyObject *)&MatrixMorphType);
    
    if (PyType_Ready(&InputType) < 0)
        return;
    Py_INCREF(&InputType);
    PyModule_AddObject(m, "Input_base", (PyObject *)&InputType);

    if (PyType_Ready(&TrigType) < 0)
        return;
    Py_INCREF(&TrigType);
    PyModule_AddObject(m, "Trig_base", (PyObject *)&TrigType);

    if (PyType_Ready(&NextTrigType) < 0)
        return;
    Py_INCREF(&NextTrigType);
    PyModule_AddObject(m, "NextTrig_base", (PyObject *)&NextTrigType);

    if (PyType_Ready(&MetroType) < 0)
        return;
    Py_INCREF(&MetroType);
    PyModule_AddObject(m, "Metro_base", (PyObject *)&MetroType);

    if (PyType_Ready(&SeqerType) < 0)
        return;
    Py_INCREF(&SeqerType);
    PyModule_AddObject(m, "Seqer_base", (PyObject *)&SeqerType);

    if (PyType_Ready(&SeqType) < 0)
        return;
    Py_INCREF(&SeqType);
    PyModule_AddObject(m, "Seq_base", (PyObject *)&SeqType);
    
    if (PyType_Ready(&ClouderType) < 0)
        return;
    Py_INCREF(&ClouderType);
    PyModule_AddObject(m, "Clouder_base", (PyObject *)&ClouderType);

    if (PyType_Ready(&CloudType) < 0)
        return;
    Py_INCREF(&CloudType);
    PyModule_AddObject(m, "Cloud_base", (PyObject *)&CloudType);

    if (PyType_Ready(&BeaterType) < 0)
        return;
    Py_INCREF(&BeaterType);
    PyModule_AddObject(m, "Beater_base", (PyObject *)&BeaterType);
    
    if (PyType_Ready(&BeatType) < 0)
        return;
    Py_INCREF(&BeatType);
    PyModule_AddObject(m, "Beat_base", (PyObject *)&BeatType);

    if (PyType_Ready(&BeatTapStreamType) < 0)
        return;
    Py_INCREF(&BeatTapStreamType);
    PyModule_AddObject(m, "BeatTapStream_base", (PyObject *)&BeatTapStreamType);
    
    if (PyType_Ready(&BeatAmpStreamType) < 0)
        return;
    Py_INCREF(&BeatAmpStreamType);
    PyModule_AddObject(m, "BeatAmpStream_base", (PyObject *)&BeatAmpStreamType);

    if (PyType_Ready(&BeatDurStreamType) < 0)
        return;
    Py_INCREF(&BeatDurStreamType);
    PyModule_AddObject(m, "BeatDurStream_base", (PyObject *)&BeatDurStreamType);

    if (PyType_Ready(&BeatEndStreamType) < 0)
        return;
    Py_INCREF(&BeatEndStreamType);
    PyModule_AddObject(m, "BeatEndStream_base", (PyObject *)&BeatEndStreamType);
    
    if (PyType_Ready(&FaderType) < 0)
        return;
    Py_INCREF(&FaderType);
    PyModule_AddObject(m, "Fader_base", (PyObject *)&FaderType);

    if (PyType_Ready(&RandiType) < 0)
        return;
    Py_INCREF(&RandiType);
    PyModule_AddObject(m, "Randi_base", (PyObject *)&RandiType);

    if (PyType_Ready(&RandhType) < 0)
        return;
    Py_INCREF(&RandhType);
    PyModule_AddObject(m, "Randh_base", (PyObject *)&RandhType);

    if (PyType_Ready(&ChoiceType) < 0)
        return;
    Py_INCREF(&ChoiceType);
    PyModule_AddObject(m, "Choice_base", (PyObject *)&ChoiceType);

    if (PyType_Ready(&RandDurType) < 0)
        return;
    Py_INCREF(&RandDurType);
    PyModule_AddObject(m, "RandDur_base", (PyObject *)&RandDurType);
    
    if (PyType_Ready(&XnoiseType) < 0)
        return;
    Py_INCREF(&XnoiseType);
    PyModule_AddObject(m, "Xnoise_base", (PyObject *)&XnoiseType);

    if (PyType_Ready(&XnoiseMidiType) < 0)
        return;
    Py_INCREF(&XnoiseMidiType);
    PyModule_AddObject(m, "XnoiseMidi_base", (PyObject *)&XnoiseMidiType);

    if (PyType_Ready(&XnoiseDurType) < 0)
        return;
    Py_INCREF(&XnoiseDurType);
    PyModule_AddObject(m, "XnoiseDur_base", (PyObject *)&XnoiseDurType);
    
    if (PyType_Ready(&RandIntType) < 0)
        return;
    Py_INCREF(&RandIntType);
    PyModule_AddObject(m, "RandInt_base", (PyObject *)&RandIntType);

    if (PyType_Ready(&UrnType) < 0)
        return;
    Py_INCREF(&UrnType);
    PyModule_AddObject(m, "Urn_base", (PyObject *)&UrnType);
    
    if (PyType_Ready(&SfPlayerType) < 0)
        return;
    Py_INCREF(&SfPlayerType);
    PyModule_AddObject(m, "SfPlayer_base", (PyObject *)&SfPlayerType);

    if (PyType_Ready(&SfPlayType) < 0)
        return;
    Py_INCREF(&SfPlayType);
    PyModule_AddObject(m, "SfPlay_base", (PyObject *)&SfPlayType);
   
    if (PyType_Ready(&SfMarkerShufflerType) < 0)
        return;
    Py_INCREF(&SfMarkerShufflerType);
    PyModule_AddObject(m, "SfMarkerShuffler_base", (PyObject *)&SfMarkerShufflerType);
    
    if (PyType_Ready(&SfMarkerShuffleType) < 0)
        return;
    Py_INCREF(&SfMarkerShuffleType);
    PyModule_AddObject(m, "SfMarkerShuffle_base", (PyObject *)&SfMarkerShuffleType);

    if (PyType_Ready(&SfMarkerLooperType) < 0)
        return;
    Py_INCREF(&SfMarkerLooperType);
    PyModule_AddObject(m, "SfMarkerLooper_base", (PyObject *)&SfMarkerLooperType);
    
    if (PyType_Ready(&SfMarkerLoopType) < 0)
        return;
    Py_INCREF(&SfMarkerLoopType);
    PyModule_AddObject(m, "SfMarkerLoop_base", (PyObject *)&SfMarkerLoopType);
    
    if (PyType_Ready(&OscType) < 0)
        return;
    Py_INCREF(&OscType);
    PyModule_AddObject(m, "Osc_base", (PyObject *)&OscType);

    if (PyType_Ready(&OscLoopType) < 0)
        return;
    Py_INCREF(&OscLoopType);
    PyModule_AddObject(m, "OscLoop_base", (PyObject *)&OscLoopType);

    if (PyType_Ready(&OscBankType) < 0)
        return;
    Py_INCREF(&OscBankType);
    PyModule_AddObject(m, "OscBank_base", (PyObject *)&OscBankType);
    
    if (PyType_Ready(&TableReadType) < 0)
        return;
    Py_INCREF(&TableReadType);
    PyModule_AddObject(m, "TableRead_base", (PyObject *)&TableReadType);
    
    if (PyType_Ready(&PulsarType) < 0)
        return;
    Py_INCREF(&PulsarType);
    PyModule_AddObject(m, "Pulsar_base", (PyObject *)&PulsarType);
    
    if (PyType_Ready(&SineType) < 0)
        return;
    Py_INCREF(&SineType);
    PyModule_AddObject(m, "Sine_base", (PyObject *)&SineType);

    if (PyType_Ready(&SineLoopType) < 0)
        return;
    Py_INCREF(&SineLoopType);
    PyModule_AddObject(m, "SineLoop_base", (PyObject *)&SineLoopType);
    
    if (PyType_Ready(&FmType) < 0)
        return;
    Py_INCREF(&FmType);
    PyModule_AddObject(m, "Fm_base", (PyObject *)&FmType);

    if (PyType_Ready(&CrossFmType) < 0)
        return;
    Py_INCREF(&CrossFmType);
    PyModule_AddObject(m, "CrossFm_base", (PyObject *)&CrossFmType);

    if (PyType_Ready(&LFOType) < 0)
        return;
    Py_INCREF(&LFOType);
    PyModule_AddObject(m, "LFO_base", (PyObject *)&LFOType);
    
    if (PyType_Ready(&BlitType) < 0)
        return;
    Py_INCREF(&BlitType);
    PyModule_AddObject(m, "Blit_base", (PyObject *)&BlitType);

    if (PyType_Ready(&RosslerType) < 0)
        return;
    Py_INCREF(&RosslerType);
    PyModule_AddObject(m, "Rossler_base", (PyObject *)&RosslerType);
    
    if (PyType_Ready(&RosslerAltType) < 0)
        return;
    Py_INCREF(&RosslerAltType);
    PyModule_AddObject(m, "RosslerAlt_base", (PyObject *)&RosslerAltType);

    if (PyType_Ready(&LorenzType) < 0)
        return;
    Py_INCREF(&LorenzType);
    PyModule_AddObject(m, "Lorenz_base", (PyObject *)&LorenzType);

    if (PyType_Ready(&LorenzAltType) < 0)
        return;
    Py_INCREF(&LorenzAltType);
    PyModule_AddObject(m, "LorenzAlt_base", (PyObject *)&LorenzAltType);
    
    if (PyType_Ready(&PhasorType) < 0)
        return;
    Py_INCREF(&PhasorType);
    PyModule_AddObject(m, "Phasor_base", (PyObject *)&PhasorType);

    if (PyType_Ready(&PointerType) < 0)
        return;
    Py_INCREF(&PointerType);
    PyModule_AddObject(m, "Pointer_base", (PyObject *)&PointerType);

    if (PyType_Ready(&TableIndexType) < 0)
        return;
    Py_INCREF(&TableIndexType);
    PyModule_AddObject(m, "TableIndex_base", (PyObject *)&TableIndexType);
    
    if (PyType_Ready(&LookupType) < 0)
        return;
    Py_INCREF(&LookupType);
    PyModule_AddObject(m, "Lookup_base", (PyObject *)&LookupType);
    
    if (PyType_Ready(&NoiseType) < 0)
        return;
    Py_INCREF(&NoiseType);
    PyModule_AddObject(m, "Noise_base", (PyObject *)&NoiseType);

    if (PyType_Ready(&PinkNoiseType) < 0)
        return;
    Py_INCREF(&PinkNoiseType);
    PyModule_AddObject(m, "PinkNoise_base", (PyObject *)&PinkNoiseType);

    if (PyType_Ready(&BrownNoiseType) < 0)
        return;
    Py_INCREF(&BrownNoiseType);
    PyModule_AddObject(m, "BrownNoise_base", (PyObject *)&BrownNoiseType);
    
    if (PyType_Ready(&BiquadType) < 0)
        return;
    Py_INCREF(&BiquadType);
    PyModule_AddObject(m, "Biquad_base", (PyObject *)&BiquadType);

    if (PyType_Ready(&BiquadxType) < 0)
        return;
    Py_INCREF(&BiquadxType);
    PyModule_AddObject(m, "Biquadx_base", (PyObject *)&BiquadxType);

    if (PyType_Ready(&BiquadaType) < 0)
        return;
    Py_INCREF(&BiquadaType);
    PyModule_AddObject(m, "Biquada_base", (PyObject *)&BiquadaType);
    
    if (PyType_Ready(&EQType) < 0)
        return;
    Py_INCREF(&EQType);
    PyModule_AddObject(m, "EQ_base", (PyObject *)&EQType);
    
    if (PyType_Ready(&ToneType) < 0)
        return;
    Py_INCREF(&ToneType);
    PyModule_AddObject(m, "Tone_base", (PyObject *)&ToneType);

    if (PyType_Ready(&DCBlockType) < 0)
        return;
    Py_INCREF(&DCBlockType);
    PyModule_AddObject(m, "DCBlock_base", (PyObject *)&DCBlockType);

    if (PyType_Ready(&AllpassType) < 0)
        return;
    Py_INCREF(&AllpassType);
    PyModule_AddObject(m, "Allpass_base", (PyObject *)&AllpassType);

    if (PyType_Ready(&Allpass2Type) < 0)
        return;
    Py_INCREF(&Allpass2Type);
    PyModule_AddObject(m, "Allpass2_base", (PyObject *)&Allpass2Type);

    if (PyType_Ready(&PhaserType) < 0)
        return;
    Py_INCREF(&PhaserType);
    PyModule_AddObject(m, "Phaser_base", (PyObject *)&PhaserType);    
    
    if (PyType_Ready(&PortType) < 0)
        return;
    Py_INCREF(&PortType);
    PyModule_AddObject(m, "Port_base", (PyObject *)&PortType);
    
    if (PyType_Ready(&DenormType) < 0)
        return;
    Py_INCREF(&DenormType);
    PyModule_AddObject(m, "Denorm_base", (PyObject *)&DenormType);
    
    if (PyType_Ready(&DistoType) < 0)
        return;
    Py_INCREF(&DistoType);
    PyModule_AddObject(m, "Disto_base", (PyObject *)&DistoType);

    if (PyType_Ready(&ClipType) < 0)
        return;
    Py_INCREF(&ClipType);
    PyModule_AddObject(m, "Clip_base", (PyObject *)&ClipType);

    if (PyType_Ready(&MirrorType) < 0)
        return;
    Py_INCREF(&MirrorType);
    PyModule_AddObject(m, "Mirror_base", (PyObject *)&MirrorType);

    if (PyType_Ready(&WrapType) < 0)
        return;
    Py_INCREF(&WrapType);
    PyModule_AddObject(m, "Wrap_base", (PyObject *)&WrapType);
    
    if (PyType_Ready(&BetweenType) < 0)
        return;
    Py_INCREF(&BetweenType);
    PyModule_AddObject(m, "Between_base", (PyObject *)&BetweenType);
    
    if (PyType_Ready(&DegradeType) < 0)
        return;
    Py_INCREF(&DegradeType);
    PyModule_AddObject(m, "Degrade_base", (PyObject *)&DegradeType);
    
    if (PyType_Ready(&CompressType) < 0)
        return;
    Py_INCREF(&CompressType);
    PyModule_AddObject(m, "Compress_base", (PyObject *)&CompressType);

    if (PyType_Ready(&GateType) < 0)
        return;
    Py_INCREF(&GateType);
    PyModule_AddObject(m, "Gate_base", (PyObject *)&GateType);
    
    if (PyType_Ready(&DelayType) < 0)
        return;
    Py_INCREF(&DelayType);
    PyModule_AddObject(m, "Delay_base", (PyObject *)&DelayType);

    if (PyType_Ready(&SDelayType) < 0)
        return;
    Py_INCREF(&SDelayType);
    PyModule_AddObject(m, "SDelay_base", (PyObject *)&SDelayType);
    
    if (PyType_Ready(&WaveguideType) < 0)
        return;
    Py_INCREF(&WaveguideType);
    PyModule_AddObject(m, "Waveguide_base", (PyObject *)&WaveguideType);

    if (PyType_Ready(&AllpassWGType) < 0)
        return;
    Py_INCREF(&AllpassWGType);
    PyModule_AddObject(m, "AllpassWG_base", (PyObject *)&AllpassWGType);
    
    if (PyType_Ready(&MidictlType) < 0)
        return;
    Py_INCREF(&MidictlType);
    PyModule_AddObject(m, "Midictl_base", (PyObject *)&MidictlType);

    if (PyType_Ready(&CtlScanType) < 0)
        return;
    Py_INCREF(&CtlScanType);
    PyModule_AddObject(m, "CtlScan_base", (PyObject *)&CtlScanType);

    if (PyType_Ready(&MidiNoteType) < 0)
        return;
    Py_INCREF(&MidiNoteType);
    PyModule_AddObject(m, "MidiNote_base", (PyObject *)&MidiNoteType);

    if (PyType_Ready(&NoteinType) < 0)
        return;
    Py_INCREF(&NoteinType);
    PyModule_AddObject(m, "Notein_base", (PyObject *)&NoteinType);

    if (PyType_Ready(&BendinType) < 0)
        return;
    Py_INCREF(&BendinType);
    PyModule_AddObject(m, "Bendin_base", (PyObject *)&BendinType);

    if (PyType_Ready(&TouchinType) < 0)
        return;
    Py_INCREF(&TouchinType);
    PyModule_AddObject(m, "Touchin_base", (PyObject *)&TouchinType);

    if (PyType_Ready(&PrograminType) < 0)
        return;
    Py_INCREF(&PrograminType);
    PyModule_AddObject(m, "Programin_base", (PyObject *)&PrograminType);
    
    if (PyType_Ready(&MidiAdsrType) < 0)
        return;
    Py_INCREF(&MidiAdsrType);
    PyModule_AddObject(m, "MidiAdsr_base", (PyObject *)&MidiAdsrType);

    if (PyType_Ready(&MidiDelAdsrType) < 0)
        return;
    Py_INCREF(&MidiDelAdsrType);
    PyModule_AddObject(m, "MidiDelAdsr_base", (PyObject *)&MidiDelAdsrType);
    
    if (PyType_Ready(&OscSendType) < 0)
        return;
    Py_INCREF(&OscSendType);
    PyModule_AddObject(m, "OscSend_base", (PyObject *)&OscSendType);

    if (PyType_Ready(&OscDataSendType) < 0)
        return;
    Py_INCREF(&OscDataSendType);
    PyModule_AddObject(m, "OscDataSend_base", (PyObject *)&OscDataSendType);
    
    if (PyType_Ready(&OscReceiveType) < 0)
        return;
    Py_INCREF(&OscReceiveType);
    PyModule_AddObject(m, "OscReceive_base", (PyObject *)&OscReceiveType);

    if (PyType_Ready(&OscReceiverType) < 0)
        return;
    Py_INCREF(&OscReceiverType);
    PyModule_AddObject(m, "OscReceiver_base", (PyObject *)&OscReceiverType);

    if (PyType_Ready(&OscListReceiveType) < 0)
        return;
    Py_INCREF(&OscListReceiveType);
    PyModule_AddObject(m, "OscListReceive_base", (PyObject *)&OscListReceiveType);
    
    if (PyType_Ready(&OscListReceiverType) < 0)
        return;
    Py_INCREF(&OscListReceiverType);
    PyModule_AddObject(m, "OscListReceiver_base", (PyObject *)&OscListReceiverType);
    
    if (PyType_Ready(&OscDataReceiveType) < 0)
        return;
    Py_INCREF(&OscDataReceiveType);
    PyModule_AddObject(m, "OscDataReceive_base", (PyObject *)&OscDataReceiveType);
    
    if (PyType_Ready(&TrigRandType) < 0)
        return;
    Py_INCREF(&TrigRandType);
    PyModule_AddObject(m, "TrigRand_base", (PyObject *)&TrigRandType);

    if (PyType_Ready(&TrigRandIntType) < 0)
        return;
    Py_INCREF(&TrigRandIntType);
    PyModule_AddObject(m, "TrigRandInt_base", (PyObject *)&TrigRandIntType);
    
    if (PyType_Ready(&TrigChoiceType) < 0)
        return;
    Py_INCREF(&TrigChoiceType);
    PyModule_AddObject(m, "TrigChoice_base", (PyObject *)&TrigChoiceType);

    if (PyType_Ready(&IterType) < 0)
        return;
    Py_INCREF(&IterType);
    PyModule_AddObject(m, "Iter_base", (PyObject *)&IterType);
    
    if (PyType_Ready(&TrigEnvType) < 0)
        return;
    Py_INCREF(&TrigEnvType);
    PyModule_AddObject(m, "TrigEnv_base", (PyObject *)&TrigEnvType);

    if (PyType_Ready(&TrigLinsegType) < 0)
        return;
    Py_INCREF(&TrigLinsegType);
    PyModule_AddObject(m, "TrigLinseg_base", (PyObject *)&TrigLinsegType);

    if (PyType_Ready(&TrigExpsegType) < 0)
        return;
    Py_INCREF(&TrigExpsegType);
    PyModule_AddObject(m, "TrigExpseg_base", (PyObject *)&TrigExpsegType);
    
    if (PyType_Ready(&TrigFuncType) < 0)
        return;
    Py_INCREF(&TrigFuncType);
    PyModule_AddObject(m, "TrigFunc_base", (PyObject *)&TrigFuncType);

    if (PyType_Ready(&TrigXnoiseType) < 0)
        return;
    Py_INCREF(&TrigXnoiseType);
    PyModule_AddObject(m, "TrigXnoise_base", (PyObject *)&TrigXnoiseType);

    if (PyType_Ready(&TrigXnoiseMidiType) < 0)
        return;
    Py_INCREF(&TrigXnoiseMidiType);
    PyModule_AddObject(m, "TrigXnoiseMidi_base", (PyObject *)&TrigXnoiseMidiType);
    
    if (PyType_Ready(&PatternType) < 0)
        return;
    Py_INCREF(&PatternType);
    PyModule_AddObject(m, "Pattern_base", (PyObject *)&PatternType);

    if (PyType_Ready(&CallAfterType) < 0)
        return;
    Py_INCREF(&CallAfterType);
    PyModule_AddObject(m, "CallAfter_base", (PyObject *)&CallAfterType);
    
    if (PyType_Ready(&BandSplitterType) < 0)
        return;
    Py_INCREF(&BandSplitterType);
    PyModule_AddObject(m, "BandSplitter_base", (PyObject *)&BandSplitterType);

    if (PyType_Ready(&BandSplitType) < 0)
        return;
    Py_INCREF(&BandSplitType);
    PyModule_AddObject(m, "BandSplit_base", (PyObject *)&BandSplitType);

    if (PyType_Ready(&FourBandMainType) < 0)
        return;
    Py_INCREF(&FourBandMainType);
    PyModule_AddObject(m, "FourBandMain_base", (PyObject *)&FourBandMainType);
    
    if (PyType_Ready(&FourBandType) < 0)
        return;
    Py_INCREF(&FourBandType);
    PyModule_AddObject(m, "FourBand_base", (PyObject *)&FourBandType);
    
    if (PyType_Ready(&HilbertMainType) < 0)
        return;
    Py_INCREF(&HilbertMainType);
    PyModule_AddObject(m, "HilbertMain_base", (PyObject *)&HilbertMainType);

    if (PyType_Ready(&HilbertType) < 0)
        return;
    Py_INCREF(&HilbertType);
    PyModule_AddObject(m, "Hilbert_base", (PyObject *)&HilbertType);

    if (PyType_Ready(&FollowerType) < 0)
        return;
    Py_INCREF(&FollowerType);
    PyModule_AddObject(m, "Follower_base", (PyObject *)&FollowerType);

    if (PyType_Ready(&Follower2Type) < 0)
        return;
    Py_INCREF(&Follower2Type);
    PyModule_AddObject(m, "Follower2_base", (PyObject *)&Follower2Type);
    
    if (PyType_Ready(&ZCrossType) < 0)
        return;
    Py_INCREF(&ZCrossType);
    PyModule_AddObject(m, "ZCross_base", (PyObject *)&ZCrossType);
    
    if (PyType_Ready(&SPannerType) < 0)
        return;
    Py_INCREF(&SPannerType);
    PyModule_AddObject(m, "SPanner_base", (PyObject *)&SPannerType);
    
    if (PyType_Ready(&PannerType) < 0)
        return;
    Py_INCREF(&PannerType);
    PyModule_AddObject(m, "Panner_base", (PyObject *)&PannerType);

    if (PyType_Ready(&PanType) < 0)
        return;
    Py_INCREF(&PanType);
    PyModule_AddObject(m, "Pan_base", (PyObject *)&PanType);

    if (PyType_Ready(&SPanType) < 0)
        return;
    Py_INCREF(&SPanType);
    PyModule_AddObject(m, "SPan_base", (PyObject *)&SPanType);

    if (PyType_Ready(&SwitcherType) < 0)
        return;
    Py_INCREF(&SwitcherType);
    PyModule_AddObject(m, "Switcher_base", (PyObject *)&SwitcherType);
    
    if (PyType_Ready(&SwitchType) < 0)
        return;
    Py_INCREF(&SwitchType);
    PyModule_AddObject(m, "Switch_base", (PyObject *)&SwitchType);

    if (PyType_Ready(&SelectorType) < 0)
        return;
    Py_INCREF(&SelectorType);
    PyModule_AddObject(m, "Selector_base", (PyObject *)&SelectorType);

    if (PyType_Ready(&VoiceManagerType) < 0)
        return;
    Py_INCREF(&VoiceManagerType);
    PyModule_AddObject(m, "VoiceManager_base", (PyObject *)&VoiceManagerType);

    if (PyType_Ready(&MixerType) < 0)
        return;
    Py_INCREF(&MixerType);
    PyModule_AddObject(m, "Mixer_base", (PyObject *)&MixerType);
    
    if (PyType_Ready(&MixerVoiceType) < 0)
        return;
    Py_INCREF(&MixerVoiceType);
    PyModule_AddObject(m, "MixerVoice_base", (PyObject *)&MixerVoiceType);
    
    if (PyType_Ready(&CounterType) < 0)
        return;
    Py_INCREF(&CounterType);
    PyModule_AddObject(m, "Counter_base", (PyObject *)&CounterType);

    if (PyType_Ready(&CountType) < 0)
        return;
    Py_INCREF(&CountType);
    PyModule_AddObject(m, "Count_base", (PyObject *)&CountType);
    
    if (PyType_Ready(&ThreshType) < 0)
        return;
    Py_INCREF(&ThreshType);
    PyModule_AddObject(m, "Thresh_base", (PyObject *)&ThreshType);

    if (PyType_Ready(&PercentType) < 0)
        return;
    Py_INCREF(&PercentType);
    PyModule_AddObject(m, "Percent_base", (PyObject *)&PercentType);

    if (PyType_Ready(&TimerType) < 0)
        return;
    Py_INCREF(&TimerType);
    PyModule_AddObject(m, "Timer_base", (PyObject *)&TimerType);
    
    if (PyType_Ready(&SelectType) < 0)
        return;
    Py_INCREF(&SelectType);
    PyModule_AddObject(m, "Select_base", (PyObject *)&SelectType);

    if (PyType_Ready(&ChangeType) < 0)
        return;
    Py_INCREF(&ChangeType);
    PyModule_AddObject(m, "Change_base", (PyObject *)&ChangeType);

    if (PyType_Ready(&ScoreType) < 0)
        return;
    Py_INCREF(&ScoreType);
    PyModule_AddObject(m, "Score_base", (PyObject *)&ScoreType);
    
    if (PyType_Ready(&FreeverbType) < 0)
        return;
    Py_INCREF(&FreeverbType);
    PyModule_AddObject(m, "Freeverb_base", (PyObject *)&FreeverbType);

    if (PyType_Ready(&WGVerbType) < 0)
        return;
    Py_INCREF(&WGVerbType);
    PyModule_AddObject(m, "WGVerb_base", (PyObject *)&WGVerbType);

    if (PyType_Ready(&ChorusType) < 0)
        return;
    Py_INCREF(&ChorusType);
    PyModule_AddObject(m, "Chorus_base", (PyObject *)&ChorusType);
    
    if (PyType_Ready(&ConvolveType) < 0)
        return;
    Py_INCREF(&ConvolveType);
    PyModule_AddObject(m, "Convolve_base", (PyObject *)&ConvolveType);

    if (PyType_Ready(&IRWinSincType) < 0)
        return;
    Py_INCREF(&IRWinSincType);
    PyModule_AddObject(m, "IRWinSinc_base", (PyObject *)&IRWinSincType);

    if (PyType_Ready(&IRPulseType) < 0)
        return;
    Py_INCREF(&IRPulseType);
    PyModule_AddObject(m, "IRPulse_base", (PyObject *)&IRPulseType);
    
    if (PyType_Ready(&IRAverageType) < 0)
        return;
    Py_INCREF(&IRAverageType);
    PyModule_AddObject(m, "IRAverage_base", (PyObject *)&IRAverageType);

    if (PyType_Ready(&IRFMType) < 0)
        return;
    Py_INCREF(&IRFMType);
    PyModule_AddObject(m, "IRFM_base", (PyObject *)&IRFMType);
    
    if (PyType_Ready(&GranulatorType) < 0)
        return;
    Py_INCREF(&GranulatorType);
    PyModule_AddObject(m, "Granulator_base", (PyObject *)&GranulatorType);

    if (PyType_Ready(&LooperType) < 0)
        return;
    Py_INCREF(&LooperType);
    PyModule_AddObject(m, "Looper_base", (PyObject *)&LooperType);
    
	if (PyType_Ready(&HarmonizerType) < 0)
        return;
    Py_INCREF(&HarmonizerType);
    PyModule_AddObject(m, "Harmonizer_base", (PyObject *)&HarmonizerType);
	
    if (PyType_Ready(&PrintType) < 0)
        return;
    Py_INCREF(&PrintType);
    PyModule_AddObject(m, "Print_base", (PyObject *)&PrintType);

    if (PyType_Ready(&M_SinType) < 0)
        return;
    Py_INCREF(&M_SinType);
    PyModule_AddObject(m, "M_Sin_base", (PyObject *)&M_SinType);    

    if (PyType_Ready(&M_CosType) < 0)
        return;
    Py_INCREF(&M_CosType);
    PyModule_AddObject(m, "M_Cos_base", (PyObject *)&M_CosType);    

    if (PyType_Ready(&M_TanType) < 0)
        return;
    Py_INCREF(&M_TanType);
    PyModule_AddObject(m, "M_Tan_base", (PyObject *)&M_TanType);

    if (PyType_Ready(&M_AbsType) < 0)
        return;
    Py_INCREF(&M_AbsType);
    PyModule_AddObject(m, "M_Abs_base", (PyObject *)&M_AbsType);    

    if (PyType_Ready(&M_SqrtType) < 0)
        return;
    Py_INCREF(&M_SqrtType);
    PyModule_AddObject(m, "M_Sqrt_base", (PyObject *)&M_SqrtType);    

    if (PyType_Ready(&M_LogType) < 0)
        return;
    Py_INCREF(&M_LogType);
    PyModule_AddObject(m, "M_Log_base", (PyObject *)&M_LogType);    

    if (PyType_Ready(&M_Log2Type) < 0)
        return;
    Py_INCREF(&M_Log2Type);
    PyModule_AddObject(m, "M_Log2_base", (PyObject *)&M_Log2Type);    

    if (PyType_Ready(&M_Log10Type) < 0)
        return;
    Py_INCREF(&M_Log10Type);
    PyModule_AddObject(m, "M_Log10_base", (PyObject *)&M_Log10Type);    

    if (PyType_Ready(&M_PowType) < 0)
        return;
    Py_INCREF(&M_PowType);
    PyModule_AddObject(m, "M_Pow_base", (PyObject *)&M_PowType);    

    if (PyType_Ready(&M_Atan2Type) < 0)
        return;
    Py_INCREF(&M_Atan2Type);
    PyModule_AddObject(m, "M_Atan2_base", (PyObject *)&M_Atan2Type);    

    if (PyType_Ready(&M_FloorType) < 0)
        return;
    Py_INCREF(&M_FloorType);
    PyModule_AddObject(m, "M_Floor_base", (PyObject *)&M_FloorType);    

    if (PyType_Ready(&M_CeilType) < 0)
        return;
    Py_INCREF(&M_CeilType);
    PyModule_AddObject(m, "M_Ceil_base", (PyObject *)&M_CeilType);    
    
    if (PyType_Ready(&M_RoundType) < 0)
        return;
    Py_INCREF(&M_RoundType);
    PyModule_AddObject(m, "M_Round_base", (PyObject *)&M_RoundType);    
    
    if (PyType_Ready(&SnapType) < 0)
        return;
    Py_INCREF(&SnapType);
    PyModule_AddObject(m, "Snap_base", (PyObject *)&SnapType);

    if (PyType_Ready(&InterpType) < 0)
        return;
    Py_INCREF(&InterpType);
    PyModule_AddObject(m, "Interp_base", (PyObject *)&InterpType);

    if (PyType_Ready(&SampHoldType) < 0)
        return;
    Py_INCREF(&SampHoldType);
    PyModule_AddObject(m, "SampHold_base", (PyObject *)&SampHoldType);

    if (PyType_Ready(&DBToAType) < 0)
        return;
    Py_INCREF(&DBToAType);
    PyModule_AddObject(m, "DBToA_base", (PyObject *)&DBToAType);

    if (PyType_Ready(&AToDBType) < 0)
        return;
    Py_INCREF(&AToDBType);
    PyModule_AddObject(m, "AToDB_base", (PyObject *)&AToDBType);

    if (PyType_Ready(&ScaleType) < 0)
        return;
    Py_INCREF(&ScaleType);
    PyModule_AddObject(m, "Scale_base", (PyObject *)&ScaleType);

    if (PyType_Ready(&CentsToTranspoType) < 0)
        return;
    Py_INCREF(&CentsToTranspoType);
    PyModule_AddObject(m, "CentsToTranspo_base", (PyObject *)&CentsToTranspoType);

    if (PyType_Ready(&TranspoToCentsType) < 0)
        return;
    Py_INCREF(&TranspoToCentsType);
    PyModule_AddObject(m, "TranspoToCents_base", (PyObject *)&TranspoToCentsType);

    if (PyType_Ready(&MToFType) < 0)
        return;
    Py_INCREF(&MToFType);
    PyModule_AddObject(m, "MToF_base", (PyObject *)&MToFType);

    if (PyType_Ready(&MToTType) < 0)
        return;
    Py_INCREF(&MToTType);
    PyModule_AddObject(m, "MToT_base", (PyObject *)&MToTType);
    
    if (PyType_Ready(&FFTMainType) < 0)
        return;
    Py_INCREF(&FFTMainType);
    PyModule_AddObject(m, "FFTMain_base", (PyObject *)&FFTMainType);

    if (PyType_Ready(&FFTType) < 0)
        return;
    Py_INCREF(&FFTType);
    PyModule_AddObject(m, "FFT_base", (PyObject *)&FFTType);

    if (PyType_Ready(&IFFTType) < 0)
        return;
    Py_INCREF(&IFFTType);
    PyModule_AddObject(m, "IFFT_base", (PyObject *)&IFFTType);

    if (PyType_Ready(&CarToPolType) < 0)
        return;
    Py_INCREF(&CarToPolType);
    PyModule_AddObject(m, "CarToPol_base", (PyObject *)&CarToPolType);

    if (PyType_Ready(&PolToCarType) < 0)
        return;
    Py_INCREF(&PolToCarType);
    PyModule_AddObject(m, "PolToCar_base", (PyObject *)&PolToCarType);

    if (PyType_Ready(&FrameDeltaMainType) < 0)
        return;
    Py_INCREF(&FrameDeltaMainType);
    PyModule_AddObject(m, "FrameDeltaMain_base", (PyObject *)&FrameDeltaMainType);
    
    if (PyType_Ready(&FrameDeltaType) < 0)
        return;
    Py_INCREF(&FrameDeltaType);
    PyModule_AddObject(m, "FrameDelta_base", (PyObject *)&FrameDeltaType);

    if (PyType_Ready(&FrameAccumType) < 0)
        return;
    Py_INCREF(&FrameAccumType);
    PyModule_AddObject(m, "FrameAccum_base", (PyObject *)&FrameAccumType);

    if (PyType_Ready(&FrameAccumMainType) < 0)
        return;
    Py_INCREF(&FrameAccumMainType);
    PyModule_AddObject(m, "FrameAccumMain_base", (PyObject *)&FrameAccumMainType);

    if (PyType_Ready(&VectralMainType) < 0)
        return;
    Py_INCREF(&VectralMainType);
    PyModule_AddObject(m, "VectralMain_base", (PyObject *)&VectralMainType);
    
    if (PyType_Ready(&VectralType) < 0)
        return;
    Py_INCREF(&VectralType);
    PyModule_AddObject(m, "Vectral_base", (PyObject *)&VectralType);
    
}
