"""
06-randoms.py - Exploring random generators.

This example presents two more generators that randomly choose values.

**EventChoice** ::

    EventChoice(values, occurrences=inf, stopEventsWhenDone=True)

EventChoice randomly pick a new value in a list of possible values.

**EventDrunk** ::

    EventDrunk(values, maxStep=2, occurrences=inf, stopEventsWhenDone=True)

EventDrunk performs a random walk over a list of values. The 'maxStep'
argument determine the larger step in the list the walk can do between
two successive events. A negative 'maxStep' is the same but repetition
are not allowed.

"""

from pyo import *

s = Server().boot()

scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00, 6.02, 6.03, 6.05, 6.07]

# Random walk for the melody.
e1 = Events(
    degree=EventDrunk(scl, maxStep=-2), beat=1 / 4.0, db=-6, attack=0.001, decay=0.05, sustain=0.5, release=0.005,
).play()

# Choose randomly for the bass.
e2 = Events(
    degree=EventChoice(scl), beat=1, db=-6, transpo=-12, attack=0.001, decay=0.05, sustain=0.5, release=0.005,
).play()

s.gui(locals())
