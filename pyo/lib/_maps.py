# encoding: utf-8
"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from math import pow, log10

######################################################################
### Map -> rescale values from sliders
######################################################################
class Map(object):
    """
    Converts value between 0 and 1 on various scales.

    Base class for Map objects.

    :Args:

        min: int or float
            Lowest value of the range.
        max: int or float
            Highest value of the range.
        scale: string {'lin', 'log'}
            Method used to scale the input value on the specified range.

    >>> m = Map(20., 20000., 'log')
    >>> print(m.get(.5))
    632.455532034
    >>> print(m.set(12000))
    0.926050416795

    """

    def __init__(self, min, max, scale):
        self._min, self._max, self._scale = float(min), float(max), scale

    def get(self, x):
        """
        Takes `x` between 0 and 1 and returns scaled value.

        """
        if x < 0:
            x = 0.0
        elif x > 1:
            x = 1.0

        if self._scale == "log":
            return pow(10, x * log10(self._max / self._min) + log10(self._min))
        else:
            return (self._max - self._min) * x + self._min

    def set(self, x):
        """
        Takes `x` in the real range and returns value unscaled
        (between 0 and 1).

        """

        if self._scale == "log":
            return log10(x / self._min) / log10(self._max / self._min)
        else:
            return (x - self._min) / (self._max - self._min)

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float
                New `min` attribute.

        """
        self._min = x

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float
                New `max` attribute.

        """
        self._max = x

    def setScale(self, x):
        """
        Replace the `scale` attribute.

        :Args:

            x: string
                New `scale` attribute.

        """
        self._scale = x

    @property
    def min(self):
        """int or float. Lowest value of the range."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """int or float. Highest value of the range."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def scale(self):
        """string. Method used to scale the input value."""
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class SLMap(Map):
    """
    Base Map class used to manage control sliders.

    Derived from Map class, a few parameters are added for sliders
    initialization.

    :Parent: :py:class:`Map`

    :Args:

        min: int or float
            Smallest value of the range.
        max: int or float
            Highest value of the range.
        scale: string {'lin', 'log'}
            Method used to scale the input value on the specified range.
        name: string
            Name of the attributes the slider is affected to.
        init: int or float
            Initial value. Specified in the real range, not between 0 and 1.
            Use `set` method to retreive the normalized corresponding value.
        res: string {'int', 'float'}, optional
            Sets the resolution of the slider. Defaults to 'float'.
        ramp: float, optional
            Ramp time, in seconds, used to smooth the signal sent from slider
            to object's attribute. Defaults to 0.025.
        dataOnly: boolean, optional
            Set this argument to True if the parameter does not accept audio
            signal as control but discrete values. If True, label will be
            marked with a star symbol (*). Defaults to False.

    >>> s = Server().boot()
    >>> s.start()
    >>> ifs = [350,360,375,388]
    >>> slmapfreq = SLMap(20., 2000., 'log', 'freq', ifs)
    >>> slmapfeed = SLMap(0, 0.25, 'lin', 'feedback', 0)
    >>> maps = [slmapfreq, slmapfeed, SLMapMul(.1)]
    >>> a = SineLoop(freq=ifs, mul=.1).out()
    >>> a.ctrl(maps)

    """

    def __init__(self, min, max, scale, name, init, res="float", ramp=0.025, dataOnly=False):
        Map.__init__(self, min, max, scale)
        self._name = name
        self._init = init
        self._res = res
        self._ramp = ramp
        self._dataOnly = dataOnly

    @property
    def name(self):
        """string. Name of the parameter to control."""
        return self._name

    @property
    def init(self):
        """float. Initial value of the slider."""
        return self._init

    @property
    def res(self):
        """string. Slider resolution {int or float}."""
        return self._res

    @property
    def ramp(self):
        """float. Ramp time in seconds."""
        return self._ramp

    @property
    def dataOnly(self):
        """boolean. True if argument does not accept audio stream."""
        return self._dataOnly


class SLMapFreq(SLMap):
    """
    SLMap with normalized values for a 'freq' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 1000.

    .. note::

        SLMapFreq values are:

        - min = 20.0
        - max = 20000.0
        - scale = 'log'
        - name = 'freq'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=1000):
        SLMap.__init__(self, 20.0, 20000.0, "log", "freq", init, "float", 0.025)


class SLMapMul(SLMap):
    """
    SLMap with normalized values for a 'mul' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 1.

    .. note::

        SLMapMul values are:

        - min = 0.0
        - max = 2.0
        - scale = 'lin'
        - name = 'mul'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.0, 2.0, "lin", "mul", init, "float", 0.025)


class SLMapPhase(SLMap):
    """
    SLMap with normalized values for a 'phase' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 0.

    .. note::

        SLMapPhase values are:

        - min = 0.0
        - max = 1.0
        - scale = 'lin'
        - name = 'phase'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=0.0):
        SLMap.__init__(self, 0.0, 1.0, "lin", "phase", init, "float", 0.025)


class SLMapPan(SLMap):
    """
    SLMap with normalized values for a 'pan' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 0.

    .. note::

        SLMapPhase values are:

        - min = 0.0
        - max = 1.0
        - scale = 'lin'
        - name = 'pan'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=0.0):
        SLMap.__init__(self, 0.0, 1.0, "lin", "pan", init, "float", 0.025)


class SLMapQ(SLMap):
    """
    SLMap with normalized values for a 'q' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 1.

    .. note::

        SLMapQ values are:

        - min = 0.1
        - max = 100.0
        - scale = 'log'
        - name = 'q'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.1, 100.0, "log", "q", init, "float", 0.025)


class SLMapDur(SLMap):
    """
    SLMap with normalized values for a 'dur' slider.

    :Parent: :py:class:`SLMap`

    :Args:

        init: int or float, optional
            Initial value. Specified in the real range, not between 0 and 1.
            Defaults to 1.

    .. note::

        SLMapDur values are:

        - min = 0.
        - max = 60.0
        - scale = 'lin'
        - name = 'dur'
        - res = 'float'
        - ramp = 0.025

    """

    def __init__(self, init=1.0):
        SLMap.__init__(self, 0.0, 60.0, "lin", "dur", init, "float", 0.025)
