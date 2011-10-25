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
"\nRetrieve informations about a soundfile.\n\n\
Prints the infos of the given soundfile to the console and returns a tuple containing:\n (number of frames, duration in seconds, sampling rate, number of channels, file format, sample type).\n\nsndinfo(path, print=False)\n\nParameters:\n\n    \
path : string\n        Path of a valid soundfile.\n    \
print : boolean, optional\n        If True, sndinfo will print sound infos to the console. Defaults to False.\n\n"


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
        return NULL;

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
        4 : 64 bit float\n\n"

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

/****** Algorithm utilities ******/
#define reducePoints_info \
"\nDouglas–Peucker curve reduction algorithm.\n\n\
This function receives a list of points as input and returns a simplified list by eliminating redundancies.\n\n\
A point is a tuple (or a list) of two floats, time and value. A list of points looks like: [(0, 0), (0.1, 0.7), (0.2, 0.5), ...]\n\n\
reducePoints(pointlist, tolerance=0.02)\n\nParameters:\n\n    \
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
{"pa_get_output_devices", (PyCFunction)portaudio_get_output_devices, METH_NOARGS, "\nReturns output devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portaudio index of each device."},
{"pa_get_input_devices", (PyCFunction)portaudio_get_input_devices, METH_NOARGS, "\nReturns input devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portaudio index of each device."},
{"pa_list_host_apis", (PyCFunction)portaudio_list_host_apis, METH_NOARGS, "Prints a list of all host apis found by Portaudio."},
{"pa_get_default_input", (PyCFunction)portaudio_get_default_input, METH_NOARGS, "Returns Portaudio default input device."},
{"pa_get_default_host_api", (PyCFunction)portaudio_get_default_host_api, METH_NOARGS, "Returns Portaudio default host_api."},
{"pa_get_default_output", (PyCFunction)portaudio_get_default_output, METH_NOARGS, "Returns Portaudio default output device."},
{"pm_count_devices", (PyCFunction)portmidi_count_devices, METH_NOARGS, "Returns the number of devices found by Portmidi."},
{"pm_list_devices", (PyCFunction)portmidi_list_devices, METH_NOARGS, "Prints a list of all devices found by Portmidi."},
{"pm_get_input_devices", (PyCFunction)portmidi_get_input_devices, METH_NOARGS, "\nReturns Midi input devices (device names, device indexes) found by Portmidi.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portmidi index of each device."},
{"pm_get_default_input", (PyCFunction)portmidi_get_default_input, METH_NOARGS, "\nReturns Portmidi default input device.\n\n Returns the device id or -1 if no interface was found."},
{"pm_get_output_devices", (PyCFunction)portmidi_get_output_devices, METH_NOARGS, "\nReturns Midi output devices (device names, device indexes) found by Portmidi.\n\n`device names` is a list of strings and `device indexes` is a list of the actual Portmidi index of each device."},
{"pm_get_default_output", (PyCFunction)portmidi_get_default_output, METH_NOARGS, "\nReturns Portmidi default output device.\n\n Returns the device id or -1 if no interface was found."},
{"sndinfo", (PyCFunction)sndinfo, METH_VARARGS|METH_KEYWORDS, sndinfo_info},
{"savefile", (PyCFunction)savefile, METH_VARARGS|METH_KEYWORDS, savefile_info},
{"reducePoints", (PyCFunction)reducePoints, METH_VARARGS|METH_KEYWORDS, reducePoints_info},
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

    if (PyType_Ready(&ControlRecType) < 0)
        return;
    Py_INCREF(&ControlRecType);
    PyModule_AddObject(m, "ControlRec_base", (PyObject *)&ControlRecType);

    if (PyType_Ready(&ControlReadType) < 0)
        return;
    Py_INCREF(&ControlReadType);
    PyModule_AddObject(m, "ControlRead_base", (PyObject *)&ControlReadType);

    if (PyType_Ready(&ControlReadTrigType) < 0)
        return;
    Py_INCREF(&ControlReadTrigType);
    PyModule_AddObject(m, "ControlReadTrig_base", (PyObject *)&ControlReadTrigType);

    if (PyType_Ready(&NoteinRecType) < 0)
        return;
    Py_INCREF(&NoteinRecType);
    PyModule_AddObject(m, "NoteinRec_base", (PyObject *)&NoteinRecType);

    if (PyType_Ready(&NoteinReadType) < 0)
        return;
    Py_INCREF(&NoteinReadType);
    PyModule_AddObject(m, "NoteinRead_base", (PyObject *)&NoteinReadType);

    if (PyType_Ready(&NoteinReadTrigType) < 0)
        return;
    Py_INCREF(&NoteinReadTrigType);
    PyModule_AddObject(m, "NoteinReadTrig_base", (PyObject *)&NoteinReadTrigType);


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

    if (PyType_Ready(&TableRecTrigType) < 0)
        return;
    Py_INCREF(&TableRecTrigType);
    PyModule_AddObject(m, "TableRecTrig_base", (PyObject *)&TableRecTrigType);
    
    if (PyType_Ready(&TableMorphType) < 0)
        return;
    Py_INCREF(&TableMorphType);
    PyModule_AddObject(m, "TableMorph_base", (PyObject *)&TableMorphType);

    if (PyType_Ready(&TrigTableRecType) < 0)
        return;
    Py_INCREF(&TrigTableRecType);
    PyModule_AddObject(m, "TrigTableRec_base", (PyObject *)&TrigTableRecType);
    
    if (PyType_Ready(&TrigTableRecTrigType) < 0)
        return;
    Py_INCREF(&TrigTableRecTrigType);
    PyModule_AddObject(m, "TrigTableRecTrig_base", (PyObject *)&TrigTableRecTrigType);
    
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

    if (PyType_Ready(&OscBankType) < 0)
        return;
    Py_INCREF(&OscBankType);
    PyModule_AddObject(m, "OscBank_base", (PyObject *)&OscBankType);
    
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
    
}
