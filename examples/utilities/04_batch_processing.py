#!/usr/bin/env python
# encoding: utf-8
"""
This script demonstrates how to use pyo to do offline batch processing.

"""
from pyo import *
import os

s = Server(audio="offline")

# path to your sound folder
folder_path = SNDS_PATH
# path to the processed sounds folder (user's home directory/batch)
output_folder = os.path.join(os.path.expanduser("~"), "pyo_batch_fx")
# create it if it does not exist
if not os.path.isdir(output_folder):
    os.mkdir(output_folder)

# get the list of files to process
sounds = [file for file in os.listdir(folder_path) if sndinfo(os.path.join(folder_path, file)) is not None]

# enter the batch processing loop
for sound in sounds:
    # retrieve info about the sound
    path = os.path.join(folder_path, sound)
    info = sndinfo(path)
    dur, sr, chnls = info[1], info[2], info[3]
    fformat = ['WAVE', 'AIFF', 'AU', 'RAW', 'SD2', 'FLAC', 'CAF', 'OGG'].index(info[4])
    samptype = ['16 bit int', '24 bit int', '32 bit int', '32 bit float',
                '64 bits float', 'U-Law encoded', 'A-Law encoded'].index(info[5])

    # set server parameters
    s.setSamplingRate(sr)
    s.setNchnls(chnls)
    s.boot()
    s.recordOptions(dur=dur, filename=os.path.join(output_folder, sound),
                    fileformat=fformat, sampletype=samptype)

    # processing
    sf = SfPlayer(path)
    bp = ButBP(sf, 1000, 2)
    dt = Disto(bp, drive=0.9, slope=0.8)
    mx = Interp(sf, dt, interp=0.5, mul=0.5).out()

    # start the render
    s.start()
    # cleanup
    s.shutdown()
