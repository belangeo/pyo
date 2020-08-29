"""
04-phasing.py - The phasing effect.

The Phaser object implements a variable number of second-order
allpass filters, allowing to quickly build complex phasing effects.

    A phaser is an electronic sound processor used to filter a signal
    by creating a series of peaks and troughs in the frequency spectrum.
    The position of the peaks and troughs of the waveform being affected
    is typically modulated so that they vary over time, creating a sweeping
    effect. For this purpose, phasers usually include a low-frequency
    oscillator. - https://en.wikipedia.org/wiki/Phaser_(effect)

A phase shifter unit can be built from scratch with the Allpass2 object,
which implement a second-order allpass filter that create, when added to
the original source, one notch in the spectrum.

"""
from pyo import *

s = Server().boot()

# Simple fadein.
fade = Fader(fadein=0.5, mul=0.2).play()

# Noisy source.
a = PinkNoise(fade)

# These LFOs modulate the `freq`, `spread` and `q` arguments of
# the Phaser object. We give a list of two frequencies in order
# to create two-streams LFOs, therefore a stereo phasing effect.
lf1 = Sine(freq=[0.1, 0.15], mul=100, add=250)
lf2 = Sine(freq=[0.18, 0.13], mul=0.4, add=1.5)
lf3 = Sine(freq=[0.07, 0.09], mul=5, add=6)

# Apply the phasing effect with 20 notches.
b = Phaser(a, freq=lf1, spread=lf2, q=lf3, num=20, mul=0.5).out()

s.gui(locals())
