"""
04-simple-midi-synth.py - Create a MIDI synthesizer as a custom class.

A more elaborate MIDI synthesizer built in a custom python class. This
makes easier to use it multiple times. In this example, two objects are
created, one for the played pitch and another playing one octave lower.

"""
from pyo import *
from random import random


class Synth:
    def __init__(self, transpo=1, mul=1):
        # Transposition factor.
        self.transpo = Sig(transpo)
        # Receive midi notes, convert pitch to Hz and manage 10 voices of polyphony.
        self.note = Notein(poly=10, scale=1, first=0, last=127)

        # Handle pitch and velocity (Notein outputs normalized amplitude (0 -> 1)).
        self.pit = self.note["pitch"] * self.transpo
        self.amp = MidiAdsr(self.note["velocity"], attack=0.001, decay=0.1, sustain=0.7, release=1, mul=0.1,)

        # Anti-aliased stereo square waves, mixed from 10 streams to 1 stream
        # to avoid channel alternation on new notes.
        self.osc1 = LFO(self.pit, sharp=0.5, type=2, mul=self.amp).mix(1)
        self.osc2 = LFO(self.pit * 0.997, sharp=0.5, type=2, mul=self.amp).mix(1)

        # Stereo mix.
        self.mix = Mix([self.osc1, self.osc2], voices=2)

        # High frequencies damping.
        self.damp = ButLP(self.mix, freq=5000)

        # Moving notches, using two out-of-phase sine wave oscillators.
        self.lfo = Sine(0.2, phase=[random(), random()]).range(250, 4000)
        self.notch = ButBR(self.damp, self.lfo, mul=mul)

    def out(self):
        "Sends the synth's signal to the audio output and return the object itself."
        self.notch.out()
        return self

    def sig(self):
        "Returns the synth's signal for future processing."
        return self.notch


s = Server()
s.setMidiInputDevice(99)  # Open all input devices.
s.boot()

# Create the midi synth.
a1 = Synth()

# Send the synth's signal into a reverb processor.
rev = STRev(a1.sig(), inpos=[0.1, 0.9], revtime=2, cutoff=4000, bal=0.15).out()

# It's very easy to double the synth sound!
# One octave lower and directly sent to the audio output.
a2 = Synth(transpo=0.5, mul=0.7).out()

s.gui(locals())
