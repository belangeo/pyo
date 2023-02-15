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

#include "ad_coreaudio.h"

#if !defined(MAC_OS_VERSION_12_0) || \
    (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_VERSION_12_0)
#define kAudioObjectPropertyElementMain kAudioObjectPropertyElementMaster
#endif

OSStatus coreaudio_input_callback(AudioDeviceID device, const AudioTimeStamp* inNow,
                                  const AudioBufferList* inInputData,
                                  const AudioTimeStamp* inInputTime,
                                  AudioBufferList* outOutputData,
                                  const AudioTimeStamp* inOutputTime,
                                  void* defptr)
{
    int i, j, bufchnls, servchnls, off1chnls, off2chnls;
    Server *server = (Server *) defptr;
    (void) outOutputData;
    const AudioBuffer* inputBuf = inInputData->mBuffers;
    float *bufdata = (float*)inputBuf->mData;
    bufchnls = inputBuf->mNumberChannels;
    servchnls = server->ichnls < bufchnls ? server->ichnls : bufchnls;

    for (i = 0; i < server->bufferSize; i++)
    {
        off1chnls = i * bufchnls + server->input_offset;
        off2chnls = i * servchnls;

        for (j = 0; j < servchnls; j++)
        {
            server->input_buffer[off2chnls + j] = (MYFLT)bufdata[off1chnls + j];
        }
    }

    return kAudioHardwareNoError;
}

OSStatus coreaudio_output_callback(AudioDeviceID device, const AudioTimeStamp* inNow,
                                   const AudioBufferList* inInputData,
                                   const AudioTimeStamp* inInputTime,
                                   AudioBufferList* outOutputData,
                                   const AudioTimeStamp* inOutputTime,
                                   void* defptr)
{
    int i, j, bufchnls, servchnls, off1chnls, off2chnls;
    Server *server = (Server *) defptr;

    (void) inInputData;

    if (server->withPortMidi == 1)
    {
        pyoGetMidiEvents(server);
    }

    Server_process_buffers(server);
    pthread_mutex_lock(&server->buf_mutex);
    AudioBuffer* outputBuf = outOutputData->mBuffers;
    bufchnls = outputBuf->mNumberChannels;
    servchnls = server->nchnls < bufchnls ? server->nchnls : bufchnls;
    float *bufdata = (float*)outputBuf->mData;

    for (i = 0; i < server->bufferSize; i++)
    {
        off1chnls = i * bufchnls + server->output_offset;
        off2chnls = i * servchnls;

        for (j = 0; j < servchnls; j++)
        {
            bufdata[off1chnls + j] = server->output_buffer[off2chnls + j];
        }
    }

    server->midi_count = 0;
    pthread_cond_signal(&server->buf_cond);
    pthread_mutex_unlock(&server->buf_mutex);

    return kAudioHardwareNoError;
}

int
coreaudio_stop_callback(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    err = AudioDeviceStop(self->output, self->outprocid);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Output AudioDeviceStop failed %d\n", (int)err);
        return -1;
    }

    if (self->duplex == 1)
    {
        err = AudioDeviceStop(self->input, self->inprocid);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Input AudioDeviceStop failed %d\n", (int)err);
            return -1;
        }
    }

    self->server_started = 0;
    return 0;
}

