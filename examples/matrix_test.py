#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-30.
"""
from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

def terrain(size=256, phase=32):
    l = []
    for i in range(size):
        ph = math.sin(i/float(phase))
        amp = 0.5 + (math.cos(math.pi * 2 * (i - size/2) / size) * 0.5)
        tmp = [amp * math.sin(2 * math.pi * 2 * (j/float(size)) + ph) for j in range(size)]
        l.append(tmp)
    return l
    
m = NewMatrix(256, 256, terrain())
#m.view()

row = Sine(50, 0, .5, .5)
col = Sine(25, 0, .5, .5)

a = MatrixPointer(m, row, col).out()

s.gui(locals())