#!/usr/bin/env python
# encoding: utf-8
"""
This script uses a matrix to store STAGES number of short samples
always renewed by recording a source sound with MatrixRecLoop.
A metronomic random playback choose a sample to play between the
first and RND_LEVEL.

"""
from pyo import *

s = Server(duplex=0).boot()

# Length of grains in samples
SIZE = 8192
# Number of successive grains kept in memory
STAGES = 32
# Amount of granularity. Lower value will repeat first grains in memory
RND_LEVEL = 4  # 1 -> STAGES
# percentage of grains that pass
GATE = 100

period = SIZE / s.getSamplingRate()
env = CosTable([(0, 0), (300, 1), (1000, 0.4), (8191, 0)])
matrix = NewMatrix(SIZE, STAGES)

src = SfPlayer("../snds/baseballmajeur_m.aif", speed=1, loop=True, mul=0.3)
m_rec = MatrixRecLoop(src, matrix)

metro = Metro(time=period / 2, poly=2).play()
trig = Percent(metro, GATE)
x = TrigLinseg(trig, [(0, 0), (period, 1)])
y = TrigRandInt(trig, max=RND_LEVEL, mul=1.0 / STAGES)
amp = TrigEnv(trig, table=env, dur=period)

out = MatrixPointer(matrix, x, y, amp).out()

s.gui(locals())
