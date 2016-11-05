"""
03-complex-resonator.py - Filtering by mean of a complex multiplication.

ComplexRes implements a resonator derived from a complex
multiplication, which is very similar to a digital filter.

It is used here to create a rhythmic chime with varying
resonance.

"""
from pyo import *
import random

s = Server().boot()

# Six random frequencies.
freqs = [random.uniform(1000, 3000) for i in range(6)]

# Six different plucking speeds.
pluck = Metro([.9,.8,.6,.4,.3,.2]).play()

# LFO applied to the decay of the resonator.
decay = Sine(.1).range(.01, .15)

# Six ComplexRes filters.
rezos = ComplexRes(pluck, freqs, decay, mul=5).out()

# Change chime frequencies every 7.2 seconds
def new():
    freqs = [random.uniform(1000, 3000) for i in range(6)]
    rezos.freq = freqs
pat = Pattern(new, 7.2).play()

s.gui(locals())
