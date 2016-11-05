#!/usr/bin/env python
# encoding: utf-8
"""
Music box. 5 voices randomly choosing frequencies over a common scale.

"""

from pyo import *

s = Server(duplex=0).boot()

#--> set list of frequencies
low_freqs = [midiToHz(m+7) for m in [36,43.01,48,55.01,60]]
mid_freqs = [midiToHz(m+7) for m in [60,62,63.93,65,67.01,69,71,72]]
high_freqs = [midiToHz(m+7) for m in [72,74,75.93,77,79.01]]
freqs = [low_freqs,low_freqs,mid_freqs,mid_freqs,high_freqs]
#<--

#-->
chx = Choice(choice=freqs, freq=[1,2,3,3,4])
port = Port(chx, risetime=.001, falltime=.001)
sines = SineLoop(port, feedback=[.06,.057,.033,.035,.016], mul=[.15,.15,.1,.1,.06])
pan = SPan(sines, pan=[0, 1, .2, .8, .5]).out()
#<--

s.gui(locals())
