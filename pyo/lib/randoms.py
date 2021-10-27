"""
Set of objects that implement different kinds of random noise generators.

"""

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
from ._core import *
from ._maps import *


class Randi(PyoObject):
    """
    Periodic pseudo-random generator with interpolation.

    Randi generates a pseudo-random number between `min` and `max`
    values at a frequency specified by `freq` parameter. Randi will
    produce straight-line interpolation between current number and the next.

    :Parent: :py:class:`PyoObject`

    :Args:

        min: float or PyoObject, optional
            Minimum value for the random generation. Defaults to 0.
        max: float or PyoObject, optional
            Maximum value for the random generation. Defaults to 1.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> freq = Randi(500, 3000, 4)
    >>> noze = Noise().mix(2)
    >>> a = Biquad(noze, freq=freq, q=5, type=2, mul=.5).out()

    """

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
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                new `min` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                new `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "min", self._min),
            SLMap(1.0, 2.0, "lin", "max", self._max),
            SLMap(0.1, 20.0, "lin", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self):
        """float or PyoObject. Minimum value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Randh(PyoObject):
    """
    Periodic pseudo-random generator.

    Randh generates a pseudo-random number between `min` and `max`
    values at a frequency specified by `freq` parameter. Randh will
    hold generated value until next generation.

    :Parent: :py:class:`PyoObject`

    :Args:

        min: float or PyoObject, optional
            Minimum value for the random generation. Defaults to 0.
        max: float or PyoObject, optional
            Maximum value for the random generation. Defaults to 1.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> freq = Randh(500, 3000, 4)
    >>> noze = Noise().mix(2)
    >>> a = Biquad(noze, freq=freq, q=5, type=2, mul=.5).out()

    """

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
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                new `min` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                new `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "min", self._min),
            SLMap(1.0, 2.0, "lin", "max", self._max),
            SLMap(0.1, 20.0, "lin", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self):
        """float or PyoObject. Minimum value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Choice(PyoObject):
    """
    Periodically choose a new value from a user list.

    Choice chooses a new value from a predefined list of floats `choice`
    at a frequency specified by `freq` parameter. Choice will
    hold choosen value until next generation.

    :Parent: :py:class:`PyoObject`

    :Args:

        choice: list of floats or list of lists of floats
            Possible values for the random generation.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> freqs = midiToHz([60,62,64,65,67,69,71,72])
    >>> rnd = Choice(choice=freqs, freq=[3,4])
    >>> a = SineLoop(rnd, feedback=0.05, mul=.2).out()

    """

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
        """
        Replace the `choice` attribute.

        :Args:

            x: list of floats or list of lists of floats
                new `choice` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._choice = x
        if type(x[0]) != list:
            [obj.setChoice(self._choice) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setChoice(wrap(self._choice, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.1, 20.0, "lin", "freq", self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def choice(self):
        """list of floats or list of lists of floats. Possible choices."""
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class RandInt(PyoObject):
    """
    Periodic pseudo-random integer generator.

    RandInt generates a pseudo-random integer number between 0 and `max`
    values at a frequency specified by `freq` parameter. RandInt will
    hold generated value until the next generation.

    :Parent: :py:class:`PyoObject`

    :Args:

        max: float or PyoObject, optional
            Maximum value for the random generation. Defaults to 100.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> freq = RandInt(max=10, freq=5, mul=100, add=500)
    >>> jit = Randi(min=0.99, max=1.01, freq=[2.33,3.41])
    >>> a = SineLoop(freq*jit, feedback=0.03, mul=.2).out()

    """

    def __init__(self, max=100, freq=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", max, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._max = max
        self._freq = freq
        max, freq, mul, add, lmax = convertArgsToLists(max, freq, mul, add)
        self._base_objs = [RandInt_base(wrap(max, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                new `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(1.0, 2.0, "lin", "max", self._max),
            SLMap(0.1, 20.0, "lin", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def max(self):
        """float or PyoObject. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class RandDur(PyoObject):
    """
    Recursive time varying pseudo-random generator.

    RandDur generates a pseudo-random number between `min` and `max`
    arguments and uses that number to set the delay time before the next
    generation. RandDur will hold the generated value until next generation.

    :Parent: :py:class:`PyoObject`

    :Args:

        min: float or PyoObject, optional
            Minimum value for the random generation. Defaults to 0.
        max: float or PyoObject, optional
            Maximum value for the random generation. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> dur = RandDur(min=[.05,0.1], max=[.4,.5])
    >>> trig = Change(dur)
    >>> amp = TrigEnv(trig, table=HannTable(), dur=dur, mul=.2)
    >>> freqs = midiToHz([60,63,67,70,72])
    >>> freq = TrigChoice(trig, choice=freqs)
    >>> a = LFO(freq=freq, type=2, mul=amp).out()

    """

    def __init__(self, min=0.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._min = min
        self._max = max
        min, max, mul, add, lmax = convertArgsToLists(min, max, mul, add)
        self._base_objs = [RandDur_base(wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                new `min` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                new `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "min", self._min),
            SLMap(1.0, 2.0, "lin", "max", self._max),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def min(self):
        """float or PyoObject. Minimum value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Xnoise(PyoObject):
    """
    X-class pseudo-random generator.

    Xnoise implements a few of the most common noise distributions.
    Each distribution generates values in the range 0 and 1.

    :Parent: :py:class:`PyoObject`

    :Args:

        dist: string or int, optional
            Distribution type. Defaults to 0.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.
        x1: float or PyoObject, optional
            First parameter. Defaults to 0.5.
        x2: float or PyoObject, optional
            Second parameter. Defaults to 0.5.

    .. note::

        Available distributions are:
            0. uniform
            1. linear minimum
            2. linear maximum
            3. triangular
            4. exponential minimum
            5. exponential maximum
            6. double (bi)exponential
            7. cauchy
            8. weibull
            9. gaussian
            10. poisson
            11. walker (drunk)
            12. loopseg (drunk with looped segments)

        Depending on the distribution, `x1` and `x2` parameters are applied
        as follow (names as string, or associated number can be used as `dist`
        parameter):

            0. uniform
                - x1: not used
                - x2: not used
            1. linear_min
                - x1: not used
                - x2: not used
            2. linear_max
                - x1: not used
                - x2: not used
            3. triangle
                - x1: not used
                - x2: not used
            4. expon_min
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            5. expon_max
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            6. biexpon
                - x1: bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
                - x2: not used
            7. cauchy
                - x1: bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
                - x2: not used
            8. weibull
                - x1: mean location {0 -> 1}
                - x2: shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
            9. gaussian
                - x1: mean location {0 -> 1}
                - x2: bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
            10. poisson
                 - x1: gravity center {0 = low values -> 10 = high values}
                 - x2: compress/expand range {0.1 = full compress -> 4 full expand}
            11. walker
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}
            12. loopseg
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Phasor(.1, 0, .5, .15)
    >>> freq = Xnoise(dist=12, freq=8, x1=1, x2=lfo, mul=1000, add=500)
    >>> jit = Randi(min=0.99, max=1.01, freq=[2.33,3.41])
    >>> a = SineLoop(freq*jit, feedback=0.03, mul=.2).out()

    """

    def __init__(self, dist=0, freq=1.0, x1=0.5, x2=0.5, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", freq, x1, x2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._dist = dist
        self._freq = freq
        self._x1 = x1
        self._x2 = x2
        dist, freq, x1, x2, mul, add, lmax = convertArgsToLists(dist, freq, x1, x2, mul, add)
        for i, t in enumerate(dist):
            if type(t) in [bytes, str]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            Xnoise_base(wrap(dist, i), wrap(freq, i), wrap(x1, i), wrap(x2, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setDist(self, x):
        """
        Replace the `dist` attribute.

        :Args:

            x: string or int
                new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes, str]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        :Args:

            x: float or PyoObject
                new `x1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        :Args:

            x: float or PyoObject
                new `x2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 12, "lin", "dist", self._dist, res="int", dataOnly=True),
            SLMap(0.001, 200.0, "log", "freq", self._freq),
            SLMap(0, 1, "lin", "x1", self._x1),
            SLMap(0, 1, "lin", "x2", self._x2),
            SLMap(0, 2500, "lin", "mul", self._mul),
            SLMap(0, 2500, "lin", "add", self._add),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self):
        """string or int. Distribution type."""
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def x1(self):
        """float or PyoObject. First parameter."""
        return self._x1

    @x1.setter
    def x1(self, x):
        self.setX1(x)

    @property
    def x2(self):
        """float or PyoObject. Second parameter."""
        return self._x2

    @x2.setter
    def x2(self, x):
        self.setX2(x)


class XnoiseMidi(PyoObject):
    """
    X-class midi notes pseudo-random generator.

    XnoiseMidi implements a few of the most common noise distributions.
    Each distribution generates integer values in the range defined with
    `mrange` parameter and output can be scaled on midi notes, hertz or
    transposition factor.

    :Parent: :py:class:`PyoObject`

    :Args:

        dist: string or int, optional
            Distribution type. Defaults to 0.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.
        x1: float or PyoObject, optional
            First parameter. Defaults to 0.5.
        x2: float or PyoObject, optional
            Second parameter. Defaults to 0.5.
        scale: int {0, 1, 2}, optional
            Output format. 0 = Midi, 1 = Hertz, 2 = transposition factor.
            In the transposition mode, the central key (the key where there
            is no transposition) is (`minrange` + `maxrange`) / 2. Defaults
            to 0.
        mrange: tuple of int, optional
            Minimum and maximum possible values, in Midi notes. Available
            only at initialization time. Defaults to (0, 127).

    .. note::

        Available distributions are:
            0. uniform
            1. linear minimum
            2. linear maximum
            3. triangular
            4. exponential minimum
            5. exponential maximum
            6. double (bi)exponential
            7. cauchy
            8. weibull
            9. gaussian
            10. poisson
            11. walker (drunk)
            12. loopseg (drunk with looped segments)

        Depending on the distribution, `x1` and `x2` parameters are applied
        as follow (names as string, or associated number can be used as `dist`
        parameter):

            0. uniform
                - x1: not used
                - x2: not used
            1. linear_min
                - x1: not used
                - x2: not used
            2. linear_max
                - x1: not used
                - x2: not used
            3. triangle
                - x1: not used
                - x2: not used
            4. expon_min
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            5. expon_max
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            6. biexpon
                - x1: bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
                - x2: not used
            7. cauchy
                - x1: bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
                - x2: not used
            8. weibull
                - x1: mean location {0 -> 1}
                - x2: shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
            9. gaussian
                - x1: mean location {0 -> 1}
                - x2: bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
            10. poisson
                 - x1: gravity center {0 = low values -> 10 = high values}
                 - x2: compress/expand range {0.1 = full compress -> 4 full expand}
            11. walker
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}
            12. loopseg
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}

    >>> s = Server().boot()
    >>> s.start()
    >>> l = Phasor(.4)
    >>> rnd = XnoiseMidi('loopseg', freq=8, x1=1, x2=l, scale=0, mrange=(60,96))
    >>> freq = Snap(rnd, choice=[0, 2, 3, 5, 7, 8, 11], scale=1)
    >>> jit = Randi(min=0.99, max=1.01, freq=[2.33,3.41])
    >>> a = SineLoop(freq*jit, feedback=0.03, mul=.2).out()

    """

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
            if type(t) in [bytes, str]:
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
        """
        Replace the `dist` attribute.

        :Args:

            x: string or int
                new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes, str]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        """
        Replace the `scale` attribute.

        Possible values are:
            0. Midi notes
            1. Hertz
            2. transposition factor (centralkey is (`minrange` + `maxrange`) / 2

        :Args:

            x: int {0, 1, 2}
                new `scale` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRange(self, mini, maxi):
        """
        Replace the `mrange` attribute.

        :Args:

            mini: int
                minimum output midi range.
            maxi: int
                maximum output midi range.

        """
        pyoArgsAssert(self, "ii", mini, maxi)
        self._mrange = (mini, maxi)
        mini, maxi, lmax = convertArgsToLists(mini, maxi)
        [obj.setRange(wrap(mini, i), wrap(maxi, i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        :Args:

            x: float or PyoObject
                new `x1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        :Args:

            x: float or PyoObject
                new `x2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 12, "lin", "dist", self._dist, res="int", dataOnly=True),
            SLMap(0.001, 200.0, "log", "freq", self._freq),
            SLMap(0, 1, "lin", "x1", self._x1),
            SLMap(0, 1, "lin", "x2", self._x2),
            SLMap(0, 2, "lin", "scale", self._scale, res="int", dataOnly=True),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self):
        """string or int. Distribution type."""
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def x1(self):
        """float or PyoObject. First parameter."""
        return self._x1

    @x1.setter
    def x1(self, x):
        self.setX1(x)

    @property
    def x2(self):
        """float or PyoObject. Second parameter."""
        return self._x2

    @x2.setter
    def x2(self, x):
        self.setX2(x)

    @property
    def scale(self):
        """int. Output format."""
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class XnoiseDur(PyoObject):
    """
    Recursive time varying X-class pseudo-random generator.

    Xnoise implements a few of the most common noise distributions.
    Each distribution generates values in the range 0 to 1, which are
    then rescaled between `min` and `max` arguments. The object uses
    the generated value to set the delay time before the next generation.
    XnoiseDur will hold the value until next generation.

    :Parent: :py:class:`PyoObject`

    :Args:

        dist: string or int, optional
            Distribution type. Can be the name of the distribution as a string
            or its associated number. Defaults to 0.
        min: float or PyoObject, optional
            Minimum value for the random generation. Defaults to 0.
        max: float or PyoObject, optional
            Maximum value for the random generation. Defaults to 1.
        x1: float or PyoObject, optional
            First parameter. Defaults to 0.5.
        x2: float or PyoObject, optional
            Second parameter. Defaults to 0.5.

    .. note::

        Available distributions are:
            0. uniform
            1. linear minimum
            2. linear maximum
            3. triangular
            4. exponential minimum
            5. exponential maximum
            6. double (bi)exponential
            7. cauchy
            8. weibull
            9. gaussian
            10. poisson
            11. walker (drunk)
            12. loopseg (drunk with looped segments)

        Depending on the distribution, `x1` and `x2` parameters are applied
        as follow (names as string, or associated number can be used as `dist`
        parameter):

            0. uniform
                - x1: not used
                - x2: not used
            1. linear_min
                - x1: not used
                - x2: not used
            2. linear_max
                - x1: not used
                - x2: not used
            3. triangle
                - x1: not used
                - x2: not used
            4. expon_min
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            5. expon_max
                - x1: slope {0 = no slope -> 10 = sharp slope}
                - x2: not used
            6. biexpon
                - x1: bandwidth {0 = huge bandwidth -> 10 = narrow bandwidth}
                - x2: not used
            7. cauchy
                - x1: bandwidth {0 = narrow bandwidth -> 10 = huge bandwidth}
                - x2: not used
            8. weibull
                - x1: mean location {0 -> 1}
                - x2: shape {0.5 = linear min, 1.5 = expon min, 3.5 = gaussian}
            9. gaussian
                - x1: mean location {0 -> 1}
                - x2: bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth}
            10. poisson
                 - x1: gravity center {0 = low values -> 10 = high values}
                 - x2: compress/expand range {0.1 = full compress -> 4 full expand}
            11. walker
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}
            12. loopseg
                 - x1: maximum value {0.1 -> 1}
                 - x2: maximum step {0.1 -> 1}

    >>> s = Server().boot()
    >>> s.start()
    >>> dur = XnoiseDur(dist="expon_min", min=[.05,0.1], max=[.4,.5], x1=3)
    >>> trig = Change(dur)
    >>> amp = TrigEnv(trig, table=HannTable(), dur=dur, mul=.2)
    >>> freqs = midiToHz([60,63,67,70,72])
    >>> freq = TrigChoice(trig, choice=freqs)
    >>> a = LFO(freq=freq, type=2, mul=amp).out()

    """

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
            if type(t) in [bytes, str]:
                dist[i] = XNOISE_DICT.get(t, 0)
        self._base_objs = [
            XnoiseDur_base(
                wrap(dist, i), wrap(min, i), wrap(max, i), wrap(x1, i), wrap(x2, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setDist(self, x):
        """
        Replace the `dist` attribute.

        :Args:

            x: string or int
                new `dist` attribute.

        """
        self._dist = x
        x, lmax = convertArgsToLists(x)
        for i, t in enumerate(x):
            if type(t) in [bytes, str]:
                x[i] = XNOISE_DICT.get(t, 0)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                new `min` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                new `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX1(self, x):
        """
        Replace the `x1` attribute.

        :Args:

            x: float or PyoObject
                new `x1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setX2(self, x):
        """
        Replace the `x2` attribute.

        :Args:

            x: float or PyoObject
                new `x2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._x2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setX2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 12, "lin", "dist", self._dist, res="int", dataOnly=True),
            SLMap(0, 20, "lin", "min", self._min),
            SLMap(0, 20, "lin", "max", self._max),
            SLMap(0, 1, "lin", "x1", self._x1),
            SLMap(0, 1, "lin", "x2", self._x2),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def dist(self):
        """string or int. Distribution type."""
        return self._dist

    @dist.setter
    def dist(self, x):
        self.setDist(x)

    @property
    def min(self):
        """float or PyoObject. Minimum value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def x1(self):
        """float or PyoObject. First parameter."""
        return self._x1

    @x1.setter
    def x1(self, x):
        self.setX1(x)

    @property
    def x2(self):
        """float or PyoObject. Second parameter."""
        return self._x2

    @x2.setter
    def x2(self, x):
        self.setX2(x)


class Urn(PyoObject):
    """
    Periodic pseudo-random integer generator without duplicates.

    Urn generates a pseudo-random integer number between 0 and `max`
    values at a frequency specified by `freq` parameter. Urn will
    hold generated value until the next generation. Urn works like RandInt,
    except that it keeps track of each number which has been generated. After
    all numbers have been outputed, the pool is reseted and the object send
    a trigger signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        max: int, optional
            Maximum value for the random generation. Defaults to 100.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.

    .. note::

        Urn will send a trigger signal when the pool is empty.
        User can retreive the trigger streams by calling obj['trig'].
        Useful to synchronize other processes.

    >>> s = Server().boot()
    >>> s.start()
    >>> mid = Urn(max=12, freq=10, add=60)
    >>> fr = MToF(mid)
    >>> sigL = SineLoop(freq=fr, feedback=.08, mul=0.3).out()
    >>> amp = TrigExpseg(mid["trig"], [(0,0),(.01,.25),(1,0)])
    >>> sigR = SineLoop(midiToHz(84), feedback=0.05, mul=amp).out(1)

    """

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
        """
        Replace the `max` attribute.

        :Args:

            x: int
                new `max` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(1, 1000, "lin", "max", self._max, res="int", dataOnly=True),
            SLMap(0.1, 20.0, "lin", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def max(self):
        """int. Maximum value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class LogiMap(PyoObject):
    """
    Random generator based on the logistic map.

    The logistic equation (sometimes called the Verhulst model or logistic
    growth curve) is a model of population growth first published by Pierre
    Verhulst (1845, 1847). The logistic map is a discrete quadratic recurrence
    equation derived from the logistic equation that can be effectively used
    as a number generator that exhibit chaotic behavior. This object uses the
    following equation:

        x[n] = (r + 3) * x[n-1] * (1.0 - x[n-1])

    where 'r' is the randomization factor between 0 and 1.

    :Parent: :py:class:`PyoObject`

    :Args:

        chaos: float or PyoObject, optional
            Randomization factor, 0.0 < chaos < 1.0. Defaults to 0.6.
        freq: float or PyoObject, optional
            Polling frequency. Defaults to 1.
        init: float, optional
            Initial value, 0.0 < init < 1.0. Defaults to 0.5.

    .. note::

        The method play() resets the internal state to the initial value.

    >>> s = Server().boot()
    >>> s.start()
    >>> val = LogiMap([0.6,0.65], [4,8])
    >>> mid = Round(Scale(val, 0, 1, [36,48], [72,84]))
    >>> hz = Snap(mid, [0,2,4,5,7,9,11], 1)
    >>> env = CosTable([(0,0), (32,1), (4064,1), (4096,0), (8192,0)])
    >>> amp = TrigEnv(Change(val), table=env, dur=[.25,.125], mul=0.3)
    >>> osc = RCOsc(hz, mul=amp).out()

    """

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
        """
        Replace the `chaos` attribute.

        :Args:

            x: float or PyoObject
                new `chaos` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                new `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.001, 0.999, "lin", "chaos", self._chaos),
            SLMap(0.1, 20.0, "lin", "freq", self._freq),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def chaos(self):
        """float or PyoObject. Randomization factor."""
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)

    @property
    def freq(self):
        """float or PyoObject. Polling frequency."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)
