#!/usr/bin/env python
# encoding: utf-8
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
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
        trtabs.append(TrigEnv(cl[index], table=tabs[i], dur=1./tabs[i].getRate()*trrnds[index]))

mix = Mix(trtabs, 2)
out = Freeverb(mix, size=.9, damp=.95, bal=.1).out()

s.gui(locals())