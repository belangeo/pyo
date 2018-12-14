#!/usr/bin/env python
# encoding: utf-8
from pyo import *

s = Server().boot()

# play voice and silence
t = SndTable(SNDS_PATH + "/transparent.aif")
m = SDelay(Metro(3).play(), 1)
voix = TrigEnv(m, t, dur=t.getDur(), mul=0.7).mix(2).out()

# the music!
freqs = [midiToHz(m+7) for m in [60,62,63.93,65,67.01,69,71,72]]
chx = Choice(choice=freqs, freq=[1,2,3,4])
port = Port(chx, risetime=.001, falltime=.001)
sines = SineLoop(port, feedback=[.06,.057,.033,.035], mul=[.15,.15,.1,.1])
music = SPan(sines, pan=[0, 1, .2, .8, .5]).mix(2)

# Follow voice amplitude
fol = Follower(voix, freq=10)
# talk = 1 if voice is playing and 0 if not
talk = fol > 0.01

# Smooth the signal
amp = Port(talk, risetime=0.05, falltime=0.1)
# rescale (1 when no voice and 0.2 when voice is playing)
ampscl = Scale(amp, outmin=1, outmax=0.2)

# Apply gain and output music...
outsynth = (music * ampscl).out()

s.gui(locals())
