"""
14-post-processing.py - Accessing Events's audio output for post-processing.

The Events framework is essentially an audio material generator. The user will
generally want to have access to the result for future treatments, as in passing
through some reverb or delay.

There are two built-in arguments that help the user to configure the audio output
of the Events object.

- `signal`: A string indicating which attribute of the instrument is its final
            audio output. The Events objects will automatically sum this attribute
            signal from all active instrument instances.
- `outs`: Determine how many channels the Events object output signal will contain.
          This value should match the number of audio streams produced by the
          instrument.

Once these two arguments are defined, the sig() method returns an audio signal that
is the sum of the active instance output signals.

"""

from pyo import *

s = Server().boot()

# A simple custom instrument. Note that the out() method is not called!
class MyInstrument(EventInstrument):
    def __init__(self, **args):
        EventInstrument.__init__(self, **args)
        self.output = LFO(freq=self.freq, sharp=[0.5, 0.6], type=2, mul=self.env)


# Some notes...
scl = EventScale("C", "aeolian", 3, 3, type=2)

# ... then the sequence of events. We are looking for a 2 streams (`outs`)
# signal in the self.output attribute (`signal`) of the instrument.
e = Events(
    instr=MyInstrument,
    degree=EventSlide(scl, segment=3, step=1),
    beat=1 / 4.0,
    db=-12,
    signal="output",
    outs=2,
    attack=0.001,
    decay=0.05,
    sustain=0.7,
    release=0.05,
).play()

# We use the sig() method to add post-processing to the events's sound.
chorus = Chorus(e.sig(), depth=1, feedback=0.25)
delay = Delay(chorus, delay=1, feedback=0.5)
reverb = WGVerb(chorus + delay, feedback=0.8, cutoff=5000, bal=0.25).out()

s.gui(locals())
