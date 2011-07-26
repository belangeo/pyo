#!/usr/bin/env python
# encoding: utf-8
"""
To record sound from input mic in the buffer (4 seconds), call:
rec.play() 

The buffer is looped with some funny parameters...
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

tab = NewTable(4)
rec = TableRec(Input(), tab)
pit = Choice(choice=[.5,.5,.75,.75,1,1,1.25,1.5], freq=[3,4])
start = Phasor(freq=.025, mul=tab.getDur()-.5)
dur = Choice(choice=[.0625,.125,.125,.125,.25,.25,.5], freq=4)
a = Looper( table=tab, # table to loop in
            pitch=pit, # transposition
            start=start, # loop start position
            dur=dur, # loop duration
            xfade=20, # crossfade duration in %
            mode=1, # looping mode 
            xfadeshape=0, # crossfade shape
            startfromloop=False, # first start position, False means from beginning of the table
            interp=2 # interpolation method
            ).out()

s.gui(locals())