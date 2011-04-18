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

static PyObject *
portaudio_count_host_api(){
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
            for (i=0; i < n; ++i){
                const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
                assert(info);
                
                if (info->maxInputChannels > 0)
                    fprintf(stdout, "%i: IN, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);
                
                if (info->maxOutputChannels > 0)
                    fprintf(stdout, "%i: OUT, name: %s, host api index: %i, default sr: %i Hz, latency: %f s\n", i, info->name, (int)info->hostApi, (int)info->defaultSampleRate, (float)info->defaultLowOutputLatency);
            }
        }        
    }
    Py_RETURN_NONE;
}

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
static PyObject *
portmidi_count_devices(){
    int numDevices;
	numDevices = Pm_CountDevices();
    return PyInt_FromLong(numDevices);
}

static PyObject *
portmidi_list_devices(){
    int i;
    printf("MIDI input devices:\n");
    for (i = 0; i < Pm_CountDevices(); i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        if (info->input) 
            printf("%d: %s, %s\n", i, info->interf, info->name);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
portmidi_get_input_devices(){
	int n, i;
    
    PyObject *list, *list_index;
    list = PyList_New(0);
    list_index = PyList_New(0);

    n = Pm_CountDevices();
    if (n < 0){
        printf("Portmidi warning: No Midi interface found\n");
    }
    else {
        for (i=0; i < n; i++){
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        
            if (info->input){
                printf("%d: %s, %s\n", i, info->interf, info->name);
                PyList_Append(list, PyString_FromString(info->name));
                PyList_Append(list_index, PyInt_FromLong(i));
            }
        }
    }
    return Py_BuildValue("OO", list, list_index);
}

static PyObject *
portmidi_get_default_input(){
    PmDeviceID i;
    
    i = Pm_GetDefaultInputDeviceID();
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (info->input) 
        printf("%d: %s, %s\n", i, info->interf, info->name);

    return PyInt_FromLong(i);
}

/****** Libsndfile utilities ******/
#define sndinfo_info \
"\nRetrieve informations about a soundfile.\n\n\
Prints the infos of the given soundfile to the console and returns a tuple containing (number of frames, duration in seconds, sampling rate, number of channels).\n\nsndinfo(path)\n\nParameters:\n\n    \
path : string\n        Path of a valid soundfile.\n\n"


static PyObject *
sndinfo(PyObject *self, PyObject *args) {
    
    SNDFILE *sf;
    SF_INFO info;
    char *path;

    if (! PyArg_ParseTuple(args, "s", &path))
        return NULL;

    /* Open the sound file. */
    info.format = 0;
    sf = sf_open(path, SFM_READ, &info);
    if (sf == NULL)
    {
        printf("Failed to open the file.\n");
        Py_RETURN_NONE;
    }
    else {
        fprintf(stdout, "name: %s\nnumber of frames: %i\nduration: %.4f sec\nsr: %.2f\nchannels: %i\n", path, (int)info.frames, ((float)info.frames / info.samplerate), (float)info.samplerate, (int)info.channels);
        PyObject *sndinfo = PyTuple_Pack(4, PyInt_FromLong(info.frames), PyFloat_FromDouble((float)info.frames / info.samplerate), PyFloat_FromDouble(info.samplerate), PyInt_FromLong(info.channels));
        sf_close(sf);
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
        1 : 24 bits int\n    \
        2 : 32 bits int\n    \
        3 : 32 bits float\n    \
        4 : 64 bits float\n\n"

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
        printf ("Not able to open output file %s.\n", recpath) ;
    }
    SF_WRITE(recfile, sampsarray, size);
    sf_close(recfile);
    free(sampsarray);
    
    Py_RETURN_NONE;    
}

/****** Conversion utilities ******/
static PyObject *
midiToHz(PyObject *self, PyObject *arg) {
    return Py_BuildValue("d", 8.1757989156437 * pow(1.0594630943593, PyFloat_AsDouble(PyNumber_Float(arg))));
}    

static PyObject *
midiToTranspo(PyObject *self, PyObject *arg) {
    return Py_BuildValue("d", pow(1.0594630943593, PyFloat_AsDouble(PyNumber_Float(arg))-60.0));
}    

static PyObject *
sampsToSec(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    return Py_BuildValue("d", PyFloat_AsDouble(PyNumber_Float(arg)) / sr);
}                         

static PyObject *
secToSamps(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    return Py_BuildValue("i", (int)(PyFloat_AsDouble(PyNumber_Float(arg)) * sr));
}                         

static PyMethodDef pyo_functions[] = {
{"pa_count_devices", (PyCFunction)portaudio_count_devices, METH_NOARGS, "Returns the number of devices found by Portaudio."},
{"pa_count_host_apis", (PyCFunction)portaudio_count_host_api, METH_NOARGS, "Returns the number of host apis found by Portaudio."},
{"pa_list_devices", (PyCFunction)portaudio_list_devices, METH_NOARGS, "Prints a list of all devices found by Portaudio."},
{"pa_get_output_devices", (PyCFunction)portaudio_get_output_devices, METH_NOARGS, "Returns output devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portaudio index of each device."},
{"pa_get_input_devices", (PyCFunction)portaudio_get_input_devices, METH_NOARGS, "Returns input devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portaudio index of each device."},
{"pa_list_host_apis", (PyCFunction)portaudio_list_host_apis, METH_NOARGS, "Prints a list of all host apis found by Portaudio."},
{"pa_get_default_input", (PyCFunction)portaudio_get_default_input, METH_NOARGS, "Returns Portaudio default input device."},
{"pa_get_default_host_api", (PyCFunction)portaudio_get_default_host_api, METH_NOARGS, "Returns Portaudio default host_api."},
{"pa_get_default_output", (PyCFunction)portaudio_get_default_output, METH_NOARGS, "Returns Portaudio default output device."},
{"pm_count_devices", (PyCFunction)portmidi_count_devices, METH_NOARGS, "Returns the number of devices found by Portmidi."},
{"pm_list_devices", (PyCFunction)portmidi_list_devices, METH_NOARGS, "Prints a list of all devices found by Portmidi."},
{"pm_get_input_devices", (PyCFunction)portmidi_get_input_devices, METH_NOARGS, "Returns Midi input devices (device names, device indexes) found by Portmidi.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portmidi index of each device."},
{"pm_get_default_input", (PyCFunction)portmidi_get_default_input, METH_NOARGS, "Returns Portmidi default input device."},
{"sndinfo", (PyCFunction)sndinfo, METH_VARARGS, sndinfo_info},
{"savefile", (PyCFunction)savefile, METH_VARARGS|METH_KEYWORDS, savefile_info},
{"midiToHz", (PyCFunction)midiToHz, METH_O, "Returns the frequency in Hertz equivalent to the given midi note."},
{"midiToTranspo", (PyCFunction)midiToTranspo, METH_O, "Returns the transposition factor equivalent to the given midi note (central key = 60)."},
{"sampsToSec", (PyCFunction)sampsToSec, METH_O, "Returns the number of samples equivalent of the given duration in seconds."},
{"secToSamps", (PyCFunction)secToSamps, METH_O, "Returns the duration in seconds equivalent to the given number of samples."},
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

    if (PyType_Ready(&DummyType) < 0)
        return;
    Py_INCREF(&DummyType);
    PyModule_AddObject(m, "Dummy_base", (PyObject *)&DummyType);

    if (PyType_Ready(&RecordType) < 0)
        return;
    Py_INCREF(&RecordType);
    PyModule_AddObject(m, "Record_base", (PyObject *)&RecordType);
    
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
    
    if (PyType_Ready(&TableRecTrigType) < 0)
        return;
    Py_INCREF(&TableRecTrigType);
    PyModule_AddObject(m, "TableRecTrig_base", (PyObject *)&TableRecTrigType);

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

    if (PyType_Ready(&MatrixRecTrigType) < 0)
        return;
    Py_INCREF(&MatrixRecTrigType);
    PyModule_AddObject(m, "MatrixRecTrig_base", (PyObject *)&MatrixRecTrigType);

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

    if (PyType_Ready(&XnoiseType) < 0)
        return;
    Py_INCREF(&XnoiseType);
    PyModule_AddObject(m, "Xnoise_base", (PyObject *)&XnoiseType);

    if (PyType_Ready(&XnoiseMidiType) < 0)
        return;
    Py_INCREF(&XnoiseMidiType);
    PyModule_AddObject(m, "XnoiseMidi_base", (PyObject *)&XnoiseMidiType);
    
    if (PyType_Ready(&RandIntType) < 0)
        return;
    Py_INCREF(&RandIntType);
    PyModule_AddObject(m, "RandInt_base", (PyObject *)&RandIntType);
    
    if (PyType_Ready(&SfPlayerType) < 0)
        return;
    Py_INCREF(&SfPlayerType);
    PyModule_AddObject(m, "SfPlayer_base", (PyObject *)&SfPlayerType);

    if (PyType_Ready(&SfPlayType) < 0)
        return;
    Py_INCREF(&SfPlayType);
    PyModule_AddObject(m, "SfPlay_base", (PyObject *)&SfPlayType);

    if (PyType_Ready(&SfPlayTrigType) < 0)
        return;
    Py_INCREF(&SfPlayTrigType);
    PyModule_AddObject(m, "SfPlayTrig_base", (PyObject *)&SfPlayTrigType);
    
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
    
    if (PyType_Ready(&TableReadType) < 0)
        return;
    Py_INCREF(&TableReadType);
    PyModule_AddObject(m, "TableRead_base", (PyObject *)&TableReadType);
 
    if (PyType_Ready(&TableReadTrigType) < 0)
        return;
    Py_INCREF(&TableReadTrigType);
    PyModule_AddObject(m, "TableReadTrig_base", (PyObject *)&TableReadTrigType);
    
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

    if (PyType_Ready(&MidiNoteType) < 0)
        return;
    Py_INCREF(&MidiNoteType);
    PyModule_AddObject(m, "MidiNote_base", (PyObject *)&MidiNoteType);

    if (PyType_Ready(&NoteinType) < 0)
        return;
    Py_INCREF(&NoteinType);
    PyModule_AddObject(m, "Notein_base", (PyObject *)&NoteinType);

    if (PyType_Ready(&MidiAdsrType) < 0)
        return;
    Py_INCREF(&MidiAdsrType);
    PyModule_AddObject(m, "MidiAdsr_base", (PyObject *)&MidiAdsrType);
    
    if (PyType_Ready(&OscSendType) < 0)
        return;
    Py_INCREF(&OscSendType);
    PyModule_AddObject(m, "OscSend_base", (PyObject *)&OscSendType);

    if (PyType_Ready(&OscReceiveType) < 0)
        return;
    Py_INCREF(&OscReceiveType);
    PyModule_AddObject(m, "OscReceive_base", (PyObject *)&OscReceiveType);

    if (PyType_Ready(&OscReceiverType) < 0)
        return;
    Py_INCREF(&OscReceiverType);
    PyModule_AddObject(m, "OscReceiver_base", (PyObject *)&OscReceiverType);
    
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
    
    if (PyType_Ready(&TrigEnvType) < 0)
        return;
    Py_INCREF(&TrigEnvType);
    PyModule_AddObject(m, "TrigEnv_base", (PyObject *)&TrigEnvType);

    if (PyType_Ready(&TrigEnvTrigType) < 0)
        return;
    Py_INCREF(&TrigEnvTrigType);
    PyModule_AddObject(m, "TrigEnvTrig_base", (PyObject *)&TrigEnvTrigType);

    if (PyType_Ready(&TrigLinsegType) < 0)
        return;
    Py_INCREF(&TrigLinsegType);
    PyModule_AddObject(m, "TrigLinseg_base", (PyObject *)&TrigLinsegType);

    if (PyType_Ready(&TrigLinsegTrigType) < 0)
        return;
    Py_INCREF(&TrigLinsegTrigType);
    PyModule_AddObject(m, "TrigLinsegTrig_base", (PyObject *)&TrigLinsegTrigType);

    if (PyType_Ready(&TrigExpsegType) < 0)
        return;
    Py_INCREF(&TrigExpsegType);
    PyModule_AddObject(m, "TrigExpseg_base", (PyObject *)&TrigExpsegType);

    if (PyType_Ready(&TrigExpsegTrigType) < 0)
        return;
    Py_INCREF(&TrigExpsegTrigType);
    PyModule_AddObject(m, "TrigExpsegTrig_base", (PyObject *)&TrigExpsegTrigType);
    
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

    if (PyType_Ready(&ThreshType) < 0)
        return;
    Py_INCREF(&ThreshType);
    PyModule_AddObject(m, "Thresh_base", (PyObject *)&ThreshType);

    if (PyType_Ready(&PercentType) < 0)
        return;
    Py_INCREF(&PercentType);
    PyModule_AddObject(m, "Percent_base", (PyObject *)&PercentType);
    
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
    
}
