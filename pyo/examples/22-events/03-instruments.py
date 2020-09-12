"""
03-instruments.py - Using custom instrument with events.

The default instrument ( DefaultInstrument ) is a basic stereo RC oscillator
passing through reverberation unit. For the events framework to be really
useful, it has to give the user the opportunity to use his own instruments.
Composing an instrument is very simple.

An Events's instrument must be derived from the EventInstrument class. Its
signature must be::

    class InstrumentName(EventInstrument):
        def __init__(self, **args):
            EventInstrument.__init__(self, **args)

The EventInstrument is responsible for the creation of the envelope, accessible
through the variable self.env, and also for clearing its resources when it's done
playing. 

All arguments given to the Events object can be retrieved in our instrument with 
the syntax self.argument_name (ex.: self.freq).

"""
from pyo import *

s = Server().boot()


class MyInstrument(EventInstrument):
    def __init__(self, **args):
        EventInstrument.__init__(self, **args)

        # self.freq is derived from the 'degree' argument.
        self.phase = Phasor([self.freq, self.freq * 1.003])

        # self.dur is derived from the 'beat' argument.
        self.duty = Expseg([(0, 0.05), (self.dur, 0.5)], exp=4).play()

        self.osc = Compare(self.phase, self.duty, mode="<", mul=1, add=-0.5)

        # EventInstrument created the amplitude envelope as self.env.
        self.filt = ButLP(self.osc, freq=5000, mul=self.env).out()


# We tell the Events object which instrument to use with the 'instr' argument.
e = Events(
    instr=MyInstrument,
    degree=EventSeq([5.00, 5.04, 5.07, 6.00]),
    beat=1 / 2.0,
    db=-12,
    attack=0.001,
    decay=0.05,
    sustain=0.5,
    release=0.005,
).play()

s.gui(locals())
