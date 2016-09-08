"""
03-fadein-fadeout.py - Creating attack, sustain and release envelope.

"""
from pyo import *
import random

s = Server().boot()

globalamp = Fader(fadein=2, fadeout=2, dur=0).play()

env = Fader(fadein=0.01, fadeout=1.5, dur=2, mul=0.3)

# Initialize  a simple wave player.
sig = RCOsc([100,100], sharp=0.7, mul=globalamp*env).out()

def play_note():
    freq = random.choice([200,250300,350,400,450,500])
    sig.freq = [freq, freq*1.005]
    env.play()

pat = Pattern(play_note, time=2).play()

s.gui(locals())

