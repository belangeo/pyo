#!/usr/bin/env python
# encoding: utf-8
"""
Simple flanger.

"""
from pyo import *

s = Server(duplex=0).boot()

# -->
src = BrownNoise(0.1).mix(2).out()
# <--

# -->
lf = Sine(freq=0.2, mul=0.0045, add=0.005)
flg = Delay(src, delay=lf, feedback=0.25).out()
# <--

s.gui(locals())
