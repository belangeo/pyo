"""
Synthesis generators.

Set of synthesis generators that can be used as sources of a signal
processing chain or as parameter's modifiers.

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

######################################################################
### Sources
######################################################################
class Sine(PyoObject):
    """
    A simple sine wave oscillator.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        phase: float or PyoObject, optional
            Phase of sampling, expressed as a fraction of a cycle (0 to 1).
            Defaults to 0.

    .. seealso::

        :py:class:`Osc`, :py:class:`Phasor`

    >>> s = Server().boot()
    >>> s.start()
    >>> sine = Sine(freq=[400,500], mul=.2).out()

    """

    def __init__(self, freq=1000, phase=0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, phase, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._phase = phase
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Sine_base(wrap(freq, i), wrap(phase, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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

    def setPhase(self, x):
        """
        Replace the `phase` attribute.

        :Args:

            x: float or PyoObject
                new `phase` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling."""
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)


class FastSine(PyoObject):
    """
    A fast sine wave approximation using the formula of a parabola.

    This object implements two sin approximations that are even faster
    than a linearly interpolated table lookup. With `quality` set to 1,
    the approximation is more accurate but also more expensive on the CPU
    (still cheaper than a Sine object). With `quality` = 0, the algorithm
    gives a worse approximation of the sin function but it is very fast
    (and well suitable for generating LFO).

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        initphase: float, optional
            Initial phase of the oscillator, between 0 and 1. Available
            at initialization time only. Defaults to 0.
        quality: int, optional
            Sets the approximation quality. 1 is more accurate but also
            more expensive on the CPU. 0 is a cheaper algorithm but is
            very fast. Defaults to 1.

    .. seealso::

        :py:class:`Sine`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = FastSine(freq=[4,5], quality=0, mul=0.02, add=1)
    >>> syn = FastSine(freq=500*lfo, quality=1, mul=0.4).out()

    """

    def __init__(self, freq=1000, initphase=0.0, quality=1, mul=1, add=0):
        pyoArgsAssert(self, "OniOO", freq, initphase, quality, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._initphase = initphase
        self._quality = quality
        freq, initphase, quality, mul, add, lmax = convertArgsToLists(freq, initphase, quality, mul, add)
        self._base_objs = [
            FastSine_base(wrap(freq, i), wrap(initphase, i), wrap(quality, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

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

    def setQuality(self, x):
        """
        Replace the `quality` attribute.

        :Args:

            x: int {0 or 1}
                new `quality` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._quality = x
        x, lmax = convertArgsToLists(x)
        [obj.setQuality(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(0, 1, "lin", "quality", self._quality, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def quality(self):
        """int. Quality of the sin approximation."""
        return self._quality

    @quality.setter
    def quality(self, x):
        self.setQuality(x)


class SineLoop(PyoObject):
    """
    A simple sine wave oscillator with feedback.

    The oscillator output, multiplied by `feedback`, is added to the position
    increment and can be used to control the brightness of the oscillator.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        feedback: float or PyoObject, optional
            Amount of the output signal added to position increment, between 0 and 1.
            Controls the brightness. Defaults to 0.

    .. seealso::

        :py:class:`Sine`, :py:class:`OscLoop`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(.25, 0, .1, .1)
    >>> a = SineLoop(freq=[400,500], feedback=lfo, mul=.2).out()

    """

    def __init__(self, freq=1000, feedback=0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, feedback, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._feedback = feedback
        freq, feedback, mul, add, lmax = convertArgsToLists(freq, feedback, mul, add)
        self._base_objs = [
            SineLoop_base(wrap(freq, i), wrap(feedback, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        :Args:

            x: float or PyoObject
                new `feedback` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0, 1, "lin", "feedback", self._feedback), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def feedback(self):
        """float or PyoObject. Brightness control."""
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Phasor(PyoObject):
    """
    A simple phase incrementor.

    Output is a periodic ramp from 0 to 1.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 100.
        phase: float or PyoObject, optional
            Phase of sampling, expressed as a fraction of a cycle (0 to 1).
            Defaults to 0.

    .. seealso::

        :py:class:`Osc`, :py:class:`Sine`

    >>> s = Server().boot()
    >>> s.start()
    >>> f = Phasor(freq=[1, 1.5], mul=1000, add=500)
    >>> sine = Sine(freq=f, mul=.2).out()

    """

    def __init__(self, freq=100, phase=0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, phase, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._phase = phase
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Phasor_base(wrap(freq, i), wrap(phase, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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

    def setPhase(self, x):
        """
        Replace the `phase` attribute.

        :Args:

            x: float or PyoObject
                new `phase` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling."""
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)


class Input(PyoObject):
    """
    Read from a numbered channel in an external audio signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        chnl: int, optional
            Input channel to read from. Defaults to 0.

    .. note::

        Requires that the Server's duplex mode is set to 1.

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input(chnl=0, mul=.7)
    >>> b = Delay(a, delay=.25, feedback=.5, mul=.5).out()

    """

    def __init__(self, chnl=0, mul=1, add=0):
        pyoArgsAssert(self, "iOO", chnl, mul, add)
        PyoObject.__init__(self, mul, add)
        self._chnl = chnl
        chnl, mul, add, lmax = convertArgsToLists(chnl, mul, add)
        self._base_objs = [Input_base(wrap(chnl, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)


class Noise(PyoObject):
    """
    A white noise generator.

    :Parent: :py:class:`PyoObject`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.1).mix(2).out()

    """

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        self._type = 0
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [Noise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setType(self, x):
        """
        Sets the generation algorithm.

        :Args:

            x: int, {0, 1}
                0 uses the system rand() method to generate number. Used as default.
                1 uses a simple linear congruential generator, cheaper than rand().

        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def type(self):
        """int {0, 1}. Sets the generation algorithm."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class PinkNoise(PyoObject):
    """
    A pink noise generator.

    Paul Kellet's implementation of pink noise generator.

    This is an approximation to a -10dB/decade filter using a weighted sum
    of first order filters. It is accurate to within +/-0.05dB above 9.2Hz
    (44100Hz sampling rate).

    :Parent: :py:class:`PyoObject`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = PinkNoise(.1).mix(2).out()

    """

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [PinkNoise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)


class BrownNoise(PyoObject):
    """
    A brown noise generator.

    The spectrum of a brown noise has a power density which decreases 6 dB
    per octave with increasing frequency (density proportional to 1/f^2).

    :Parent: :py:class:`PyoObject`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = BrownNoise(.1).mix(2).out()

    """

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [BrownNoise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)


class FM(PyoObject):
    """
    A simple frequency modulation generator.

    Implements frequency modulation synthesis based on Chowning's algorithm.

    :Parent: :py:class:`PyoObject`

    :Args:

        carrier: float or PyoObject, optional
            Carrier frequency in cycles per second. Defaults to 100.
        ratio: float or PyoObject, optional
            A factor that, when multiplied by the `carrier` parameter,
            gives the modulator frequency. Defaults to 0.5.
        index: float or PyoObject, optional
            The modulation index. This value multiplied by the modulator
            frequency gives the modulator amplitude. Defaults to 5.

    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,3), (20,40), (300,10), (1000,5), (8191,3)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, table=ind, dur=4)
    >>> f = FM(carrier=[251,250], ratio=[.2498,.2503], index=tr, mul=.2).out()

    """

    def __init__(self, carrier=100, ratio=0.5, index=5, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", carrier, ratio, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._carrier = carrier
        self._ratio = ratio
        self._index = index
        carrier, ratio, index, mul, add, lmax = convertArgsToLists(carrier, ratio, index, mul, add)
        self._base_objs = [
            Fm_base(wrap(carrier, i), wrap(ratio, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.

        :Args:

            x: float or PyoObject
                new `carrier` attribute.

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
                new `ratio` attribute.

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
                new `index` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(10, 500, "lin", "carrier", self._carrier),
            SLMap(0.01, 10, "lin", "ratio", self._ratio),
            SLMap(0, 20, "lin", "index", self._index),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def carrier(self):
        """float or PyoObject. Carrier frequency in cycles per second."""
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/Carrier ratio."""
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


class CrossFM(PyoObject):
    """
    Cross frequency modulation generator.

    Frequency modulation synthesis where the output of both oscillators
    modulates the frequency of the other one.

    :Parent: :py:class:`PyoObject`

    :Args:

        carrier: float or PyoObject, optional
            Carrier frequency in cycles per second. Defaults to 100.
        ratio: float or PyoObject, optional
            A factor that, when multiplied by the `carrier` parameter,
            gives the modulator frequency. Defaults to 0.5.
        ind1: float or PyoObject, optional
            The carrier index. This value multiplied by the carrier
            frequency gives the carrier amplitude for modulating the
            modulation oscillator frequency.
            Defaults to 2.
        ind1: float or PyoObject, optional
            The modulation index. This value multiplied by the modulation
            frequency gives the modulation amplitude for modulating the
            carrier oscillator frequency.
            Defaults to 2.

    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,20), (200,5), (1000,2), (8191,1)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, table=ind, dur=4)
    >>> f = CrossFM(carrier=[250.5,250], ratio=[.2499,.2502], ind1=tr, ind2=tr, mul=.2).out()

    """

    def __init__(self, carrier=100, ratio=0.5, ind1=2, ind2=2, mul=1, add=0):
        pyoArgsAssert(self, "OOOOOO", carrier, ratio, ind1, ind2, mul, add)
        PyoObject.__init__(self, mul, add)
        self._carrier = carrier
        self._ratio = ratio
        self._ind1 = ind1
        self._ind2 = ind2
        carrier, ratio, ind1, ind2, mul, add, lmax = convertArgsToLists(carrier, ratio, ind1, ind2, mul, add)
        self._base_objs = [
            CrossFm_base(wrap(carrier, i), wrap(ratio, i), wrap(ind1, i), wrap(ind2, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.

        :Args:

            x: float or PyoObject
                new `carrier` attribute.

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
                new `ratio` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInd1(self, x):
        """
        Replace the `ind1` attribute.

        :Args:

            x: float or PyoObject
                new `ind1` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._ind1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInd2(self, x):
        """
        Replace the `ind2` attribute.

        :Args:

            x: float or PyoObject
                new `ind2` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._ind2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(10, 500, "lin", "carrier", self._carrier),
            SLMap(0.01, 10, "lin", "ratio", self._ratio),
            SLMap(0, 20, "lin", "ind1", self._ind1),
            SLMap(0, 20, "lin", "ind2", self._ind2),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def carrier(self):
        """float or PyoObject. Carrier frequency in cycles per second."""
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/Carrier ratio."""
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def ind1(self):
        """float or PyoObject. Carrier index."""
        return self._ind1

    @ind1.setter
    def ind1(self, x):
        self.setInd1(x)

    @property
    def ind2(self):
        """float or PyoObject. Modulation index."""
        return self._ind2

    @ind2.setter
    def ind2(self, x):
        self.setInd2(x)


class Blit(PyoObject):
    """
    Band limited impulse train synthesis.

    Impulse train generator with control over the number of harmonics
    in the spectrum, which gives oscillators with very low aliasing.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 100.
        harms: float or PyoObject, optional
            Number of harmonics in the generated spectrum. Defaults to 40.

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(freq=4, mul=.02, add=1)
    >>> lf2 = Sine(freq=.25, mul=10, add=30)
    >>> a = Blit(freq=[100, 99.7]*lfo, harms=lf2, mul=.3).out()

    """

    def __init__(self, freq=100, harms=40, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, harms, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._harms = harms
        freq, harms, mul, add, lmax = convertArgsToLists(freq, harms, mul, add)
        self._base_objs = [Blit_base(wrap(freq, i), wrap(harms, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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

    def setHarms(self, x):
        """
        Replace the `harms` attribute.

        :Args:

            x: float or PyoObject
                new `harms` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._harms = x
        x, lmax = convertArgsToLists(x)
        [obj.setHarms(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(1, 5000, "log", "freq", self._freq),
            SLMap(2, 100, "lin", "harms", self._harms),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def harms(self):
        """float or PyoObject. Number of harmonics."""
        return self._harms

    @harms.setter
    def harms(self, x):
        self.setHarms(x)


class Rossler(PyoObject):
    """
    Chaotic attractor for the Rossler system.

    The Rossler attractor is a system of three non-linear ordinary differential
    equations. These differential equations define a continuous-time dynamical
    system that exhibits chaotic dynamics associated with the fractal properties
    of the attractor.

    :Parent: :py:class:`PyoObject`

    :Args:

        pitch: float or PyoObject, optional
            Controls the speed, in the range 0 -> 1, of the variations. With values
            below 0.2, this object can be used as a low frequency oscillator (LFO)
            and above 0.2, it will generate a broad spectrum noise with harmonic peaks.
            Defaults to 0.25.
        chaos: float or PyoObject, optional
            Controls the chaotic behavior, in the range 0 -> 1, of the oscillator.
            0 means nearly periodic while 1 is totally chaotic. Defaults to 0.5.
        stereo, boolean, optional
            If True, 2 streams will be generated, one with the X variable signal of
            the algorithm and a second composed of the Y variable signal of the algorithm.
            These two signal are strongly related in their frequency spectrum but
            the Y signal is out-of-phase by approximatly 180 degrees. Useful to create
            alternating LFOs. Available at initialization only. Defaults to False.

    .. seealso::

        :py:class:`Lorenz`, :py:class:`ChenLee`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Rossler(pitch=.003, stereo=True, mul=.2, add=.2)
    >>> b = Rossler(pitch=[.5,.48], mul=a).out()

    """

    def __init__(self, pitch=0.25, chaos=0.5, stereo=False, mul=1, add=0):
        pyoArgsAssert(self, "OObOO", pitch, chaos, stereo, mul, add)
        PyoObject.__init__(self, mul, add)
        self._pitch = pitch
        self._chaos = chaos
        self._stereo = stereo
        pitch, chaos, mul, add, lmax = convertArgsToLists(pitch, chaos, mul, add)
        self._base_objs = []
        self._alt_objs = []
        for i in range(lmax):
            self._base_objs.append(Rossler_base(wrap(pitch, i), wrap(chaos, i), wrap(mul, i), wrap(add, i)))
            if self._stereo:
                self._base_objs.append(RosslerAlt_base(self._base_objs[-1], wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        """
        Replace the `chaos` attribute.

        :Args:

            x: float or PyoObject
                new `chaos` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "pitch", self._pitch),
            SLMap(0.0, 1.0, "lin", "chaos", self._chaos),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def pitch(self):
        """float or PyoObject. Speed of the variations."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        """float or PyoObject. Chaotic behavior."""
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class Lorenz(PyoObject):
    """
    Chaotic attractor for the Lorenz system.

    The Lorenz attractor is a system of three non-linear ordinary differential
    equations. These differential equations define a continuous-time dynamical
    system that exhibits chaotic dynamics associated with the fractal properties
    of the attractor.

    :Parent: :py:class:`PyoObject`

    :Args:

        pitch: float or PyoObject, optional
            Controls the speed, in the range 0 -> 1, of the variations. With values
            below 0.2, this object can be used as a low frequency oscillator (LFO)
            and above 0.2, it will generate a broad spectrum noise with harmonic peaks.
            Defaults to 0.25.
        chaos: float or PyoObject, optional
            Controls the chaotic behavior, in the range 0 -> 1, of the oscillator.
            0 means nearly periodic while 1 is totally chaotic. Defaults to 0.5
        stereo, boolean, optional
            If True, 2 streams will be generated, one with the X variable signal of
            the algorithm and a second composed of the Y variable signal of the algorithm.
            These two signal are strongly related in their frequency spectrum but
            the Y signal is out-of-phase by approximatly 180 degrees. Useful to create
            alternating LFOs. Available at initialization only. Defaults to False.

    .. seealso::

        :py:class:`Rossler`, :py:class:`ChenLee`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Lorenz(pitch=.003, stereo=True, mul=.2, add=.2)
    >>> b = Lorenz(pitch=[.4,.38], mul=a).out()

    """

    def __init__(self, pitch=0.25, chaos=0.5, stereo=False, mul=1, add=0):
        pyoArgsAssert(self, "OObOO", pitch, chaos, stereo, mul, add)
        PyoObject.__init__(self, mul, add)
        self._pitch = pitch
        self._chaos = chaos
        self._stereo = stereo
        pitch, chaos, mul, add, lmax = convertArgsToLists(pitch, chaos, mul, add)
        self._base_objs = []
        self._alt_objs = []
        for i in range(lmax):
            self._base_objs.append(Lorenz_base(wrap(pitch, i), wrap(chaos, i), wrap(mul, i), wrap(add, i)))
            if self._stereo:
                self._base_objs.append(LorenzAlt_base(self._base_objs[-1], wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        """
        Replace the `chaos` attribute.

        :Args:

            x: float or PyoObject
                new `chaos` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "pitch", self._pitch),
            SLMap(0.0, 1.0, "lin", "chaos", self._chaos),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def pitch(self):
        """float or PyoObject. Speed of the variations."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        """float or PyoObject. Chaotic behavior."""
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class ChenLee(PyoObject):
    """
    Chaotic attractor for the Chen-Lee system.

    The ChenLee attractor is a system of three non-linear ordinary differential
    equations. These differential equations define a continuous-time dynamical
    system that exhibits chaotic dynamics associated with the fractal properties
    of the attractor.

    :Parent: :py:class:`PyoObject`

    :Args:

        pitch: float or PyoObject, optional
            Controls the speed, in the range 0 -> 1, of the variations. With values
            below 0.2, this object can be used as a low frequency oscillator (LFO)
            and above 0.2, it will generate a broad spectrum noise with harmonic peaks.
            Defaults to 0.25.
        chaos: float or PyoObject, optional
            Controls the chaotic behavior, in the range 0 -> 1, of the oscillator.
            0 means nearly periodic while 1 is totally chaotic. Defaults to 0.5
        stereo, boolean, optional
            If True, 2 streams will be generated, one with the X variable signal of
            the algorithm and a second composed of the Y variable signal of the algorithm.
            These two signal are strongly related in their frequency spectrum but
            the Y signal is slightly out-of-phase. Useful to create alternating LFOs.
            Available at initialization only. Defaults to False.

    .. seealso::

        :py:class:`Rossler`, :py:class:`Lorenz`

    >>> s = Server().boot()
    >>> s.start()
    >>> a = ChenLee(pitch=.001, chaos=0.2, stereo=True, mul=.5, add=.5)
    >>> b = ChenLee(pitch=1, chaos=a, mul=0.5).out()

    """

    def __init__(self, pitch=0.25, chaos=0.5, stereo=False, mul=1, add=0):
        pyoArgsAssert(self, "OObOO", pitch, chaos, stereo, mul, add)
        PyoObject.__init__(self, mul, add)
        self._pitch = pitch
        self._chaos = chaos
        self._stereo = stereo
        pitch, chaos, mul, add, lmax = convertArgsToLists(pitch, chaos, mul, add)
        self._base_objs = []
        self._alt_objs = []
        for i in range(lmax):
            self._base_objs.append(ChenLee_base(wrap(pitch, i), wrap(chaos, i), wrap(mul, i), wrap(add, i)))
            if self._stereo:
                self._base_objs.append(ChenLeeAlt_base(self._base_objs[-1], wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        """
        Replace the `chaos` attribute.

        :Args:

            x: float or PyoObject
                new `chaos` attribute. {0. -> 1.}

        """
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "pitch", self._pitch),
            SLMap(0.0, 1.0, "lin", "chaos", self._chaos),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def pitch(self):
        """float or PyoObject. Speed of the variations."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        """float or PyoObject. Chaotic behavior."""
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class LFO(PyoObject):
    """
    Band-limited Low Frequency Oscillator with different wave shapes.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Oscillator frequency in cycles per second. The frequency is
            internally clamped between 0.00001 and sr/4. Defaults to 100.
        sharp: float or PyoObject, optional
            Sharpness factor between 0 and 1. Sharper waveform results
            in more harmonics in the spectrum. Defaults to 0.5.
        type: int, optional
            Waveform type. eight possible values :
                0. Saw up (default)
                1. Saw down
                2. Square
                3. Triangle
                4. Pulse
                5. Bipolar pulse
                6. Sample and hold
                7. Modulated Sine

    >>> s = Server().boot()
    >>> s.start()
    >>> lf = Sine([.31,.34], mul=15, add=20)
    >>> lf2 = LFO([.43,.41], sharp=.7, type=2, mul=.4, add=.4)
    >>> a = LFO(freq=lf, sharp=lf2, type=7, mul=100, add=300)
    >>> b = SineLoop(freq=a, feedback=0.12, mul=.2).out()

    """

    def __init__(self, freq=100, sharp=0.5, type=0, mul=1, add=0):
        pyoArgsAssert(self, "OOiOO", freq, sharp, type, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._sharp = sharp
        self._type = type
        freq, sharp, type, mul, add, lmax = convertArgsToLists(freq, sharp, type, mul, add)
        self._base_objs = [
            LFO_base(wrap(freq, i), wrap(sharp, i), wrap(type, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        :Args:

            x: float or PyoObject
                New `freq` attribute, in cycles per seconds.

        """
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSharp(self, x):
        """
        Replace the `sharp` attribute.

        :Args:

            x: float or PyoObject
                New `sharp` attribute, in the range 0 -> 1.

        """
        pyoArgsAssert(self, "O", x)
        self._sharp = x
        x, lmax = convertArgsToLists(x)
        [obj.setSharp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        :Args:

            x: int
                New `type` attribute. Choices are :
                    0. Saw up
                    1. Saw down
                    2. Square
                    3. Triangle
                    4. Pulse
                    5. Bipolar pulse
                    6. Sample and hold
                    7. Modulated Sine


        """
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.1, self.getSamplingRate() * 0.25, "log", "freq", self._freq),
            SLMap(0.0, 1.0, "lin", "sharp", self._sharp),
            SLMap(0, 7, "lin", "type", self._type, "int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Oscillator frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def sharp(self):
        """float or PyoObject. Sharpness factor {0 -> 1}."""
        return self._sharp

    @sharp.setter
    def sharp(self, x):
        self.setSharp(x)

    @property
    def type(self):
        """int. Waveform type."""
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class SumOsc(PyoObject):
    """
    Discrete summation formulae to produce complex spectra.

    This object implements a discrete summation formulae taken from
    the paper 'The synthesis of complex audio spectra by means of
    discrete summation formulae' by James A. Moorer. The formulae
    used is of this form:

    (sin(theta) - a * sin(theta - beta)) / (1 + a**2 - 2 * a * cos(beta))

    where 'theta' and 'beta' are periodic functions and 'a' is
    the modulation index, providing control over the damping of
    the partials.

    The resulting sound is related to the family of modulation
    techniques but this formulae express 'one-sided' spectra,
    useful to avoid aliasing from the negative frequencies.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Base frequency in cycles per second. Defaults to 100.
        ratio: float or PyoObject, optional
            A factor used to stretch or compress the partial serie by
            manipulating the frequency of the modulation oscillator.
            Integer ratios give harmonic spectra. Defaults to 0.5.
        index: float or PyoObject, optional
            Damping of successive partials, between 0 and 1. With a
            value of 0.5, each partial is 6dB lower than the previous
            partial. Defaults to 0.5.

    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,.3), (20,.85), (300,.7), (1000,.5), (8191,.3)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, table=ind, dur=4)
    >>> f = SumOsc(freq=[301,300], ratio=[.2498,.2503], index=tr, mul=.2).out()

    """

    def __init__(self, freq=100, ratio=0.5, index=0.5, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", freq, ratio, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._ratio = ratio
        self._index = index
        freq, ratio, index, mul, add, lmax = convertArgsToLists(freq, ratio, index, mul, add)
        self._base_objs = [
            SumOsc_base(wrap(freq, i), wrap(ratio, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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

    def setRatio(self, x):
        """
        Replace the `ratio` attribute.

        :Args:

            x: float or PyoObject
                new `ratio` attribute.

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
                new `index` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(10, 500, "lin", "freq", self._freq),
            SLMap(0.01, 10, "lin", "ratio", self._ratio),
            SLMap(0, 1, "lin", "index", self._index),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Base frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def ratio(self):
        """float or PyoObject. Base/modulator frequency ratio."""
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def index(self):
        """float or PyoObject. Index, high frequency damping."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class SuperSaw(PyoObject):
    """
    Roland JP-8000 Supersaw emulator.

    This object implements an emulation of the Roland JP-8000 Supersaw algorithm.
    The shape of the waveform is produced from 7 sawtooth oscillators detuned
    against each other over a period of time. It allows control over the depth
    of the detuning and the balance between central and sideband oscillators.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 100.
        detune: float or PyoObject, optional
            Depth of the detuning, between 0 and 1. 0 means all oscillators are
            tuned to the same frequency and 1 means sideband oscillators are at
            maximum detuning regarding the central frequency. Defaults to 0.5.
        bal: float or PyoObject, optional
            Balance between central oscillator and sideband oscillators. A value
            of 0 outputs only the central oscillator while a value of 1 gives a
            mix of all oscillators with the central one lower than the sidebands.
            Defaults to 0.7.

    .. seealso::

        :py:class:`Phasor`, :py:class:`SineLoop`

    >>> s = Server().boot()
    >>> s.start()
    >>> lfd = Sine([.4,.3], mul=.2, add=.5)
    >>> a = SuperSaw(freq=[49,50], detune=lfd, bal=0.7, mul=0.2).out()

    """

    def __init__(self, freq=100, detune=0.5, bal=0.7, mul=1, add=0):
        pyoArgsAssert(self, "OOOOO", freq, detune, bal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._detune = detune
        self._bal = bal
        freq, detune, bal, mul, add, lmax = convertArgsToLists(freq, detune, bal, mul, add)
        self._base_objs = [
            SuperSaw_base(wrap(freq, i), wrap(detune, i), wrap(bal, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

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

    def setDetune(self, x):
        """
        Replace the `detune` attribute.

        :Args:

            x: float or PyoObject
                new `detune` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._detune = x
        x, lmax = convertArgsToLists(x)
        [obj.setDetune(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        """
        Replace the `bal` attribute.

        :Args:

            x: float or PyoObject
                new `bal` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setBal(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(0, 1, "lin", "detune", self._detune),
            SLMap(0, 1, "lin", "bal", self._bal),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def detune(self):
        """float or PyoObject. Depth of the detuning."""
        return self._detune

    @detune.setter
    def detune(self, x):
        self.setDetune(x)

    @property
    def bal(self):
        """float or PyoObject. Balance between central and sideband oscillators."""
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class RCOsc(PyoObject):
    """
    Waveform aproximation of a RC circuit.

    A RC circuit is a capacitor and a resistor in series, giving a logarithmic
    growth followed by an exponential decay.

    :Parent: :py:class:`PyoObject`

    :Args:

        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 100.
        sharp: float or PyoObject, optional
            Slope of the attack and decay of the waveform, between 0 and 1.
            A value of 0 gives a triangular waveform and 1 gives almost a
            square wave. Defaults to 0.25.

    .. seealso::

        :py:class:`Osc`, :py:class:`LFO`, :py:class:`SineLoop`, :py:class:`SumOsc`

    >>> s = Server().boot()
    >>> s.start()
    >>> fr = RCOsc(freq=[.48,.5], sharp=.2, mul=300, add=600)
    >>> a = RCOsc(freq=fr, sharp=.1, mul=.2).out()

    """

    def __init__(self, freq=100, sharp=0.25, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, sharp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._sharp = sharp
        freq, sharp, mul, add, lmax = convertArgsToLists(freq, sharp, mul, add)
        self._base_objs = [RCOsc_base(wrap(freq, i), wrap(sharp, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

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

    def setSharp(self, x):
        """
        Replace the `sharp` attribute.

        :Args:

            x: float or PyoObject
                new `sharp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._sharp = x
        x, lmax = convertArgsToLists(x)
        [obj.setSharp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0, 1, "lin", "sharp", self._sharp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def sharp(self):
        """float or PyoObject. Sharpness of the waveform."""
        return self._sharp

    @sharp.setter
    def sharp(self, x):
        self.setSharp(x)
