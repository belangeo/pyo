"""
04-record-perf.py - Recording the performance on disk.

The Server object allow the recording of the overall playback
(that is exactly what your hear). The "Rec Start" button of the
Server's window is doing that with default parameters. It'll
record a file called "pyo_rec.wav" (16-bit, 44100 Hz) on the
user's desktop.

You can control the recording with the Server's method called
`recordOptions`, the arguments are:

- dur : The duration of the recording, a value of -1 means
        record forever (recstop() must be called by the user).
- filename : Indicates the location of the recorded file.
- fileformat : The format of the audio file (see documentation
               for available formats).
- sampletype : The sample type of the audio file (see documentation
               for available types).

The recording can be triggered programmatically with the Server's
methods `recstart()` and `recstop()`. In order to record multiple files
from a unique performance, it is possible to set the filename
with an argument to `recstart()`.


"""
from pyo import *
import os

s = Server().boot()

# Path of the recorded sound file.
path = os.path.join(os.path.expanduser("~"), "Desktop", "synth.wav")
# Record for 10 seconds a 24-bit wav file.
s.recordOptions(dur=10, filename=path, fileformat=0, sampletype=1)

# Creates an amplitude envelope
amp = Fader(fadein=1, fadeout=1, dur=10, mul=0.3).play()

# A Simple synth
lfo = Sine(freq=[0.15, 0.16]).range(1.25, 1.5)
fm2 = CrossFM(carrier=200, ratio=lfo, ind1=10, ind2=2, mul=amp).out()

# Starts the recording for 10 seconds...
s.recstart()

s.gui(locals())
