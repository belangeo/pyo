"""
11-filters.py - Modifying generator's output with filters.

The EventGenerator class contains many methods that can be used to filter
the sequence of output values in order to get the desired results. These
filters return an EventFilter object, which is also a child of EventGenerator,
that perform the operation.

Available filter methods are:

    - floor:
        Return an EventFilter computing the largest integer less than or
        equal to its input value.
    - ceil:
        Return an EventFilter computing the smallest integer greater than
        or equal to its input value.
    - round:
        Return an EventFilter computing the nearest integer to its input value.
    - snap:
        Return an EventFilter which choose the nearest value of its input
        value in a list of choices.
    - deviate:
        Return an EventFilter which randomly move, up or down, its input value.
    - clip:
        Return an EventFilter which clips its input value between predefined
        limits.
    - scale:
        Return an EventFilter which maps its input value, in the range 0 to 1,
        to an output range, with a scaling curve.
    - rescale:
        Return an EventFilter which maps its input value, given in an input
        range, to an output range with a scaling curve.
    - iftrue:
        Return an EventFilter which compares its input value to a comparison
        value and outputs it if the comparison is True.

"""

from random import weibullvariate
from pyo import *

s = Server().boot()

scl = EventScale(root="C", scale="wholeTone", first=4, octaves=3)

# Takes a Weibull distribution, scales the values between 48 and 84, rounds
# them to the nearest integer, then snaps to the nearest value in the given
# scale and uses the result as midi note if it's below 84.
e = Events(
    midinote=EventCall(weibullvariate, 0.5, 0.5).scale(48, 84, 1).round().snap(scl).iftrue("<", 84),
    beat=1 / 4.0,
    db=-6,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# Rescale values from a pink noise to the range 48 to 72 and snap to the given scale.
e2 = Events(
    midinote=EventNoise(type=1).rescale(-1, 1, 48, 72, 1).snap(scl),
    beat=1 / 4.0,
    db=-6,
    transpo=12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

s.gui(locals())
