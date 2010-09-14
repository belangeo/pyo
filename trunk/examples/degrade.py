#!/usr/bin/env python
# encoding: utf-8

"""
Lot of harmonics synthesis sound passed through a Degrade object.

"""
from pyo import *
s = Server().boot()

wt = HarmTable([1,0,.2,0,.10,0,0,.1,0,0,0,.15]).normalize()
lf = Sine([.15,.2], 0, .35, .5)
a = Osc(table=wt, freq=[50,101,149.7,201.3, 251.8], mul = lf)
t = ChebyTable([1,0,.3,0,.2,0,.143,0,.111])
pp = Print(1.-lf)
b = Lookup(t, a, 1.-lf)
c = Degrade(b, bitdepth=5.967, srscale=0.0233, mul=.5).out()
c.ctrl()

s.gui(locals())