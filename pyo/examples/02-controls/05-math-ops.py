"""
05-math-ops.py - Audio objects and arithmetic expresssions.

This script shows how a PyoObject reacts when used inside an
arithmetic expression.

Multiplication, addition, division and substraction can be applied
between pyo objects or between pyo objects and numbers. Doing so
returns a Dummy object that outputs the result of the operation.

A Dummy object is only a place holder to keep track of arithmetic
operations on audio objects.

PyoObject can also be used in expression with the exponent (**),
modulo (%) and unary negative (-) operators.

"""
from pyo import *

s = Server().boot()
s.amp = 0.1

# Full scale sine wave
a = Sine()

# Creates a Dummy object `b` with `mul` attribute
# set to 0.5 and leaves `a` unchanged.
b = a * 0.5
b.out()

# Instance of Dummy class
print(b)

# Computes a ring modulation between two PyoObjects
# and scales the amplitude of the resulting signal.
c = Sine(300)
d = a * c * 0.3
d.out()

# PyoObject can be used with Exponent operator.
e = c ** 10 * 0.4
e.out(1)

# Displays the ringmod and the rectified signals.
sp = Spectrum([d, e])
sc = Scope([d, e])

s.gui(locals())
