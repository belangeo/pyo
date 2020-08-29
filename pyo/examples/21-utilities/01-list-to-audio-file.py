"""
Saving list of floats to audio file on disk

**01-list-to-audio-file.py**

This program generates 5 seconds of white noise with pure python function
and saves the samples in a file called noise.aif on the Desktop.

"""

from pyo import savefile
from random import uniform
import os

# Create the path for the audio file.
home = os.path.expanduser("~")
path = os.path.join(home, "Desktop", "noise.aif")

# Audio file properties (sampling rate, duration and number of channels).
sr = 44100
dur = 5
chnls = 2

# Generate a list of `chnls` sub-lists with `sr * dur` floats in each of them.
samples = [[uniform(-0.5, 0.5) for i in range(sr * dur)] for i in range(chnls)]

# Save the list of floats in an audio file on disk.
savefile(samples=samples, path=path, sr=sr, channels=chnls, fileformat=1, sampletype=1)
