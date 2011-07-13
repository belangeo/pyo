#!/usr/bin/env python
# encoding: utf-8

from pyo import *
from vocoder_lib import SimpleVocoder, Vocoder 
                
s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
a1 = SfPlayer(SNDS_PATH + "/accord.aif", loop=True)
b = Noise()
b1 = SineLoop(freq=100, feedback=.2, mul=.5)

lfo = Sine(.05, 0, 50, 100)
voc = Vocoder(in1=a, in2=b, base=lfo, spread=[1.2,1.22], num=20, mul=.3).out()
    
s.gui(locals())
