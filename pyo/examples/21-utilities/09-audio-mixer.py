"""
Mixing multiple inputs to multiple outputs with fading time

**09-audio-mixer.py**

The Mixer object allows to route multiple inputs to multiple outputs
with individual amplitude. A portamento time, defined with the `time`
argument, is applied to amplitude changes to make smooth mixing
variations.

The initialization argument `outs` lets defined the number of available
outputs and the argument `chnls` specifies how many channels per output.
If `chnls` is 2 and a single audio stream is given to an input, the signal
will be dupplicated to fill the two channels of the voice.

"""
from pyo import *
import random

s = Server(duplex=0).boot()

# Define some input signals for the mixer.
low = SfPlayer("../snds/flute.aif", speed=0.5, loop=True, mul=0.3)
mid = SfPlayer("../snds/flute.aif", speed=0.75, loop=True, mul=0.3)
hig = SfPlayer("../snds/flute.aif", speed=1.0, loop=True, mul=0.3)

# Mixer with 3 outputs, 2 channels per voice and 0.5 second of amplitude fading time.
mm = Mixer(outs=3, chnls=2, time=0.5)

# Route the mixer's outputs to some effects.
octaveBelow = Harmonizer(mm[0], transpo=-12, feedback=0.25, add=mm[0]).out()
fifthBelow = Harmonizer(mm[1], transpo=-7, feedback=0.25, add=mm[1]).out()
secondAbove = Harmonizer(mm[2], transpo=2, feedback=0.25, add=mm[2]).out()

# Add signals as inputs to the mixer.
mm.addInput(voice=0, input=low)
mm.addInput(voice=1, input=mid)
mm.addInput(voice=2, input=hig)

# Initial assignment of inputs to outputs with amplitude balance.
mm.setAmp(vin=0, vout=0, amp=0.25)
mm.setAmp(vin=1, vout=2, amp=0.25)

# Dictionaries used to print assigment messages.
inputs = {0: "low", 1: "mid", 2: "hig"}
outputs = {0: "Octave below", 1: "Fifth below", 2: "Second above"}

# Dynamic assignation of inputs to outputs with random amplitude.
def assign():
    vin = random.randint(0, 2)
    vout = random.randint(0, 2)
    amp = random.choice([0, 0, 0.25, 0.33, 0.5])
    print("%s -> %s, amp = %f" % (inputs[vin], outputs[vout], amp))
    mm.setAmp(vin=vin, vout=vout, amp=amp)


# Call a new assignment every 3 seconds.
pat = Pattern(function=assign, time=3).play()

s.gui(locals())
