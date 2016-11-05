#!/usr/bin/env python
# encoding: utf-8
"""
This script demonstrates how to use a matrix to do some
algorithmic generation of notes.

"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

mat = [[36,41,43,48], [48,51,53,57], [60,62,67,68], [70,72,74,77]]

mm = NewMatrix(4, 4, mat)

x = RandInt(max=4, freq=[4, 8], mul=.25)
met = Metro(time=[.5, 1]).play()
y = Counter(input=met, min=0, max=4, mul=.25)
a = MatrixPointer(matrix=mm, x=x, y=y)

synth = LFO(freq=[MToF(a[0]-12), MToF(a[1])], sharp=.75, type=2, mul=.25)
chor = Chorus(synth).out()

s.gui(locals())
