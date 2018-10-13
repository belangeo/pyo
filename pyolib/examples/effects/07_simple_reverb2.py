#!/usr/bin/env python
# encoding: utf-8
"""
Simple reverb based on Schroeder algorithm.
4 parallel comb filters --> 2 serial allpass filters.

"""
from pyo import *

s = Server(duplex=0).boot()

a = SfPlayer("../snds/flute.aif", loop=True, mul=0.3).mix(2).out()

comb1 = Delay(a, delay=[0.0297,0.0277], feedback=0.65)
comb2 = Delay(a, delay=[0.0371,0.0393], feedback=0.51)
comb3 = Delay(a, delay=[0.0411,0.0409], feedback=0.5)
comb4 = Delay(a, delay=[0.0137,0.0155], feedback=0.73)

combsum = a + comb1 + comb2 + comb3 + comb4

all1 = Allpass(combsum, delay=[.005,.00507], feedback=0.75)
all2 = Allpass(all1, delay=[.0117,.0123], feedback=0.61)
lowp = Tone(all2, freq=3500, mul=.2).out()


s.gui(locals())
