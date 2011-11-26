#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2011-11-25.
"""
from pyo import *

s = Server().boot()

snd = 'ounkmaster.aif'

chnls = sndinfo(snd)[3]
size = 4096
olaps = 8
win = 5

snd = SfPlayer(snd, loop=True, mul=.5)

fin = FFT(snd, size=size, overlaps=olaps, wintype=win)

pol = CarToPol(fin["real"], fin["imag"])

# bin-by-bin amplitude threshold
thresh = Sig(.1)
thresh.ctrl([SLMap(0.0001,1,"log","value",0.3)], title="Threshold")

# attenuation
mult = Sig(.1)
mult.ctrl([SLMap(0.0001,1,"log","value",0.0001)], title="Attenuation")

amp = Compare(pol["mag"]*50, thresh, ">")
scl = amp * (1 - mult) + mult 

delta = Vectral(pol["mag"]*scl, framesize=size, overlaps=olaps, down=.35)
delta.ctrl()

car = PolToCar(delta, pol["ang"])

fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps, wintype=win).mix(chnls).out()

s.gui(locals())