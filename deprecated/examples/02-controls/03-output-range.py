"""
03-output-range.py - The `mul` and `add` attributes.

Almost all audio objects have a `mul` and `add` attributes.
These are defined inside the PyoObject, which is the base
class for all objects generating audio signal. The manual
page of the PyoObject explains all behaviours common to
audio objects.

An audio signal outputs samples as floating-point numbers in
the range -1 to 1. The `mul` and `add` attributes can be used
to change the output range. Common uses are for modulating the
amplitude of a sound or for building control signals like low
frequency oscillators.

A shortcut to automatically manipulate both `mul` and `add`
attributes is to call the range(min, max) method of the
PyoObject. This method sets `mul` and `add` attributes
according to the desired `min` and `max` output values. It
assumes that the generated signal is in the range -1 to 1.

"""
from pyo import *

s = Server().boot().start()

# The `mul` attribute multiplies each sample by its value.
a = Sine(freq=100, mul=0.1)

# The `add` attribute adds an offset to each sample.
# The multiplication is applied before the addition.
b = Sine(freq=100, mul=0.5, add=0.5)

# Using the range(min, max) method allows to automatically
# compute both `mul` and `add` attributes.
c = Sine(freq=100).range(-0.25, 0.5)

# Displays the waveforms
sc = Scope([a, b, c])

s.gui(locals())
