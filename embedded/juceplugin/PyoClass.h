/**************************************************************************
 * Copyright 2018 Olivier Belanger                                        *
 *                                                                        *
 * This file is part of pyo-plug, an audio plugin using the python        *
 * module pyo to create the dsp.                                          *
 *                                                                        *
 * pyo-plug is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU Lesser General Public License as         *
 * published by the Free Software Foundation, either version 3 of the     *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * pyo-plug is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU Lesser General Public License for more details.                    *
 *                                                                        *
 * You should have received a copy of the GNU LGPL along with pyo-plug.   *
 * If not, see <http://www.gnu.org/licenses/>.                            *
 *                                                                        *
 *************************************************************************/
#ifndef PYOCLASS_H_INCLUDED
#define PYOCLASS_H_INCLUDED

#include "m_pyo.h"
#include "../JuceLibraryCode/JuceHeader.h"

typedef int callPtr(int);

class Pyo {
    public:
        Pyo();

        /*
        ** Terminates this object's interpreter.
        */
        ~Pyo();

        /*
        ** Creates a python interpreter and initialize a pyo server inside it.
        ** This function must be called, once per Pyo object, before any other
        ** calls.
        **
        ** arguments:
        **   nChannels : int, number of in/out channels.
        **   bufferSize : int, number of samples per buffer.
        **   sampleRate : int, sample rate frequency.
        **
        ** All arguments should be equal to the host audio settings.
        */
        void setup(int nChannels, int bufferSize, int sampleRate);

        /*
        ** This function can be used to pass the DAW's bpm value to the 
        ** python interpreter. Changes the value of the BPM variable 
        ** (which defaults to 60).
        **
        ** arguments:
        **   bpm : double, new BPM.
        */
        void setbpm(double bpm);

        /*
        ** This function fills pyo's input buffers with new samples, tells pyo to
        ** process a buffer of samples and fills the host's output buffer with new
        ** samples. Should be called once per process block, inside the host's
        ** processBlock function.
        **
        ** arguments:
        **   buffer : AudioBuffer<float>&, Juce's audio buffer object.
        */
        void process(AudioSampleBuffer& buffer);

        /*
        ** Execute a python script "file" in the objectès thread's interpreter.
        ** An integer "add" is needed to indicate if the pyo server should be
        ** reboot or not.
        **
        ** arguments:
        **   file : const char * or const String &,
        **             filename to execute as a python script. The file is first
        **             searched in the current working directory. If not found,
        **             the module will try to open it as an absolute path.
        **   add, int, if positive, the commands in the file will be added to whatever
        **             is already running in the pyo server. If 0, the server will be
        **             cleared before executing the file.
        **
        ** returns 0 (no error), 1 (failed to open the file) or 2 (bad code in file).
        */
        int loadfile(const char *file, int add);
        int loadfile(const String &file, int add);

        /*
        ** Executes any raw valid python statement. With this function, one can
        ** dynamically creates and manipulates audio objects and algorithms.
        **
        ** arguments:
        **   msg : const char * or const String &
        **         pointer to a string containing the statement to execute.
        **
        ** returns 0 (no error) or 1 (bad code in file).
        **
        ** Example (for a Pyo object named `pyo`):
        **
        ** pyo.exec("pits = [0.001, 0.002, 0.003, 0.004]")
        ** pyo.exec("fr = Rossler(pitch=pits, chaos=0.9, mul=250, add=500)")
        ** pyo.exec("b = SumOsc(freq=fr, ratio=0.499, index=0.4, mul=0.2).out()")
        */
        int exec(const char *msg);
        int exec(const String &msg);

        /*
        ** Sends a numerical value to an existing Sig or SigTo object.
        **
        ** arguments:
        **   name : const char * or const String &,
        **          variable name of the object.
        **   value : float, value to be assigned.
        **
        ** Example:
        **
        ** inside the script file:
        **
        ** freq = SigTo(value=440, time=0.1, init=440)
        **
        ** Inside Juce (for a Pyo object named `pyo`):
        **
        ** pyo.value("freq", 880);
        */
        int value(const char *name, float value);
        int value(const String &name, float value);

        /*
        ** Sends an array of numerical values to an existing Sig or SigTo object.
        **
        ** arguments:
        **   name : const char * or const String &,
        **          variable name of the object.
        **   value : float *, array of floats.
        **   len : int, number of elements in the array.
        **
        ** Example:
        **
        ** inside the script file:
        **
        ** freq = SigTo(value=[100,200,300,400], time=0.1, init=[100,200,300,400])
        **
        ** Inside Juce (for a Pyo object named `pyo`):
        **
        ** float frequencies[4] = {150, 250, 350, 450};
        ** pyo.value("freq", frequencies, 4);
        */
        int value(const char *name, float *value, int len);
        int value(const String &name, float *value, int len);

        /*
        ** Sends a numerical value to a Pyo object's attribute.
        **
        ** arguments:
        **   name : const char * or const String &,
        **          object name and attribute separated by a dot.
        **   value : float, value to be assigned.
        **
        ** Example:
        **
        ** inside the script file:
        **
        ** filter = Biquad(input=Noise(0.5), freq=1000, q=4, type=2)
        **
        ** Inside Juce (for a Pyo object named `pyo`):
        **
        ** pyo.set("filter.freq", 2000);
        */
        int set(const char *name, float value);
        int set(const String &name, float value);

        /*
        ** Sends an array of numerical values to a Pyo object's attribute.
        **
        ** arguments:
        **   name : const char * or const String &,
        **          object name and attribute separated by a dot.
        **   value : float *, array of floats.
        **   len : int, number of elements in the array.
        **
        ** Example:
        **
        ** inside the script file:
        **
        ** filters = Biquad(input=Noise(0.5), freq=[250, 500, 1000, 2000], q=5, type=2)
        **
        ** Inside Juce (for a Pyo object named `pyo`):
        **
        ** float frequencies[4] = {350, 700, 1400, 2800};
        ** pyo.set("filters.freq", frequencies, 4);
        */
        int set(const char *name, float *value, int len);
        int set(const String &name, float *value, int len);

        /*
        ** Sends a MIDI messges to the Server.
        **
        ** Example (for a Pyo object named `pyo`):
        **
        ** pyo.midi(144, 60, 127) //Send a Note on message on channel 1 for note # 60
        ** pyo.midi(128, 60, 0)   //Send a Note off messge on channel 1 for note # 60
        */
        void midi(int status, int data1, int data2);

        /*
        ** Shutdown and reboot the pyo server while keeping current in/out buffers.
        ** This will erase audio objects currently active within the server.
        **
        */
        void clear();

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
