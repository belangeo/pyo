#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function
"""
Mixing multiple inputs to multiple outputs with fade time.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

# Inputs
a = SfPlayer("../snds/ounkmaster.aif", loop=True, mul=.3)
b = SfPlayer("../snds/flute.aif", loop=True, mul=.1)
c = SfPlayer("../snds/baseballmajeur_m.aif", loop=True, mul=.2)

# 3 outputs mixer, 1 second of amplitude fade time
mm = Mixer(outs=3, chnls=2, time=1)

# Outputs
fx1 = Disto(mm[0], drive=.99, slope=.85, mul=.1).out()
fx2 = Freeverb(mm[1], size=.95, damp=.7, mul=2).out()
fx3 = Harmonizer(mm[2], transpo=-7, feedback=.25, mul=2).out()

# Add inputs to the mixer
mm.addInput(voice=0, input=a)
mm.addInput(voice=1, input=b)
mm.addInput(voice=2, input=c)

# Static assignation of inputs to outputs with amplitude balance
# mm.setAmp(vin=0, vout=0, amp=.15)
# mm.setAmp(vin=0, vout=1, amp=.5)
# mm.setAmp(vin=1, vout=0, amp=.25)
# mm.setAmp(vin=1, vout=2, amp=.5)
# mm.setAmp(vin=2, vout=1, amp=.35)
# mm.setAmp(vin=2, vout=2, amp=.5)

inputs = {0: "ounkmaster", 1: "flute", 2: "baseballmajeur"}
outputs = {0: "disto", 1: "reverb", 2: "harmonizer"}

# Dynamic assignation of inputs to outputs with random amplitude
def assign():
    vin = random.randint(0, 2)
    vout = random.randint(0, 2)
    amp = random.choice([0,0,.25,.33])
    print("%s -> %s, amp = %f" % (inputs[vin], outputs[vout], amp))
    mm.setAmp(vin=vin, vout=vout, amp=amp)

pat = Pattern(function=assign, time=3).play()

s.gui(locals())
