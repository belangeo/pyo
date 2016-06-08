"""
This example shows how to manage analog inputs and outputs.

"""

# Analog inputs comes right after the stereo audio channels.
i1 = Tone(Input(2), 8) # analog in 0
i2 = Tone(Input(3), 8) # analog in 1

lfo = Sine([8,10], mul=0.2)

# Analog outputs comes right after the stereo audio channels.
o1 = Clip(i1+lfo[0], 0, 1).out(2) # analog out 0
o2 = Clip(i2+lfo[1], 0, 1).out(3) # analog out 1

