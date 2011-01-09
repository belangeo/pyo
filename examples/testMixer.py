#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-12-21.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0, audio="coreaudio").boot()

a = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.2)
b = FM(carrier=200, ratio=[.5013,.4998], index=6, mul=.2)
mm = Mixer(outs=3, chnls=2, time=.025)
fx1 = Disto(mm[0], drive=.9, slope=.9, mul=.2).out()
fx2 = Freeverb(mm[1], size=.8, damp=.8).out()
fx3 = Harmonizer(mm[2], transpo=1, feedback=.75).out()
mm.addInput(0, a)
mm.addInput(1, b)
mm.setAmp(0,0,.5)
mm.setAmp(0,1,.5)
mm.setAmp(1,2,.5)
mm.setAmp(1,1,.5)

s.gui(locals())