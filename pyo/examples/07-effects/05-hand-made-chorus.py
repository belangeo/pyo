"""
06-hand-made-chorus.py - Hand-written 8 delay lines stereo chorus.

A chorus (or ensemble) is a modulation effect used to create a richer,
thicker sound and add subtle movement. The effect roughly simulates
the slight variations in pitch and timing that occur when multiple
performers sing or play the same part.

A single voice chorus uses a single delay that creates a single modulated
duplicate of the incoming audio. Basic chorus effects and inexpensive guitar
pedals are often single-voice.

A multiple voice chorus uses multiple modulated delays to create a richer
sound with more movement than a single voice chorus.

The Chorus object (from pyo) implements an 8 delay lines chorus and should
use less CPU than the hand-written version. this example's purpose is only
to show how it works or to be used as a starting point to build an extended
version. 

"""
from pyo import *

s = Server(duplex=0).boot()

# Start a source sound.
sf = SfPlayer("../snds/baseballmajeur_m.aif", speed=1, loop=True, mul=0.3)
# Mix the source in stereo and send the signal to the output.
sf2 = sf.mix(2).out()

# Sets values for 8 LFO'ed delay lines (you can add more if you want!).
# LFO frequencies.
freqs = [0.254, 0.465, 0.657, 0.879, 1.23, 1.342, 1.654, 1.879]
# Center delays in seconds.
cdelay = [0.0087, 0.0102, 0.0111, 0.01254, 0.0134, 0.01501, 0.01707, 0.0178]
# Modulation depths in seconds.
adelay = [0.001, 0.0012, 0.0013, 0.0014, 0.0015, 0.0016, 0.002, 0.0023]

# Create 8 sinusoidal LFOs with center delays "cdelay" and depths "adelay".
lfos = Sine(freqs, mul=adelay, add=cdelay)

# Create 8 modulated delay lines with a little feedback and send the signals
# to the output. Streams 1, 3, 5, 7 to the left and streams 2, 4, 6, 8 to the
# right (default behaviour of the out() method).
delays = Delay(sf, lfos, feedback=0.5, mul=0.5).out()

s.gui(locals())
