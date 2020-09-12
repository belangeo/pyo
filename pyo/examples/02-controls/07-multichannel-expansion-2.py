"""
07-multichannel-expansion-2.py - Extended multichannel expansion.

When using multichannel expansion with lists of different lengths,
the longer list is used to set the number of streams and smaller
lists will be wrap-around to fill the holes.

This feature is very useful to create complex sonorities.

"""
from pyo import *

s = Server().boot()

# 12 streams with different combinations of `freq` and `ratio`.
a = SumOsc(
    freq=[100, 150.2, 200.5, 250.7],
    ratio=[0.501, 0.753, 1.255],
    index=[0.3, 0.4, 0.5, 0.6, 0.7, 0.4, 0.5, 0.3, 0.6, 0.7, 0.3, 0.5],
    mul=0.05,
)

# Adds a stereo reverberation to the signal
rev = Freeverb(a.mix(2), size=0.80, damp=0.70, bal=0.30).out()

s.gui(locals())
