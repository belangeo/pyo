"""
05-breakpoints-functions.py - Multi-segments envelopes.

Linseg and Expseg objects draw a series of line segments between 
specified break-points, either linear or exponential.

These objects wait for play() call to start reading the envelope.

They have methods to set loop mode, call pause/play without reset,
and replace the breakpoints.

One can use the graph() method to open a graphical display of the current 
envelope, edit it, and copy the points (in the list format) to the 
clipboard (Menu "File" of the graph display => "Copy all Points ..."). 
This makes it easier to explore and paste the result into the python 
script when happy with the envelope!

"""
from pyo import *
import random

s = Server().boot()

# Randomly built 10-points amplitude envelope.
t = 0
points = [(0.0, 0.0), (2.0, 0.0)]
for i in range(8):
    t += random.uniform(0.1, 0.2)
    v = random.uniform(0.1, 0.9)
    points.insert(-1, (t, v))

amp = Expseg(points, exp=3, mul=0.3)
amp.graph(title="Amplitude envelope")

sig = RCOsc(freq=[150, 151], sharp=0.85, mul=amp)

# A simple linear function to vary the amount of frequency shifting.
sft = Linseg([(0.0, 0.0), (0.5, 20.0), (2, 0.0)])
sft.graph(yrange=(0.0, 20.0), title="Frequency shift")

fsg = FreqShift(sig, shift=sft).out()

rev = WGVerb(sig + fsg, feedback=0.9, cutoff=3500, bal=0.3).out()


def playnote():
    "Start the envelopes to play an event."
    amp.play()
    sft.play()


# Periodically call a function.
pat = Pattern(playnote, 2).play()

s.gui(locals())
