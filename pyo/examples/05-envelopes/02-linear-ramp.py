"""
02-linear-ramp.py - Portamento, glissando, ramping.

The SigTo object allows to create audio glissando between the
current value and the target value, within a user-defined time.
The target value can be a float or another PyoObject. A new ramp
is created everytime the target value changes.

Also:

The VarPort object acts similarly but works only with float and
can call a user-defined callback when the ramp reaches the target
value.

The PyoObject.set() method is another way create a ramp for any
given parameter that accept audio signal but is not already
controlled with a PyoObject.

"""
from pyo import *

s = Server().boot()

# 2 seconds linear ramp starting at 0.0 and ending at 0.3.
amp = SigTo(value=0.3, time=2.0, init=0.0)

# Pick a new value four times per second.
pick = Choice([200, 250, 300, 350, 400], freq=4)

# Print the chosen frequency
pp = Print(pick, method=1, message="Frequency")

# Add a little portamento on an audio target and detune a second frequency.
freq = SigTo(pick, time=0.01, mul=[1, 1.005])
# Play with portamento time.
freq.ctrl([SLMap(0, 0.25, "lin", "time", 0.01, dataOnly=True)])

# Play a simple wave.
sig = RCOsc(freq, sharp=0.7, mul=amp).out()

s.gui(locals())
