#!/usr/bin/env python3
# encoding: utf-8
from pyo import *

s = Server().boot()

# Setup up a soundfile player.
sf = SfPlayer('../snds/drumloop.wav', loop=True)
sf2 = sf.mix(2)

gat = Gate(sf2, thresh=-50, risetime=0.005, falltime=0.04, lookahead=4,
           outputAmp=True)
gat.ctrl()

#rev = WGVerb(sf2, feedback=[0.86,0.85], cutoff=[5500,5000], bal=1.0)
rev = Freeverb(sf2, size=[0.86,0.85], damp=0.6, bal=1.0)
cmp = Compress(rev, thresh=-12, ratio=3, risetime=0.005, falltime=0.05, 
               lookahead=4, knee=0.5, mul=gat)

output = Interp(sf2, cmp, interp=0.2).out()
output.ctrl()

s.gui(locals())
