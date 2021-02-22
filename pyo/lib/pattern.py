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


class Pattern(PyoObject):

    def __init__(self, function, time=1, arg=None):
        pyoArgsAssert(self, "cO", function, time)
        PyoObject.__init__(self)
        self._function = getWeakMethodRef(function)
        self._time = time
        self._arg = arg
        function, time, arg, lmax = convertArgsToLists(function, time, arg)
        self._base_objs = [
            Pattern_base(WeakMethod(wrap(function, i)), wrap(time, i), wrap(arg, i)) for i in range(lmax)
        ]

    def setFunction(self, x):
        pyoArgsAssert(self, "c", x)
        self._function = getWeakMethodRef(x)
        x, lmax = convertArgsToLists(x)
        [obj.setFunction(WeakMethod(wrap(x, i))) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArg(self, x):
        self._arg = x
        x, lmax = convertArgsToLists(x)
        [obj.setArg(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, x=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    @property
    def function(self):
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def arg(self):
        return self._arg

    @arg.setter
    def arg(self, x):
        self.setArg(x)


class Score(PyoObject):

    def __init__(self, input, fname="event_"):
        pyoArgsAssert(self, "os", input, fname)
        PyoObject.__init__(self)
        self._input = input
        self._fname = fname
        self._in_fader = InputFader(input)
        in_fader, fname, lmax = convertArgsToLists(self._in_fader, fname)
        self._base_objs = [Score_base(wrap(in_fader, i), wrap(fname, i)) for i in range(lmax)]
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

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class CallAfter(PyoObject):

    def __init__(self, function, time=1, arg=None):
        pyoArgsAssert(self, "cn", function, time)
        PyoObject.__init__(self)
        self._function = getWeakMethodRef(function)
        self._time = time
        self._arg = arg
        function, time, arg, lmax = convertArgsToLists(function, time, arg)
        self._base_objs = [
            CallAfter_base(WeakMethod(wrap(function, i)), wrap(time, i), wrap(arg, i)) for i in range(lmax)
        ]
        self._init_play()

    def out(self, x=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def setTime(self, x):
        pyoArgsAssert(self, "n", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArg(self, x):
        self._arg = x
        x, lmax = convertArgsToLists(x)
        [obj.setArg(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def arg(self):
        return self._arg

    @arg.setter
    def arg(self, x):
        self.setArg(x)