int
Server_coreaudio_init(Server *self)
{
    OSStatus err = kAudioHardwareNoError;
    UInt32 count, i, numdevices;
    char *name;
    AudioDeviceID mOutputDevice = kAudioDeviceUnknown;
    AudioDeviceID mInputDevice = kAudioDeviceUnknown;
    Boolean writable;
    AudioTimeStamp now;
    AudioObjectPropertyAddress property_address;
    UInt32 propertysize = 256;

    property_address.mSelector = kAudioHardwarePropertyDevices;
    property_address.mScope = kAudioObjectPropertyScopeGlobal;
    property_address.mElement = kAudioObjectPropertyElementMain;

    now.mFlags = kAudioTimeStampHostTimeValid;
    now.mHostTime = AudioGetCurrentHostTime();

    /* create mutex */
    err = pthread_mutex_init(&self->buf_mutex, NULL);
    if (err) {
        Server_error(self, "Could not create mutex\nReason: %s\n", (char*)&err);
        return -1;
    }
    /* create mutex condition*/
    err = pthread_cond_init(&self->buf_cond, NULL);
    if (err) {
        Server_error(self, "Could not create mutex condition\nReason: %s\n", (char*)&err);
        return -1;
    }

    /************************************/
    /* List Coreaudio available devices */
    /************************************/
    err = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &property_address, 0, NULL, &count);
    if (err)
    {
        Server_error(self, "Get data size error for kAudioObjectSystemObject.");
    }

    AudioDeviceID *devices = (AudioDeviceID*) PyMem_RawMalloc(count);
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, NULL, &count, devices);
    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Get kAudioHardwarePropertyDevices error %s\n", (char*)&err);
        PyMem_RawFree(devices);
    }

    numdevices = count / sizeof(AudioDeviceID);
    Server_debug(self, "Coreaudio : Number of devices: %i\n", numdevices);

    property_address.mSelector = kAudioDevicePropertyDeviceName;
    for (i = 0; i < numdevices; ++i)
    {
        name = (char*)PyMem_RawMalloc(256);
        propertysize = 256;
        err = AudioObjectGetPropertyData(devices[i], &property_address, 0, NULL, &propertysize, name);
        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            PyMem_RawFree(name);
            break;
        }

        Server_debug(self, "   %d : \"%s\"\n", i, name);
        PyMem_RawFree(name);
    }

    /************************************/
    /* Acquire input and output devices */
    /************************************/
    /* Acquire input audio device */
    if (self->duplex == 1)
    {
        if (self->input != -1)
            mInputDevice = devices[self->input];

        if (mInputDevice == kAudioDeviceUnknown)
        {
            count = sizeof(mInputDevice);
            property_address.mSelector = kAudioHardwarePropertyDefaultInputDevice;
            err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, 0, &count, &mInputDevice);

            if (err != kAudioHardwareNoError)
            {
                Server_error(self, "Get kAudioHardwarePropertyDefaultInputDevice error %s\n", (char*)&err);
                PyMem_RawFree(devices);
                return -1;
            }
        }
        property_address.mSelector = kAudioDevicePropertyDeviceName;
        name = (char*)PyMem_RawMalloc(256);
        propertysize = 256;
        err = AudioObjectGetPropertyData(mInputDevice, &property_address, 0, NULL, &propertysize, name);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }

        Server_debug(self, "Coreaudio : Uses input device : \"%s\"\n", name);
        self->input = mInputDevice;

        PyMem_RawFree(name);
    }

    /* Acquire output audio device */
    if (self->output != -1)
        mOutputDevice = devices[self->output];

    if (mOutputDevice == kAudioDeviceUnknown)
    {
        count = sizeof(mOutputDevice);
        property_address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &property_address, 0, 0, &count, &mOutputDevice);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Get kAudioHardwarePropertyDefaultOutputDevice error %s\n", (char*)&err);
            PyMem_RawFree(devices);
            return -1;
        }
    }

    property_address.mSelector = kAudioDevicePropertyDeviceName;
    name = (char*)PyMem_RawMalloc(256);
    propertysize = 256;
    err = AudioObjectGetPropertyData(mOutputDevice, &property_address, 0, NULL, &propertysize, name);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }

    Server_debug(self, "Coreaudio : Uses output device : \"%s\"\n", name);
    self->output = mOutputDevice;

    PyMem_RawFree(name);
    PyMem_RawFree(devices);

    /*************************************************/
    /* Get in/out buffer frame and buffer frame size */
    /*************************************************/
    UInt32 bufferSize;
    AudioValueRange range;
    Float64 sampleRate;

    /* Get input device buffer frame size and buffer frame size range */
    if (self->duplex == 1)
    {
        count = sizeof(UInt32);
        property_address.mSelector = kAudioDevicePropertyBufferFrameSize;
        err = AudioObjectGetPropertyData(mInputDevice, &property_address, 0, NULL, &count, &bufferSize);

        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);

        Server_debug(self, "Coreaudio : Coreaudio input device buffer size = %ld\n", bufferSize);

        count = sizeof(AudioValueRange);
        property_address.mSelector = kAudioDevicePropertyBufferSizeRange;
        err = AudioObjectGetPropertyData(mInputDevice, &property_address, 0, NULL, &count, &range);

        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);

        Server_debug(self, "Coreaudio : Coreaudio input device buffer size range = %f -> %f\n", range.mMinimum, range.mMaximum);

        /* Get input device sampling rate */
        count = sizeof(Float64);
        property_address.mSelector = kAudioDevicePropertyNominalSampleRate;
        err = AudioObjectGetPropertyData(mInputDevice, &property_address, 0, NULL, &count, &sampleRate);

        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);

        Server_debug(self, "Coreaudio : Coreaudio input device sampling rate = %.2f\n", sampleRate);
    }

    /* Get output device buffer frame size and buffer frame size range */
    count = sizeof(UInt32);
    property_address.mSelector = kAudioDevicePropertyBufferFrameSize;
    err = AudioObjectGetPropertyData(mOutputDevice, &property_address, 0, NULL, &count, &bufferSize);

    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);

    Server_debug(self, "Coreaudio : Coreaudio output device buffer size = %ld\n", bufferSize);

    count = sizeof(AudioValueRange);
    property_address.mSelector = kAudioDevicePropertyBufferSizeRange;
    err = AudioObjectGetPropertyData(mOutputDevice, &property_address, 0, NULL, &count, &range);

    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);

    Server_debug(self, "Coreaudio : Coreaudio output device buffer size range = %.2f -> %.2f\n", range.mMinimum, range.mMaximum);

    /* Get output device sampling rate */
    count = sizeof(Float64);
    property_address.mSelector = kAudioDevicePropertyNominalSampleRate;
    err = AudioObjectGetPropertyData(mOutputDevice, &property_address, 0, NULL, &count, &sampleRate);

    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);

    Server_debug(self, "Coreaudio : Coreaudio output device sampling rate = %.2f\n", sampleRate);


    /****************************************/
    /********* Set audio properties *********/
    /****************************************/
    /* set/get the buffersize for the devices */
    count = sizeof(UInt32);
    property_address.mSelector = kAudioDevicePropertyBufferFrameSize;
    err = AudioObjectSetPropertyData(mOutputDevice, &property_address, 0, NULL, count, &self->bufferSize);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        self->bufferSize = bufferSize;
        err = AudioObjectSetPropertyData(mOutputDevice, &property_address, 0, NULL, count, &self->bufferSize);

        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        else
            Server_debug(self, "pyo buffer size set to output device buffer size : %i\n", self->bufferSize);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device buffer size successfully: %i\n", self->bufferSize);

    if (self->duplex == 1)
    {
        err = AudioObjectSetPropertyData(mInputDevice, &property_address, 0, NULL, count, &self->bufferSize);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        }
    }

    /* set/get the sampling rate for the devices */
    count = sizeof(double);
    double pyoSamplingRate = self->samplingRate;
    property_address.mSelector = kAudioDevicePropertyNominalSampleRate;
    err = AudioObjectSetPropertyData(mOutputDevice, &property_address, 0, NULL, count, &pyoSamplingRate);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        self->samplingRate = (double)sampleRate;
        err = AudioObjectSetPropertyData(mOutputDevice, &property_address, 0, NULL, count, &sampleRate);

        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        else
            Server_debug(self, "pyo sampling rate set to output device sampling rate : %i\n", self->samplingRate);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device sampling rate successfully: %.2f\n", self->samplingRate);

    if (self->duplex == 1)
    {
        pyoSamplingRate = self->samplingRate;
        err = AudioObjectSetPropertyData(mInputDevice, &property_address, 0, NULL, count, &pyoSamplingRate);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        }
    }


    /****************************************/
    /* Input and output stream descriptions */
    /****************************************/
    AudioStreamBasicDescription outputStreamDescription;
    AudioStreamBasicDescription inputStreamDescription;

    // Get input device stream configuration
    if (self->duplex == 1)
    {
        AudioObjectPropertyAddress pa;
            pa.mSelector = kAudioDevicePropertyStreamFormat;
            pa.mScope = kAudioDevicePropertyScopeInput;
            pa.mElement = kAudioObjectPropertyElementMain;
        count = sizeof(AudioStreamBasicDescription);
        err = AudioObjectGetPropertyData(mInputDevice, &pa, 0, NULL, &count, &inputStreamDescription);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);
        }
        else
        {
            inputStreamDescription.mSampleRate = (Float64)self->samplingRate;
            if (self->ichnls + self->input_offset > (int)inputStreamDescription.mChannelsPerFrame)
            {
                property_address.mSelector = kAudioDevicePropertyDeviceName;
                name = (char*)PyMem_RawMalloc(256);
                propertysize = 256;
                err = AudioObjectGetPropertyData(mInputDevice, &property_address, 0, NULL, &propertysize, name);

                if (err != kAudioHardwareNoError)
                {
                    Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
                }
                Server_warning(self, "Coreaudio input device `%s` has fewer channels (%d) than requested (%d).\n", name,
                               inputStreamDescription.mChannelsPerFrame,
                               self->ichnls + self->input_offset);
                PyMem_RawFree(name);
                self->ichnls = inputStreamDescription.mChannelsPerFrame;
                self->input_offset = 0;
            }
        }

        Server_debug(self, "Coreaudio : Coreaudio driver input stream sampling rate = %.2f\n", inputStreamDescription.mSampleRate);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream bytes per frame = %i\n", inputStreamDescription.mBytesPerFrame);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream number of channels = %i\n", inputStreamDescription.mChannelsPerFrame);
    }

    /* Get output device stream configuration */
    count = sizeof(AudioStreamBasicDescription);
    property_address.mSelector = kAudioDevicePropertyStreamFormat;
    err = AudioObjectGetPropertyData(mOutputDevice, &property_address, 0, NULL, &count, &outputStreamDescription);

    if (err != kAudioHardwareNoError) {
        Server_error(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);
    }
    else
    {
        outputStreamDescription.mSampleRate = (Float64)self->samplingRate;
    }

    Server_debug(self, "Coreaudio : Coreaudio driver output stream sampling rate = %.2f\n", outputStreamDescription.mSampleRate);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream bytes per frame = %i\n", outputStreamDescription.mBytesPerFrame);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream number of channels = %i\n", outputStreamDescription.mChannelsPerFrame);


    /**************************************************/
    /********* Set input and output callbacks *********/
    /**************************************************/
    if (self->duplex == 1)
    {
        self->inprocid = NULL;
        err = AudioDeviceCreateIOProcID(self->input, (AudioDeviceIOProc) coreaudio_input_callback, self, &self->inprocid);    // setup our device with an IO proc
        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Input AudioDeviceAddIOProc failed %d\n", (int)err);
            return -1;
        }

        AudioObjectPropertyAddress pas;
            pas.mSelector = kAudioDevicePropertyIOProcStreamUsage;
            pas.mScope = kAudioDevicePropertyScopeInput;
            pas.mElement = kAudioObjectPropertyElementMain;
        err = AudioObjectGetPropertyData(self->input, &pas, 0, NULL, &propertysize, &writable);
        AudioHardwareIOProcStreamUsage *input_su = (AudioHardwareIOProcStreamUsage*)PyMem_RawMalloc(propertysize);
        input_su->mIOProc = (void*)coreaudio_input_callback;
        err = AudioObjectGetPropertyData(self->input, &pas, 0, NULL, &propertysize, input_su);

        for (i = 0; i < inputStreamDescription.mChannelsPerFrame; ++i)
        {
            input_su->mStreamIsOn[i] = 1;
        }

        err = AudioObjectSetPropertyData(self->input, &property_address, 0, NULL, propertysize, input_su);
        PyMem_RawFree(input_su);
    }

    self->outprocid = NULL;
    err = AudioDeviceCreateIOProcID(self->output, (AudioDeviceIOProc) coreaudio_output_callback, self, &self->outprocid);    // setup our device with an IO proc

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Output AudioDeviceCreateIOProcID failed %d\n", (int)err);
        return -1;
    }

    property_address.mSelector = kAudioDevicePropertyIOProcStreamUsage;
    err = AudioObjectGetPropertyData(self->output, &property_address, 0, NULL, &propertysize, &writable);
    AudioHardwareIOProcStreamUsage *output_su = (AudioHardwareIOProcStreamUsage*)PyMem_RawMalloc(propertysize);
    output_su->mIOProc = (void*)coreaudio_output_callback;
    err = AudioObjectGetPropertyData(self->output, &property_address, 0, NULL, &propertysize, output_su);

    for (i = 0; i < outputStreamDescription.mChannelsPerFrame; ++i)
    {
        output_su->mStreamIsOn[i] = 1;
    }
    err = AudioObjectSetPropertyData(self->output, &property_address, 0, NULL, propertysize, output_su);
    PyMem_RawFree(output_su);
    return 0;
}

