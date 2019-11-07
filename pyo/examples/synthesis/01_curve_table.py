#!/usr/bin/env python
# encoding: utf-8
"""
Curve table variations used as an amplitude envelope.

"""
from pyo import *

s = Server(duplex=0).boot()

t = CurveTable([(0,0),(2048,.5),(4096, .2),(6144,.5),(8192,0)], tension=0, bias=20).normalize()
t.view(title="Waveform at initialization")

# LFO applied on amplitude value of the synths
a = Osc(table=t, freq=2, mul=.1)

synth1 = BrownNoise(a).mix(2).out()
synth2 = FM(carrier=[100,50], ratio=[.495,1.01], index=10, mul=a).out()

# LFO from 0 to 20
c = Sine(.1, 0, 10, 10)

# Modifying the bias parameter 10 times per second
def change():
    # get the current value of the LFO
    val = c.get()
    t.setBias(val)
    t.normalize()
    t.refreshView()

p = Pattern(change, .1).play()

s.gui(locals())
