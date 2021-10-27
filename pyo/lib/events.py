"""
This submodule contains a set of tools to generate sequence of events.

The purpose of the Event framework is to allow the user to generate a
sequence of events with as few as possible parameters to specify.

:py:class:`Events` is the heart of the framework. An Events object computes
parameters, generally designed with event generator objects, builds the events
and plays the sequence.

See the Events framework examples in the documentation for different use cases.

"""

"""
Copyright 2019 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
import math
import copy
import random
from ._core import *
from .pattern import Pattern, CallAfter
from .tableprocess import TableRead
from .controls import Fader, Adsr
from .generators import RCOsc
from .effects import STRev
from .pan import Pan

inf = math.inf

PYO_EVENT_OPERATOR_ADD = 1000
PYO_EVENT_OPERATOR_SUB = 1001
PYO_EVENT_OPERATOR_MUL = 1002
PYO_EVENT_OPERATOR_DIV = 1003
PYO_EVENT_OPERATOR_POW = 1004
PYO_EVENT_OPERATOR_MOD = 1005
PYO_EVENT_OPERATOR_FLR = 1006

PYO_EVENT_FILTER_FLOOR = 1100
PYO_EVENT_FILTER_CEIL = 1101
PYO_EVENT_FILTER_ROUND = 1102
PYO_EVENT_FILTER_ABS = 1103
PYO_EVENT_FILTER_SNAP = 1104
PYO_EVENT_FILTER_DEVIATE = 1105
PYO_EVENT_FILTER_CLIP = 1106
PYO_EVENT_FILTER_SCALE = 1107
PYO_EVENT_FILTER_RESCALE = 1108
PYO_EVENT_FILTER_IFTRUE = 1109

# Utility functions
###################
def degreeToMidiNote(value):
    "Converts an octave.degree notation to a MIDI note."
    st = str(value)
    if st.count(".") == 0:
        try:
            oct = int(st)
            deg = 0
        except:
            oct = 0
            deg = 0
    elif st.count(".") == 1:
        dotPos = st.find(".")
        if len(st) >= (dotPos + 3):
            st = st[: dotPos + 3]
        try:
            oct, deg = st.split(".")
            if len(deg) == 1:
                deg = "%s0" % deg
            oct, deg = int(oct), int(deg)
        except:
            oct, deg = 0, 0

    return oct * 12 + deg


def midiNoteToDegree(value):
    "Converts a MIDI note to an octave.degree notation."
    oct = int(value / 12)
    deg = value % 12
    return "%d.%02d" % (oct, deg)


def getValueFromAttribute(master, key, currentDict, valueIfNone=None):
    "Retrieve the value of an Events attribute, resolving its type."
    attribute = master[key]
    returnValue = None

    if key in currentDict:
        return currentDict[key]

    elif attribute is None:
        returnValue = valueIfNone

    elif isinstance(attribute, EventKey):
        if attribute.externalMaster is None:
            if attribute.getKey() in currentDict:
                returnValue = currentDict[attribute.getKey()]
            else:
                returnValue = getValueFromAttribute(master, attribute.getKey(), currentDict)
                currentDict[attribute.getKey()] = returnValue
        else:
            returnValue = attribute.externalMaster.getCurrentDict().get(attribute.getKey(), None)

    elif isinstance(attribute, PyoObject):
        returnValue = attribute.get(False)
    elif isinstance(attribute, PyoTableObject):
        returnValue = attribute
    else:
        attribute.setMaster(master)
        returnValue = attribute.next()

    currentDict[key] = returnValue

    return returnValue


# Utility classes
#################
class MarkovGen:
    def __init__(self, lst, order=2):
        self.originalList = lst
        self.temporaryList = []
        self.playedNotes = []
        self.order = order
        self.startPlayback()

    def startPlayback(self):
        self.playedNotes = []
        for val in self.originalList:
            self.temporaryList.append(val)
        for i in range(self.order):
            self.temporaryList.append(self.originalList[i])
        self.playedNotes = self.originalList[len(self.originalList) - self.order :]

    def next(self):
        newValue = 0
        condition = False
        self.probTable = []

        for i in range(len(self.temporaryList) - self.order):
            for iord in range(self.order):
                if (
                    self.playedNotes[len(self.playedNotes) - (iord + 1)]
                    != self.temporaryList[(self.order - 1) + i - iord]
                ):
                    condition = False
                    break
                else:
                    condition = True

            if condition:
                self.probTable.append(self.temporaryList[i + self.order])

        newValue = self.probTable[random.randint(0, (len(self.probTable) - 1))]
        self.playedNotes.append(newValue)
        return newValue

    def setOrder(self, order):
        if order != self.order:
            self.order = order
            self.startPlayback()


# Instruments
#############
class EventInstrument(object):
    """
    Base class for an Events instrument. All attributes given to the Events
    object can be accessed as self.attribute_name inside the instrument.

    This base class constructs an envelope, named self.env, according to the
    value given to 'envelope' (ex.: a LinTable object) or to 'attack', 'decay',
    'sustain' and 'release' attributes of the event. The envelope is also
    scaled by the value of self.amp, defined by 'amp', 'db' or 'midivel'
    arguments of the Events object.

    This base class also creates a self.freq variable based on 'freq', 'degree'
    or 'midinote' arguments. This variable can be used in the instrument to
    control the pitch of the sound.

    All resources are automatically destroyed when the lifetime of the event
    is over. The lifetime of the event is set as self.dur + self.tail ('dur'
    or 'beat' and 'tail' arguments of Events).

    .. note::

        The user has almost no reason to instantiate an EventInstrument object
        himself. Instead, he should use it as a parent class for its own instruments.

    """

    def __init__(self, **args):
        for key, val in args.items():
            setattr(self, key, val)

        if self.envelope is not None:
            self.env = TableRead(self.envelope, 1.0 / self.dur, mul=self.amp).play()
        elif self.decay is not None:
            self.env = Adsr(self.attack, self.decay, self.sustain, self.release, dur=self.dur, mul=self.amp).play()
        else:
            self.env = Fader(fadein=self.attack, fadeout=self.release, dur=self.dur, mul=self.amp).play()

        self.clearWhenDone = CallAfter(self.clear, time=0)
        self.clearWhenDone.play(delay=self.dur + self.tail, dur=0.25)

    def clear(self):
        self.removeFunction(self.instanceId)


class DefaultInstrument(EventInstrument):
    """
    The default instrument, playing a stereo RC oscillator, used when
    'instr' attribute is not defined for an Events object.

    """

    def __init__(self, **args):
        EventInstrument.__init__(self, **args)
        self.osc = RCOsc(freq=self.freq, sharp=0.5, mul=self.env)
        self.sig = self.osc.mix(2).out()


# Event Scale
#############
class EventScale:
    """
    Musical scale builder.

    EventScale constructs a list of pitches according to its arguments.

    EventScale works similarly to list, ie. uses slicing with square brackets
    to access data, with the first element at index 0.

    It also accept the len() function, which returns the number of elements in
    the scale.

    :Args:

        root: str, optional
            The base note (fundamental) of the scale. Possible values are:
            'C', 'C#', 'Db', 'D', 'D#', 'Eb', 'E', 'F', 'F#',
            'Gb', 'G', 'G#', 'Ab', 'A', 'A#', 'Bb', 'B'.
            Defaults to 'C'.
        scale: str, optional
            The scale name to construct. Possible scales are:
            'major', 'minorH', 'minorM', 'ionian', 'dorian', 'phrygian',
            'lydian', 'mixolydian', 'aeolian', 'locrian', 'wholeTone',
            'majorPenta', 'minorPenta', 'egyptian', 'majorBlues', 'minorBlues',
            'minorHungarian'.
            Defaults to 'major'.
        first: int, optional
            The first octave of the generated scale, in multiple of 12. A value
            of 4, for a root of 'C' means the first note of the scale will be 48.
            Defaults to 4.
        octaves: int, optional
            The number of octaves in the generated scale. Defaults to 2.
        type: int, optional
            The unit type in which the values are stored. Possible types are:
                0: MIDI note
                1: Hertz
                2: octave.degree notation (MIDI note 48 is 4.00 in octave.degrees)

    .. note::

        Here is a table showing the relationship between the three unit types that
        EventScale can handle.

        ======== ======== ========
        midi     oct.deg  Hertz
        ======== ======== ========
        48       4.00     130.81
        50       4.02     146.83
        52       4.04     164.81
        53       4.05     174.61
        55       4.07     195.99
        57       4.09     220.00
        59       4.11     246.94
        60       5.00     261.62
        ======== ======== ========

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = EventScale(root="C", scale="major", first=4, octaves=2, type=2)
    >>> e = Events(degree=EventDrunk(scl, maxStep=-2), beat=1/4., db=-6).play()

    """

    def __init__(self, root="C", scale="major", first=4, octaves=2, type=0):
        self.rootDegrees = {
            "C": 0,
            "C#": 1,
            "Db": 1,
            "D": 2,
            "D#": 3,
            "Eb": 3,
            "E": 4,
            "F": 5,
            "F#": 6,
            "Gb": 6,
            "G": 7,
            "G#": 8,
            "Ab": 8,
            "A": 9,
            "A#": 10,
            "Bb": 10,
            "B": 11,
        }
        self.scales = {
            "major": [0, 2, 4, 5, 7, 9, 11],
            "minorH": [0, 2, 3, 5, 7, 8, 11],
            "minorM": [0, 2, 3, 5, 7, 9, 11],
            "ionian": [0, 2, 4, 5, 7, 9, 11],
            "dorian": [0, 2, 3, 5, 7, 9, 10],
            "phrygian": [0, 1, 3, 5, 7, 8, 10],
            "lydian": [0, 2, 4, 6, 7, 9, 11],
            "mixolydian": [0, 2, 4, 5, 7, 9, 10],
            "aeolian": [0, 2, 3, 5, 7, 8, 10],
            "locrian": [0, 1, 3, 5, 6, 8, 10],
            "wholeTone": [0, 2, 4, 6, 8, 10],
            "majorPenta": [0, 2, 4, 7, 9],
            "minorPenta": [0, 3, 5, 7, 10],
            "egyptian": [0, 2, 5, 7, 10],
            "majorBlues": [0, 2, 5, 7, 9],
            "minorBlues": [0, 3, 5, 8, 10],
            "minorHungarian": [0, 2, 3, 6, 7, 8, 11],
        }
        self._init = True
        self._length = 0
        self._root = "c"
        self._scale = "major"
        self._first = first
        self._octaves = octaves
        self._type = type
        self.setRoot(root)
        self.setScale(scale)
        self.data = []
        self._init = False
        self._populate()

    def _populate(self):
        if self._init:
            return

        del self.data[:]
        degree = self.rootDegrees.get(self._root, self.rootDegrees["C"])
        currentScale = self.scales.get(self._scale, self.scales["major"])
        length = len(currentScale)
        for i in range(length * self._octaves + 1):
            octave = (self._first + int(i / length)) * 12
            self.data.append(currentScale[i % length] + octave + degree)
        self._length = len(self.data)
        if self._type == 1:
            self.data = midiToHz(self.data)
        elif self._type == 2:
            self.data = [midiNoteToDegree(x) for x in self.data]

    def __len__(self):
        return self._length

    def __getitem__(self, key):
        return self.data[key]

    def __setitem__(self, key, item):
        self.data[key] = item

    def setRoot(self, x):
        """
        Replace the `root` attribute and reconstruct the scale.

        :Args:

            x: string
                New `root` attribute.

        """
        if x not in self.rootDegrees:
            print("EventScale: does not recognize root '%s'..." % x)
            print("... Using 'C'.")
            x = "C"
        if x != self._root:
            self._root = x
            self._populate()

    def setScale(self, x):
        """
        Replace the `scale` attribute and reconstruct the scale.

        :Args:

            x: string
                New `scale` attribute.

        """
        if x not in self.scales:
            print("EventScale: does not recognize scale '%s'..." % x)
            print("... Using 'major'.")
            x = "major"
        if x != self._scale:
            self._scale = x
            self._populate()

    def setFirst(self, x):
        """
        Replace the `first` attribute and reconstruct the scale.

        :Args:

            x: int
                New `int` attribute.

        """
        if x != self._first:
            self._first = x
            self._populate()

    def setOctaves(self, x):
        """
        Replace the `octaves` attribute and reconstruct the scale.

        :Args:

            x: int
                New `octaves` attribute.

        """
        if x != self._octaves:
            self._octaves = x
            self._populate()

    def setType(self, x):
        """
        Replace the `type` attribute and reconstruct the scale.

        :Args:

            x: int
                New `type` attribute.

        """
        if x != self._type:
            self._type = x
            self._populate()

    @property
    def root(self):
        """string. Name of the fundamental key."""
        return self._root

    @root.setter
    def root(self, x):
        self.setRoot(x)

    @property
    def scale(self):
        """string. Name of scale to generate."""
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)

    @property
    def first(self):
        """int. First octave to generate."""
        return self._first

    @first.setter
    def first(self, x):
        self.setFirst(x)

    @property
    def octaves(self):
        """int. Number of octaves to generate."""
        return self._octaves

    @octaves.setter
    def octaves(self, x):
        self.setOctaves(x)

    @property
    def type(self):
        """int. Unit in which pitch values are stored."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


