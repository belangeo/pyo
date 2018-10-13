#!/usr/bin/env python
# encoding: utf-8
"""
Detuned waveguide bank.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

src = SfPlayer("../snds/ounkmaster.aif", loop=True, mul=.1)

lf = Sine(freq=[random.uniform(.005, .015) for i in range(8)],
          mul=[.02,.04,.06,.08,.1,.12,.14,.16],
          add=[50,100,150,200,250,300,350,400])
lf2 = Sine(.005, mul=.2, add=.7)

det_wg = AllpassWG(src, freq=lf, feed=.999, detune=lf2, mul=.25).out()

s.gui(locals())
