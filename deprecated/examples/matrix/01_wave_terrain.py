#!/usr/bin/env python
# encoding: utf-8
"""
Simple wave terrain synthesis. The terrain is generated with sin functions.

"""
from pyo import *
import random, math

s = Server(duplex=0).boot()


def terrain(size=256, freq=1, phase=16):
    l = []
    xfreq = 2 * math.pi * freq
    for j in range(size):
        ph = math.sin(j / float(phase))
        tmp = [math.sin(xfreq * (i / float(size)) + ph) for i in range(size)]
        l.append(tmp)
    return l


SIZE = 512
m = NewMatrix(SIZE, SIZE, terrain(SIZE, freq=2, phase=12)).normalize()
m.view()
rnd = Randi(min=0.05, max=0.45, freq=0.1)
x = Sine(freq=[50, 50.2, 99.5, 99.76, 149.97, 151.34], mul=0.49, add=0.5)
y = Sine(freq=12.5, mul=rnd, add=0.5)
a = MatrixPointer(matrix=m, x=x, y=y, mul=0.05).out()

s.gui(locals())
