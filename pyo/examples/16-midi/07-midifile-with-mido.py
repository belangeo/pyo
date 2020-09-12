"""
07-midifile-with-mido.py - Reading a MIDI file with mido and sending the events to pyo.

This example shows how simple it is to play a MIDI file with mido and send the events
to an audio synth build with pyo.

"""
from pyo import *

# Try to import MidiFile from the mido module. You can install mido with pip:
#   pip install mido

try:
    from mido import MidiFile
except:
    print("The `mido` module must be installed to run this example!")
    exit()

s = Server().boot().start()

# A little audio synth to play the MIDI events.
mid = Notein()
amp = MidiAdsr(mid["velocity"])
pit = MToF(mid["pitch"])
osc = Osc(SquareTable(), freq=pit, mul=amp).mix(1)
rev = STRev(osc, revtime=1, cutoff=4000, bal=0.2).out()

# Opening the MIDI file...
mid = MidiFile("../snds/mapleleafrag.mid")

# ... and reading its content.
for message in mid.play():
    # For each message, we convert it to integer data with the bytes()
    # method and send the values to pyo's Server with the addMidiEvent()
    # method. This method programmatically adds a MIDI message to the
    # server's internal MIDI event buffer.
    s.addMidiEvent(*message.bytes())
