#!/usr/bin/env python
# encoding: utf-8
"""
Created by Guacamole Au Tofu on 2010-03-22.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

a = Xnoise(type=12, freq=8, x1=1, x2=.2, mul=300, add=400)
aa = Print(a, 1)
b = Sine(a, mul=.5).out()

s.gui(locals())


######################
######################
# 0 = uniform
# 1 = linear min
# 2 = linear max
# 3 = triangular
# 4 = exponential min (x1 = slope)
# 5 = exponential max (x1 = slope)
# 6 = biexponential (x1 = bandwidth)
# 7 = cauchy (x1 = bandwidth)
# 8 = weibull (x1 = locator, x2 = shape)
# 9 = gaussian (x1 = locator, x2 = bandwidth)
# 10 = poisson (x1 = gravity center, x2 = compress/expand)
# 11 = walker (x1 = max value, x2 = max step)
#
######################
######################
