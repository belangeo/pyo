"""
12-sharing-values.py - Reading and using other parameter's value.

So far, we've seen events that are independent of each other. They
are not aware of the data produced by the others. Making these data
available adds musical intelligence to our processes. There is two
ways to share values with events. The first one is to read a value
produced by the same Events object where we want to use it. The 
second way is to read the data from a parameter of another Events
object.

In both cases, if the parameter that is read is not yet processed by
the Events object, it will first be computed, and stored for further
uses, and then shared.

We use the `EventKey(key, master=None)` to read the value of another
parameter.

"""

from pyo import *

s = Server().boot()

scl = EventScale(root="C", scale="major", first=4, octaves=3)

e = Events(
    midinote=EventDrunk(scl, maxStep=2),
    # The higher the pitch, the lower the amplitude.
    db=EventKey("midinote").rescale(48, 84, -3, -24, 1),
    beat=1 / 4.0,
    transpo=12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

e2 = Events(  # Define arguments (unused in the instrument) to build phrases.
    line1=EventSeq([0, 4, 2, 0, 4, 2, 0, 2, 4, 6, 4, 2], occurrences=2, stopEventsWhenDone=False),
    line2=EventSeq([0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1], occurrences=2, stopEventsWhenDone=False),
    # Use previously defined arguments to create a sequence of pitch phrases.
    midinote=EventIndex(scl, EventSeq([EventKey("line1"), EventKey("line2")])),
    # Use the db amplitude from the previous Events object (`master` argument).
    db=EventKey("db", master=e),
    beat=1 / 4.0,
).play()

s.gui(locals())
