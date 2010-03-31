#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-30.
"""
from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

def terrain(size=256, phase=23):
    l = []
    for i in range(size):
        ph = math.sin(i/float(phase))
        amp = 0.5 + (math.cos(math.pi * 2 * (i - size/2) / size) * 0.5)
        tmp = [amp * math.sin(2 * math.pi * 2 * (j/float(size)) + ph) for j in range(size)]
        l.append(tmp)
    return l

"""
    SIZE = 512
    m = NewMatrix(SIZE, SIZE, terrain(SIZE))
    m.normalize()
    m.view()

    lfrow = Sine(.1, 0, .124, .25)
    lfcol = Sine(.15, 0, .124, .25)
    row = Sine([124.5,124.76], 0, lfrow, .5)
    col = Sine(250, 0, .25, lfcol)

    a = MatrixPointer(m, row, col).out()
"""

SIZE = 512
mm = NewMatrix(SIZE, SIZE)

fmind = Sine(.2, 0, 5, 6)
aa = FM(index=fmind)

def trigueux():
    aa.speed = random.uniform(.25, 1)
    print "sdfsdfsdfsdfsdf"
    
rec = MatrixRec(aa, mm, 1)
tr = TrigFunc(rec["trig"], trigueux)

lfrow = Sine(.1, 0, .24, .25)
lfcol = Sine(.15, 0, .124, .25)
row = Sine(1, 0, lfrow, .5)
col = Sine(1.5, 0, lfcol, .5)

c = MatrixPointer(mm, row, col).out()
    
s.gui(locals())
