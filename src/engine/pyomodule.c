/**************************************************************************
 * Copyright 2009-2015 Olivier Belanger                                   *
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

#include <Python.h>
#include <math.h>
#include "py2to3.h"
#include "sndfile.h"
#include "pyomodule.h"
#include "servermodule.h"
#include "streammodule.h"
#include "pvstreammodule.h"
#include "dummymodule.h"
#include "tablemodule.h"
#include "matrixmodule.h"

#ifdef USE_PORTAUDIO
#include "ad_portaudio.h"
static PyObject * with_portaudio() { Py_INCREF(Py_True); return Py_True; };
#else
#define pa_warning "Pyo built without Portaudio support.\n"
static PyObject * portaudio_get_version() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_version_text() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_count_host_apis() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_list_host_apis() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_default_host_api() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_count_devices() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_list_devices() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_devices_infos() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_output_devices() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_output_max_channels(PyObject *self, PyObject *arg) { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_input_max_channels(PyObject *self, PyObject *arg) { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_input_devices() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_default_input() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * portaudio_get_default_output() { PySys_WriteStdout(pa_warning); Py_RETURN_NONE; };
static PyObject * with_portaudio() { Py_INCREF(Py_False); return Py_False; };
#endif

#ifdef USE_PORTMIDI
#include "md_portmidi.h"
static PyObject * with_portmidi() { Py_INCREF(Py_True); return Py_True; };
#else
#define pm_warning "Pyo built without Portmidi sipport.\n"
static PyObject * portmidi_count_devices() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * portmidi_list_devices() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * portmidi_get_input_devices() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * portmidi_get_output_devices() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * portmidi_get_default_input() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * portmidi_get_default_output() { PySys_WriteStdout(pm_warning); Py_RETURN_NONE; };
static PyObject * with_portmidi() { Py_INCREF(Py_False); return Py_False; };
#endif

#ifdef USE_JACK
static PyObject * with_jack() { Py_INCREF(Py_True); return Py_True; };
#else
static PyObject * with_jack() { Py_INCREF(Py_False); return Py_False; };
#endif

#ifdef USE_COREAUDIO
static PyObject * with_coreaudio() { Py_INCREF(Py_True); return Py_True; };
#else
static PyObject * with_coreaudio() { Py_INCREF(Py_False); return Py_False; };
#endif

#ifdef USE_OSC
static PyObject * with_osc() { Py_INCREF(Py_True); return Py_True; };
#else
static PyObject * with_osc() { Py_INCREF(Py_False); return Py_False; };
#endif

/** Portaudio utility functions __doc__ strings. **/
/**************************************************/

#define portaudio_count_host_apis_info \
"\nReturns the number of host apis found by Portaudio.\n\n\
>>> c = pa_count_host_apis()\n\
>>> print(c)\n\
1\n\n"

#define portaudio_get_version_info \
"\nReturns the version number, as an integer, of the current portaudio installation.\n\n\
>>> v = pa_get_version()\n\
>>> print(v)\n\
1899\n\n"

#define portaudio_get_version_text_info \
"\nReturns the textual description of the current portaudio installation.\n\n\
>>> desc = pa_get_version_text()\n\
>>> print(desc)\n\
PortAudio V19-devel (built Oct 8 2012 16:25:16)\n\n"

#define portaudio_list_host_apis_info \
"\nPrints a list of all host apis found by Portaudio.\n\n\
>>> pa_list_host_apis()\n\
index: 0, id: 5, name: Core Audio, num devices: 6, default in: 0, default out: 2\n\n"

#define portaudio_get_default_host_api_info \
"\nReturns the index number of Portaudio's default host api.\n\n\
>>> h = pa_get_default_host_api()\n\
>>> print(h)\n\
0\n\n"

#define portaudio_count_devices_info \
"\nReturns the number of devices found by Portaudio.\n\n\
>>> c = pa_count_devices()\n\
>>> print(c)\n\
6\n\n"

#define portaudio_list_devices_info \
"\nPrints a list of all devices found by Portaudio.\n\n\
>>> pa_list_devices()\n\
AUDIO devices:\n\
0: IN, name: Built-in Microphone, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n\
1: IN, name: Built-in Input, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n\
2: OUT, name: Built-in Output, host api index: 0, default sr: 44100 Hz, latency: 0.001088 s\n\
3: IN, name: UA-4FX, host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n\
3: OUT, name: UA-4FX, host api index: 0, default sr: 44100 Hz, latency: 0.003061 s\n\
4: IN, name: Soundflower (2ch), host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n\
4: OUT, name: Soundflower (2ch), host api index: 0, default sr: 44100 Hz, latency: 0.000000 s\n\
5: IN, name: Soundflower (16ch), host api index: 0, default sr: 44100 Hz, latency: 0.010000 s\n\
5: OUT, name: Soundflower (16ch), host api index: 0, default sr: 44100 Hz, latency: 0.000000 s\n\n"

#define portaudio_get_devices_infos_info \
"\nReturns informations about all devices found by Portaudio.\n\n\
This function returns two dictionaries, one containing a dictionary for each input device and one containing a dictionary for each output device. \
Keys of outer dictionaries are the device index as returned by Portaudio. Keys of inner dictionaries are: 'name', 'host api index', 'default sr' and 'latency'.\n\n\
>>> inputs, outputs = pa_get_devices_infos()\n\
>>> print('- Inputs:')\n\
>>> for index in sorted(inputs.keys()):\n\
...     print('  Device index:', index)\n\
...     for key in ['name', 'host api index', 'default sr', 'latency']:\n\
...         print('    %s:' % key, inputs[index][key])\n\
>>> print('- Outputs:')\n\
>>> for index in sorted(outputs.keys()):\n\
...     print('  Device index:', index)\n\
...     for key in ['name', 'host api index', 'default sr', 'latency']:\n\
...         print('    %s:' % key, outputs[index][key])\n\n"

#define portaudio_get_output_devices_info \
"\nReturns output devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortaudio index of each device.\n\n\
>>> outs = pa_get_output_devices()\n\
>>> print(outs)\n\
(['Built-in Output', 'UA-4FX', 'Soundflower (2ch)', 'Soundflower (16ch)'], [2, 3, 4, 5])\n\n"

#define portaudio_get_output_max_channels_info \
"\nRetrieve the maximum number of output channels for the specified device.\n\n:Args:\n\n    \
x: int\n        Device index as listed by Portaudio (see pa_get_output_devices).\n\n\
>>> device = 'HDA Intel PCH: STAC92xx Analog (hw:0,0)'\n\
>>> dev_list, dev_index =  pa_get_output_devices()\n\
>>> dev = dev_index[dev_list.index(device)]\n\
>>> print('Device index:', dev)\n\
>>> maxouts = pa_get_output_max_channels(dev)\n\
>>> maxins = pa_get_input_max_channels(dev)\n\
>>> print('Max outputs:', maxouts)\n\
>>> print('Max inputs:', maxins)\n\
>>> if maxouts >= 2 and maxins >= 2:\n\
...     nchnls = 2\n\
>>> else:\n\
...     nchnls = 1\n\n"

#define portaudio_get_input_max_channels_info \
"\nRetrieve the maximum number of input channels for the specified device.\n\n:Args:\n\n    \
x: int\n        Device index as listed by Portaudio (see pa_get_input_devices).\n\n\
>>> device = 'HDA Intel PCH: STAC92xx Analog (hw:0,0)'\n\
>>> dev_list, dev_index =  pa_get_output_devices()\n\
>>> dev = dev_index[dev_list.index(device)]\n\
>>> print('Device index:', dev)\n\
>>> maxouts = pa_get_output_max_channels(dev)\n\
>>> maxins = pa_get_input_max_channels(dev)\n\
>>> print('Max outputs', maxouts)\n\
>>> print('Max inputs:', maxins)\n\
>>> if maxouts >= 2 and maxins >= 2:\n\
...     nchnls = 2\n\
>>> else:\n\
...     nchnls = 1\n\n"

#define portaudio_get_input_devices_info \
"\nReturns input devices (device names, device indexes) found by Portaudio.\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortaudio index of each device.\n\n\
>>> ins = pa_get_input_devices()\n\
>>> print(ins)\n\
(['Built-in Microphone', 'Built-in Input', 'UA-4FX', 'Soundflower (2ch)', 'Soundflower (16ch)'], [0, 1, 3, 4, 5])\n\n"

#define portaudio_get_default_input_info \
"\nReturns the index number of Portaudio's default input device.\n\n\
>>> names, indexes = pa_get_input_devices()\n\
>>> name = names[indexes.index(pa_get_default_input())]\n\
>>> print(name)\n\
'Built-in Microphone'\n\n"

#define portaudio_get_default_output_info \
"\nReturns the index number of Portaudio's default output device.\n\n\
>>> names, indexes = pa_get_output_devices()\n\
>>> name = names[indexes.index(pa_get_default_output())]\n\
>>> print(name)\n\
'UA-4FX'\n\n"

/** Portmidi utility functions __doc__ strings. **/
/*************************************************/

#define portmidi_count_devices_info \
"\nReturns the number of devices found by Portmidi.\n\n\
>>> c = pm_count_devices()\n\
>>> print(c)\n\
6\n\n"

#define portmidi_list_devices_info \
"\nPrints a list of all devices found by Portmidi.\n\n\
>>> pm_list_devices()\n\
MIDI devices:\n\
0: IN, name: IAC Driver Bus 1, interface: CoreMIDI\n\
1: IN, name: from MaxMSP 1, interface: CoreMIDI\n\
2: IN, name: from MaxMSP 2, interface: CoreMIDI\n\
3: OUT, name: IAC Driver Bus 1, interface: CoreMIDI\n\
4: OUT, name: to MaxMSP 1, interface: CoreMIDI\n\
5: OUT, name: to MaxMSP 2, interface: CoreMIDI\n\n"

#define portmidi_get_input_devices_info \
"\nReturns midi input devices (device names, device indexes) found by Portmidi.\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortmidi index of each device.\n\n\
>>> ins = pm_get_input_devices()\n\
>>> print(ins)\n\
(['IAC Driver Bus 1', 'from MaxMSP 1', 'from MaxMSP 2'], [0, 1, 2])\n\n"

