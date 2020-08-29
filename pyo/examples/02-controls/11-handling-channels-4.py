"""
11-handling-channels-4.py - Explicit control of the physical outputs.

If `chnl` is a list, successive values in the list will be assigned
to successive streams.

"""
from pyo import *

# Creates a Server with 8 channels
s = Server(nchnls=8).boot()

amps = [0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4]

# Generates 8 sine waves with
# increasing amplitudes
a = Sine(freq=500, mul=amps)

# Sets the output channels ordering
a.out(chnl=[3, 4, 2, 5, 1, 6, 0, 7])

s.gui(locals())
