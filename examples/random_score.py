#!/usr/bin/env python
# encoding: utf-8
"""
Calling Python function from an audio stream with the Score object.
 
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=256, duplex=0).boot()

a = SfPlayer(SNDS_PATH + '/accord.aif', mul=.5).stop()

b = Linseg([(0,0),(.05,1),(.2,0)], mul=.5).stop()
c = Sine(1000,0,b).out()

d = Linseg([(0,0),(.05,1),(.2,0)], mul=.5).stop()
e = Noise(d).out(1)

def event_0():
    a.speed = random.uniform(1.5,2.5)
    a.out()
    
def event_1():
    c.freq = random.uniform(700,1000)
    b.play()
    
def event_2():
    d.play()
    
tr = RandInt(3, 5)
sc = Score(input=tr, fname='event_')

s.gui(locals())