#!/usr/bin/env python
# encoding: utf-8
"""
3 oscillators FM synthesis class.

"""
from pyo import *
import math

s = Server(duplex=0).boot()

t = HarmTable([1, 0.1])


class FM3:
    def __init__(self, fcar, ratio1, ratio2, index1, index2, out=0):
        self.fmod = fcar * ratio1
        self.fmodmod = self.fmod * ratio2
        self.amod = self.fmod * index1
        self.amodmod = self.fmodmod * index2
        self.modmod = Sine(self.fmodmod, mul=self.amodmod)
        self.mod = Sine(self.fmod + self.modmod, mul=self.amod)
        self.car = Osc(t, fcar + self.mod, mul=0.2)
        self.eq = EQ(self.car, freq=fcar, q=0.707, boost=-12)
        self.out = DCBlock(self.eq).out(out)


a = FM3(125.00, 0.33001, 2.9993, 8, 4, 0)
b = FM3(125.08, 0.33003, 2.9992, 8, 4, 1)
c = FM3(249.89, 0.33004, 2.9995, 8, 4, 0)
d = FM3(249.91, 0.33006, 2.9991, 8, 4, 1)

s.gui(locals())
