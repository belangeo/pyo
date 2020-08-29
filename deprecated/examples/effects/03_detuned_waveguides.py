#!/usr/bin/env python
# encoding: utf-8
"""
Detuned waveguide bank.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

src = SfPlayer("../snds/ounkmaster.aif", loop=True, mul=0.1)

lf = Sine(
    freq=[random.uniform(0.005, 0.015) for i in range(8)],
    mul=[0.02, 0.04, 0.06, 0.08, 0.1, 0.12, 0.14, 0.16],
    add=[50, 100, 150, 200, 250, 300, 350, 400],
)
lf2 = Sine(0.005, mul=0.2, add=0.7)

det_wg = AllpassWG(src, freq=lf, feed=0.999, detune=lf2, mul=0.25).out()

s.gui(locals())
