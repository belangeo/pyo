#!/usr/bin/env python
# encoding: utf-8

"""
Synthesis sound with lot of harmonics passed through a Degrade object.

"""
from pyo import *

s = Server(duplex=0).boot()

# Fadein
f = Fader(fadein=4, mul=0.3).play()

# Waveform table
wt = HarmTable([1, 0, 0.2, 0, 0.10, 0, 0, 0.1, 0, 0, 0, 0.15]).normalize()

# 6 waveform readers with amplitude variations
lf = Sine([0.15, 0.2], 0, 0.35, 0.5)
a = Osc(table=wt, freq=[25, 50.1, 101, 149.7, 201.3, 251.8], mul=lf)

# table lookup with waveforms as input indexes
t = ChebyTable([1, 0, 0.3, 0, 0.2, 0, 0.143, 0, 0.111])
b = Lookup(t, a, 1.0 - lf)

# signal with lot of harmonics passed through a Degrade object
c = Degrade(b, bitdepth=5.967, srscale=0.0233, mul=f).out()
c.ctrl()

s.gui(locals())
