#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-09-10.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

num = 70
freqs = [random.uniform(200,1000) for i in range(num)]
start1 = [i*.5 for i in range(num)]
fade1 = Fader([1]*num, 1, 5, mul=.05).play(dur=5, delay=start1)
a = SineLoop(freqs, feedback=.05, mul=fade1).out(dur=5, delay=start1)

start2 = 25
dur2 = 30
snds = ['snd_1.aif', 'snd_2.aif', 'snd_3.aif', 'snd_4.aif']
tabs = SndTable(snds) 
fade2 = Fader(5, 10, dur2, mul=.7).play(dur=dur2, delay=start2)
b = Beat(time=.125, w1=[90,30,30,20], w2=[30,90,50,40], w3=[0,30,30,40], poly=1).play(dur=dur2, delay=start2)
out = TrigEnv(b, tabs, b['dur'], mul=b['amp']*fade2).out(dur=dur2, delay=start2)

start3 = 35
dur3 = 30
fade3 = Fader(10, 10, dur3, mul=.1).play(dur=dur3, delay=start3)
fm = FM(carrier=[149,100,151,50]*3, ratio=[.2499,.501,.75003], index=10, mul=fade3).out(dur=dur3, delay=start3)

s.gui(locals())