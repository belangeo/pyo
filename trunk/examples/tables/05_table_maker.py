#!/usr/bin/env python
# encoding: utf-8
"""
Creates a new sound table from random chunks of a soundfile.

"""
from pyo import *
import random, os

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

path = "../snds/baseballmajeur_m.aif"
dur = sndinfo(path)[1]

t = SndTable(path, start=0, stop=1)
a = Looper(t, pitch=[1.,1.], dur=t.getDur(), xfade=5, mul=.4).out()

def addsnd():
    start = random.uniform(0, dur*0.7)
    duration = random.uniform(.1, .3)
    pos = random.uniform(0.05, t.getDur()-0.5)
    cross = random.uniform(0.04, duration/2)
    t.insert(path, pos=pos, crossfade=cross, start=start, stop=start+duration)

def gen():
    start = random.uniform(0, dur*0.7)
    duration = random.uniform(.1, .3)
    t.setSound(path, start=start, stop=start+duration)
    for i in range(10):
        addsnd()

    a.dur = t.getDur()

gen()

s.gui(locals())