#!/usr/bin/env python
# encoding: utf-8

"""
Live convolution example. A white noise is filtered by four impulses
taken from the input mic. Call r1.play(), r2.play(), r3.play(), r4.play()
while making some noise in the mic to fill the impulse response tables.
The slider handles the morphing between the four impulses.

Call t1.view(), t2.view(), t3.view(), t4.view() to view impulse response tables.

Circular convolution is very expensive, so keep TLEN (in samples) small.

"""
from pyo import *

s = Server(duplex=1).boot()

sf = Noise(0.5)

TLEN = 1024

inmic = Input()

t1 = NewTable(length=sampsToSec(TLEN), chnls=1)
r1 = TableRec(inmic, table=t1, fadetime=0.001)

t2 = NewTable(length=sampsToSec(TLEN), chnls=1)
r2 = TableRec(inmic, table=t2, fadetime=0.001)

t3 = NewTable(length=sampsToSec(TLEN), chnls=1)
r3 = TableRec(inmic, table=t3, fadetime=0.001)

t4 = NewTable(length=sampsToSec(TLEN), chnls=1)
r4 = TableRec(inmic, table=t4, fadetime=0.001)

pha = Sig(0)
pha.ctrl(title="Impulse responses morphing")

t = NewTable(length=sampsToSec(TLEN), chnls=1)
m = TableMorph(pha, t, [t1, t2, t3, t4])
a = Convolve(sf, table=t, size=t.getSize(), mul=0.1).mix(2).out()

s.gui(locals())
