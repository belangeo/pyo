#!/usr/bin/env python
# encoding: utf-8
"""
Exponential cloud of sounds...

"""

from pyo import *

s = Server(duplex=0).boot()

snds = ["../snds/snd_%d.aif" % i for i in range(1, 7)]
num = len(snds)
olaps = 4

tabs = []
trtabs = []
trrnds = []

lfo = Expseg([(0, 0), (20, 100)], loop=True, exp=5).play()
cl = Cloud(density=lfo, poly=num * olaps).play()

for i in range(num):
    tabs.append(SndTable(snds[i]))

for j in range(olaps):
    offset = j * num
    for i in range(num):
        index = i + offset
        trrnds.append(TrigChoice(cl[index], choice=[0.5, 0.75, 1, 1.25, 1.5, 2]))
        trtabs.append(TrigEnv(cl[index], table=tabs[i], dur=1.0 / tabs[i].getRate() * trrnds[index], interp=4,))

mix = Mix(trtabs, voices=2)
out = Freeverb(mix, size=0.9, damp=0.95, bal=0.1, mul=0.3).out()

s.gui(locals())
