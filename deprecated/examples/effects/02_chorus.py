#!/usr/bin/env python
# encoding: utf-8
"""
Hand-written 8 delay lines chorus.

"""
from pyo import *

s = Server(duplex=0).boot()

# --> Start a sound
sf = SfPlayer("../snds/baseballmajeur_m.aif", speed=1, loop=True, mul=0.3)
sf2 = sf.mix(2).out()
# <--

# --> Sets values for 8 delay lines
# delay line frequencies
freqs = [0.254, 0.465, 0.657, 0.879, 1.23, 1.342, 1.654, 1.879]
# delay line center delays
cdelay = [0.0087, 0.0102, 0.0111, 0.01254, 0.0134, 0.01501, 0.01707, 0.0178]
# delay line delay amplitudes
adelay = [0.001, 0.0012, 0.0013, 0.0014, 0.0015, 0.0016, 0.002, 0.0023]
# modulation depth
depth = Sig(1)
# <--

# --> Add the delay lines to the source sound
lfos = Sine(freqs, mul=adelay * depth, add=cdelay)
delays = Delay(sf, lfos, feedback=0.5, mul=0.5).out()
# <--

s.gui(locals())
