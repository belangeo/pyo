"""
06-multichannel-expansion.py - What is a `Stream`? Polyphonic objects.

List expansion is a powerful technique for generating many audio
streams at once.

What is a "stream"? A "stream" is a monophonic channel of samples.
It is the basic structure over which all the library is built. Any
PyoObject can handle as many streams as necessary to represent the
defined process. When a polyphonic (ie more than one stream) object
is asked to send its signals to the output, the server will use the
arguments (`chnl` and `inc`) of the out() method to distribute the
streams over the available output channels.

Almost all attributes of all objects of the library accept list of
values instead of a single value. The object will create internally
the same number of streams than the length of the largest list
given to an attribute at the initialization time. Each value of the
list is used to generate one stream. Shorter lists will wrap around
when reaching the end of the list.

A PyoObject is considered by other object as a list. The function
`len(obj)` returns the number of streams managed by the object. This
feature is useful to create a polyphonic dsp chain.

"""
from pyo import *

s = Server().boot()

### Using multichannel-expansion to create a square wave ###

# Sets fundamental frequency.
freq = 100
# Sets the highest harmonic.
high = 20

# Generates the list of harmonic frequencies (odd only).
harms = [freq * i for i in range(1, high) if i % 2 == 1]
# Generates the list of harmonic amplitudes (1 / n).
amps = [0.33 / i for i in range(1, high) if i % 2 == 1]

# Creates all sine waves at once.
a = Sine(freq=harms, mul=amps)
# Prints the number of streams managed by "a".
print(len(a))

# The mix(voices) method (defined in PyoObject) mixes
# the object streams into `voices` streams.
b = a.mix(voices=1).out()

# Displays the waveform.
sc = Scope(b)

s.gui(locals())
