#!/usr/bin/env python
# encoding: utf-8
from __future__ import print_function
"""
This script demonstrates how to use pyo to do synthesis batch generation.

"""
import os
from pyo import *
s = Server(duplex=0, audio="offline")

# output folder
output_folder = os.path.join(os.path.expanduser("~"), "pyo_batch_synth")
if not os.path.isdir(output_folder):
    os.mkdir(output_folder)

# output file duration
dur = 2

NUM = 12
for i in range(NUM):
    s.boot()
    note = 60 + i
    noteFreq = midiToHz(note)
    s.recordOptions(dur=dur+.1, filename=os.path.join(output_folder, "file_%02d.wav" % note),
                    fileformat=0, sampletype=0)

    ### processing goes here ###
    env = Adsr(attack=0.005, decay=0.15, sustain=0.7, release=1.7, dur=dur).play()
    qenv = Pow(env, 4, mul=0.8)
    osc1 = SineLoop(freq=noteFreq, feedback=0.075, mul=qenv).out()
    osc2 = SineLoop(freq=noteFreq*1.01, feedback=0.075, mul=qenv).out(1)

    # start th render
    s.start()
    # cleanup
    s.shutdown()

print("Batch processing done")

