#!/usr/bin/env python
# encoding: utf-8
"""
This script records any number of buffers and loop them.

Call r.play() (as many times as you want) to record a buffer from the input mic.

"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=1).boot()

buffer_length = 1 # seconds

objs = []

def cp():
    t2 = t.copy()
    f = Fader(fadein=1, mul=0.5).play()
    pl = Osc(t2, freq=1./buffer_length, interp=4, mul=f).out()
    objs.extend([t2, f, pl])

mic = Input([0,1])
t = NewTable(length=buffer_length, chnls=2)
r = TableRec(mic, table=t, fadetime=0.1)
tr = TrigFunc(r["trig"], function=cp)

s.gui(locals())