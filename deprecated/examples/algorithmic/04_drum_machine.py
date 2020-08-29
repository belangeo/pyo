#!/usr/bin/env python
# encoding: utf-8
"""
Algoritmic rhythm machine...

"""
from pyo import *

# Set this constant to True to loop over preset bank
WITH_PRESET = False

snds3 = [
    "../snds/alum1.wav",
    "../snds/alum2.wav",
    "../snds/alum3.wav",
    "../snds/alum4.wav",
]

presets = [
    [
        [16, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0],
        [16, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0],
        [16, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0],
    ],
    [
        [16, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1],
        [16, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0],
        [16, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0],
    ],
    [
        [16, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1],
        [16, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0],
        [16, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0],
    ],
    [
        [16, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1],
        [16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0],
        [16, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1],
    ],
]

s = Server(duplex=0).boot()

tm = Xnoise(dist=9, freq=2.34, x1=0.5, x2=3, mul=0.0025, add=0.12)
b = Beat(time=tm, w1=[90, 30, 30, 20], w2=[30, 90, 50, 40], w3=[0, 30, 30, 40])
if WITH_PRESET:
    b.setPresets(presets)
    b.recall(0)
b.play()

tabs = SndTable(snds3)
out = TrigEnv(b, table=tabs, dur=b["dur"] * 2, interp=4, mul=b["amp"] * 0.5)
pan = SPan(out, pan=[0.25, 0.4, 0.6, 0.75]).mix(2)
rev = WGVerb(pan, feedback=0.65, cutoff=3500, bal=0.2).out()


def change():
    b.fill()
    b.new()


def chAndCall(x):
    b.fill()
    b.recall(x)


x = 0
pre = 0


def ch():
    global x, pre
    x += 1
    if (x % 5) == 0:
        if WITH_PRESET:
            pre = (pre + 1) % 3
            chAndCall(pre)
        else:
            change()


tt = TrigFunc(b["end"][0], function=ch)

s.gui(locals())
