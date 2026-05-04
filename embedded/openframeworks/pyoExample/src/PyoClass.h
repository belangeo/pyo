#ifndef __pyoclass_h_
#define __pyoclass_h_

#include <string>
#include <vector>
#include "m_pyo.h"

typedef int callPtr(int);

class Pyo {
    public:
		//Pyo();
        ~Pyo();
        void setup(int inChannels, int outChannels, int bufferSize, int sampleRate);
        void process(float *buffer);
        void fillin(float *buffer);
        void clear();
        int loadfile(const char *file, int add);
        int exec(const char *msg);
        int value(const char *name, float value);
        int value(const char *name, float *value, int len);
        int set(const char *name, float value);
        int set(const char *name, float *value, int len);
		void setDebug(int debugVal);
		std::vector<std::string> getStdout();
		std::string getErrorMsg();
		bool isProcessing();

    private:
		//PyGILState_STATE gstate; // to acquire and release the Global Interpreter Lock (GIL)
        int inChannels;
        int outChannels;
        int bufferSize;
        int sampleRate;
		int debug;
        PyThreadState *interpreter;
        float *pyoInBuffer;
        float *pyoOutBuffer;
        callPtr *pyoCallback;
        int pyoId;
		bool processing;
        char pyoMsg[262144];
};

#endif
