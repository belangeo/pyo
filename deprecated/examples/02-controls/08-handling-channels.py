"""
08-handling-channels.py - Managing object's internal audio streams.

Because audio objects expand their number of streams according to lists
given to their arguments and the fact that an audio object is considered
as a list, if a multi-streams object is given as an argument to another
audio object, the later will also be expanded in order to process all
given streams. This is really powerful to create polyphonic processes
without copying long chunks of code but it can be very CPU expensive.

In this example, we create a square from a sum of sine waves. After that,
a chorus is applied to the resulting waveform. If we don't mix down the
square wave, we got tens of Chorus objects in the processing chain (one
per sine). This can easily overrun the CPU. The exact same result can
be obtained with only one Chorus applied to the sum of the sine waves.
The `mix(voices)` method of the PyoObject helps the handling of channels
in order to save CPU cycles. Here, we down mix all streams to only two
streams (to maintain the stereo) before processing the Chorus arguments.

"""
from pyo import *

s = Server().boot()

# Sets fundamental frequency and highest harmonic.
freq = 100
high = 20

# Generates lists for frequencies and amplitudes
harms = [freq * i for i in range(1, high) if i % 2 == 1]
amps = [0.33 / i for i in range(1, high) if i % 2 == 1]

# Creates a square wave by additive synthesis.
a = Sine(freq=harms, mul=amps)
print("Number of Sine streams: %d" % len(a))

# Mix down the number of streams of "a" before computing the Chorus.
b = Chorus(a.mix(2), feedback=0.5).out()
print("Number of Chorus streams: %d" % len(b))

s.gui(locals())
