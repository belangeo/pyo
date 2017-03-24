#!/usr/bin/env python
# encoding: utf-8
"""
Hand-written 8 delay lines chorus.

"""
from pyo import *

s = Server(duplex=0).boot()

#--> Start a sound
sf = SfPlayer('../snds/baseballmajeur_m.aif', speed=1, loop=True, mul=.3)
sf2 = sf.mix(2).out()
#<--

#--> Sets values for 8 delay lines
# delay line frequencies
freqs = [.254, .465, .657, .879, 1.23, 1.342, 1.654, 1.879]
# delay line center delays
cdelay = [.0087, .0102, .0111, .01254, .0134, .01501, .01707, .0178]
# delay line delay amplitudes
adelay = [.001, .0012, .0013, .0014, .0015, .0016, .002, .0023]
# modulation depth
depth = Sig(1)
#<--

#--> Add the delay lines to the source sound
lfos = Sine(freqs, mul=adelay*depth, add=cdelay)
delays = Delay(sf, lfos, feedback=.5, mul=.5).out()
#<--

s.gui(locals())
