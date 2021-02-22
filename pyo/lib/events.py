# Copyright 2009-2021 Olivier Belanger
# 
# This file is part of pyo, a python module to help digital signal
# processing script creation.
#
# pyo is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# pyo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with pyo.  If not, see <http://www.gnu.org/licenses/>.

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
    oct = int(value / 12)
    deg = value % 12
    return "%d.%02d" % (oct, deg)


def getValueFromAttribute(master, key, currentDict, valueIfNone=None):
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

    def __init__(self, **args):
        EventInstrument.__init__(self, **args)
        self.osc = RCOsc(freq=self.freq, sharp=0.5, mul=self.env)
        self.sig = self.osc.mix(2).out()


# Event Scale
#############
class EventScale:

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
        if x not in self.rootDegrees:
            print("EventScale: does not recognize root '%s'..." % x)
            print("... Using 'C'.")
            x = "C"
        if x != self._root:
            self._root = x
            self._populate()

    def setScale(self, x):
        if x not in self.scales:
            print("EventScale: does not recognize scale '%s'..." % x)
            print("... Using 'major'.")
            x = "major"
        if x != self._scale:
            self._scale = x
            self._populate()

    def setFirst(self, x):
        if x != self._first:
            self._first = x
            self._populate()

    def setOctaves(self, x):
        if x != self._octaves:
            self._octaves = x
            self._populate()

    def setType(self, x):
        if x != self._type:
            self._type = x
            self._populate()

    @property
    def root(self):
        return self._root

    @root.setter
    def root(self, x):
        self.setRoot(x)

    @property
    def scale(self):
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)

    @property
    def first(self):
        return self._first

    @first.setter
    def first(self, x):
        self.setFirst(x)

    @property
    def octaves(self):
        return self._octaves

    @octaves.setter
    def octaves(self, x):
        self.setOctaves(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


# Event Generators
##################
class EventGenerator:

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
        return EventFilter(self, PYO_EVENT_FILTER_FLOOR)

    def ceil(self):
        return EventFilter(self, PYO_EVENT_FILTER_CEIL)

    def round(self):
        return EventFilter(self, PYO_EVENT_FILTER_ROUND)

    def abs(self):
        return EventFilter(self, PYO_EVENT_FILTER_ABS)

    def snap(self, choice):
        return EventFilter(self, PYO_EVENT_FILTER_SNAP, choice)

    def deviate(self, depth):
        return EventFilter(self, PYO_EVENT_FILTER_DEVIATE, depth)

    def clip(self, mini, maxi):
        return EventFilter(self, PYO_EVENT_FILTER_CLIP, mini, maxi)

    def scale(self, mini, maxi, expon):
        return EventFilter(self, PYO_EVENT_FILTER_SCALE, mini, maxi, expon)

    def rescale(self, inmin, inmax, outmin, outmax, expon):
        return EventFilter(self, PYO_EVENT_FILTER_RESCALE, inmin, inmax, outmin, outmax, expon)

    def iftrue(self, op, comp):
        return EventFilter(self, PYO_EVENT_FILTER_IFTRUE, op, comp)


class EventDummy(EventGenerator):

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

    def __init__(self, key, master=None):
        EventGenerator.__init__(self)
        self.key = key
        self.needToResetTarget = False
        self.externalMaster = master

    def getKey(self):
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
        evts = Events()
        for attr in list(self):
            try:
                evts[attr] = self[attr].copy()
            except:
                evts[attr] = self[attr]
        return evts

    def play(self, dur=0, delay=0):
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
        self.callNextEvent.stop(wait=wait)

    def getCurrentDict(self):
        return self.currentDict

    def sig(self):
        return self.output

    def _processEvent(self):

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
        if instanceId in self.actives:
            del self.actives[instanceId]
        if self["signal"] is not None:
            self.output.value = sum(
                [getattr(instr, self["signal"]) for instr in self.actives.values() if hasattr(instr, self["signal"])]
            )
