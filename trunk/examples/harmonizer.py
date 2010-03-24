#!/usr/bin/env python
# encoding: utf-8
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
import math
from pyo import *
    
s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

sf = SfPlayer(DEMOS_PATH + '/transparent.aif', speed=1, loop=True, mul=.5).out(1)

BASE_HZ = midiToHz(60)

env = HannTable()

wsize = .05
trans = -0.1

new_hz = midiToHz(60+trans)
ratio = new_hz / BASE_HZ
rate = (ratio-1) / wsize

ind = Phasor(-rate, [0, .5])
win = Pointer(env, ind)
snd = Delay(sf, ind*wsize, feedback = 0.9, mul=win).out([0,0])

s.gui(locals())

#Harmonizer(input, transpo, winsize, feedback, transpodev, windev, overlaps, mul, add)
