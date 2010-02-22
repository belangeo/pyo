#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-02-21.
"""
import math
from pyo import *

def midiToHz(x):
    return 8.175798 * math.pow(1.0594633, x)
    
s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

sf = SfPlayer(DEMOS_PATH + '/transparent.aif', speed=1, loop=True, mul=.5).out(1)

BASE_HZ = midiToHz(60)

env = HannTable()

wsize = .05
trans = .005

new_hz = midiToHz(60+trans)
ratio = new_hz / BASE_HZ
rate = (ratio-1) / wsize

ind = Phasor(-rate, [0, .5])
win = Pointer(env, ind)
snd = Delay(sf, ind*wsize, feedback = .7, mul=win).out([0,0])

s.gui(locals())

#Harmonizer(input, transpo, winsize, transpodev, windev, overlaps, mul, add)