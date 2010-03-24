#!/usr/bin/env python
# encoding: utf-8
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=256, duplex=0).boot()

a = SfPlayer('/Users/olipet/Documents/MaxMSP/@sons/sequenceur/17rap.1').stop()

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
sc = Score(tr)

s.gui(locals())