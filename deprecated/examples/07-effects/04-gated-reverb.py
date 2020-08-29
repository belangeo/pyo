"""
05-gated-reverb.py - Stereo ping-pong delay.

Gated reverb is an audio processing technique that is applied to recordings of drums
to make the drums sound powerful and punchy, while keeping the overall mix clean and
transparent-sounding. The gated reverb effect, which was most popular in the 1980s, 
is made using a combination of strong reverb and a noise gate.

"""
from pyo import *
import random

s = Server().boot()

sf = SfPlayer("../snds/drumloop.wav", loop=True)
gat = Gate(sf, thresh=-50, risetime=0.005, falltime=0.04, lookahead=4, outputAmp=True)
gat.ctrl()

rev = STRev(sf, inpos=0.5, revtime=1.5, cutoff=5000, bal=1, roomSize=0.8, firstRefGain=-3)
hip = ButHP(rev, freq=100)
cmp = Compress(hip, thresh=-12, ratio=3, risetime=0.005, falltime=0.05, lookahead=4, knee=0.5, mul=gat,)

output = Interp(sf, cmp, interp=0.2, mul=0.5).out()
output.ctrl()


s.gui(locals())
