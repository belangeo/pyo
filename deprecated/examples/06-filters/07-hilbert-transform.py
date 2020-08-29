"""
07-hilbert-transform.py - Barberpole-like phasing effect.

This example uses two frequency shifters (based on complex
modulation) linearly shifting the frequency content of a sound.

Frequency shifting is similar to ring modulation, except the
upper and lower sidebands are separated into individual outputs.

"""
from pyo import *


class ComplexMod:
    """
    Complex modulation used to shift the frequency
    spectrum of the input sound.
    """

    def __init__(self, hilb, freq):
        # Quadrature oscillator (sine, cosine).
        self._quad = Sine(freq, [0, 0.25])
        # real * cosine.
        self._mod1 = hilb["real"] * self._quad[1]
        # imaginary * sine.
        self._mod2 = hilb["imag"] * self._quad[0]
        # Up shift corresponds to the sum frequencies.
        self._up = (self._mod1 + self._mod2) * 0.7

    def out(self, chnl=0):
        self._up.out(chnl)
        return self


s = Server().boot()

# Large spectrum source.
src = PinkNoise(0.2)

# Apply the Hilbert transform.
hilb = Hilbert(src)

# LFOs controlling the amount of frequency shifting.
lf1 = Sine(0.03, mul=6)
lf2 = Sine(0.05, mul=6)

# Stereo Single-Sideband Modulation.
wetl = ComplexMod(hilb, lf1).out()
wetr = ComplexMod(hilb, lf2).out(1)

# Mixed with the dry sound.
dry = src.mix(2).out()

s.gui(locals())
