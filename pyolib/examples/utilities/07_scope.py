#!/usr/bin/env python
# encoding: utf-8
"""
Simple scope example.

"""
from pyo import *

class Scope:

    def __init__(self, input, length=0.05):
        self.input = input
        self.table = NewTable(length=length, chnls=len(input))
        self.table.view(title="Signal Scope")
        self.trig = Metro(time=length).play()
        self.rec = TrigTableRec(self.input, self.trig, self.table)
        self.trf = TrigFunc(self.trig, function=self.update)

    def start(self, x):
        if x: self.trig.play()
        else: self.trig.stop()

    def update(self):
        self.table.refreshView()

s = Server(duplex=1).boot()

CHNLS = 2
LENGTH = 0.05

inp = Input(chnl=list(range(CHNLS)))
scope = Scope(inp, LENGTH)

s.gui(locals())
