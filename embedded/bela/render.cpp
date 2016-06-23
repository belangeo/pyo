#include <BeagleRT.h>
#include <cmath>
#include <iostream>
#include <Utilities.h>
#include "PyoClass.h"

Pyo pyo;

bool setup(BeagleRTContext *context, void *userData)
{
    // Initialize a pyo server.
    pyo.setup(context->audioChannels, context->audioFrames, 
              context->audioSampleRate, context->analogChannels);
    // Load a python file.
    pyo.loadfile("/root/BeagleRT/source/main.py", 0);

    return true;
}

void render(BeagleRTContext *context, void *userData)
{
    // Fill pyo input buffer (channels 0-1) with audio samples.
    pyo.fillin(context->audioIn);
    // Fill pyo input buffer (channels 2+) with analog inputs.
    pyo.analogin(context->analogIn);
    // Call pyo processing function and retrieve back stereo outputs.
    pyo.process(context->audioOut);
    // Get back pyo output channels 2+ as analog outputs.
    pyo.analogout(context->analogOut);
}

void cleanup(BeagleRTContext *context, void *userData)
{
}