# Event Generators
##################
class EventGenerator:
    """
    Base class for all event generators.

    This class contains the common behaviours of all event generators.

    Each EventGenerator contains a particular algorithm that can produce a
    sequence of values triggered by an Events mecanism for one of its arguments.

    The EventGenerator allows very flexible control of the algorithm parameters.
    It can be a single value, another EventGenerator or an audio signal (PyoObject).

    Arithmetic operations are allowed on EventGenerator. An EventDummy is
    then created to apply the operation to each value produced by the generator.

    Arithmetic operators are:

        +: float, PyoObject or EventGenerator
            Addition.
        -: float, PyoObject or EventGenerator
            Substraction
        *: float, PyoObject or EventGenerator
            Multiplication
        /: float, PyoObject or EventGenerator
            Division
        %:  float, PyoObject or EventGenerator
            Modulo (remaining of the division)
        **: float, PyoObject or EventGenerator
            Exponent
        //: float, PyoObject or EventGenerator
            Quantizer (returns te nearest multiple of its argument)

    EventGenerator has a number of filter methods that can be applied on
    any generator to modify its output values. Available filter methods are:

        floor:
            Return an EventFilter computing the largest integer less than or
            equal to its input value.
        ceil:
            Return an EventFilter computing the smallest integer greater than
            or equal to its input value.
        round:
            Return an EventFilter computing the nearest integer to its input value.
        snap:
            Return an EventFilter which choose the nearest value of its input
            value in a list of choices.
        deviate:
            Return an EventFilter which randomly move, up or down, its input value.
        clip:
            Return an EventFilter which clips its input value between predefined
            limits.
        scale:
            Return an EventFilter which maps its input value, in the range 0 to 1,
            to an output range, with a scaling curve.
        rescale:
            Return an EventFilter which maps its input value, given in an input
            range, to an output range with a scaling curve.
        iftrue:
            Return an EventFilter which compares its input value to a comparison
            value and outputs it if the comparison is True.

    """

    def __init__(self):
        self.generator = None
        self.master = None
        self.stopEventsWhenDone = False

    def _inspect_generator(self, generator):
        # Inspect the value passed as a generator argument.
        if type(generator) in [type([]), type(())]:
            generator = EventSeq(generator)
        elif type(generator) in [type(0), type(0.0)]:
            generator = EventSeq([generator])
        return generator

    def _inspect_values(self, values):
        # Inspect the value passed as a values argument.
        if not values:
            values = [0]
        return values

    def _inspect_occurrences(self, occurrences):
        # Inspect the value passed as an occurrences argument.
        if occurrences < 1:
            occurrences = 1
        return occurrences

    def _internalGeneratorNextCall(self):
        # Return the next value of an internal generator.
        if isinstance(self.generator, EventKey):
            if self.generator.needToResetTarget:
                self.generator.needToResetTarget = False
                try:
                    self.master[self.generator.getKey()].reset()
                    if self.generator.getKey() in self.master.getCurrentDict():
                        del self.master.getCurrentDict()[self.generator.getKey()]
                except:
                    pass
            value = getValueFromAttribute(self.master, self.generator.getKey(), self.master.getCurrentDict())
        else:
            self.generator.setMaster(self.master)
            value = self.generator.next()

        if value is None:
            self.generator = None
            return self.next()

        return value

    def _checkValueTypeAndIncrementCount(self, value):
        # Final check of the value produced by the generator in the next() method.
        self.count += 1
        if isinstance(value, EventGenerator):
            self.generator = value
            self.generator.reset()
            value = self.next()
        elif isinstance(value, PyoObject):
            value = value.get(False)
        return value

    def __add__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_ADD)

    def __sub__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_SUB)

    def __mul__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_MUL)

    def __div__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_DIV)

    def __truediv__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_DIV)

    def __pow__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_POW)

    def __mod__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_MOD)

    def __floordiv__(self, generator):
        generator = self._inspect_generator(generator)
        return EventDummy(self, generator, PYO_EVENT_OPERATOR_FLR)

    def setMaster(self, master):
        # Keep a reference of the master player (an Events object).
        self.master = master

    def copy(self):
        # Return a deep copy of this generator (used in Events.events()).
        return copy.deepcopy(self)

    def reset(self):
        # Reset the generator to its init state. Implemented in child objects.
        pass

    def next(self):
        # Internally called to produce the next value. Implemented in child objects.
        pass

    def resetEmbeddedGenerator(self):
        # Recursively resets embedded generators. Used when starting a sequence.
        if self.generator is not None:
            self.generator.resetEmbeddedGenerator()
            self.generator.reset()

    def floor(self):
        """
        Return an EventFilter computing the largest integer less than or
        equal to its input value.

        """
        return EventFilter(self, PYO_EVENT_FILTER_FLOOR)

    def ceil(self):
        """
        Return an EventFilter computing the smallest integer greater than or
        equal to its input value.

        """
        return EventFilter(self, PYO_EVENT_FILTER_CEIL)

    def round(self):
        """
        Return an EventFilter computing the nearest integer to its input value.
        If two values are equally close, rounding is done toward the even choice.

        """
        return EventFilter(self, PYO_EVENT_FILTER_ROUND)

    def abs(self):
        """
        Return an EventFilter computing the absolute value of its input value.

        """
        return EventFilter(self, PYO_EVENT_FILTER_ABS)

    def snap(self, choice):
        """
        Return an EventFilter which choose the nearest value of its input value
        in the `choice` list.

        :Args:

            choice: list of floats
                Possible values to output.

        """
        return EventFilter(self, PYO_EVENT_FILTER_SNAP, choice)

    def deviate(self, depth):
        """
        Return an EventFilter which randomly move, up or down, its input value
        according to the argument `depth`, in percent.

        :Args:

            depth: float or PyoObject
                Percentage of deviation, between 0 and 100.

        """
        return EventFilter(self, PYO_EVENT_FILTER_DEVIATE, depth)

    def clip(self, mini, maxi):
        """
        Return an EventFilter which clips its input value between the limits
        `mini` and `maxi`.

        :Args:

            mini: float or PyoObject
                Minimum output value.
            maxi: float or PyoObject
                Maximum output value.

        """
        return EventFilter(self, PYO_EVENT_FILTER_CLIP, mini, maxi)

    def scale(self, mini, maxi, expon):
        """
        Return an EventFilter which maps its input value, in the range 0 to 1,
        to an output range, with a scaling curve deternimed bu the `expon` value.

        :Args:

            mini: float or PyoObject
                Minimum output value.
            maxi: float or PyoObject
                Maximum output value.
            expon: float or PyoObject
                Exponent value, specifies the nature of the scaling curve.
                Values between 0 and 1 give a logarithmic curve, and values
                higher than 1 give an exponential curve.

        """
        return EventFilter(self, PYO_EVENT_FILTER_SCALE, mini, maxi, expon)

    def rescale(self, inmin, inmax, outmin, outmax, expon):
        """
        Return an EventFilter which maps its input value, in the range `inmin`
        to `inmax`, to an output range, `outmin` to `outmax`, with a scaling
        curve deternimed bu the `expon` value.

        :Args:

            inmin: float or PyoObject
                Minimum input value.
            inmax: float or PyoObject
                Maximum input value.
            outmin: float or PyoObject
                Minimum output value.
            outmax: float or PyoObject
                Maximum output value.
            expon: float or PyoObject
                Exponent value, specifies the nature of the scaling curve.
                Values between 0 and 1 give a logarithmic curve, and values
                higher than 1 give an exponential curve.

        """
        return EventFilter(self, PYO_EVENT_FILTER_RESCALE, inmin, inmax, outmin, outmax, expon)

    def iftrue(self, op, comp):
        """
        Return an EventFilter which compares its input value to the value
        given to `comp` argument, using the comparison operator `op`. If
        the result is True, the input value is sent to the output, otherwise,
        the last valid value is sent again.

        :Args:

            op: string
                The comparison operator. Valid operators are:
                '<', '<=', '>', '>=', '==', '!='.
            comp: float or PyoObject
                Comparison value.

        """
        return EventFilter(self, PYO_EVENT_FILTER_IFTRUE, op, comp)


