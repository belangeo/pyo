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


class MatrixRec(PyoObject):

    def __init__(self, input, matrix, fadetime=0, delay=0):
        pyoArgsAssert(self, "omni", input, matrix, fadetime, delay)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._in_fader = InputFader(input).stop()
        in_fader, matrix, fadetime, delay, lmax = convertArgsToLists(self._in_fader, matrix, fadetime, delay)
        self._base_objs = [
            MatrixRec_base(wrap(in_fader, i), wrap(matrix, i), wrap(fadetime, i), wrap(delay, i))
            for i in range(len(matrix))
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)


class MatrixRecLoop(PyoObject):

    def __init__(self, input, matrix):
        pyoArgsAssert(self, "om", input, matrix)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._in_fader = InputFader(input)
        in_fader, matrix, lmax = convertArgsToLists(self._in_fader, matrix)
        self._base_objs = [MatrixRecLoop_base(wrap(in_fader, i), wrap(matrix, i)) for i in range(len(matrix))]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)


class MatrixPointer(PyoObject):

    def __init__(self, matrix, x, y, mul=1, add=0):
        pyoArgsAssert(self, "mooOO", matrix, x, y, mul, add)
        PyoObject.__init__(self, mul, add)
        self._matrix = matrix
        self._x = x
        self._y = y
        matrix, x, y, mul, add, lmax = convertArgsToLists(matrix, x, y, mul, add)
        self._base_objs = [
            MatrixPointer_base(wrap(matrix, i), wrap(x, i), wrap(y, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setMatrix(self, x):
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX(self, x):
        pyoArgsAssert(self, "o", x)
        self._x = x
        x, lmax = convertArgsToLists(x)
        [obj.setX(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setY(self, x):
        pyoArgsAssert(self, "o", x)
        self._y = x
        x, lmax = convertArgsToLists(x)
        [obj.setY(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def matrix(self):
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, x):
        self.setX(x)

    @property
    def y(self):
        return self._y

    @y.setter
    def y(self, x):
        self.setY(x)


class MatrixMorph(PyoObject):

    def __init__(self, input, matrix, sources):
        pyoArgsAssert(self, "oml", input, matrix, sources)
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._sources = sources
        self._in_fader = InputFader(input)
        in_fader, matrix, lmax = convertArgsToLists(self._in_fader, matrix)
        self._base_sources = [source[0] for source in sources]
        self._base_objs = [
            MatrixMorph_base(wrap(in_fader, i), wrap(matrix, i), self._base_sources) for i in range(len(matrix))
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        pyoArgsAssert(self, "m", x)
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSources(self, x):
        pyoArgsAssert(self, "l", x)
        self._sources = x
        self._base_sources = [source[0] for source in x]
        [obj.setSources(self._base_sources) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def matrix(self):
        return self._matrix

    @matrix.setter
    def matrix(self, x):
        self.setMatrix(x)

    @property
    def sources(self):
        return self._sources

    @sources.setter
    def sources(self, x):
        self.setSources(x)
