"""
07-managing-scales.py - Using EventScale to manage list of notes.

EventScale constructs a list of pitches according to its arguments.

EventScale works similarly to list, ie. uses slicing with square
brackets to access data, with the first element at index 0.

It also accept the len() function, which returns the number of
elements in the scale.

The object can generate most common scales, namely: 'major',
'minorH', 'minorM', 'ionian', 'dorian', 'phrygian', 'lydian',
'mixolydian', 'aeolian', 'locrian', 'wholeTone', 'majorPenta',
'minorPenta', 'egyptian', 'majorBlues', 'minorBlues' and
'minorHungarian'.

The pitches of the scales can be in one of three forms, midi note,
cycles per second or octave.degree notation, according to the 'type'
argument.

"""

import random
from pyo import *

s = Server().boot()

# With type=2, the scale use octave.degree notation.
scl = EventScale(root="C", scale="major", first=4, octaves=2, type=2)


def change():
    "This function asks for a new scale based on new random values."
    scl.root = random.choice(["C", "D", "F"])
    scl.scale = random.choice(["ionian", "dorian", "phrygian"])
    scl.first = random.randint(3, 5)

    # Show the new scale parameters.
    print("Root = %s, scale = %s, first octave = %d" % (scl.root, scl.scale, scl.first))

    # Start a new sequence of events.
    e.play()


# This sequence produces 24 events and stop. We use the 'atend' argument
# to set a function to call when the sequence finishes. This function
# will change the scale used and start a new sequence of 24 notes.
e = Events(
    degree=EventDrunk(scl, maxStep=-2, occurrences=24),
    beat=1 / 4.0,
    db=[-6, -12, -9, -12],
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
    atend=change,
).play()

s.gui(locals())
