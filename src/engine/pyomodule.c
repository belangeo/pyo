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

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <math.h>
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
libsndfile_get_format(int fileformat, int sampletype)
{
    int format = 0;

    switch (fileformat)
    {
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

    if (fileformat != 7)
    {
        switch (sampletype)
        {
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
p_sndinfo(PyObject *self, PyObject *args, PyObject *kwds)
{
    SNDFILE *sf;
    SF_INFO info;
    char *path;
    char fileformat[5];
    char sampletype[16];
    int format, subformat, print = 0;
    Py_ssize_t psize;

    static char *kwlist[] = {"path", "print", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "s#|i", kwlist, &path, &psize, &print))
    {
        PySys_WriteStderr("Pyo error: sndinfo called with wrong arguments.\n");
        Py_RETURN_NONE;
    }

    /* Open the sound file. */
    info.format = 0;
    sf = sf_open(path, SFM_READ, &info);

    if (sf == NULL)
    {
        Py_RETURN_NONE;
    }

    /* Retrieve file format */
    format = (int)info.format & SF_FORMAT_TYPEMASK;
    subformat = (int)info.format & SF_FORMAT_SUBMASK;

    sf_close(sf);

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

    return Py_BuildValue("Lffiss", info.frames, (float)info.frames / info.samplerate, (float)info.samplerate, info.channels, fileformat, sampletype);
}

static PyObject *
p_savefile(PyObject *self, PyObject *args, PyObject *kwds)
{
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
        return PyLong_FromLong(-1);

    recinfo.samplerate = sr;
    recinfo.channels = channels;
    recinfo.format = libsndfile_get_format(fileformat, sampletype);

    if (channels == 1)
    {
        size = PyList_Size(samples);
        sampsarray = (MYFLT *)PyMem_RawMalloc(size * sizeof(MYFLT));

        for (i = 0; i < size; i++)
        {
            sampsarray[i] = PyFloat_AsDouble(PyList_GET_ITEM(samples, i));
        }
    }
    else
    {
        if (PyList_Size(samples) != channels)
        {
            PySys_WriteStdout("Pyo error: savefile's samples list size and channels number must be the same!\n");
            return PyLong_FromLong(-1);
        }

        size = PyList_Size(PyList_GET_ITEM(samples, 0)) * channels;
        sampsarray = (MYFLT *)PyMem_RawMalloc(size * sizeof(MYFLT));

        for (i = 0; i < (size / channels); i++)
        {
            for (j = 0; j < channels; j++)
            {
                sampsarray[i * channels + j] = PyFloat_AsDouble(PyList_GET_ITEM(PyList_GET_ITEM(samples, j), i));
            }
        }
    }

    if (! (recfile = sf_open(recpath, SFM_WRITE, &recinfo)))
    {
        PySys_WriteStdout("Pyo error: savefile failed to open output file %s.\n", recpath);
        return PyLong_FromLong(-1);
    }

    // Sets the encoding quality for FLAC and OGG compressed formats
    if (fileformat == 5 || fileformat == 7)
    {
        sf_command(recfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(double));
    }

    SF_WRITE(recfile, sampsarray, size);
    sf_close(recfile);
    PyMem_RawFree(sampsarray);

    Py_RETURN_NONE;
}

static PyObject *
p_savefileFromTable(PyObject *self, PyObject *args, PyObject *kwds)
{
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
        return PyLong_FromLong(-1);

    base_objs = PyObject_GetAttrString(table, "_base_objs");
    channels = PyList_Size(base_objs);
    tablestreamlist = PyList_New(channels);

    for (i = 0; i < channels; i++)
    {
        PyList_SET_ITEM(tablestreamlist, i, PYO_CALL_METHOD_RET(PyList_GetItem(base_objs, i), "getTableStream", NULL));
    }

    sr = (int)TableStream_getSamplingRate((TableStream *)PyList_GetItem(tablestreamlist, 0));
    size = TableStream_getSize((TableStream *)PyList_GetItem(tablestreamlist, 0));

    recinfo.samplerate = sr;
    recinfo.channels = channels;
    recinfo.format = libsndfile_get_format(fileformat, sampletype);

    if (! (recfile = sf_open(recpath, SFM_WRITE, &recinfo)))
    {
        PySys_WriteStdout("Pyo error: savefileFromTable failed to open output file %s.\n", recpath);
        Py_XDECREF(base_objs);
        Py_XDECREF(tablestreamlist);
        return PyLong_FromLong(-1);
    }

    // Sets the encoding quality for FLAC and OGG compressed formats
    if (fileformat == 5 || fileformat == 7)
    {
        sf_command(recfile, SFC_SET_VBR_ENCODING_QUALITY, &quality, sizeof(double));
    }

    if (channels == 1)
    {
        MYFLT *data;

        if (size < (sr * 60))
        {
            data = TableStream_getData((TableStream *)PyList_GetItem(tablestreamlist, 0));
            sampsarray = (MYFLT *)PyMem_RawMalloc(size * sizeof(MYFLT));

            for (i = 0; i < size; i++)
            {
                sampsarray[i] = data[i];
            }

            SF_WRITE(recfile, sampsarray, size);
        }
        else
        {
            data = TableStream_getData((TableStream *)PyList_GetItem(tablestreamlist, 0));
            num_items = sr * 30;
            sampsarray = (MYFLT *)PyMem_RawMalloc(num_items * sizeof(MYFLT));

            do
            {
                if ((size - count) < num_items)
                    num_items = size - count;

                for (i = 0; i < num_items; i++)
                {
                    sampsarray[i] = data[count++];
                }

                SF_WRITE(recfile, sampsarray, num_items);
            }
            while (num_items == (sr * 30));
        }
    }
    else
    {
        MYFLT *data[channels];

        if (size < (sr * 60))
        {
            for (j = 0; j < channels; j++)
            {
                data[j] = TableStream_getData((TableStream *)PyList_GetItem(tablestreamlist, j));
            }

            sampsarray = (MYFLT *)PyMem_RawMalloc(size * channels * sizeof(MYFLT));

            for (i = 0; i < size; i++)
            {
                for (j = 0; j < channels; j++)
                {
                    sampsarray[i * channels + j] = data[j][i];
                }
            }

            SF_WRITE(recfile, sampsarray, size * channels);
        }
        else
        {
            for (j = 0; j < channels; j++)
            {
                data[j] = TableStream_getData((TableStream *)PyList_GetItem(tablestreamlist, j));
            }

            num_items = sr * 30;
            sampsarray = (MYFLT *)PyMem_RawMalloc(num_items * channels * sizeof(MYFLT));

            do
            {
                if ((size - count) < num_items)
                    num_items = size - count;

                for (i = 0; i < num_items; i++)
                {
                    for (j = 0; j < channels; j++)
                    {
                        sampsarray[i * channels + j] = data[j][count];
                    }

                    count++;
                }

                SF_WRITE(recfile, sampsarray, num_items * channels);
            }
            while (num_items == (sr * 30));
        }
    }

    sf_close(recfile);
    PyMem_RawFree(sampsarray);
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
gen_lp_impulse(MYFLT *array, int size, float freq)
{
    int i, ppi;
    MYFLT pp, ppf, env, scl, invSum, val;
    int half = size / 2;
    MYFLT vsum = 0.0;
    MYFLT envPointerScaling = 1.0 / (size + 1) * 1024.0;
    MYFLT sincScaling = (MYFLT)half;

    for (i = 0; i < half; i++)
    {
        pp = i * envPointerScaling;
        ppi = (int)pp;
        ppf = pp - ppi;
        env = HALF_BLACKMAN[ppi] + (HALF_BLACKMAN[ppi + 1] - HALF_BLACKMAN[ppi]) * ppf;
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

    for (i = 0; i < half; i++)
    {
        array[i] *= invSum;
    }

    for (i = 1; i < half; i++)
    {
        array[half + i] = array[half - i];
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
lp_conv(MYFLT *samples, MYFLT *impulse, int num_samps, int size, int gain)
{
    int i, j, count, tmp_count;
    MYFLT val;
    MYFLT intmp[size];

    for (i = 0; i < size; i++)
    {
        intmp[i] = 0.0;
    }

    count = 0;

    for (i = 0; i < num_samps; i++)
    {
        val = 0.0;
        tmp_count = count;

        for (j = 0; j < size; j++)
        {
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
        return PyLong_FromLong(-1);

    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);

    if (sf == NULL)
    {
        PySys_WriteStdout("Pyo error: upsamp failed to open the input file %s.\n", inpath);
        return PyLong_FromLong(-1);
    }

    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)PyMem_RawMalloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)PyMem_RawMalloc(snd_chnls * sizeof(MYFLT *));

    for (i = 0; i < snd_chnls; i++)
        samples[i] = (MYFLT *)PyMem_RawMalloc(snd_size * sizeof(MYFLT));

    for (i = 0; i < num_items; i++)
        samples[i % snd_chnls][(int)(i / snd_chnls)] = tmp[i];

    PyMem_RawFree(tmp);

    /* upsampling */
    upsamples = (MYFLT **)PyMem_RawMalloc(snd_chnls * sizeof(MYFLT *));

    for (i = 0; i < snd_chnls; i++)
        upsamples[i] = (MYFLT *)PyMem_RawMalloc(snd_size * up * sizeof(MYFLT));

    for (i = 0; i < snd_size; i++)
    {
        for (j = 0; j < snd_chnls; j++)
        {
            upsamples[j][i * up] = samples[j][i];

            for (k = 1; k < up; k++)
            {
                upsamples[j][i * up + k] = 0.0;
            }
        }
    }

    if (order > 2)
    {
        /* apply lowpass filter */
        sincfunc = (MYFLT *)PyMem_RawMalloc(order * sizeof(MYFLT));
        gen_lp_impulse(sincfunc, order, PI / up);

        for (i = 0; i < snd_chnls; i++)
        {
            lp_conv(upsamples[i], sincfunc, snd_size * up, order, up);
        }

        PyMem_RawFree(sincfunc);
    }

    /* save upsampled file */
    info.samplerate = snd_sr * up;
    tmp = (MYFLT *)PyMem_RawMalloc(num_items * up * sizeof(MYFLT));

    for (i = 0; i < (snd_size * up); i++)
    {
        for (j = 0; j < snd_chnls; j++)
        {
            tmp[i * snd_chnls + j] = upsamples[j][i];
        }
    }

    if (! (sf = sf_open(outpath, SFM_WRITE, &info)))
    {
        PySys_WriteStdout("Pyo error: upsamp failed to open output file %s.\n", outpath);
        PyMem_RawFree(tmp);

        for (i = 0; i < snd_chnls; i++)
        {
            PyMem_RawFree(samples[i]);
            PyMem_RawFree(upsamples[i]);
        }

        PyMem_RawFree(samples);
        PyMem_RawFree(upsamples);
        return PyLong_FromLong(-1);
    }

    SF_WRITE(sf, tmp, num_items * up);
    sf_close(sf);

    /* clean-up */
    PyMem_RawFree(tmp);

    for (i = 0; i < snd_chnls; i++)
    {
        PyMem_RawFree(samples[i]);
        PyMem_RawFree(upsamples[i]);
    }

    PyMem_RawFree(samples);
    PyMem_RawFree(upsamples);

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
        return PyLong_FromLong(-1);

    /* opening input soundfile */
    info.format = 0;
    sf = sf_open(inpath, SFM_READ, &info);

    if (sf == NULL)
    {
        PySys_WriteStdout("Pyo error: downsamp failed to open the input file %s.\n", inpath);
        return PyLong_FromLong(-1);
    }

    snd_size = info.frames;
    snd_sr = info.samplerate;
    snd_chnls = info.channels;
    num_items = snd_size * snd_chnls;
    tmp = (MYFLT *)PyMem_RawMalloc(num_items * sizeof(MYFLT));
    sf_seek(sf, 0, SEEK_SET);
    SF_READ(sf, tmp, num_items);
    sf_close(sf);
    samples = (MYFLT **)PyMem_RawMalloc(snd_chnls * sizeof(MYFLT *));

    for (i = 0; i < snd_chnls; i++)
        samples[i] = (MYFLT *)PyMem_RawMalloc(snd_size * sizeof(MYFLT));

    for (i = 0; i < num_items; i++)
        samples[i % snd_chnls][(int)(i / snd_chnls)] = tmp[i];

    PyMem_RawFree(tmp);

    if (order > 2)
    {
        /* apply lowpass filter */
        sincfunc = (MYFLT *)PyMem_RawMalloc(order * sizeof(MYFLT));
        gen_lp_impulse(sincfunc, order, PI / down);

        for (i = 0; i < snd_chnls; i++)
        {
            lp_conv(samples[i], sincfunc, snd_size, order, 1);
        }

        PyMem_RawFree(sincfunc);
    }

    /* downsampling */
    samples_per_channels = (snd_size / down) + (snd_size % down);
    downsamples = (MYFLT **)PyMem_RawMalloc(snd_chnls * sizeof(MYFLT *));

    for (i = 0; i < snd_chnls; i++)
    {
        downsamples[i] = (MYFLT *)PyMem_RawMalloc(samples_per_channels * sizeof(MYFLT));

        for (j = 0; j < samples_per_channels; j++)
        {
            downsamples[i][j] = 0.0;
        }
    }

    for (i = 0; i < samples_per_channels; i++)
    {
        for (j = 0; j < snd_chnls; j++)
        {
            if (i * down < snd_size)
                downsamples[j][i] = samples[j][i * down];
            else
                downsamples[j][i] = 0.0;
        }
    }

    /* save downsampled file */
    info.samplerate = snd_sr / down;
    tmp = (MYFLT *)PyMem_RawMalloc(snd_chnls * samples_per_channels * sizeof(MYFLT));

    for (i = 0; i < samples_per_channels; i++)
    {
        for (j = 0; j < snd_chnls; j++)
        {
            tmp[i * snd_chnls + j] = downsamples[j][i];
        }
    }

    if (! (sf = sf_open(outpath, SFM_WRITE, &info)))
    {
        PySys_WriteStdout("Pyo error: downsamp failed to open the output file %s.\n", outpath);
        PyMem_RawFree(tmp);

        for (i = 0; i < snd_chnls; i++)
        {
            PyMem_RawFree(samples[i]);
            PyMem_RawFree(downsamples[i]);
        }

        PyMem_RawFree(samples);
        PyMem_RawFree(downsamples);
        return PyLong_FromLong(-1);
    }

    SF_WRITE(sf, tmp, snd_chnls * samples_per_channels);
    sf_close(sf);

    /* clean-up */
    PyMem_RawFree(tmp);

    for (i = 0; i < snd_chnls; i++)
    {
        PyMem_RawFree(samples[i]);
        PyMem_RawFree(downsamples[i]);
    }

    PyMem_RawFree(samples);
    PyMem_RawFree(downsamples);

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

typedef struct STACK_RECORD
{
    int nAnchorIndex;
    int nFloaterIndex;
    struct STACK_RECORD *precPrev;
} STACK_RECORD;

STACK_RECORD *m_pStack = NULL;

static void StackPush( int nAnchorIndex, int nFloaterIndex )
{
    STACK_RECORD *precPrev = m_pStack;
    m_pStack = (STACK_RECORD *)PyMem_RawMalloc( sizeof(STACK_RECORD) );
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
    PyMem_RawFree( precStack );
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
        Py_RETURN_NONE;

    nPointsCount = PyList_Size(pointlist);

    pPointsX = (MYFLT *)PyMem_RawMalloc(nPointsCount * sizeof(MYFLT));
    pPointsY = (MYFLT *)PyMem_RawMalloc(nPointsCount * sizeof(MYFLT));
    pnUseFlag = (int *)PyMem_RawMalloc(nPointsCount * sizeof(int));

    tup = PyList_GET_ITEM(pointlist, 0);

    if (PyTuple_Check(tup) == 1)
    {
        for (i = 0; i < nPointsCount; i++)
        {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 0));
            pPointsY[i] = PyFloat_AsDouble(PyTuple_GET_ITEM(tup, 1));
            pnUseFlag[i] = 0;
        }
    }
    else
    {
        for (i = 0; i < nPointsCount; i++)
        {
            tup = PyList_GET_ITEM(pointlist, i);
            pPointsX[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 0));
            pPointsY[i] = PyFloat_AsDouble(PyList_GET_ITEM(tup, 1));
            pnUseFlag[i] = 0;
        }
    }

    // rescale points between 0. and 1.
    xMax = pPointsX[nPointsCount - 1];
    yMin = 9999999999.9;
    yMax = -999999.9;

    for (i = 0; i < nPointsCount; i++)
    {
        if (pPointsY[i] < yMin)
            yMin = pPointsY[i];
        else if (pPointsY[i] > yMax)
            yMax = pPointsY[i];
    }

    for (i = 0; i < nPointsCount; i++)
    {
        pPointsX[i] = pPointsX[i] / xMax;
        pPointsY[i] = (pPointsY[i] - yMin) / yMax;
    }

    // filter...
    pnUseFlag[0] = pnUseFlag[nPointsCount - 1] = 1;
    nAnchorIndex = 0;
    nFloaterIndex = nPointsCount - 1;
    StackPush( nAnchorIndex, nFloaterIndex );

    while ( StackPop( &nAnchorIndex, &nFloaterIndex ) )
    {
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

        for ( nVertexIndex = nAnchorIndex + 1; nVertexIndex < nFloaterIndex; nVertexIndex++ )
        {
            //compare to anchor
            dVertexVecX = pPointsX[ nVertexIndex ] - pPointsX[ nAnchorIndex ];
            dVertexVecY = pPointsY[ nVertexIndex ] - pPointsY[ nAnchorIndex ];
            dVertexVecLength = sqrt( dVertexVecX * dVertexVecX
                                     + dVertexVecY * dVertexVecY );
            //dot product:
            dProjScalar = dVertexVecX * dAnchorUnitVecX + dVertexVecY * dAnchorUnitVecY;

            if ( dProjScalar < 0.0 )
                dVertexDistanceToSegment = dVertexVecLength;
            else
            {
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

            if ( dMaxDistThisSegment < dVertexDistanceToSegment )
            {
                dMaxDistThisSegment = dVertexDistanceToSegment;
                nVertexIndexMaxDistance = nVertexIndex;
            }
        }

        if ( dMaxDistThisSegment <= dTolerance )   //use line segment
        {
            pnUseFlag[ nAnchorIndex ] = 1;
            pnUseFlag[ nFloaterIndex ] = 1;
        }
        else
        {
            StackPush( nAnchorIndex, nVertexIndexMaxDistance );
            StackPush( nVertexIndexMaxDistance, nFloaterIndex );
        }
    }

    pPointsOut = PyList_New(0);

    for (i = 0; i < nPointsCount; i++)
    {
        if (pnUseFlag[i] == 1)
        {
            PyList_Append(pPointsOut, PyList_GET_ITEM(pointlist, i));
        }
    }

    PyMem_RawFree(pPointsX);
    PyMem_RawFree(pPointsY);
    PyMem_RawFree(pnUseFlag);

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
        return PyLong_FromLong(-1);

    pf = PySequence_Fast(p, NULL);
    pf1 = PySequence_Fast(p1, NULL);
    pf2 = PySequence_Fast(p2, NULL);

    if (PyTuple_Check(p))
        Py_DECREF(p);
    if (PyTuple_Check(p1))
        Py_DECREF(p1);
    if (PyTuple_Check(p2))
        Py_DECREF(p2);

    if (xlog == 0)
    {
        xscale = xmax - xmin;
        xp[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xscale;
        xp1[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xscale;
        xp2[0] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xscale;
    }
    else
    {
        xscale = MYLOG10(xmax / xmin);
        xp[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 0)) / xmin) / xscale;
        xp1[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 0)) / xmin) / xscale;
        xp2[0] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 0)) / xmin) / xscale;
    }

    if (ylog == 0)
    {
        yscale = ymax - ymin;
        xp[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / yscale;
        xp1[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / yscale;
        xp2[1] = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / yscale;
    }
    else
    {
        yscale = MYLOG10(ymax / ymin);
        xp[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf, 1)) / ymin) / yscale;
        xp1[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf1, 1)) / ymin) / yscale;
        xp2[1] = MYLOG10(PyFloat_AsDouble(PySequence_Fast_GET_ITEM(pf2, 1)) / ymin) / yscale;
    }

    xDelta = xp2[0] - xp1[0];
    yDelta = xp2[1] - xp1[1];
    u = ((xp[0] - xp1[0]) * xDelta + (xp[1] - xp1[1]) * yDelta) / (xDelta * xDelta + yDelta * yDelta);

    if (u < 0.0)
    {
        closest[0] = xp1[0];
        closest[1] = xp1[1];
    }
    else if (u > 1.0)
    {
        closest[0] = xp2[0];
        closest[1] = xp2[1];
    }
    else
    {
        closest[0] = xp1[0] + u * xDelta;
        closest[1] = xp1[1] + u * yDelta;
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
    PyObject *data, *fdata, *out, *inout, *ftup, *yrange = NULL, *fyrange = NULL;
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

    if (yrange)
    {
        fyrange = PySequence_Fast(yrange, NULL);
        ymin = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 0));
        ymax = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(fyrange, 1));
    }

    ydiff = ymax - ymin;
    log10ymin = log10(ymin);
    log10ymax = log10(ymax);

    fdata = PySequence_Fast(data, NULL);
    datasize = PySequence_Fast_GET_SIZE(fdata);
    xdata = (double *)PyMem_RawMalloc(datasize * sizeof(double));
    ydata = (double *)PyMem_RawMalloc(datasize * sizeof(double));

    /* acquire data + normalization */
    if (log == 0)
    {
        for (i = 0; i < datasize; i++)
        {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
            ydata[i] = (tmp - ymin) / ydiff;
        }
    }
    else
    {
        for (i = 0; i < datasize; i++)
        {
            ftup = PySequence_Fast(PySequence_Fast_GET_ITEM(fdata, i), NULL);
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 0));
            xdata[i] = tmp / totaldur;
            tmp = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(ftup, 1));
            ydata[i] = log10(tmp / ymin) / log10(ymax / ymin);
        }
    }

    cxdata = (double *)PyMem_RawMalloc((num + 5) * sizeof(double));
    cydata = (double *)PyMem_RawMalloc((num + 5) * sizeof(double));

    /* generates cos interpolation */
    for (i = 0; i < (datasize - 1); i++)
    {
        x1 = xdata[i];
        x2 = xdata[i + 1];
        y1 = ydata[i];
        y2 = ydata[i + 1];
        steps = (int)((x2 - x1) * num);

        if (steps <= 0)
            continue;

        for (j = 0; j < steps; j++)
        {
            mu = (1.0 - cos(j / (float)steps * PI)) * 0.5;
            cxdata[count] = x1 + inc * j;
            cydata[count++] = y1 + (y2 - y1) * mu;
        }
    }

    cxdata[count] = xdata[datasize - 1];
    cydata[count++] = ydata[datasize - 1];

    /* denormalization */
    if (log == 0)
    {
        for (i = 0; i < count; i++)
        {
            cxdata[i] *= totaldur;
            cydata[i] = cydata[i] * ydiff + ymin;
        }
    }
    else
    {
        for (i = 0; i < count; i++)
        {
            cxdata[i] *= totaldur;
            cydata[i] = pow(10.0, cydata[i] * (log10ymax - log10ymin) + log10ymin);
        }
    }

    /* output Python's list of lists */
    out = PyList_New(0);

    for (i = 0; i < count; i++)
    {
        inout = PyList_New(0);
        PyList_Append(inout, PyFloat_FromDouble(cxdata[i]));
        PyList_Append(inout, PyFloat_FromDouble(cydata[i]));
        PyList_Append(out, inout);
    }

    PyMem_RawFree(xdata);
    PyMem_RawFree(ydata);
    PyMem_RawFree(cxdata);
    PyMem_RawFree(cydata);

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
        Py_RETURN_NONE;

    if (PyNumber_Check(data))
        type = 0;
    else if (PyList_Check(data))
        type = 1;
    else
    {
        Py_RETURN_NONE;
    }

    if (xlog == 0 && ylog == 0)
    {
        datascl = xmax - xmin;
        curscl = ymax - ymin;
        curscl /= datascl;

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            return Py_BuildValue("d", (val - xmin) * curscl + ymin);
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                PyList_SET_ITEM(out, i, PyFloat_FromDouble((val - xmin) * curscl + ymin));
            }

            return out;
        }
    }
    else if (xlog == 0 && ylog == 1)
    {
        if (xmin == 0)
            xmin = 0.000001;

        datascl = xmax - xmin;
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);

            if (val == 0)
                val = 0.000001;

            val = (val - xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));

                if (val == 0)
                    val = 0.000001;

                val = (val - xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }

            return out;
        }
    }
    else if (xlog == 1 && ylog == 0)
    {
        datascl = MYLOG10(xmax / xmin);
        curscl = ymax - ymin;

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", val * curscl + ymin);
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(val * curscl + ymin));
            }

            return out;
        }
    }
    else if (xlog == 1 && ylog == 1)
    {
        datascl = MYLOG10(xmax / xmin);
        curscl = MYLOG10(ymax / ymin);
        ymin = MYLOG10(ymin);

        if (type == 0)
        {
            val = PyFloat_AsDouble(data);
            val = MYLOG10(val / xmin) / datascl;
            return Py_BuildValue("d", MYPOW(10.0, val * curscl + ymin));
        }
        else if (type == 1)
        {
            cnt = PyList_Size(data);
            out = PyList_New(cnt);

            for (i = 0; i < cnt; i++)
            {
                val = PyFloat_AsDouble(PyList_GET_ITEM(data, i));
                val = MYLOG10(val / xmin) / datascl;
                PyList_SET_ITEM(out, i, PyFloat_FromDouble(MYPOW(10.0, val * curscl + ymin)));
            }

            return out;
        }
    }
    else
    {
        Py_RETURN_NONE;
    }

    Py_RETURN_NONE;
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
        return PyLong_FromLong(-1);

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

