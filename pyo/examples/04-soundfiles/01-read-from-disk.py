"""
01-read-from-disk.py - Soundfile playback from disk.

SfPlayer and friends read samples from a file on disk with control
over playback speed and looping mode.

Player family:
    - **SfPlayer** : Reads many soundfile formats from disk.
    - **SfMarkerLooper** : AIFF with markers soundfile looper.
    - **SfMarkerShuffler** : AIFF with markers soundfile shuffler.

Reading sound file from disk can save a lot of RAM, especially if
the soundfile is big, but it is more CPU expensive than loading
the sound file in memory in a first pass.

"""
from pyo import *

s = Server().boot()

path = SNDS_PATH + "/transparent.aif"

# stereo playback with a slight shift between the two channels.
sf = SfPlayer(path, speed=[1, 0.995], loop=True, mul=0.4).out()

s.gui(locals())
