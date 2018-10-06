#!/usr/bin/env python
"""
Generates 5 seconds of white noise and saves it in a file called
noise.aif on the Desktop.

"""

from pyo import savefile
from random import uniform
import os

home = os.path.expanduser("~")
path = os.path.join(home, "Desktop", "noise.aif")
sr = 44100
dur = 5
chnls = 2

samples = [[uniform(-0.5,0.5) for i in range(sr*dur)] for i in range(chnls)]
savefile(samples=samples, path=path, sr=sr, channels=chnls, fileformat=1, sampletype=1)
