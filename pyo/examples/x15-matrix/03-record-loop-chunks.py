"""
03-record-loop-chunks.py - Using a matrix to record multiple chunks of sound.

This script uses a matrix to store STAGES number of short samples always
renewed by recording a source sound with MatrixRecLoop. A metronomic random
playback choose a sample to play between the first and RND_LEVEL stages. A
stage is represented by a single row in the matrix.

"""

from pyo import *

s = Server(duplex=0).boot()

# Length of grains in samples (length of a row in the matrix).
SIZE = 8192
# Number of successive grains kept in memory (number of rows in the matrix).
STAGES = 32
# Amount of granularity. Lower value will repeat first grains in memory more often.
RND_LEVEL = 8  # 1 -> STAGES
# Number of overlaps.
OLAPS = 4
# Percentage of grains that play.
GATE = 100

# Length of a grain in seconds.
period = SIZE / s.getSamplingRate()

# Envelope of the grains.
env = CosTable([(0, 0), (300, 1), (1000, 0.4), (8191, 0)])

# Creates the matrix.
matrix = NewMatrix(SIZE, STAGES)

# The source sound to record in the matrix.
src = SfPlayer("../snds/baseballmajeur_m.aif", speed=1, loop=True, mul=0.3)

# The matrix recorder.
m_rec = MatrixRecLoop(src, matrix)

# Triggers to start the grains.
metro = Metro(time=period / OLAPS, poly=OLAPS).play()

# Allows a percentage of the triggers to pass.
trig = Percent(metro, GATE)

# Generates a ramp from 0 to 1 to read a row in the matrix.
x = TrigLinseg(trig, [(0, 0), (period, 1)])
# Randomly chooses a row, between 0 and RND_LEVEL, in the matrix.
y = TrigRandInt(trig, max=RND_LEVEL, mul=1.0 / STAGES)
# Reads the amplitude envelope.
amp = TrigEnv(trig, table=env, dur=period)

# Reads a row, applies the envelope, and outputs the result.
out = MatrixPointer(matrix, x, y, amp).out()

s.gui(locals())
