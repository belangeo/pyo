"""
03-parallel-proc.py - Multiple processes on a single source.

This example shows how to play different audio objects side-by-side.
Every processing object (ie the ones that modify an audio source) have
a first argument called "input". This argument takes the audio object
to process.

Note the input variable given to each processing object and the call
to the out() method of each object that should send its samples to the
output.

"""
from pyo import *

s = Server().boot()
s.amp = 0.1

# Creates a sine wave as the source to process.
a = Sine()

# Passes the sine wave through an harmonizer.
hr = Harmonizer(a).out()

# Also through a chorus.
ch = Chorus(a).out()

# And through a frequency shifter.
sh = FreqShift(a).out()

s.gui(locals())
