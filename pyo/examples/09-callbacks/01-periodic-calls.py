"""
01-periodic-calls.py - Periodic event generation.

When an audio thread is running, the user must take care of
not freezing the interpreter lock by using blocking function
like sleep(). It is better to use objects from the library
that are designed to call a function without blocking the
interpreter. Because these objects are aware of the server's
timeline, they are well suited for generating rhythmic
sequences of functions calls.

Pyo offers three objects for calling function:

- Pattern: Periodically calls a Python function. 
- Score: Calls functions by incrementation of a preformatted name. 
- CallAfter: Calls a Python function after a given time.

This example shows the usage of the Pattern object to create a 
sequence of audio events.

"""
from pyo import *
import random

s = Server().boot()

# A small instrument to play the events emitted by the function call.
amp = Fader(fadein=0.005, fadeout=0.05, mul=0.15)
osc = RCOsc(freq=[100, 100], mul=amp).out()
dly = Delay(osc, delay=1.0, feedback=0.5).out()


def new_event():
    # Choose a duration for this event.
    dur = random.choice([0.125, 0.125, 0.125, 0.25, 0.25, 0.5, 1])

    # Assigns the new duration to the envelope.
    amp.dur = dur
    # Assigns the new duration to the caller, thus the next function call
    # will be only after the current event has finished.
    pat.time = dur

    # Choose a new frequency.
    freq = random.choice(midiToHz([60, 62, 63, 65, 67, 68, 71, 72]))

    # Replace oscillator's frequencies.
    osc.freq = [freq, freq * 1.003]

    # Start the envelope.
    amp.play()


# A Pattern object periodically call the referred function given as
# argument. The "time" argument is the delay between successive calls.
# The play() method must be explicitly called for a Pattern object
# to start its processing loop.
pat = Pattern(function=new_event, time=0.25).play()

s.gui(locals())
