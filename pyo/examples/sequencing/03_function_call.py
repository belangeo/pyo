#!/usr/bin/env python
# encoding: utf-8

"""
Python function regularly called with a Pattern object.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

amp = Fader(fadein=1, mul=0.25).play()
src1 = BrownNoise(mul=amp)
src2 = FM(carrier=[50, 100], ratio=[1.01, 0.495], index=[10, 13], mul=amp * 0.3)

frs = SigTo(value=[250, 700, 1800, 3000], time=0.25, init=[250, 700, 1800, 3000])
out = Biquadx(src1 + src2, freq=frs, q=20, type=2, stages=2, mul=4).out()


def change():
    f1 = random.uniform(200, 500)
    f2 = random.uniform(500, 1000)
    f3 = random.uniform(1000, 2000)
    f4 = random.uniform(2000, 4000)
    lst = [f1, f2, f3, f4]
    frs.value = lst
    print("%.2f, %.2f, %.2f, %.2f" % tuple(lst))


lfo = Sine(freq=0.1, mul=0.5, add=0.75)
pat = Pattern(function=change, time=lfo).play()

s.gui(locals())
