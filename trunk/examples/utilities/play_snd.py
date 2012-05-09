#!/usr/bin/env python
# encoding: utf-8
"""
Simple soundfile player used by 06_separated_threads.py example.

"""
from pyo import *

s = Server(duplex=0).boot()

sf = SfPlayer('../snds/snd_1.aif')
sf2 = sf.mix(2).out()

s.start()
