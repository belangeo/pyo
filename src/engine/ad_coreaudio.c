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
    for (i=0; i<server->bufferSize; i++) {
        off1chnls = i*bufchnls+server->input_offset;
        off2chnls = i*servchnls;
        for (j=0; j<servchnls; j++) {
            server->input_buffer[off2chnls+j] = (MYFLT)bufdata[off1chnls+j];
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

    if (server->withPortMidi == 1) {
        pyoGetMidiEvents(server);
    }

    Server_process_buffers(server);
    AudioBuffer* outputBuf = outOutputData->mBuffers;
    bufchnls = outputBuf->mNumberChannels;
    servchnls = server->nchnls < bufchnls ? server->nchnls : bufchnls;
    float *bufdata = (float*)outputBuf->mData;
    for (i=0; i<server->bufferSize; i++) {
        off1chnls = i*bufchnls+server->output_offset;
        off2chnls = i*servchnls;
        for(j=0; j<servchnls; j++) {
            bufdata[off1chnls+j] = server->output_buffer[off2chnls+j];
        }
    }
    server->midi_count = 0;

    return kAudioHardwareNoError;
}

int
coreaudio_stop_callback(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceStop(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceStop failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceStop(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceStop failed %d\n", (int)err);
        return -1;
    }
    self->server_started = 0;
    return 0;
}

int
Server_coreaudio_init(Server *self)
{
    OSStatus err = kAudioHardwareNoError;
    UInt32 count, namelen, propertySize;
    int i, numdevices;
    char *name;
    AudioDeviceID mOutputDevice = kAudioDeviceUnknown;
    AudioDeviceID mInputDevice = kAudioDeviceUnknown;
    Boolean writable;
    AudioTimeStamp now;

    now.mFlags = kAudioTimeStampHostTimeValid;
    now.mHostTime = AudioGetCurrentHostTime();

    /************************************/
    /* List Coreaudio available devices */
    /************************************/
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &count, 0);
    AudioDeviceID *devices = (AudioDeviceID*) malloc(count);
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &count, devices);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Get kAudioHardwarePropertyDevices error %s\n", (char*)&err);
        free(devices);
    }

    numdevices = count / sizeof(AudioDeviceID);
    Server_debug(self, "Coreaudio : Number of devices: %i\n", numdevices);

    for (i=0; i<numdevices; ++i) {
        err = AudioDeviceGetPropertyInfo(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            break;
        }

        char *name = (char*)malloc(count);
        err = AudioDeviceGetProperty(devices[i], 0, false, kAudioDevicePropertyDeviceName, &count, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %d %08X\n", (char*)&err, i, devices[i]);
            free(name);
            break;
        }
        Server_debug(self, "   %d : \"%s\"\n", i, name);
        free(name);
    }

    /************************************/
    /* Acquire input and output devices */
    /************************************/
    /* Acquire input audio device */
    if (self->duplex == 1) {
        if (self->input != -1)
            mInputDevice = devices[self->input];

        if (mInputDevice==kAudioDeviceUnknown) {
            count = sizeof(mInputDevice);
            err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &count, (void *) &mInputDevice);
            if (err != kAudioHardwareNoError) {
                Server_error(self, "Get kAudioHardwarePropertyDefaultInputDevice error %s\n", (char*)&err);
                return -1;
            }
        }

        err = AudioDeviceGetPropertyInfo(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }
        name = (char*)malloc(namelen);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mInputDevice);
        }
        Server_debug(self, "Coreaudio : Uses input device : \"%s\"\n", name);
        self->input = mInputDevice;
        free(name);
    }

    /* Acquire output audio device */
    if (self->output != -1)
        mOutputDevice = devices[self->output];

    if (mOutputDevice==kAudioDeviceUnknown) {
        count = sizeof(mOutputDevice);
        err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &count, (void *) &mOutputDevice);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Get kAudioHardwarePropertyDefaultOutputDevice error %s\n", (char*)&err);
            return -1;
        }
    }

    err = AudioDeviceGetPropertyInfo(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, 0);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Info kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    name = (char*)malloc(namelen);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyDeviceName, &namelen, name);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Get kAudioDevicePropertyDeviceName error %s A %08X\n", (char*)&err, mOutputDevice);
    }
    Server_debug(self, "Coreaudio : Uses output device : \"%s\"\n", name);
    self->output = mOutputDevice;
    free(name);

    /*************************************************/
    /* Get in/out buffer frame and buffer frame size */
    /*************************************************/
    UInt32 bufferSize;
    AudioValueRange range;
    Float64 sampleRate;

    /* Get input device buffer frame size and buffer frame size range */
    if (self->duplex == 1) {
        count = sizeof(UInt32);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, &bufferSize);
        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device buffer size = %ld\n", bufferSize);

        count = sizeof(AudioValueRange);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyBufferSizeRange, &count, &range);
        if (err != kAudioHardwareNoError)
            Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device buffer size range = %f -> %f\n", range.mMinimum, range.mMaximum);

        /* Get input device sampling rate */
        count = sizeof(Float64);
        err = AudioDeviceGetProperty(mInputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, &sampleRate);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        Server_debug(self, "Coreaudio : Coreaudio input device sampling rate = %.2f\n", sampleRate);
    }

    /* Get output device buffer frame size and buffer frame size range */
    count = sizeof(UInt32);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferFrameSize, &count, &bufferSize);
    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferFrameSize error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device buffer size = %ld\n", bufferSize);

    count = sizeof(AudioValueRange);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyBufferSizeRange, &count, &range);
    if (err != kAudioHardwareNoError)
        Server_error(self, "Get kAudioDevicePropertyBufferSizeRange error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device buffer size range = %.2f -> %.2f\n", range.mMinimum, range.mMaximum);

    /* Get output device sampling rate */
    count = sizeof(Float64);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyNominalSampleRate, &count, &sampleRate);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
    Server_debug(self, "Coreaudio : Coreaudio output device sampling rate = %.2f\n", sampleRate);


    /****************************************/
    /********* Set audio properties *********/
    /****************************************/
    /* set/get the buffersize for the devices */
    count = sizeof(UInt32);
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        self->bufferSize = bufferSize;
        err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        else
            Server_debug(self, "pyo buffer size set to output device buffer size : %i\n", self->bufferSize);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device buffer size successfully: %i\n", self->bufferSize);

    if (self->duplex == 1) {
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyBufferFrameSize, count, &self->bufferSize);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyBufferFrameSize error %4.4s\n", (char*)&err);
        }
    }

    /* set/get the sampling rate for the devices */
    count = sizeof(double);
    double pyoSamplingRate = self->samplingRate;
    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        self->samplingRate = (double)sampleRate;
        err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &sampleRate);
        if (err != kAudioHardwareNoError)
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        else
            Server_debug(self, "pyo sampling rate set to output device sampling rate : %i\n", self->samplingRate);
    }
    else
        Server_debug(self, "Coreaudio : Changed output device sampling rate successfully: %.2f\n", self->samplingRate);

    if (self->duplex ==1) {
        pyoSamplingRate = self->samplingRate;
        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyNominalSampleRate, count, &pyoSamplingRate);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "set kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        }
    }


    /****************************************/
    /* Input and output stream descriptions */
    /****************************************/
    AudioStreamBasicDescription outputStreamDescription;
    AudioStreamBasicDescription inputStreamDescription;

    // Get input device stream configuration
    if (self->duplex == 1) {
        count = sizeof(AudioStreamBasicDescription);
        err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

        /*
        inputStreamDescription.mSampleRate = (Float64)self->samplingRate;

        err = AudioDeviceSetProperty(mInputDevice, &now, 0, false, kAudioDevicePropertyStreamFormat, count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "-- Set kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

        // Print new input stream description
        err = AudioDeviceGetProperty(mInputDevice, 0, true, kAudioDevicePropertyStreamFormat, &count, &inputStreamDescription);
        if (err != kAudioHardwareNoError)
            Server_debug(self, "Get kAudioDevicePropertyNominalSampleRate error %s\n", (char*)&err);
        */
        Server_debug(self, "Coreaudio : Coreaudio driver input stream sampling rate = %.2f\n", inputStreamDescription.mSampleRate);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream bytes per frame = %i\n", inputStreamDescription.mBytesPerFrame);
        Server_debug(self, "Coreaudio : Coreaudio driver input stream number of channels = %i\n", inputStreamDescription.mChannelsPerFrame);
    }

    /* Get output device stream configuration */
    count = sizeof(AudioStreamBasicDescription);
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyStreamFormat, &count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

    /*
    outputStreamDescription.mSampleRate = (Float64)self->samplingRate;

    err = AudioDeviceSetProperty(mOutputDevice, &now, 0, false, kAudioDevicePropertyStreamFormat, count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Set kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);

    // Print new output stream description
    err = AudioDeviceGetProperty(mOutputDevice, 0, false, kAudioDevicePropertyStreamFormat, &count, &outputStreamDescription);
    if (err != kAudioHardwareNoError)
        Server_debug(self, "Get kAudioDevicePropertyStreamFormat error %s\n", (char*)&err);
    */
    Server_debug(self, "Coreaudio : Coreaudio driver output stream sampling rate = %.2f\n", outputStreamDescription.mSampleRate);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream bytes per frame = %i\n", outputStreamDescription.mBytesPerFrame);
    Server_debug(self, "Coreaudio : Coreaudio driver output stream number of channels = %i\n", outputStreamDescription.mChannelsPerFrame);


    /**************************************************/
    /********* Set input and output callbacks *********/
    /**************************************************/
    if (self->duplex == 1) {
        err = AudioDeviceAddIOProc(self->input, coreaudio_input_callback, (void *) self);    // setup our device with an IO proc
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceAddIOProc failed %d\n", (int)err);
            return -1;
        }
        err = AudioDeviceGetPropertyInfo(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
        AudioHardwareIOProcStreamUsage *input_su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
        input_su->mIOProc = (void*)coreaudio_input_callback;
        err = AudioDeviceGetProperty(self->input, 0, true, kAudioDevicePropertyIOProcStreamUsage, &propertySize, input_su);
        for (i=0; i<inputStreamDescription.mChannelsPerFrame; ++i) {
            input_su->mStreamIsOn[i] = 1;
        }
        err = AudioDeviceSetProperty(self->input, &now, 0, true, kAudioDevicePropertyIOProcStreamUsage, propertySize, input_su);
    }

    err = AudioDeviceAddIOProc(self->output, coreaudio_output_callback, (void *) self);    // setup our device with an IO proc
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceAddIOProc failed %d\n", (int)err);
        return -1;
    }
    err = AudioDeviceGetPropertyInfo(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, &writable);
    AudioHardwareIOProcStreamUsage *output_su = (AudioHardwareIOProcStreamUsage*)malloc(propertySize);
    output_su->mIOProc = (void*)coreaudio_output_callback;
    err = AudioDeviceGetProperty(self->output, 0, false, kAudioDevicePropertyIOProcStreamUsage, &propertySize, output_su);
    for (i=0; i<outputStreamDescription.mChannelsPerFrame; ++i) {
        output_su->mStreamIsOn[i] = 1;
    }
    err = AudioDeviceSetProperty(self->output, &now, 0, false, kAudioDevicePropertyIOProcStreamUsage, propertySize, output_su);

    return 0;
}

int
Server_coreaudio_deinit(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceRemoveIOProc(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceRemoveIOProc failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceRemoveIOProc(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceRemoveIOProc failed %d\n", (int)err);
        return -1;
    }

    return 0;
}

int
Server_coreaudio_start(Server *self)
{
    OSStatus err = kAudioHardwareNoError;

    if (self->duplex == 1) {
        err = AudioDeviceStart(self->input, coreaudio_input_callback);
        if (err != kAudioHardwareNoError) {
            Server_error(self, "Input AudioDeviceStart failed %d\n", (int)err);
            return -1;
        }
    }

    err = AudioDeviceStart(self->output, coreaudio_output_callback);
    if (err != kAudioHardwareNoError) {
        Server_error(self, "Output AudioDeviceStart failed %d\n", (int)err);
        return -1;
    }
    return 0;
}

int
Server_coreaudio_stop(Server *self)
{
    coreaudio_stop_callback(self);
    self->server_stopped = 1;
    return 0;
}
