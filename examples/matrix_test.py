#!/usr/bin/env python
# encoding: utf-8
"""
Test with PyoMatrixObject.
TEST == 0 : 
    Simple wave terrain synthesis. The terrain is generated with sin functions.
TEST == 1 :
    Wave terrain synthesis of a live recording of FM synthesis in the matrix.
TEST == 2 :
    How to use the matrix to do some algorithmic generation of notes.

"""
from pyo import *
import random, math

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

def terrain(size=256, freq=1, phase=16):
    l = []
    xfreq = 2 * math.pi * freq
    for i in range(size):
        ph = math.sin(i/float(phase))
        tmp = [math.sin(xfreq * (j/float(size)) + ph) for j in range(size)]
        l.append(tmp)
    return l

TEST = 0
if TEST == 0:
    SIZE = 512
    m = NewMatrix(SIZE, SIZE, terrain(SIZE)).normalize()
    m.view()
    rnd = Randi(0.05, 0.45, .1)
    x = Sine([99.5,99.76], 0, .49, .5)
    y = Sine(12.5, 0, rnd, .5)
    a = MatrixPointer(m, x, y, mul=.25).out()
if TEST == 1:
    SIZE = 256
    mm = NewMatrix(SIZE, SIZE)
    fmind = Sine(.2, 0, 2, 2.5)
    fmrat = Sine(.33, 0, .05, .5)
    aa = FM(carrier=10, ratio=fmrat, index=fmind)
    rec = MatrixRec(aa, mm, 0).play()
    lfx = Sine(.1, 0, .24, .25)
    lfy = Sine(.15, 0, .124, .25)
    x = Sine(1000, 0, lfx, .5)
    y = Sine(1.5, 0, lfy, .5)
    c = MatrixPointer(mm, x, y).out()

    def func():
        print "End of recording"
        
    tr = TrigFunc(rec['trig'], func)
if TEST == 2:
    mat = [[0,1,2,3], [10,11,12,13], [20,21,22,23], [30,31,32,33]]
    mm = NewMatrix(4,4,mat)
    y = RandInt(4, freq=2, mul=.25)
    met = Metro(time=4).play()
    x = Counter(met, 0, 4, mul=.25)
    a = MatrixPointer(mm, x, y)
    p = Print(a, 1)
            
s.gui(locals())
