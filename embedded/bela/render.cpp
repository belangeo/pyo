#include <BeagleRT.h>
#include <cmath>
#include <iostream>
#include <Utilities.h>
#include "PyoClass.h"

Pyo pyo;

bool setup(BeagleRTContext *context, void *userData)
{
	std::cout << context->audioFrames << std::endl;
    // initialize a pyo server
    pyo.setup(context->audioChannels, context->audioFrames, context->audioSampleRate);
    // load a python file
    pyo.loadfile("/root/BeagleRT/source/main.py", 0);

	return true;
}

void render(BeagleRTContext *context, void *userData)
{
    pyo.fillin(context->audioIn);
    pyo.process(context->audioOut);
}

void cleanup(BeagleRTContext *context, void *userData)
{

}
