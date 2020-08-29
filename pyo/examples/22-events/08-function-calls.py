"""
08-function-calls.py - Using custom algorithms with python function calls.

**EventCall** ::

    EventCall(function, *args, occurrences=inf, stopEventsWhenDone=True)

EventCall calls a function, with any number of arguments (\*args) and uses
its return value for the given parameter. The example below use a function
from the random module, *randrange*, with arguments and a user-defined
function, without argument, to create a rising, then falling, amplitude curve.

"""

import random
from pyo import *

s = Server().boot()

db = -30
dir = 1


def riseFallAmp():
    "Rises and falls amplitude between -30 and -3 dB, 1 db at the time."
    global db, dir
    db += dir
    if db >= -3:
        dir = -1
    elif db < -30:
        dir = 1
    return db


# Midi notes are chosen randomly with a function from the random module,
# while the amplitude change according to the riseFallAmp function's output.
e = Events(
    midinote=EventCall(random.randrange, 48, 72, 3),
    beat=1 / 4.0,
    db=EventCall(riseFallAmp),
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()


s.gui(locals())
