"""
03-midi-envelope.py - Shaping the sound of a MIDI synth.

This program illustrates a classic way of shaping the dynamic of the
sound of a MIDI synthesizer with two MIDI-triggered envelope objects:

- MidiAdsr: a classic Attack - Decay - Sustain - Release envelope.
- MidiDelAdsr: a classic ADSR envelope with a delay added prior the attack.

We will use these envelopes to control the gain of two parts of the sound.
The first part is an oscillator at the root frequency with a sharp attack.
The second part is a pair of oscillators slightly detuned raising smoothly
after a half-second delay. This gradually introduces beating into the sound.

"""
from pyo import *

s = Server()
s.setMidiInputDevice(99)  # Open all input devices.
s.boot()

# Automatically converts MIDI pitches to frequencies in Hz.
notes = Notein(scale=1)
notes.keyboard()

# MIDI-triggered ADSR envelope.
env1 = MidiAdsr(notes["velocity"], attack=0.005, decay=0.1, sustain=0.7, release=0.5, mul=0.1)

# MIDI-triggered DADSR envelope (a classic ADSR with an adjustable pre-delay).
env2 = MidiDelAdsr(notes["velocity"], delay=0.5, attack=1, decay=0.5, sustain=0.5, release=0.5, mul=0.1)

# Root frequency appears instantly.
sig1 = RCOsc(freq=notes["pitch"], sharp=0.5, mul=env1).mix().mix(2).out()

# Small frequency deviations appear smoothly after a half-second delay.
sig2 = RCOsc(freq=notes["pitch"] * 0.992, sharp=0.8, mul=env2).mix()
sig3 = RCOsc(freq=notes["pitch"] * 1.008, sharp=0.8, mul=env2).mix()

# Create a stereo signal from the frequency deviations.
stereo = Mix([sig2, sig3], voices=2)

# Sum the signals and apply a global reverberation.
rev = WGVerb(sig1 + stereo, feedback=0.8, cutoff=5000, bal=0.3).out()

s.gui(locals())
