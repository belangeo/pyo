#ifndef PYOCLASS_H_INCLUDED
#define PYOCLASS_H_INCLUDED

#include "m_pyo.h"
#include "../JuceLibraryCode/JuceHeader.h"

typedef int callPtr(int);

class Pyo {
    public:
        ~Pyo();
        void setup(int nChannels, int bufferSize, int sampleRate);
        void process(AudioSampleBuffer& buffer);
        void clear();
        int loadfile(const char *file, int add);
        int loadfile(const String &file, int add);
        int exec(const char *msg);
        int exec(const String &msg);
        int value(const char *name, float value);
        int value(const String &name, float value);
        int value(const char *name, float *value, int len);
        int value(const String &name, float *value, int len);
        int set(const char *name, float value);
        int set(const String &name, float value);
        int set(const char *name, float *value, int len);
        int set(const String &name, float *value, int len);
        void midi(int status, int data1, int data2);

    private:
        int nChannels;
        int bufferSize;
        int sampleRate;
        PyThreadState *interpreter;
        float *pyoInBuffer;
        float *pyoOutBuffer;
        callPtr *pyoCallback;
        int pyoId;
        char pyoMsg[262144];
};

#endif  // PYOCLASS_H_INCLUDED
