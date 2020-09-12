"""
03-gated-verb.py - Gated reverb applied to a drum loop.

The gated reverb effect, which was most popular in the 1980s, is made using a
combination of strong reverb and a noise gate. The drum sound passes through
a strong reverb, which is rapidly cut off with a gate driven by the dry signal.

"""
from pyo import *

s = Server().boot()

# Play the drum lopp..
sf = SfPlayer("../snds/drumloop.wav", loop=True)

# Use a gate to generate the gain curve that will be applied to the reverb.
gate = Gate(sf, thresh=-50, risetime=0.005, falltime=0.04, lookahead=4, outputAmp=True)

# Strong reverb.
rev = Freeverb(sf.mix(2), size=0.95, damp=0.3, bal=1.0)

# Compress the reverb signal and control its amplitude with the gating signal.
cmp = Compress(rev, thresh=-12, ratio=3, risetime=0.005, falltime=0.05, lookahead=4, knee=0.5, mul=gate,)

# Balance between the dry and wet (gated-reverb) signals.
output = Interp(sf.mix(2), cmp, interp=0.2).out()

s.gui(locals())
