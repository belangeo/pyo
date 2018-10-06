#!/usr/bin/env python
# encoding: utf-8
"""
Fuzz distortion. High gain followed by, asymmetrical clipping,
hard clip on top, soft compression on bottom.

"""
from pyo import *

s = Server(duplex=0).boot()

SOURCE = "../snds/flute.aif"
BP_CENTER_FREQ = 250
BP_Q = 2
BOOST = 25
LP_CUTOFF_FREQ = 3500
BALANCE = 1

src = SfPlayer(SOURCE, loop=True).mix(2)

# Transfert function for signal lower than 0
table = ExpTable([(0,-.25),(4096,0),(8192,0)], exp=10)
table.view()
# Transfert function for signal higher than 0
high_table = CosTable([(0,0),(4096,0),(4598,1),(8192,1)])

table.add(high_table)

# Bandpass filter and boost gain applied on input signal
bp = Biquad(src, freq=BP_CENTER_FREQ, q=BP_Q, type=2)
boost = Sig(bp, mul=BOOST)

# Apply transfert function
sig = Lookup(table, boost)

# Lowpass filter on distorted signal
lp = ButLP(sig, freq=LP_CUTOFF_FREQ, mul=.7)

# Balance between dry and wet signals
out = Interp(src, lp, interp=BALANCE).out()

sc = Scope(out)

s.gui(locals())
