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