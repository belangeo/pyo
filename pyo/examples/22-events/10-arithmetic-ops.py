"""
10-arithmetic-ops.py - Using arithmetic with event generators.

Arithmetic operators can be used with EventGenerator objects to control
their behaviour (and change the pattern of the sequence of values).

Available operators are:

`+` : addition

`-` : substraction

`*` : multiplication

`/` : division

`**`: exponent

`%` : modulo (remaining of the division)

`//`: quantizer (returns te nearest multiple of its argument)

Arithmetic can be done with event generators as both operand and operator.

"""

from pyo import *

s = Server().boot()

# Half-semitone scale (the octave is divided in 24 equally-spaced steps).
# The integer (a semitone in midi note) is divided by 2, which mean that
# there are two values inside a single integer.
e = Events(
    midinote=EventDrunk(list(range(48)), maxStep=-2) / 2.0 + 72,
    beat=1 / 4.0,
    db=-12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# Quantize to multiples of a minor third.
e2 = Events(
    midinote=EventCall(random.uniform, 60, 72) // 3,
    beat=1 / 4.0,
    db=-12,
    durmul=1,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# EventGenerator as arithmetic operator. Kind of ostinato.
e3 = Events(
    midinote=EventSeq([48, 50, 52]) - EventSeq([0, 2, 4, 6]),
    beat=1 / 4.0,
    db=-12,
    durmul=1,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

s.gui(locals())
