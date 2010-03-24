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
s = Server().boot()

wt = HarmTable([1,0,.2,0,.10,0,0,.1,0,0,0,.15])
lf = Sine([.15,.2], 0, .45, .5)
a = Osc(table=wt, freq=[50,101,149.7,201.3, 251.8], mul = lf)
t = ChebyTable([1,0,.3,0,.2,0,.143,0,.111])
b = Lookup(t, a, 1.-lf)
c = Degrade(b, bitdepth=5.967, srscale=0.0233).out()

c.ctrl()

s.gui(locals())