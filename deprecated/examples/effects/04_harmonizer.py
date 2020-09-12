#!/usr/bin/env python
# encoding: utf-8
"""
Hand-written harmonizer.

"""
from pyo import *

s = Server(duplex=0).boot()

sf = SfPlayer("../snds/flute.aif", speed=1, loop=True, mul=0.5).out()

env = WinTable(8)

wsize = 0.1
trans = -7

ratio = pow(2.0, trans / 12.0)
rate = -(ratio - 1) / wsize

ind = Phasor(freq=rate, phase=[0, 0.5])
win = Pointer(table=env, index=ind, mul=0.7)
snd = Delay(sf, delay=ind * wsize, mul=win).mix(1).out(1)

s.gui(locals())
