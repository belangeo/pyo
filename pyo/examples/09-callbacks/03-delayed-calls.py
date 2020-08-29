"""
03-delayed-calls.py - Calling a function once, after a given delay.

If you want to setup a callback once in the future, the CallAfter
object is very easy to use. You just give it the function name, the
time to wait before making the call and an optional argument.

"""
from pyo import *

s = Server().boot()

# A four-streams oscillator to produce a chord.
amp = Fader(fadein=0.005, fadeout=0.05, mul=0.2).play()
osc = SineLoop(freq=[0, 0, 0, 0], feedback=0.05, mul=amp)
rev = WGVerb(osc.mix(2), feedback=0.8, cutoff=4000, bal=0.2).out()

# A function to change the oscillator's frequencies and start the envelope.
def set_osc_freqs(notes):
    print(notes)
    osc.set(attr="freq", value=midiToHz(list(notes)), port=0.005)
    amp.play()


# Initial chord.
set_osc_freqs([60, 64, 67, 72])

# We must be sure that our CallAfter object stays alive as long as
# it waits to call its function. If we don't keep a reference to it,
# it will be garbage-collected before doing its job.
call = None


def new_notes(notes):
    global call  # Use a global variable.
    amp.stop()  # Start the fadeout of the current notes...
    # ... then, 50 ms later, call the function that change the frequencies.
    call = CallAfter(set_osc_freqs, time=0.05, arg=notes)


# The sequence of events. We use a tuple for the list of frequencies
# because PyoObjects spread lists as argument over all their internal
# streams. This means that with a list of frequencies, only the first
# frequency would be passed to the callback of the first (and single)
# stream (a list of functions at first argument would create a
# multi-stream object). A tuple is treated as a single argument.
c1 = CallAfter(new_notes, time=0.95, arg=(60, 64, 67, 69))
c2 = CallAfter(new_notes, time=1.95, arg=(60, 65, 69, 76))
c3 = CallAfter(new_notes, time=2.95, arg=(62, 65, 69, 74))
c4 = CallAfter(new_notes, time=3.45, arg=(59, 65, 67, 74))
c5 = CallAfter(new_notes, time=3.95, arg=(60, 64, 67, 72))
# The last event activates the fadeout of the amplitude envelope.
c6 = CallAfter(amp.stop, time=5.95, arg=None)

s.gui(locals())
