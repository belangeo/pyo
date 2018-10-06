"""
04-serial-proc.py - Chaining processes on a single source.

This example shows how to chain processes on a single source.
Every processing object (ie the ones that modify an audio source) have
a first argument called "input". This argument takes the audio object
to process.

Note the input variable given to each Harmonizer.

"""
from pyo import *

s = Server().boot()
s.amp = 0.1

# Creates a sine wave as the source to process.
a = Sine().out()

# Passes the sine wave through an harmonizer.
h1 = Harmonizer(a).out()

# Then the harmonized sound through another harmonizer.
h2 = Harmonizer(h1).out()

# And again...
h3 = Harmonizer(h2).out()

# And again...
h4 = Harmonizer(h3).out()

s.gui(locals())
