"""
01-lowpass-filters.py - The effect of the order of a filter.

For this first example about filtering, we compare the frequency
spectrum of three common lowpass filters.

- Tone : IIR first-order lowpass
- ButLP : IIR second-order lowpass (Butterworth)
- MoogLP : IIR fourth-order lowpass (+ resonance as an extra parameter)

Complementary highpass filters for the Tone and ButLP objects are Atone
and ButHP. Another common highpass filter is the DCBlock object, which
can be used to remove DC component from an audio signal.

The next example will present bandpass filters.

"""
from pyo import *

s = Server().boot()

# White noise generator
n = Noise(0.5)

# Common cutoff frequency control
freq = Sig(1000)
freq.ctrl([SLMap(50, 5000, "lin", "value", 1000)], title="Cutoff Frequency")

# Three different lowpass filters
tone = Tone(n, freq)
butlp = ButLP(n, freq)
mooglp = MoogLP(n, freq)

# Interpolates between input objects to produce a single output
sel = Selector([tone, butlp, mooglp]).out()
sel.ctrl(title="Filter selector (0=Tone, 1=ButLP, 2=MoogLP)")

# Displays the spectrum contents of the chosen source
sp = Spectrum(sel)

s.gui(locals())
