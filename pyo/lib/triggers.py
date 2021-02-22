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


class Trig(PyoObject):

    def __init__(self):
        PyoObject.__init__(self)
        self._base_objs = [Trig_base()]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass


class Metro(PyoObject):

    def __init__(self, time=1, poly=1):
        pyoArgsAssert(self, "OI", time, poly)
        PyoObject.__init__(self)
        self._time = time
        self._poly = poly
        time, lmax = convertArgsToLists(time)
        self._base_objs = [
            Metro_base(wrap(time, i) * poly, (float(j) / poly)) for i in range(lmax) for j in range(poly)
        ]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i) * self._poly) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)


class Seq(PyoObject):

    def __init__(self, time=1, seq=[1], poly=1, onlyonce=False, speed=1):
        pyoArgsAssert(self, "OlIBO", time, seq, poly, onlyonce, speed)
        PyoObject.__init__(self)
        self._time = time
        self._seq = seq
        self._poly = poly
        self._onlyonce = onlyonce
        self._speed = speed
        time, speed, lmax = convertArgsToLists(time, speed)
        if type(seq[0]) != list:
            self._base_players = [Seqer_base(wrap(time, i), seq, poly, onlyonce, wrap(speed, i)) for i in range(lmax)]
        else:
            seqlen = len(seq)
            lmax = max(seqlen, lmax)
            self._base_players = [
                Seqer_base(wrap(time, i), wrap(seq, i), poly, onlyonce, wrap(speed, i)) for i in range(lmax)
            ]
        self._base_objs = [Seq_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setSpeed(self, x):
        pyoArgsAssert(self, "O", x)
        self._speed = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpeed(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setSeq(self, x):
        pyoArgsAssert(self, "l", x)
        self._seq = x
        if type(x[0]) != list:
            [obj.setSeq(x) for i, obj in enumerate(self._base_players)]
        else:
            [obj.setSeq(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setOnlyonce(self, x):
        pyoArgsAssert(self, "B", x)
        self._onlyonce = x
        [obj.setOnlyonce(x) for obj in self._base_players]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def speed(self):
        return self._speed

    @speed.setter
    def speed(self, x):
        self.setSpeed(x)

    @property
    def seq(self):
        return self._seq

    @seq.setter
    def seq(self, x):
        self.setSeq(x)

    @property
    def onlyonce(self):
        return self._onlyonce

    @onlyonce.setter
    def onlyonce(self, x):
        self.setOnlyonce(x)


class Cloud(PyoObject):

    def __init__(self, density=10, poly=1):
        pyoArgsAssert(self, "OI", density, poly)
        PyoObject.__init__(self)
        self._density = density
        self._poly = poly
        density, lmax = convertArgsToLists(density)
        self._base_players = [Clouder_base(wrap(density, i), poly) for i in range(lmax)]
        self._base_objs = [Cloud_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]

    def setDensity(self, x):
        pyoArgsAssert(self, "O", x)
        self._density = x
        x, lmax = convertArgsToLists(x)
        [obj.setDensity(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def density(self):
        return self._density

    @density.setter
    def density(self, x):
        self.setDensity(x)


class Beat(PyoObject):

    def __init__(self, time=0.125, taps=16, w1=80, w2=50, w3=30, poly=1, onlyonce=False):
        pyoArgsAssert(self, "OinnnIB", time, taps, w1, w2, w3, poly, onlyonce)
        PyoObject.__init__(self)
        self._tap_dummy = []
        self._amp_dummy = []
        self._dur_dummy = []
        self._end_dummy = []
        self._time = time
        self._taps = taps
        self._w1 = w1
        self._w2 = w2
        self._w3 = w3
        self._poly = poly
        self._onlyonce = onlyonce
        time, taps, w1, w2, w3, lmax = convertArgsToLists(time, taps, w1, w2, w3)
        self._base_players = [
            Beater_base(wrap(time, i), wrap(taps, i), wrap(w1, i), wrap(w2, i), wrap(w3, i), poly, onlyonce)
            for i in range(lmax)
        ]
        self._base_objs = [Beat_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._tap_objs = [BeatTapStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._amp_objs = [BeatAmpStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._dur_objs = [BeatDurStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._end_objs = [BeatEndStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]

    def __getitem__(self, i):
        if i == "tap":
            self._tap_dummy.append(Dummy([obj for obj in self._tap_objs]))
            return self._tap_dummy[-1]
        if i == "amp":
            self._amp_dummy.append(Dummy([obj for obj in self._amp_objs]))
            return self._amp_dummy[-1]
        if i == "dur":
            self._dur_dummy.append(Dummy([obj for obj in self._dur_objs]))
            return self._dur_dummy[-1]
        if i == "end":
            self._end_dummy.append(Dummy([obj for obj in self._end_objs]))
            return self._end_dummy[-1]
        if type(i) == slice:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def get(self, identifier="amp", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def reset(self):
        [obj.reset() for obj in self._base_players]

    def new(self, now=False):
        [obj.new(now) for i, obj in enumerate(self._base_players)]

    def fill(self):
        [obj.fill() for i, obj in enumerate(self._base_players)]

    def store(self, x):
        pyoArgsAssert(self, "I", x)
        [obj.store(x) for i, obj in enumerate(self._base_players)]

    def recall(self, x):
        pyoArgsAssert(self, "I", x)
        [obj.recall(x) for i, obj in enumerate(self._base_players)]

    def getPresets(self):
        if len(self._base_players) == 1:
            return self._base_players[0].getPresets()
        else:
            return [obj.getPresets() for obj in self._base_players]

    def setPresets(self, x):
        pyoArgsAssert(self, "l", x)
        if len(self._base_players) == 1:
            return self._base_players[0].setPresets(x)
        else:
            return [obj.setPresets(x[i]) for i, obj in enumerate(self._base_players)]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setTaps(self, x):
        pyoArgsAssert(self, "I", x)
        self._taps = x
        x, lmax = convertArgsToLists(x)
        [obj.setTaps(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setW1(self, x):
        pyoArgsAssert(self, "n", x)
        self.setWeights(w1=x)

    def setW2(self, x):
        pyoArgsAssert(self, "n", x)
        self.setWeights(w2=x)

    def setW3(self, x):
        pyoArgsAssert(self, "n", x)
        self.setWeights(w3=x)

    def setWeights(self, w1=None, w2=None, w3=None):
        if w1 is not None:
            self._w1 = w1
        if w2 is not None:
            self._w2 = w2
        if w3 is not None:
            self._w3 = w3
        w1, w2, w3, lmax = convertArgsToLists(w1, w2, w3)
        [obj.setWeights(wrap(w1, i), wrap(w2, i), wrap(w3, i)) for i, obj in enumerate(self._base_players)]

    def setOnlyonce(self, x):
        pyoArgsAssert(self, "B", x)
        self._onlyonce = x
        [obj.setOnlyonce(x) for obj in self._base_players]

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._tap_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._tap_objs)]
        self._amp_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._amp_objs)]
        self._dur_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._dur_objs)]
        self._end_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._end_objs)]
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._tap_objs]
        [obj.stop(wait) for obj in self._amp_objs]
        [obj.stop(wait) for obj in self._dur_objs]
        [obj.stop(wait) for obj in self._end_objs]
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def taps(self):
        return self._taps

    @taps.setter
    def taps(self, x):
        self.setTaps(x)

    @property
    def w1(self):
        return self._w1

    @w1.setter
    def w1(self, x):
        self.setW1(x)

    @property
    def w2(self):
        return self._w2

    @w2.setter
    def w2(self, x):
        self.setW2(x)

    @property
    def w3(self):
        return self._w3

    @w3.setter
    def w3(self, x):
        self.setW3(x)

    @property
    def onlyonce(self):
        return self._onlyonce

    @onlyonce.setter
    def onlyonce(self, x):
        self.setOnlyonce(x)


class TrigRandInt(PyoObject):

    def __init__(self, input, max=100.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, max, mul, add, lmax = convertArgsToLists(self._in_fader, max, mul, add)
        self._base_objs = [
            TrigRandInt_base(wrap(in_fader, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class TrigRand(PyoObject):

    def __init__(self, input, min=0.0, max=1.0, port=0.0, init=0.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOnnOO", input, min, max, port, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._port = port
        self._in_fader = InputFader(input)
        in_fader, min, max, port, init, mul, add, lmax = convertArgsToLists(
            self._in_fader, min, max, port, init, mul, add
        )
        self._base_objs = [
            TrigRand_base(
                wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(port, i), wrap(init, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPort(self, x):
        pyoArgsAssert(self, "n", x)
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setPort(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def port(self):
        return self._port

    @port.setter
    def port(self, x):
        self.setPort(x)


class TrigChoice(PyoObject):

    def __init__(self, input, choice, port=0.0, init=0.0, mul=1, add=0):
        pyoArgsAssert(self, "olnnOO", input, choice, port, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._choice = choice
        self._port = port
        self._in_fader = InputFader(input)
        in_fader, port, init, mul, add, lmax = convertArgsToLists(self._in_fader, port, init, mul, add)
        if type(choice[0]) != list:
            self._base_objs = [
                TrigChoice_base(wrap(in_fader, i), choice, wrap(port, i), wrap(init, i), wrap(mul, i), wrap(add, i))
                for i in range(lmax)
            ]
        else:
            choicelen = len(choice)
            lmax = max(choicelen, lmax)
            self._base_objs = [
                TrigChoice_base(
                    wrap(in_fader, i), wrap(choice, i), wrap(port, i), wrap(init, i), wrap(mul, i), wrap(add, i)
                )
                for i in range(lmax)
            ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setChoice(self, x):
        pyoArgsAssert(self, "l", x)
        self._choice = x
        if type(x[0]) != list:
            [obj.setChoice(self._choice) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setChoice(wrap(self._choice, i)) for i, obj in enumerate(self._base_objs)]

    def setPort(self, x):
        pyoArgsAssert(self, "n", x)
        self._port = x
        x, lmax = convertArgsToLists(x)
        [obj.setPort(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def choice(self):
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)

    @property
    def port(self):
        return self._port

    @port.setter
    def port(self, x):
        self.setPort(x)


class TrigFunc(PyoObject):

    def __init__(self, input, function, arg=None):
        pyoArgsAssert(self, "oc", input, function)
        PyoObject.__init__(self)
        self._input = input
        self._function = getWeakMethodRef(function)
        self._arg = arg
        self._in_fader = InputFader(input)
        in_fader, function, arg, lmax = convertArgsToLists(self._in_fader, function, arg)
        self._base_objs = [
            TrigFunc_base(wrap(in_fader, i), WeakMethod(wrap(function, i)), wrap(arg, i)) for i in range(lmax)
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

    def setFunction(self, x):
        pyoArgsAssert(self, "c", x)
        self._function = getWeakMethodRef(x)
        x, lmax = convertArgsToLists(x)
        [obj.setFunction(WeakMethod(wrap(x, i))) for i, obj in enumerate(self._base_objs)]

    def setArg(self, x):
        self._arg = x
        x, lmax = convertArgsToLists(x)
        [obj.setArg(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def function(self):
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)

    @property
    def arg(self):
        return self._arg

    @arg.setter
    def arg(self, x):
        self.setArg(x)


class TrigEnv(PyoObject):

    def __init__(self, input, table, dur=1, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "otOiOO", input, table, dur, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._table = table
        self._dur = dur
        self._interp = interp
        self._in_fader = InputFader(input)
        in_fader, table, dur, interp, mul, add, lmax = convertArgsToLists(self._in_fader, table, dur, interp, mul, add)
        self._base_objs = [
            TrigEnv_base(wrap(in_fader, i), wrap(table, i), wrap(dur, i), wrap(interp, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class TrigLinseg(PyoObject):

    def __init__(self, input, list, mul=1, add=0):
        pyoArgsAssert(self, "olOO", input, list, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._list = list
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [TrigLinseg_base(wrap(in_fader, i), list, wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setList(self, x):
        pyoArgsAssert(self, "l", x)
        self._list = x
        [obj.setList(x) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        self.setList(x)

    def getPoints(self):
        return self._list

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)


class TrigExpseg(PyoObject):

    def __init__(self, input, list, exp=10, inverse=True, mul=1, add=0):
        pyoArgsAssert(self, "olnbOO", input, list, exp, inverse, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._list = list
        self._exp = exp
        self._inverse = inverse
        self._in_fader = InputFader(input)
        in_fader, exp, inverse, mul, add, lmax = convertArgsToLists(self._in_fader, exp, inverse, mul, add)
        self._base_objs = [
            TrigExpseg_base(wrap(in_fader, i), list, wrap(exp, i), wrap(inverse, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setList(self, x):
        pyoArgsAssert(self, "l", x)
        self._list = x
        [obj.setList(x) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInverse(self, x):
        pyoArgsAssert(self, "b", x)
        self._inverse = x
        x, lmax = convertArgsToLists(x)
        [obj.setInverse(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        self.setList(x)

    def getPoints(self):
        return self._list

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)

    @property
    def inverse(self):
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)


class TrigXnoise(PyoObject):

    def __init__(self, input, dist=0, x1=0.5, x2=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, x1, x2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._dist = dist
        self._x1 = x1
        self._x2 = x2
        self._in_fader = InputFader(input)
        in_fader, dist, x1, x2, mul, add, lmax = convertArgsToLists(self._in_fader, dist, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) in [bytes_t, unicode_t]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            TrigXnoise_base(wrap(in_fader, i), wrap(dist, i), wrap(x1, i), wrap(x2, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDist(self, x):
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes_t, unicode_t]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        pyoArgsAssert(self, "O", x)
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        pyoArgsAssert(self, "O", x)
        self._x2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def x1(self):
        return self._x1

    @x1.setter
    def x1(self, x):
        self.setX1(x)

    @property
    def x2(self):
        return self._x2

    @x2.setter
    def x2(self, x):
        self.setX2(x)


class TrigXnoiseMidi(PyoObject):

    def __init__(self, input, dist=0, x1=0.5, x2=0.5, scale=0, mrange=(0, 127), mul=1, add=0):
        pyoArgsAssert(self, "oOOixOO", input, x1, x2, scale, mrange, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._dist = dist
        self._x1 = x1
        self._x2 = x2
        self._scale = scale
        self._mrange = mrange
        self._in_fader = InputFader(input)
        in_fader, dist, x1, x2, scale, mrange, mul, add, lmax = convertArgsToLists(
            self._in_fader, dist, x1, x2, scale, mrange, mul, add
        )
        for i, t in enumerate(dist):
            if type(t) in [bytes_t, unicode_t]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            TrigXnoiseMidi_base(
                wrap(in_fader, i),
                wrap(dist, i),
                wrap(x1, i),
                wrap(x2, i),
                wrap(scale, i),
                wrap(mrange, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDist(self, x):
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes_t, unicode_t]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        pyoArgsAssert(self, "i", x)
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRange(self, mini, maxi):
        pyoArgsAssert(self, "ii", mini, maxi)
        self._mrange = (mini, maxi)
        mini, maxi, lmax = convertArgsToLists(mini, maxi)
        [obj.setRange(wrap(mini, i), wrap(maxi, i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        pyoArgsAssert(self, "O", x)
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        pyoArgsAssert(self, "O", x)
        self._x2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def x1(self):
        return self._x1

    @x1.setter
    def x1(self, x):
        self.setX1(x)

    @property
    def x2(self):
        return self._x2

    @x2.setter
    def x2(self, x):
        self.setX2(x)

    @property
    def scale(self):
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class Counter(PyoObject):

    def __init__(self, input, min=0, max=100, dir=0, mul=1, add=0):
        pyoArgsAssert(self, "oiiiOO", input, min, max, dir, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._dir = dir
        self._in_fader = InputFader(input)
        in_fader, min, max, dir, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, dir, mul, add)
        self._base_objs = [
            Counter_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(dir, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "i", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "i", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDir(self, x):
        pyoArgsAssert(self, "i", x)
        self._dir = x
        x, lmax = convertArgsToLists(x)
        [obj.setDir(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self, value=None):
        if value is not None:
            pyoArgsAssert(self, "i", value)
        value, lmax = convertArgsToLists(value)
        [obj.reset(wrap(value, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def dir(self):
        return self._dir

    @dir.setter
    def dir(self, x):
        self.setDir(x)


class Select(PyoObject):

    def __init__(self, input, value=0, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._value = value
        self._in_fader = InputFader(input)
        in_fader, value, mul, add, lmax = convertArgsToLists(self._in_fader, value, mul, add)
        self._base_objs = [
            Select_base(wrap(in_fader, i), wrap(value, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setValue(self, x):
        pyoArgsAssert(self, "i", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Change(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [Change_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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


class Thresh(PyoObject):

    def __init__(self, input, threshold=0.0, dir=0, mul=1, add=0):
        pyoArgsAssert(self, "oOiOO", input, threshold, dir, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._threshold = threshold
        self._dir = dir
        self._in_fader = InputFader(input)
        in_fader, threshold, dir, mul, add, lmax = convertArgsToLists(self._in_fader, threshold, dir, mul, add)
        self._base_objs = [
            Thresh_base(wrap(in_fader, i), wrap(threshold, i), wrap(dir, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThreshold(self, x):
        pyoArgsAssert(self, "O", x)
        self._threshold = x
        x, lmax = convertArgsToLists(x)
        [obj.setThreshold(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDir(self, x):
        pyoArgsAssert(self, "i", x)
        self._dir = x
        x, lmax = convertArgsToLists(x)
        [obj.setDir(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def threshold(self):
        return self._threshold

    @threshold.setter
    def threshold(self, x):
        self.setThreshold(x)

    @property
    def dir(self):
        return self._dir

    @dir.setter
    def dir(self, x):
        self.setDir(x)


class Percent(PyoObject):

    def __init__(self, input, percent=50.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, percent, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._percent = percent
        self._in_fader = InputFader(input)
        in_fader, percent, mul, add, lmax = convertArgsToLists(self._in_fader, percent, mul, add)
        self._base_objs = [
            Percent_base(wrap(in_fader, i), wrap(percent, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPercent(self, x):
        pyoArgsAssert(self, "O", x)
        self._percent = x
        x, lmax = convertArgsToLists(x)
        [obj.setPercent(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def percent(self):
        return self._percent

    @percent.setter
    def percent(self, x):
        self.setPercent(x)


class Timer(PyoObject):

    def __init__(self, input, input2, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", input, input2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = [
            Timer_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

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


class Iter(PyoObject):

    def __init__(self, input, choice, init=0.0, mul=1, add=0):
        pyoArgsAssert(self, "olnOO", input, choice, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._choice = choice
        self._in_fader = InputFader(input)
        in_fader, init, mul, add, lmax = convertArgsToLists(self._in_fader, init, mul, add)
        x = self._flatten(choice)
        if type(x[0]) != list:
            self._base_objs = [
                Iter_base(wrap(in_fader, i), x, wrap(init, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
            ]
        else:
            choicelen = len(x)
            lmax = max(choicelen, lmax)
            self._base_objs = [
                Iter_base(wrap(in_fader, i), wrap(x, i), wrap(init, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
            ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def _flatten(self, x):
        if type(x[0]) != list:
            lst = []
            for ob in x:
                if isinstance(ob, PyoObject):
                    lst.extend(ob.getBaseObjects())
                else:
                    lst.append(ob)
        else:
            lst = []
            for sub in x:
                sublst = []
                for ob in sub:
                    if isinstance(ob, PyoObject):
                        sublst.extend(ob.getBaseObjects())
                    else:
                        sublst.append(ob)
                lst.append(sublst)
        return lst

    def setChoice(self, x):
        pyoArgsAssert(self, "l", x)
        self._choice = x
        x = self._flatten(x)
        if type(x[0]) != list:
            [obj.setChoice(x) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setChoice(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self, x=0):
        pyoArgsAssert(self, "I", x)
        [obj.reset(x) for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def choice(self):
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)


class Count(PyoObject):

    def __init__(self, input, min=0, max=0, mul=1, add=0):
        pyoArgsAssert(self, "oiiOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Count_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "i", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "i", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class NextTrig(PyoObject):

    def __init__(self, input, input2, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", input, input2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = [
            NextTrig_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

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


class TrigVal(PyoObject):

    def __init__(self, input, value=0.0, init=0.0, mul=1, add=0):
        pyoArgsAssert(self, "oOnOO", input, value, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._value = value
        self._in_fader = InputFader(input)
        in_fader, value, init, mul, add, lmax = convertArgsToLists(self._in_fader, value, init, mul, add)
        self._base_objs = [
            TrigVal_base(wrap(in_fader, i), wrap(value, i), wrap(init, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setValue(self, x):
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Euclide(PyoObject):

    def __init__(self, time=0.125, taps=16, onsets=10, poly=1):
        pyoArgsAssert(self, "OiiI", time, taps, onsets, poly)
        PyoObject.__init__(self)
        self._tap_dummy = []
        self._amp_dummy = []
        self._dur_dummy = []
        self._end_dummy = []
        self._time = time
        self._taps = taps
        self._onsets = onsets
        self._poly = poly
        time, taps, onsets, lmax = convertArgsToLists(time, taps, onsets)
        self._base_players = [
            Beater_base(wrap(time, i), wrap(taps, i), wrap([100] * lmax, i), poly) for i in range(lmax)
        ]
        self._base_objs = [Beat_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._tap_objs = [BeatTapStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._amp_objs = [BeatAmpStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._dur_objs = [BeatDurStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._end_objs = [BeatEndStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        for i in range(lmax):
            preset = [wrap(taps, i)] + self.__generate__(wrap(onsets, i), wrap(taps, i))
            self._base_players[i].setPresets([preset])
            self._base_players[i].recall(0)

    def __generate__(self, m, k):
        if m < 1:
            m = 1
        if k < 1:
            k = 1
        if m > k:
            m = k
        k -= m
        mv, kv = [1], [0]
        while k > 1:
            if m > k:
                m, k = k, m - k
                mv, kv = mv + kv, mv
            else:
                m, k = m, k - m
                mv, kv = mv + kv, kv
        return mv * m + kv * k

    def __getitem__(self, i):
        if i == "tap":
            self._tap_dummy.append(Dummy([obj for obj in self._tap_objs]))
            return self._tap_dummy[-1]
        if i == "amp":
            self._amp_dummy.append(Dummy([obj for obj in self._amp_objs]))
            return self._amp_dummy[-1]
        if i == "dur":
            self._dur_dummy.append(Dummy([obj for obj in self._dur_objs]))
            return self._dur_dummy[-1]
        if i == "end":
            self._end_dummy.append(Dummy([obj for obj in self._end_objs]))
            return self._end_dummy[-1]
        if type(i) == slice:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def get(self, identifier="amp", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setTaps(self, x):
        pyoArgsAssert(self, "i", x)
        self._taps = x
        x, onsets, lmax = convertArgsToLists(x, self._onsets)
        for i in range(len(self._base_players)):
            preset = [wrap(x, i)] + self.__generate__(wrap(onsets, i), wrap(x, i))
            self._base_players[i].setPresets([preset])
            self._base_players[i].recall(0)

    def setOnsets(self, x):
        pyoArgsAssert(self, "i", x)
        self._onsets = x
        x, taps, lmax = convertArgsToLists(x, self._taps)
        for i in range(len(self._base_players)):
            preset = [wrap(taps, i)] + self.__generate__(wrap(x, i), wrap(taps, i))
            self._base_players[i].setPresets([preset])
            self._base_players[i].recall(0)

    def reset(self):
        [obj.reset() for obj in self._base_players]

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._tap_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._tap_objs)]
        self._amp_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._amp_objs)]
        self._dur_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._dur_objs)]
        self._end_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._end_objs)]
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._tap_objs]
        [obj.stop(wait) for obj in self._amp_objs]
        [obj.stop(wait) for obj in self._dur_objs]
        [obj.stop(wait) for obj in self._end_objs]
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def taps(self):
        return self._taps

    @taps.setter
    def taps(self, x):
        self.setTaps(x)

    @property
    def onsets(self):
        return self._onsets

    @onsets.setter
    def onsets(self, x):
        self.setOnsets(x)


class TrigBurst(PyoObject):

    def __init__(self, input, time=0.25, count=10, expand=1.0, ampfade=1.0, poly=1):
        pyoArgsAssert(self, "oOinnI", input, time, count, expand, ampfade, poly)
        PyoObject.__init__(self)
        self._tap_dummy = []
        self._amp_dummy = []
        self._dur_dummy = []
        self._end_dummy = []
        self._input = input
        self._time = time
        self._count = count
        self._expand = expand
        self._ampfade = ampfade
        self._poly = poly
        self._in_fader = InputFader(input)
        in_fader, time, count, expand, ampfade, lmax = convertArgsToLists(self._in_fader, time, count, expand, ampfade)
        self._base_players = [
            TrigBurster_base(wrap(in_fader, i), wrap(time, i), wrap(count, i), wrap(expand, i), wrap(ampfade, i), poly)
            for i in range(lmax)
        ]
        self._base_objs = [TrigBurst_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)]
        self._tap_objs = [
            TrigBurstTapStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)
        ]
        self._amp_objs = [
            TrigBurstAmpStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)
        ]
        self._dur_objs = [
            TrigBurstDurStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)
        ]
        self._end_objs = [
            TrigBurstEndStream_base(wrap(self._base_players, j), i) for i in range(poly) for j in range(lmax)
        ]
        self._init_play()

    def __getitem__(self, i):
        if i == "tap":
            self._tap_dummy.append(Dummy([obj for obj in self._tap_objs]))
            return self._tap_dummy[-1]
        if i == "amp":
            self._amp_dummy.append(Dummy([obj for obj in self._amp_objs]))
            return self._amp_dummy[-1]
        if i == "dur":
            self._dur_dummy.append(Dummy([obj for obj in self._dur_objs]))
            return self._dur_dummy[-1]
        if i == "end":
            self._end_dummy.append(Dummy([obj for obj in self._end_objs]))
            return self._end_dummy[-1]
        if type(i) == slice:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print("'i' too large!")

    def get(self, identifier="amp", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTime(self, x):
        pyoArgsAssert(self, "n", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setCount(self, x):
        pyoArgsAssert(self, "i", x)
        self._count = x
        x, lmax = convertArgsToLists(x)
        [obj.setCount(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setExpand(self, x):
        pyoArgsAssert(self, "n", x)
        self._expand = x
        x, lmax = convertArgsToLists(x)
        [obj.setExpand(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setAmpfade(self, x):
        pyoArgsAssert(self, "n", x)
        self._ampfade = x
        x, lmax = convertArgsToLists(x)
        [obj.setAmpfade(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._tap_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._tap_objs)]
        self._amp_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._amp_objs)]
        self._dur_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._dur_objs)]
        self._end_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._end_objs)]
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._tap_objs]
        [obj.stop(wait) for obj in self._amp_objs]
        [obj.stop(wait) for obj in self._dur_objs]
        [obj.stop(wait) for obj in self._end_objs]
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
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
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)

    @property
    def count(self):
        return self._count

    @count.setter
    def count(self, x):
        self.setCount(x)

    @property
    def expand(self):
        return self._expand

    @expand.setter
    def expand(self, x):
        self.setExpand(x)

    @property
    def ampfade(self):
        return self._ampfade

    @ampfade.setter
    def ampfade(self, x):
        self.setAmpfade(x)
