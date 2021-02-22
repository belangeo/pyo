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


class Biquad(PyoObject):

    def __init__(self, input, freq=1000, q=1, type=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOiOO", input, freq, q, type, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, mul, add)
        self._base_objs = [
            Biquad_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(type, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class Biquadx(PyoObject):

    def __init__(self, input, freq=1000, q=1, type=0, stages=4, mul=1, add=0):
        pyoArgsAssert(self, "oOOiiOO", input, freq, q, type, stages, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._stages = stages
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, stages, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, q, type, stages, mul, add
        )
        self._base_objs = [
            Biquadx_base(
                wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(type, i), wrap(stages, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)

    @property
    def stages(self):
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class Biquada(PyoObject):

    def __init__(
        self, input, b0=0.005066, b1=0.010132, b2=0.005066, a0=1.070997, a1=-1.979735, a2=0.929003, mul=1, add=0
    ):
        pyoArgsAssert(self, "oOOOOOOOO", input, b0, b1, b2, a0, a1, a2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._b0 = Sig(b0)
        self._b1 = Sig(b1)
        self._b2 = Sig(b2)
        self._a0 = Sig(a0)
        self._a1 = Sig(a1)
        self._a2 = Sig(a2)
        self._in_fader = InputFader(input)
        in_fader, b0, b1, b2, a0, a1, a2, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._b0, self._b1, self._b2, self._a0, self._a1, self._a2, mul, add
        )
        self._base_objs = [
            Biquada_base(
                wrap(in_fader, i),
                wrap(b0, i),
                wrap(b1, i),
                wrap(b2, i),
                wrap(a0, i),
                wrap(a1, i),
                wrap(a2, i),
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

    def setB0(self, x):
        pyoArgsAssert(self, "O", x)
        self._b0.value = x

    def setB1(self, x):
        pyoArgsAssert(self, "O", x)
        self._b1.value = x

    def setB2(self, x):
        pyoArgsAssert(self, "O", x)
        self._b2.value = x

    def setA0(self, x):
        pyoArgsAssert(self, "O", x)
        self._a0.value = x

    def setA1(self, x):
        pyoArgsAssert(self, "O", x)
        self._a1.value = x

    def setA2(self, x):
        pyoArgsAssert(self, "O", x)
        self._a2.value = x

    def setCoeffs(self, *args, **kwds):
        for i, val in enumerate(args):
            attr = getattr(self, ["_b0", "_b1", "_b2", "_a0", "_a1", "_a2"][i])
            attr.value = val
        for key in kwds.keys():
            if hasattr(self, key):
                attr = getattr(self, "_" + key)
                attr.value = kwds[key]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def b0(self):
        return self._b0

    @b0.setter
    def b0(self, x):
        self.setB0(x)

    @property
    def b1(self):
        return self._b1

    @b1.setter
    def b1(self, x):
        self.setB1(x)

    @property
    def b2(self):
        return self._b2

    @b2.setter
    def b2(self, x):
        self.setB2(x)

    @property
    def a0(self):
        return self._a0

    @a0.setter
    def a0(self, x):
        self.setA0(x)

    @property
    def a1(self):
        return self._a1

    @a1.setter
    def a1(self, x):
        self.setA1(x)

    @property
    def a2(self):
        return self._a2

    @a2.setter
    def a2(self, x):
        self.setA2(x)


class EQ(PyoObject):

    def __init__(self, input, freq=1000, q=1, boost=-3.0, type=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOiOO", input, freq, q, boost, type, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._boost = boost
        self._type = type
        self._in_fader = InputFader(input)
        in_fader, freq, q, boost, type, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, q, boost, type, mul, add
        )
        self._base_objs = [
            EQ_base(
                wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(boost, i), wrap(type, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBoost(self, x):
        pyoArgsAssert(self, "O", x)
        self._boost = x
        x, lmax = convertArgsToLists(x)
        [obj.setBoost(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def boost(self):
        return self._boost

    @boost.setter
    def boost(self, x):
        self.setBoost(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class Tone(PyoObject):

    def __init__(self, input, freq=1000, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [Tone_base(wrap(in_fader, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Atone(PyoObject):

    def __init__(self, input, freq=1000, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [
            Atone_base(wrap(in_fader, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Port(PyoObject):

    def __init__(self, input, risetime=0.05, falltime=0.05, init=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOnOO", input, risetime, falltime, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, init, mul, add, lmax = convertArgsToLists(
            self._in_fader, risetime, falltime, init, mul, add
        )
        self._base_objs = [
            Port_base(
                wrap(in_fader, i), wrap(risetime, i), wrap(falltime, i), wrap(init, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setRiseTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def risetime(self):
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)


class DCBlock(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DCBlock_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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


class BandSplit(PyoObject):

    def __init__(self, input, num=6, min=20, max=20000, q=1, mul=1, add=0):
        pyoArgsAssert(self, "oINNOOO", input, num, min, max, q, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._num = num
        self._min = min
        self._max = max
        self._q = q
        self._in_fader = InputFader(input)
        in_fader, q, lmax = convertArgsToLists(self._in_fader, q)
        self._op_duplicate = lmax
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [BandSplitter_base(wrap(in_fader, i), num, min, max, wrap(q, i)) for i in range(lmax)]
        self._base_objs = []
        for j in range(num):
            for i in range(lmax):
                self._base_objs.append(BandSplit_base(wrap(self._base_players, i), j, wrap(mul, j), wrap(add, j)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class FourBand(PyoObject):

    def __init__(self, input, freq1=150, freq2=500, freq3=2000, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOO", input, freq1, freq2, freq3, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq1 = freq1
        self._freq2 = freq2
        self._freq3 = freq3
        self._in_fader = InputFader(input)
        in_fader, freq1, freq2, freq3, lmax = convertArgsToLists(self._in_fader, freq1, freq2, freq3)
        self._op_duplicate = lmax
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [
            FourBandMain_base(wrap(in_fader, i), wrap(freq1, i), wrap(freq2, i), wrap(freq3, i)) for i in range(lmax)
        ]
        self._base_objs = []
        for j in range(4):
            for i in range(lmax):
                self._base_objs.append(FourBand_base(wrap(self._base_players, i), j, wrap(mul, j), wrap(add, j)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq1(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq1(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFreq2(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq2(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFreq3(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq3 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq3(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq1(self):
        return self._freq1

    @freq1.setter
    def freq1(self, x):
        self.setFreq1(x)

    @property
    def freq2(self):
        return self._freq2

    @freq2.setter
    def freq2(self, x):
        self.setFreq2(x)

    @property
    def freq3(self):
        return self._freq3

    @freq3.setter
    def freq3(self, x):
        self.setFreq3(x)


class MultiBand(PyoObject):

    def __init__(self, input, num=8, mul=1, add=0):
        pyoArgsAssert(self, "oIOO", input, num, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._num = num
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        self._op_duplicate = lmax
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [MultiBandMain_base(wrap(in_fader, i), num) for i in range(lmax)]
        self._base_objs = []
        for j in range(num):
            for i in range(lmax):
                self._base_objs.append(MultiBand_base(wrap(self._base_players, i), j, wrap(mul, j), wrap(add, j)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFrequencies(self, freqs):
        pyoArgsAssert(self, "l", freqs)
        if len(freqs) != (self._num - 1):
            print(
                "pyo warning: MultiBand frequency list length must be egal to the number of bands, minus 1, of the object."
            )
            return
        [obj.setFrequencies(freqs) for obj in self._base_players]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Hilbert(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._real_dummy = []
        self._imag_dummy = []
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        mul, add, lmax2 = convertArgsToLists(mul, add)
        self._base_players = [HilbertMain_base(wrap(in_fader, i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax2):
            for j in range(lmax):
                self._base_objs.append(Hilbert_base(wrap(self._base_players, j), 0, wrap(mul, i), wrap(add, i)))
                self._base_objs.append(Hilbert_base(wrap(self._base_players, j), 1, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def __getitem__(self, str):
        if str == "real":
            self._real_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if (i % 2) == 0]))
            return self._real_dummy[-1]
        if str == "imag":
            self._imag_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if (i % 2) == 1]))
            return self._imag_dummy[-1]

    def get(self, identifier="real", all=False):
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

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


class Allpass(PyoObject):

    def __init__(self, input, delay=0.01, feedback=0, maxdelay=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOnOO", input, delay, feedback, maxdelay, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._delay = delay
        self._feedback = feedback
        self._maxdelay = maxdelay
        self._in_fader = InputFader(input)
        in_fader, delay, feedback, maxdelay, mul, add, lmax = convertArgsToLists(
            self._in_fader, delay, feedback, maxdelay, mul, add
        )
        self._base_objs = [
            Allpass_base(
                wrap(in_fader, i), wrap(delay, i), wrap(feedback, i), wrap(maxdelay, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDelay(self, x):
        pyoArgsAssert(self, "O", x)
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def delay(self):
        return self._delay

    @delay.setter
    def delay(self, x):
        self.setDelay(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Allpass2(PyoObject):

    def __init__(self, input, freq=1000, bw=100, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, bw, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._bw = bw
        self._in_fader = InputFader(input)
        in_fader, freq, bw, mul, add, lmax = convertArgsToLists(self._in_fader, freq, bw, mul, add)
        self._base_objs = [
            Allpass2_base(wrap(in_fader, i), wrap(freq, i), wrap(bw, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBw(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)


class Phaser(PyoObject):

    def __init__(self, input, freq=1000, spread=1.1, q=10, feedback=0, num=8, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOiOO", input, freq, spread, q, feedback, num, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._spread = spread
        self._q = q
        self._feedback = feedback
        self._num = num
        self._in_fader = InputFader(input)
        in_fader, freq, spread, q, feedback, num, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, spread, q, feedback, num, mul, add
        )
        self._base_objs = [
            Phaser_base(
                wrap(in_fader, i),
                wrap(freq, i),
                wrap(spread, i),
                wrap(q, i),
                wrap(feedback, i),
                wrap(num, i),
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

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

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
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Vocoder(PyoObject):

    def __init__(self, input, input2, freq=60, spread=1.25, q=20, slope=0.5, stages=24, mul=1, add=0):
        pyoArgsAssert(self, "ooOOOOiOO", input, input2, freq, spread, q, slope, stages, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._freq = freq
        self._spread = spread
        self._q = q
        self._slope = slope
        self._stages = stages
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, freq, spread, q, slope, stages, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, freq, spread, q, slope, stages, mul, add
        )
        self._base_objs = [
            Vocoder_base(
                wrap(in_fader, i),
                wrap(in_fader2, i),
                wrap(freq, i),
                wrap(spread, i),
                wrap(q, i),
                wrap(slope, i),
                wrap(stages, i),
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

    def setInput2(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

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

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        pyoArgsAssert(self, "O", x)
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def slope(self):
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)

    @property
    def stages(self):
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class IRWinSinc(PyoObject):

    def __init__(self, input, freq=1000, bw=500, type=0, order=256, mul=1, add=0):
        pyoArgsAssert(self, "oOOiiOO", input, freq, bw, type, order, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._bw = bw
        self._type = type
        if (order % 2) != 0:
            order += 1
            print("order argument of IRWinSinc must be even, set to %i" % order)
        self._order = order
        self._in_fader = InputFader(input)
        in_fader, freq, bw, type, order, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, bw, type, order, mul, add
        )
        self._base_objs = [
            IRWinSinc_base(
                wrap(in_fader, i), wrap(freq, i), wrap(bw, i), wrap(type, i), wrap(order, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBandwidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class IRAverage(PyoObject):

    def __init__(self, input, order=256, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, order, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        if (order % 2) != 0:
            order += 1
            print("order argument of IRAverage must be even, set to %i" % order)
        self._order = order
        self._in_fader = InputFader(input)
        in_fader, order, mul, add, lmax = convertArgsToLists(self._in_fader, order, mul, add)
        self._base_objs = [
            IRAverage_base(wrap(in_fader, i), wrap(order, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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


class IRPulse(PyoObject):

    def __init__(self, input, freq=500, bw=2500, type=0, order=256, mul=1, add=0):
        pyoArgsAssert(self, "oOOiiOO", input, freq, bw, type, order, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._bw = bw
        self._type = type
        if (order % 2) != 0:
            order += 1
            print("order argument of IRPulse must be even, set to %i" % order)
        self._order = order
        self._in_fader = InputFader(input)
        in_fader, freq, bw, type, order, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, bw, type, order, mul, add
        )
        self._base_objs = [
            IRPulse_base(
                wrap(in_fader, i), wrap(freq, i), wrap(bw, i), wrap(type, i), wrap(order, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBandwidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class IRFM(PyoObject):

    def __init__(self, input, carrier=1000, ratio=0.5, index=3, order=256, mul=1, add=0):
        pyoArgsAssert(self, "oOOOiOO", input, carrier, ratio, index, order, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._carrier = carrier
        self._ratio = ratio
        self._index = index
        if (order % 2) != 0:
            order += 1
            print("order argument of IRFM must be even, set to %i" % order)
        self._order = order
        self._in_fader = InputFader(input)
        in_fader, carrier, ratio, index, order, mul, add, lmax = convertArgsToLists(
            self._in_fader, carrier, ratio, index, order, mul, add
        )
        self._base_objs = [
            IRFM_base(
                wrap(in_fader, i),
                wrap(carrier, i),
                wrap(ratio, i),
                wrap(index, i),
                wrap(order, i),
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

    def setCarrier(self, x):
        pyoArgsAssert(self, "O", x)
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def carrier(self):
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class SVF(PyoObject):

    def __init__(self, input, freq=1000, q=1, type=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOO", input, freq, q, type, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._type = type
        self._in_fader = InputFader(input)
        in_fader, freq, q, type, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, type, mul, add)
        self._base_objs = [
            SVF_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(type, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "O", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class SVF2(PyoObject):

    def __init__(self, input, freq=1000, q=1, shelf=-3, type=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOOO", input, freq, q, shelf, type, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._shelf = shelf
        self._type = type
        self._order = list(range(10))
        self._in_fader = InputFader(input)
        in_fader, freq, q, shelf, type, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, q, shelf, type, mul, add
        )
        self._base_objs = [
            SVF2_base(
                wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(shelf, i), wrap(type, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setShelf(self, x):
        pyoArgsAssert(self, "O", x)
        self._shelf = x
        x, lmax = convertArgsToLists(x)
        [obj.setShelf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "O", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOrder(self, x):
        pyoArgsAssert(self, "l", x)
        self._order = x
        [obj.setOrder(x) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def shelf(self):
        return self._shelf

    @shelf.setter
    def shelf(self, x):
        self.setShelf(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)

    @property
    def order(self):
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class Average(PyoObject):

    def __init__(self, input, size=10, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, size, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._size = size
        self._in_fader = InputFader(input)
        in_fader, size, mul, add, lmax = convertArgsToLists(self._in_fader, size, mul, add)
        self._base_objs = [
            Average_base(wrap(in_fader, i), wrap(size, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
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


class Reson(PyoObject):

    def __init__(self, input, freq=1000, q=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, q, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._in_fader = InputFader(input)
        in_fader, freq, q, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, mul, add)
        self._base_objs = [
            Reson_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class Resonx(PyoObject):

    def __init__(self, input, freq=1000, q=1, stages=4, mul=1, add=0):
        pyoArgsAssert(self, "oOOiOO", input, freq, q, stages, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._stages = stages
        self._in_fader = InputFader(input)
        in_fader, freq, q, stages, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, stages, mul, add)
        self._base_objs = [
            Resonx_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(stages, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def stages(self):
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class ButLP(PyoObject):

    def __init__(self, input, freq=1000, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [
            ButLP_base(wrap(in_fader, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class ButHP(PyoObject):

    def __init__(self, input, freq=1000, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [
            ButHP_base(wrap(in_fader, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class ButBP(PyoObject):

    def __init__(self, input, freq=1000, q=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, q, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._in_fader = InputFader(input)
        in_fader, freq, q, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, mul, add)
        self._base_objs = [
            ButBP_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class ButBR(PyoObject):

    def __init__(self, input, freq=1000, q=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, q, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._q = q
        self._in_fader = InputFader(input)
        in_fader, freq, q, mul, add, lmax = convertArgsToLists(self._in_fader, freq, q, mul, add)
        self._base_objs = [
            ButBR_base(wrap(in_fader, i), wrap(freq, i), wrap(q, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class MoogLP(PyoObject):

    def __init__(self, input, freq=1000, res=0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, res, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._res = res
        self._in_fader = InputFader(input)
        in_fader, freq, res, mul, add, lmax = convertArgsToLists(self._in_fader, freq, res, mul, add)
        self._base_objs = [
            MoogLP_base(wrap(in_fader, i), wrap(freq, i), wrap(res, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRes(self, x):
        pyoArgsAssert(self, "O", x)
        self._res = x
        x, lmax = convertArgsToLists(x)
        [obj.setRes(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def res(self):
        return self._res

    @res.setter
    def res(self, x):
        self.setRes(x)


class ComplexRes(PyoObject):

    def __init__(self, input, freq=1000, decay=0.25, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, freq, decay, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._decay = decay
        self._in_fader = InputFader(input)
        in_fader, freq, decay, mul, add, lmax = convertArgsToLists(self._in_fader, freq, decay, mul, add)
        self._base_objs = [
            ComplexRes_base(wrap(in_fader, i), wrap(freq, i), wrap(decay, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        pyoArgsAssert(self, "O", x)
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def decay(self):
        return self._decay

    @decay.setter
    def decay(self, x):
        self.setDecay(x)