static double
Midi_clip(double x)
{
    return x > 256.0 ? 256.0 : x < -256 ? -256.0 : x;
}

static PyObject *
midiToHz(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
    {
        x = Midi_clip(PyFloat_AsDouble(arg));
        return Py_BuildValue("d", 440.0 * MYPOW(2.0, (x - 69) / 12.0));
    }
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = Midi_clip(PyFloat_AsDouble(PyList_GET_ITEM(arg, i)));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(440.0 * MYPOW(2.0, (x - 69) / 12.0)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = Midi_clip(PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i)));
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
hzToMidi(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", 12.0 * MYLOG2(PyFloat_AsDouble(arg) / 440.0) + 69);
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(12.0 * MYLOG2(x / 440.0) + 69));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
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
midiToTranspo(PyObject *self, PyObject *arg)
{
    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", pow(1.0594630943593, PyFloat_AsDouble(arg) - 60.0));
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x - 60.0)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyFloat_FromDouble(pow(1.0594630943593, x - 60.0)));
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
sampsToSec(PyObject *self, PyObject *arg)
{
    PyObject *server = PyServer_get_server();

    if (server == NULL)
    {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `sampsToSec` function.\n");
        Py_RETURN_NONE;
    }

    PyObject *srobj = PYO_CALL_METHOD_RET(server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("d", PyFloat_AsDouble(arg) / sr);
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyFloat_FromDouble(x / sr));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
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
secToSamps(PyObject *self, PyObject *arg)
{
    PyObject *server = PyServer_get_server();

    if (server == NULL)
    {
        PySys_WriteStdout("Pyo error: A Server must be booted before calling `secToSamps` function.\n");
        Py_RETURN_NONE;
    }

    PyObject *srobj = PYO_CALL_METHOD_RET(server, "getSamplingRate", NULL);
    double sr = PyFloat_AsDouble(srobj);
    Py_DECREF(srobj);

    int count = 0;
    int i = 0;
    double x = 0.0;
    PyObject *newseq = NULL;

    if (PyNumber_Check(arg))
        return Py_BuildValue("l", (long)(PyFloat_AsDouble(arg) * sr));
    else if (PyList_Check(arg))
    {
        count = PyList_Size(arg);
        newseq = PyList_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyList_GET_ITEM(arg, i));
            PyList_SET_ITEM(newseq, i, PyLong_FromLong((long)(x * sr)));
        }

        return newseq;
    }
    else if (PyTuple_Check(arg))
    {
        count = PyTuple_Size(arg);
        newseq = PyTuple_New(count);

        for (i = 0; i < count; i++)
        {
            x = PyFloat_AsDouble(PyTuple_GET_ITEM(arg, i));
            PyTuple_SET_ITEM(newseq, i, PyLong_FromLong((long)(x * sr)));
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
serverCreated(PyObject *self)
{
    if (PyServer_get_server() != NULL)
    {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else
    {
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
serverBooted(PyObject *self)
{
    int boot;
    PyObject *server;

    if (PyServer_get_server() != NULL)
    {
        server = PyServer_get_server();
        boot = Pyo_CallMethod_AsLong(server, "getIsBooted");

        if (boot == 0)
        {
            Py_INCREF(Py_False);
            return Py_False;
        }
        else
        {
            Py_INCREF(Py_True);
            return Py_True;
        }
    }
    else
    {
        PySys_WriteStdout("Pyo Warning: A Server must be created before calling `serverBooted` function.\n");
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef pyo_functions[] =
{
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
    {"p_sndinfo", (PyCFunction)p_sndinfo, METH_VARARGS | METH_KEYWORDS, ""},
    {"p_savefile", (PyCFunction)p_savefile, METH_VARARGS | METH_KEYWORDS, ""},
    {"p_savefileFromTable", (PyCFunction)p_savefileFromTable, METH_VARARGS | METH_KEYWORDS, ""},
    {"p_upsamp", (PyCFunction)p_upsamp, METH_VARARGS | METH_KEYWORDS, ""},
    {"p_downsamp", (PyCFunction)p_downsamp, METH_VARARGS | METH_KEYWORDS, ""},
    {"reducePoints", (PyCFunction)reducePoints, METH_VARARGS | METH_KEYWORDS, reducePoints_info},
    {"distanceToSegment", (PyCFunction)distanceToSegment, METH_VARARGS | METH_KEYWORDS, distanceToSegment_info},
    {"rescale", (PyCFunction)rescale, METH_VARARGS | METH_KEYWORDS, rescale_info},
    {"floatmap", (PyCFunction)floatmap, METH_VARARGS | METH_KEYWORDS, floatmap_info},
    {"linToCosCurve", (PyCFunction)linToCosCurve, METH_VARARGS | METH_KEYWORDS, linToCosCurve_info},
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

static struct PyModuleDef pyo_moduledef;
static const char *pyo_state_module_key = LIB_BASE_NAME "._module";

static int
module_add_heap_object(PyObject *module, const char *name, PyTypeObject *(*factory)(PyObject *), PyoRuntimeTypeId id)
{
    PyTypeObject *type = factory(module);

    if (type == NULL)
        return -1;

    if (PyoType_SetCurrent(id, (PyTypeObject *)Py_NewRef((PyObject *)type)) < 0)
    {
        Py_DECREF(type);
        return -1;
    }

    if (PyModule_AddObjectRef(module, name, (PyObject *)type) < 0)
    {
        Py_DECREF(type);
        return -1;
    }

    Py_DECREF(type);
    return 0;
}

PyoModuleState *
PyoState_Get(void)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    PyObject *interp_dict = PyInterpreterState_GetDict(interp);
    PyObject *module;

    if (interp_dict == NULL)
        return NULL;

    module = PyDict_GetItemString(interp_dict, pyo_state_module_key);

    if (module == NULL)
        return NULL;

    return (PyoModuleState *)PyModule_GetState(module);
}

PyTypeObject *
PyoType_GetCurrent(PyoRuntimeTypeId id)
{
    PyoModuleState *state = PyoState_Get();

    if (state == NULL || id < 0 || id >= PYO_RUNTIME_TYPE_COUNT)
        return NULL;

    return state->runtime_types[id];
}

int
PyoType_SetCurrent(PyoRuntimeTypeId id, PyTypeObject *type)
{
    PyoModuleState *state = PyoState_Get();

    if (state == NULL || id < 0 || id >= PYO_RUNTIME_TYPE_COUNT)
        return -1;

    Py_XSETREF(state->runtime_types[id], type);
    return 0;
}

void
PyoState_Init(PyoModuleState *state)
{
    int i;

    if (state == NULL)
        return;

    state->current_server_id = 0;
    state->rand_seed = 1u;

    for (i = 0; i < MAX_NBR_SERVER; i++)
        state->servers[i] = NULL;

    for (i = 0; i < PYO_NUM_RND_OBJS; i++)
        state->rnd_objs_count[i] = 0;

    for (i = 0; i < PYO_RUNTIME_TYPE_COUNT; i++)
        state->runtime_types[i] = NULL;
}

static void
PyoState_ClearTypes(PyoModuleState *state)
{
    int i;

    if (state == NULL)
        return;

    for (i = 0; i < PYO_RUNTIME_TYPE_COUNT; i++)
        Py_CLEAR(state->runtime_types[i]);
}

static int
pyo_exec(PyObject *m)
{
    PyInterpreterState *interp;
    PyObject *interp_dict;

    PyoState_Init((PyoModuleState *)PyModule_GetState(m));

    interp = PyInterpreterState_Get();
    interp_dict = PyInterpreterState_GetDict(interp);

    if (interp_dict == NULL)
        return -1;

    if (PyDict_SetItemString(interp_dict, pyo_state_module_key, m) < 0)
        return -1;

    if (module_add_heap_object(m, "Server_base", PyoCreateServerType, PYO_RUNTIME_TYPE_SERVER) < 0)
        return -1;
#ifdef USE_PORTMIDI
    if (module_add_heap_object(m, "MidiListener_base", PyoCreateMidiListenerType, PYO_RUNTIME_TYPE_MIDI_LISTENER) < 0)
        return -1;
    if (module_add_heap_object(m, "MidiDispatcher_base", PyoCreateMidiDispatcherType, PYO_RUNTIME_TYPE_MIDI_DISPATCHER) < 0)
        return -1;
#endif
#ifdef USE_OSC
    if (module_add_heap_object(m, "OscListener_base", PyoCreateOscListenerType, PYO_RUNTIME_TYPE_OSC_LISTENER) < 0)
        return -1;
    if (module_add_heap_object(m, "OscSend_base", PyoCreateOscSendType, PYO_RUNTIME_TYPE_OSC_SEND) < 0)
        return -1;
    if (module_add_heap_object(m, "OscDataSend_base", PyoCreateOscDataSendType, PYO_RUNTIME_TYPE_OSC_DATA_SEND) < 0)
        return -1;
    if (module_add_heap_object(m, "OscReceive_base", PyoCreateOscReceiveType, PYO_RUNTIME_TYPE_OSC_RECEIVE) < 0)
        return -1;
    if (module_add_heap_object(m, "OscReceiver_base", PyoCreateOscReceiverType, PYO_RUNTIME_TYPE_OSC_RECEIVER) < 0)
        return -1;
    if (module_add_heap_object(m, "OscListReceive_base", PyoCreateOscListReceiveType, PYO_RUNTIME_TYPE_OSC_LIST_RECEIVE) < 0)
        return -1;
    if (module_add_heap_object(m, "OscListReceiver_base", PyoCreateOscListReceiverType, PYO_RUNTIME_TYPE_OSC_LIST_RECEIVER) < 0)
        return -1;
    if (module_add_heap_object(m, "OscDataReceive_base", PyoCreateOscDataReceiveType, PYO_RUNTIME_TYPE_OSC_DATA_RECEIVE) < 0)
        return -1;
#endif
    if (module_add_heap_object(m, "Stream", PyoCreateStreamType, PYO_RUNTIME_TYPE_STREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TriggerStream", PyoCreateTriggerStreamType, PYO_RUNTIME_TYPE_TRIGGER_STREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "PVStream", PyoCreatePVStreamType, PYO_RUNTIME_TYPE_PV_STREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "Dummy_base", PyoCreateDummyType, PYO_RUNTIME_TYPE_DUMMY) < 0)
        return -1;
    if (module_add_heap_object(m, "TriggerDummy_base", PyoCreateTriggerDummyType, PYO_RUNTIME_TYPE_TRIGGER_DUMMY) < 0)
        return -1;
    if (module_add_heap_object(m, "TableStream", PyoCreateTableStreamType, PYO_RUNTIME_TYPE_TABLE_STREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MatrixStream", PyoCreateMatrixStreamType, PYO_RUNTIME_TYPE_MATRIX_STREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "Record_base", PyoCreateRecordType, PYO_RUNTIME_TYPE_RECORD) < 0)
        return -1;
    if (module_add_heap_object(m, "ControlRec_base", PyoCreateControlRecType, PYO_RUNTIME_TYPE_CONTROLREC) < 0)
        return -1;
    if (module_add_heap_object(m, "ControlRead_base", PyoCreateControlReadType, PYO_RUNTIME_TYPE_CONTROLREAD) < 0)
        return -1;
    if (module_add_heap_object(m, "NoteinRec_base", PyoCreateNoteinRecType, PYO_RUNTIME_TYPE_NOTEINREC) < 0)
        return -1;
    if (module_add_heap_object(m, "NoteinRead_base", PyoCreateNoteinReadType, PYO_RUNTIME_TYPE_NOTEINREAD) < 0)
        return -1;
    if (module_add_heap_object(m, "Compare_base", PyoCreateCompareType, PYO_RUNTIME_TYPE_COMPARE) < 0)
        return -1;
    if (module_add_heap_object(m, "Mix_base", PyoCreateMixType, PYO_RUNTIME_TYPE_MIX) < 0)
        return -1;
    if (module_add_heap_object(m, "Sig_base", PyoCreateSigType, PYO_RUNTIME_TYPE_SIG) < 0)
        return -1;
    if (module_add_heap_object(m, "SigTo_base", PyoCreateSigToType, PYO_RUNTIME_TYPE_SIGTO) < 0)
        return -1;
    if (module_add_heap_object(m, "VarPort_base", PyoCreateVarPortType, PYO_RUNTIME_TYPE_VARPORT) < 0)
        return -1;
    if (module_add_heap_object(m, "InputFader_base", PyoCreateInputFaderType, PYO_RUNTIME_TYPE_INPUTFADER) < 0)
        return -1;
    if (module_add_heap_object(m, "Adsr_base", PyoCreateAdsrType, PYO_RUNTIME_TYPE_ADSR) < 0)
        return -1;
    if (module_add_heap_object(m, "Linseg_base", PyoCreateLinsegType, PYO_RUNTIME_TYPE_LINSEG) < 0)
        return -1;
    if (module_add_heap_object(m, "Expseg_base", PyoCreateExpsegType, PYO_RUNTIME_TYPE_EXPSEG) < 0)
        return -1;
    if (module_add_heap_object(m, "HarmTable_base", PyoCreateHarmTableType, PYO_RUNTIME_TYPE_HARMTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "ChebyTable_base", PyoCreateChebyTableType, PYO_RUNTIME_TYPE_CHEBYTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "HannTable_base", PyoCreateHannTableType, PYO_RUNTIME_TYPE_HANNTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "SincTable_base", PyoCreateSincTableType, PYO_RUNTIME_TYPE_SINCTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "WinTable_base", PyoCreateWinTableType, PYO_RUNTIME_TYPE_WINTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "ParaTable_base", PyoCreateParaTableType, PYO_RUNTIME_TYPE_PARATABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "LinTable_base", PyoCreateLinTableType, PYO_RUNTIME_TYPE_LINTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "LogTable_base", PyoCreateLogTableType, PYO_RUNTIME_TYPE_LOGTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "CosLogTable_base", PyoCreateCosLogTableType, PYO_RUNTIME_TYPE_COSLOGTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "CosTable_base", PyoCreateCosTableType, PYO_RUNTIME_TYPE_COSTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "CurveTable_base", PyoCreateCurveTableType, PYO_RUNTIME_TYPE_CURVETABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "ExpTable_base", PyoCreateExpTableType, PYO_RUNTIME_TYPE_EXPTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "SndTable_base", PyoCreateSndTableType, PYO_RUNTIME_TYPE_SNDTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "DataTable_base", PyoCreateDataTableType, PYO_RUNTIME_TYPE_DATATABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "NewTable_base", PyoCreateNewTableType, PYO_RUNTIME_TYPE_NEWTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "TableRec_base", PyoCreateTableRecType, PYO_RUNTIME_TYPE_TABLEREC) < 0)
        return -1;
    if (module_add_heap_object(m, "TableRecTimeStream_base", PyoCreateTableRecTimeStreamType, PYO_RUNTIME_TYPE_TABLERECTIMESTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TableMorph_base", PyoCreateTableMorphType, PYO_RUNTIME_TYPE_TABLEMORPH) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigTableRec_base", PyoCreateTrigTableRecType, PYO_RUNTIME_TYPE_TRIGTABLEREC) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigTableRecTimeStream_base", PyoCreateTrigTableRecTimeStreamType, PYO_RUNTIME_TYPE_TRIGTABLERECTIMESTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TableWrite_base", PyoCreateTableWriteType, PYO_RUNTIME_TYPE_TABLEWRITE) < 0)
        return -1;
    if (module_add_heap_object(m, "TablePut_base", PyoCreateTablePutType, PYO_RUNTIME_TYPE_TABLEPUT) < 0)
        return -1;
    if (module_add_heap_object(m, "NewMatrix_base", PyoCreateNewMatrixType, PYO_RUNTIME_TYPE_NEWMATRIX) < 0)
        return -1;
    if (module_add_heap_object(m, "MatrixPointer_base", PyoCreateMatrixPointerType, PYO_RUNTIME_TYPE_MATRIXPOINTER) < 0)
        return -1;
    if (module_add_heap_object(m, "MatrixRec_base", PyoCreateMatrixRecType, PYO_RUNTIME_TYPE_MATRIXREC) < 0)
        return -1;
    if (module_add_heap_object(m, "MatrixRecLoop_base", PyoCreateMatrixRecLoopType, PYO_RUNTIME_TYPE_MATRIXRECLOOP) < 0)
        return -1;
    if (module_add_heap_object(m, "MatrixMorph_base", PyoCreateMatrixMorphType, PYO_RUNTIME_TYPE_MATRIXMORPH) < 0)
        return -1;
    if (module_add_heap_object(m, "Input_base", PyoCreateInputType, PYO_RUNTIME_TYPE_INPUT) < 0)
        return -1;
    if (module_add_heap_object(m, "Trig_base", PyoCreateTrigType, PYO_RUNTIME_TYPE_TRIG) < 0)
        return -1;
    if (module_add_heap_object(m, "NextTrig_base", PyoCreateNextTrigType, PYO_RUNTIME_TYPE_NEXTTRIG) < 0)
        return -1;
    if (module_add_heap_object(m, "Metro_base", PyoCreateMetroType, PYO_RUNTIME_TYPE_METRO) < 0)
        return -1;
    if (module_add_heap_object(m, "Seqer_base", PyoCreateSeqerType, PYO_RUNTIME_TYPE_SEQER) < 0)
        return -1;
    if (module_add_heap_object(m, "Seq_base", PyoCreateSeqType, PYO_RUNTIME_TYPE_SEQ) < 0)
        return -1;
    if (module_add_heap_object(m, "Clouder_base", PyoCreateClouderType, PYO_RUNTIME_TYPE_CLOUDER) < 0)
        return -1;
    if (module_add_heap_object(m, "Cloud_base", PyoCreateCloudType, PYO_RUNTIME_TYPE_CLOUD) < 0)
        return -1;
    if (module_add_heap_object(m, "Beater_base", PyoCreateBeaterType, PYO_RUNTIME_TYPE_BEATER) < 0)
        return -1;
    if (module_add_heap_object(m, "Beat_base", PyoCreateBeatType, PYO_RUNTIME_TYPE_BEAT) < 0)
        return -1;
    if (module_add_heap_object(m, "BeatTapStream_base", PyoCreateBeatTapStreamType, PYO_RUNTIME_TYPE_BEATTAPSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "BeatAmpStream_base", PyoCreateBeatAmpStreamType, PYO_RUNTIME_TYPE_BEATAMPSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "BeatDurStream_base", PyoCreateBeatDurStreamType, PYO_RUNTIME_TYPE_BEATDURSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "BeatEndStream_base", PyoCreateBeatEndStreamType, PYO_RUNTIME_TYPE_BEATENDSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "Fader_base", PyoCreateFaderType, PYO_RUNTIME_TYPE_FADER) < 0)
        return -1;
    if (module_add_heap_object(m, "Randi_base", PyoCreateRandiType, PYO_RUNTIME_TYPE_RANDI) < 0)
        return -1;
    if (module_add_heap_object(m, "Randh_base", PyoCreateRandhType, PYO_RUNTIME_TYPE_RANDH) < 0)
        return -1;
    if (module_add_heap_object(m, "Choice_base", PyoCreateChoiceType, PYO_RUNTIME_TYPE_CHOICE) < 0)
        return -1;
    if (module_add_heap_object(m, "RandDur_base", PyoCreateRandDurType, PYO_RUNTIME_TYPE_RANDDUR) < 0)
        return -1;
    if (module_add_heap_object(m, "Xnoise_base", PyoCreateXnoiseType, PYO_RUNTIME_TYPE_XNOISE) < 0)
        return -1;
    if (module_add_heap_object(m, "XnoiseMidi_base", PyoCreateXnoiseMidiType, PYO_RUNTIME_TYPE_XNOISEMIDI) < 0)
        return -1;
    if (module_add_heap_object(m, "XnoiseDur_base", PyoCreateXnoiseDurType, PYO_RUNTIME_TYPE_XNOISEDUR) < 0)
        return -1;
    if (module_add_heap_object(m, "RandInt_base", PyoCreateRandIntType, PYO_RUNTIME_TYPE_RANDINT) < 0)
        return -1;
    if (module_add_heap_object(m, "Urn_base", PyoCreateUrnType, PYO_RUNTIME_TYPE_URN) < 0)
        return -1;
    if (module_add_heap_object(m, "SfPlayer_base", PyoCreateSfPlayerType, PYO_RUNTIME_TYPE_SFPLAYER) < 0)
        return -1;
    if (module_add_heap_object(m, "SfPlay_base", PyoCreateSfPlayType, PYO_RUNTIME_TYPE_SFPLAY) < 0)
        return -1;
    if (module_add_heap_object(m, "SfMarkerShuffler_base", PyoCreateSfMarkerShufflerType, PYO_RUNTIME_TYPE_SFMARKERSHUFFLER) < 0)
        return -1;
    if (module_add_heap_object(m, "SfMarkerShuffle_base", PyoCreateSfMarkerShuffleType, PYO_RUNTIME_TYPE_SFMARKERSHUFFLE) < 0)
        return -1;
    if (module_add_heap_object(m, "SfMarkerLooper_base", PyoCreateSfMarkerLooperType, PYO_RUNTIME_TYPE_SFMARKERLOOPER) < 0)
        return -1;
    if (module_add_heap_object(m, "SfMarkerLoop_base", PyoCreateSfMarkerLoopType, PYO_RUNTIME_TYPE_SFMARKERLOOP) < 0)
        return -1;
    if (module_add_heap_object(m, "Osc_base", PyoCreateOscType, PYO_RUNTIME_TYPE_OSC) < 0)
        return -1;
    if (module_add_heap_object(m, "OscLoop_base", PyoCreateOscLoopType, PYO_RUNTIME_TYPE_OSCLOOP) < 0)
        return -1;
    if (module_add_heap_object(m, "OscTrig_base", PyoCreateOscTrigType, PYO_RUNTIME_TYPE_OSCTRIG) < 0)
        return -1;
    if (module_add_heap_object(m, "OscBank_base", PyoCreateOscBankType, PYO_RUNTIME_TYPE_OSCBANK) < 0)
        return -1;
    if (module_add_heap_object(m, "SumOsc_base", PyoCreateSumOscType, PYO_RUNTIME_TYPE_SUMOSC) < 0)
        return -1;
    if (module_add_heap_object(m, "TableRead_base", PyoCreateTableReadType, PYO_RUNTIME_TYPE_TABLEREAD) < 0)
        return -1;
    if (module_add_heap_object(m, "Pulsar_base", PyoCreatePulsarType, PYO_RUNTIME_TYPE_PULSAR) < 0)
        return -1;
    if (module_add_heap_object(m, "Sine_base", PyoCreateSineType, PYO_RUNTIME_TYPE_SINE) < 0)
        return -1;
    if (module_add_heap_object(m, "FastSine_base", PyoCreateFastSineType, PYO_RUNTIME_TYPE_FASTSINE) < 0)
        return -1;
    if (module_add_heap_object(m, "SineLoop_base", PyoCreateSineLoopType, PYO_RUNTIME_TYPE_SINELOOP) < 0)
        return -1;
    if (module_add_heap_object(m, "Fm_base", PyoCreateFmType, PYO_RUNTIME_TYPE_FM) < 0)
        return -1;
    if (module_add_heap_object(m, "CrossFm_base", PyoCreateCrossFmType, PYO_RUNTIME_TYPE_CROSSFM) < 0)
        return -1;
    if (module_add_heap_object(m, "LFO_base", PyoCreateLFOType, PYO_RUNTIME_TYPE_LFO) < 0)
        return -1;
    if (module_add_heap_object(m, "Blit_base", PyoCreateBlitType, PYO_RUNTIME_TYPE_BLIT) < 0)
        return -1;
    if (module_add_heap_object(m, "Rossler_base", PyoCreateRosslerType, PYO_RUNTIME_TYPE_ROSSLER) < 0)
        return -1;
    if (module_add_heap_object(m, "RosslerAlt_base", PyoCreateRosslerAltType, PYO_RUNTIME_TYPE_ROSSLERALT) < 0)
        return -1;
    if (module_add_heap_object(m, "Lorenz_base", PyoCreateLorenzType, PYO_RUNTIME_TYPE_LORENZ) < 0)
        return -1;
    if (module_add_heap_object(m, "LorenzAlt_base", PyoCreateLorenzAltType, PYO_RUNTIME_TYPE_LORENZALT) < 0)
        return -1;
    if (module_add_heap_object(m, "ChenLee_base", PyoCreateChenLeeType, PYO_RUNTIME_TYPE_CHENLEE) < 0)
        return -1;
    if (module_add_heap_object(m, "ChenLeeAlt_base", PyoCreateChenLeeAltType, PYO_RUNTIME_TYPE_CHENLEEALT) < 0)
        return -1;
    if (module_add_heap_object(m, "Phasor_base", PyoCreatePhasorType, PYO_RUNTIME_TYPE_PHASOR) < 0)
        return -1;
    if (module_add_heap_object(m, "SuperSaw_base", PyoCreateSuperSawType, PYO_RUNTIME_TYPE_SUPERSAW) < 0)
        return -1;
    if (module_add_heap_object(m, "Pointer_base", PyoCreatePointerType, PYO_RUNTIME_TYPE_POINTER) < 0)
        return -1;
    if (module_add_heap_object(m, "TableIndex_base", PyoCreateTableIndexType, PYO_RUNTIME_TYPE_TABLEINDEX) < 0)
        return -1;
    if (module_add_heap_object(m, "Lookup_base", PyoCreateLookupType, PYO_RUNTIME_TYPE_LOOKUP) < 0)
        return -1;
    if (module_add_heap_object(m, "Noise_base", PyoCreateNoiseType, PYO_RUNTIME_TYPE_NOISE) < 0)
        return -1;
    if (module_add_heap_object(m, "PinkNoise_base", PyoCreatePinkNoiseType, PYO_RUNTIME_TYPE_PINKNOISE) < 0)
        return -1;
    if (module_add_heap_object(m, "BrownNoise_base", PyoCreateBrownNoiseType, PYO_RUNTIME_TYPE_BROWNNOISE) < 0)
        return -1;
    if (module_add_heap_object(m, "Biquad_base", PyoCreateBiquadType, PYO_RUNTIME_TYPE_BIQUAD) < 0)
        return -1;
    if (module_add_heap_object(m, "Biquadx_base", PyoCreateBiquadxType, PYO_RUNTIME_TYPE_BIQUADX) < 0)
        return -1;
    if (module_add_heap_object(m, "Biquada_base", PyoCreateBiquadaType, PYO_RUNTIME_TYPE_BIQUADA) < 0)
        return -1;
    if (module_add_heap_object(m, "EQ_base", PyoCreateEQType, PYO_RUNTIME_TYPE_EQ) < 0)
        return -1;
    if (module_add_heap_object(m, "Tone_base", PyoCreateToneType, PYO_RUNTIME_TYPE_TONE) < 0)
        return -1;
    if (module_add_heap_object(m, "Atone_base", PyoCreateAtoneType, PYO_RUNTIME_TYPE_ATONE) < 0)
        return -1;
    if (module_add_heap_object(m, "DCBlock_base", PyoCreateDCBlockType, PYO_RUNTIME_TYPE_DCBLOCK) < 0)
        return -1;
    if (module_add_heap_object(m, "Allpass_base", PyoCreateAllpassType, PYO_RUNTIME_TYPE_ALLPASS) < 0)
        return -1;
    if (module_add_heap_object(m, "Allpass2_base", PyoCreateAllpass2Type, PYO_RUNTIME_TYPE_ALLPASS2) < 0)
        return -1;
    if (module_add_heap_object(m, "Phaser_base", PyoCreatePhaserType, PYO_RUNTIME_TYPE_PHASER) < 0)
        return -1;
    if (module_add_heap_object(m, "Vocoder_base", PyoCreateVocoderType, PYO_RUNTIME_TYPE_VOCODER) < 0)
        return -1;
    if (module_add_heap_object(m, "Port_base", PyoCreatePortType, PYO_RUNTIME_TYPE_PORT) < 0)
        return -1;
    if (module_add_heap_object(m, "Denorm_base", PyoCreateDenormType, PYO_RUNTIME_TYPE_DENORM) < 0)
        return -1;
    if (module_add_heap_object(m, "Disto_base", PyoCreateDistoType, PYO_RUNTIME_TYPE_DISTO) < 0)
        return -1;
    if (module_add_heap_object(m, "Clip_base", PyoCreateClipType, PYO_RUNTIME_TYPE_CLIP) < 0)
        return -1;
    if (module_add_heap_object(m, "Mirror_base", PyoCreateMirrorType, PYO_RUNTIME_TYPE_MIRROR) < 0)
        return -1;
    if (module_add_heap_object(m, "Wrap_base", PyoCreateWrapType, PYO_RUNTIME_TYPE_WRAP) < 0)
        return -1;
    if (module_add_heap_object(m, "Between_base", PyoCreateBetweenType, PYO_RUNTIME_TYPE_BETWEEN) < 0)
        return -1;
    if (module_add_heap_object(m, "Degrade_base", PyoCreateDegradeType, PYO_RUNTIME_TYPE_DEGRADE) < 0)
        return -1;
    if (module_add_heap_object(m, "Compress_base", PyoCreateCompressType, PYO_RUNTIME_TYPE_COMPRESS) < 0)
        return -1;
    if (module_add_heap_object(m, "Gate_base", PyoCreateGateType, PYO_RUNTIME_TYPE_GATE) < 0)
        return -1;
    if (module_add_heap_object(m, "Balance_base", PyoCreateBalanceType, PYO_RUNTIME_TYPE_BALANCE) < 0)
        return -1;
    if (module_add_heap_object(m, "Delay_base", PyoCreateDelayType, PYO_RUNTIME_TYPE_DELAY) < 0)
        return -1;
    if (module_add_heap_object(m, "SDelay_base", PyoCreateSDelayType, PYO_RUNTIME_TYPE_SDELAY) < 0)
        return -1;
    if (module_add_heap_object(m, "Waveguide_base", PyoCreateWaveguideType, PYO_RUNTIME_TYPE_WAVEGUIDE) < 0)
        return -1;
    if (module_add_heap_object(m, "AllpassWG_base", PyoCreateAllpassWGType, PYO_RUNTIME_TYPE_ALLPASSWG) < 0)
        return -1;
    if (module_add_heap_object(m, "Midictl_base", PyoCreateMidictlType, PYO_RUNTIME_TYPE_MIDICTL) < 0)
        return -1;
    if (module_add_heap_object(m, "CtlScan_base", PyoCreateCtlScanType, PYO_RUNTIME_TYPE_CTLSCAN) < 0)
        return -1;
    if (module_add_heap_object(m, "CtlScan2_base", PyoCreateCtlScan2Type, PYO_RUNTIME_TYPE_CTLSCAN2) < 0)
        return -1;
    if (module_add_heap_object(m, "MidiNote_base", PyoCreateMidiNoteType, PYO_RUNTIME_TYPE_MIDINOTE) < 0)
        return -1;
    if (module_add_heap_object(m, "Notein_base", PyoCreateNoteinType, PYO_RUNTIME_TYPE_NOTEIN) < 0)
        return -1;
    if (module_add_heap_object(m, "NoteinTrig_base", PyoCreateNoteinTrigType, PYO_RUNTIME_TYPE_NOTEINTRIG) < 0)
        return -1;
    if (module_add_heap_object(m, "Bendin_base", PyoCreateBendinType, PYO_RUNTIME_TYPE_BENDIN) < 0)
        return -1;
    if (module_add_heap_object(m, "Touchin_base", PyoCreateTouchinType, PYO_RUNTIME_TYPE_TOUCHIN) < 0)
        return -1;
    if (module_add_heap_object(m, "Programin_base", PyoCreatePrograminType, PYO_RUNTIME_TYPE_PROGRAMIN) < 0)
        return -1;
    if (module_add_heap_object(m, "MidiAdsr_base", PyoCreateMidiAdsrType, PYO_RUNTIME_TYPE_MIDIADSR) < 0)
        return -1;
    if (module_add_heap_object(m, "MidiDelAdsr_base", PyoCreateMidiDelAdsrType, PYO_RUNTIME_TYPE_MIDIDELADSR) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigRand_base", PyoCreateTrigRandType, PYO_RUNTIME_TYPE_TRIGRAND) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigRandInt_base", PyoCreateTrigRandIntType, PYO_RUNTIME_TYPE_TRIGRANDINT) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigVal_base", PyoCreateTrigValType, PYO_RUNTIME_TYPE_TRIGVAL) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigChoice_base", PyoCreateTrigChoiceType, PYO_RUNTIME_TYPE_TRIGCHOICE) < 0)
        return -1;
    if (module_add_heap_object(m, "Iter_base", PyoCreateIterType, PYO_RUNTIME_TYPE_ITER) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigEnv_base", PyoCreateTrigEnvType, PYO_RUNTIME_TYPE_TRIGENV) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigLinseg_base", PyoCreateTrigLinsegType, PYO_RUNTIME_TYPE_TRIGLINSEG) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigExpseg_base", PyoCreateTrigExpsegType, PYO_RUNTIME_TYPE_TRIGEXPSEG) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigFunc_base", PyoCreateTrigFuncType, PYO_RUNTIME_TYPE_TRIGFUNC) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigXnoise_base", PyoCreateTrigXnoiseType, PYO_RUNTIME_TYPE_TRIGXNOISE) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigXnoiseMidi_base", PyoCreateTrigXnoiseMidiType, PYO_RUNTIME_TYPE_TRIGXNOISEMIDI) < 0)
        return -1;
    if (module_add_heap_object(m, "Pattern_base", PyoCreatePatternType, PYO_RUNTIME_TYPE_PATTERN) < 0)
        return -1;
    if (module_add_heap_object(m, "CallAfter_base", PyoCreateCallAfterType, PYO_RUNTIME_TYPE_CALLAFTER) < 0)
        return -1;
    if (module_add_heap_object(m, "CallAlways_base", PyoCreateCallAlwaysType, PYO_RUNTIME_TYPE_CALLALWAYS) < 0)
        return -1;
    if (module_add_heap_object(m, "BandSplitter_base", PyoCreateBandSplitterType, PYO_RUNTIME_TYPE_BANDSPLITTER) < 0)
        return -1;
    if (module_add_heap_object(m, "BandSplit_base", PyoCreateBandSplitType, PYO_RUNTIME_TYPE_BANDSPLIT) < 0)
        return -1;
    if (module_add_heap_object(m, "FourBandMain_base", PyoCreateFourBandMainType, PYO_RUNTIME_TYPE_FOURBAND_MAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "FourBand_base", PyoCreateFourBandType, PYO_RUNTIME_TYPE_FOURBAND) < 0)
        return -1;
    if (module_add_heap_object(m, "HilbertMain_base", PyoCreateHilbertMainType, PYO_RUNTIME_TYPE_HILBERTMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "Hilbert_base", PyoCreateHilbertType, PYO_RUNTIME_TYPE_HILBERT) < 0)
        return -1;
    if (module_add_heap_object(m, "Follower_base", PyoCreateFollowerType, PYO_RUNTIME_TYPE_FOLLOWER) < 0)
        return -1;
    if (module_add_heap_object(m, "Follower2_base", PyoCreateFollower2Type, PYO_RUNTIME_TYPE_FOLLOWER2) < 0)
        return -1;
    if (module_add_heap_object(m, "ZCross_base", PyoCreateZCrossType, PYO_RUNTIME_TYPE_ZCROSS) < 0)
        return -1;
    if (module_add_heap_object(m, "SPanner_base", PyoCreateSPannerType, PYO_RUNTIME_TYPE_SPANNER) < 0)
        return -1;
    if (module_add_heap_object(m, "Panner_base", PyoCreatePannerType, PYO_RUNTIME_TYPE_PANNER) < 0)
        return -1;
    if (module_add_heap_object(m, "Pan_base", PyoCreatePanType, PYO_RUNTIME_TYPE_PAN) < 0)
        return -1;
    if (module_add_heap_object(m, "SPan_base", PyoCreateSPanType, PYO_RUNTIME_TYPE_SPAN) < 0)
        return -1;
    if (module_add_heap_object(m, "Switcher_base", PyoCreateSwitcherType, PYO_RUNTIME_TYPE_SWITCHER) < 0)
        return -1;
    if (module_add_heap_object(m, "Switch_base", PyoCreateSwitchType, PYO_RUNTIME_TYPE_SWITCH) < 0)
        return -1;
    if (module_add_heap_object(m, "Selector_base", PyoCreateSelectorType, PYO_RUNTIME_TYPE_SELECTOR) < 0)
        return -1;
    if (module_add_heap_object(m, "VoiceManager_base", PyoCreateVoiceManagerType, PYO_RUNTIME_TYPE_VOICEMANAGER) < 0)
        return -1;
    if (module_add_heap_object(m, "Mixer_base", PyoCreateMixerType, PYO_RUNTIME_TYPE_MIXER) < 0)
        return -1;
    if (module_add_heap_object(m, "MixerVoice_base", PyoCreateMixerVoiceType, PYO_RUNTIME_TYPE_MIXERVOICE) < 0)
        return -1;
    if (module_add_heap_object(m, "Counter_base", PyoCreateCounterType, PYO_RUNTIME_TYPE_COUNTER) < 0)
        return -1;
    if (module_add_heap_object(m, "Count_base", PyoCreateCountType, PYO_RUNTIME_TYPE_COUNTOBJ) < 0)
        return -1;
    if (module_add_heap_object(m, "Thresh_base", PyoCreateThreshType, PYO_RUNTIME_TYPE_THRESH) < 0)
        return -1;
    if (module_add_heap_object(m, "Percent_base", PyoCreatePercentType, PYO_RUNTIME_TYPE_PERCENT) < 0)
        return -1;
    if (module_add_heap_object(m, "Timer_base", PyoCreateTimerType, PYO_RUNTIME_TYPE_TIMER) < 0)
        return -1;
    if (module_add_heap_object(m, "Select_base", PyoCreateSelectType, PYO_RUNTIME_TYPE_SELECT) < 0)
        return -1;
    if (module_add_heap_object(m, "Change_base", PyoCreateChangeType, PYO_RUNTIME_TYPE_CHANGE) < 0)
        return -1;
    if (module_add_heap_object(m, "Score_base", PyoCreateScoreType, PYO_RUNTIME_TYPE_SCORE) < 0)
        return -1;
    if (module_add_heap_object(m, "Freeverb_base", PyoCreateFreeverbType, PYO_RUNTIME_TYPE_FREEVERB) < 0)
        return -1;
    if (module_add_heap_object(m, "WGVerb_base", PyoCreateWGVerbType, PYO_RUNTIME_TYPE_WGVERB) < 0)
        return -1;
    if (module_add_heap_object(m, "Chorus_base", PyoCreateChorusType, PYO_RUNTIME_TYPE_CHORUS) < 0)
        return -1;
    if (module_add_heap_object(m, "Convolve_base", PyoCreateConvolveType, PYO_RUNTIME_TYPE_CONVOLVE) < 0)
        return -1;
    if (module_add_heap_object(m, "IRWinSinc_base", PyoCreateIRWinSincType, PYO_RUNTIME_TYPE_IRWINSINC) < 0)
        return -1;
    if (module_add_heap_object(m, "IRPulse_base", PyoCreateIRPulseType, PYO_RUNTIME_TYPE_IRPULSE) < 0)
        return -1;
    if (module_add_heap_object(m, "IRAverage_base", PyoCreateIRAverageType, PYO_RUNTIME_TYPE_IRAVERAGE) < 0)
        return -1;
    if (module_add_heap_object(m, "IRFM_base", PyoCreateIRFMType, PYO_RUNTIME_TYPE_IRFM) < 0)
        return -1;
    if (module_add_heap_object(m, "Granulator_base", PyoCreateGranulatorType, PYO_RUNTIME_TYPE_GRANULATOR) < 0)
        return -1;
    if (module_add_heap_object(m, "Looper_base", PyoCreateLooperType, PYO_RUNTIME_TYPE_LOOPER) < 0)
        return -1;
    if (module_add_heap_object(m, "LooperTimeStream_base", PyoCreateLooperTimeStreamType, PYO_RUNTIME_TYPE_LOOPERTIMESTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "Harmonizer_base", PyoCreateHarmonizerType, PYO_RUNTIME_TYPE_HARMONIZER) < 0)
        return -1;
    if (module_add_heap_object(m, "Print_base", PyoCreatePrintType, PYO_RUNTIME_TYPE_PRINT) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Sin_base", PyoCreateMSinType, PYO_RUNTIME_TYPE_M_SIN) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Cos_base", PyoCreateMCosType, PYO_RUNTIME_TYPE_M_COS) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Tan_base", PyoCreateMTanType, PYO_RUNTIME_TYPE_M_TAN) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Abs_base", PyoCreateMAbsType, PYO_RUNTIME_TYPE_M_ABS) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Sqrt_base", PyoCreateMSqrtType, PYO_RUNTIME_TYPE_M_SQRT) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Log_base", PyoCreateMLogType, PYO_RUNTIME_TYPE_M_LOG) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Log2_base", PyoCreateMLog2Type, PYO_RUNTIME_TYPE_M_LOG2) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Log10_base", PyoCreateMLog10Type, PYO_RUNTIME_TYPE_M_LOG10) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Pow_base", PyoCreateMPowType, PYO_RUNTIME_TYPE_M_POW) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Atan2_base", PyoCreateMAtan2Type, PYO_RUNTIME_TYPE_M_ATAN2) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Floor_base", PyoCreateMFloorType, PYO_RUNTIME_TYPE_M_FLOOR) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Ceil_base", PyoCreateMCeilType, PYO_RUNTIME_TYPE_M_CEIL) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Round_base", PyoCreateMRoundType, PYO_RUNTIME_TYPE_M_ROUND) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Tanh_base", PyoCreateMTanhType, PYO_RUNTIME_TYPE_M_TANH) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Exp_base", PyoCreateMExpType, PYO_RUNTIME_TYPE_M_EXP) < 0)
        return -1;
    if (module_add_heap_object(m, "Snap_base", PyoCreateSnapType, PYO_RUNTIME_TYPE_SNAP) < 0)
        return -1;
    if (module_add_heap_object(m, "Interp_base", PyoCreateInterpType, PYO_RUNTIME_TYPE_INTERP) < 0)
        return -1;
    if (module_add_heap_object(m, "SampHold_base", PyoCreateSampHoldType, PYO_RUNTIME_TYPE_SAMPHOLD) < 0)
        return -1;
    if (module_add_heap_object(m, "DBToA_base", PyoCreateDBToAType, PYO_RUNTIME_TYPE_DBTOA) < 0)
        return -1;
    if (module_add_heap_object(m, "AToDB_base", PyoCreateAToDBType, PYO_RUNTIME_TYPE_ATODB) < 0)
        return -1;
    if (module_add_heap_object(m, "Scale_base", PyoCreateScaleType, PYO_RUNTIME_TYPE_SCALE) < 0)
        return -1;
    if (module_add_heap_object(m, "CentsToTranspo_base", PyoCreateCentsToTranspoType, PYO_RUNTIME_TYPE_CENTSTOTRANSPO) < 0)
        return -1;
    if (module_add_heap_object(m, "TranspoToCents_base", PyoCreateTranspoToCentsType, PYO_RUNTIME_TYPE_TRANSPOTOCENTS) < 0)
        return -1;
    if (module_add_heap_object(m, "MToF_base", PyoCreateMToFType, PYO_RUNTIME_TYPE_MTOF) < 0)
        return -1;
    if (module_add_heap_object(m, "FToM_base", PyoCreateFToMType, PYO_RUNTIME_TYPE_FTOM) < 0)
        return -1;
    if (module_add_heap_object(m, "MToT_base", PyoCreateMToTType, PYO_RUNTIME_TYPE_MTOT) < 0)
        return -1;
    if (module_add_heap_object(m, "FFTMain_base", PyoCreateFFTMainType, PYO_RUNTIME_TYPE_FFTMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "FFT_base", PyoCreateFFTType, PYO_RUNTIME_TYPE_FFT) < 0)
        return -1;
    if (module_add_heap_object(m, "IFFT_base", PyoCreateIFFTType, PYO_RUNTIME_TYPE_IFFT) < 0)
        return -1;
    if (module_add_heap_object(m, "IFFTMatrix_base", PyoCreateIFFTMatrixType, PYO_RUNTIME_TYPE_IFFTMATRIX) < 0)
        return -1;
    if (module_add_heap_object(m, "CarToPol_base", PyoCreateCarToPolType, PYO_RUNTIME_TYPE_CARTOPOL) < 0)
        return -1;
    if (module_add_heap_object(m, "PolToCar_base", PyoCreatePolToCarType, PYO_RUNTIME_TYPE_POLTOCAR) < 0)
        return -1;
    if (module_add_heap_object(m, "FrameDeltaMain_base", PyoCreateFrameDeltaMainType, PYO_RUNTIME_TYPE_FRAMEDELTAMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "FrameDelta_base", PyoCreateFrameDeltaType, PYO_RUNTIME_TYPE_FRAMEDELTA) < 0)
        return -1;
    if (module_add_heap_object(m, "FrameAccum_base", PyoCreateFrameAccumType, PYO_RUNTIME_TYPE_FRAMEACCUM) < 0)
        return -1;
    if (module_add_heap_object(m, "FrameAccumMain_base", PyoCreateFrameAccumMainType, PYO_RUNTIME_TYPE_FRAMEACCUMMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "VectralMain_base", PyoCreateVectralMainType, PYO_RUNTIME_TYPE_VECTRALMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "Vectral_base", PyoCreateVectralType, PYO_RUNTIME_TYPE_VECTRAL) < 0)
        return -1;
    if (module_add_heap_object(m, "Min_base", PyoCreateMinType, PYO_RUNTIME_TYPE_MIN) < 0)
        return -1;
    if (module_add_heap_object(m, "Max_base", PyoCreateMaxType, PYO_RUNTIME_TYPE_MAX) < 0)
        return -1;
    if (module_add_heap_object(m, "Delay1_base", PyoCreateDelay1Type, PYO_RUNTIME_TYPE_DELAY1) < 0)
        return -1;
    if (module_add_heap_object(m, "RCOsc_base", PyoCreateRCOscType, PYO_RUNTIME_TYPE_RCOSC) < 0)
        return -1;
    if (module_add_heap_object(m, "Yin_base", PyoCreateYinType, PYO_RUNTIME_TYPE_YIN) < 0)
        return -1;
    if (module_add_heap_object(m, "SVF_base", PyoCreateSVFType, PYO_RUNTIME_TYPE_SVF) < 0)
        return -1;
    if (module_add_heap_object(m, "SVF2_base", PyoCreateSVF2Type, PYO_RUNTIME_TYPE_SVF2) < 0)
        return -1;
    if (module_add_heap_object(m, "Average_base", PyoCreateAverageType, PYO_RUNTIME_TYPE_AVERAGE) < 0)
        return -1;
    if (module_add_heap_object(m, "CvlVerb_base", PyoCreateCvlVerbType, PYO_RUNTIME_TYPE_CVLVERB) < 0)
        return -1;
    if (module_add_heap_object(m, "Spectrum_base", PyoCreateSpectrumType, PYO_RUNTIME_TYPE_SPECTRUM) < 0)
        return -1;
    if (module_add_heap_object(m, "Reson_base", PyoCreateResonType, PYO_RUNTIME_TYPE_RESON) < 0)
        return -1;
    if (module_add_heap_object(m, "Resonx_base", PyoCreateResonxType, PYO_RUNTIME_TYPE_RESONX) < 0)
        return -1;
    if (module_add_heap_object(m, "ButLP_base", PyoCreateButLPType, PYO_RUNTIME_TYPE_BUTLP) < 0)
        return -1;
    if (module_add_heap_object(m, "ButHP_base", PyoCreateButHPType, PYO_RUNTIME_TYPE_BUTHP) < 0)
        return -1;
    if (module_add_heap_object(m, "ButBP_base", PyoCreateButBPType, PYO_RUNTIME_TYPE_BUTBP) < 0)
        return -1;
    if (module_add_heap_object(m, "ButBR_base", PyoCreateButBRType, PYO_RUNTIME_TYPE_BUTBR) < 0)
        return -1;
    if (module_add_heap_object(m, "MoogLP_base", PyoCreateMoogLPType, PYO_RUNTIME_TYPE_MOOGLP) < 0)
        return -1;
    if (module_add_heap_object(m, "PVAnal_base", PyoCreatePVAnalType, PYO_RUNTIME_TYPE_PVANAL) < 0)
        return -1;
    if (module_add_heap_object(m, "PVSynth_base", PyoCreatePVSynthType, PYO_RUNTIME_TYPE_PVSYNTH) < 0)
        return -1;
    if (module_add_heap_object(m, "PVTranspose_base", PyoCreatePVTransposeType, PYO_RUNTIME_TYPE_PVTRANSPOSE) < 0)
        return -1;
    if (module_add_heap_object(m, "PVVerb_base", PyoCreatePVVerbType, PYO_RUNTIME_TYPE_PVVERB) < 0)
        return -1;
    if (module_add_heap_object(m, "PVGate_base", PyoCreatePVGateType, PYO_RUNTIME_TYPE_PVGATE) < 0)
        return -1;
    if (module_add_heap_object(m, "PVAddSynth_base", PyoCreatePVAddSynthType, PYO_RUNTIME_TYPE_PVADDSYNTH) < 0)
        return -1;
    if (module_add_heap_object(m, "PVCross_base", PyoCreatePVCrossType, PYO_RUNTIME_TYPE_PVCROSS) < 0)
        return -1;
    if (module_add_heap_object(m, "PVMult_base", PyoCreatePVMultType, PYO_RUNTIME_TYPE_PVMULT) < 0)
        return -1;
    if (module_add_heap_object(m, "PVMorph_base", PyoCreatePVMorphType, PYO_RUNTIME_TYPE_PVMORPH) < 0)
        return -1;
    if (module_add_heap_object(m, "PVFilter_base", PyoCreatePVFilterType, PYO_RUNTIME_TYPE_PVFILTER) < 0)
        return -1;
    if (module_add_heap_object(m, "PVDelay_base", PyoCreatePVDelayType, PYO_RUNTIME_TYPE_PVDELAY) < 0)
        return -1;
    if (module_add_heap_object(m, "PVBuffer_base", PyoCreatePVBufferType, PYO_RUNTIME_TYPE_PVBUFFER) < 0)
        return -1;
    if (module_add_heap_object(m, "PVShift_base", PyoCreatePVShiftType, PYO_RUNTIME_TYPE_PVSHIFT) < 0)
        return -1;
    if (module_add_heap_object(m, "PVAmpMod_base", PyoCreatePVAmpModType, PYO_RUNTIME_TYPE_PVAMPMOD) < 0)
        return -1;
    if (module_add_heap_object(m, "PVFreqMod_base", PyoCreatePVFreqModType, PYO_RUNTIME_TYPE_PVFREQMOD) < 0)
        return -1;
    if (module_add_heap_object(m, "PVBufLoops_base", PyoCreatePVBufLoopsType, PYO_RUNTIME_TYPE_PVBUFLOOPS) < 0)
        return -1;
    if (module_add_heap_object(m, "PVBufTabLoops_base", PyoCreatePVBufTabLoopsType, PYO_RUNTIME_TYPE_PVBUFTABLOOPS) < 0)
        return -1;
    if (module_add_heap_object(m, "PVMix_base", PyoCreatePVMixType, PYO_RUNTIME_TYPE_PVMIX) < 0)
        return -1;
    if (module_add_heap_object(m, "Granule_base", PyoCreateGranuleType, PYO_RUNTIME_TYPE_GRANULE) < 0)
        return -1;
    if (module_add_heap_object(m, "TableScale_base", PyoCreateTableScaleType, PYO_RUNTIME_TYPE_TABLESCALE) < 0)
        return -1;
    if (module_add_heap_object(m, "TrackHold_base", PyoCreateTrackHoldType, PYO_RUNTIME_TYPE_TRACKHOLD) < 0)
        return -1;
    if (module_add_heap_object(m, "ComplexRes_base", PyoCreateComplexResType, PYO_RUNTIME_TYPE_COMPLEXRES) < 0)
        return -1;
    if (module_add_heap_object(m, "STReverb_base", PyoCreateSTReverbType, PYO_RUNTIME_TYPE_STREVERB) < 0)
        return -1;
    if (module_add_heap_object(m, "STRev_base", PyoCreateSTRevType, PYO_RUNTIME_TYPE_STREV) < 0)
        return -1;
    if (module_add_heap_object(m, "Pointer2_base", PyoCreatePointer2Type, PYO_RUNTIME_TYPE_POINTER2) < 0)
        return -1;
    if (module_add_heap_object(m, "Centroid_base", PyoCreateCentroidType, PYO_RUNTIME_TYPE_CENTROID) < 0)
        return -1;
    if (module_add_heap_object(m, "AttackDetector_base", PyoCreateAttackDetectorType, PYO_RUNTIME_TYPE_ATTACK_DETECTOR) < 0)
        return -1;
    if (module_add_heap_object(m, "SmoothDelay_base", PyoCreateSmoothDelayType, PYO_RUNTIME_TYPE_SMOOTHDELAY) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurster_base", PyoCreateTrigBursterType, PYO_RUNTIME_TYPE_TRIGBURSTER) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurst_base", PyoCreateTrigBurstType, PYO_RUNTIME_TYPE_TRIGBURST) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurstTapStream_base", PyoCreateTrigBurstTapStreamType, PYO_RUNTIME_TYPE_TRIGBURSTTAPSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurstAmpStream_base", PyoCreateTrigBurstAmpStreamType, PYO_RUNTIME_TYPE_TRIGBURSTAMPSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurstDurStream_base", PyoCreateTrigBurstDurStreamType, PYO_RUNTIME_TYPE_TRIGBURSTDURSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "TrigBurstEndStream_base", PyoCreateTrigBurstEndStreamType, PYO_RUNTIME_TYPE_TRIGBURSTENDSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "Scope_base", PyoCreateScopeType, PYO_RUNTIME_TYPE_SCOPE) < 0)
        return -1;
    if (module_add_heap_object(m, "PeakAmp_base", PyoCreatePeakAmpType, PYO_RUNTIME_TYPE_PEAKAMP) < 0)
        return -1;
    if (module_add_heap_object(m, "MainParticle_base", PyoCreateMainParticleType, PYO_RUNTIME_TYPE_MAINPARTICLE) < 0)
        return -1;
    if (module_add_heap_object(m, "Particle_base", PyoCreateParticleType, PYO_RUNTIME_TYPE_PARTICLE) < 0)
        return -1;
    if (module_add_heap_object(m, "MainParticle2_base", PyoCreateMainParticle2Type, PYO_RUNTIME_TYPE_MAINPARTICLE2) < 0)
        return -1;
    if (module_add_heap_object(m, "Particle2_base", PyoCreateParticle2Type, PYO_RUNTIME_TYPE_PARTICLE2) < 0)
        return -1;
    if (module_add_heap_object(m, "AtanTable_base", PyoCreateAtanTableType, PYO_RUNTIME_TYPE_ATANTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "RawMidi_base", PyoCreateRawMidiType, PYO_RUNTIME_TYPE_RAWMIDI) < 0)
        return -1;
    if (module_add_heap_object(m, "Resample_base", PyoCreateResampleType, PYO_RUNTIME_TYPE_RESAMPLE) < 0)
        return -1;
    if (module_add_heap_object(m, "Exprer_base", PyoCreateExprerType, PYO_RUNTIME_TYPE_EXPRER) < 0)
        return -1;
    if (module_add_heap_object(m, "Expr_base", PyoCreateExprType, PYO_RUNTIME_TYPE_EXPR) < 0)
        return -1;
    if (module_add_heap_object(m, "PadSynthTable_base", PyoCreatePadSynthTableType, PYO_RUNTIME_TYPE_PADSYNTHTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "LogiMap_base", PyoCreateLogiMapType, PYO_RUNTIME_TYPE_LOGIMAP) < 0)
        return -1;
    if (module_add_heap_object(m, "SharedTable_base", PyoCreateSharedTableType, PYO_RUNTIME_TYPE_SHAREDTABLE) < 0)
        return -1;
    if (module_add_heap_object(m, "TableFill_base", PyoCreateTableFillType, PYO_RUNTIME_TYPE_TABLEFILL) < 0)
        return -1;
    if (module_add_heap_object(m, "TableScan_base", PyoCreateTableScanType, PYO_RUNTIME_TYPE_TABLESCAN) < 0)
        return -1;
    if (module_add_heap_object(m, "HRTFData_base", PyoCreateHRTFDataType, PYO_RUNTIME_TYPE_HRTFDATA) < 0)
        return -1;
    if (module_add_heap_object(m, "HRTFSpatter_base", PyoCreateHRTFSpatterType, PYO_RUNTIME_TYPE_HRTFSPATTER) < 0)
        return -1;
    if (module_add_heap_object(m, "HRTF_base", PyoCreateHRTFType, PYO_RUNTIME_TYPE_HRTF) < 0)
        return -1;
    if (module_add_heap_object(m, "Expand_base", PyoCreateExpandType, PYO_RUNTIME_TYPE_EXPAND) < 0)
        return -1;
    if (module_add_heap_object(m, "RMS_base", PyoCreateRMSType, PYO_RUNTIME_TYPE_RMS) < 0)
        return -1;
    if (module_add_heap_object(m, "MidiLinseg_base", PyoCreateMidiLinsegType, PYO_RUNTIME_TYPE_MIDILINSEG) < 0)
        return -1;
    if (module_add_heap_object(m, "MultiBandMain_base", PyoCreateMultiBandMainType, PYO_RUNTIME_TYPE_MULTIBAND_MAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "MultiBand_base", PyoCreateMultiBandType, PYO_RUNTIME_TYPE_MULTIBAND) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Div_base", PyoCreateMDivType, PYO_RUNTIME_TYPE_M_DIV) < 0)
        return -1;
    if (module_add_heap_object(m, "M_Sub_base", PyoCreateMSubType, PYO_RUNTIME_TYPE_M_SUB) < 0)
        return -1;
    if (module_add_heap_object(m, "Binauraler_base", PyoCreateBinauralerType, PYO_RUNTIME_TYPE_BINAURALER) < 0)
        return -1;
    if (module_add_heap_object(m, "Binaural_base", PyoCreateBinauralType, PYO_RUNTIME_TYPE_BINAURAL) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLMain_base", PyoCreateMMLMainType, PYO_RUNTIME_TYPE_MMLMAIN) < 0)
        return -1;
    if (module_add_heap_object(m, "MML_base", PyoCreateMMLType, PYO_RUNTIME_TYPE_MML) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLFreqStream_base", PyoCreateMMLFreqStreamType, PYO_RUNTIME_TYPE_MMLFREQSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLAmpStream_base", PyoCreateMMLAmpStreamType, PYO_RUNTIME_TYPE_MMLAMPSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLDurStream_base", PyoCreateMMLDurStreamType, PYO_RUNTIME_TYPE_MMLDURSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLEndStream_base", PyoCreateMMLEndStreamType, PYO_RUNTIME_TYPE_MMLENDSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLXStream_base", PyoCreateMMLXStreamType, PYO_RUNTIME_TYPE_MMLXSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLYStream_base", PyoCreateMMLYStreamType, PYO_RUNTIME_TYPE_MMLYSTREAM) < 0)
        return -1;
    if (module_add_heap_object(m, "MMLZStream_base", PyoCreateMMLZStreamType, PYO_RUNTIME_TYPE_MMLZSTREAM) < 0)
        return -1;

    if (PyModule_AddStringConstant(m, "PYO_VERSION", PYO_VERSION) < 0)
        return -1;
#ifdef COMPILE_EXTERNALS
    EXTERNAL_OBJECTS
    if (PyModule_AddIntConstant(m, "WITH_EXTERNALS", 1) < 0)
        return -1;
#else
    if (PyModule_AddIntConstant(m, "WITH_EXTERNALS", 0) < 0)
        return -1;
#endif
#ifndef USE_DOUBLE
    if (PyModule_AddIntConstant(m, "USE_DOUBLE", 0) < 0)
        return -1;
#else
    if (PyModule_AddIntConstant(m, "USE_DOUBLE", 1) < 0)
        return -1;
#endif

    return 0;
}

static void
pyo_free(void *module)
{
    PyoState_ClearTypes((PyoModuleState *)PyModule_GetState((PyObject *)module));
}

static PyModuleDef_Slot pyo_module_slots[] =
{
    {Py_mod_exec, pyo_exec},
#if PY_VERSION_HEX >= 0x030c00f0  // Python 3.12+
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED},
#endif
#if PY_VERSION_HEX >= 0x030d00f0  // Python 3.13+
    // signal that this module does not supports running without an active GIL
    {Py_mod_gil, Py_MOD_GIL_USED},
#endif
    {0, NULL}
};

static struct PyModuleDef pyo_moduledef =
{
    PyModuleDef_HEAD_INIT,
    .m_name = LIB_BASE_NAME,
    .m_doc = "Python digital signal processing module.",
    .m_size = sizeof(PyoModuleState),
    .m_methods = pyo_functions,
    .m_slots = pyo_module_slots,
    .m_free = pyo_free,
};

PyMODINIT_FUNC
#ifndef USE_DOUBLE
PyInit__pyo(void)
#else
PyInit__pyo64(void)
#endif
{
    return PyModuleDef_Init(&pyo_moduledef);
}
