#!/usr/bin/env python
"""Generates 5 seconds of white noise."""

from pyo import savefile
from random import uniform

sr, dur, chnls, path = 44100, 5, 2, "/Users/olipet/Desktop/noise.aif"

samples = [[uniform(-0.7,0.7) for i in range(sr*dur)] for i in range(chnls)]    
savefile(samples=samples, path=path, sr=sr, channels=chnls, format=2)