#!/usr/bin/env python
# encoding: utf-8
"""
Control window.

The ctrl() method (defined in PyoObject) shows a slider window
with which the user can set the value of the object's attributes.
If an audio object is given at a particular argument, the attributes
will not appear in the ctrl window.

"""
from pyo import *

s = Server(duplex=0).boot()

a = FM(carrier=150, ratio=.4958, index=10, mul=.2)
a.ctrl(title="Frequency modulation controls")

b = a.mix(2)
b.out()

s.gui(locals())
