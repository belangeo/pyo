#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-02-22.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

sf = Noise(.5)

TLEN = 1000

inmic = Input()

t1 = NewTable(length=sampsToSec(TLEN), chnls=1)
r1 = TableRec(inmic, table=t1, fadetime=.001)

t2 = NewTable(length=sampsToSec(TLEN), chnls=1)
r2 = TableRec(inmic, table=t2, fadetime=.001)

t3 = NewTable(length=sampsToSec(TLEN), chnls=1)
r3 = TableRec(inmic, table=t3, fadetime=.001)

t4 = NewTable(length=sampsToSec(TLEN), chnls=1)
r4 = TableRec(inmic, table=t4, fadetime=.001)

pha = Sig(0)
pha.ctrl()

t = NewTable(length=sampsToSec(TLEN), chnls=1)
m = TableMorph(pha, t, [t1,t2,t3,t4])
a = Convolve(sf, table=t, size=t.getSize(), mul=.2).out()

s.gui(locals())