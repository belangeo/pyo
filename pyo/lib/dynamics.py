"""
Objects to modify the dynamic range and sample quality of audio signals.

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


class Clip(PyoObject):
    """
    Clips a signal to a predefined limit.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        min: float or PyoObject, optional
            Minimum possible value. Defaults to -1.
        max: float or PyoObject, optional
            Maximum possible value. Defaults to 1.

    .. seealso::

        :py:class:`Mirror`, :py:class:`Wrap`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> lfoup = Sine(freq=.25, mul=.48, add=.5)
    >>> lfodown = 0 - lfoup
    >>> c = Clip(a, min=lfodown, max=lfoup, mul=.4).mix(2).out()

    """

    def __init__(self, input, min=-1.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Clip_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                New `min` attribute.

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
                New `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(-1.0, 0.0, "lin", "min", self._min),
            SLMap(0.0, 1.0, "lin", "max", self._max),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        """float or PyoObject. Minimum possible value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum possible value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Mirror(PyoObject):
    """
    Reflects the signal that exceeds the `min` and `max` thresholds.

    This object is useful for table indexing or for clipping and
    modeling an audio signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        min: float or PyoObject, optional
            Minimum possible value. Defaults to 0.
        max: float or PyoObject, optional
            Maximum possible value. Defaults to 1.

    .. note::

        If `min` is higher than `max`, then the output will be the average of the two.

    .. seealso::

        :py:class:`Clip`, :py:class:`Wrap`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=[300,301])
    >>> lfmin = Sine(freq=1.5, mul=.25, add=-0.75)
    >>> lfmax = Sine(freq=2, mul=.25, add=0.75)
    >>> b = Mirror(a, min=lfmin, max=lfmax)
    >>> c = Tone(b, freq=2500, mul=.15).out()

    """

    def __init__(self, input, min=0.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Mirror_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                New `min` attribute.

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
                New `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "min", self._min),
            SLMap(0.0, 1.0, "lin", "max", self._max),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        """float or PyoObject. Minimum possible value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum possible value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Degrade(PyoObject):
    """
    Signal quality reducer.

    Degrade takes an audio signal and reduces the sampling rate and/or
    bit-depth as specified.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        bitdepth: float or PyoObject, optional
            Signal quantization in bits. Must be in range 1 -> 32.
            Defaults to 16.
        srscale: float or PyoObject, optional
            Sampling rate multiplier. Must be in range 0.0009765625 -> 1.
            Defaults to 1.

    .. seealso::

        :py:class:`Disto`, :py:class:`Clip`

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SquareTable()
    >>> a = Osc(table=t, freq=[100,101], mul=.5)
    >>> lfo = Sine(freq=.2, mul=6, add=8)
    >>> lfo2 = Sine(freq=.25, mul=.45, add=.55)
    >>> b = Degrade(a, bitdepth=lfo, srscale=lfo2, mul=.3).out()

    """

    def __init__(self, input, bitdepth=16, srscale=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, bitdepth, srscale, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._bitdepth = bitdepth
        self._srscale = srscale
        self._in_fader = InputFader(input)
        in_fader, bitdepth, srscale, mul, add, lmax = convertArgsToLists(self._in_fader, bitdepth, srscale, mul, add)
        self._base_objs = [
            Degrade_base(wrap(in_fader, i), wrap(bitdepth, i), wrap(srscale, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setBitdepth(self, x):
        """
        Replace the `bitdepth` attribute.

        :Args:

            x: float or PyoObject
                New `bitdepth` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bitdepth = x
        x, lmax = convertArgsToLists(x)
        [obj.setBitdepth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSrscale(self, x):
        """
        Replace the `srscale` attribute.

        :Args:

            x: float or PyoObject
                New `srscale` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._srscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setSrscale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(1.0, 32.0, "log", "bitdepth", self._bitdepth),
            SLMap(0.0009765625, 1.0, "log", "srscale", self._srscale),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def bitdepth(self):
        """float or PyoObject. Signal quantization in bits."""
        return self._bitdepth

    @bitdepth.setter
    def bitdepth(self, x):
        self.setBitdepth(x)

    @property
    def srscale(self):
        """float or PyoObject. Sampling rate multiplier."""
        return self._srscale

    @srscale.setter
    def srscale(self, x):
        self.setSrscale(x)


class Compress(PyoObject):
    """
    Reduces the dynamic range of an audio signal.

    Compress reduces the volume of loud sounds or amplifies quiet sounds by
    narrowing or compressing an audio signal's dynamic range.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        thresh: float or PyoObject, optional
            Level, expressed in dB, above which the signal is reduced.
            Reference level is 0dB. Defaults to -20.
        ratio: float or PyoObject, optional
            Determines the input/output ratio for signals above the
            threshold. Defaults to 2.
        risetime: float or PyoObject, optional
            Used in amplitude follower, time to reach upward value in
            seconds. Defaults to 0.01.
        falltime: float or PyoObject, optional
            Used in amplitude follower, time to reach downward value in
            seconds. Defaults to 0.1.
        lookahead: float, optional
            Delay length, in ms, for the "look-ahead" buffer. Range is
            0 -> 25 ms. Defaults to 5.0.
        knee: float optional
            Shape of the transfert function around the threshold, specified
            in the range 0 -> 1.

            A value of 0 means a hard knee and a value of 1.0 means a softer
            knee. Defaults to 0.
        outputAmp: boolean, optional
            If True, the object's output signal will be the compression level
            alone, not the compressed signal.

            It can be useful if 2 or more channels need to be linked on the
            same compression slope. Defaults to False.

            Available at initialization only.

    .. seealso::

        :py:class:`Gate`, :py:class:`Expand`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + '/transparent.aif', loop=True)
    >>> b = Compress(a, thresh=-24, ratio=6, risetime=.01, falltime=.2, knee=0.5).mix(2).out()

    """

    def __init__(
        self,
        input,
        thresh=-20,
        ratio=2,
        risetime=0.01,
        falltime=0.1,
        lookahead=5.0,
        knee=0,
        outputAmp=False,
        mul=1,
        add=0,
    ):
        pyoArgsAssert(
            self, "oOOOOnnbOO", input, thresh, ratio, risetime, falltime, lookahead, knee, outputAmp, mul, add
        )
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._thresh = thresh
        self._ratio = ratio
        self._risetime = risetime
        self._falltime = falltime
        self._lookahead = lookahead
        self._knee = knee
        self._in_fader = InputFader(input)
        in_fader, thresh, ratio, risetime, falltime, lookahead, knee, outputAmp, mul, add, lmax = convertArgsToLists(
            self._in_fader, thresh, ratio, risetime, falltime, lookahead, knee, outputAmp, mul, add
        )
        self._base_objs = [
            Compress_base(
                wrap(in_fader, i),
                wrap(thresh, i),
                wrap(ratio, i),
                wrap(risetime, i),
                wrap(falltime, i),
                wrap(lookahead, i),
                wrap(knee, i),
                wrap(outputAmp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThresh(self, x):
        """
        Replace the `thresh` attribute.

        :Args:

            x: float or PyoObject
                New `thresh` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        """
        Replace the `ratio` attribute.

        :Args:

            x: float or PyoObject
                New `ratio` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        """
        Replace the `risetime` attribute.

        :Args:

            x: float or PyoObject
                New `risetime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        """
        Replace the `falltime` attribute.

        :Args:

            x: float or PyoObject
                New `falltime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        """
        Replace the `lookahead` attribute.

        :Args:

            x: float
                New `lookahead` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setKnee(self, x):
        """
        Replace the `knee` attribute.

        :Args:

            x: float
                New `knee` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._knee = x
        x, lmax = convertArgsToLists(x)
        [obj.setKnee(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(-60.0, 0.0, "lin", "thresh", self._thresh),
            SLMap(1.0, 10.0, "lin", "ratio", self._ratio),
            SLMap(0.001, 0.3, "lin", "risetime", self._risetime),
            SLMap(0.001, 0.3, "lin", "falltime", self._falltime),
            SLMap(0, 25, "lin", "lookahead", self._lookahead, dataOnly=True),
            SLMap(0, 1, "lin", "knee", self._knee, dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def thresh(self):
        """float or PyoObject. Level above which the signal is reduced."""
        return self._thresh

    @thresh.setter
    def thresh(self, x):
        self.setThresh(x)

    @property
    def ratio(self):
        """float or PyoObject. in/out ratio for signals above the threshold."""
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def risetime(self):
        """float or PyoObject. Time to reach upward value in seconds."""
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to reach downward value in seconds."""
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        """float. Delay length, in ms, of the "look-ahead" buffer."""
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)

    @property
    def knee(self):
        """float. Shape of the transfert function around the threshold."""
        return self._knee

    @knee.setter
    def knee(self, x):
        self.setKnee(x)


