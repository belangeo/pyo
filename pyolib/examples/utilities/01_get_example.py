#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function

"""
The PyoObject.get() method can be used to convert audio stream to usable python data.

"""
from pyo import *

s = Server(duplex=0).boot()

lfos = Sine(freq=[.1,.2,.4,.3], mul=100, add=500)
synth = SineLoop(freq=lfos, feedback=.07, mul=.05).out()

def print_val():
    # Print all four frequency values assigned to SineLoop's freq argument
    print("%.2f, %.2f, %.2f, %.2f" % tuple(lfos.get(all=True)))

pat = Pattern(print_val, .25).play()

s.gui(locals())
