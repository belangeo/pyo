"""
05-control-change.py - Adding control changes to our MIDI synthesizer class.

This example adds three continuous MIDI controllers to the previous one
(04-simple-midi-synth.py). The first one applied the pitch bend to the
`transpo` argument, which multiply the pitches from the Notein object.
The second is a continuous controller used to change the cutoff frequency
(`hfdamp` argument) of the lowpass filter. The last one sets the speed
(`lfofreq` argument) of moving notche oscillations in the spectrum. 

"""
from pyo import *
from random import random


class Synth:
    # Added some arguments that will be controlled with MIDI controllers.
    def __init__(self, transpo=1, hfdamp=5000, lfofreq=0.2, mul=1):
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

        # High frequencies damping, use argument `hfdamp` to allow MIDI control.
        self.damp = ButLP(self.mix, freq=hfdamp)

        # Moving notches, use argument `lfofreq` to allow MIDI control.
        self.lfo = Sine(lfofreq, phase=[random(), random()]).range(250, 4000)
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

### Setup the MIDI controllers.

# Bendin can output MIDI note values or transposition factors (scale=1).
# A value of 2 to `brange` argument means that the bending range will be
# 2 semitones above and below the current note.
transpo = Bendin(brange=2, scale=1)

# High frequency damping mapped to controller number 1.
hfdamp = Midictl(ctlnumber=1, minscale=500, maxscale=10000, init=5000)

# Frequency of the LFO applied to the speed of the moving notches.
lfofreq = Midictl(ctlnumber=2, minscale=0.1, maxscale=8, init=0.2)


# Create the midi synth.
a1 = Synth(transpo, hfdamp, lfofreq)

# Send the synth's signal into a reverb processor.
rev = STRev(a1.sig(), inpos=[0.1, 0.9], revtime=2, cutoff=4000, bal=0.15).out()

s.gui(locals())
