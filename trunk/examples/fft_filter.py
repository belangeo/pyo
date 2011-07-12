#! /usr/bin/env python
# encoding: utf-8
"""
Spectral domain filter.
created by belangeo, 31-05-2011

"""
from pyo import *

s = Server().boot()

a = Noise(.1).mix(2)

size = 1024
olaps = 4

fin = FFT(a, size=size, overlaps=olaps, wintype=2)

t = ExpTable([(0,0),(3,0),(10,1),(20,0),(30,.8),(50,0),(70,.6),(150,0),(512,0)], size=size/2)
t.graph(title="Filter shape")
amp = TableIndex(t, fin["bin"])

re = fin["real"] * amp
im = fin["imag"] * amp

fout = IFFT(re, im, size=size, overlaps=olaps, wintype=2).mix(2).out()

s.gui(locals())