#define portmidi_get_output_devices_info \
"\nReturns midi output devices (device names, device indexes) found by Portmidi.\n\n`device names` is a list of strings and `device indexes` is a list of the actual\nPortmidi index of each device.\n\n\
>>> outs = pm_get_output_devices()\n\
>>> print(outs)\n\
(['IAC Driver Bus 1', 'to MaxMSP 1', 'to MaxMSP 2'], [3, 4, 5])\n\n"

#define portmidi_get_default_input_info \
"\nReturns the index number of Portmidi's default input device.\n\n\
>>> names, indexes = pm_get_input_devices()\n\
>>> name = names[indexes.index(pm_get_default_input())]\n\
>>> print(name)\n\
'IAC Driver Bus 1'\n\n"

#define portmidi_get_default_output_info \
"\nReturns the index number of Portmidi's default output device.\n\n\
>>> names, indexes = pm_get_output_devices()\n\
>>> name = names[indexes.index(pm_get_default_output())]\n\
>>> print(name)\n\
'IAC Driver Bus 1'\n\n"


/****** Libsndfile utilities ******/
static int
libsndfile_get_format(int fileformat, int sampletype) {
    int format = 0;
    switch (fileformat) {
        case 0:
            format = SF_FORMAT_WAV;
            break;
        case 1:
            format = SF_FORMAT_AIFF;
            break;
        case 2:
            format = SF_FORMAT_AU;
            break;
        case 3:
            format = SF_FORMAT_RAW;
            break;
        case 4:
            format = SF_FORMAT_SD2;
            break;
        case 5:
            format = SF_FORMAT_FLAC;
            break;
        case 6:
            format = SF_FORMAT_CAF;
            break;
        case 7:
            format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
            break;
    }
    if (fileformat != 7) {
        switch (sampletype) {
            case 0:
                format = format | SF_FORMAT_PCM_16;
                break;
            case 1:
                format = format | SF_FORMAT_PCM_24;
                break;
            case 2:
                format = format | SF_FORMAT_PCM_32;
                break;
            case 3:
                format = format | SF_FORMAT_FLOAT;
                break;
            case 4:
                format = format | SF_FORMAT_DOUBLE;
                break;
            case 5:
                format = format | SF_FORMAT_ULAW;
                break;
            case 6:
                format = format | SF_FORMAT_ALAW;
                break;
        }
    }
    return format;
}

static PyObject *
p_sndinfo(PyObject *self, PyObject *args, PyObject *kwds) {
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    char fileformat[5];
    char sampletype[16];
    int format, subformat, print = 0;
    Py_ssize_t psize;

    static char *kwlist[] = {"path", "print", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#|i", kwlist, &path, &psize, &print)) {
        PySys_WriteStderr("Pyo error: sndinfo called with wrong arguments.\n");
        Py_RETURN_NONE;
    }

    /* Open the sound file. */
    info.format = 0;
    sf = sf_open(path, SFM_READ, &info);
    if (sf == NULL) {
        Py_RETURN_NONE;
    }

    /* Retrieve file format */
    format = (int)info.format & SF_FORMAT_TYPEMASK;
    subformat = (int)info.format & SF_FORMAT_SUBMASK;
    if (format == SF_FORMAT_WAV)
        strcpy(fileformat, "WAVE");
    else if (format == SF_FORMAT_AIFF)
        strcpy(fileformat, "AIFF");
    else if (format == SF_FORMAT_AU)
        strcpy(fileformat, "AU");
    else if (format == SF_FORMAT_RAW)
        strcpy(fileformat, "RAW");
    else if (format == SF_FORMAT_SD2)
        strcpy(fileformat, "SD2");
    else if (format == SF_FORMAT_FLAC)
        strcpy(fileformat, "FLAC");
    else if (format == SF_FORMAT_CAF)
        strcpy(fileformat, "CAF");
    else if (format == SF_FORMAT_OGG)
        strcpy(fileformat, "OGG");
    else if (format == SF_FORMAT_RF64)
        strcpy(fileformat, "RF64");
    else
        strcpy(fileformat, "????");

    /* Retrieve sample type */
    if (subformat == SF_FORMAT_PCM_S8)
        strcpy(sampletype, "s8 bit int");
    else if (subformat == SF_FORMAT_PCM_U8)
        strcpy(sampletype, "u8 bit int");
    else if (subformat == SF_FORMAT_PCM_16)
        strcpy(sampletype, "16 bit int");
    else if (subformat == SF_FORMAT_PCM_24)
        strcpy(sampletype, "24 bit int");
    else if (subformat == SF_FORMAT_PCM_32)
        strcpy(sampletype, "32 bit int");
    else if (subformat == SF_FORMAT_FLOAT)
        strcpy(sampletype, "32 bit float");
    else if (subformat == SF_FORMAT_DOUBLE)
        strcpy(sampletype, "64 bit float");
    else if (subformat == SF_FORMAT_ULAW)
        strcpy(sampletype, "U-Law encoded");
    else if (subformat == SF_FORMAT_ALAW)
        strcpy(sampletype, "A-Law encoded");
    else if (subformat == SF_FORMAT_VORBIS)
        strcpy(sampletype, "vorbis encoding");
    else
        strcpy(sampletype, "Unknown...");

    if (print)
        PySys_WriteStdout("name: %s\nnumber of frames: %i\nduration: %.4f sec\nsr: %.2f\nchannels: %i\nformat: %s\nsample type: %s\n",
                          path, (int)info.frames, ((float)info.frames / info.samplerate), (float)info.samplerate, (int)info.channels, 
                          fileformat, sampletype);
    PyObject *sndinfos = PyTuple_Pack(6, PyInt_FromLong(info.frames), PyFloat_FromDouble((float)info.frames / info.samplerate),
                                        PyFloat_FromDouble(info.samplerate), PyInt_FromLong(info.channels), 
                                        PyUnicode_FromString(fileformat), PyUnicode_FromString(sampletype));
    sf_close(sf);
    return sndinfos;
}

static PyObject *
p_savefile(PyObject *self, PyObject *args, PyObject *kwds) {
    int i, j, size;
    char *recpath;
    PyObject *samples;
    MYFLT *sampsarray;
    int sr = 44100;
    int channels = 1;
    int fileformat = 0;
    int sampletype = 0;
    double quality = 0.4;
    SNDFILE *recfile;
    SF_INFO recinfo;
    Py_ssize_t psize;
    static char *kwlist[] = {"samples", "path", "sr", "channels", "fileformat", "sampletype", "quality", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os#|iiiid", kwlist, &samples, &recpath, &psize, &sr, &channels, &fileformat, &sampletype, &quality))
        return PyInt_FromLong(-1);

    recinfo.samplerate = sr;
    recinfo.channels = channels;
    recinfo.format = libsndfile_get_format(fileformat, sampletype);

    if (channels == 1) {
        size = PyList_Size(samples);
        sampsarray = (MYFLT *)malloc(size * sizeof(MYFLT));
        for (i=0; i<size; i++) {
            sampsarray[i] = PyFloat_AsDouble(PyList_GET_ITEM(samples, i));
        }
    }
    else {
        if (PyList_Size(samples) != channels) {
            PySys_WriteStdout("Pyo error: savefile's samples list size and channels number must be the same!\n");
            return PyInt_FromLong(-1);
        }
        size = PyList_Size(PyList_GET_ITEM(samples, 0)) * channels;
        sampsarray = (MYFLT *)malloc(size * sizeof(MYFLT));
        for (i=0; i<(size/channels); i++) {
            for (j=0; j<channels; j++) {
                sampsarray[i*channels+j] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(samples, j), i));
            }
        }
    }
    if (! (recfile = sf_open(recpath, SFM_WRITE, &recinfo))) {
        PySys_WriteStdout("Pyo error: savefile failed to open output file %s.\n", recpath);
        return PyInt_FromLong(-1);
    }

    // Sets the encoding quality for FLAC and OGG compressed formats
    if (fileformat == 5 || fileformat == 7) {
        sf_command(recfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(double));
    }

    SF_WRITE(recfile, sampsarray, size);
    sf_close(recfile);
    free(sampsarray);

    Py_RETURN_NONE;
}

