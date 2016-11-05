#!/usr/bin/env python
# encoding: utf-8
"""
Hand-written pulsar synthesis.

"""
from pyo import *
import random

s = Server(sr=48000, nchnls=2, buffersize=512, duplex=0).boot()

# simple pulsar waveform
t = HarmTable([1,0,.3,0,.2,0,0,.1], size=32768)

# Chorus of reader indexes around frequencies 50, 100, 150 and 200 Hz
a = Phasor(freq=[((i%4)+1)*50*random.uniform(.99, 1.01) for i in range(16)])

# Duty cycle control
frac = Sig(.5)
frac.ctrl(title="Pulsar duty cycle")

# Pulsar synthesis
scl = a * (a <= frac) * (1. / frac)
c = Pointer(table=HannTable(), index=scl, mul=.05)
d = Pointer(table=t, index=scl, mul=c).out()

s.gui(locals())
