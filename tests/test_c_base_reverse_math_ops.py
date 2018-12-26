#!/usr/bin/env python3
# encoding: utf-8
from pyo import *

s = Server().boot()

a = Sine([100, 200])
b = 0.5 * a[0]
c = 0.5 + a[0]
d = 0.5 - a[0]
d = 0.5 / a[0]

# This one can work by calling MULTIPLY macro with -1 as mul argument.
#g = -a[0]

s.gui(locals())