class EventDummy(EventGenerator):
    "An EventGenerator created internally to handle arithmetic on Events."

    def __init__(self, generator1, generator2, type):
        EventGenerator.__init__(self)
        self.generator1 = generator1
        self.generator2 = generator2
        self.type = type

    def next(self):
        self.generator1.setMaster(self.master)
        v1 = self.generator1.next()

        if isinstance(self.generator2, PyoObject):
            v2 = self.generator2.get(False)
        else:
            self.generator2.setMaster(self.master)
            v2 = self.generator2.next()

        if v1 is None or v2 is None:
            return None
        else:
            if self.type == PYO_EVENT_OPERATOR_ADD:
                return v1 + v2
            elif self.type == PYO_EVENT_OPERATOR_SUB:
                return v1 - v2
            elif self.type == PYO_EVENT_OPERATOR_MUL:
                return v1 * v2
            elif self.type == PYO_EVENT_OPERATOR_DIV:
                return v1 / v2
            elif self.type == PYO_EVENT_OPERATOR_POW:
                return v1 ** v2
            elif self.type == PYO_EVENT_OPERATOR_MOD:
                return v1 % v2
            elif self.type == PYO_EVENT_OPERATOR_FLR:
                return math.floor(v1 / v2 + 0.5) * v2


class EventFilter(EventGenerator):
    "An EventGenerator created internally to handle simple filter on Events."

    def __init__(self, generator, type, *args):
        EventGenerator.__init__(self)
        self.generator = generator
        self.type = type
        self.args = args
        self.lastValue = 0.0

    def next(self):
        self.generator.setMaster(self.master)
        value = self.generator.next()

        if value is None:
            return None

        args = []
        for arg in self.args:
            if isinstance(arg, PyoObject):
                args.append(arg.get(False))
            elif isinstance(arg, EventGenerator):
                arg.setMaster(self.master)
                args.append(arg.next())
            else:
                args.append(arg)

        if self.type == PYO_EVENT_FILTER_FLOOR:
            return math.floor(value)
        elif self.type == PYO_EVENT_FILTER_CEIL:
            return math.ceil(value)
        elif self.type == PYO_EVENT_FILTER_ROUND:
            return round(value)
        elif self.type == PYO_EVENT_FILTER_ABS:
            return abs(value)
        elif self.type == PYO_EVENT_FILTER_SNAP:
            return min(args[0], key=lambda x: abs(x - value))
        elif self.type == PYO_EVENT_FILTER_DEVIATE:
            depth = args[0] * 0.01
            return value * random.uniform(1.0 - depth, 1.0 + depth)
        elif self.type == PYO_EVENT_FILTER_CLIP:
            if value < args[0]:
                value = args[0]
            elif value > args[1]:
                value = args[1]
            return value
        elif self.type == PYO_EVENT_FILTER_SCALE:
            if value < 0.0:
                value = 0.0
            elif value > 1.0:
                value = 1.0
            value = value ** args[2]
            return value * (args[1] - args[0]) + args[0]
        elif self.type == PYO_EVENT_FILTER_RESCALE:
            if value < args[0]:
                value = args[0]
            elif value > args[1]:
                value = args[1]
            value = (value - args[0]) / (args[1] - args[0])
            value = value ** args[4]
            return value * (args[3] - args[2]) + args[2]
        elif self.type == PYO_EVENT_FILTER_IFTRUE:
            if args[0] == "<":
                istrue = value < args[1]
            elif args[0] == "<=":
                istrue = value <= args[1]
            elif args[0] == ">":
                istrue = value > args[1]
            elif args[0] == ">=":
                istrue = value >= args[1]
            elif args[0] == "==":
                istrue = value == args[1]
            elif args[0] == "!=":
                istrue = value != args[1]
            if istrue:
                self.lastValue = value
                return value
            else:
                return self.lastValue


