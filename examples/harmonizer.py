#!/usr/bin/env python
# encoding: utf-8
"""
Hand-written harmonizer.

"""

import math
from pyo import *
    
s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

sf = SfPlayer(DEMOS_PATH + '/transparent.aif', speed=1, loop=True, mul=.5).out(1)

BASE_HZ = midiToHz(60)

env = HannTable()

wsize = .04
trans = 7

new_hz = midiToHz(60+trans)
ratio = new_hz / BASE_HZ
rate = (ratio-1) / wsize

ind = Phasor(-rate, [0, .5])
win = Pointer(env, ind)
snd = Delay(sf, ind*wsize, mul=win).out([0,0])

s.gui(locals())

#Harmonizer(input, transpo, feedback, winsize, mul, add)
