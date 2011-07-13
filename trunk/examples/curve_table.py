#!/usr/bin/env python
# encoding: utf-8

from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

t = CurveTable([(0,0), (2048,.5), (4096, .2), (6144,.5), (8192,0)], 
               tension=0, bias=20).normalize()

a = Osc(table=t, freq=2, mul=.1)       
b = Noise(a).mix(2).out()

#  LFO from 0 to 20
c = Sine(.1, 0, 10, 10)
 
# Modifying the bias parameter 10 times per second                
def change():
    # get the current value of the LFO
    val = c.get()
    print val
    t.setBias(val)
    t.normalize()
    
p = Pattern(change, .1)
p.play()

t.view(title="Waveform at initialization")
s.gui(locals())