###################
class EventKey(EventGenerator):
    """
    An EventGenerator that allow to retrieve the value of another parameter.

    EventKey returns the current value of another parameter of the Events
    object where it is used. From there, other processes can be applied
    (arithmetics, filters) to transform this value.

    EventKey can also read parameter values from another Events object when
    one is passed as `master` argument.

    :Args:

        key: string
            The name of the parameter to read from.
        master: Events, optional
            The Events object from which to read the parameter value. If
            None (the default), the current Events object is used.


    >>> s = Server().boot()
    >>> s.start()
    >>> # The lower the pitch value, the louder is the note.
    >>> dbkey = EventKey("midinote").rescale(48,84,-3,-32,1)
    >>> e = Events(midinote=list(range(48,84,2)), beat=1/4., db=dbkey).play()

    """

    def __init__(self, key, master=None):
        EventGenerator.__init__(self)
        self.key = key
        self.needToResetTarget = False
        self.externalMaster = master

    def getKey(self):
        """
        Returns the key, as a string, of the parameter to read from.

        """
        return self.key

    def reset(self):
        self.needToResetTarget = True

    def next(self):
        if self.externalMaster is None:
            return getValueFromAttribute(self.master, self.getKey(), self.master.getCurrentDict())
        else:
            return getValueFromAttribute(self.externalMaster, self.getKey(), self.externalMaster.getCurrentDict())


