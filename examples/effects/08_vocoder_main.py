#!/usr/bin/env python
# encoding: utf-8
"""
Main script using hand-written vocoder (defined in vocoder_lib.py).

"""
from pyo import *
from vocoder_lib import MyVocoder

s = Server(sr=44100, nchnls=2, duplex=0).boot()

a = SfPlayer('../snds/baseballmajeur_m.aif', loop=True)
b = PinkNoise(.1)

voc = MyVocoder(in1=a, in2=b, base=70, spread=[1.49,1.5], q=10, num=8).out()
voc.ctrl()

s.gui(locals())
