#!/usr/bin/env python
# encoding: utf-8
"""
To record sound from input mic in the buffer (4 seconds), call:
rec.play() 

The buffer is looped with some funny parameters...
"""
from pyo64 import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

tab = NewTable(4)
rec = TableRec(Input(), tab)
pit = Choice(choice=[.5,.5,.75,.75,1,1,1.25,1.5], freq=[3,4])
start = Phasor(freq=.025, mul=tab.getDur())
dur = Choice(choice=[.0625,.125,.125,.125,.25,.25,.5], freq=4)
a = Looper(table=tab, pitch=pit, start=start, dur=dur, xfade=20).out()

s.gui(locals())