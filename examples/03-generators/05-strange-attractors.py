"""
05-strange-attractors.py - Non-linear ordinary differential equations.

Oscilloscope part of the tutorial
---------------------------------

A strange attractor is a system of three non-linear ordinary
differential equations. These differential equations define a
continuous-time dynamical system that exhibits chaotic dynamics
associated with the fractal properties of the attractor.

There is three strange attractors in the library, the Rossler,
the Lorenz and the ChenLee objects. Each one can output stereo
signal if the `stereo` argument is set to True.

Use the "voice" slider of the window "Input interpolator" to
interpolate between the three sources.

Audio part of the tutorial
--------------------------

It's possible to create very interesting LFO with strange
attractors. The last part of this tutorial shows the use of
Lorenz's output to drive the frequency of two sine wave oscillators.

"""
from pyo import *

s = Server().boot()

### Oscilloscope ###

# LFO applied to the `chaos` attribute
lfo = Sine(0.2).range(0, 1)

# Rossler attractor
n1 = Rossler(pitch=0.5, chaos=lfo, stereo=True)

# Lorenz attractor
n2 = Lorenz(pitch=0.5, chaos=lfo, stereo=True)

# ChenLee attractor
n3 = ChenLee(pitch=0.5, chaos=lfo, stereo=True)

# Interpolates between input objects to produce a single output
sel = Selector([n1, n2, n3])
sel.ctrl(title="Input interpolator (0=Rossler, 1=Lorenz, 2=ChenLee)")

# Displays the waveform of the chosen attractor
sc = Scope(sel)

### Audio ###

# Lorenz with very low pitch value that acts as a LFO
freq = Lorenz(0.005, chaos=0.7, stereo=True, mul=250, add=500)
a = Sine(freq, mul=0.3).out()

s.gui(locals())
