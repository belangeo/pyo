#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-22.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

a = Xnoise(7, 4, 0.5, 0.5, 300, 400)
aa = Print(a, 1)
b = Sine(a, mul=.5).out()

s.gui(locals())