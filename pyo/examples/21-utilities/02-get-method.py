"""
Retrieving current value of a PyoObject as a python float

**02-get-method.py**

The get() method of the PyoObject allow the user to retrieve the current
value of an audio signal as a python float. This can useful if one wants
to use an audio signal to drive an algorithm written in pure python.

If the `all` argument of the get() method is True, it will return a list
with the current value of all streams managed by the object. If False (the
default), te value of the first audio stream will be returned as a float::

    lfos = Sine(freq=[.1,.2,.4,.3], mul=100, add=500)
    synth = SineLoop(freq=lfos, feedback=.07, mul=.05).out()

    def print_val():
        # Print all four frequencies assigned to SineLoop's freq argument
        print(lfos.get(all=True))

    pat = Pattern(print_val, .25).play()

Complete example
----------------
"""

from pyo import *

s = Server(duplex=0).boot()

t = CurveTable([(0, 0), (2048, 0.5), (4096, 0.2), (6144, 0.5), (8192, 0)], tension=0, bias=20).normalize()
t.view(title="LFO Waveform")

# LFO applied on amplitude value of the synths.
a = Osc(table=t, freq=2, mul=0.1)

# Make some modulated noise...
synth1 = BrownNoise(a).mix(2).out()
synth2 = FM(carrier=[100, 50], ratio=[0.495, 1.01], index=10, mul=a).out()

# Oscillator from which to get values to modify the shape of the LFO waveform.
c = Sine(0.1, 0, 10, 10)


def change():
    # Get the current value of the oscillator.
    val = c.get()
    # Print the value to the console.
    print("Current oscillator value:", val)
    # Change the bias of the curve and normalize the table.
    t.setBias(val)
    t.normalize()


# Call change() function 10 times per second.
p = Pattern(change, 0.1).play()

s.gui(locals())