static PyObject *
p_savefileFromTable(PyObject *self, PyObject *args, PyObject *kwds) {
    int i, j, size;
    char *recpath;
    PyObject *table;
    PyObject *base_objs;
    PyObject *tablestreamlist;
    MYFLT *sampsarray;
    int sr = 44100;
    int channels = 1;
    int fileformat = 0;
    int sampletype = 0;
    double quality = 0.4;
    int count = 0;
    int num_items = 0;
    SNDFILE *recfile;
    SF_INFO recinfo;
    Py_ssize_t psize;
    static char *kwlist[] = {"table", "path", "fileformat", "sampletype", "quality", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os#|iid", kwlist, &table, &recpath, &psize, &fileformat, &sampletype, &quality))
        return PyInt_FromLong(-1);

    base_objs = PyObject_GetAttrString(table, "_base_objs");
    channels = PyList_Size(base_objs);
    tablestreamlist = PyList_New(channels);
    for (i=0; i<channels; i++) {
        PyList_SET_ITEM(tablestreamlist, i, PyObject_CallMethod(PyList_GetItem(base_objs, i), "getTableStream", NULL));
    }
    sr = (int)TableStream_getSamplingRate((PyObject *)PyList_GetItem(tablestreamlist, 0));
    size = TableStream_getSize((PyObject *)PyList_GetItem(tablestreamlist, 0));

    recinfo.samplerate = sr;
    recinfo.channels = channels;
    recinfo.format = libsndfile_get_format(fileformat, sampletype);

    if (! (recfile = sf_open(recpath, SFM_WRITE, &recinfo))) {
        PySys_WriteStdout("Pyo error: savefileFromTable failed to open output file %s.\n", recpath);
        Py_XDECREF(base_objs);
        Py_XDECREF(tablestreamlist);
        return PyInt_FromLong(-1);
    }

    // Sets the encoding quality for FLAC and OGG compressed formats
    if (fileformat == 5 || fileformat == 7) {
        sf_command(recfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(double));
    }

    if (channels == 1) {
        MYFLT *data;
        if (size < (sr * 60)) {
            data = TableStream_getData((PyObject *)PyList_GetItem(tablestreamlist, 0));
            sampsarray = (MYFLT *)malloc(size * sizeof(MYFLT));
            for (i=0; i<size; i++) {
                sampsarray[i] = data[i];
            }
            SF_WRITE(recfile, sampsarray, size);
        }
        else {
            data = TableStream_getData((PyObject *)PyList_GetItem(tablestreamlist, 0));
            num_items = sr * 30;
            sampsarray = (MYFLT *)malloc(num_items * sizeof(MYFLT));
            do {
                if ((size - count) < num_items)
                    num_items = size - count;
                for (i=0; i<num_items; i++) {
                    sampsarray[i] = data[count++];
                }
                SF_WRITE(recfile, sampsarray, num_items);
            } while (num_items == (sr * 30));
        }
    }
    else {
        MYFLT *data[channels];
        if (size < (sr * 60)) {
            for (j=0; j<channels; j++) {
                data[j] = TableStream_getData((PyObject *)PyList_GetItem(tablestreamlist, j));
            }
            sampsarray = (MYFLT *)malloc(size * channels * sizeof(MYFLT));
            for (i=0; i<size; i++) {
                for (j=0; j<channels; j++) {
                    sampsarray[i*channels+j] = data[j][i];
                }
            }
            SF_WRITE(recfile, sampsarray, size * channels);
        }
        else {
            for (j=0; j<channels; j++) {
                data[j] = TableStream_getData((PyObject *)PyList_GetItem(tablestreamlist, j));
            }
            num_items = sr * 30;
            sampsarray = (MYFLT *)malloc(num_items * channels * sizeof(MYFLT));
            do {
                if ((size - count) < num_items)
                    num_items = size - count;
                for (i=0; i<num_items; i++) {
                    for (j=0; j<channels; j++) {
                        sampsarray[i*channels+j] = data[j][count];
                    }
                    count++;
                }
                SF_WRITE(recfile, sampsarray, num_items * channels);
            } while (num_items == (sr * 30));
        }
    }

    sf_close(recfile);
    free(sampsarray);
    Py_XDECREF(base_objs);
    Py_XDECREF(tablestreamlist);

    Py_RETURN_NONE;
}

/****** Sampling rate conversions ******/

MYFLT HALF_BLACKMAN[513] = {5.999999848427251e-05, 6.0518785176100209e-05, 6.2141079979483038e-05, 6.4805892179720104e-05, 6.8557070335373282e-05, 7.335994450841099e-05, 7.9284000094048679e-05, 8.6251806351356208e-05, 9.4344803073909134e-05, 0.00010353395919082686, 0.0001138320003519766, 0.0001252776273759082, 0.00013784394832327962, 0.00015158756286837161, 0.00016646583389956504, 0.00018252100562676787, 0.00019978794443886727, 0.00021828405442647636, 0.00023800843337085098, 0.00025901006301864982, 0.0002812814200296998, 0.00030484798480756581, 0.00032972017652355134, 0.00035596732050180435, 0.00038358545862138271, 0.0004126313142478466, 0.00044307118514552712, 0.00047501336666755378, 0.00050844199722632766, 0.00054337596520781517, 0.00057988864136859775, 0.00061800965340808034, 0.00065775914117693901, 0.000699152413289994, 0.00074227934237569571, 0.00078715570271015167, 0.00083377416012808681, 0.00088227324886247516, 0.0009326221770606935, 0.00098489224910736084, 0.0010391034884378314, 0.0010953464079648256, 0.001153626712039113, 0.0012140328763052821, 0.0012765693245455623, 0.001341317780315876, 0.0014083425048738718, 0.0014776336029171944, 0.001549328095279634, 0.0016234172508120537, 0.0017000052612274885, 0.0017791179707273841, 0.0018608199898153543, 0.0019451823318377137, 0.0020322385244071484, 0.0021220885682851076, 0.0022147782146930695, 0.0023103870917111635, 0.0024089745711535215, 0.0025105655658990145, 0.0026152802165597677, 0.0027231767307966948, 0.0028343265876173973, 0.0029487889260053635, 0.0030666270758956671, 0.0031879479065537453, 0.0033128033392131329, 0.0034412886016070843, 0.0035734693519771099, 0.0037094042636454105, 0.0038491983432322741, 0.0039929361082613468, 0.0041406778618693352, 0.004292510449886322, 0.0044485158286988735, 0.004608803428709507, 0.0047734435647726059, 0.0049425391480326653, 0.005116121843457222, 0.0052943285554647446, 0.0054772454313933849, 0.0056649716570973396, 0.0058575910516083241, 0.0060551739297807217, 0.0062578483484685421, 0.0064656869508326054, 0.0066788033582270145, 0.0068972636945545673, 0.0071212123148143291, 0.0073507223278284073, 0.007585874292999506, 0.0078268209472298622, 0.0080736298114061356, 0.0083263935521245003, 0.0085852388292551041, 0.0088502718135714531, 0.0091215828433632851, 0.0093993041664361954, 0.0096835149452090263, 0.0099743194878101349, 0.010271874256432056, 0.010576239787042141, 0.010887577198445797, 0.01120593398809433, 0.01153149176388979, 0.011864298023283482, 0.012204526923596859, 0.012552268803119659, 0.012907638214528561, 0.013270745985209942, 0.013641729019582272, 0.014020670205354691, 0.014407743699848652, 0.014803030528128147, 0.015206646174192429, 0.015618747100234032, 0.016039434820413589, 0.0164688341319561, 0.01690707728266716, 0.017354268580675125, 0.017810540273785591, 0.018276045098900795, 0.018750874325633049, 0.019235162064433098, 0.01972905732691288, 0.020232660695910454, 0.020746102556586266, 0.021269544959068298, 0.021803082898259163, 0.022346852347254753, 0.022900991141796112, 0.023465657606720924, 0.024040926247835159, 0.024626968428492546, 0.025223886594176292, 0.025831848382949829, 0.026450937613844872, 0.02708134613931179, 0.027723187580704689, 0.02837657742202282, 0.029041649773716927, 0.029718579724431038, 0.030407454818487167, 0.03110840916633606, 0.03182162344455719, 0.032547183334827423, 0.033285260200500488, 0.034035947173833847, 0.034799445420503616, 0.035575807094573975, 0.036365248262882233, 0.037167854607105255, 0.037983741611242294, 0.038813117891550064, 0.039656046777963638, 0.040512733161449432, 0.041383236646652222, 0.042267743498086929, 0.043166369199752808, 0.044079229235649109, 0.045006513595581055, 0.045948274433612823, 0.046904727816581726, 0.047875978052616119, 0.048862140625715256, 0.049863360822200775, 0.050879742950201035, 0.051911454647779465, 0.052958611398935318, 0.054021358489990234, 0.055099856108427048, 0.056194130331277847, 0.057304393500089645, 0.0584307461977005, 0.059573329985141754, 0.060732249170541763, 0.061907690018415451, 0.063099689781665802, 0.064308419823646545, 0.065534010529518127, 0.066776573657989502, 0.068036213517189026, 0.069313108921051025, 0.070607319474220276, 0.071918979287147522, 0.073248207569122314, 0.074595145881175995, 0.075959883630275726, 0.07734256237745285, 0.078743241727352142, 0.080162093043327332, 0.08159918338060379, 0.083054669201374054, 0.084528610110282898, 0.086021184921264648, 0.087532415986061096, 0.089062459766864777, 0.090611375868320465, 0.092179328203201294, 0.093766368925571442, 0.095372647047042847, 0.096998192369937897, 0.098643146455287933, 0.10030759125947952, 0.10199161618947983, 0.10369531810283661, 0.10541882365942001, 0.10716214776039124, 0.10892540961503983, 0.11070869863033295, 0.11251209676265717, 0.11433566361665726, 0.11617954820394516, 0.11804373562335968, 0.11992833018302917, 0.12183342128992081, 0.12375906854867935, 0.12570534646511078, 0.12767235934734344, 0.12966008484363556, 0.13166864216327667, 0.13369807600975037, 0.13574843108654022, 0.13781978189945221, 0.13991223275661469, 0.14202572405338287, 0.14416038990020752, 0.14631621539592743, 0.14849328994750977, 0.15069162845611572, 0.15291133522987366, 0.15515235066413879, 0.15741473436355591, 0.15969853103160858, 0.1620037853717804, 0.16433051228523254, 0.16667875647544861, 0.16904847323894501, 0.17143970727920532, 0.17385250329971313, 0.17628686130046844, 0.17874275147914886, 0.18122029304504395, 0.18371935188770294, 0.18623997271060944, 0.18878217041492462, 0.1913459450006485, 0.19393126666545868, 0.19653819501399994, 0.19916661083698273, 0.20181652903556824, 0.20448794960975647, 0.20718084275722504, 0.20989517867565155, 0.2126309871673584, 0.21538813412189484, 0.21816661953926086, 0.2209663987159729, 0.22378745675086975, 0.22662979364395142, 0.22949324548244476, 0.23237781226634979, 0.23528343439102173, 0.23821006715297699, 0.24115763604640961, 0.24412614107131958, 0.24711540341377258, 0.25012537837028503, 0.25315603613853455, 0.25620725750923157, 0.25927898287773132, 0.26237118244171143, 0.26548364758491516, 0.26861634850502014, 0.27176916599273682, 0.27494201064109802, 0.2781347930431366, 0.28134745359420776, 0.28457978367805481, 0.28783169388771057, 0.29110309481620789, 0.29439383745193481, 0.29770383238792419, 0.30103299021720886, 0.30438104271888733, 0.30774796009063721, 0.31113356351852417, 0.31453773379325867, 0.31796032190322876, 0.3214012086391449, 0.32486018538475037, 0.32833707332611084, 0.33183175325393677, 0.33534407615661621, 0.33887386322021484, 0.34242099523544312, 0.34598517417907715, 0.34956631064414978, 0.35316416621208191, 0.35677862167358398, 0.3604094386100769, 0.36405652761459351, 0.36771953105926514, 0.37139829993247986, 0.37509268522262573, 0.37880244851112366, 0.38252738118171692, 0.38626736402511597, 0.39002197980880737, 0.39379113912582397, 0.39757457375526428, 0.40137210488319397, 0.40518343448638916, 0.40900847315788269, 0.41284680366516113, 0.41669824719429016, 0.42056256532669067, 0.42443951964378357, 0.42832884192466736, 0.4322303831577301, 0.43614372611045837, 0.44006863236427307, 0.44400492310523987, 0.4479522705078125, 0.45191043615341187, 0.45587921142578125, 0.45985805988311768, 0.46384698152542114, 0.46784573793411255, 0.47185373306274414, 0.47587084770202637, 0.47989678382873535, 0.48393124341964722, 0.48797392845153809, 0.49202454090118408, 0.49608278274536133, 0.50014835596084595, 0.50422090291976929, 0.50830012559890747, 0.5123857855796814, 0.51647758483886719, 0.52057504653930664, 0.52467787265777588, 0.5287858247756958, 0.53289848566055298, 0.53701561689376831, 0.54113680124282837, 0.54526180028915405, 0.54939013719558716, 0.55352163314819336, 0.55765581130981445, 0.56179243326187134, 0.56593120098114014, 0.57007157802581787, 0.57421320676803589, 0.57835590839385986, 0.58249920606613159, 0.58664274215698242, 0.59078621864318848, 0.59492921829223633, 0.5990714430809021, 0.60321247577667236, 0.60735195875167847, 0.61148953437805176, 0.61562496423721313, 0.61975759267807007, 0.62388718128204346, 0.62801331281661987, 0.63213574886322021, 0.6362539529800415, 0.64036762714385986, 0.64447635412216187, 0.64857983589172363, 0.65267753601074219, 0.65676921606063843, 0.66085445880889893, 0.6649329662322998, 0.66900408267974854, 0.67306756973266602, 0.67712306976318359, 0.68117010593414307, 0.68520838022232056, 0.68923747539520264, 0.69325697422027588, 0.69726645946502686, 0.70126563310623169, 0.70525401830673218, 0.70923143625259399, 0.71319711208343506, 0.71715086698532104, 0.72109222412109375, 0.7250208854675293, 0.72893643379211426, 0.73283845186233521, 0.73672652244567871, 0.74060028791427612, 0.74445939064025879, 0.74830329418182373, 0.75213176012039185, 0.75594443082809448, 0.75974071025848389, 0.76352030038833618, 0.76728278398513794, 0.77102780342102051, 0.77475500106811523, 0.77846395969390869, 0.78215426206588745, 0.78582549095153809, 0.78947734832763672, 0.79310941696166992, 0.79672133922576904, 0.80031275749206543, 0.80388307571411133, 0.80743205547332764, 0.8109593391418457, 0.81446456909179688, 0.81794726848602295, 0.82140713930130005, 0.82484376430511475, 0.82825678586959839, 0.83164584636688232, 0.8350105881690979, 0.83835059404373169, 0.84166562557220459, 0.84495508670806885, 0.84821879863739014, 0.85145628452301025, 0.8546673059463501, 0.85785144567489624, 0.86100828647613525, 0.86413758993148804, 0.86723899841308594, 0.87031209468841553, 0.87335652112960815, 0.87637203931808472, 0.87935841083526611, 0.88231492042541504, 0.88524156808853149, 0.88813787698745728, 0.89100354909896851, 0.89383822679519653, 0.89664167165756226, 0.89941352605819702, 0.90215343236923218, 0.90486115217208862, 0.90753632783889771, 0.91017866134643555, 0.91278791427612305, 0.91536372900009155, 0.91790568828582764, 0.92041373252868652, 0.92288732528686523, 0.92532640695571899, 0.92773056030273438, 0.93009954690933228, 0.93243312835693359, 0.93473094701766968, 0.93699288368225098, 0.93921846151351929, 0.94140768051147461, 0.94356006383895874, 0.94567543268203735, 0.94775348901748657, 0.94979411363601685, 0.95179694890975952, 0.95376187562942505, 0.95568859577178955, 0.95757681131362915, 0.95942646265029907, 0.96123719215393066, 0.9630088210105896, 0.96474123001098633, 0.96643412113189697, 0.96808725595474243, 0.96970051527023315, 0.97127372026443481, 0.97280663251876831, 0.97429907321929932, 0.97575092315673828, 0.9771619439125061, 0.97853195667266846, 0.97986090183258057, 0.98114854097366333, 0.98239481449127197, 0.98359936475753784, 0.98476219177246094, 0.98588317632675171, 0.98696213960647583, 0.98799896240234375, 0.98899352550506592, 0.98994570970535278, 0.99085539579391479, 0.9917224645614624, 0.99254685640335083, 0.99332839250564575, 0.99406707286834717, 0.99476277828216553, 0.99541538953781128, 0.99602478742599487, 0.99659103155136108, 0.99711394309997559, 0.99759352207183838, 0.99802964925765991, 0.99842232465744019, 0.9987715482711792, 0.99907714128494263, 0.99933922290802002, 0.99955761432647705, 0.9997323751449585, 0.99986344575881958, 0.9999508261680603, 0.99999451637268066, 0.99999451637268066};
/*
 gen_lp_impulse generates a sinc function to be used as a lowpass impulse response.
 array is the container where to save the impulse response function.
 size is the convolution impulse response length in samples.
 freq is the cutoff frequency in radians.
*/
static void
gen_lp_impulse(MYFLT *array, int size, float freq) {
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
 num_samps is the number of samples to filter.
 size is the filter order. Minimum suggested = 16, ideal = 128 or higher.
 gain is the gain of the filter.
*/
static void
lp_conv(MYFLT *samples, MYFLT *impulse, int num_samps, int size, int gain) {
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
p_upsamp(PyObject *self, PyObject *args, PyObject *kwds)
{
    unsigned int i, j, k;
    char *inpath;
    char *outpath;
    SNDFILE *sf;
    SF_INFO info;
    Py_ssize_t psize, psize2;
    unsigned int snd_size, snd_sr, snd_chnls, num_items;
    MYFLT *sincfunc;
    MYFLT *tmp;
    MYFLT **samples;
    MYFLT **upsamples;
    unsigned int up = 4;
    int order = 128;
    static char *kwlist[] = {"path", "outfile", "up", "order", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#s#|ii", kwlist, &inpath, &psize, &outpath, &psize2, &up, &order))
        return PyInt_FromLong(-1);

    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);
    if (sf == NULL) {
        PySys_WriteStdout("Pyo error: upsamp failed to open the input file %s.\n", inpath);
        return PyInt_FromLong(-1);
    }
    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT *));
    for(i=0; i<snd_chnls; i++)
        samples[i] = (MYFLT *)malloc(snd_size * sizeof(MYFLT));

    for (i=0; i<num_items; i++)
        samples[i%snd_chnls][(int)(i/snd_chnls)] = tmp[i];
    free(tmp);

    /* upsampling */
    upsamples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT *));
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
        PySys_WriteStdout("Pyo error: upsamp failed to open output file %s.\n", outpath);
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
p_downsamp(PyObject *self, PyObject *args, PyObject *kwds)
{
    unsigned int i, j;
    char *inpath;
    char *outpath;
    SNDFILE *sf;
    SF_INFO info;
    Py_ssize_t psize, psize2;
    unsigned int snd_size, snd_sr, snd_chnls, num_items, samples_per_channels;
    MYFLT *sincfunc;
    MYFLT *tmp;
    MYFLT **samples;
    MYFLT **downsamples;
    int down = 4;
    int order = 128;
    static char *kwlist[] = {"path", "outfile", "down", "order", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#s#|ii", kwlist, &inpath, &psize, &outpath, &psize2, &down, &order))
        return PyInt_FromLong(-1);

    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);
    if (sf == NULL) {
        PySys_WriteStdout("Pyo error: downsamp failed to open the input file %s.\n", inpath);
        return PyInt_FromLong(-1);
    }
    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)malloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT *));
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
    samples_per_channels = (snd_size / down) + (snd_size % down);
    downsamples = (MYFLT **)malloc(snd_chnls * sizeof(MYFLT *));
    for(i=0; i<snd_chnls; i++) {
        downsamples[i] = (MYFLT *)malloc(samples_per_channels * sizeof(MYFLT));
        for (j=0; j<samples_per_channels; j++) {
            downsamples[i][j] = 0.0;
        }
    }

    for (i=0; i<samples_per_channels; i++) {
        for (j=0; j<snd_chnls; j++) {
            if (i*down < snd_size)
                downsamples[j][i] = samples[j][i*down];
            else
                downsamples[j][i] = 0.0;
        }
    }

    /* save downsampled file */
    info.samplerate = snd_sr / down;
    tmp = (MYFLT *)malloc(snd_chnls * samples_per_channels * sizeof(MYFLT));
    for (i=0; i<samples_per_channels; i++) {
        for (j=0; j<snd_chnls; j++) {
            tmp[i*snd_chnls+j] = downsamples[j][i];
        }
    }

    if (! (sf = sf_open(outpath, SFM_WRITE, &info))) {
        PySys_WriteStdout("Pyo error: downsamp failed to open the output file %s.\n", outpath);
        free(tmp);
        for (i=0; i<snd_chnls; i++) {
            free(samples[i]);
            free(downsamples[i]);
        }
        free(samples);
        free(downsamples);
        return PyInt_FromLong(-1);
    }

    SF_WRITE(sf, tmp, snd_chnls * samples_per_channels);
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
This function receives a list of points as input and returns a simplified list by\neliminating redundancies.\n\n\
A point is a tuple (or a list) of two floats, time and value. A list of points looks like:\n\n        \
[ (0, 0), (0.1, 0.7), (0.2, 0.5), ... ] \n\n\
:Args:\n\n    \
pointlist: list of lists or list of tuples\n        List of points (time, value) to filter.\n    \
tolerance: float, optional\n        Normalized distance threshold under which a point is\n        excluded from the list. Defaults to 0.02."

