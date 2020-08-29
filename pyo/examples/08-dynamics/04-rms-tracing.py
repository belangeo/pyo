"""
04-rms-tracing.py - Auto-wah effect.

The auto-wah effect (also know as "envelope following filter") is like a
wah-wah effect, but instead of being controlled by a pedal, it is the RMS
amplitude of the input sound which control it. The envelope follower (RMS)
is rescaled and used to change the frequency of a bandpass filter applied
to the source.

"""
from pyo import *

s = Server().boot()

MINFREQ = 250
MAXFREQ = 5000

# Play the drum lopp.
sf = SfPlayer("../snds/drumloop.wav", loop=True)

# Follow the amplitude envelope of the input sound.
follow = Follower(sf)

# Scale the amplitude envelope (0 -> 1) to the desired frequency
# range (MINFREQ -> MAXFREQ).
freq = Scale(follow, outmin=MINFREQ, outmax=MAXFREQ)

# Filter the signal with a band pass. Play with the Q to make the
# effect more or less present.
filter = ButBP(sf.mix(2), freq=freq, q=2).out()

s.gui(locals())
