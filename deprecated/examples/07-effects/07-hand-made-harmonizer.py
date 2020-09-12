"""
07-hand-made-harmonizer.py - Hand-written harmonizer effect.

A harmonizer is a type of pitch shifter that combines the "shifted" pitch
with the original pitch to create a two or more note harmony.

The implementation consists of two overlapping delay lines for which the
reading head speed is tuned to transpose the signal by an amount specified
in semitones.

The Harmonizer object (from pyo) implements an pitch shifter and should
use less CPU than the hand-written version. this example's purpose is only
to show how it works or to be used as a starting point to build an extended
version. 

"""
from pyo import *

s = Server(duplex=0).boot()

# Play a melodic sound and send its signal to the left speaker.
sf = SfPlayer("../snds/flute.aif", speed=1, loop=True, mul=0.5).out()

# Half-sine window used as the amplitude envelope of the overlaps.
env = WinTable(8)

# Length of the window in seconds.
wsize = 0.1

# Amount of transposition in semitones.
trans = -7

# Compute the transposition ratio.
ratio = pow(2.0, trans / 12.0)

# Compute the reading head speed.
rate = -(ratio - 1) / wsize

# Two reading heads out-of-phase.
ind = Phasor(freq=rate, phase=[0, 0.5])

# Each head reads the amplitude envelope...
win = Pointer(table=env, index=ind, mul=0.7)

# ... and modulates the delay time (scaled by the window size) of a delay line.
# mix(1) is used to mix the two overlaps on a single audio stream.
snd = Delay(sf, delay=ind * wsize, mul=win).mix(1)

# The transposed signal is sent to the right speaker.
snd.out(1)

s.gui(locals())
