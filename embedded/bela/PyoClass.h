#pragma once

#include "m_pyo.h"

typedef int callPtr(int);

class Pyo {
    public:
        ~Pyo();
        void setup(int nChannels, int bufferSize, int sampleRate, int nAnalogChannels);
        void process(float *buffer);
        void fillin(float *buffer);
        void analogin(float *buffer);
        void analogout(float *buffer);
        void clear();
        int loadfile(const char *file, int add);
        int exec(const char *msg);
        int value(const char *name, float value);
        int value(const char *name, float *value, int len);
        int set(const char *name, float value);
        int set(const char *name, float *value, int len);

    private:
        int nChannels;
        int bufferSize;
        int sampleRate;
        int nAnalogChannels;
        int nTotalChannels;
        PyThreadState *interpreter;
        float *pyoInBuffer;
        float *pyoOutBuffer;
        callPtr *pyoCallback;
        int pyoId;
        char pyoMsg[262144];
};
