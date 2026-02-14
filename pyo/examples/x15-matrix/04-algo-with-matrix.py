"""
04-algo-with-matrix.py - MIDI algorithm with a 2D matrix.

This script demonstrates how to use a matrix to do some algorithmic generation of notes.

"""

from pyo import *

s = Server(duplex=0, audio="jack").boot()

# Matrix of 4 rows of 4 MIDI notes each.
mat = [[36, 41, 43, 48], [48, 51, 53, 57], [60, 62, 67, 68], [70, 72, 74, 77]]

notes = NewMatrix(4, 4, mat)

# X position is chosen randomly (2 audio streams for a stereo output).
x = RandInt(max=4, freq=[4, 8], mul=0.25)

# Y position is a simple metronomic count from 0 to 3 (again 2 audio streams).
met = Metro(time=[0.5, 1]).play()
y = Counter(input=met, min=0, max=4, mul=0.25)

# Pick MIDI notes in the matrix.
midi = MatrixPointer(matrix=notes, x=x, y=y)

# Converts MIDI notes to frequencies.
freq = [MToF(midi[0] - 12), MToF(midi[1])]

# Simple synthesizer.
synth = LFO(freq=freq, sharp=0.75, type=2, mul=0.25)
chorus = Chorus(synth).out()

s.gui(locals())
