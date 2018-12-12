#!/usr/bin/env python
# encoding: utf-8
"""
Simple reverb based on Schroeder algorithm.
4 serial allpass filters --> 4 parallel lowpass filters.

"""
from pyo import *

s = Server(duplex=0).boot()

a = SfPlayer("../snds/flute.aif", loop=True, mul=0.25).mix(2).out()

b1 = Allpass(a, delay=[.0204,.02011], feedback=0.35)
b2 = Allpass(b1, delay=[.06653,.06641], feedback=0.41)
b3 = Allpass(b2, delay=[.035007,.03504], feedback=0.5)
b4 = Allpass(b3, delay=[.023021 ,.022987], feedback=0.65)

c1 = Tone(b1, 5000, mul=0.2).out()
c2 = Tone(b2, 3000, mul=0.2).out()
c3 = Tone(b3, 1500, mul=0.2).out()
c4 = Tone(b4, 500, mul=0.2).out()

s.gui(locals())
