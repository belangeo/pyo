"""
Sending audio streams as Open Sound Control messages

**03-send-streams.py**

This program does not generate any sound. All it does is to produce
audio streams that are then send on an open port as OSC messages in
order to control a granulation process created in the example 
*02-receive-streams.py*.

"""
from pyo import *

s = Server().boot()

# Manual control of the density of grains per second.
dens = Sig(0.5)
dens.ctrl(title="Density of grains per second")

# Generate a normalized random position in the sound with interpolation.
pos = Randi(min=0.00, max=1.00, freq=0.1)

# Manual control of the transposition per grain.
pit = Sig(0)
pit.ctrl(title="Transposition per grain")

# Manual control of the grain's duration.
dur = Sig(0.5)
dur.ctrl(title="Grain duration")

# Takes audio signals and sends their current value as OSC messages every buffer size.
send = OscSend(
    input=[dens, pos, pit, dur],
    port=9000,
    address=["/density", "/position", "/pitch_rand", "/duration"],
    host="127.0.0.1",
)

s.gui(locals())
