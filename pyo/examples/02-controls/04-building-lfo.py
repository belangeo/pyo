"""
04-building-lfo.py - Audio control of parameters.

One of the most important thing with computer music is the
trajectories taken by parameters over time. This is what
gives life to the synthesized sound.

One way to create moving values is by connecting a low
frequency oscillator to an object's attribute. This script
shows that process.

Other possibilities that will be covered later use random
class objects or feature extraction from an audio signal.

"""
from pyo import *

s = Server().boot()

# Creates a noise source
n = Noise()

# Creates an LFO oscillating +/- 500 around 1000 (filter's frequency)
lfo1 = Sine(freq=0.1, mul=500, add=1000)
# Creates an LFO oscillating between 2 and 8 (filter's Q)
lfo2 = Sine(freq=0.4).range(2, 8)
# Creates a dynamic bandpass filter applied to the noise source
bp1 = ButBP(n, freq=lfo1, q=lfo2).out()

# The LFO object provides more waveforms than just a sine wave

# Creates a ramp oscillating +/- 1000 around 12000 (filter's frequency)
lfo3 = LFO(freq=0.25, type=1, mul=1000, add=1200)
# Creates a square oscillating between 4 and 12 (filter's Q)
lfo4 = LFO(freq=4, type=2).range(4, 12)
# Creates a second dynamic bandpass filter applied to the noise source
bp2 = ButBP(n, freq=lfo3, q=lfo4).out(1)

s.gui(locals())
