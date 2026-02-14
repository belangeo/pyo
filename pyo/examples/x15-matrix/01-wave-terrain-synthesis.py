"""
01-wave-terrain-synthesis.py - Simple Wave Terrain Synthesis.

Wave Terrain Synthesis (WTS) is a sound synthesis technique that combines
elements of traditional waveforms and more complex geometric manipulations,
typically for creating new and unique sounds. It was developed as a way to
generate evolving textures and timbres, using an approach that involves
mapping a wave (or set of waves) to a 3D surface.

In this example, the terrain is generated with sin functions with sliding
phase. It is then scanned with sine waves assigned to x and y positions in
the 2D table.

"""

from pyo import *
import random, math

s = Server(duplex=0).boot()


# Function to generate the terrain in a matrix.
def terrain(size=256, freq=1, phase=16):
    mat = []
    xfreq = 2 * math.pi * freq
    for j in range(size):
        ph = math.sin(j / float(phase))
        mat.append([math.sin(xfreq * (i / float(size)) + ph) for i in range(size)])
    return mat


# Size of a square matrix.
SIZE = 512

# Creates the terrain.
field = NewMatrix(SIZE, SIZE, terrain(SIZE, freq=2, phase=12)).normalize()
field.view()

# Creates the x and y moving positions to scan the matrix.
rnd = Randi(min=0.05, max=0.45, freq=0.1)
x = Sine(freq=[50, 50.2, 99.5, 99.76, 149.97, 151.34], mul=0.49, add=0.5)
y = Sine(freq=12.5, mul=rnd, add=0.5)

# Reads interpolated values in the matrix.
a = MatrixPointer(matrix=field, x=x, y=y, mul=0.05).out()

s.gui(locals())
