"""
13-using-audio-objects.py - Using audio objects as generators.

Because Events computes its events in real time, it is possible to use
the current value of audio streams as generators for parameter (even
as generators for EventGenerator's arguments). So, wherever a float or
an EventGenerator is possible, a PyoObject is too.

Considering the wide range of audio signal possibilities pyo is offering,
it is worthy to explore these combinations! 

"""

from pyo import *

s = Server().boot()

# Attack - decay envelope.
env = CosTable([(0, 0.0), (64, 1.0), (8191, 0.0)])

# The pool of notes.
scl = EventScale(root="C", scale="egyptian", first=4, octaves=3)

# LFO on the duration multiplier.
durmul1 = Sine(freq=0.1).range(0.5, 1.5)
# Random on the length of each segment.
segment1 = RandInt(max=6, freq=0.5)
# Random on the distance between segments starting points.
step1 = RandInt(max=6, freq=0.75, add=-3)

e1 = Events(
    midinote=EventSlide(scl, segment1, step1), beat=1 / 4.0, db=[-12, -18, -18], envelope=env, durmul=durmul1,
).play()

# Out-of-phase LFO on the duration multiplier.
durmul2 = Sine(freq=0.1, phase=0.5).range(0.5, 1.5)
segment2 = RandInt(max=6, freq=0.5)
step2 = RandInt(max=6, freq=0.75, add=-3)

e2 = Events(
    midinote=EventSlide(scl, segment2, step2),
    db=[-9, -15, -15],
    envelope=env,
    durmul=durmul2,
    beat=1 / 4.0,
    bpm=60,
    transpo=-12,
).play()

s.gui(locals())
