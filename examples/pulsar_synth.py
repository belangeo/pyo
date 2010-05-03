#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-05-02.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

# simple pulsar synthesis
t = HarmTable([1,0,.3,0,.2,0,0,.1])

a = Phasor(freq=[149.5,150])
frac = Sig(.5)
frac.ctrl()

b = Compare(input=a, comp=frac, mode="<=")
scl = a * b * (1. / frac)
c = Pointer(table=HannTable(), index=scl, mul=.5)
d = Pointer(table=t, index=scl, mul=c).out()

s.gui(locals())