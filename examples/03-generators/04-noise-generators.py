"""
04-noise-generators.py - Different pseudo-random noise generators.

There are three noise generators (beside random generators that
will be covered later) in the library. These are the classic
white noise, pink noise and brown noise.

Noise:
White noise generator, flat spectrum.

PinkNoise:
Pink noise generator, 3dB rolloff per octave.

BrownNoise:
Brown noise generator, 6dB rolloff per octave.

Use the "voice" slider of the window "Input interpolator" to
interpolate between the three sources.

"""
from pyo import *

s = Server().boot()

# White noise
n1 = Noise(0.3)

# Pink noise
n2 = PinkNoise(0.3)

# Brown noise
n3 = BrownNoise(0.3)

# Interpolates between input objects to produce a single output
sel = Selector([n1, n2, n3]).out()
sel.ctrl(title="Input interpolator (0=White, 1=Pink, 2=Brown)")

# Displays the spectrum contents of the chosen source
sp = Spectrum(sel)

s.gui(locals())
