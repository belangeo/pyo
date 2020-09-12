"""
05-complex-sequences.py - Exploring generators that sequence values.

This example explores two more generators that sequence values.

**EventSlide** ::

    EventSlide(values, segment, step, startpos=0, wraparound=True,
               occurrences=inf, stopEventsWhenDone=True)

EventSlide plays sub-melodies of length 'segment' and then moves by 'step'
positions from the beginning of the last segment to start another one, and
so on. The argument 'step' can be negative to move backward and the 'startpos'
can also be negative to start from the end of the list. If 'wraparound' is True,
indexing wraps around if goes past beginning or end. If False, the playback
stops if it goes outside the list bounds.

**EventIndex** ::

    EventIndex(values, index, occurrences=inf, stopEventsWhenDone=True)

EventIndex uses an 'index' parameter to read specific positions into the
list 'values'. This is useful, for instance, to read a scale using degree
values.

"""

from pyo import *

s = Server().boot()

scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00, 6.02, 6.03, 6.05, 6.07]

# 3 notes segments, moving forward 1 index each iteration.
e = Events(
    degree=EventSlide(scl, segment=3, step=1, startpos=0),
    beat=1 / 2.0,
    db=-6,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# 4 notes segments, moving backward (from the end) 2 indexes each iteration.
e2 = Events(
    degree=EventSlide(scl, segment=4, step=-2, startpos=-1),
    beat=1,
    db=-12,
    transpo=12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

# Arpeggio on the root chord.
e3 = Events(
    degree=EventIndex(scl, EventSeq([0, 4, 2, 0, 4, 2, 0, 2, 4, 7, 4, 2])),
    db=EventSeq([-6, -12, -12]),
    beat=1 / 4.0,
    transpo=-12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

s.gui(locals())
