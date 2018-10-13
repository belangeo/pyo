#!/usr/bin/env python
# encoding: utf-8
"""
Multislider in ctrl window.

If a list of values is given at a particular argument, the ctrl window
will show a multislider to set each value separately.

"""
from pyo import *

s = Server(duplex=0).boot()

a = BrownNoise()
b = Biquadx(a, freq=[200, 400, 800, 1600, 3200, 6400], q=10, type=2).out()
b.ctrl(title="Bank of bandpass filters")

s.gui(locals())
