"""
03-read-from-ram.py - Soundfile playback from RAM.

Reading a sound file from the RAM gives the advantage of a very
fast access to every loaded samples. This is very useful for a
lot of processes, such as granulation, looping, creating envelopes
and waveforms and many others.

The simplest way of loading a sound in RAM is to use the SndTable
object. This example loads a sound file and reads it in loop.
We will see some more evolved processes later...

"""
from pyo import *

s = Server().boot()

path = SNDS_PATH + "/transparent.aif"

# Loads the sound file in RAM. Beginning and ending points
# can be controlled with "start" and "stop" arguments.
t = SndTable(path)

# Gets the frequency relative to the table length.
freq = t.getRate()

# Simple stereo looping playback (right channel is 180 degrees out-of-phase).
osc = Osc(table=t, freq=freq, phase=[0, 0.5], mul=0.4).out()

s.gui(locals())
