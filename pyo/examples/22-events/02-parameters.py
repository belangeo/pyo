"""
02-parameters.py - Built-in arguments and conversions.

An Events object is build on top of the python's dictionary. Any argument
passed at the creation of the object becomes a key inside the built-in
structure. A lot of argument are pre-defined automatically processed to
facilitate the construction of event's frequency, duration, amplitude,
and envelope.

Duration
--------

The duration of an event can be defined with 'dur' argument, which is a
duration in seconds, or with 'beat' argument. A value given to 'beat' is
converted in seconds (and registered at the key 'dur'), according to the
value given to the 'bpm' argument (which defaults to 120). A beat value
of 1 is the duration of the quarter note, 2 is a half note, 1/2 is an
eighth note, and so on...

The first argument that reaches the end of its sequence triggers the end
of the Events's playback::

    e = Events(freq = EventSeq([250, 300, 350, 400]),
               beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4)).play()

The 'durmul' argument allow to change the note duration without changing
the time gap between successive events::

    # Staccato
    e1 = Events(freq = EventSeq([250, 300, 350, 400]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4),
                durmul = 0.5).play()

    # Overlapping
    e2 = Events(freq = EventSeq([250, 300, 350, 400]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4),
                durmul = 1.5).play()

Frequency
---------

The 'freq' argument can be used directly to specify the root note. But
it is sometimes easier to create a melody with 'midinote' or 'degree',
and let the engine do the conversion to frequency for you. The 'midinote'
argument is a MIDI note number between 0 and 127, fractionnal numbers
are allowed to do micro-tonal scaling. The 'degree' argument needs a
value in octave.degree format where octave is the number of the desired
octave and degree is a 2-digits degree number between 00 and 11. For
an example, the MIDI note 48 is 4.00 in octave.degree notation.

The midinote and degree arguments will automatically convert and store
the frequency, in Hz, as the 'freq' argument::

    e3 = Events(midinote = EventSeq([60, 64, 67, 72]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4)).play()

The 'degree' argument uses an octave.degree notation. Ex.: 6.07 means
the perfect fifth (07) of the sixth octave::

    e4 = Events(degree = EventSeq([5.00, 5.04, 5.07, 6.00]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4)).play()

One can transpose an entire sequence (or sequence the transposition
values) with the 'transpo' argument. Transposition values are relative
midi note (0 means no transposition, -12 is the lower octave)::

    e5 = Events(degree = EventSeq([5.00, 5.04, 5.07, 6.00]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4),
                transpo = -12).play()

Amplitude
---------

Amplitude values are given either with 'amp' (linear amplitude), 'db'
(decibel values) or 'midivel' (MIDI velocity between 0 and 127). Decibel
values and MIDI velocity are internally converted to linear gain and stored
as the 'amp' argument::

    # Sequence with accented notes.
    e6 = Events(degree = EventSeq([5.00, 5.04, 5.07, 6.00]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4),
                db = EventSeq([-3, -6, -12, -3])).play()

Envelope
--------

Different shapes for the amplitude envelope are defined according to the
presence of certain parameters. The amplitude envelope is automatically
created in the EventGenerator __init__ method.

If 'envelope' receives a PyoTableObject, it will be used as the amplitude
envelope of the events. If 'envelope' is None and 'decay' gets a value,
the envelope will be an ADSR built with 'attack', 'decay', 'sustain' and 
'release' argument. If both 'envelope' and 'decay' are None, the envelope
will be an ASR, defined with 'attack', 'sustain' and 'release' arguments.

The following defines an Adsr envelope::

    e7 = Events(degree = EventSeq([5.00, 5.04, 5.07, 6.00]),
                beat = EventSeq([1/2., 1/4., 1/4., 1], occurrences=4),
                attack = 0.001, decay = 0.05, sustain = 0.5, release = 0.005,
                db = EventSeq([-3, -6, -12, -3])).play()

Complete Example
----------------

"""
from pyo import *

s = Server().boot()

### Envelope

# Using a PyoTableObject to define the envelope.
env = CosTable([(0, 0.0), (128, 1.0), (1024, 0.7), (4096, 0.7), (8192, 0.0)])

e8 = Events(
    degree=EventSeq([5.00, 5.04, 5.07, 6.00]),
    beat=EventSeq([1 / 2.0, 1 / 4.0, 1 / 4.0, 1], occurrences=4),
    db=EventSeq([-3, -6, -12, -3]),
    envelope=env,
).play()

s.gui(locals())
