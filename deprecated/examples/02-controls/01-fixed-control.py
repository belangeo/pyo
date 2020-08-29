"""
01-fixed-control.py - Number as argument.

Audio objects behaviour can be controlled by passing
value to their arguments at initialization time.

"""
from pyo import *

s = Server().boot()
s.amp = 0.1

# Sets fundamental frequency
freq = 200

# Approximates a triangle waveform by adding odd harmonics with
# amplitude proportional to the inverse square of the harmonic number.
h1 = Sine(freq=freq, mul=1).out()
h2 = Sine(freq=freq * 3, phase=0.5, mul=1.0 / pow(3, 2)).out()
h3 = Sine(freq=freq * 5, mul=1.0 / pow(5, 2)).out()
h4 = Sine(freq=freq * 7, phase=0.5, mul=1.0 / pow(7, 2)).out()
h5 = Sine(freq=freq * 9, mul=1.0 / pow(9, 2)).out()
h6 = Sine(freq=freq * 11, phase=0.5, mul=1.0 / pow(11, 2)).out()

# Displays the final waveform
sp = Scope(h1 + h2 + h3 + h4 + h5 + h6)

s.gui(locals())
