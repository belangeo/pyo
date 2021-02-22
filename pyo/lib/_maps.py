# encoding: utf-8

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

from math import pow, log10

######################################################################
### Map -> rescale values from sliders
######################################################################
class Map(object):

    def __init__(self, min, max, scale):
        self._min, self._max, self._scale = float(min), float(max), scale

    def get(self, x):
        if x < 0:
            x = 0.0
        elif x > 1:
            x = 1.0

        if self._scale == "log":
            return pow(10, x * log10(self._max / self._min) + log10(self._min))
        else:
            return (self._max - self._min) * x + self._min

    def set(self, x):
        if self._scale == "log":
            return log10(x / self._min) / log10(self._max / self._min)
        else:
            return (x - self._min) / (self._max - self._min)

    def setMin(self, x):
        self._min = x

    def setMax(self, x):
        self._max = x

    def setScale(self, x):
        self._scale = x

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
    def scale(self):
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class SLMap(Map):

    def __init__(self, min, max, scale, name, init, res="float", ramp=0.025, dataOnly=False):
        Map.__init__(self, min, max, scale)
        self._name = name
        self._init = init
        self._res = res
        self._ramp = ramp
        self._dataOnly = dataOnly

    @property
    def name(self):
        return self._name

    @property
    def init(self):
        return self._init

    @property
    def res(self):
        return self._res

    @property
    def ramp(self):
        return self._ramp

    @property
    def dataOnly(self):
        return self._dataOnly


class SLMapFreq(SLMap):

    def __init__(self, init=1000):
        SLMap.__init__(self, 20.0, 20000.0, "log", "freq", init, "float", 0.025)


class SLMapMul(SLMap):

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.0, 2.0, "lin", "mul", init, "float", 0.025)


class SLMapPhase(SLMap):

    def __init__(self, init=0.0):
        SLMap.__init__(self, 0.0, 1.0, "lin", "phase", init, "float", 0.025)


class SLMapPan(SLMap):

    def __init__(self, init=0.0):
        SLMap.__init__(self, 0.0, 1.0, "lin", "pan", init, "float", 0.025)


class SLMapQ(SLMap):

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.1, 100.0, "log", "q", init, "float", 0.025)


class SLMapDur(SLMap):

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.0, 60.0, "lin", "dur", init, "float", 0.025)