class Gate(PyoObject):
    """
    Allows a signal to pass only when its amplitude is above a set threshold.

    A noise gate is used when the level of the signal is below the level of
    the noise floor. The threshold is set above the level of the noise and so when
    there is no signal the gate is closed. A noise gate does not remove noise
    from the signal. When the gate is open both the signal and the noise will
    pass through.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        thresh: float or PyoObject, optional
            Level, expressed in dB, below which the gate is closed.
            Reference level is 0dB. Defaults to -70.
        risetime: float or PyoObject, optional
            Time to open the gate in seconds. Defaults to 0.01.
        falltime: float or PyoObject, optional
            Time to close the gate in seconds. Defaults to 0.05.
        lookahead: float, optional
            Delay length, in ms, for the "look-ahead" buffer. Range is
            0 -> 25 ms. Defaults to 5.0.
        outputAmp: boolean, optional
            If True, the object's output signal will be the gating level
            alone, not the gated signal.

            It can be useful if 2 or more channels need to linked on the
            same gating slope. Defaults to False.

            Available at initialization only.

    .. seealso::

        :py:class:`Compress`, :py:class:`Expand`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + '/transparent.aif', speed=[1,.5], loop=True)
    >>> gt = Gate(sf, thresh=-24, risetime=0.005, falltime=0.01, lookahead=5, mul=.4).out()

    """

    def __init__(self, input, thresh=-70, risetime=0.01, falltime=0.05, lookahead=5.0, outputAmp=False, mul=1, add=0):
        pyoArgsAssert(self, "oOOOnbOO", input, thresh, risetime, falltime, lookahead, outputAmp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._thresh = thresh
        self._risetime = risetime
        self._falltime = falltime
        self._lookahead = lookahead
        self._in_fader = InputFader(input)
        in_fader, thresh, risetime, falltime, lookahead, outputAmp, mul, add, lmax = convertArgsToLists(
            self._in_fader, thresh, risetime, falltime, lookahead, outputAmp, mul, add
        )
        self._base_objs = [
            Gate_base(
                wrap(in_fader, i),
                wrap(thresh, i),
                wrap(risetime, i),
                wrap(falltime, i),
                wrap(lookahead, i),
                wrap(outputAmp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThresh(self, x):
        """
        Replace the `thresh` attribute.

        :Args:

            x: float or PyoObject
                New `thresh` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        """
        Replace the `risetime` attribute.

        :Args:

            x: float or PyoObject
                New `risetime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        """
        Replace the `falltime` attribute.

        :Args:

            x: float or PyoObject
                New `falltime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        """
        Replace the `lookahead` attribute.

        :Args:

            x: float
                New `lookahead` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(-100.0, 0.0, "lin", "thresh", self._thresh),
            SLMap(0.0001, 0.3, "lin", "risetime", self._risetime),
            SLMap(0.0001, 0.3, "lin", "falltime", self._falltime),
            SLMap(0, 25, "lin", "lookahead", self._lookahead, dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def thresh(self):
        """float or PyoObject. Level below which the gate is closed."""
        return self._thresh

    @thresh.setter
    def thresh(self, x):
        self.setThresh(x)

    @property
    def risetime(self):
        """float or PyoObject. Time to open the gate in seconds."""
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to close the gate in seconds."""
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        """float. Delay length, in ms, of the "look-ahead" buffer."""
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)


class Balance(PyoObject):
    """
    Adjust rms power of an audio signal according to the rms power of another.

    The rms power of a signal is adjusted to match that of a comparator signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        input2: PyoObject
            Comparator signal.
        freq: float or PyoObject, optional
            Cutoff frequency of the lowpass filter in hertz. Default to 10.

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + '/accord.aif', speed=[.99,1], loop=True, mul=.3)
    >>> comp = SfPlayer(SNDS_PATH + '/transparent.aif', speed=[.99,1], loop=True, mul=.3)
    >>> out = Balance(sf, comp, freq=10).out()

    """

    def __init__(self, input, input2, freq=10, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, input2, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._freq = freq
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, freq, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, freq, mul, add)
        self._base_objs = [
            Balance_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(freq, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Input signal to process.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        """
        Replace the `input2` attribute.

        Comparator signal.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Cutoff frequency of the lowpass filter, in Hertz.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.1, 100.0, "log", "freq", self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        """PyoObject. Comparator signal."""
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the lowpass filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Min(PyoObject):
    """
    Outputs the minimum of two values.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        comp: float or PyoObject, optional
            Comparison value. If `input` is lower than this value,
            `input` is send to the output, otherwise, `comp` is outputted.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Triangle wave
    >>> a = Phasor([249,250])
    >>> b = Min(a, comp=a*-1+1, mul=4, add=-1)
    >>> c = Tone(b, freq=1500, mul=.5).out()

    """

    def __init__(self, input, comp=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, comp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._comp = comp
        self._in_fader = InputFader(input)
        in_fader, comp, mul, add, lmax = convertArgsToLists(self._in_fader, comp, mul, add)
        self._base_objs = [Min_base(wrap(in_fader, i), wrap(comp, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        """
        Replace the `comp` attribute.

        :Args:

            x: float or PyoObject
                New `comp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 1, "lin", "comp", self._comp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def comp(self):
        """float or PyoObject. Comparison value."""
        return self._comp

    @comp.setter
    def comp(self, x):
        self.setComp(x)


class Max(PyoObject):
    """
    Outputs the maximum of two values.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        comp: float or PyoObject, optional
            Comparison value. If `input` is higher than this value,
            `input` is send to the output, otherwise, `comp` is outputted.

    >>> s = Server().boot()
    >>> s.start()
    >>> # Assimetrical clipping
    >>> a = Phasor(500, mul=2, add=-1)
    >>> b = Max(a, comp=-0.3)
    >>> c = Tone(b, freq=1500, mul=.5).out()

    """

    def __init__(self, input, comp=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, comp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._comp = comp
        self._in_fader = InputFader(input)
        in_fader, comp, mul, add, lmax = convertArgsToLists(self._in_fader, comp, mul, add)
        self._base_objs = [Max_base(wrap(in_fader, i), wrap(comp, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        """
        Replace the `comp` attribute.

        :Args:

            x: float or PyoObject
                New `comp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 1, "lin", "comp", self._comp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def comp(self):
        """float or PyoObject. Comparison value."""
        return self._comp

    @comp.setter
    def comp(self, x):
        self.setComp(x)


class Expand(PyoObject):
    """
    Expand the dynamic range of an audio signal.

    The Expand object will boost the volume of the input sound if it rises
    above the upper threshold. It will also reduce the volume of the input
    sound if it falls below the lower threshold. This process will "expand"
    the audio signal's dynamic range.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        downthresh: float or PyoObject, optional
            Level, expressed in dB, below which the signal is getting
            softer, according to the `ratio`. Reference level is 0dB.
            Defaults to -20.
        upthresh: float or PyoObject, optional
            Level, expressed in dB, above which the signal is getting
            louder, according to the same `ratio` as the lower threshold.
            Reference level is 0dB. Defaults to -20.
        ratio: float or PyoObject, optional
            The `ratio` argument controls is the amount of expansion (with
            a ratio of 4, if there is a rise of 2 dB above the upper
            threshold, the output signal will rises by 8 dB), and contrary
            for the lower threshold. Defaults to 2.
        risetime: float or PyoObject, optional
            Used in amplitude follower, time to reach upward value in
            seconds. Defaults to 0.01.
        falltime: float or PyoObject, optional
            Used in amplitude follower, time to reach downward value in
            seconds. Defaults to 0.1.
        lookahead: float, optional
            Delay length, in ms, for the "look-ahead" buffer. Range is
            0 -> 25 ms. Defaults to 5.0.
        outputAmp: boolean, optional
            If True, the object's output signal will be the expansion level
            alone, not the expanded signal.

            It can be useful if 2 or more channels need to be linked on the
            same expansion slope. Defaults to False.

            Available at initialization only.

    .. seealso::

        :py:class:`Compress`, :py:class:`Gate`

    >>> s = Server().boot()
    >>> s.start()
    >>> # original to the left channel
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=0.7)
    >>> ori = Delay(sf, 0.005).out()
    >>> # expanded to the right channel
    >>> ex = Expand(sf, downthresh=-20, upthresh=-20, ratio=4, mul=0.5).out(1)

    """

    def __init__(
        self,
        input,
        downthresh=-40,
        upthresh=-10,
        ratio=2,
        risetime=0.01,
        falltime=0.1,
        lookahead=5.0,
        outputAmp=False,
        mul=1,
        add=0,
    ):
        pyoArgsAssert(
            self, "oOOOOOnbOO", input, downthresh, upthresh, ratio, risetime, falltime, lookahead, outputAmp, mul, add
        )
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._downthresh = downthresh
        self._upthresh = upthresh
        self._ratio = ratio
        self._risetime = risetime
        self._falltime = falltime
        self._lookahead = lookahead
        self._in_fader = InputFader(input)
        (
            in_fader,
            downthresh,
            upthresh,
            ratio,
            risetime,
            falltime,
            lookahead,
            outputAmp,
            mul,
            add,
            lmax,
        ) = convertArgsToLists(
            self._in_fader, downthresh, upthresh, ratio, risetime, falltime, lookahead, outputAmp, mul, add
        )
        self._base_objs = [
            Expand_base(
                wrap(in_fader, i),
                wrap(downthresh, i),
                wrap(upthresh, i),
                wrap(ratio, i),
                wrap(risetime, i),
                wrap(falltime, i),
                wrap(lookahead, i),
                wrap(outputAmp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDownThresh(self, x):
        """
        Replace the `downthresh` attribute.

        :Args:

            x: float or PyoObject
                New `downthresh` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._downthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setDownThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setUpThresh(self, x):
        """
        Replace the `upthresh` attribute.

        :Args:

            x: float or PyoObject
                New `upthresh` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._upthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setUpThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        """
        Replace the `ratio` attribute.

        :Args:

            x: float or PyoObject
                New `ratio` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        """
        Replace the `risetime` attribute.

        :Args:

            x: float or PyoObject
                New `risetime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        """
        Replace the `falltime` attribute.

        :Args:

            x: float or PyoObject
                New `falltime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        """
        Replace the `lookahead` attribute.

        :Args:

            x: float
                New `lookahead` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(-90.0, 0.0, "lin", "downthresh", self._downthresh),
            SLMap(-60.0, 0.0, "lin", "upthresh", self._upthresh),
            SLMap(1.0, 10.0, "lin", "ratio", self._ratio),
            SLMap(0.001, 0.3, "lin", "risetime", self._risetime),
            SLMap(0.001, 0.3, "lin", "falltime", self._falltime),
            SLMap(0, 25, "lin", "lookahead", self._lookahead, dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def downthresh(self):
        """float or PyoObject. Level below which the signal is reduced."""
        return self._downthresh

    @downthresh.setter
    def downthresh(self, x):
        self.setDownThresh(x)

    @property
    def upthresh(self):
        """float or PyoObject. Level above which the signal is boosted."""
        return self._upthresh

    @upthresh.setter
    def upthresh(self, x):
        self.setUpThresh(x)

    @property
    def ratio(self):
        """float or PyoObject. in/out ratio for signals outside thresholds."""
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def risetime(self):
        """float or PyoObject. Time to reach upward value in seconds."""
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to reach downward value in seconds."""
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        """float. Delay length, in ms, of the "look-ahead" buffer."""
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)
