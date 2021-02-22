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


class Osc(PyoObject):

    def __init__(self, table, freq=1000, phase=0, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "tOOiOO", table, freq, phase, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._freq = freq
        self._phase = phase
        self._interp = interp
        table, freq, phase, interp, mul, add, lmax = convertArgsToLists(table, freq, phase, interp, mul, add)
        self._base_objs = [
            Osc_base(wrap(table, i), wrap(freq, i), wrap(phase, i), wrap(interp, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPhase(self, x):
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class OscLoop(PyoObject):

    def __init__(self, table, freq=1000, feedback=0, mul=1, add=0):
        pyoArgsAssert(self, "tOOOO", table, freq, feedback, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._freq = freq
        self._feedback = feedback
        table, freq, feedback, mul, add, lmax = convertArgsToLists(table, freq, feedback, mul, add)
        self._base_objs = [
            OscLoop_base(wrap(table, i), wrap(freq, i), wrap(feedback, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class OscTrig(PyoObject):

    def __init__(self, table, trig, freq=1000, phase=0, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "toOOiOO", table, trig, freq, phase, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._trig = trig
        self._freq = freq
        self._phase = phase
        self._interp = interp
        table, trig, freq, phase, interp, mul, add, lmax = convertArgsToLists(
            table, trig, freq, phase, interp, mul, add
        )
        self._base_objs = [
            OscTrig_base(
                wrap(table, i),
                wrap(trig, i),
                wrap(freq, i),
                wrap(phase, i),
                wrap(interp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTrig(self, x):
        pyoArgsAssert(self, "o", x)
        self._trig = x
        x, lmax = convertArgsToLists(x)
        [obj.setTrig(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPhase(self, x):
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def trig(self):
        return self._trig

    @trig.setter
    def trig(self, x):
        self.setTrig(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class OscBank(PyoObject):

    def __init__(
        self, table, freq=100, spread=1, slope=0.9, frndf=1, frnda=0, arndf=1, arnda=0, num=24, fjit=False, mul=1, add=0
    ):
        pyoArgsAssert(self, "tOOOOOOOibOO", table, freq, spread, slope, frndf, frnda, arndf, arnda, num, fjit, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._freq = freq
        self._spread = spread
        self._slope = slope
        self._frndf = frndf
        self._frnda = frnda
        self._arndf = arndf
        self._arnda = arnda
        self._fjit = fjit
        self._num = num
        table, freq, spread, slope, frndf, frnda, arndf, arnda, num, fjit, mul, add, lmax = convertArgsToLists(
            table, freq, spread, slope, frndf, frnda, arndf, arnda, num, fjit, mul, add
        )
        self._base_objs = [
            OscBank_base(
                wrap(table, i),
                wrap(freq, i),
                wrap(spread, i),
                wrap(slope, i),
                wrap(frndf, i),
                wrap(frnda, i),
                wrap(arndf, i),
                wrap(arnda, i),
                wrap(num, i),
                wrap(fjit, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpread(self, x):
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        pyoArgsAssert(self, "O", x)
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFrndf(self, x):
        pyoArgsAssert(self, "O", x)
        self._frndf = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrndf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFrnda(self, x):
        pyoArgsAssert(self, "O", x)
        self._frnda = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrnda(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArndf(self, x):
        pyoArgsAssert(self, "O", x)
        self._arndf = x
        x, lmax = convertArgsToLists(x)
        [obj.setArndf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArnda(self, x):
        pyoArgsAssert(self, "O", x)
        self._arnda = x
        x, lmax = convertArgsToLists(x)
        [obj.setArnda(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFjit(self, x):
        pyoArgsAssert(self, "b", x)
        self._fjit = x
        x, lmax = convertArgsToLists(x)
        [obj.setFjit(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def spread(self):
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def slope(self):
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)

    @property
    def frndf(self):
        return self._frndf

    @frndf.setter
    def frndf(self, x):
        self.setFrndf(x)

    @property
    def frnda(self):
        return self._frnda

    @frnda.setter
    def frnda(self, x):
        self.setFrnda(x)

    @property
    def arndf(self):
        return self._arndf

    @arndf.setter
    def arndf(self, x):
        self.setArndf(x)

    @property
    def arnda(self):
        return self._arnda

    @arnda.setter
    def arnda(self, x):
        self.setArnda(x)

    @property
    def fjit(self):
        return self._fjit

    @fjit.setter
    def fjit(self, x):
        self.setFjit(x)


class TableRead(PyoObject):

    def __init__(self, table, freq=1, loop=0, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "tObiOO", table, freq, loop, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._freq = freq
        self._loop = loop
        self._interp = interp
        self._keeplast = 0
        table, freq, loop, interp, mul, add, lmax = convertArgsToLists(table, freq, loop, interp, mul, add)
        self._base_objs = [
            TableRead_base(wrap(table, i), wrap(freq, i), wrap(loop, i), wrap(interp, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setKeepLast(self, x):
        pyoArgsAssert(self, "b", x)
        self._keeplast = x
        x, lmax = convertArgsToLists(x)
        [obj.setKeepLast(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def loop(self):
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class Pulsar(PyoObject):

    def __init__(self, table, env, freq=100, frac=0.5, phase=0, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "ttOOOiOO", table, env, freq, frac, phase, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._env = env
        self._freq = freq
        self._frac = frac
        self._phase = phase
        self._interp = interp
        table, env, freq, frac, phase, interp, mul, add, lmax = convertArgsToLists(
            table, env, freq, frac, phase, interp, mul, add
        )
        self._base_objs = [
            Pulsar_base(
                wrap(table, i),
                wrap(env, i),
                wrap(freq, i),
                wrap(frac, i),
                wrap(phase, i),
                wrap(interp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFrac(self, x):
        pyoArgsAssert(self, "O", x)
        self._frac = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrac(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPhase(self, x):
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def frac(self):
        return self._frac

    @frac.setter
    def frac(self, x):
        self.setFrac(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class Pointer(PyoObject):

    def __init__(self, table, index, mul=1, add=0):
        pyoArgsAssert(self, "toOO", table, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._index = index
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [
            Pointer_base(wrap(table, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class Pointer2(PyoObject):

    def __init__(self, table, index, interp=4, autosmooth=True, mul=1, add=0):
        pyoArgsAssert(self, "toibOO", table, index, interp, autosmooth, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._index = index
        self._interp = interp
        self._autosmooth = autosmooth
        table, index, interp, autosmooth, mul, add, lmax = convertArgsToLists(
            table, index, interp, autosmooth, mul, add
        )
        self._base_objs = [
            Pointer2_base(
                wrap(table, i), wrap(index, i), wrap(interp, i), wrap(autosmooth, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setAutoSmooth(self, x):
        pyoArgsAssert(self, "b", x)
        self._autosmooth = x
        x, lmax = convertArgsToLists(x)
        [obj.setAutoSmooth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)

    @property
    def autosmooth(self):
        return self._autosmooth

    @autosmooth.setter
    def autosmooth(self, x):
        self.setAutoSmooth(x)


class TableIndex(PyoObject):

    def __init__(self, table, index, mul=1, add=0):
        pyoArgsAssert(self, "toOO", table, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._index = index
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [
            TableIndex_base(wrap(table, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class Lookup(PyoObject):

    def __init__(self, table, index, mul=1, add=0):
        pyoArgsAssert(self, "toOO", table, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._index = index
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [Lookup_base(wrap(table, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class TableRec(PyoObject):

    def __init__(self, input, table, fadetime=0):
        pyoArgsAssert(self, "otn", input, table, fadetime)
        PyoObject.__init__(self)
        self._time_dummy = []
        self._input = input
        self._table = table
        self._in_fader = InputFader(input).stop()
        in_fader, table, fadetime, lmax = convertArgsToLists(self._in_fader, table, fadetime)
        self._base_objs = [
            TableRec_base(wrap(in_fader, i), wrap(table, i), wrap(fadetime, i)) for i in range(len(table))
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._time_objs = [TableRecTimeStream_base(obj) for obj in self._base_objs]

    def __getitem__(self, i):
        if i == "time":
            self._time_dummy.append(Dummy([obj for obj in self._time_objs]))
            return self._time_dummy[-1]
        return PyoObject.__getitem__(self, i)

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

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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


class TableWrite(PyoObject):

    def __init__(self, input, pos, table, mode=0, maxwindow=1024):
        pyoArgsAssert(self, "ootii", input, pos, table, mode, maxwindow)
        PyoObject.__init__(self)
        self._input = input
        self._pos = pos
        self._table = table
        self._mode = mode
        self._maxwindow = maxwindow
        self._in_fader = InputFader(input)
        in_fader, pos, table, mode, maxwindow, lmax = convertArgsToLists(self._in_fader, pos, table, mode, maxwindow)
        self._base_objs = [
            TableWrite_base(wrap(in_fader, i), wrap(pos, i), wrap(table, i), wrap(mode, i), wrap(maxwindow, i))
            for i in range(len(table))
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

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        pyoArgsAssert(self, "o", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)


class TableMorph(PyoObject):

    def __init__(self, input, table, sources):
        pyoArgsAssert(self, "otl", input, table, sources)
        PyoObject.__init__(self)
        self._input = input
        self._table = table
        self._sources = sources
        self._in_fader = InputFader(input)
        in_fader, table, lmax = convertArgsToLists(self._in_fader, table)
        self._base_sources = [source[0] for source in sources]
        self._base_objs = [
            TableMorph_base(wrap(in_fader, i), wrap(table, i), self._base_sources) for i in range(len(table))
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

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def sources(self):
        return self._sources

    @sources.setter
    def sources(self, x):
        self.setSources(x)


class Granulator(PyoObject):

    def __init__(self, table, env, pitch=1, pos=0, dur=0.1, grains=8, basedur=0.1, mul=1, add=0):
        pyoArgsAssert(self, "ttOOOinOO", table, env, pitch, pos, dur, grains, basedur, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._env = env
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._grains = grains
        self._basedur = basedur
        table, env, pitch, pos, dur, grains, basedur, mul, add, lmax = convertArgsToLists(
            table, env, pitch, pos, dur, grains, basedur, mul, add
        )
        self._base_objs = [
            Granulator_base(
                wrap(table, i),
                wrap(env, i),
                wrap(pitch, i),
                wrap(pos, i),
                wrap(dur, i),
                wrap(grains, i),
                wrap(basedur, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setGrains(self, x):
        pyoArgsAssert(self, "i", x)
        self._grains = x
        x, lmax = convertArgsToLists(x)
        [obj.setGrains(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBaseDur(self, x):
        pyoArgsAssert(self, "n", x)
        self._basedur = x
        x, lmax = convertArgsToLists(x)
        [obj.setBaseDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def grains(self):
        return self._grains

    @grains.setter
    def grains(self, x):
        self.setGrains(x)

    @property
    def basedur(self):
        return self._basedur

    @basedur.setter
    def basedur(self, x):
        self.setBaseDur(x)


class TrigTableRec(PyoObject):

    def __init__(self, input, trig, table, fadetime=0):
        pyoArgsAssert(self, "ootn", input, trig, table, fadetime)
        PyoObject.__init__(self)
        self._time_dummy = []
        self._input = input
        self._trig = trig
        self._table = table
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(trig)
        in_fader, in_fader2, table, fadetime, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, table, fadetime
        )
        self._base_objs = [
            TrigTableRec_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(table, i), wrap(fadetime, i))
            for i in range(len(table))
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._time_objs = [TrigTableRecTimeStream_base(obj) for obj in self._base_objs]
        self._init_play()

    def __getitem__(self, i):
        if i == "time":
            self._time_dummy.append(Dummy([obj for obj in self._time_objs]))
            return self._time_dummy[-1]
        return PyoObject.__getitem__(self, i)

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

    def setTrig(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._trig = x
        self._in_fader2.setInput(x, fadetime)

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def trig(self):
        return self._trig

    @trig.setter
    def trig(self, x):
        self.setTrig(x)

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class Looper(PyoObject):

    def __init__(
        self,
        table,
        pitch=1,
        start=0,
        dur=1.0,
        xfade=20,
        mode=1,
        xfadeshape=0,
        startfromloop=False,
        interp=2,
        autosmooth=False,
        mul=1,
        add=0,
    ):
        pyoArgsAssert(
            self,
            "tOOOOiibibOO",
            table,
            pitch,
            start,
            dur,
            xfade,
            mode,
            xfadeshape,
            startfromloop,
            interp,
            autosmooth,
            mul,
            add,
        )
        PyoObject.__init__(self, mul, add)
        self._time_dummy = []
        self._appendfade = 0
        self._fadeinseconds = 0
        self._table = table
        self._pitch = pitch
        self._start = start
        self._dur = dur
        self._xfade = xfade
        self._mode = mode
        self._xfadeshape = xfadeshape
        self._startfromloop = startfromloop
        self._interp = interp
        self._autosmooth = autosmooth
        (
            table,
            pitch,
            start,
            dur,
            xfade,
            mode,
            xfadeshape,
            startfromloop,
            interp,
            autosmooth,
            mul,
            add,
            lmax,
        ) = convertArgsToLists(
            table, pitch, start, dur, xfade, mode, xfadeshape, startfromloop, interp, autosmooth, mul, add
        )
        self._base_objs = [
            Looper_base(
                wrap(table, i),
                wrap(pitch, i),
                wrap(start, i),
                wrap(dur, i),
                wrap(xfade, i),
                wrap(mode, i),
                wrap(xfadeshape, i),
                wrap(startfromloop, i),
                wrap(interp, i),
                wrap(autosmooth, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._time_objs = [LooperTimeStream_base(obj) for obj in self._base_objs]
        self._init_play()

    def __getitem__(self, i):
        if i == "time":
            self._time_dummy.append(Dummy([obj for obj in self._time_objs]))
            return self._time_dummy[-1]
        return PyoObject.__getitem__(self, i)

    def stop(self, wait=0):
        if self._time_objs is not None:
            [obj.stop(wait) for obj in self._time_objs]
        return PyoObject.stop(self, wait)

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStart(self, x):
        pyoArgsAssert(self, "O", x)
        self._start = x
        x, lmax = convertArgsToLists(x)
        [obj.setStart(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setXfade(self, x):
        pyoArgsAssert(self, "O", x)
        self._xfade = x
        x, lmax = convertArgsToLists(x)
        [obj.setXfade(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setXfadeShape(self, x):
        pyoArgsAssert(self, "i", x)
        self._xfadeshape = x
        x, lmax = convertArgsToLists(x)
        [obj.setXfadeShape(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStartFromLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._startfromloop = x
        x, lmax = convertArgsToLists(x)
        [obj.setStartFromLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setAutoSmooth(self, x):
        pyoArgsAssert(self, "b", x)
        self._autosmooth = x
        x, lmax = convertArgsToLists(x)
        [obj.setAutoSmooth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    def loopnow(self):
        [obj.loopnow() for obj in self._base_objs]

    def appendFadeTime(self, x):
        pyoArgsAssert(self, "b", x)
        self._appendfade = x
        x, lmax = convertArgsToLists(x)
        [obj.appendFadeTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def fadeInSeconds(self, x):
        pyoArgsAssert(self, "b", x)
        self._fadeinseconds = x
        x, lmax = convertArgsToLists(x)
        [obj.fadeInSeconds(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def start(self):
        return self._start

    @start.setter
    def start(self, x):
        self.setStart(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def xfade(self):
        return self._xfade

    @xfade.setter
    def xfade(self, x):
        self.setXfade(x)

    @property
    def xfadeshape(self):
        return self._xfadeshape

    @xfadeshape.setter
    def xfadeshape(self, x):
        self.setXfadeShape(x)

    @property
    def startfromloop(self):
        return self._startfromloop

    @startfromloop.setter
    def startfromloop(self, x):
        self.setStartFromLoop(x)

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)

    @property
    def interp(self):
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)

    @property
    def autosmooth(self):
        return self._autosmooth

    @autosmooth.setter
    def autosmooth(self, x):
        self.setAutoSmooth(x)


class TablePut(PyoObject):

    def __init__(self, input, table):
        pyoArgsAssert(self, "ot", input, table)
        PyoObject.__init__(self)
        self._input = input
        self._table = table
        self._in_fader = InputFader(input).stop()
        in_fader, table, lmax = convertArgsToLists(self._in_fader, table)
        self._base_objs = [TablePut_base(wrap(in_fader, i), wrap(table, i)) for i in range(len(table))]
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

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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


class TableFill(PyoObject):

    def __init__(self, input, table):
        pyoArgsAssert(self, "ot", input, table)
        PyoObject.__init__(self)
        self._input = input
        self._table = table
        self._in_fader = InputFader(input)
        in_fader, table, lmax = convertArgsToLists(self._in_fader, table)
        self._base_objs = [TableFill_base(wrap(in_fader, i), wrap(table, i)) for i in range(len(table))]
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

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def getCurrentPos(self, all=False):
        pyoArgsAssert(self, "B", all)
        if not all:
            return self._base_objs[0].getCurrentPos()
        else:
            return [obj.getCurrentPos() for obj in self._base_objs]

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


class Granule(PyoObject):

    def __init__(self, table, env, dens=50, pitch=1, pos=0, dur=0.1, mul=1, add=0):
        pyoArgsAssert(self, "ttOOOOOO", table, env, dens, pitch, pos, dur, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._env = env
        self._dens = dens
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._sync = 1
        table, env, dens, pitch, pos, dur, mul, add, lmax = convertArgsToLists(
            table, env, dens, pitch, pos, dur, mul, add
        )
        self._base_objs = [
            Granule_base(
                wrap(table, i),
                wrap(env, i),
                wrap(dens, i),
                wrap(pitch, i),
                wrap(pos, i),
                wrap(dur, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDens(self, x):
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSync(self, x):
        pyoArgsAssert(self, "b", x)
        self._sync = x
        x, lmax = convertArgsToLists(x)
        [obj.setSync(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)


class TableScale(PyoObject):

    def __init__(self, table, outtable, mul=1, add=0):
        pyoArgsAssert(self, "ttOO", table, outtable, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._outtable = outtable
        table, outtable, mul, add, lmax = convertArgsToLists(table, outtable, mul, add)
        self._base_objs = [
            TableScale_base(wrap(table, i), wrap(outtable, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOuttable(self, x):
        pyoArgsAssert(self, "t", x)
        self._outtable = x
        x, lmax = convertArgsToLists(x)
        [obj.setOuttable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def outtable(self):
        return self._outtable

    @outtable.setter
    def outtable(self, x):
        self.setOuttable(x)


class Particle(PyoObject):

    def __init__(self, table, env, dens=50, pitch=1, pos=0, dur=0.1, dev=0.01, pan=0.5, chnls=1, mul=1, add=0):
        pyoArgsAssert(self, "ttOOOOOOIOO", table, env, dens, pitch, pos, dur, dev, pan, chnls, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._env = env
        self._dens = dens
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._dev = dev
        self._pan = pan
        self._chnls = chnls
        table, env, dens, pitch, pos, dur, dev, pan, mul, add, lmax = convertArgsToLists(
            table, env, dens, pitch, pos, dur, dev, pan, mul, add
        )
        self._base_players = [
            MainParticle_base(
                wrap(table, i),
                wrap(env, i),
                wrap(dens, i),
                wrap(pitch, i),
                wrap(pos, i),
                wrap(dur, i),
                wrap(dev, i),
                wrap(pan, i),
                chnls,
            )
            for i in range(lmax)
        ]
        self._base_objs = []
        for i in range(lmax):
            for j in range(chnls):
                self._base_objs.append(Particle_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setEnv(self, x):
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDens(self, x):
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPos(self, x):
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDev(self, x):
        pyoArgsAssert(self, "O", x)
        self._dev = x
        x, lmax = convertArgsToLists(x)
        [obj.setDev(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPan(self, x):
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def dev(self):
        return self._dev

    @dev.setter
    def dev(self, x):
        self.setDev(x)

    @property
    def pan(self):
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)


class Particle2(PyoObject):

    def __init__(
        self,
        table,
        env,
        dens=50,
        pitch=1,
        pos=0,
        dur=0.1,
        dev=0.01,
        pan=0.5,
        filterfreq=18000,
        filterq=0.7,
        filtertype=0,
        chnls=1,
        mul=1,
        add=0,
    ):
        pyoArgsAssert(
            self,
            "ttOOOOOOOOOIOO",
            table,
            env,
            dens,
            pitch,
            pos,
            dur,
            dev,
            pan,
            filterfreq,
            filterq,
            filtertype,
            chnls,
            mul,
            add,
        )
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._env = env
        self._dens = dens
        self._pitch = pitch
        self._pos = pos
        self._dur = dur
        self._dev = dev
        self._pan = pan
        self._filterfreq = filterfreq
        self._filterq = filterq
        self._filtertype = filtertype
        self._chnls = chnls
        (
            table,
            env,
            dens,
            pitch,
            pos,
            dur,
            dev,
            pan,
            filterfreq,
            filterq,
            filtertype,
            mul,
            add,
            lmax,
        ) = convertArgsToLists(table, env, dens, pitch, pos, dur, dev, pan, filterfreq, filterq, filtertype, mul, add)
        self._base_players = [
            MainParticle2_base(
                wrap(table, i),
                wrap(env, i),
                wrap(dens, i),
                wrap(pitch, i),
                wrap(pos, i),
                wrap(dur, i),
                wrap(dev, i),
                wrap(pan, i),
                wrap(filterfreq, i),
                wrap(filterq, i),
                wrap(filtertype, i),
                chnls,
            )
            for i in range(lmax)
        ]
        self._base_objs = []
        for i in range(lmax):
            for j in range(chnls):
                self._base_objs.append(Particle2_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setEnv(self, x):
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDens(self, x):
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPitch(self, x):
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPos(self, x):
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDev(self, x):
        pyoArgsAssert(self, "O", x)
        self._dev = x
        x, lmax = convertArgsToLists(x)
        [obj.setDev(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPan(self, x):
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFilterfreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._filterfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFilterfreq(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFilterq(self, x):
        pyoArgsAssert(self, "O", x)
        self._filterq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFilterq(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFiltertype(self, x):
        pyoArgsAssert(self, "O", x)
        self._filtertype = x
        x, lmax = convertArgsToLists(x)
        [obj.setFiltertype(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def dev(self):
        return self._dev

    @dev.setter
    def dev(self, x):
        self.setDev(x)

    @property
    def pan(self):
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)

    @property
    def filterfreq(self):
        return self._filterfreq

    @filterfreq.setter
    def filterfreq(self, x):
        self.setFilterfreq(x)

    @property
    def filterq(self):
        return self._filterq

    @filterq.setter
    def filterq(self, x):
        self.setFilterq(x)

    @property
    def filtertype(self):
        return self._filtertype

    @filtertype.setter
    def filtertype(self, x):
        self.setFiltertype(x)


class TableScan(PyoObject):

    def __init__(self, table, mul=1, add=0):
        pyoArgsAssert(self, "tOO", table, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        table, mul, add, lmax = convertArgsToLists(table, mul, add)
        self._base_objs = [TableScan_base(wrap(table, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)
