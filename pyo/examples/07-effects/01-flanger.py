"""
01-flanger.py - Hand-made simple flanger.

A flanger is an audio effect produced by mixing two identical signals
together, one signal delayed by a small and gradually changing period.
This produces a swept comb filter effect: peaks and notches are produced
in the resulting frequency spectrum, related to each other in a linear
harmonic series. Varying the time delay causes these to sweep up and
down the frequency spectrum. 

"""
from pyo import *

s = Server().boot()

# Rich frequency spectrum as stereo input source.
amp = Fader(fadein=0.25, mul=0.5).play()
src = PinkNoise(amp).mix(2)

# Flanger parameters                        == unit ==
middelay = 0.005  # seconds

depth = Sig(0.99)  # 0 --> 1
depth.ctrl(title="Modulation Depth")
lfospeed = Sig(0.2)  # Hertz
lfospeed.ctrl(title="LFO Frequency in Hz")
feedback = Sig(0.5, mul=0.95)  # 0 --> 1
feedback.ctrl(title="Feedback")

# LFO with adjusted output range to control the delay time in seconds.
lfo = Sine(freq=lfospeed, mul=middelay * depth, add=middelay)

# Dynamically delayed signal. The source passes through a DCBlock
# to ensure there is no DC offset in the signal (with feedback, DC
# offset can be fatal!).
flg = Delay(DCBlock(src), delay=lfo, feedback=feedback)

# Mix the original source with its delayed version.
# Compress the mix to normalize the output signal.
cmp = Compress(src + flg, thresh=-20, ratio=4).out()

s.gui(locals())
