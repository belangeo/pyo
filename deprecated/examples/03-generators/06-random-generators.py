"""
06-random-generators.py - Overview of some random generators of pyo.

The pyo's random category contains many objects that can be used
for different purposes. This category really deserves to be studied.

In this tutorial, we use three random objects (Choice, Randi, RandInt)
to control the pitches, the amplitude and the tone of a simple synth.

We will come back to random generators when we will talk about musical
algorithms.

"""
from pyo import *

s = Server().boot()

# Two streams of midi pitches chosen randomly in a predefined list.
# The argument `choice` of Choice object can be a list of lists to
# list-expansion.
mid = Choice(choice=[60, 62, 63, 65, 67, 69, 71, 72], freq=[2, 3])

# Two small jitters applied on frequency streams.
# Randi interpolates between old and new values.
jit = Randi(min=0.993, max=1.007, freq=[4.3, 3.5])

# Converts midi pitches to frequencies and applies the jitters.
fr = MToF(mid, mul=jit)

# Chooses a new feedback value, between 0 and 0.15, every 4 seconds.
fd = Randi(min=0, max=0.15, freq=0.25)

# RandInt generates a pseudo-random integer number between 0 and `max`
# values at a frequency specified by `freq` parameter. It holds the
# value until the next generation.
# Generates an new LFO frequency once per second.
sp = RandInt(max=6, freq=1, add=8)
# Creates an LFO oscillating between 0 and 0.4.
amp = Sine(sp, mul=0.2, add=0.2)

# A simple synth...
a = SineLoop(freq=fr, feedback=fd, mul=amp).out()

s.gui(locals())
