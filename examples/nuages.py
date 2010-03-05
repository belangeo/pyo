#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-03-02.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

snds = ['/Users/olipet/Documents/MaxMSP/@sons/sequenceur/0%dgam%d' % (i,i) for i in range(1,7)]
num = len(snds)
olaps = 4

tabs = []
trtabs = []
trrnds = []

lfo = Phasor(.025)
cl = Cloud(density=lfo, poly=num*olaps).play()

for i in range(num):
    tabs.append(SndTable(snds[i]))

for j in range(olaps):
    offset = j * num
    for i in range(num):
        index = i + offset
        trrnds.append(TrigRand(cl[index], .5, 2))
        trtabs.append(TrigEnv(cl[index], tabs[i], 1./tabs[i].getRate()*trrnds[index]))

mix = Mix(trtabs, 2)
out = Freeverb(mix, size=.9, damp=.95, bal=.1).out()

s.gui(locals())