#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function
"""
Wave terrain synthesis of a live recording of FM synthesis in the matrix.

"""
from pyo import *

s = Server(duplex=0).boot()

SIZE = 256
mm = NewMatrix(SIZE, SIZE)

fmind = Sine(freq=1.2, mul=2, add=2.5)
fmrat = Sine(freq=1.33, mul=.25, add=.5)
aa = FM(carrier=10, ratio=fmrat, index=fmind)
rec = MatrixRec(input=aa, matrix=mm).play()

lfx = Sine(freq=.07, mul=.24, add=.25)
lfy = Sine(freq=.05, mul=.2, add=.25)
x = Sine(freq=[505,499.9], mul=lfx, add=.5)
y = Sine(freq=[40.5,37.6], mul=lfy, add=.5)
c = MatrixPointer(matrix=mm, x=x, y=y, mul=.25)
filt = Tone(input=c, freq=3000).out()

def func():
    print("End of recording")

tr = TrigFunc(rec['trig'], func)

s.gui(locals())
