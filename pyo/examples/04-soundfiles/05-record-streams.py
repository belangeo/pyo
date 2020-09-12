"""
05-record-streams.py - Recording individual audio streams on disk.

The Record object can be used to record specific audio streams
from a performance. It can be useful to record a sound in mutiple
tracks to make post-processing on individual part easier. This
example record the bass, the mid and the higher part in three
separated files on the user's desktop.

The `fileformat` and `sampletype` arguments are the same as in
the Server's `recordOptions` method.

"""
from pyo import *
import os

s = Server().boot()

# Defines sound file paths.
path = os.path.join(os.path.expanduser("~"), "Desktop")
bname = os.path.join(path, "bass.wav")
mname = os.path.join(path, "mid.wav")
hname = os.path.join(path, "high.wav")

# Creates an amplitude envelope
amp = Fader(fadein=1, fadeout=1, dur=10, mul=0.3).play()

# Bass voice
blfo = Sine(freq=[0.15, 0.16]).range(78, 82)
bass = RCOsc(freq=blfo, mul=amp).out()

# Mid voice
mlfo = Sine(freq=[0.18, 0.19]).range(0.24, 0.26)
mid = FM(carrier=1600, ratio=mlfo, index=5, mul=amp * 0.3).out()

# High voice
hlfo = Sine(freq=[0.1, 0.11, 0.12, 0.13]).range(7000, 8000)
high = Sine(freq=hlfo, mul=amp * 0.1).out()

# Creates the recorders
brec = Record(bass, filename=bname, chnls=2, fileformat=0, sampletype=1)
mrec = Record(mid, filename=mname, chnls=2, fileformat=0, sampletype=1)
hrec = Record(high, filename=hname, chnls=2, fileformat=0, sampletype=1)

# After 10.1 seconds, recorder objects will be automatically deleted.
# This will trigger their stop method and properly close the sound files.
clean = Clean_objects(10.1, brec, mrec, hrec)

# Starts the internal timer of Clean_objects. Use its own thread.
clean.start()

# Starts the Server, in order to be sync with the cleanup process.
s.start()

s.gui(locals())
