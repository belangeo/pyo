#!/usr/bin/env python
# encoding: utf-8
"""
Audio control with LFOs.

"""
from pyo import *

s = Server(duplex=0).boot()

# LFO (sine wave) +/- 5 (mul) around 10 (add), range = 5 -> 15.
# Control the frequency of the square wave LFO.
freqctl = Sine(freq=0.1, mul=5, add=10)

# LFO (square wave) +/- 0.05 (mul) around 0.07 (add), range = 0.02 -> 0.12.
# Control the feedback of the SineLoop oscillator.
feedctl = LFO(freq=freqctl, sharp=0.8, type=2, mul=0.05, add=0.07)

synth = SineLoop(freq=[201.32, 199.76, 200, 201.55], feedback=feedctl, mul=0.1).out()

s.gui(locals())