typedef struct STACK_RECORD {
    int nAnchorIndex;
    int nFloaterIndex;
    struct STACK_RECORD *precPrev;
} STACK_RECORD;

STACK_RECORD *m_pStack = NULL;

static void StackPush( int nAnchorIndex, int nFloaterIndex )
{
    STACK_RECORD *precPrev = m_pStack;
    m_pStack = (STACK_RECORD *)malloc( sizeof(STACK_RECORD) );
    m_pStack->nAnchorIndex = nAnchorIndex;
    m_pStack->nFloaterIndex = nFloaterIndex;
    m_pStack->precPrev = precPrev;
}

static int StackPop( int *pnAnchorIndex, int *pnFloaterIndex )
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
        MYFLT xMax, yMin, yMax;

        static char *kwlist[] = {"pointlist", "tolerance", NULL};

        if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_F, kwlist, &pointlist, &dTolerance))
            return PyInt_FromLong(-1);

        nPointsCount = PyList_Size(pointlist);

	// TODO: these malloc's are never freed.
        pPointsX = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
        pPointsY = (MYFLT *)malloc(nPointsCount * sizeof(MYFLT));
        pnUseFlag = (int *)malloc(nPointsCount * sizeof(int));

        tup = PyList_GET_ITEM(pointlist, 0);
        if (PyTuple_Check(tup) == 1) {
            for (i=0; i<nPointsCount; i++) {
                tup = PyList_GET_ITEM(pointlist, i);
                pPointsX[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
                pPointsY[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
                pnUseFlag[i] = 0;
            }
        }
        else {
            for (i=0; i<nPointsCount; i++) {
                tup = PyList_GET_ITEM(pointlist, i);
                pPointsX[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 0));
                pPointsY[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 1));
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
This function returns the shortest distance from a point to a line segment\nnormalized between 0 and 1.\n\n\
A point is a tuple (or a list) of two floats, time and value. `p` is the point for which\nto find the distance from line `p1` to `p2`.\n\n\
:Args:\n\n    \
p: list or tuple\n        Point for which to find the distance.\n    \
p1: list or tuple\n        First point of the segment.\n    \
p2: list or tuple\n        Second point of the segment.\n    \
xmin: float, optional\n        Minimum value on the X axis.\n    \
xmax: float, optional\n        Maximum value on the X axis.\n    \
ymin: float, optional\n        Minimum value on the Y axis.\n    \
ymax: float, optional\n        Maximum value on the Y axis.\n    \
xlog: boolean, optional\n        Set this argument to True if X axis has a logarithmic scaling.\n    \
ylog: boolean, optional\n        Set this argument to True if Y axis has a logarithmic scaling."

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
        xp[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xscale;
        xp1[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xscale;
        xp2[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xscale;
    }
    else {
        xscale = MYLOG10(xmax / xmin);
        xp[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xmin) / xscale;
        xp1[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xmin) / xscale;
        xp2[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xmin) / xscale;
    }
    if (ylog == 0) {
        yscale = ymax - ymin;
        xp[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / yscale;
        xp1[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / yscale;
        xp2[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / yscale;
    }
    else {
        yscale = MYLOG10(ymax / ymin);
        xp[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / ymin) / yscale;
        xp1[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / ymin) / yscale;
        xp2[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / ymin) / yscale;
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
A point is a tuple (or a list) of two floats, time and value.\n\n:Args:\n\n    \
data: list of points\n        Set of points between which will be inserted interpolated segments.\n    \
yrange: list of 2 floats, optional\n        Minimum and maximum values on the Y axis. Defaults to [0., 1.].\n    \
totaldur: float, optional\n        X axis duration. Defaults to 1.\n    \
points: int, optional\n        Number of points in the output list. Defaults to 1024.\n    \
log: boolean, optional\n        Set this value to True if the Y axis has a logarithmic scale. Defaults to False\n\n\
>>> s = Server().boot()\n\
>>> a = [(0,0), (0.25, 1), (0.33, 1), (1,0)]\n\
>>> b = linToCosCurve(a, yrange=[0, 1], totaldur=1, points=8192)\n\
>>> t = DataTable(size=len(b), init=[x[1] for x in b])\n\
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
        ymin = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 0));
        ymax = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 1));
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
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
            ydata[i] = (tmp - ymin) / ydiff;
        }
    }
    else {
        for (i=0; i<datasize; i++) {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
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
This function takes data in the range `xmin` - `xmax` and returns corresponding values\nin the range `ymin` - `ymax`.\n\n\
`data` can be either a number or a list. Return value is of the same type as `data`\nwith all values rescaled.\n\n\
:Argss:\n\n    \
data: float or list of floats\n        Values to convert.\n    \
xmin: float, optional\n        Minimum value of the input range.\n    \
xmax: float, optional\n        Maximum value of the input range.\n    \
ymin: float, optional\n        Minimum value of the output range.\n    \
ymax: float, optional\n        Maximum value of the output range.\n    \
xlog: boolean, optional\n        Set this argument to True if the input range has a logarithmic scaling.\n    \
ylog: boolean, optional\n        Set this argument to True if the output range has a logarithmic scaling.\n\n\
>>> a = 0.5\n\
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n\
>>> print(b)\n\
632.453369141\n\
>>> a = [0, .4, .8]\n\
>>> b = rescale(a, 0, 1, 20, 20000, False, True)\n\
>>> print(b)\n\
[20.000001907348633, 316.97738647460938, 5023.7705078125]\n\n"

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
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
    if (xlog == 0 && ylog == 0) {
        datascl = xmax - xmin;
        curscl = ymax - ymin;
        curscl /= datascl;
        if (type == 0) {
            val = PyFloat_AsDouble(data);
            return Py_BuildValue("d", (val - xmin) * curscl + ymin);
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
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
            val = PyFloat_AsDouble(data);
            if (val == 0)
                val = 0.000001;
            val = (val - xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
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
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", val * curscl + ymin);
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
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
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1) {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);
            for (i=0; i<cnt; i++) {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }
            return out;
        }
    }
    else {
        Py_INCREF(Py_None);
        return Py_None;
    }
	Py_INCREF(Py_None);
	return Py_None;
}

#define floatmap_info \
"\nConverts values from a 0-1 range to an output range.\n\n\
This function takes data in the range `0` - `1` and returns corresponding values\nin the range `min` - `max`.\n\n\
:Argss:\n\n    \
x: float\n        Value to convert, in the range 0 to 1.\n    \
min: float, optional\n        Minimum value of the output range. Defaults to 0.\n    \
max: float, optional\n        Maximum value of the output range. Defaults to 1.\n    \
exp: float, optional\n        Power factor (1 (default) is linear, les than 1 is logarithmic, greter than 1 is exponential).\n\n\
>>> a = 0.5\n\
>>> b = floatmap(a, 0, 1, 4)\n\
>>> print(b)\n\
0.0625\n\n"

static PyObject *
floatmap(PyObject *self, PyObject *args, PyObject *kwds)
{
    MYFLT x = 0.0;
    MYFLT min = 0.0;
    MYFLT max = 1.0;
    MYFLT exp = 1.0;

    static char *kwlist[] = {"x", "min", "max", "exp", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_FFF, kwlist, &x, &min, &max, &exp))
        return PyInt_FromLong(-1);

    if (x < 0.0)
        x = 0.0;
    else if (x > 1.0)
        x = 1.0;
    if (exp != 1.0)
        x = MYPOW(x, exp);

    return Py_BuildValue("d", x * (max - min) + min);
}

/****** Conversion utilities ******/
#define midiToHz_info \
"\nConverts a midi note value to frequency in Hertz.\n\n:Args:\n\n    \
x: int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (48, 60, 62, 67)\n\
>>> b = midiToHz(a)\n\
>>> print(b)\n\
(130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656)\n\
>>> a = [48, 60, 62, 67]\n\
>>> b = midiToHz(a)\n\
>>> print(b)\n\
[130.8127826503271, 261.62556530066814, 293.66476791748823, 391.9954359818656]\n\
>>> b = midiToHz(60.0)\n\
>>> print(b)\n\
261.625565301\n\n"

static PyObject *
midiToHz(PyObject *self, PyObject *arg) {
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 440.0 * MYPOW(2.0, (PyFloat_AsDouble(arg) - 69) / 12.0));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(440.0 * MYPOW(2.0, (x - 69) / 12.0)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(440.0 * MYPOW(2.0, (x - 69) / 12.0)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define hzToMidi_info \
"\nConverts a frequency in Hertz to a midi note value.\n\n:Args:\n\n    \
x: float\n        Frequency in Hertz. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (110.0, 220.0, 440.0, 880.0)\n\
>>> b = hzToMidi(a)\n\
>>> print(b)\n\
(45.0, 57.0, 69.0, 81.0)\n\
>>> a = [110.0, 220.0, 440.0, 880.0]\n\
>>> b = hzToMidi(a)\n\
>>> print(b)\n\
[45.0, 57.0, 69.0, 81.0]\n\
>>> b = hzToMidi(440.0)\n\
>>> print(b)\n\
69.0\n\n"

static PyObject *
hzToMidi(PyObject *self, PyObject *arg) {
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 12.0 * MYLOG2(PyFloat_AsDouble(arg) / 440.0) + 69);
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(12.0 * MYLOG2(x / 440.0) + 69));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(12.0 * MYLOG2(x / 440.0) + 69));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define midiToTranspo_info \
"\nConverts a midi note value to transposition factor (central key = 60).\n\n:Args:\n\n    \
x: int or float\n        Midi note. `x` can be a number, a list or a tuple, otherwise the function returns None.\n\n\
>>> a = (48, 60, 62, 67)\n\
>>> b = midiToTranspo(a)\n\
>>> print(b)\n    \
(0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281)\n\
>>> a = [48, 60, 62, 67]\n\
>>> b = midiToTranspo(a)\n\
>>> print(b)\n\
[0.49999999999997335, 1.0, 1.122462048309383, 1.4983070768767281]\n\
>>> b = midiToTranspo(60.0)\n\
>>> print(b)\n\
1.0\n\n"

static PyObject *
midiToTranspo(PyObject *self, PyObject *arg) {
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", pow(1.0594630943593, PyFloat_AsDouble(arg)-60.0));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x-60.0)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x-60.0)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define sampsToSec_info \
"\nReturns the duration in seconds equivalent to the number of samples given as an argument.\n\n:Args:\n\n    \
x: int or float\n        Duration in samples. `x` can be a number, a list or a tuple, otherwise function returns None.\n\n\
>>> s = Server().boot()\n\
>>> a = (64, 128, 256)\n\
>>> b = sampsToSec(a)\n\
>>> print(b)\n\
(0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194)\n\
>>> a = [64, 128, 256]\n\
>>> b = sampsToSec(a)\n\
>>> print(b)\n\
[0.0014512471655328798, 0.0029024943310657597, 0.0058049886621315194]\n\
>>> b = sampsToSec(8192)\n\
>>> print(b)\n\
0.185759637188\n\n"

static PyObject *
sampsToSec(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    if (server == NULL) {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `sampsToSec` function.\n");
        Py_RETURN_NONE;
    }
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("d", PyFloat_AsDouble(arg) / sr);
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}

#define secToSamps_info \
"\nReturns the number of samples equivalent to the duration in seconds given as an argument.\n\n:Args:\n\n    \
x: int or float\n        Duration in seconds. `x` can be a number, a list or a tuple, otherwise function returns None.\n\n\
>>> s = Server().boot()\n\
>>> a = (0.1, 0.25, 0.5, 1)\n\
>>> b = secToSamps(a)\n\
>>> print(b)\n\
(4410, 11025, 22050, 44100)\n\
>>> a = [0.1, 0.25, 0.5, 1]\n\
>>> b = secToSamps(a)\n\
>>> print(b)\n\
[4410, 11025, 22050, 44100]\n\
>>> b = secToSamps(2.5)\n\
>>> print(b)\n\
110250\n\n"

static PyObject *
secToSamps(PyObject *self, PyObject *arg) {
    PyObject *server = PyServer_get_server();
    if (server == NULL) {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `secToSamps` function.\n");
        Py_RETURN_NONE;
    }
    double sr = PyFloat_AsDouble(PyObject_CallMethod(server, "getSamplingRate", NULL));
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;
    if (PyNumber_Check(arg))
        return Py_BuildValue("l", (long)(PyFloat_AsDouble(arg) * sr));
    else if (PyList_Check(arg)) {
        count = PyList_Size(arg);
        newseq = PyList_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyInt_FromLong((long)(x * sr)));
        }
        return newseq;
    }
    else if (PyTuple_Check(arg)) {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);
        for (i=0; i<count; i++) {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyInt_FromLong((long)(x * sr)));
        }
        return newseq;
    }
    else
        Py_RETURN_NONE;
}

