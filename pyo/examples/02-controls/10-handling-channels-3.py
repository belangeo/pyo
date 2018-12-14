"""
10-handling-channels-3.py - Random multichannel outputs.

If `chnl` is negative, streams begin at 0, increment the output
number by inc and wrap around the global number of channels.
Then, the list of streams is scrambled.

"""
from pyo import *

# Creates a Server with 8 channels
s = Server(nchnls=8).boot()

amps = [.05,.1,.15,.2,.25,.3,.35,.4]

# Generates 8 sine waves with
# increasing amplitudes
a = Sine(freq=500, mul=amps)

# Shuffles physical output channels
a.out(chnl=-1)

s.gui(locals())
