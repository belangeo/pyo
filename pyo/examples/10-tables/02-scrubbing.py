"""
02-scrubbing.py - Navigate through a sound table.

The SndTable object allows to transfer the data from a soundfile
into a function table for a fast access to every sample.

This example illustrates how to do scrubbing with the mouse in
a sound window. The mouse position is then used to control the
position and the balance of a simple stereo granulator.

Give the focus to the Scrubbing window then click and move the mouse...

"""

from pyo import *

s = Server().boot()

# The callback given to the SndTable.view() method.
def mouse(mpos):
    print("X = %.2f, Y = %.2f" % tuple(mpos))
    # X value controls the granulator pointer position.
    position.value = mpos[0]
    # Y value controls the balance between left and right channels.
    l, r = 1.0 - mpos[1], mpos[1]
    leftRightAmp.value = [l, r]


# Load and normalize the sound in the table.
snd = SndTable("../snds/ounkmaster.aif").normalize()
# Open the waveform view with a mouse position callback.
snd.view(title="Scrubbing window", mouse_callback=mouse)

# Left and right channel gain values.
leftRightAmp = SigTo([1, 1], time=0.1, init=[1, 1], mul=0.1)
# Position, in samples, in the SndTable.
position = SigTo(0.5, time=0.1, init=0.5, mul=snd.getSize(), add=Noise(5))

# Simple sound granulator.
gran = Granulator(
    table=snd,  # the sound table.
    env=HannTable(),  # the grain envelope.
    pitch=[0.999, 1.0011],  # global pitch (change every grain).
    pos=position,  # position in the table where to start a new grain.
    dur=Noise(0.002, 0.1),  # duration of the grain (can be used to transpose per grain).
    grains=64,  # the number of grains.
    basedur=0.1,  # duration for which the grain is not transposed.
    mul=leftRightAmp,  # stereo gain.
).out()

s.gui(locals())
