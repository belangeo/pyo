#!/usr/bin/env python
# encoding: utf-8
"""
This script demonstrates how to use pyo to do offline batch processing.

"""
import os
from pyo import *
s = Server(audio="offline").boot()

# input sound
sndpath = SNDS_PATH + "/accord.aif"
# output folder
recpath = os.path.join(os.path.expanduser("~"), "Desktop", "pyo_batch")
if not os.path.isdir(recpath):
    os.mkdir(recpath)

# output file duration
dur = sndinfo(sndpath)[1]

NUM = 10
for i in range(NUM):
    note = 12 + i
    noteFreq = midiToHz(note)
    s.recordOptions(dur=dur+.1, filename=os.path.join(recpath, "file_%02d.wav" % note), fileformat=0, sampletype=0)

    ### processing goes here ###
    osc = Sine(freq=noteFreq, mul=i*0.01+.02, add=1)
    a = SfPlayer(sndpath, speed=osc, loop=False, mul=0.7).mix(2).out()

    ############################

    s.start() # s.stop() is automatically called when rendering is done
    # do not reboot the server after the last pass
    if i < (NUM-1):
        s.shutdown()
        s.boot()

print "Batch processing done"

