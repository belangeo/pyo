#! /usr/bin/env python
# encoding: utf-8
"""
Spectral domain noise reduction.

"""
from pyo import *

s = Server(duplex=0).boot()

snd = "../snds/flute.aif"
chnls = sndinfo(snd)[3]
src = SfPlayer(snd, loop=True, mul=0.5)

size = 1024
olaps = 4

# bin-by-bin amplitude threshold
thresh = Sig(0.1)
thresh.ctrl([SLMap(0.0001, 1, "log", "value", 0.1)], title="Threshold")

# attenuation
mult = Sig(0.1)
mult.ctrl([SLMap(0.0001, 1, "log", "value", 0.1)], title="Attenuation")

fin = FFT(src, size=size, overlaps=olaps)

mag = Sqrt(fin["real"] * fin["real"] + fin["imag"] * fin["imag"])
amp = Compare(mag * 50, thresh, ">")
scl = amp * (1 - mult) + mult
re = fin["real"] * scl
im = fin["imag"] * scl

fout = IFFT(re, im, size=size, overlaps=olaps)
ffout = Mix(fout, chnls, mul=0.5).out()

s.gui(locals())
