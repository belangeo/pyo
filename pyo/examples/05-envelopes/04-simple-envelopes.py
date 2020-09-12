"""
04-simple-envelopes.py - ASR and ADSR envelopes.

The Fader object is a simple way to setup an Attack/Sustain/Release envelope.
This envelope allows to apply fadein and fadeout on audio streams.

If the `dur` argument of the Fader object is set to 0 (the default), the
object waits for a stop() command before activating the release part of
the envelope. Otherwise, the sum of `fadein` and `fadeout` must be less
than or egal to `dur` and the envelope runs to the end on a play() command.

The Adsr object (Attack/Decay/Sustain/Release) acts exactly like the Fader
object, with a more flexible (and so common) kind of envelope.

"""
from pyo import *
import random

s = Server().boot()

# Infinite sustain for the global envelope.
globalamp = Fader(fadein=2, fadeout=2, dur=0).play()

# Envelope for discrete events, sharp attack, long release.
env = Adsr(attack=0.01, decay=0.1, sustain=0.5, release=1.5, dur=2, mul=0.5)
# setExp method can be used to create exponential or logarithmic envelope.
env.setExp(0.75)

# Initialize  a simple wave player and apply both envelopes.
sig = SuperSaw(freq=[100, 101], detune=0.6, bal=0.8, mul=globalamp * env).out()


def play_note():
    "Play a new note with random frequency and jitterized envelope."
    freq = random.choice(midiToHz([36, 38, 41, 43, 45]))
    sig.freq = [freq, freq * 1.005]
    env.attack = random.uniform(0.002, 0.01)
    env.decay = random.uniform(0.1, 0.5)
    env.sustain = random.uniform(0.3, 0.6)
    env.release = random.uniform(0.8, 1.4)
    # Start the envelope for the event.
    env.play()


# Periodically call a function.
pat = Pattern(play_note, time=2).play()

s.gui(locals())
