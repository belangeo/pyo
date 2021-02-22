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

from ._core import *


class PVAnal(PyoPVObject):

    def __init__(self, input, size=1024, overlaps=4, wintype=2, callback=None):
        pyoArgsAssert(self, "oiiic", input, size, overlaps, wintype, callback)
        PyoPVObject.__init__(self)
        self._input = input
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        self._callback = callback
        self._in_fader = InputFader(input)
        in_fader, size, overlaps, wintype, callback, lmax = convertArgsToLists(
            self._in_fader, size, overlaps, wintype, callback
        )
        self._base_objs = [
            PVAnal_base(wrap(in_fader, i), wrap(size, i), wrap(overlaps, i), wrap(wintype, i), wrap(callback, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOverlaps(self, x):
        pyoArgsAssert(self, "i", x)
        self._overlaps = x
        x, lmax = convertArgsToLists(x)
        [obj.setOverlaps(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCallback(self, x):
        pyoArgsAssert(self, "c", x)
        self._callback = x
        x, lmax = convertArgsToLists(x)
        [obj.setCallback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def overlaps(self):
        return self._overlaps

    @overlaps.setter
    def overlaps(self, x):
        self.setOverlaps(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class PVSynth(PyoObject):

    def __init__(self, input, wintype=2, mul=1, add=0):
        pyoArgsAssert(self, "piOO", input, wintype, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._wintype = wintype
        input, wintype, mul, add, lmax = convertArgsToLists(self._input, wintype, mul, add)
        self._base_objs = [
            PVSynth_base(wrap(input, i), wrap(wintype, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class PVAddSynth(PyoObject):

    def __init__(self, input, pitch=1, num=100, first=0, inc=1, mul=1, add=0):
        pyoArgsAssert(self, "pOiiiOO", input, pitch, num, first, inc, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._pitch = pitch
        self._num = num
        self._first = first
        self._inc = inc
        input, pitch, num, first, inc, mul, add, lmax = convertArgsToLists(
            self._input, pitch, num, first, inc, mul, add
        )
        self._base_objs = [
            PVAddSynth_base(
                wrap(input, i), wrap(pitch, i), wrap(num, i), wrap(first, i), wrap(inc, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setNum(self, x):
        pyoArgsAssert(self, "i", x)
        self._num = x
        x, lmax = convertArgsToLists(x)
        [obj.setNum(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFirst(self, x):
        pyoArgsAssert(self, "i", x)
        self._first = x
        x, lmax = convertArgsToLists(x)
        [obj.setFirst(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInc(self, x):
        pyoArgsAssert(self, "i", x)
        self._inc = x
        x, lmax = convertArgsToLists(x)
        [obj.setInc(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def num(self):
        return self._num

    @num.setter
    def num(self, x):
        self.setNum(x)

    @property
    def first(self):
        return self._first

    @first.setter
    def first(self, x):
        self.setFirst(x)

    @property
    def inc(self):
        return self._inc

    @inc.setter
    def inc(self, x):
        self.setInc(x)


class PVTranspose(PyoPVObject):

    def __init__(self, input, transpo=1):
        pyoArgsAssert(self, "pO", input, transpo)
        PyoPVObject.__init__(self)
        self._input = input
        self._transpo = transpo
        input, transpo, lmax = convertArgsToLists(self._input, transpo)
        self._base_objs = [PVTranspose_base(wrap(input, i), wrap(transpo, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTranspo(self, x):
        pyoArgsAssert(self, "O", x)
        self._transpo = x
        x, lmax = convertArgsToLists(x)
        [obj.setTranspo(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def transpo(self):
        return self._transpo

    @transpo.setter
    def transpo(self, x):
        self.setTranspo(x)


class PVVerb(PyoPVObject):

    def __init__(self, input, revtime=0.75, damp=0.75):
        pyoArgsAssert(self, "pOO", input, revtime, damp)
        PyoPVObject.__init__(self)
        self._input = input
        self._revtime = revtime
        self._damp = damp
        input, revtime, damp, lmax = convertArgsToLists(self._input, revtime, damp)
        self._base_objs = [PVVerb_base(wrap(input, i), wrap(revtime, i), wrap(damp, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRevtime(self, x):
        pyoArgsAssert(self, "O", x)
        self._revtime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRevtime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDamp(self, x):
        pyoArgsAssert(self, "O", x)
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def revtime(self):
        return self._revtime

    @revtime.setter
    def revtime(self, x):
        self.setRevtime(x)

    @property
    def damp(self):
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)


class PVGate(PyoPVObject):

    def __init__(self, input, thresh=-20, damp=0.0, inverse=False):
        pyoArgsAssert(self, "pOOb", input, thresh, damp, inverse)
        PyoPVObject.__init__(self)
        self._input = input
        self._thresh = thresh
        self._damp = damp
        self._inverse = inverse
        input, thresh, damp, inverse, lmax = convertArgsToLists(self._input, thresh, damp, inverse)
        self._base_objs = [
            PVGate_base(wrap(input, i), wrap(thresh, i), wrap(damp, i), wrap(inverse, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setThresh(self, x):
        pyoArgsAssert(self, "O", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDamp(self, x):
        pyoArgsAssert(self, "O", x)
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInverse(self, x):
        pyoArgsAssert(self, "b", x)
        self._inverse = x
        x, lmax = convertArgsToLists(x)
        [obj.setInverse(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def thresh(self):
        return self._thresh

    @thresh.setter
    def thresh(self, x):
        self.setThresh(x)

    @property
    def damp(self):
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)

    @property
    def inverse(self):
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)


class PVCross(PyoPVObject):

    def __init__(self, input, input2, fade=1):
        pyoArgsAssert(self, "ppO", input, input2, fade)
        PyoPVObject.__init__(self)
        self._input = input
        self._input2 = input2
        self._fade = fade
        input, input2, fade, lmax = convertArgsToLists(self._input, self._input2, fade)
        self._base_objs = [PVCross_base(wrap(input, i), wrap(input2, i), wrap(fade, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInput2(self, x):
        pyoArgsAssert(self, "p", x)
        self._input2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFade(self, x):
        pyoArgsAssert(self, "O", x)
        self._fade = x
        x, lmax = convertArgsToLists(x)
        [obj.setFade(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def fade(self):
        return self._fade

    @fade.setter
    def fade(self, x):
        self.setFade(x)


class PVMult(PyoPVObject):

    def __init__(self, input, input2):
        pyoArgsAssert(self, "pp", input, input2)
        PyoPVObject.__init__(self)
        self._input = input
        self._input2 = input2
        input, input2, lmax = convertArgsToLists(self._input, self._input2)
        self._base_objs = [PVMult_base(wrap(input, i), wrap(input2, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInput2(self, x):
        pyoArgsAssert(self, "p", x)
        self._input2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)


class PVMorph(PyoPVObject):

    def __init__(self, input, input2, fade=0.5):
        pyoArgsAssert(self, "ppO", input, input2, fade)
        PyoPVObject.__init__(self)
        self._input = input
        self._input2 = input2
        self._fade = fade
        input, input2, fade, lmax = convertArgsToLists(self._input, self._input2, fade)
        self._base_objs = [PVMorph_base(wrap(input, i), wrap(input2, i), wrap(fade, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInput2(self, x):
        pyoArgsAssert(self, "p", x)
        self._input2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFade(self, x):
        pyoArgsAssert(self, "O", x)
        self._fade = x
        x, lmax = convertArgsToLists(x)
        [obj.setFade(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def fade(self):
        return self._fade

    @fade.setter
    def fade(self, x):
        self.setFade(x)


class PVFilter(PyoPVObject):

    def __init__(self, input, table, gain=1, mode=0):
        pyoArgsAssert(self, "ptOi", input, table, gain, mode)
        PyoPVObject.__init__(self)
        self._input = input
        self._table = table
        self._gain = gain
        self._mode = mode
        input, table, gain, mode, lmax = convertArgsToLists(self._input, table, gain, mode)
        self._base_objs = [
            PVFilter_base(wrap(input, i), wrap(table, i), wrap(gain, i), wrap(mode, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setGain(self, x):
        pyoArgsAssert(self, "O", x)
        self._gain = x
        x, lmax = convertArgsToLists(x)
        [obj.setGain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def gain(self):
        return self._gain

    @gain.setter
    def gain(self, x):
        self.setGain(x)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)


class PVDelay(PyoPVObject):

    def __init__(self, input, deltable, feedtable, maxdelay=1.0, mode=0):
        pyoArgsAssert(self, "pttni", input, deltable, feedtable, maxdelay, mode)
        PyoPVObject.__init__(self)
        self._input = input
        self._deltable = deltable
        self._feedtable = feedtable
        self._maxdelay = maxdelay
        self._mode = mode
        input, deltable, feedtable, maxdelay, mode, lmax = convertArgsToLists(
            self._input, deltable, feedtable, maxdelay, mode
        )
        self._base_objs = [
            PVDelay_base(wrap(input, i), wrap(deltable, i), wrap(feedtable, i), wrap(maxdelay, i), wrap(mode, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDeltable(self, x):
        pyoArgsAssert(self, "t", x)
        self._deltable = x
        x, lmax = convertArgsToLists(x)
        [obj.setDeltable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedtable(self, x):
        pyoArgsAssert(self, "t", x)
        self._feedtable = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedtable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def deltable(self):
        return self._deltable

    @deltable.setter
    def deltable(self, x):
        self.setDeltable(x)

    @property
    def feedtable(self):
        return self._feedtable

    @feedtable.setter
    def feedtable(self, x):
        self.setFeedtable(x)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)


class PVBuffer(PyoPVObject):

    def __init__(self, input, index, pitch=1.0, length=1.0):
        pyoArgsAssert(self, "poOn", input, index, pitch, length)
        PyoPVObject.__init__(self)
        self._input = input
        self._index = index
        self._pitch = pitch
        self._length = length
        input, index, pitch, length, lmax = convertArgsToLists(self._input, index, pitch, length)
        self._base_objs = [
            PVBuffer_base(wrap(input, i), wrap(index, i), wrap(pitch, i), wrap(length, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLength(self, x):
        pyoArgsAssert(self, "n", x)
        self._length = x
        x, lmax = convertArgsToLists(x)
        [obj.setLength(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def length(self):
        return self._length

    @length.setter
    def length(self, x):
        self.setLength(x)


class PVShift(PyoPVObject):

    def __init__(self, input, shift=0):
        pyoArgsAssert(self, "pO", input, shift)
        PyoPVObject.__init__(self)
        self._input = input
        self._shift = shift
        input, shift, lmax = convertArgsToLists(self._input, shift)
        self._base_objs = [PVShift_base(wrap(input, i), wrap(shift, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setShift(self, x):
        pyoArgsAssert(self, "O", x)
        self._shift = x
        x, lmax = convertArgsToLists(x)
        [obj.setShift(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def shift(self):
        return self._shift

    @shift.setter
    def shift(self, x):
        self.setShift(x)


class PVAmpMod(PyoPVObject):

    def __init__(self, input, basefreq=1, spread=0, shape=0):
        pyoArgsAssert(self, "pOOi", input, basefreq, spread, shape)
        PyoPVObject.__init__(self)
        self._input = input
        self._basefreq = basefreq
        self._spread = spread
        self._shape = shape
        input, basefreq, spread, shape, lmax = convertArgsToLists(self._input, basefreq, spread, shape)
        self._base_objs = [
            PVAmpMod_base(wrap(input, i), wrap(basefreq, i), wrap(spread, i), wrap(shape, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBasefreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._basefreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setBasefreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpread(self, x):
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setShape(self, x):
        pyoArgsAssert(self, "i", x)
        self._shape = x
        x, lmax = convertArgsToLists(x)
        [obj.setShape(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def basefreq(self):
        return self._basefreq

    @basefreq.setter
    def basefreq(self, x):
        self.setBasefreq(x)

    @property
    def spread(self):
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def shape(self):
        return self._shape

    @shape.setter
    def shape(self, x):
        self.setShape(x)


class PVFreqMod(PyoPVObject):

    def __init__(self, input, basefreq=1, spread=0, depth=0.1, shape=0):
        pyoArgsAssert(self, "pOOOi", input, basefreq, spread, depth, shape)
        PyoPVObject.__init__(self)
        self._input = input
        self._basefreq = basefreq
        self._spread = spread
        self._depth = depth
        self._shape = shape
        input, basefreq, spread, depth, shape, lmax = convertArgsToLists(self._input, basefreq, spread, depth, shape)
        self._base_objs = [
            PVFreqMod_base(wrap(input, i), wrap(basefreq, i), wrap(spread, i), wrap(depth, i), wrap(shape, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBasefreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._basefreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setBasefreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpread(self, x):
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDepth(self, x):
        pyoArgsAssert(self, "O", x)
        self._depth = x
        x, lmax = convertArgsToLists(x)
        [obj.setDepth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setShape(self, x):
        pyoArgsAssert(self, "i", x)
        self._shape = x
        x, lmax = convertArgsToLists(x)
        [obj.setShape(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def basefreq(self):
        return self._basefreq

    @basefreq.setter
    def basefreq(self, x):
        self.setBasefreq(x)

    @property
    def spread(self):
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def depth(self):
        return self._depth

    @depth.setter
    def depth(self, x):
        self.setDepth(x)

    @property
    def shape(self):
        return self._shape

    @shape.setter
    def shape(self, x):
        self.setShape(x)


class PVBufLoops(PyoPVObject):

    def __init__(self, input, low=1.0, high=1.0, mode=0, length=1.0):
        pyoArgsAssert(self, "pOOin", input, low, high, mode, length)
        PyoPVObject.__init__(self)
        self._input = input
        self._low = low
        self._high = high
        self._mode = mode
        self._length = length
        input, low, high, mode, length, lmax = convertArgsToLists(self._input, low, high, mode, length)
        self._base_objs = [
            PVBufLoops_base(wrap(input, i), wrap(low, i), wrap(high, i), wrap(mode, i), wrap(length, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLow(self, x):
        pyoArgsAssert(self, "O", x)
        self._low = x
        x, lmax = convertArgsToLists(x)
        [obj.setLow(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHigh(self, x):
        pyoArgsAssert(self, "O", x)
        self._high = x
        x, lmax = convertArgsToLists(x)
        [obj.setHigh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def low(self):
        return self._low

    @low.setter
    def low(self, x):
        self.setLow(x)

    @property
    def high(self):
        return self._high

    @high.setter
    def high(self, x):
        self.setHigh(x)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)


class PVBufTabLoops(PyoPVObject):

    def __init__(self, input, speed, length=1.0):
        pyoArgsAssert(self, "ptn", input, speed, length)
        PyoPVObject.__init__(self)
        self._input = input
        self._speed = speed
        self._length = length
        input, speed, length, lmax = convertArgsToLists(self._input, speed, length)
        self._base_objs = [PVBufTabLoops_base(wrap(input, i), wrap(speed, i), wrap(length, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpeed(self, x):
        pyoArgsAssert(self, "t", x)
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def speed(self):
        return self._speed

    @speed.setter
    def speed(self, x):
        self.setSpeed(x)


class PVMix(PyoPVObject):

    def __init__(self, input, input2):
        pyoArgsAssert(self, "pp", input, input2)
        PyoPVObject.__init__(self)
        self._input = input
        self._input2 = input2
        input, input2, lmax = convertArgsToLists(self._input, self._input2)
        self._base_objs = [PVMix_base(wrap(input, i), wrap(input2, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x):
        pyoArgsAssert(self, "p", x)
        self._input = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInput2(self, x):
        pyoArgsAssert(self, "p", x)
        self._input2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInput2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)
