#!/usr/bin/env python
"""
Generates 5 seconds of white noise and saves it in a file called
noise.aif in the home directory.

"""

from pyo import savefile
from random import uniform
import os

home = os.path.expanduser("~")
sr, dur, chnls, path = 44100, 5, 2, os.path.join(home, "noise.aif")

samples = [[uniform(-0.7,0.7) for i in range(sr*dur)] for i in range(chnls)]    
savefile(samples=samples, path=path, sr=sr, channels=chnls, format=8)