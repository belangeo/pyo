#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-03-10.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

t = CurveTable([(0,0), (2048,.5), (4096, .2), (6144,.5), (8192,0)], 
               tension=0, bias=20).normalize()

a = Osc(table=t, freq=2, mul=.25)       
b = Noise(a).out()

# lfo sur bias parameter
c = Sine(.1, 0, 10, 10)
                
def change():
    val = c.get()
    print val
    t.setBias(val)
    t.normalize()
    
p = Pattern(change, .1)
p.play()

t.view()
s.gui(locals())