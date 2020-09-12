"""
02-bandpass-filters.py - Narrowing a bandpass filter bandwidth.

This example illustrates the difference between a simple IIR second-order
bandpass filter and a cascade of second-order bandpass filters. A cascade
of four bandpass filters with a high Q can be used as a efficient resonator
on the signal.

"""
from pyo import *

s = Server().boot()

# White noise generator
n = Noise(0.5)

# Common cutoff frequency control
freq = Sig(1000)
freq.ctrl([SLMap(50, 5000, "lin", "value", 1000)], title="Cutoff Frequency")

# Common filter's Q control
q = Sig(5)
q.ctrl([SLMap(0.7, 20, "log", "value", 5)], title="Filter's Q")

# Second-order bandpass filter
bp1 = Reson(n, freq, q=q)
# Cascade of second-order bandpass filters
bp2 = Resonx(n, freq, q=q, stages=4)

# Interpolates between input objects to produce a single output
sel = Selector([bp1, bp2]).out()
sel.ctrl(title="Filter selector (0=Reson, 1=Resonx)")

# Displays the spectrum contents of the chosen source
sp = Spectrum(sel)

s.gui(locals())
