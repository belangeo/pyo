#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-30.
"""
from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

def terrain(size=256, freq=1.25, phase=16):
    l = []
    for i in range(size):
        ph = math.sin(i/float(phase))
        tmp = [math.sin(2 * math.pi * freq * (j/float(size)) + ph) for j in range(size)]
        l.append(tmp)
    return l

TEST = 1
if TEST == 0:
    SIZE = 256
    m = NewMatrix(SIZE, SIZE, terrain(SIZE))
    m.normalize()
    m.view()
    rnd = Randi(0.05, 0.45, .1)
    row = Sine([99.5,99.76], 0, .49, .5)
    col = Sine(12.5, 0, rnd, .5)
    a = MatrixPointer(m, row, col, mul=.25).out()
if TEST == 1:
    SIZE = 256
    mm = NewMatrix(SIZE, SIZE)
    fmind = Sine(.2, 0, 2, 2.5)
    fmrat = Sine(.33, 0, .05, .5)
    aa = FM(carrier=25, ratio=fmrat, index=fmind)
    rec = MatrixRec(aa, mm, 0)
    lfrow = Sine(.1, 0, .24, .25)
    lfcol = Sine(.15, 0, .124, .25)
    row = Sine(10, 0, lfrow, .5)
    col = Sine(1.5, 0, lfcol, .5)
    c = MatrixPointer(mm, row, col).out()

    def func():
        print "poutine"
        
    tr = TrigFunc(rec['trig'], func)
if TEST == 2:
    mat = [[0,1,2,3], [10,11,12,13], [20,21,22,23], [30,31,32,33]]
    mm = NewMatrix(4,4,mat)
    ph = RandInt(4, mul=.25)
    co = Sig(0)
    a = MatrixPointer(mm, co, ph)
    p = Print(a, 1)
            
s.gui(locals())
