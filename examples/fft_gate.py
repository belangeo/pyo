#! /usr/bin/env python
# encoding: utf-8
"""
Spectral domain noise reduction.
created by belangeo, 31-05-2011

"""
from pyo import *

s = Server().boot()

snd = "ounkmaster.aif"
chnls = sndinfo(snd)[3]
src = SfPlayer(snd, loop=True, mul=.5)

size = 1024 
olaps = 4

# bin-by-bin amplitude threshold
thresh = Sig(.1)
thresh.ctrl([SLMap(0.0001,1,"log","value",.1)], title="Threshold")

# attenuation
mult = Sig(.1)
mult.ctrl([SLMap(0.0001,1,"log","value",.1)], title="Attenuation")

fin = FFT(src, size=size, overlaps=olaps)

mag = Sqrt(fin["real"]*fin["real"] + fin["imag"]*fin["imag"])
amp = Compare(mag*50, thresh, ">")
scl = amp * (1 - mult) + mult 
re = fin["real"] * scl
im = fin["imag"] * scl

fout = IFFT(re, im, size=size, overlaps=olaps)
ffout = Mix(fout, chnls, mul=.5).out()

s.gui(locals())

