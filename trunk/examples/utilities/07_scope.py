#!/usr/bin/env python
# encoding: utf-8
"""
Simple scope example.

"""
from pyo import *

s = Server(duplex=1).boot()

CHNLS = 2
LENGTH = 0.05

t = NewTable(length=LENGTH, chnls=CHNLS)
inp = Input(chnl=range(CHNLS))
m = Metro(time=LENGTH).play()
rec = TrigTableRec(input=inp, trig=m, table=t)

t.view(title="Oscilloscope")
def update():
    t.refreshView()
    
tf = TrigFunc(input=m, function=update)

s.gui(locals())
