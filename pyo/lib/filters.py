"""
Different kinds of audio filtering operations.

An audio filter is designed to amplify, pass or attenuate (negative amplification)
some frequency ranges. Common types include low-pass, which pass through
frequencies below their cutoff frequencies, and progressively attenuates
frequencies above the cutoff frequency. A high-pass filter does the opposite,
passing high frequencies above the cutoff frequency, and progressively
attenuating frequencies below the cutoff frequency. A bandpass filter passes
frequencies between its two cutoff frequencies, while attenuating those outside
the range. A band-reject filter, attenuates frequencies between its two cutoff
frequencies, while passing those outside the 'reject' range.

An all-pass filter, passes all frequencies, but affects the phase of any given
sinusoidal component according to its frequency.

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


class Biquad(PyoObject):
    """
    A sweepable general purpose biquadratic digital filter.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff or center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined (for bandpass filters) as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.
        type: int, optional
            Filter type. Five possible values :
                0. lowpass (default)
                1. highpass
                2. bandpass
                3. bandstop
                4. allpass

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = Biquad(a, freq=lfo, q=5, type=2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute.
                    0.lowpass
                    1. highpass
                    2. bandpass
                    3. bandstop
                    4. allpass

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMapQ(self._q),
            SLMap(0, 4, "lin", "type", self._type, res="int", dataOnly=True),
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
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        """int. Filter type."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class Biquadx(PyoObject):
    """
    A multi-stages sweepable general purpose biquadratic digital filter.

    Biquadx is equivalent to a filter consisting of more layers of Biquad
    with the same arguments, serially connected. It is faster than using
    a large number of instances of the Biquad object, It uses less memory
    and allows filters with sharper cutoff.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff or center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined (for bandpass filters) as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.
        type: int, optional
            Filter type. Five possible values :
                0. lowpass (default)
                1. highpass
                2. bandpass
                3. bandstop
                4. allpass
        stages: int, optional
            The number of filtering stages in the filter stack. Defaults to 4.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = Biquadx(a, freq=lfo, q=5, type=2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute.
                0. lowpass
                1. highpass
                2. bandpass
                3. bandstop
                4. allpass

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        """
        Replace the `stages` attribute.

        :Args:

            x: int
                New `stages` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMapQ(self._q),
            SLMap(0, 4, "lin", "type", self._type, res="int", dataOnly=True),
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
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        """int. Filter type."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)

    @property
    def stages(self):
        """int. The number of filtering stages."""
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class Biquada(PyoObject):
    """
    A general purpose biquadratic digital filter (floating-point arguments).

    A digital biquad filter is a second-order recursive linear filter, containing
    two poles and two zeros. Biquada is a "Direct Form 1" implementation of a Biquad
    filter:

    y[n] = ( b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2] ) / a0

    This object is directly controlled via the six coefficients, as floating-point
    values or audio stream, of the filter. There is no clipping of the values given as
    coefficients, so, unless you know what you do, it is recommended to use the Biquad
    object, which is controlled with frequency, Q and type arguments.

    The default values of the object give a lowpass filter with a 1000 Hz cutoff frequency.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        b0: float or PyoObject, optional
            Amplitude of the current sample. Defaults to 0.005066.
        b1: float or PyoObject, optional
            Amplitude of the first input sample delayed. Defaults to 0.010132.
        b2: float or PyoObject, optional
            Amplitude of the second input sample delayed. Defaults to 0.005066.
        a0: float or PyoObject, optional
            Overall gain coefficient. Defaults to 1.070997.
        a1: float or PyoObject, optional
            Amplitude of the first output sample delayed. Defaults to -1.979735.
        a2: float or PyoObject, optional
            Amplitude of the second output sample delayed. Defaults to 0.929003.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lf = Sine([1.5, 2], mul=.07, add=-1.9)
    >>> f = Biquada(a, 0.005066, 0.010132, 0.005066, 1.070997, lf, 0.929003).out()

    """

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

    def setB0(self, x):
        """
        Replace the `b0` attribute.

        :Args:

            x: float or PyoObject
                New `b0` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b0.value = x

    def setB1(self, x):
        """
        Replace the `b1` attribute.

        :Args:

            x: float or PyoObject
                New `b1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b1.value = x

    def setB2(self, x):
        """
        Replace the `b2` attribute.

        :Args:

            x: float or PyoObject
                New `b2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._b2.value = x

    def setA0(self, x):
        """
        Replace the `a0` attribute.

        :Args:

            x: float or PyoObject
                New `a0` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a0.value = x

    def setA1(self, x):
        """
        Replace the `a1` attribute.

        :Args:

            x: float or PyoObject
                New `a1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a1.value = x

    def setA2(self, x):
        """
        Replace the `a2` attribute.

        :Args:

            x: float or PyoObject
                New `a2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._a2.value = x

    def setCoeffs(self, *args, **kwds):
        """
        Replace all filter coefficients.

        :Args:

            b0: float or PyoObject, optional
                New `b0` attribute.
            b1: float or PyoObject, optional
                New `b1` attribute.
            b2: float or PyoObject, optional
                New `b2` attribute.
            a0: float or PyoObject, optional
                New `a0` attribute.
            a1: float or PyoObject, optional
                New `a1` attribute.
            a2: float or PyoObject, optional
                New `a2` attribute.

        """
        for i, val in enumerate(args):
            attr = getattr(self, ["_b0", "_b1", "_b2", "_a0", "_a1", "_a2"][i])
            attr.value = val
        for key in kwds.keys():
            if hasattr(self, key):
                attr = getattr(self, "_" + key)
                attr.value = kwds[key]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def b0(self):
        """float or PyoObject. `b0` coefficient."""
        return self._b0

    @b0.setter
    def b0(self, x):
        self.setB0(x)

    @property
    def b1(self):
        """float or PyoObject. `b1` coefficient."""
        return self._b1

    @b1.setter
    def b1(self, x):
        self.setB1(x)

    @property
    def b2(self):
        """float or PyoObject. `b2` coefficient."""
        return self._b2

    @b2.setter
    def b2(self, x):
        self.setB2(x)

    @property
    def a0(self):
        """float or PyoObject. `a0` coefficient."""
        return self._a0

    @a0.setter
    def a0(self, x):
        self.setA0(x)

    @property
    def a1(self):
        """float or PyoObject. `a1` coefficient."""
        return self._a1

    @a1.setter
    def a1(self, x):
        self.setA1(x)

    @property
    def a2(self):
        """float or PyoObject. `a2` coefficient."""
        return self._a2

    @a2.setter
    def a2(self, x):
        self.setA2(x)


class EQ(PyoObject):
    """
    Equalizer filter.

    EQ is a biquadratic digital filter designed for equalization. It
    provides peak/notch and lowshelf/highshelf filters for building
    parametric equalizers.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff or center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.
        boost: float or PyoObject, optional
            Gain, expressed in dB, to add or remove at the center frequency.
            Default to -3.
        type: int, optional
            Filter type. Three possible values :
                0. peak/notch (default)
                1. lowshelf
                2. highshelf

    >>> s = Server().boot()
    >>> s.start()
    >>> amp = Fader(1, 1, mul=.15).play()
    >>> src = PinkNoise(amp)
    >>> fr = Sine(.2, 0, 500, 1500)
    >>> boo = Sine([4, 4], 0, 6)
    >>> out = EQ(src, freq=fr, q=1, boost=boo, type=0).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBoost(self, x):
        """
        Replace the `boost` attribute, expressed in dB.

        :Args:

            x: float or PyoObject
                New `boost` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._boost = x
        x, lmax = convertArgsToLists(x)
        [obj.setBoost(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute.
                0. peak
                1. lowshelf
                2. highshelf

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMapQ(self._q),
            SLMap(-40.0, 40.0, "lin", "boost", self._boost),
            SLMap(0, 2, "lin", "type", self._type, res="int", dataOnly=True),
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
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def boost(self):
        """float or PyoObject. Boost factor of the filter."""
        return self._boost

    @boost.setter
    def boost(self, x):
        self.setBoost(x)

    @property
    def type(self):
        """int. Filter type."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class Tone(PyoObject):
    """
    A first-order recursive low-pass filter with variable frequency response.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 1000.

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.3)
    >>> lf = Sine(freq=.2, mul=800, add=1000)
    >>> f = Tone(n, lf).mix(2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Atone(PyoObject):
    """
    A first-order recursive high-pass filter with variable frequency response.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 1000.

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.3)
    >>> lf = Sine(freq=.2, mul=5000, add=6000)
    >>> f = Atone(n, lf).mix(2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Port(PyoObject):
    """
    Exponential portamento.

    Perform an exponential portamento on an audio signal with
    different rising and falling times.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        risetime: float or PyoObject, optional
            Time to reach upward value in seconds. Defaults to 0.05.
        falltime: float or PyoObject, optional
            Time to reach downward value in seconds. Defaults to 0.05.
        init: float, optional
            Initial state of the internal memory. Available at intialization
            time only. Defaults to 0.

    >>> s = Server().boot()
    >>> s.start()
    >>> from random import uniform
    >>> x = Sig(value=500)
    >>> p = Port(x, risetime=.1, falltime=1)
    >>> a = Sine(freq=[p, p*1.01], mul=.2).out()
    >>> def new_freq():
    ...     x.value = uniform(400, 800)
    >>> pat = Pattern(function=new_freq, time=1).play()

    """

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.001, 10.0, "log", "risetime", self._risetime),
            SLMap(0.001, 10.0, "log", "falltime", self._falltime),
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


class DCBlock(PyoObject):
    """
    Implements the DC blocking filter.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.01)
    >>> w = Delay(n, delay=[0.02, 0.01], feedback=.995, mul=.5)
    >>> f = DCBlock(w).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DCBlock_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class BandSplit(PyoObject):
    """
    Splits an input signal into multiple frequency bands.

    The input signal will be separated into `num` bands between `min`
    and `max` frequencies using second-order bandpass filters. Each
    band will then be assigned to an independent audio stream.
    Useful for multiband processing.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        num: int, optional
            Number of frequency bands created. Available at initialization
            time only. Defaults to 6.
        min: float, optional
            Lowest frequency. Available at initialization time only.
            Defaults to 20.
        max: float, optional
            Highest frequency. Available at initialization time only.
            Defaults to 20000.
        q: float or PyoObject, optional
            Q of the filters, defined as center frequency / bandwidth.
            Should be between 1 and 500. Defaults to 1.

    .. seealso::

        :py:class:`FourBand`, :py:class:`MultiBand`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfos = Sine(freq=[.3,.4,.5,.6,.7,.8], mul=.5, add=.5)
    >>> n = PinkNoise(.5)
    >>> a = BandSplit(n, num=6, min=250, max=4000, q=5, mul=lfos).out()

    """

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

    def setQ(self, x):
        """
        Replace the `q` attribute.

        :Args:

            x: float or PyoObject
                new `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filters."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class FourBand(PyoObject):
    """
    Splits an input signal into four frequency bands.

    The input signal will be separated into 4 bands around `freqs`
    arguments using fourth-order Linkwitz-Riley lowpass and highpass
    filters. Each band will then be assigned to an independent audio
    stream. The sum of the four bands reproduces the same signal as
    the `input`. Useful for multiband processing.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq1: float or PyoObject, optional
            First crossover frequency. First band will contain signal
            from 0 Hz to `freq1` Hz. Defaults to 150.
        freq2: float or PyoObject, optional
            Second crossover frequency. Second band will contain signal
            from `freq1` Hz to `freq2`. `freq2` is the lower limit of the
            third band signal. Defaults to 500.
        freq3: float or PyoObject, optional
            Third crossover frequency. It's the upper limit of the third
            band signal and fourth band will contain signal from `freq3`
            to sr/2. Defaults to 2000.

    .. seealso::

        :py:class:`BandSplit`, :py:class:`MultiBand`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfos = Sine(freq=[.3,.4,.5,.6], mul=.5, add=.5)
    >>> n = PinkNoise(.3)
    >>> a = FourBand(n, freq1=250, freq2=1000, freq3=2500, mul=lfos).out()

    """

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

    def setFreq1(self, x):
        """
        Replace the `freq1` attribute.

        :Args:

            x: float or PyoObject
                new `freq1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq1(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFreq2(self, x):
        """
        Replace the `freq2` attribute.

        :Args:

            x: float or PyoObject
                new `freq2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq2(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFreq3(self, x):
        """
        Replace the `freq3` attribute.

        :Args:

            x: float or PyoObject
                new `freq3` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq3 = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq3(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(40, 300, "log", "freq1", self._freq1),
            SLMap(300, 1000, "log", "freq2", self._freq2),
            SLMap(1000, 5000, "log", "freq3", self._freq3),
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
    def freq1(self):
        """float or PyoObject. First crossover frequency."""
        return self._freq1

    @freq1.setter
    def freq1(self, x):
        self.setFreq1(x)

    @property
    def freq2(self):
        """float or PyoObject. Second crossover frequency."""
        return self._freq2

    @freq2.setter
    def freq2(self, x):
        self.setFreq2(x)

    @property
    def freq3(self):
        """float or PyoObject. Third crossover frequency."""
        return self._freq3

    @freq3.setter
    def freq3(self, x):
        self.setFreq3(x)


class MultiBand(PyoObject):
    """
    Splits an input signal into multiple frequency bands.

    The input signal will be separated into `num` logarithmically spaced
    frequency bands using fourth-order Linkwitz-Riley lowpass and highpass
    filters. Each band will then be assigned to an independent audio
    stream. The sum of all bands reproduces the same signal as the `input`.
    Useful for multiband processing.

    User-defined frequencies can be assigned with the `setFrequencies()` method.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        num: int, optional
            Number of frequency bands created, between 2 and 16.
            Available at initialization time only. Defaults to 8.

    .. seealso::

        :py:class:`BandSplit`, :py:class:`FourBand`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfos = Sine(freq=[.1,.2,.3,.4,.5,.6,.7,.8], mul=.5, add=.5)
    >>> n = PinkNoise(.3)
    >>> a = MultiBand(n, num=8, mul=lfos).out()

    """

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

    def setFrequencies(self, freqs):
        """
        Change the filters splitting frequencies.

        This method can be used to change filter frequencies to a particular set.
        Frequency list length must be egal to the number of bands minus 1.

        :Args:

            freqs: list of floats
                New frequencies used as filter cutoff frequencies.

        """
        pyoArgsAssert(self, "l", freqs)
        if len(freqs) != (self._num - 1):
            print(
                "pyo warning: MultiBand frequency list length must be egal to the number of bands, minus 1, of the object."
            )
            return
        [obj.setFrequencies(freqs) for obj in self._base_players]

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Hilbert(PyoObject):
    """
    Hilbert transform.

    Hilbert is an IIR filter based implementation of a broad-band 90 degree
    phase difference network. The outputs of hilbert have an identical
    frequency response to the input (i.e. they sound the same), but the two
    outputs have a constant phase difference of 90 degrees, plus or minus some
    small amount of error, throughout the entire frequency range. The outputs
    are in quadrature.

    Hilbert is useful in the implementation of many digital signal processing
    techniques that require a signal in phase quadrature. The real part corresponds
    to the cosine output of hilbert, while the imaginary part corresponds to the
    sine output. The two outputs have a constant phase difference throughout the
    audio range that corresponds to the phase relationship between cosine and sine waves.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    .. note::

        Real and imaginary parts are two separated set of streams.
        The user should call :

        |  Hilbert['real'] to retrieve the real part.
        |  Hilbert['imag'] to retrieve the imaginary part.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/accord.aif", loop=True, mul=0.5).out(0)
    >>> b = Hilbert(a)
    >>> quad = Sine([250, 500], [0, .25])
    >>> mod1 = b['real'] * quad[0]
    >>> mod2 = b['imag'] * quad[1]
    >>> up = (mod1 - mod2) * 0.7
    >>> down = mod1 + mod2
    >>> up.out(1)

    """

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
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "real" or "imag" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"real", "imag"}
                Address string parameter identifying audio stream.
                Defaults to "real".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Allpass(PyoObject):
    """
    Delay line based allpass filter.

    Allpass is based on the combination of feedforward and feedback comb
    filter. This kind of filter is often used in simple digital reverb
    implementations.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        delay: float or PyoObject, optional
            Delay time in seconds. Defaults to 0.01.
        feedback: float or PyoObject, optional
            Amount of output signal sent back into the delay line.
            Defaults to 0.
        maxdelay: float, optional
            Maximum delay length in seconds. Available only at initialization.
            Defaults to 1.

    >>> # SIMPLE REVERB
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=0.25).mix(2).out()
    >>> b1 = Allpass(a, delay=[.0204,.02011], feedback=0.25)
    >>> b2 = Allpass(b1, delay=[.06653,.06641], feedback=0.31)
    >>> b3 = Allpass(b2, delay=[.035007,.03504], feedback=0.4)
    >>> b4 = Allpass(b3, delay=[.023021 ,.022987], feedback=0.55)
    >>> c1 = Tone(b1, 5000, mul=0.2).out()
    >>> c2 = Tone(b2, 3000, mul=0.2).out()
    >>> c3 = Tone(b3, 1500, mul=0.2).out()
    >>> c4 = Tone(b4, 500, mul=0.2).out()

    """

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

    def setDelay(self, x):
        """
        Replace the `delay` attribute.

        :Args:

            x: float or PyoObject
                New `delay` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        :Args:

            x: float or PyoObject
                New `feedback` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.001, self._maxdelay, "log", "delay", self._delay),
            SLMap(0.0, 1.0, "lin", "feedback", self._feedback),
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
    def delay(self):
        """float or PyoObject. Delay time in seconds."""
        return self._delay

    @delay.setter
    def delay(self, x):
        self.setDelay(x)

    @property
    def feedback(self):
        """float or PyoObject. Amount of output signal sent back into the delay line."""
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Allpass2(PyoObject):
    """
    Second-order phase shifter allpass.

    This kind of filter is used in phaser implementation. The signal
    of this filter, when added to original sound, creates a notch in
    the spectrum at frequencies that are in phase opposition.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        bw: float or PyoObject, optional
            Bandwidth of the filter in Hertz. Defaults to 100.

    >>> s = Server().boot()
    >>> s.start()
    >>> # 3 STAGES PHASER
    >>> a = BrownNoise(.025).mix(2).out()
    >>> blfo = Sine(freq=.1, mul=250, add=500)
    >>> b = Allpass2(a, freq=blfo, bw=125).out()
    >>> clfo = Sine(freq=.14, mul=500, add=1000)
    >>> c = Allpass2(b, freq=clfo, bw=350).out()
    >>> dlfo = Sine(freq=.17, mul=1000, add=2500)
    >>> d = Allpass2(c, freq=dlfo, bw=800).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        """
        Replace the `bw` attribute.

        :Args:

            x: float or PyoObject
                New `bw` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBw(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(10, 1000, "lin", "bw", self._bw), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        """float or PyoObject. Bandwidth of the filter."""
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)


class Phaser(PyoObject):
    """
    Multi-stages second-order phase shifter allpass filters.

    Phaser implements `num` number of second-order allpass filters.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the first notch. Defaults to 1000.
        spread: float or PyoObject, optional
            Spreading factor for upper notch frequencies. Defaults to 1.1.
        q: float or PyoObject, optional
            Q of the filter as center frequency / bandwidth. Defaults to 10.
        feedback: float or PyoObject, optional
            Amount of output signal which is fed back into the input of the
            allpass chain. Defaults to 0.
        num: int, optional
            The number of allpass stages in series. Defines the number of
            notches in the spectrum. Defaults to 8.

            Available at initialization only.

    >>> s = Server().boot()
    >>> s.start()
    >>> fade = Fader(fadein=.1, mul=.07).play()
    >>> a = Noise(fade).mix(2).out()
    >>> lf1 = Sine(freq=[.1, .15], mul=100, add=250)
    >>> lf2 = Sine(freq=[.18, .15], mul=.4, add=1.5)
    >>> b = Phaser(a, freq=lf1, spread=lf2, q=1, num=20, mul=.5).out(0)

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpread(self, x):
        """
        Replace the `spread` attribute.

        :Args:

            x: float or PyoObject
                New `spread` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        :Args:

            x: float or PyoObject
                New `feedback` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(20, 2000, "log", "freq", self._freq),
            SLMap(0.5, 2, "lin", "spread", self._spread),
            SLMap(0.5, 100, "log", "q", self._q),
            SLMap(0, 1, "lin", "feedback", self._feedback),
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
    def freq(self):
        """float or PyoObject. Center frequency of the first notch."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def spread(self):
        """float or PyoObject. Spreading factor for upper notch frequencies."""
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def q(self):
        """float or PyoObject. Q factor of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def feedback(self):
        """float or PyoObject. Feedback factor of the filter."""
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Vocoder(PyoObject):
    """
    Applies the spectral envelope of a first sound to the spectrum of a second sound.

    The vocoder is an analysis/synthesis system, historically used to reproduce
    human speech. In the encoder, the first input (spectral envelope) is passed
    through a multiband filter, each band is passed through an envelope follower,
    and the control signals from the envelope followers are communicated to the
    decoder. The decoder applies these (amplitude) control signals to corresponding
    filters modifying the second source (exciter).

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Spectral envelope. Gives the spectral properties of the bank of filters.
            For best results, this signal must have a dynamic spectrum, both for
            amplitudes and frequencies.
        input2: PyoObject
            Exciter. Spectrum to filter. For best results, this signal must have a
            broadband spectrum with few amplitude variations.
        freq: float or PyoObject, optional
            Center frequency of the first band. This is the base frequency used to
            compute the upper bands. Defaults to 60.
        spread: float or PyoObject, optional
            Spreading factor for upper band frequencies. Each band is
            `freq * pow(order, spread)`, where order is the harmonic rank of the band.
            Defaults to 1.25.
        q: float or PyoObject, optional
            Q of the filters as `center frequency / bandwidth`. Higher values imply
            more resonance around the center frequency. Defaults to 20.
        slope: float or PyoObject, optional
            Time response of the envelope follower. Lower values mean smoother changes,
            while higher values mean a better time accuracy. Defaults to 0.5.
        stages: int, optional
            The number of bands in the filter bank. Defines the number of notches in
            the spectrum. Defaults to 24.

    .. note::

        Altough parameters can be audio signals, values are sampled only four times
        per buffer size. To avoid artefacts, it is recommended to keep variations
        at low rate (< 20 Hz).

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH+'/transparent.aif', loop=True)
    >>> ex = BrownNoise(0.5)
    >>> voc = Vocoder(sf, ex, freq=80, spread=1.2, q=20, slope=0.5)
    >>> out = voc.mix(2).out()

    """

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
        """
        Replace the `input` attribute.

        :Args:

            x: PyoObject
                The spectral envelope.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        """
        Replace the `input2` attribute.

        :Args:

            x: PyoObject
                The exciter.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSpread(self, x):
        """
        Replace the `spread` attribute.

        :Args:

            x: float or PyoObject
                New `spread` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        """
        Replace the `slope` attribute.

        :Args:

            x: float or PyoObject
                New `slope` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        """
        Replace the `stages` attribute.

        :Args:

            x: int
                New `stages` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(10, 1000, "log", "freq", self._freq),
            SLMap(0.25, 2, "lin", "spread", self._spread),
            SLMap(0.5, 200, "log", "q", self._q),
            SLMap(0, 1, "lin", "slope", self._slope),
            SLMap(2, 64, "lin", "stages", self._stages, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. The spectral envelope."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        """PyoObject. The exciter."""
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the first band."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def spread(self):
        """float or PyoObject. Spreading factor for upper band frequencies."""
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def q(self):
        """float or PyoObject. Q factor of the filters."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def slope(self):
        """float or PyoObject. Time response of the envelope follower."""
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)

    @property
    def stages(self):
        """int. The number of bands in the filter bank."""
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class IRWinSinc(PyoObject):
    """
    Windowed-sinc filter using circular convolution.

    IRWinSinc uses circular convolution to implement standard filters like
    lowpass, highpass, bandreject and bandpass with very flat passband
    response and sharp roll-off. User can defined the length, in samples,
    of the impulse response, also known as the filter kernel.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Frequency cutoff for lowpass and highpass and center frequency for
            bandjrect and bandpass filters, expressed in Hz. Defaults to 1000.
        bw: float or PyoObject, optional
            Bandwidth, expressed in Hertz, for bandreject and bandpass filters.
            Defaults to 500.
        type: int, optional
            Filter type. Four possible values :
                0. lowpass (default)
                1. highpass
                2. bandreject
                3. bandpass
        order: int {even number}, optional
            Length, in samples, of the filter kernel used for convolution. Available
            at initialization time only. Defaults to 256.

            This value must be even. Higher is the order and sharper is the roll-off
            of the filter, but it is also more expensive to compute.

    .. note::

        Convolution is very expensive to compute, so the length of the impulse
        response (the `order` parameter) must be kept very short to run in real time.

        Note that although `freq` and `bw` can be PyoObjects, the impulse response of
        the filter is only updated once per buffer size.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.3)
    >>> lfr = Sine([.15, .2], mul=2000, add=3500)
    >>> lbw = Sine([.3, .25], mul=1000, add=1500)
    >>> b = IRWinSinc(a, freq=lfr, bw=lbw, type=3, order=256).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        """
        Replace the `bw` attribute.

        :Args:

            x: float or PyoObject
                New `bw` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBandwidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute.
                0. lowpass
                1. highpass
                2. bandreject
                3. bandpass

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(20.0, 10000.0, "log", "bw", self._bw),
            SLMap(0, 3, "lin", "type", self._type, res="int", dataOnly=True),
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
    def freq(self):
        """float or PyoObject. Cutoff or Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        """float or PyoObject. Bandwidth for bandreject and bandpass filters."""
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def type(self):
        """int. Filter type {0 = lowpass, 1 = highpass, 2 = bandreject, 3 = bandpass}."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class IRAverage(PyoObject):
    """
    Moving average filter using circular convolution.

    IRAverage uses circular convolution to implement an average filter. This
    filter is designed to reduce the noise in the input signal while keeping
    as much as possible the step response of the original signal. User can
    defined the length, in samples, of the impulse response, also known as
    the filter kernel. This controls the ratio of removed noise vs the fidelity
    of the original step response.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        order: int {even number}, optional
            Length, in samples, of the filter kernel used for convolution. Available
            at initialization time only. Defaults to 256.

            This value must be even. A high order will reduced more noise and will
            have a higher damping effect on the step response, but it is also more
            expensive to compute.

    .. note::

        Convolution is very expensive to compute, so the length of the impulse
        response (the `order` parameter) must be kept very short to run in real time.

    >>> s = Server().boot()
    >>> s.start()
    >>> nz = Noise(.05)
    >>> a = Sine([300, 400], mul=.25, add=nz)
    >>> b = IRAverage(a, order=128).out()

    """

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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class IRPulse(PyoObject):
    """
    Comb-like filter using circular convolution.

    IRPulse uses circular convolution to implement standard comb-like
    filters consisting of an harmonic series with fundamental `freq` and
    a comb filter with the first notch at `bw` frequency. The `type`
    parameter defines variations of this pattern. User can defined the length,
    in samples, of the impulse response, also known as the filter kernel.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Fundamental frequency of the spikes in the filter's spectrum, expressed
            in Hertz. Defaults to 500.
        bw: float or PyoObject, optional
            Frequency, expressed in Hertz, of the first notch in the comb filtering.
            Defaults to 2500.
        type: int, optional
            Filter type. Four possible values :
                0. Pulse & comb (default)
                1. Pulse & comb & lowpass
                2. Pulse (odd harmonics) & comb
                3. Pulse (odd harmonics) & comb & lowpass
        order: int {even number}, optional
            Length, in samples, of the filter kernel used for convolution. Available
            at initialization time only. Defaults to 256.

            This value must be even. Higher is the order and sharper is the roll-off
            of the filter, but it is also more expensive to compute.

    .. note::

        Convolution is very expensive to compute, so the length of the impulse
        response (the `order` parameter) must be kept very short to run in real time.

        Note that although `freq` and `bw` can be PyoObjects, the impulse response of
        the filter is only updated once per buffer size.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.5)
    >>> b = IRPulse(a, freq=[245, 250], bw=2500, type=3, order=256).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBw(self, x):
        """
        Replace the `bw` attribute.

        :Args:

            x: float or PyoObject
                New `bw` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bw = x
        x, lmax = convertArgsToLists(x)
        [obj.setBandwidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute. Filter type. Four possible values:
                    0. Pulse & comb (default)
                    1. Pulse & comb & lowpass
                    2. Pulse (odd harmonics) & comb
                    3. Pulse (odd harmonics) & comb & lowpass

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(20.0, 10000.0, "log", "bw", self._bw),
            SLMap(0, 3, "lin", "type", self._type, res="int", dataOnly=True),
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
    def freq(self):
        """float or PyoObject. Cutoff or Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def bw(self):
        """float or PyoObject. Bandwidth for bandreject and bandpass filters."""
        return self._bw

    @bw.setter
    def bw(self, x):
        self.setBw(x)

    @property
    def type(self):
        """int. Filter type {0 = pulse, 1 = pulse_lp, 2 = pulse_odd, 3 = pulse_odd_lp}."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class IRFM(PyoObject):
    """
    Filters a signal with a frequency modulation spectrum using circular convolution.

    IRFM uses circular convolution to implement filtering with a frequency
    modulation spectrum. User can defined the length, in samples, of the
    impulse response, also known as the filter kernel. The higher the `order`,
    the narrower the bandwidth around each of the FM components.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        carrier: float or PyoObject, optional
            Carrier frequency in cycles per second. Defaults to 1000.
        ratio: float or PyoObject, optional
            A factor that, when multiplied by the `carrier` parameter,
            gives the modulator frequency. Defaults to 0.5.
        index: float or PyoObject, optional
            The modulation index. This value multiplied by the modulator
            frequency gives the modulator amplitude. Defaults to 3.
        order: int {even number}, optional
            Length, in samples, of the filter kernel used for convolution.
            Available at initialization time only. Defaults to 256.

            This value must be even. Higher is the order and sharper is the
            roll-off of the filter, but it is also more expensive to compute.

    .. note::

        Convolution is very expensive to compute, so the length of the impulse
        response (the `order` parameter) must be kept very short to run in real time.

        Note that although `carrier`, `ratio` and `index` can be PyoObjects, the
        impulse response of the filter is only updated once per buffer size.

    >>> s = Server().boot()
    >>> s.start()
    >>> nz = Noise(.7)
    >>> lf = Sine(freq=[.2, .25], mul=.125, add=.5)
    >>> b = IRFM(nz, carrier=3500, ratio=lf, index=3, order=256).out()

    """

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

    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.

        :Args:

            x: float or PyoObject
                New `carrier` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: float or PyoObject
                New `index` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(20.0, 10000.0, "log", "carrier", self._carrier),
            SLMap(0.01, 4.0, "log", "ratio", self._ratio),
            SLMap(0.0, 20.0, "lin", "index", self._index),
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
    def carrier(self):
        """float or PyoObject. Carrier frequency in Hz."""
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/carrier ratio."""
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def index(self):
        """float or PyoObject. Modulation index."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class SVF(PyoObject):
    """
    Fourth-order state variable filter allowing continuous change of the filter type.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff or center frequency of the filter. Defaults to 1000.

            Because this filter becomes unstable at higher frequencies,
            the `freq` parameter is limited to one-sixth of the sampling rate.
        q: float or PyoObject, optional
            Q of the filter, defined (for bandpass filters) as freq/bandwidth.
            Should be between 0.5 and 50. Defaults to 1.
        type: float or PyoObject, optional
            This value, in the range 0 to 1, controls the filter type crossfade
            on the continuum lowpass-bandpass-highpass.

            - 0.0 = lowpass (default)
            - 0.5 = bandpass
            - 1.0 = highpass

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.2)
    >>> lf1 = Sine([.22,.25]).range(500, 2000)
    >>> lf2 = Sine([.17,.15]).range(1, 4)
    >>> lf3 = Sine([.23,.2]).range(0, 1)
    >>> b = SVF(a, freq=lf1, q=lf2, type=lf3).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute. Limited in the upper bound to
        one-sixth of the sampling rate.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 0.5 and 50.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute. Must be in the range 0 to 1.

        This value allows the filter type to sweep from a lowpass (0)
        to a bandpass (0.5) and then, from the bandpass to a highpass (1).

        :Args:

            x: float or PyoObject
                New `type` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(20, 7350, "log", "freq", self._freq),
            SLMap(0.5, 10, "log", "q", self._q),
            SLMap(0, 1, "lin", "type", self._type),
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
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def type(self):
        """float or PyoObject. Crossfade between filter types."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class SVF2(PyoObject):
    """
    Second-order state variable filter allowing continuous change of the filter type.

    This 2-pole multimode filter is described in the book "The Art of VA Filter Design"
    by Vadim Zavalishin (version 2.1.0 when this object was created).

    Several filter types are available with continuous change between them. The default
    order (controlled with the `type` argument) is:

        - lowpass
        - bandpass
        - highpass
        - highshelf
        - bandshelf
        - lowshelf
        - notch
        - peak
        - allpass
        - unit gain bandpass

    The filter types order can be changed with the `setOrder` method. The first filter type
    is always copied at the end of the order list so we can create a glitch-free loop of
    filter types with a Phasor given as `type` argument. Ex.:

        >>> lfo = Phasor(freq=.5, mul=3)
        >>> svf2 = SVF2(Noise(.25), freq=2500, q=2, shelf=-6, type=lfo).out()
        >>> svf2.order = [2, 1, 0] # highpass -> bandpass -> lowpass -> highpass

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff or center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined (for bandpass filters) as freq/bandwidth.
            Should be between 0.5 and 50. Defaults to 1.
        shelf: float or PyoObject, optional
            Gain, between -24 dB and 24 dB, used by shelving filters. Defaults to -3.
        type: float or PyoObject, optional
            This value, in the range 0 to 10, controls the filter type crossfade
            on the continuum defined by the filter types order. The default order
            (which can be changed with the `setOrder` method) is:

            - 0 = lowpass
            - 1 = bandpass
            - 2 = highpass
            - 3 = highshelf
            - 4 = bandshelf
            - 5 = lowshelf
            - 6 = notch
            - 7 = peak
            - 8 = allpass
            - 9 = unit gain bandpass

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(0.25).mix(2)
    >>> tp = Phasor(.25, mul=10)
    >>> fr = Sine(.3).range(500, 5000)
    >>> svf2 = SVF2(n, freq=fr, q=3, shelf=-6, type=tp).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 0.5 and 50.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setShelf(self, x):
        """
        Replace the `shelf` attribute. Should be between -24 dB and 24 dB.

        :Args:

            x: float or PyoObject
                New `shelf` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._shelf = x
        x, lmax = convertArgsToLists(x)
        [obj.setShelf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute. Must be in the range 0 to 10.

        This value allows the filter type to crossfade between several filter types.

        :Args:

            x: float or PyoObject
                New `type` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOrder(self, x):
        """
        Change the filter types ordering.

        The ordering is a list of one to ten integers indicating the filter
        types order used by the `type` argument to crossfade between them.
        Types, as integer, are:

            - 0 = lowpass
            - 1 = bandpass
            - 2 = highpass
            - 3 = highshelf
            - 4 = bandshelf
            - 5 = lowshelf
            - 6 = notch
            - 7 = peak
            - 8 = allpass
            - 9 = unit gain bandpass

        :Args:

            x: list of ints
                New types ordering.

        """
        pyoArgsAssert(self, "l", x)
        self._order = x
        [obj.setOrder(x) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(0.5, 20, "log", "q", self._q),
            SLMap(-24, 24, "lin", "shelf", self._shelf),
            SLMap(0, 10, "lin", "type", self._type),
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
    def freq(self):
        """float or PyoObject. Cutoff or center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def shelf(self):
        """float or PyoObject. Shelving gain of the filter."""
        return self._shelf

    @shelf.setter
    def shelf(self, x):
        self.setShelf(x)

    @property
    def type(self):
        """float or PyoObject. Crossfade between filter types."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)

    @property
    def order(self):
        """list of ints. Filter types ordering."""
        return self._order

    @order.setter
    def order(self, x):
        self.setOrder(x)


class Average(PyoObject):
    """
    Moving average filter.

    As the name implies, the moving average filter operates by averaging a number
    of points from the input signal to produce each point in the output signal.
    In spite of its simplicity, the moving average filter is optimal for
    a common task: reducing random noise while retaining a sharp step response.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        size: int, optional
            Filter kernel size, which is the number of samples used in the
            moving average. Default to 10.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.025)
    >>> b = Sine(250, mul=0.3)
    >>> c = Average(a+b, size=100).out()

    """

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

    def setSize(self, x):
        """
        Replace the `size` attribute.

        :Args:

            x: int
                New `size` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(2, 256, "lin", "size", self._size, res="int", dataOnly=True), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        """int. Filter kernel size in samples."""
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)


class Reson(PyoObject):
    """
    A second-order resonant bandpass filter.

    Reson implements a classic resonant bandpass filter, as described in:

    Dodge, C., Jerse, T., "Computer Music, Synthesis, Composition and Performance".

    Reson uses less CPU than the equivalent filter with a Biquad object.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = Reson(a, freq=lfo, q=5).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class Resonx(PyoObject):
    """
    A multi-stages second-order resonant bandpass filter.

    Resonx implements a stack of the classic resonant bandpass filter, as described in:

    Dodge, C., Jerse, T., "Computer Music, Synthesis, Composition and Performance".

    Resonx is equivalent to a filter consisting of more layers of Reson
    with the same arguments, serially connected. It is faster than using
    a large number of instances of the Reson object, it uses less memory
    and allows filters with sharper cutoff.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.
        stages: int, optional
            The number of filtering stages in the filter stack. Defaults to 4.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = Resonx(a, freq=lfo, q=5).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStages(self, x):
        """
        Replace the `stages` attribute.

        :Args:

            x: int
                New `stages` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._stages = x
        x, lmax = convertArgsToLists(x)
        [obj.setStages(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapQ(self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)

    @property
    def stages(self):
        """int. The number of filtering stages."""
        return self._stages

    @stages.setter
    def stages(self, x):
        self.setStages(x)


class ButLP(PyoObject):
    """
    A second-order Butterworth lowpass filter.

    ButLP implements a second-order IIR Butterworth lowpass filter,
    which has a maximally flat passband and a very good precision and
    stopband attenuation.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 1000.

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.3)
    >>> lf = Sine(freq=.2, mul=800, add=1000)
    >>> f = ButLP(n, lf).mix(2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class ButHP(PyoObject):
    """
    A second-order Butterworth highpass filter.

    ButHP implements a second-order IIR Butterworth highpass filter,
    which has a maximally flat passband and a very good precision and
    stopband attenuation.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 1000.

    >>> s = Server().boot()
    >>> s.start()
    >>> n = Noise(.2)
    >>> lf = Sine(freq=.2, mul=1500, add=2500)
    >>> f = ButHP(n, lf).mix(2).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class ButBP(PyoObject):
    """
    A second-order Butterworth bandpass filter.

    ButBP implements a second-order IIR Butterworth bandpass filter,
    which has a maximally flat passband and a very good precision and
    stopband attenuation.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.7)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = ButBP(a, freq=lfo, q=5).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(1, 100, "log", "q", self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class ButBR(PyoObject):
    """
    A second-order Butterworth band-reject filter.

    ButBR implements a second-order IIR Butterworth band-reject filter,
    which has a maximally flat passband and a very good precision and
    stopband attenuation.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        q: float or PyoObject, optional
            Q of the filter, defined as freq/bandwidth.
            Should be between 1 and 500. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.2)
    >>> lfo = Sine(freq=[.2, .25], mul=1000, add=1500)
    >>> f = ButBR(a, freq=lfo, q=1).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQ(self, x):
        """
        Replace the `q` attribute. Should be between 1 and 500.

        :Args:

            x: float or PyoObject
                New `q` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._q = x
        x, lmax = convertArgsToLists(x)
        [obj.setQ(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(1, 100, "log", "q", self._q), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def q(self):
        """float or PyoObject. Q of the filter."""
        return self._q

    @q.setter
    def q(self, x):
        self.setQ(x)


class MoogLP(PyoObject):
    """
    A fourth-order resonant lowpass filter.

    Digital approximation of the Moog VCF, giving a decay of 24dB/oct.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter. Defaults to 1000.
        res: float or PyoObject, optional
            Amount of Resonance of the filter, usually between 0 (no resonance)
            and 1 (medium resonance). Self-oscillation occurs when the
            resonance is >= 1. Can go up to 10. Defaults to 0.

    >>> s = Server().boot()
    >>> s.start()
    >>> ph = Phasor(40)
    >>> sqr = Round(ph, add=-0.5)
    >>> lfo = Sine(freq=[.4, .5], mul=2000, add=2500)
    >>> fil = MoogLP(sqr, freq=lfo, res=1.25).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRes(self, x):
        """
        Replace the `res` attribute.

        :Args:

            x: float or PyoObject
                New `res` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._res = x
        x, lmax = convertArgsToLists(x)
        [obj.setRes(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0.0, 1.0, "lin", "res", self._res), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Cutoff frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def res(self):
        """float or PyoObject. Amount of resonance of the filter."""
        return self._res

    @res.setter
    def res(self, x):
        self.setRes(x)


class ComplexRes(PyoObject):
    """
    Complex one-pole resonator filter.

    ComplexRes implements a resonator derived from a complex
    multiplication, which is very similar to a digital filter.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Center frequency of the filter. Defaults to 1000.
        decay: float or PyoObject, optional
            Decay time, in seconds, for the filter's response.
            Defaults to 0.25.

    >>> s = Server().boot()
    >>> s.start()
    >>> env = HannTable()
    >>> trigs = Metro(.2, poly=4).play()
    >>> amp = TrigEnv(trigs, table=env, dur=0.005, mul=2)
    >>> im = Noise(mul=amp)
    >>> res = ComplexRes(im, freq=[950,530,780,1490], decay=1).out()

    """

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

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.

        :Args:

            x: float or PyoObject
                New `decay` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0.0001, 10, "log", "decay", self._decay), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def freq(self):
        """float or PyoObject. Center frequency of the filter."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def decay(self):
        """float or PyoObject. Decay time of the filter's response."""
        return self._decay

    @decay.setter
    def decay(self, x):
        self.setDecay(x)
