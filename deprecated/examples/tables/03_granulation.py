#!/usr/bin/env python
# encoding: utf-8

"""
Classical granulation stretching...

"""

from pyo import *

s = Server(duplex=0).boot()

snd = SndTable("../snds/baseballmajeur_m.aif")
snd.view()

pos = Phasor(freq=snd.getRate() * 0.25, mul=snd.getSize(), add=Noise(3))
dur = Noise(0.001, 0.1)

gran = Granulator(
    table=snd, env=WinTable(7), pitch=[0.999, 1.0011], pos=pos, dur=dur, grains=40, basedur=0.1, mul=0.05,
).out()

s.gui(locals())
