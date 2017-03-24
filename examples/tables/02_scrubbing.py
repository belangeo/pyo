#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function

"""
Scrubbing in a sound window.
Give the focus to the Scrubbing window then click and move the mouse...

"""

from pyo import *
s = Server(duplex=0).boot()

def mouse(mpos):
    print("X = %.2f, Y = %.2f" % tuple(mpos))
    pos.value = mpos[0]
    l, r = 1. - mpos[1], mpos[1]
    mul.value = [l, r]

snd = SndTable('../snds/ounkmaster.aif').normalize()
snd.view(title="Scrubbing window", mouse_callback=mouse)

mul = SigTo([1,1], time=0.1, init=[1,1], mul=.1)
pos = SigTo(0.5, time=0.1, init=0.5, mul=snd.getSize(), add=Noise(5))

gran = Granulator(table=snd, env=HannTable(), pitch=[.999, 1.0011], pos=pos,
                  dur=Noise(.002, .1), grains=40, basedur=.1, mul=mul).out()

s.gui(locals())
