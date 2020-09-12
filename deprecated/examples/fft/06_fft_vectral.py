#!/usr/bin/env python
# encoding: utf-8
"""
Spectral gate and blur.

"""
from pyo import *

s = Server(duplex=0).boot()

snd = "../snds/ounkmaster.aif"

chnls = sndinfo(snd)[3]
size = 2048
olaps = 8
win = 5

snd = SfPlayer(snd, loop=True, mul=0.3)

fin = FFT(snd, size=size, overlaps=olaps, wintype=win)

pol = CarToPol(fin["real"], fin["imag"])

# bin-by-bin amplitude threshold
thresh = Sig(0.2)
thresh.ctrl([SLMap(0.0001, 1, "log", "value", 0.2)], title="Gate Threshold")

# attenuation
mult = Sig(0.1)
mult.ctrl([SLMap(0.0001, 1, "log", "value", 0.0001)], title="Gate Attenuation")

amp = Compare(pol["mag"] * 50, thresh, ">")
scl = amp * (1 - mult) + mult

delta = Vectral(pol["mag"] * scl, framesize=size, overlaps=olaps, down=0.35)
delta.ctrl(title="Blur controls")

car = PolToCar(delta, pol["ang"])

fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps, wintype=win).mix(chnls).out()

s.gui(locals())
