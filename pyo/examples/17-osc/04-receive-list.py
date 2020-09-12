"""
Receiving OSC messages as list of audio streams

**04-receive-list.py**

OscListReceive receives list of values over a network via the Open Sound
Control protocol. It converts the values as audio streams that can be used
to control audio process.

"""
from pyo import *

s = Server().boot()

source = SfPlayer("../snds/flute.aif", loop=True, mul=0.7)

# Receives a list of two values from a TouchOSC's XY pad and converts
# them as audio streams. All streams for a given address can be retrieve
# with the syntax: rec[address]. One can access the individual streams
# for an address with slicing: rec[address][0], rec[address][1], etc.
rec = OscListReceive(port=9002, address="/4/xy", num=2)

# Sets initial values for the OSC streams. This allow the program to run with
# minimal behaviour even if no message have been sent on this address.
rec.setValue("/4/xy", [0.5, 0.5])

# Assign the value on the X axis to the drive parameter, and
# the value on the Y axis to the slope (lowpass filter) parameter.
disto = Disto(source, drive=Sqrt(rec["/4/xy"][0]), slope=Sqrt(rec["/4/xy"][1]), mul=0.5).mix(2).out()

s.gui(locals())
