#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-02-22.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

def sampsToMs(x):
    return x / s.getSamplingRate()

#sf = SfPlayer('/Users/olipet/Desktop/sons/cacanne4.aiff', loop=True, speed=1, mul=.5)
sf = Noise(.5)

TLEN = 400

inmic = Input()

t1 = NewTable(length=sampsToMs(TLEN), chnls=1)
rec = TableRec(inmic, table=t1, fadetime=.001)

t2 = NewTable(length=sampsToMs(TLEN), chnls=1)
rec2 = TableRec(inmic, table=t2, fadetime=.001)

t3 = NewTable(length=sampsToMs(TLEN), chnls=1)
rec3 = TableRec(inmic, table=t3, fadetime=.001)

t4 = NewTable(length=sampsToMs(TLEN), chnls=1)
rec4 = TableRec(inmic, table=t4, fadetime=.001)


maps = [SLMap(0,1,"lin","phase",0)]
pha = Phasor(0, 0)
pha.ctrl(maps)


t = NewTable(length=sampsToMs(TLEN), chnls=1)
m = TableMorph(pha, t, [t1,t2])
a = Convolve(sf, table=t, size=t.getSize()).out()

s.gui(locals())