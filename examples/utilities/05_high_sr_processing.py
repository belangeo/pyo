#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function
"""
This script demonstrates how to use pyo to apply processing on an
up sampled sound. Steps are:

1 - Up sampling the source.
2 - Offline processing on the up sampled sound.
3 - Down sampled the processed sound.

"""
import os
from pyo import *

UP = 8
DOWN = 8
SIZE = 512
IN_SND = "../snds/baseballmajeur_m.aif"
UP_SND = os.path.join(os.path.expanduser("~"), "Desktop", "baseballmajeur_up_%i.aif" % UP)
PROC_SND = os.path.join(os.path.expanduser("~"), "Desktop", "baseballmajeur_disto.aif")
DOWN_SND = os.path.join(os.path.expanduser("~"), "Desktop", "baseballmajeur_disto_down.aif")
DUR = sndinfo(IN_SND)[1]
SR = sndinfo(IN_SND)[2]

print("Up sampling %i times..." % UP)
upsamp(IN_SND, UP_SND, UP, SIZE)

print("Apply distortion at a sampling rate of %i Hz." % (SR*UP))
s = Server(sr=SR*UP, nchnls=1, duplex=0, audio="offline").boot()
s.recordOptions(dur=DUR+.1, filename=PROC_SND, fileformat=0, sampletype=0)
sf = SfPlayer(IN_SND, loop=False, interp=4, mul=0.7)
dist = Disto(sf, drive=0.75, slope=0.7, mul=0.3)
filt = Biquad(dist, freq=8000, q=0.7, type=0).out()
s.start()

print("Down sampling %i times..." % DOWN)
downsamp(PROC_SND, DOWN_SND, DOWN, SIZE)

os.remove(UP_SND)
os.remove(PROC_SND)

print("Done")
