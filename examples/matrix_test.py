#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-30.
"""
from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

def terrain(size=256, phase=512):
    l = []
    for i in range(size):
        ph = math.sin(i*(i*.125)/float(phase))
        amp = 0.5 + (math.cos(math.pi * 2 * (i - size/2) / size) * 0.5)
        tmp = [amp * math.sin(2 * math.pi * 2 * (j/float(size)) + ph) for j in range(size)]
        l.append(tmp)
    return l

def noisy(size=256, phase=64):
    l = []
    for i in range(size):
        ph = math.sin(i/float(phase))
        amp = 0.5 + (math.cos(math.pi * 2 * (i - size/2) / size) * 0.5)
        tmp = [amp * math.sin(2 * math.pi * 2 * (j/float(size)) + (ph*random.uniform(0,.25))) for j in range(size)]
        l.append(tmp)
    return l



m = NewMatrix(256, 256, terrain(256))
m.view()

lfrow = Sine(.1, 0, .124, .25)
lfcol = Sine(.15, 0, .24, .25)
row = Sine(124.5, 0, lfrow, .5)
col = Sine(250, 0, .25, lfcol)

a = MatrixPointer(m, row, col).out()

s.gui(locals())
