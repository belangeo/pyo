#!/usr/bin/env python
# encoding: utf-8
"""
Ring modulators used as exciter of a waveguide bank.

"""
from pyo import *
import random

s = Server().boot()

tab_m = HarmTable([1, 0, 0, 0, 0, 0.3, 0, 0, 0, 0, 0, 0.2, 0, 0, 0, 0, 0, 0.1, 0, 0, 0, 0, 0.05]).normalize()
tab_p = HarmTable([1, 0, 0.33, 0, 0.2, 0, 0.143, 0, 0.111])


class Ring:
    def __init__(self, fport=250, fmod=100, amp=0.3):
        self.mod = Osc(tab_m, freq=fmod, mul=amp)
        self.port = Osc(tab_p, freq=fport, mul=self.mod)

    def out(self):
        self.port.out()
        return self

    def sig(self):
        return self.port


lf = Sine(0.03, mul=0.5, add=1)
rg = Ring(
    fport=[random.choice([62.5, 125, 187.5, 250]) * random.uniform(0.99, 1.01) for i in range(8)],
    fmod=lf * [random.choice([25, 50, 75, 100]) * random.uniform(0.99, 1.01) for i in range(8)],
    amp=0.1,
)

res = Waveguide(rg.sig(), freq=[30.1, 60.05, 119.7, 181, 242.5, 303.33], dur=30, mul=0.1).out()

s.gui(locals())