/************* Server quieries *************/
#define serverCreated_info \
"\nReturns True if a Server object is already created, otherwise, returns False.\n\n\
>>> print(serverCreated())\n\
False\n\
>>> s = Server()\n\
>>> print(serverCreated())\n\
True\n\n"

static PyObject *
serverCreated(PyObject *self) {
    if (PyServer_get_server() != NULL) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

#define serverBooted_info \
"\nReturns True if an already created Server is booted, otherwise, returns False.\n\n\
>>> s = Server()\n\
>>> print(serverBooted())\n\
False\n\
>>> s.boot()\n\
>>> print(serverBooted())\n\
True\n\n"

static PyObject *
serverBooted(PyObject *self) {
    int boot;
    PyObject *server;
    if (PyServer_get_server() != NULL) {
        server = PyServer_get_server();
        boot = PyInt_AsLong(PyObject_CallMethod(server, "getIsBooted", NULL));
        if (boot == 0) {
            Py_INCREF(Py_False);
            return Py_False;
        }
        else {
            Py_INCREF(Py_True);
            return Py_True;
        }
    }
    else {
        PySys_WriteStdout("Pyo Warning: A Server must be created before calling `serverBooted` function.\n");
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef pyo_functions[] = {
{"pa_get_version", (PyCFunction)portaudio_get_version, METH_NOARGS, portaudio_get_version_info},
{"pa_get_version_text", (PyCFunction)portaudio_get_version_text, METH_NOARGS, portaudio_get_version_text_info},
{"pa_count_devices", (PyCFunction)portaudio_count_devices, METH_NOARGS, portaudio_count_devices_info},
{"pa_count_host_apis", (PyCFunction)portaudio_count_host_apis, METH_NOARGS, portaudio_count_host_apis_info},
{"pa_list_devices", (PyCFunction)portaudio_list_devices, METH_NOARGS, portaudio_list_devices_info},
{"pa_get_devices_infos", (PyCFunction)portaudio_get_devices_infos, METH_NOARGS, portaudio_get_devices_infos_info},
{"pa_get_input_max_channels", (PyCFunction)portaudio_get_input_max_channels, METH_O, portaudio_get_input_max_channels_info},
{"pa_get_output_max_channels", (PyCFunction)portaudio_get_output_max_channels, METH_O, portaudio_get_output_max_channels_info},
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
{"p_sndinfo", (PyCFunction)p_sndinfo, METH_VARARGS|METH_KEYWORDS, ""},
{"p_savefile", (PyCFunction)p_savefile, METH_VARARGS|METH_KEYWORDS, ""},
{"p_savefileFromTable", (PyCFunction)p_savefileFromTable, METH_VARARGS|METH_KEYWORDS, ""},
{"p_upsamp", (PyCFunction)p_upsamp, METH_VARARGS|METH_KEYWORDS, ""},
{"p_downsamp", (PyCFunction)p_downsamp, METH_VARARGS|METH_KEYWORDS, ""},
{"reducePoints", (PyCFunction)reducePoints, METH_VARARGS|METH_KEYWORDS, reducePoints_info},
{"distanceToSegment", (PyCFunction)distanceToSegment, METH_VARARGS|METH_KEYWORDS, distanceToSegment_info},
{"rescale", (PyCFunction)rescale, METH_VARARGS|METH_KEYWORDS, rescale_info},
{"floatmap", (PyCFunction)floatmap, METH_VARARGS|METH_KEYWORDS, floatmap_info},
{"linToCosCurve", (PyCFunction)linToCosCurve, METH_VARARGS|METH_KEYWORDS, linToCosCurve_info},
{"midiToHz", (PyCFunction)midiToHz, METH_O, midiToHz_info},
{"hzToMidi", (PyCFunction)hzToMidi, METH_O, hzToMidi_info},
{"midiToTranspo", (PyCFunction)midiToTranspo, METH_O, midiToTranspo_info},
{"sampsToSec", (PyCFunction)sampsToSec, METH_O, sampsToSec_info},
{"secToSamps", (PyCFunction)secToSamps, METH_O, secToSamps_info},
{"serverCreated", (PyCFunction)serverCreated, METH_NOARGS, serverCreated_info},
{"serverBooted", (PyCFunction)serverBooted, METH_NOARGS, serverBooted_info},
{"withPortaudio", (PyCFunction)with_portaudio, METH_NOARGS, "Returns True if pyo is built with portaudio support."},
{"withPortmidi", (PyCFunction)with_portmidi, METH_NOARGS, "Returns True if pyo is built with portmidi support."},
{"withJack", (PyCFunction)with_jack, METH_NOARGS, "Returns True if pyo is built with jack support."},
{"withCoreaudio", (PyCFunction)with_coreaudio, METH_NOARGS, "Returns True if pyo is built with coreaudio support."},
{"withOSC", (PyCFunction)with_osc, METH_NOARGS, "Returns True if pyo is built with OSC (Open Sound Control) support."},
{NULL, NULL, 0, NULL},
};

#if PY_MAJOR_VERSION >= 3
// TODO: Pyo likely has a bunch of state stored in global variables right now, they should ideally be stored
// in an interpreter specific struct as described in https://docs.python.org/3/howto/cporting.html
static struct PyModuleDef pyo_moduledef = {
    PyModuleDef_HEAD_INIT,
    LIB_BASE_NAME,/* m_name */
    "Python digital signal processing module.",/* m_doc */
    0,/* m_size */
    pyo_functions,/* m_methods */
    NULL,/* m_reload */
    NULL,/* m_traverse */
    NULL,/* m_clear */
    NULL,/* m_free */
};
#endif

static PyObject *
module_add_object(PyObject *module, const char *name, PyTypeObject *type) {
    if (PyType_Ready(type) < 0)
        Py_RETURN_NONE;
    Py_INCREF(type);
    PyModule_AddObject(module, name, (PyObject *)type);
    Py_RETURN_NONE;
}

PyMODINIT_FUNC
#if PY_MAJOR_VERSION >= 3
#ifndef USE_DOUBLE
PyInit__pyo(void)
#else
PyInit__pyo64(void)
#endif
#else
#ifndef USE_DOUBLE
init_pyo(void)
#else
init_pyo64(void)
#endif
#endif

{
    PyObject *m;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&pyo_moduledef);
#else
    m = Py_InitModule3(LIB_BASE_NAME, pyo_functions, "Python digital signal processing module.");
#endif

    module_add_object(m, "Server_base", &ServerType);
#ifdef USE_PORTMIDI
    module_add_object(m, "MidiListener_base", &MidiListenerType);
    module_add_object(m, "MidiDispatcher_base", &MidiDispatcherType);
#endif
#ifdef USE_OSC
    module_add_object(m, "OscListener_base", &OscListenerType);
    module_add_object(m, "OscSend_base", &OscSendType);
    module_add_object(m, "OscDataSend_base", &OscDataSendType);
    module_add_object(m, "OscReceive_base", &OscReceiveType);
    module_add_object(m, "OscReceiver_base", &OscReceiverType);
    module_add_object(m, "OscListReceive_base", &OscListReceiveType);
    module_add_object(m, "OscListReceiver_base", &OscListReceiverType);
    module_add_object(m, "OscDataReceive_base", &OscDataReceiveType);
#endif
    module_add_object(m, "Stream", &StreamType);
    module_add_object(m, "TriggerStream", &TriggerStreamType);
    module_add_object(m, "PVStream", &PVStreamType);
    module_add_object(m, "Dummy_base", &DummyType);
    module_add_object(m, "TriggerDummy_base", &TriggerDummyType);
    module_add_object(m, "TableStream", &TableStreamType);
    module_add_object(m, "MatrixStream", &MatrixStreamType);
    module_add_object(m, "Record_base", &RecordType);
    module_add_object(m, "ControlRec_base", &ControlRecType);
    module_add_object(m, "ControlRead_base", &ControlReadType);
    module_add_object(m, "NoteinRec_base", &NoteinRecType);
    module_add_object(m, "NoteinRead_base", &NoteinReadType);
    module_add_object(m, "Compare_base", &CompareType);
    module_add_object(m, "Mix_base", &MixType);
    module_add_object(m, "Sig_base", &SigType);
    module_add_object(m, "SigTo_base", &SigToType);
    module_add_object(m, "VarPort_base", &VarPortType);
    module_add_object(m, "InputFader_base", &InputFaderType);
    module_add_object(m, "Adsr_base", &AdsrType);
    module_add_object(m, "Linseg_base", &LinsegType);
    module_add_object(m, "Expseg_base", &ExpsegType);
    module_add_object(m, "HarmTable_base", &HarmTableType);
    module_add_object(m, "ChebyTable_base", &ChebyTableType);
    module_add_object(m, "HannTable_base", &HannTableType);
    module_add_object(m, "SincTable_base", &SincTableType);
    module_add_object(m, "WinTable_base", &WinTableType);
    module_add_object(m, "ParaTable_base", &ParaTableType);
    module_add_object(m, "LinTable_base", &LinTableType);
    module_add_object(m, "LogTable_base", &LogTableType);
    module_add_object(m, "CosLogTable_base", &CosLogTableType);
    module_add_object(m, "CosTable_base", &CosTableType);
    module_add_object(m, "CurveTable_base", &CurveTableType);
    module_add_object(m, "ExpTable_base", &ExpTableType);
    module_add_object(m, "SndTable_base", &SndTableType);
    module_add_object(m, "DataTable_base", &DataTableType);
    module_add_object(m, "NewTable_base", &NewTableType);
    module_add_object(m, "TableRec_base", &TableRecType);
    module_add_object(m, "TableRecTimeStream_base", &TableRecTimeStreamType);
    module_add_object(m, "TableMorph_base", &TableMorphType);
    module_add_object(m, "TrigTableRec_base", &TrigTableRecType);
    module_add_object(m, "TrigTableRecTimeStream_base", &TrigTableRecTimeStreamType);
    module_add_object(m, "TableWrite_base", &TableWriteType);
    module_add_object(m, "TablePut_base", &TablePutType);
    module_add_object(m, "NewMatrix_base", &NewMatrixType);
    module_add_object(m, "MatrixPointer_base", &MatrixPointerType);
    module_add_object(m, "MatrixRec_base", &MatrixRecType);
    module_add_object(m, "MatrixRecLoop_base", &MatrixRecLoopType);
    module_add_object(m, "MatrixMorph_base", &MatrixMorphType);
    module_add_object(m, "Input_base", &InputType);
    module_add_object(m, "Trig_base", &TrigType);
    module_add_object(m, "NextTrig_base", &NextTrigType);
    module_add_object(m, "Metro_base", &MetroType);
    module_add_object(m, "Seqer_base", &SeqerType);
    module_add_object(m, "Seq_base", &SeqType);
    module_add_object(m, "Clouder_base", &ClouderType);
    module_add_object(m, "Cloud_base", &CloudType);
    module_add_object(m, "Beater_base", &BeaterType);
    module_add_object(m, "Beat_base", &BeatType);
    module_add_object(m, "BeatTapStream_base", &BeatTapStreamType);
    module_add_object(m, "BeatAmpStream_base", &BeatAmpStreamType);
    module_add_object(m, "BeatDurStream_base", &BeatDurStreamType);
    module_add_object(m, "BeatEndStream_base", &BeatEndStreamType);
    module_add_object(m, "Fader_base", &FaderType);
    module_add_object(m, "Randi_base", &RandiType);
    module_add_object(m, "Randh_base", &RandhType);
    module_add_object(m, "Choice_base", &ChoiceType);
    module_add_object(m, "RandDur_base", &RandDurType);
    module_add_object(m, "Xnoise_base", &XnoiseType);
    module_add_object(m, "XnoiseMidi_base", &XnoiseMidiType);
    module_add_object(m, "XnoiseDur_base", &XnoiseDurType);
    module_add_object(m, "RandInt_base", &RandIntType);
    module_add_object(m, "Urn_base", &UrnType);
    module_add_object(m, "SfPlayer_base", &SfPlayerType);
    module_add_object(m, "SfPlay_base", &SfPlayType);
    module_add_object(m, "SfMarkerShuffler_base", &SfMarkerShufflerType);
    module_add_object(m, "SfMarkerShuffle_base", &SfMarkerShuffleType);
    module_add_object(m, "SfMarkerLooper_base", &SfMarkerLooperType);
    module_add_object(m, "SfMarkerLoop_base", &SfMarkerLoopType);
    module_add_object(m, "Osc_base", &OscType);
    module_add_object(m, "OscLoop_base", &OscLoopType);
    module_add_object(m, "OscTrig_base", &OscTrigType);
    module_add_object(m, "OscBank_base", &OscBankType);
    module_add_object(m, "SumOsc_base", &SumOscType);
    module_add_object(m, "TableRead_base", &TableReadType);
    module_add_object(m, "Pulsar_base", &PulsarType);
    module_add_object(m, "Sine_base", &SineType);
    module_add_object(m, "FastSine_base", &FastSineType);
    module_add_object(m, "SineLoop_base", &SineLoopType);
    module_add_object(m, "Fm_base", &FmType);
    module_add_object(m, "CrossFm_base", &CrossFmType);
    module_add_object(m, "LFO_base", &LFOType);
    module_add_object(m, "Blit_base", &BlitType);
    module_add_object(m, "Rossler_base", &RosslerType);
    module_add_object(m, "RosslerAlt_base", &RosslerAltType);
    module_add_object(m, "Lorenz_base", &LorenzType);
    module_add_object(m, "LorenzAlt_base", &LorenzAltType);
    module_add_object(m, "ChenLee_base", &ChenLeeType);
    module_add_object(m, "ChenLeeAlt_base", &ChenLeeAltType);
    module_add_object(m, "Phasor_base", &PhasorType);
    module_add_object(m, "SuperSaw_base", &SuperSawType);
    module_add_object(m, "Pointer_base", &PointerType);
    module_add_object(m, "TableIndex_base", &TableIndexType);
    module_add_object(m, "Lookup_base", &LookupType);
    module_add_object(m, "Noise_base", &NoiseType);
    module_add_object(m, "PinkNoise_base", &PinkNoiseType);
    module_add_object(m, "BrownNoise_base", &BrownNoiseType);
    module_add_object(m, "Biquad_base", &BiquadType);
    module_add_object(m, "Biquadx_base", &BiquadxType);
    module_add_object(m, "Biquada_base", &BiquadaType);
    module_add_object(m, "EQ_base", &EQType);
    module_add_object(m, "Tone_base", &ToneType);
    module_add_object(m, "Atone_base", &AtoneType);
    module_add_object(m, "DCBlock_base", &DCBlockType);
    module_add_object(m, "Allpass_base", &AllpassType);
    module_add_object(m, "Allpass2_base", &Allpass2Type);
    module_add_object(m, "Phaser_base", &PhaserType);
    module_add_object(m, "Vocoder_base", &VocoderType);
    module_add_object(m, "Port_base", &PortType);
    module_add_object(m, "Denorm_base", &DenormType);
    module_add_object(m, "Disto_base", &DistoType);
    module_add_object(m, "Clip_base", &ClipType);
    module_add_object(m, "Mirror_base", &MirrorType);
    module_add_object(m, "Wrap_base", &WrapType);
    module_add_object(m, "Between_base", &BetweenType);
    module_add_object(m, "Degrade_base", &DegradeType);
    module_add_object(m, "Compress_base", &CompressType);
    module_add_object(m, "Gate_base", &GateType);
    module_add_object(m, "Balance_base", &BalanceType);
    module_add_object(m, "Delay_base", &DelayType);
    module_add_object(m, "SDelay_base", &SDelayType);
    module_add_object(m, "Waveguide_base", &WaveguideType);
    module_add_object(m, "AllpassWG_base", &AllpassWGType);
    module_add_object(m, "Midictl_base", &MidictlType);
    module_add_object(m, "CtlScan_base", &CtlScanType);
    module_add_object(m, "CtlScan2_base", &CtlScan2Type);
    module_add_object(m, "MidiNote_base", &MidiNoteType);
    module_add_object(m, "Notein_base", &NoteinType);
    module_add_object(m, "NoteinTrig_base", &NoteinTrigType);
    module_add_object(m, "Bendin_base", &BendinType);
    module_add_object(m, "Touchin_base", &TouchinType);
    module_add_object(m, "Programin_base", &PrograminType);
    module_add_object(m, "MidiAdsr_base", &MidiAdsrType);
    module_add_object(m, "MidiDelAdsr_base", &MidiDelAdsrType);
    module_add_object(m, "TrigRand_base", &TrigRandType);
    module_add_object(m, "TrigRandInt_base", &TrigRandIntType);
    module_add_object(m, "TrigVal_base", &TrigValType);
    module_add_object(m, "TrigChoice_base", &TrigChoiceType);
    module_add_object(m, "Iter_base", &IterType);
    module_add_object(m, "TrigEnv_base", &TrigEnvType);
    module_add_object(m, "TrigLinseg_base", &TrigLinsegType);
    module_add_object(m, "TrigExpseg_base", &TrigExpsegType);
    module_add_object(m, "TrigFunc_base", &TrigFuncType);
    module_add_object(m, "TrigXnoise_base", &TrigXnoiseType);
    module_add_object(m, "TrigXnoiseMidi_base", &TrigXnoiseMidiType);
    module_add_object(m, "Pattern_base", &PatternType);
    module_add_object(m, "CallAfter_base", &CallAfterType);
    module_add_object(m, "BandSplitter_base", &BandSplitterType);
    module_add_object(m, "BandSplit_base", &BandSplitType);
    module_add_object(m, "FourBandMain_base", &FourBandMainType);
    module_add_object(m, "FourBand_base", &FourBandType);
    module_add_object(m, "HilbertMain_base", &HilbertMainType);
    module_add_object(m, "Hilbert_base", &HilbertType);
    module_add_object(m, "Follower_base", &FollowerType);
    module_add_object(m, "Follower2_base", &Follower2Type);
    module_add_object(m, "ZCross_base", &ZCrossType);
    module_add_object(m, "SPanner_base", &SPannerType);
    module_add_object(m, "Panner_base", &PannerType);
    module_add_object(m, "Pan_base", &PanType);
    module_add_object(m, "SPan_base", &SPanType);
    module_add_object(m, "Switcher_base", &SwitcherType);
    module_add_object(m, "Switch_base", &SwitchType);
    module_add_object(m, "Selector_base", &SelectorType);
    module_add_object(m, "VoiceManager_base", &VoiceManagerType);
    module_add_object(m, "Mixer_base", &MixerType);
    module_add_object(m, "MixerVoice_base", &MixerVoiceType);
    module_add_object(m, "Counter_base", &CounterType);
    module_add_object(m, "Count_base", &CountType);
    module_add_object(m, "Thresh_base", &ThreshType);
    module_add_object(m, "Percent_base", &PercentType);
    module_add_object(m, "Timer_base", &TimerType);
    module_add_object(m, "Select_base", &SelectType);
    module_add_object(m, "Change_base", &ChangeType);
    module_add_object(m, "Score_base", &ScoreType);
    module_add_object(m, "Freeverb_base", &FreeverbType);
    module_add_object(m, "WGVerb_base", &WGVerbType);
    module_add_object(m, "Chorus_base", &ChorusType);
    module_add_object(m, "Convolve_base", &ConvolveType);
    module_add_object(m, "IRWinSinc_base", &IRWinSincType);
    module_add_object(m, "IRPulse_base", &IRPulseType);
    module_add_object(m, "IRAverage_base", &IRAverageType);
    module_add_object(m, "IRFM_base", &IRFMType);
    module_add_object(m, "Granulator_base", &GranulatorType);
    module_add_object(m, "Looper_base", &LooperType);
    module_add_object(m, "LooperTimeStream_base", &LooperTimeStreamType);
    module_add_object(m, "Harmonizer_base", &HarmonizerType);
    module_add_object(m, "Print_base", &PrintType);
    module_add_object(m, "M_Sin_base", &M_SinType);
    module_add_object(m, "M_Cos_base", &M_CosType);
    module_add_object(m, "M_Tan_base", &M_TanType);
    module_add_object(m, "M_Abs_base", &M_AbsType);
    module_add_object(m, "M_Sqrt_base", &M_SqrtType);
    module_add_object(m, "M_Log_base", &M_LogType);
    module_add_object(m, "M_Log2_base", &M_Log2Type);
    module_add_object(m, "M_Log10_base", &M_Log10Type);
    module_add_object(m, "M_Pow_base", &M_PowType);
    module_add_object(m, "M_Atan2_base", &M_Atan2Type);
    module_add_object(m, "M_Floor_base", &M_FloorType);
    module_add_object(m, "M_Ceil_base", &M_CeilType);
    module_add_object(m, "M_Round_base", &M_RoundType);
    module_add_object(m, "M_Tanh_base", &M_TanhType);
    module_add_object(m, "M_Exp_base", &M_ExpType);
    module_add_object(m, "Snap_base", &SnapType);
    module_add_object(m, "Interp_base", &InterpType);
    module_add_object(m, "SampHold_base", &SampHoldType);
    module_add_object(m, "DBToA_base", &DBToAType);
    module_add_object(m, "AToDB_base", &AToDBType);
    module_add_object(m, "Scale_base", &ScaleType);
    module_add_object(m, "CentsToTranspo_base", &CentsToTranspoType);
    module_add_object(m, "TranspoToCents_base", &TranspoToCentsType);
    module_add_object(m, "MToF_base", &MToFType);
    module_add_object(m, "FToM_base", &FToMType);
    module_add_object(m, "MToT_base", &MToTType);
    module_add_object(m, "FFTMain_base", &FFTMainType);
    module_add_object(m, "FFT_base", &FFTType);
    module_add_object(m, "IFFT_base", &IFFTType);
    module_add_object(m, "CarToPol_base", &CarToPolType);
    module_add_object(m, "PolToCar_base", &PolToCarType);
    module_add_object(m, "FrameDeltaMain_base", &FrameDeltaMainType);
    module_add_object(m, "FrameDelta_base", &FrameDeltaType);
    module_add_object(m, "FrameAccum_base", &FrameAccumType);
    module_add_object(m, "FrameAccumMain_base", &FrameAccumMainType);
    module_add_object(m, "VectralMain_base", &VectralMainType);
    module_add_object(m, "Vectral_base", &VectralType);
    module_add_object(m, "Min_base", &MinType);
    module_add_object(m, "Max_base", &MaxType);
    module_add_object(m, "Delay1_base", &Delay1Type);
    module_add_object(m, "RCOsc_base", &RCOscType);
    module_add_object(m, "Yin_base", &YinType);
    module_add_object(m, "SVF_base", &SVFType);
    module_add_object(m, "SVF2_base", &SVF2Type);
    module_add_object(m, "Average_base", &AverageType);
    module_add_object(m, "CvlVerb_base", &CvlVerbType);
    module_add_object(m, "Spectrum_base", &SpectrumType);
    module_add_object(m, "Reson_base", &ResonType);
    module_add_object(m, "Resonx_base", &ResonxType);
    module_add_object(m, "ButLP_base", &ButLPType);
    module_add_object(m, "ButHP_base", &ButHPType);
    module_add_object(m, "ButBP_base", &ButBPType);
    module_add_object(m, "ButBR_base", &ButBRType);
    module_add_object(m, "MoogLP_base", &MoogLPType);
    module_add_object(m, "PVAnal_base", &PVAnalType);
    module_add_object(m, "PVSynth_base", &PVSynthType);
    module_add_object(m, "PVTranspose_base", &PVTransposeType);
    module_add_object(m, "PVVerb_base", &PVVerbType);
    module_add_object(m, "PVGate_base", &PVGateType);
    module_add_object(m, "PVAddSynth_base", &PVAddSynthType);
    module_add_object(m, "PVCross_base", &PVCrossType);
    module_add_object(m, "PVMult_base", &PVMultType);
    module_add_object(m, "PVMorph_base", &PVMorphType);
    module_add_object(m, "PVFilter_base", &PVFilterType);
    module_add_object(m, "PVDelay_base", &PVDelayType);
    module_add_object(m, "PVBuffer_base", &PVBufferType);
    module_add_object(m, "PVShift_base", &PVShiftType);
    module_add_object(m, "PVAmpMod_base", &PVAmpModType);
    module_add_object(m, "PVFreqMod_base", &PVFreqModType);
    module_add_object(m, "PVBufLoops_base", &PVBufLoopsType);
    module_add_object(m, "PVBufTabLoops_base", &PVBufTabLoopsType);
    module_add_object(m, "PVMix_base", &PVMixType);
    module_add_object(m, "Granule_base", &GranuleType);
    module_add_object(m, "TableScale_base", &TableScaleType);
    module_add_object(m, "TrackHold_base", &TrackHoldType);
    module_add_object(m, "ComplexRes_base", &ComplexResType);
    module_add_object(m, "STReverb_base", &STReverbType);
    module_add_object(m, "STRev_base", &STRevType);
    module_add_object(m, "Pointer2_base", &Pointer2Type);
    module_add_object(m, "Centroid_base", &CentroidType);
    module_add_object(m, "AttackDetector_base", &AttackDetectorType);
    module_add_object(m, "SmoothDelay_base", &SmoothDelayType);
    module_add_object(m, "TrigBurster_base", &TrigBursterType);
    module_add_object(m, "TrigBurst_base", &TrigBurstType);
    module_add_object(m, "TrigBurstTapStream_base", &TrigBurstTapStreamType);
    module_add_object(m, "TrigBurstAmpStream_base", &TrigBurstAmpStreamType);
    module_add_object(m, "TrigBurstDurStream_base", &TrigBurstDurStreamType);
    module_add_object(m, "TrigBurstEndStream_base", &TrigBurstEndStreamType);
    module_add_object(m, "Scope_base", &ScopeType);
    module_add_object(m, "PeakAmp_base", &PeakAmpType);
    module_add_object(m, "MainParticle_base", &MainParticleType);
    module_add_object(m, "Particle_base", &ParticleType);
    module_add_object(m, "MainParticle2_base", &MainParticle2Type);
    module_add_object(m, "Particle2_base", &Particle2Type);
    module_add_object(m, "AtanTable_base", &AtanTableType);
    module_add_object(m, "RawMidi_base", &RawMidiType);
    module_add_object(m, "Resample_base", &ResampleType);
    module_add_object(m, "Expr_base", &ExprType);
    module_add_object(m, "PadSynthTable_base", &PadSynthTableType);
    module_add_object(m, "LogiMap_base", &LogiMapType);
    module_add_object(m, "SharedTable_base", &SharedTableType);
    module_add_object(m, "TableFill_base", &TableFillType);
    module_add_object(m, "TableScan_base", &TableScanType);
    module_add_object(m, "HRTFData_base", &HRTFDataType);
    module_add_object(m, "HRTFSpatter_base", &HRTFSpatterType);
    module_add_object(m, "HRTF_base", &HRTFType);
    module_add_object(m, "Expand_base", &ExpandType);
    module_add_object(m, "RMS_base", &RMSType);
    module_add_object(m, "MidiLinseg_base", &MidiLinsegType);
    module_add_object(m, "MultiBandMain_base", &MultiBandMainType);
    module_add_object(m, "MultiBand_base", &MultiBandType);
    module_add_object(m, "M_Div_base", &M_DivType);
    module_add_object(m, "M_Sub_base", &M_SubType);

    PyModule_AddStringConstant(m, "PYO_VERSION", PYO_VERSION);
#ifdef COMPILE_EXTERNALS
    EXTERNAL_OBJECTS
    PyModule_AddIntConstant(m, "WITH_EXTERNALS", 1);
#else
    PyModule_AddIntConstant(m, "WITH_EXTERNALS", 0);
#endif
#ifndef USE_DOUBLE
    PyModule_AddIntConstant(m, "USE_DOUBLE", 0);
#else
    PyModule_AddIntConstant(m, "USE_DOUBLE", 1);
#endif

#if PY_MAJOR_VERSION >= 3
    return m;
#endif
}
