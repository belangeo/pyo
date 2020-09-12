"""
09-embedding-generators.py - From words to musical phrases.

If we consider that an EventGenerator is producing a word, then a sequence of
EventGenerator can be used to build a sentence or a phrase. When a generators
encounters another generator as its value, the inner generator is embedded
into the output stream. That is, the inner generator takes over until it runs
out of values to return. Then, the control returns to the outer generator.
There is no limit to the number of levels of embedding.

"""

from pyo import *

s = Server().boot()

# Random walk over 3-note melody segments.
e = Events(
    degree=EventDrunk(
        [
            EventSeq([5.00, 5.02, 5.03], 1),
            EventSeq([5.07, 5.05, 5.03], 1),
            EventSeq([5.07, 5.08, 5.11], 1),
            EventSeq([6.00, 6.07, 6.03], 1),
            EventSeq([6.07, 6.05, 6.07], 1),
            EventSeq([6.07, 6.08, 6.11], 1),
            EventSeq([7.00, 7.03, 7.02], 1),
            EventSeq([7.05, 7.08, 7.07], 1),
            EventSeq([7.08, 7.11, 8.00], 1),
            EventSeq([8.00, 7.07, 7.03], 1),
        ],
        maxStep=EventSeq([2] * 8 + [0] * 4),
        occurrences=inf,
    ),
    beat=1 / 3.0,
    db=[-6, -12, -12],
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# Chord progression: I - II - I - V.
e2 = Events(
    degree=EventSeq(
        [
            EventSeq([4.00, 4.03, 4.07], 2),
            EventSeq([4.02, 4.05, 4.08], 2),
            EventSeq([4.00, 4.03, 4.07], 2),
            EventSeq([3.11, 4.02, 4.07], 2),
        ],
        occurrences=inf,
    ),
    beat=1 / 3.0,
    db=[-6, -12, -12],
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

s.gui(locals())
