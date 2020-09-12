#!/usr/bin/env python
# encoding: utf-8
"""
This script records any number of buffers and loop them.

Call r.play() (as many times as you want) to record a buffer from the input mic.

"""
from pyo import *

s = Server(duplex=1).boot()

buffer_length = 1  # seconds

objs = []


def cp():
    # make a copy of the table.
    t2 = t.copy()
    # start looping the copied table.
    f = Fader(fadein=0.1, mul=0.5).play()
    pl = Osc(t2, freq=1.0 / buffer_length, interp=4, mul=f).out()
    # save a references to these audio objects to keep them alive.
    objs.extend([t2, f, pl])


mic = Input([0, 1])

# the table for the recording.
t = NewTable(length=buffer_length, chnls=2)
r = TableRec(mic, table=t, fadetime=0.1)
# once the recording is done, cp() is called.
tr = TrigFunc(r["trig"], function=cp)

s.gui(locals())
