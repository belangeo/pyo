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


class Randi(PyoObject):

    def __init__(self, min=0.0, max=1.0, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", min, max, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._min = min
        self._max = max
        self._freq = freq
        min, max, freq, mul, add, lmax = convertArgsToLists(min, max, freq, mul, add)
        self._base_objs = [
            Randi_base(wrap(min, i), wrap(max, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Randh(PyoObject):

    def __init__(self, min=0.0, max=1.0, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", min, max, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._min = min
        self._max = max
        self._freq = freq
        min, max, freq, mul, add, lmax = convertArgsToLists(min, max, freq, mul, add)
        self._base_objs = [
            Randh_base(wrap(min, i), wrap(max, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Choice(PyoObject):

    def __init__(self, choice, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "lOOO", choice, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._choice = choice
        self._freq = freq
        freq, mul, add, lmax = convertArgsToLists(freq, mul, add)
        if type(choice[0]) != list:
            self._base_objs = [Choice_base(choice, wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        else:
            choicelen = len(choice)
            lmax = max(choicelen, lmax)
            self._base_objs = [
                Choice_base(wrap(choice, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
            ]
        self._init_play()

    def setChoice(self, x):
        pyoArgsAssert(self, "l", x)
        self._choice = x
        if type(x[0]) != list:
            [obj.setChoice(self._choice) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setChoice(wrap(self._choice, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def choice(self):
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class RandInt(PyoObject):

    def __init__(self, max=100, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", max, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._max = max
        self._freq = freq
        max, freq, mul, add, lmax = convertArgsToLists(max, freq, mul, add)
        self._base_objs = [RandInt_base(wrap(max, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class RandDur(PyoObject):

    def __init__(self, min=0.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._min = min
        self._max = max
        min, max, mul, add, lmax = convertArgsToLists(min, max, mul, add)
        self._base_objs = [RandDur_base(wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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


class Xnoise(PyoObject):

    def __init__(self, dist=0, freq=1.0, x1=0.5, x2=0.5, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", freq, x1, x2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._dist = dist
        self._freq = freq
        self._x1 = x1
        self._x2 = x2
        dist, freq, x1, x2, mul, add, lmax = convertArgsToLists(dist, freq, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) in [bytes_t, unicode_t]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            Xnoise_base(wrap(dist, i), wrap(freq, i), wrap(x1, i), wrap(x2, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

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

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

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


class XnoiseMidi(PyoObject):

    def __init__(self, dist=0, freq=1.0, x1=0.5, x2=0.5, scale=0, mrange=(0, 127), mul=1, add=0):
        pyoArgsAssert(self, "OOOixOO", freq, x1, x2, scale, mrange, mul, add)
        PyoObject.__init__(self, mul, add)
        self._dist = dist
        self._freq = freq
        self._x1 = x1
        self._x2 = x2
        self._scale = scale
        self._mrange = mrange
        dist, freq, x1, x2, scale, mrange, mul, add, lmax = convertArgsToLists(
            dist, freq, x1, x2, scale, mrange, mul, add
        )
        for i, t in enumerate(dist):
            if type(t) in [bytes_t, unicode_t]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            XnoiseMidi_base(
                wrap(dist, i),
                wrap(freq, i),
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

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

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


class XnoiseDur(PyoObject):

    def __init__(self, dist=0, min=0.0, max=1.0, x1=0.5, x2=0.5, mul=1, add=0):
        pyoArgsAssert(self, "OOOOOO", min, max, x1, x2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._dist = dist
        self._min = min
        self._max = max
        self._x1 = x1
        self._x2 = x2
        dist, min, max, x1, x2, mul, add, lmax = convertArgsToLists(dist, min, max, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) in [bytes_t, unicode_t]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            XnoiseDur_base(
                wrap(dist, i), wrap(min, i), wrap(max, i), wrap(x1, i), wrap(x2, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setDist(self, x):
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes_t, unicode_t]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def dist(self):
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

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


class Urn(PyoObject):

    def __init__(self, max=100, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "iOOO", max, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._max = max
        self._freq = freq
        max, freq, mul, add, lmax = convertArgsToLists(max, freq, mul, add)
        self._base_objs = [Urn_base(wrap(max, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setMax(self, x):
        pyoArgsAssert(self, "i", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class LogiMap(PyoObject):

    def __init__(self, chaos=0.6, freq=1.0, init=0.5, mul=1, add=0):
        pyoArgsAssert(self, "OOnOO", chaos, freq, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._chaos = chaos
        self._freq = freq
        chaos, freq, init, mul, add, lmax = convertArgsToLists(chaos, freq, init, mul, add)
        self._base_objs = [
            LogiMap_base(wrap(chaos, i), wrap(freq, i), wrap(init, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setChaos(self, x):
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def chaos(self):
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)
