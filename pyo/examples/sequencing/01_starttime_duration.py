#!/usr/bin/env python
# encoding: utf-8
"""
Show how to use `dur` and `delay` parameters of play() and out()
methods to sequence events over time.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

num = 70
freqs = [random.uniform(100, 1000) for i in range(num)]
start1 = [i * 0.5 for i in range(num)]
fade1 = Fader([1] * num, 1, 5, mul=0.03).play(dur=5, delay=start1)
a = SineLoop(freqs, feedback=0.05, mul=fade1).out(dur=5, delay=start1)

start2 = 30
dur2 = 40
snds = [
    "../snds/alum1.wav",
    "../snds/alum2.wav",
    "../snds/alum3.wav",
    "../snds/alum4.wav",
]
tabs = SndTable(snds)
fade2 = Fader(0.05, 10, dur2, mul=0.7).play(dur=dur2, delay=start2)
b = Beat(time=0.125, w1=[90, 30, 30, 20], w2=[30, 90, 50, 40], w3=[0, 30, 30, 40], poly=1).play(dur=dur2, delay=start2)
out = TrigEnv(b, tabs, b["dur"], mul=b["amp"] * fade2).out(dur=dur2, delay=start2)

start3 = 45
dur3 = 30
fade3 = Fader(15, 15, dur3, mul=0.02).play(dur=dur3, delay=start3)
fm = FM(carrier=[149, 100, 151, 50] * 3, ratio=[0.2499, 0.501, 0.75003], index=10, mul=fade3).out(
    dur=dur3, delay=start3
)

s.gui(locals())