###################
class EventSeq(EventGenerator):
    """
    Plays through an entire list of values many times.

    EventSeq will loop over its list of values a number of times
    defined by the occurrences argument.

    :Args:

        values: EventScale or list
            List of values to loop over. Values in list can be floats,
            PyoObject or other EventGenerator.
        occurrences: int, optional
            Number of times the sequence is entirely played in loop.
            Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    .. note::

        If an Events argument receives a single value or a list, it will be
        automatically converted to an EventSeq.

    >>> s = Server().boot()
    >>> s.start()
    >>> e = Events(freq=EventSeq(midiToHz([60, 64, 67, 72])), beat=1/4.).play()

    """

    def __init__(self, values, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.length * self.occurrences

    def reset(self):
        self.generator = None
        self.count = self.completed = 0

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        self.length = len(self.values)

        if self.count < self.length:
            value = self.values[self.count]
            return self._checkValueTypeAndIncrementCount(value)
        else:
            self.completed += 1
            if self.completed >= self.occurrences:
                return None
            else:
                self.count = 0
                return self.next()


class EventSlide(EventGenerator):
    """
    Plays overlapping segments from a list of values.

    EventSlide will play a segment of length `segment` from startpos,
    then another segment with a start position incremented by `step`,
    and so on.

    :Args:

        values: EventScale or list
            List of values to read. Values in list can be floats,
            PyoObject or other EventGenerator.
        segment: int, PyoObject or EventGenerator
            Number of values of each segment.
        step: int, PyoObject or EventGenerator
            How far to step the start of each segment from the previous. A
            negative value will step backward.
        startpos: int, optional
            The start position of the first segment. A negative value sets
            the position backward starting from the end of the list.
            Defaults to 0.
        wraparound: bool, optional
            If 'wraparound' if True, indexing wraps around if goes past the
            beginning or the end of the list. If False, the playback stops
            if it goes outside the list bounds. Defaults to True.
        occurrences: int, optional
            Number of entire segments to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00]
    >>> e = Events(degree=EventSlide(scl, 3, 1, 0), beat = 1/4., db = -6).play()

    """

    def __init__(self, values, segment, step, startpos=0, wraparound=True, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.wraparound = wraparound
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.segment = segment
        self.lastSegment = 1
        self.step = step
        self.lastStep = 1
        if startpos < 0:
            self.startpos = self.length + startpos
        else:
            self.startpos = startpos
        self.reset()

    def __len__(self):
        return self.segment * self.occurrences

    def reset(self):
        self.generator = None
        self.count = self.completed = 0
        self.start = self.startpos
        if isinstance(self.segment, EventGenerator):
            self.segment.reset()
        if isinstance(self.step, EventGenerator):
            self.step.reset()

    def getStepValue(self):
        if isinstance(self.step, PyoObject):
            value = self.step.get(False)
        elif isinstance(self.step, EventGenerator):
            self.step.setMaster(self.master)
            value = self.step.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastStep
        else:
            value = self.step
        if abs(value) >= self.length:
            if value < 0:
                value = -(self.length - 1)
            else:
                value = self.length - 1

        self.lastStep = int(value)
        return self.lastStep

    def getSegmentValue(self):
        if isinstance(self.segment, PyoObject):
            value = self.segment.get(False)
        elif isinstance(self.segment, EventGenerator):
            self.segment.setMaster(self.master)
            value = self.segment.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastSegment
        else:
            value = self.segment
        if value < 1:
            value = 1

        self.lastSegment = int(value)
        return self.lastSegment

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        self.length = len(self.values)

        segment = self.getSegmentValue()
        step = self.getStepValue()

        if segment is None or step is None:
            return None

        if self.count < segment:
            if step < 0:
                position = self.start - self.count
            else:
                position = self.start + self.count
            if position < 0:
                if self.wraparound:
                    position += self.length
                else:
                    return None
            elif position >= self.length:
                if self.wraparound:
                    position -= self.length
                else:
                    return None
            value = self.values[position]
            return self._checkValueTypeAndIncrementCount(value)
        else:
            self.completed += 1
            if self.completed >= self.occurrences:
                return None
            else:
                self.count = 0
                self.start += step
                if self.start < 0:
                    self.start += self.length
                elif self.start >= self.length:
                    self.start -= self.length
                return self.next()


class EventIndex(EventGenerator):
    """
    Plays values from a list based on a position index.

    :Args:

        values: EventScale or list
            List of values to read. Values in list can be floats,
            PyoObject or other EventGenerator.
        index: int, PyoObject or EventGenerator
            Position to read in the list, starting at 0.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00]
    >>> arp = EventSeq([0, 2, 4, 2, 1, 3, 5, 3, 1, 6, 4, 1])
    >>> e = Events(degree = EventIndex(scl, arp), beat = 1/4., db = -6).play()

    """

    def __init__(self, values, index, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.index = index
        self.lastIndex = 0
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.generator = None
        self.count = 0
        if isinstance(self.index, EventGenerator):
            self.index.reset()

    def getIndexValue(self):
        if isinstance(self.index, PyoObject):
            value = self.index.get(False)
        elif isinstance(self.index, EventGenerator):
            self.index.setMaster(self.master)
            value = self.index.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastIndex
        else:
            value = self.index
        if value < 0:
            value = 0
        elif value >= self.length:
            value = self.length - 1

        self.lastIndex = int(value)
        return self.lastIndex

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        self.length = len(self.values)

        index = self.getIndexValue()
        if index is None:
            return None

        if self.count < self.occurrences:
            value = self.values[index]
            return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


class EventMarkov(EventGenerator):
    """
    Applies a Markov algorithm to a list of values.

    A Markov chain is a stochastic model describing a sequence of possible events
    in which the probability of each event depends only on the state attained in
    the previous events.

    :Args:

        values: EventScale or list
            Original list of values.
        order: int, PyoObject or EventGenerator, optional
            Order of the Markov chain, between 1 and 10. Determines how many past
            values will be used to build the probability table for the next note.
            Defaults to 2.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> jesus =  [67,69,71,74,72,72,76,74,74,79,78,79,74,71,67,69,71,72,74,76,74,72,71]
    >>> jesus += [69,71,67,66,67,69,62,66,69,72,71,69,71,67,69,71,74,72,72,76,74,74,79]
    >>> jesus += [78,79,74,71,67,69,71,64,74,72,71,69,67,62,67,66,67,71,74,79,74,71,67]
    >>> e = Events(midinote=EventMarkov(jesus, 2), beat=1/4., db=-6).play()

    """

    def __init__(self, values, order=2, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.lastOrder = self.order = order
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.markov = MarkovGen(values)
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.generator = None
        self.count = 0
        if isinstance(self.order, EventGenerator):
            self.order.reset()

    def getOrderValue(self):
        if isinstance(self.order, PyoObject):
            value = self.order.get(False)
        elif isinstance(self.order, EventGenerator):
            self.order.setMaster(self.master)
            value = self.order.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastOrder
        else:
            value = self.order
        if value < 0:
            value = 0
        elif value > 10:
            value = 10

        self.lastOrder = int(value)
        return self.lastOrder

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        order = self.getOrderValue()
        if order is None:
            return None

        self.markov.setOrder(order)

        if self.count < self.occurrences:
            value = self.markov.next()
            return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


###################
class EventChoice(EventGenerator):
    """
    Plays values randomly chosen from a list.

    :Args:

        values: EventScale or list
            List of possible values to read. Values in list can be floats,
            PyoObject or other EventGenerator.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00]
    >>> e = Events(degree = EventChoice(scl), beat = 1/4., db = -6).play()

    """

    def __init__(self, values, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.generator = None
        self.count = 0

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        self.length = len(self.values)

        if self.count < self.occurrences:
            value = random.choice(self.values)
            return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


class EventDrunk(EventGenerator):
    """
    Performs a random walk over a list of values.

    A random walk is a stochastic process that consists of a succession of
    random steps, within a distance of +/- `maxStep` from the previous state.

    :Args:

        values: EventScale or list
            List of values to read. Values in list can be floats,
            PyoObject or other EventGenerator.
        maxStep: int, PyoObject or EventGenerator, optional
            Determine the larger step the walk can do between two successive
            events. A negative 'maxStep' is the same but repetition are not
            allowed. Defaults to 2.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = [5.00, 5.02, 5.03, 5.05, 5.07, 5.08, 5.10, 6.00]
    >>> e = Events(degree=EventDrunk(scl, maxStep=-2), beat=1/4., db=-6).play()

    """

    def __init__(self, values, maxStep=2, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.values = self._inspect_values(values)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.lastMaxStep = self.maxStep = maxStep
        self.length = len(self.values)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.generator = None
        self.allowRepetition = True
        self.count = 0
        self.index = int(self.length / 2)
        if isinstance(self.maxStep, EventGenerator):
            self.maxStep.reset()

    def getMaxStepValue(self):
        if isinstance(self.maxStep, PyoObject):
            value = self.maxStep.get(False)
        elif isinstance(self.maxStep, EventGenerator):
            self.maxStep.setMaster(self.master)
            value = self.maxStep.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastMaxStep
        else:
            value = self.maxStep
        self.allowRepetition = True
        if value < 0:
            value = -value
            self.allowRepetition = False
        elif value >= self.length:
            value = self.length - 2

        self.lastMaxStep = int(value)
        return self.lastMaxStep

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        self.length = len(self.values)

        maxStep = self.getMaxStepValue()
        if maxStep is None:
            return None

        if self.count < self.occurrences:
            dev = random.randint(-maxStep, maxStep)
            while dev == 0 and not self.allowRepetition:
                dev = random.randint(-maxStep, maxStep)
            self.index += dev
            if self.index < 0:
                self.index = -self.index - dev + 1
            elif self.index >= self.length:
                self.index = self.length - (self.index - self.length) - dev - 1
            value = self.values[self.index]
            return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


class EventNoise(EventGenerator):
    """
    Return a random value between -1.0 and 1.0.

    EventNoise returns a random value between -1.0 and 1.0, based on one of
    three common noise generators, white, pink (1/f) and brown (1/f^2).

    :Args:

        type: int, optional
            The type of noise used to generate the random sequence. Available
            types are:

            0: white noise (default)
            1: pink noise
            2:brown noise
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> scl = EventScale("C", "aeolian", 4, 4)
    >>> note = EventNoise(1).rescale(-1,1,48,84,1).snap(scl)
    >>> e = Events(midinote=note, beat=1/4., db=-6).play()

    """

    def __init__(self, type=0, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.lastType = self.type = type
        self.occurrences = self._inspect_occurrences(occurrences)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.count = 0
        self.y1 = 0.0
        self.c0 = self.c1 = self.c2 = self.c3 = self.c4 = self.c5 = self.c6 = 0.0

    def getTypeValue(self):
        if isinstance(self.type, PyoObject):
            value = self.type.get(False)
        elif isinstance(self.type, EventGenerator):
            self.type.setMaster(self.master)
            value = self.type.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastType
        else:
            value = self.type
        if value < 0:
            value = 0
        elif value > 2:
            value = 2

        self.lastType = int(value)
        return self.lastType

    def next(self):
        type = self.getTypeValue()
        if type is None:
            return None

        if self.count < self.occurrences:
            if type == 0:
                return self._checkValueTypeAndIncrementCount(random.uniform(-1.0, 1.0))
            elif type == 1:
                rnd = random.uniform(-0.99, 0.99)
                self.c0 = self.c0 * 0.99886 + rnd * 0.0555179
                self.c1 = self.c1 * 0.99332 + rnd * 0.0750759
                self.c2 = self.c2 * 0.96900 + rnd * 0.1538520
                self.c3 = self.c3 * 0.86650 + rnd * 0.3104856
                self.c4 = self.c4 * 0.55000 + rnd * 0.5329522
                self.c5 = self.c5 * -0.7616 - rnd * 0.0168980
                val = self.c0 + self.c1 + self.c2 + self.c3 + self.c4 + self.c5 + self.c6 + rnd * 0.5362
                self.c6 = rnd * 0.115926
                return self._checkValueTypeAndIncrementCount(val * 0.2)
            else:
                rnd = random.uniform(-0.99, 0.99)
                self.y1 = rnd + (self.y1 - rnd) * 0.996
                return self._checkValueTypeAndIncrementCount(self.y1 * 20.0)
        else:
            return None


###################
class EventCall(EventGenerator):
    """
    Calls a function, with any number of arguments, and uses its return value.

    EventCall can call any function (built-in, from a module or user-defined)
    and use its return as the value for the Events's parameter where it is used.
    The function *must* return a single number.

    :Args:

        function: callable
            The function to call, which should return the value to use.
        args: int, PyoObject or EventGenerator, optional
            Any number of arguments to pass to the function call. If given a
            PyoObject or an EventGenerator, these will be resolved for each
            event and the result passed, as number, to the function.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> from random import randrange
    >>> e = Events(midinote=EventCall(randrange, 48, 72, 3), beat=1/4., db=-6).play()

    """

    def __init__(self, function, *args, **kwargs):
        EventGenerator.__init__(self)
        self.function = function
        self.args = args
        self.occurrences = self._inspect_occurrences(kwargs.pop("occurrences", inf))
        self.stopEventsWhenDone = kwargs.pop("stopEventsWhenDone", True)
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.generator = None
        self.count = 0

    def next(self):
        if self.generator is not None:
            return self._internalGeneratorNextCall()

        args = []
        for arg in self.args:
            if isinstance(arg, PyoObject):
                args.append(arg.get(False))
            elif isinstance(arg, EventGenerator):
                arg.setMaster(self.master)
                args.append(arg.next())
            else:
                args.append(arg)

        if self.count < self.occurrences:
            value = self.function(*args)
            return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


###################
class EventConditional(EventGenerator):
    """
    Executes one generator or the other depending on the result of a condition.

    EventConditional takes three values or generators as arguments and if the
    value of `condition` is True (anything that python considers True), the
    `iftrue` argument is used to produce the value for the event, otherwise
    th `iffalse` argument is used.

    :Args:

        condition: int, PyoObject or EventGenerator
            Conditional value. True for everything python considers True.
        iftrue: int, PyoObject or EventGenerator
            Output value if the condition is True.
        iffalse: int, PyoObject or EventGenerator
            Output value if the condition is False.
        occurrences: int, optional
            Number of values to play. Defaults to inf (infinite).
        stopEventsWhenDone: bool, optional
            If True, the Events playback will stop if this generator reaches
            its end. If False, the Events will ignore this signal and probably
            get None as value for the given parameter. It's the user
            responsability to handle this case correctly. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> from random import randrange
    >>> scl = EventScale("C", "aeolian", 4, 3)
    >>> bit = EventChoice([0, 1, 1, 1])
    >>> pittrue = EventSlide(scl, segment=3, step=1, startpos=0)
    >>> veltrue = EventDrunk(range(64, 127), maxStep=5)
    >>> pit = EventConditional(bit, pittrue, 0)
    >>> vel = EventConditional(bit, veltrue, 0)
    >>> e = Events(midinote=pit, beat=1/4., midivel=vel).play()

    """

    def __init__(self, condition, iftrue, iffalse, occurrences=inf, stopEventsWhenDone=True):
        EventGenerator.__init__(self)
        self.condition = self._inspect_generator(condition)
        self.iftrue = self._inspect_generator(iftrue)
        self.iffalse = self._inspect_generator(iffalse)
        self.occurrences = self._inspect_occurrences(occurrences)
        self.stopEventsWhenDone = stopEventsWhenDone
        self.reset()

    def __len__(self):
        return self.occurrences

    def reset(self):
        self.count = 0
        self.lastCondition = self.lastIftrue = self.lastIffalse = 0

    def getConditionValue(self):
        if isinstance(self.condition, PyoObject):
            value = self.condition.get(False)
        elif isinstance(self.condition, EventGenerator):
            self.condition.setMaster(self.master)
            value = self.condition.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastCondition
        else:
            value = self.condition

        self.lastCondition = value
        return self.lastCondition

    def getIftrueValue(self):
        if isinstance(self.iftrue, PyoObject):
            value = self.iftrue.get(False)
        elif isinstance(self.iftrue, EventGenerator):
            self.iftrue.setMaster(self.master)
            value = self.iftrue.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastIftrue
        else:
            value = self.iftrue

        self.lastIftrue = value
        return self.lastIftrue

    def getIffalseValue(self):
        if isinstance(self.iffalse, PyoObject):
            value = self.iffalse.get(False)
        elif isinstance(self.iffalse, EventGenerator):
            self.iffalse.setMaster(self.master)
            value = self.iffalse.next()
            if value is None:
                if self.stopEventsWhenDone:
                    return None
                else:
                    value = self.lastIffalse
        else:
            value = self.iffalse

        self.lastIffalse = value
        return self.lastIffalse

    def next(self):
        condition = self.getConditionValue()
        if condition is None:
            return None

        if self.count < self.occurrences:
            if condition:
                value = self.getIftrueValue()
            else:
                value = self.getIffalseValue()
            if value is None:
                return None
            else:
                return self._checkValueTypeAndIncrementCount(value)
        else:
            return None


# Event Player
##############
class Events(dict):
    """
    Sequencing user-defined events to form musical phrases.

    The Events object is the primary tool in the events framework. It uses
    generators (derived from EventGenerator) as value for its arguments to
    build a sequence of events, each of them with their own parameters.

    Each time Events needs to produce a new event, it collects values from
    the generators given to its arguments, builds a parameter dictionary
    and gives it to a new instance of the audio instrument referenced to
    its 'instr' argument.

    The object produces new events until one of its generators reaches the
    end of its sequence.

    Events is a child of the dictionary class, which means that every argument
    given at its initialization will become a new key (with its associated
    value) in its memory. These keys will serve to create the parameter
    dictionary passed to the audio instrument instance playing this event.
    Inside the instrument instance, the value associated to these keys will
    be retrieved as instance's attributes, with the syntax self.key_name.

    The user can create as many new keys as needed to control its instrument,
    but there is already a number of pre-defined keys for which Events will
    do some processing and build useful parameters. Here is the list, grouped
    by themes, of pre-defined keys to overwrite:

    **Instrument**
        - instr: class, optional
            Reference to a custom class with which the events will be played.
            Defaults to DefaultInstrument.
        - signal: string, optional
            Name of the attribute in the instrument defintion retrieved as the
            output signal of the Events object. The sig() method returns the
            sum, as an audio signal, of all active instances. This can be useful
            to do post-processing on the signal produced by the events. Defaults
            to None.

    **Constants**
        - bpm: int, optional
            Beat-Per-Minute value used by the `beat` key to compute event's duration.
            Defaults to 120.
        - outs: int, optional
            Number of output channels in the audio signal returned by the sig() method.
            This value should match the number of audio streams produced by the instrument.
            Defaults to 2.

    **Duration keys**
        - dur: float, PyoObject or EventGenerator, optional
            Duration, in seconds, before the next event. Defaults to 1.
        - beat: float, PyoObject or EventGenerator, optional
            Duration, in beat value, before the next event (1 beat = quarter note at BPM).
            If defined, this value will be used to compute the duration in seconds for the
            `dur` key. Defaults to None.
        - durmul: float, PyoObject or EventGenerator, optional
            Event duration multiplier (only affects the duration of the event's lifetime,
            not the time to wait before the next event). Defaults to 1.
        - tail: float, PyoObject or EventGenerator, optional
            Duration, in seconds, to wait before deleting the instrument's instance when
            its envelope has ended. Useful to let a reverb tail to finish before cleaning-up
            the instance. Defaults to 2.

    **Amplitude keys**
        - amp: float, PyoObject or EventGenerator, optional
            Linear gain for the event (1 is nominal gain). Defaults to 0.7.
        - dB: float, PyoObject or EventGenerator, optional
            Gain, in decibels, for the event. If defined, this value will be used to compute
            the linear gain for the `amp` key. Defaults to None.
        - midivel: float, PyoObject or EventGenerator, optional
            Midi velocity, between 0 and 127, for the event. If defined, this value will be
            used to compute the linear gain for the `amp` key. Defaults to None.

    **Envelope keys**
        - envelope: PyoTableObject, optional
            User-defined envelope as a PyoTableObject. If defined, this will be the envelope
            created for the event. Defaults to None.
        - attack: float, PyoObject or EventGenerator, optional
            Rising time, in seconds, of an ASR or ADSR envelope. This envelope is created if
            `envelope` is None. Defaults to 0.005.
        - decay: float, PyoObject or EventGenerator, optional
            If defined, its the decay time, in seconds, of an ADSR envelope, otherwise the
            envelope will be an ASR (Attack - Sustain - Release). Defaults to None.
        - sustain: float, PyoObject or EventGenerator, optional
            Sustain linear gain of an ADSR or ASR envelope. Defaults to 0.7.
        - release: float, PyoObject or EventGenerator, optional
            Release time, in seconds, of an ASR or ADSR envelope. This envelope is created if
            `envelope` is None. Defaults to 0.05.

    **Pitch keys**
        - freq: float, PyoObject or EventGenerator, optional
            Frequency, in cycle per seconds, for the event. Defaults to 250.
        - midinote: float, PyoObject or EventGenerator, optional
            Midi pitch, between 0 and 127, for the event. If defined, this value will be used
            to compute the frequency in cycles per second for the `freq` key. Defaults to None.
        - degree: float, PyoObject or EventGenerator, optional
            Octave.degree pitch notation (ex.: 6.00, 6.04, 6.07). If defined, this value will be
            used to compute the frequency in cycles per second for the `freq` key. Defaults to None.
        - transpo: float, PyoObject or EventGenerator, optional
            Transposition, in midi note value (-12 is an octave lower), automatically computed in
            the value of the `freq` key. Defaults to 0.

    **Ending keys**
        - atend: python callable, optional
            If defined, a function to call when all events are played. This can be useful to sequence
            multiple Events objects. Defaults to None

    >>> s = Server().boot()
    >>> s.start()
    >>> env = CosTable([(0,0.0),(64,1.0),(8191,0.0)])
    >>> scl = EventScale(root="C", scale="egyptian", first=4, octaves=3)
    >>> seg = RandInt(max=6, freq=0.5)
    >>> step = RandInt(max=6, freq=0.75, add=-3)
    >>> note = EventSlide(scl, seg, step)
    >>> e = Events(midinote=note, beat=1/4., db=[-3, -9, -9], envelope=env, durmul=1.25).play()

    """

    def __init__(self, **args):
        # Instrument key
        self["instr"] = DefaultInstrument
        self["signal"] = None
        # Constant keys
        self["bpm"] = 120
        self["outs"] = 2
        # Duration keys
        self["dur"] = 1
        self["beat"] = None
        self["durmul"] = 1
        self["tail"] = 2
        # Amplitude keys
        self["amp"] = 0.7
        self["db"] = None
        self["midivel"] = None
        # Envelope keys
        self["envelope"] = None
        self["attack"] = 0.005
        self["decay"] = None
        self["sustain"] = 0.7
        self["release"] = 0.05
        # Pitch keys
        self["freq"] = 250
        self["midinote"] = None
        self["degree"] = None
        self["transpo"] = 0
        # Ending keys
        self["atend"] = None

        self.instanceId = 0
        self.maxInstanceId = 2 ** 31

        # Add user-supplied arguments as dict attributes.
        for item in args.items():
            self[item[0]] = item[1]

        self.callNextEvent = Pattern(self._processEvent).stop()

        self.currentDict = {}

        self.output = Sig([0] * self["outs"])

    def events(self):
        """ Return a copy of this Events object. """
        evts = Events()
        for attr in list(self):
            try:
                evts[attr] = self[attr].copy()
            except:
                evts[attr] = self[attr]
        return evts

    def play(self, dur=0, delay=0):
        """
        Start the events playback.

        This method returns `self`, allowing it to be applied at the object
        creation.

        :Args:

            dur: float, optional
                Duration, in seconds, of the object's activation. The default
                is 0 and means infinite duration.
            delay: float, optional
                Delay, in seconds, before the object's activation. Defaults to 0.

        """
        if not issubclass(self["instr"], EventInstrument):
            print("`instr` argument must be a sub-class of EventInstrument...")
            print("... Events is using DefaultInstrument!")
            self["instr"] = DefaultInstrument

        self.actives = {}

        self.callAtEnd = None
        if callable(self["atend"]):
            self.callAtEnd = CallAfter(self["atend"], 0).stop()

        for key in [k for k in self.keys() if k not in ["instr", "signal", "bpm", "outs", "atend"]]:
            if self[key] is None or isinstance(self[key], PyoObjectBase):
                pass
            elif type(self[key]) in [type([]), type(())]:
                self[key] = EventSeq(self[key])
            elif type(self[key]) in [type(0), type(0.0)]:
                self[key] = EventSeq([self[key]])
            else:
                self[key].resetEmbeddedGenerator()
                self[key].reset()

        self.callNextEvent.play(dur=dur, delay=delay)

        return self

    def stop(self, wait=0):
        """
        Stop the events playback.

        :Args:

            wait: float, optional
                Delay, in seconds, before the process is actually stopped.
                Defaults to 0.

        """
        self.callNextEvent.stop(wait=wait)

    def getCurrentDict(self):
        return self.currentDict

    def sig(self):
        """
        Return the audio output signal (sum of all active instances), if defined.

        The audio output signal of an Events object is the sum of the active
        instrument instances's attribute whose name is the same as given to
        the 'signal' key. The number of audio streams in the output signal is
        determined by the value for the key 'outs', it should match the number
        of audio streams produced by the instrument.

        """
        return self.output

    def _processEvent(self):
        """Create instrument instances and add them to the active list."""

        quarterDur = 60.0 / self["bpm"]

        self.currentDict = {
            "removeFunction": self._remove,
            "bpm": self["bpm"],
            "outs": self["outs"],
            "instanceId": self.instanceId,
        }

        # Compute time before next event and current event's duration.
        if self["beat"] is not None:
            nextBeat = getValueFromAttribute(self, "beat", self.currentDict)
            if nextBeat is not None:
                nextDurMul = getValueFromAttribute(self, "durmul", self.currentDict)
                if nextDurMul is not None:
                    timeBeforeNextEvent = quarterDur * nextBeat
                    self.currentDict["dur"] = timeBeforeNextEvent * nextDurMul
        else:
            nextDur = getValueFromAttribute(self, "dur", self.currentDict)
            if nextDur is not None:
                nextDurMul = getValueFromAttribute(self, "durmul", self.currentDict)
                if nextDurMul is not None:
                    timeBeforeNextEvent = nextDur
                    self.currentDict["dur"] = timeBeforeNextEvent * nextDurMul

        if "dur" not in self.currentDict:
            self.callNextEvent.stop()
            return
        else:
            self.callNextEvent.time = timeBeforeNextEvent

        ending = False

        # Process event's every other attributes.
        proscribe = ["dur", "beat", "instr", "bpm", "outs", "midinote", "degree", "db", "midivel", "atend", "signal"]
        for arg in [k for k in self.keys() if k not in proscribe]:
            if arg == "freq":
                transpo = getValueFromAttribute(self, "transpo", self.currentDict, valueIfNone=0)
                transpo = midiToTranspo(transpo + 60)
                self.currentDict["transpo"] = transpo
                if self["midinote"] is not None:
                    midiNote = getValueFromAttribute(self, "midinote", self.currentDict)
                    if midiNote is not None:
                        self.currentDict["midinote"] = midiNote
                        self.currentDict["freq"] = midiToHz(midiNote) * transpo
                    elif self["midinote"].stopEventsWhenDone:
                        ending = True
                elif self["degree"] is not None:
                    degree = getValueFromAttribute(self, "degree", self.currentDict)
                    if degree is not None:
                        midiNote = degreeToMidiNote(degree)
                        self.currentDict["midinote"] = midiNote
                        self.currentDict["freq"] = midiToHz(midiNote) * transpo
                    elif self["degree"].stopEventsWhenDone:
                        ending = True
                else:
                    freq = getValueFromAttribute(self, "freq", self.currentDict)
                    if freq is not None:
                        self.currentDict["freq"] = freq * transpo
                    elif self["freq"].stopEventsWhenDone:
                        ending = True
            elif arg == "amp":
                if self["db"] is not None:
                    db = getValueFromAttribute(self, "db", self.currentDict)
                    if db is not None:
                        self.currentDict["db"] = db
                        self.currentDict["amp"] = pow(10.0, db * 0.05)
                    elif self["db"].stopEventsWhenDone:
                        ending = True
                elif self["midivel"] is not None:
                    midivel = getValueFromAttribute(self, "midivel", self.currentDict)
                    if midivel is not None:
                        midivel = max(min(midivel, 127), 0)
                        self.currentDict["midivel"] = midivel
                        self.currentDict["amp"] = midivel / 127.0
                    elif self["midivel"].stopEventsWhenDone:
                        ending = True
                else:
                    amp = getValueFromAttribute(self, "amp", self.currentDict)
                    if amp is not None:
                        self.currentDict["amp"] = amp
                    elif self["amp"].stopEventsWhenDone:
                        ending = True
            elif self[arg] == None:
                self.currentDict[arg] = None
            else:
                value = getValueFromAttribute(self, arg, self.currentDict)
                if value is not None:
                    self.currentDict[arg] = value
                elif self[arg].stopEventsWhenDone:
                    ending = True

        if ending:
            if self.callAtEnd is not None:
                self.callAtEnd.play()
            self.callNextEvent.stop()
        else:
            self.actives[self.instanceId] = self["instr"](**self.currentDict)
            if self["signal"] is not None:
                self.output.value = sum(
                    [
                        getattr(instr, self["signal"])
                        for instr in self.actives.values()
                        if hasattr(instr, self["signal"])
                    ]
                )

        self.instanceId += 1
        if self.instanceId >= self.maxInstanceId:
            self.instanceId = 0

    def _remove(self, instanceId):
        """ Removes an instrument instance from the active list. """
        if instanceId in self.actives:
            del self.actives[instanceId]
        if self["signal"] is not None:
            self.output.value = sum(
                [getattr(instr, self["signal"]) for instr in self.actives.values() if hasattr(instr, self["signal"])]
            )
