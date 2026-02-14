"""
02-matrix-record.py - Wave Terrain Synthesis of a live FM synthesis.

In this example, an FM synthesis is generated and recorded, row after
row, in the matrix used as the terrain for the synthesis. It is then
scanned with sine waves assigned to x and y positions in the 2D table.

"""

from pyo import *

s = Server(duplex=0).boot()

# Size of a square matrix.
SIZE = 256
# Creates the terrain.
field = NewMatrix(SIZE, SIZE)

# Produces a source sound to record in the terrain.
fmind = Sine(freq=1.2, mul=2, add=2.5)
fmrat = Sine(freq=1.33, mul=0.25, add=0.5)
fm = FM(carrier=10, ratio=fmrat, index=fmind)

# Record the source into the matrix, row after row, until the matrix is full.
rec = MatrixRec(input=fm, matrix=field).play()

# LFOs applied to the amplitude of the x and y pointers reading the matrix.
lfx = Sine(freq=0.07, mul=0.24, add=0.25)
lfy = Sine(freq=0.05, mul=0.2, add=0.25)
# The x and y reading pointers.
x = Sine(freq=[505, 499.9], mul=lfx, add=0.5)
y = Sine(freq=[40.5, 37.6], mul=lfy, add=0.5)

# 2D table lookup with linear interpolation in the matrix.
c = MatrixPointer(matrix=field, x=x, y=y, mul=0.25)

# Lowpass filtering of the output signal.
filt = Tone(input=c, freq=3000).out()


# Signals when the recording is done.
def func():
    print("End of recording")


tr = TrigFunc(rec["trig"], func)

s.gui(locals())
