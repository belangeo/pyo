#!/usr/bin/env python
# encoding: utf-8
"""
Simple flanger.

"""
from pyo import *

s = Server(duplex=0).boot()

#-->
src = BrownNoise(.1).mix(2).out()
#<--

#-->
lf = Sine(freq=.2, mul=.0045, add=.005)
flg = Delay(src, delay=lf, feedback=.25).out()
#<--

s.gui(locals())
