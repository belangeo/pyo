"""
07-moving-points.py - Periodically rewrite a break-point function table.

This example shows how we can dynamically modify a function table to create a
curve that varies over time.

Note: Rewriting a large table can produce xruns in the audio output. Usually,
for this kind of processes, we want to keep the table size relatively small.
 
"""
from pyo import *

s = Server().boot()

# Initialize an empty table.
table = LinTable([(0, 0), (255, 0)], size=256)
table.view()

# Two LFOs whose values will change the center points in the table.
lfo1 = Sine(0.1, phase=0.75, mul=0.5, add=0.5)
lfo2 = Sine(0.15, phase=0.75, mul=0.5, add=0.5)


def create_line():
    "Function to create a new line."
    lst = [(0, 0)]  # First point of the table at value 0.
    lst.append((8, lfo1.get()))  # Second point, from first LFO.
    lst.append((128, lfo2.get()))  # Third point, from second LFO.
    lst.append((255, 0))  # Last point of the table at value 0.

    # Replace the table content with the new list of points.
    table.replace(lst)


# Call the function "create_line" every 50 ms.
pat = Pattern(function=create_line, time=0.05).play()

# Little test case...
amp = Osc(table, freq=4, mul=0.4)
synth = RCOsc(freq=[99.5, 100], sharp=0.3, mul=amp).out()

s.gui(locals())
