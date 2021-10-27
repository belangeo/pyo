#! /usr/bin/env python
# encoding: utf-8
"""
Spectral domain filter.

"""
from pyo import *

s = Server(duplex=0).boot()

a = Noise(0.1).mix(2)

size = 1024
olaps = 4
filter_init = [
    (0, 0.0000),
    (5, 0.9337),
    (13, 0.0000),
    (21, 0.4784),
    (32, 0.0000),
    (37, 0.1927),
    (size // 2, 0.0000),
]

fin = FFT(a, size=size, overlaps=olaps, wintype=2)

t = ExpTable(filter_init, size=size // 2)
t.graph(title="Filter shape")
amp = TableIndex(t, fin["bin"])

re = fin["real"] * amp
im = fin["imag"] * amp

fout = IFFT(re, im, size=size, overlaps=olaps, wintype=2).mix(2).out()

s.gui(locals())
