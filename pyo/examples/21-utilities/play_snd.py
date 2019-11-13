"""
Simple soundfile player with delay used by 08-nultiple-threads.py example.

"""
from pyo import *

s = Server(duplex=0).boot()

sf = SfPlayer('../snds/snd_1.aif', mul=0.7)
dd = SmoothDelay(sf, delay=0.25, feedback=[0.5, 0.5]).out()

s.start()
