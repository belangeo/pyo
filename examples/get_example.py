#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Bélanger on 2010-02-25.
"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

a = Sine([.1,.2,.3], 0, 100, 500)
b = Sine(a, mul=.5).out()

def print_val():
    print a.get(True)
    
p = Pattern(print_val, .25)
p.play()    

s.gui(locals())