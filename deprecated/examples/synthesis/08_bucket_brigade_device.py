#!/usr/bin/env python
# encoding: utf-8
"""
bucket-brigade device (BBD) synthesis. This implementation consists of
two delay lines cross-feeded with an internal ring-modulator and a lowpass
filter inside each delay line. The delay lines are feeded with a sine wave.

"""
from pyo import *

s = Server(duplex=0).boot()

t = HarmTable(size=32768)
src = Osc(t, 100)
src.ctrl(title="Input oscillator controls")
in_src = src * 0.025

feed = .8
cross_feed = .99

del_1 = Delay(in_src, delay=Sine(.005, 0, .05, .25))
sine_1 = Osc(t, Sine(.007, 0, 50, 250))
ring_1 = del_1 * sine_1
filt_1 = Biquad(ring_1, 3000)

del_2 = Delay(in_src, delay=Sine(.003, 0, .08, .3))
sine_2 = Osc(t, Sine(.008, 0, 40, 200))
ring_2 = del_2 * sine_2
filt_2 = Biquad(ring_2, 3000)

cross_1 = filt_2 * cross_feed
cross_2 = filt_1 * cross_feed

del_1.setInput(filt_1 * feed + cross_1 + in_src)
del_2.setInput(filt_2 * feed + cross_2 + in_src)

mix = Mix([filt_1, filt_2], voices=2)
output = Compress(mix, thresh=-30, ratio=4, knee=0.5).out()

s.gui(locals())
