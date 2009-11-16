/*
 *  pyomodule.c
 *  
 *
 *  Created by Olivier Bélanger on 24/10/09.
 *
 */

#include <Python.h>
#include "portaudio.h"
#include "pyomodule.h"
#include "servermodule.h"
#include "streammodule.h"
#include "dummymodule.h"
#include "tablemodule.h"

/* Portaudio stuff */
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

static PyObject*
portaudio_count_devices(){
    int numDevices;
    
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 ) {
        printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
    }
    return PyInt_FromLong(numDevices);
}

static PyObject*
portaudio_list_devices(){
    int n = Pa_GetDeviceCount();
    if (n < 0){
        portaudio_assert(n, "Pa_GetDeviceCount");
    }
    
    int i;
    for (i=0; i < n; ++i){
        const PaDeviceInfo *info=Pa_GetDeviceInfo(i);
        assert(info);
        
        if (info->maxInputChannels > 0){
            fprintf(stdout, "%i) IN %s default: %i Hz, %f s latency\n", i, info->name, (int)info->defaultSampleRate, (float)info->defaultLowInputLatency);
        }
        if (info->maxOutputChannels > 0){
            fprintf(stdout, "%i) OUT %s default: %i Hz, %f s latency\n", i, info->name, (int)info->defaultSampleRate, (float)info->defaultLowOutputLatency);
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef pyo_functions[] = {
{"pa_count_devices", (PyCFunction)portaudio_count_devices, METH_NOARGS, "Returns the number of devices founded by Portaudio."},
{"pa_list_devices", (PyCFunction)portaudio_list_devices, METH_NOARGS, "Lists all devices founded by Portaudio."},
{NULL, NULL, 0, NULL},
};

PyMODINIT_FUNC
init_pyo(void)
{
    PyObject *m;
    
    m = Py_InitModule3("_pyo", pyo_functions, "Python digital signal processing module.");

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

    if (PyType_Ready(&MixType) < 0)
        return;
    Py_INCREF(&MixType);
    PyModule_AddObject(m, "Mix_base", (PyObject *)&MixType);
    
    if (PyType_Ready(&TableStreamType) < 0)
        return;
    Py_INCREF(&TableStreamType);
    PyModule_AddObject(m, "TableStream", (PyObject *)&TableStreamType);
    
    if (PyType_Ready(&HarmTableType) < 0)
        return;
    Py_INCREF(&HarmTableType);
    PyModule_AddObject(m, "HarmTable", (PyObject *)&HarmTableType);

    if (PyType_Ready(&HannTableType) < 0)
        return;
    Py_INCREF(&HannTableType);
    PyModule_AddObject(m, "HannTable", (PyObject *)&HannTableType);
    
    if (PyType_Ready(&SndTableType) < 0)
        return;
    Py_INCREF(&SndTableType);
    PyModule_AddObject(m, "SndTable", (PyObject *)&SndTableType);

    if (PyType_Ready(&InputType) < 0)
        return;
    Py_INCREF(&InputType);
    PyModule_AddObject(m, "Input_base", (PyObject *)&InputType);

    if (PyType_Ready(&FaderType) < 0)
        return;
    Py_INCREF(&FaderType);
    PyModule_AddObject(m, "Fader_base", (PyObject *)&FaderType);
    
    if (PyType_Ready(&OscType) < 0)
        return;
    Py_INCREF(&OscType);
    PyModule_AddObject(m, "Osc_base", (PyObject *)&OscType);

    if (PyType_Ready(&SineType) < 0)
        return;
    Py_INCREF(&SineType);
    PyModule_AddObject(m, "Sine_base", (PyObject *)&SineType);

    if (PyType_Ready(&NoiseType) < 0)
        return;
    Py_INCREF(&NoiseType);
    PyModule_AddObject(m, "Noise_base", (PyObject *)&NoiseType);
    
    if (PyType_Ready(&BiquadType) < 0)
        return;
    Py_INCREF(&BiquadType);
    PyModule_AddObject(m, "Biquad_base", (PyObject *)&BiquadType);

    if (PyType_Ready(&DistoType) < 0)
        return;
    Py_INCREF(&DistoType);
    PyModule_AddObject(m, "Disto_base", (PyObject *)&DistoType);

    if (PyType_Ready(&MidictlType) < 0)
        return;
    Py_INCREF(&MidictlType);
    PyModule_AddObject(m, "Midictl_base", (PyObject *)&MidictlType);

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
    
}