int
Server_coreaudio_deinit(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    /* destroy mutex */
    err = pthread_mutex_destroy(&self->buf_mutex);
    if (err) {
        Server_error(self, "Could not destroy mutex\nReason: %s\n", (char*)&err);
    }
    /* destroy mutex condition*/
    err = pthread_cond_destroy(&self->buf_cond);
    if (err) {
        Server_error(self, "Could not destroy mutex condition\nReason: %s\n", (char*)&err);
    }

    if (self->duplex == 1)
    {
        AudioDeviceDestroyIOProcID(self->input, self->inprocid);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Input AudioDeviceRemoveIOProc failed %d\n", (int)err);
            return -1;
        }
    }

    AudioDeviceDestroyIOProcID(self->output, self->outprocid);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Output AudioDeviceRemoveIOProc failed %d\n", (int)err);
        return -1;
    }

    return 0;
}

int
Server_coreaudio_start(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1)
    {
        err = AudioDeviceStart(self->input, self->inprocid);

        if (err != kAudioHardwareNoError)
        {
            Server_error(self, "Input AudioDeviceStart failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceStart(self->output, self->outprocid);

    if (err != kAudioHardwareNoError)
    {
        Server_error(self, "Output AudioDeviceStart failed %d\n", (int)err);
        return -1;
    }
    self->server_started = 1;
    self->server_stopped = 0;

    return 0;
}

int
Server_coreaudio_stop(Server *self)
{
    coreaudio_stop_callback(self);
    self->server_stopped = 1;
    return 0;
}
