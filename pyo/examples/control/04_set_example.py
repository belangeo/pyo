#!/usr/bin/env python
# encoding: utf-8
"""
The set() method of PyoObject lets change parameter's value with portamento.
call go() to start changes.
call reset() to get back initial values.

"""
from pyo import *
from random import uniform

s = Server(duplex=0).boot()

a = FM(carrier=[uniform(197,203) for i in range(10)],
       ratio=[uniform(0.49,0.51) for i in range(10)],
       index=[uniform(10,15) for i in range(10)], mul=.05).out()

def go():
    a.set("carrier", [uniform(395,405) for i in range(10)], 20)
    a.set("ratio", [uniform(0.49,0.51) for i in range(10)], 18)
    a.set("index", [uniform(5,9) for i in range(10)], 23)

def reset():
    a.carrier = [uniform(197,203) for i in range(10)]
    a.ratio = [uniform(0.49,0.51) for i in range(10)]
    a.index = [uniform(8,12) for i in range(10)]

s.gui(locals())
