"""
Set of objects to perform operations on PyoTableObjects.

PyoTableObjects are 1 dimension containers. They can be used to
store audio samples or algorithmic sequences for future uses.

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


class Osc(PyoObject):
    """
    A simple oscillator reading a waveform table.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        phase: float or PyoObject, optional
            Phase of sampling, expressed as a fraction of a cycle (0 to 1).
            Defaults to 0.
        interp: int, optional
            Choice of the interpolation method. Defaults to 2.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic

    .. seealso::

        :py:class:`Phasor`, :py:class:`Sine`

    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111,0,.091])
    >>> a = Osc(table=t, freq=[100,99.2], mul=.2).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                new `interp` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMapPhase(self._phase),
            SLMap(1, 4, "lin", "interp", self._interp, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

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

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class OscLoop(PyoObject):
    """
    A simple oscillator with feedback reading a waveform table.

    OscLoop reads a waveform table with linear interpolation and feedback control.
    The oscillator output, multiplied by `feedback`, is added to the position
    increment and can be used to control the brightness of the oscillator.


    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        feedback: float or PyoObject, optional
            Amount of the output signal added to position increment, between 0 and 1.
            Controls the brightness. Defaults to 0.

    .. seealso::

        :py:class:`Osc`, :py:class:`SineLoop`

    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143])
    >>> lfo = Sine(.5, 0, .05, .05)
    >>> a = OscLoop(table=t, freq=[100,99.3], feedback=lfo, mul=.2).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

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


class OscTrig(PyoObject):
    """
    An oscillator reading a waveform table with sample accurate reset signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        trig: PyoObject
            Trigger signal. Reset the table pointer position to zero on
            each trig.
        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1000.
        phase: float or PyoObject, optional
            Phase of sampling, expressed as a fraction of a cycle (0 to 1).
            Defaults to 0.
        interp: int, optional
            Choice of the interpolation method. Defaults to 2.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic

    .. seealso::

        :py:class:`Osc`, :py:class:`Phasor`, :py:class:`Sine`

    >>> s = Server().boot()
    >>> s.start()
    >>> tab = SndTable(SNDS_PATH+"/transparent.aif")
    >>> tim = Phasor([-0.2,-0.25], mul=tab.getDur()-0.005, add=0.005)
    >>> rst = Metro(tim).play()
    >>> a = OscTrig(tab, rst, freq=tab.getRate(), mul=.4).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTrig(self, x):
        """
        Replace the `trig` attribute.

        :Args:

            x: PyoObject
                new `trig` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._trig = x
        x, lmax = convertArgsToLists(x)
        [obj.setTrig(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                new `interp` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMapPhase(self._phase),
            SLMap(1, 4, "lin", "interp", self._interp, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def trig(self):
        """PyoObject. Trigger signal. Reset pointer position to zero"""
        return self._trig

    @trig.setter
    def trig(self, x):
        self.setTrig(x)

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

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class OscBank(PyoObject):
    """
    Any number of oscillators reading a waveform table.

    OscBank mixes the output of any number of oscillators. The frequencies
    of each oscillator is controlled with two parameters, the base frequency
    `freq` and a coefficient of expansion `spread`. Frequencies are computed
    with the following formula (`n` is the order of the partial):

    f_n = freq + freq * spread * n

    The frequencies and amplitudes can be modulated by two random generators
    with interpolation (each partial have a different set of randoms).

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        freq: float or PyoObject, optional
            Base frequency in cycles per second. Defaults to 100.
        spread: float or PyoObject, optional
            Coefficient of expansion used to compute partial frequencies.
            If `spread` is 0, all partials will be at the base frequency.
            A value of 1 will generate integer harmonics, a value of 2
            will skip even harmonics and non-integer values will generate
            different series of inharmonic frequencies. Defaults to 1.
        slope: float or PyoObject, optional
            specifies the multiplier in the series of amplitude coefficients.
            This is a power series: the nth partial will have an amplitude of
            (slope ** n), i.e. strength values trace an exponential curve.
            Defaults to 1.
        frndf: float or PyoObject, optional
            Frequency, in cycle per second, of the frequency modulations.
            Defaults to 1.
        frnda: float or PyoObject, optional
            Maximum frequency deviation (positive and negative) in portion of
            the partial frequency. A value of 1 means that the frequency can
            drift from 0 Hz to twice the partial frequency. A value of 0
            deactivates the frequency deviations. Defaults to 0.
        arndf: float or PyoObject, optional
            Frequency, in cycle per second, of the amplitude modulations.
            Defaults to 1.
        arnda: float or PyoObject, optional
            Amount of amplitude deviation. 0 deactivates the amplitude
            modulations and 1 gives full amplitude modulations.
            Defaults to 0.
        num: int, optional
            Number of oscillators. Available at initialization only.
            Defaults to 24.
        fjit: boolean, optional
            If True, a small jitter is added to the frequency of each partial.
            For a large number of oscillators and a very small `spread`, the
            periodicity between partial frequencies can cause very strange
            artefact. Adding a jitter breaks the periodicity. Defaults to False.

    .. seealso::

        :py:class:`Osc`

    .. note::

        Altough parameters can be audio signals, values are sampled only once
        per buffer size. To avoid artefacts, it is recommended to keep variations
        at low rate (< 20 Hz).

    >>> s = Server().boot()
    >>> s.start()
    >>> ta = HarmTable([1,.3,.2])
    >>> tb = HarmTable([1])
    >>> f = Fader(fadein=.1).play()
    >>> a = OscBank(ta,100,spread=0,frndf=.25,frnda=.01,num=[10,10],fjit=True,mul=f*0.5).out()
    >>> b = OscBank(tb,250,spread=.25,slope=.8,arndf=4,arnda=1,num=[10,10],mul=f*0.4).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setSpread(self, x):
        """
        Replace the `spread` attribute.

        :Args:

            x: float or PyoObject
                new `spread` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        """
        Replace the `slope` attribute.

        :Args:

            x: float or PyoObject
                new `slope` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFrndf(self, x):
        """
        Replace the `frndf` attribute.

        :Args:

            x: float or PyoObject
                new `frndf` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._frndf = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrndf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFrnda(self, x):
        """
        Replace the `frnda` attribute.

        :Args:

            x: float or PyoObject
                new `frnda` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._frnda = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrnda(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArndf(self, x):
        """
        Replace the `arndf` attribute.

        :Args:

            x: float or PyoObject
                new `arndf` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._arndf = x
        x, lmax = convertArgsToLists(x)
        [obj.setArndf(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setArnda(self, x):
        """
        Replace the `arnda` attribute.

        :Args:

            x: float or PyoObject
                new `arnda` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._arnda = x
        x, lmax = convertArgsToLists(x)
        [obj.setArnda(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFjit(self, x):
        """
        Replace the `fjit` attribute.

        :Args:

            x: boolean
                new `fjit` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._fjit = x
        x, lmax = convertArgsToLists(x)
        [obj.setFjit(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.001, 15000, "log", "freq", self._freq),
            SLMap(0.001, 2, "log", "spread", self._spread),
            SLMap(0, 1, "lin", "slope", self._slope),
            SLMap(0.001, 20, "log", "frndf", self._frndf),
            SLMap(0, 1, "lin", "frnda", self._frnda),
            SLMap(0.001, 20, "log", "arndf", self._arndf),
            SLMap(0, 1, "lin", "arnda", self._arnda),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def spread(self):
        """float or PyoObject. Frequency expansion factor."""
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)

    @property
    def slope(self):
        """float or PyoObject. Multiplier in the series of amplitudes."""
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)

    @property
    def frndf(self):
        """float or PyoObject. Frequency of the frequency modulations."""
        return self._frndf

    @frndf.setter
    def frndf(self, x):
        self.setFrndf(x)

    @property
    def frnda(self):
        """float or PyoObject. Maximum frequency deviation from 0 to 1."""
        return self._frnda

    @frnda.setter
    def frnda(self, x):
        self.setFrnda(x)

    @property
    def arndf(self):
        """float or PyoObject. Frequency of the amplitude modulations."""
        return self._arndf

    @arndf.setter
    def arndf(self, x):
        self.setArndf(x)

    @property
    def arnda(self):
        """float or PyoObject. Amount of amplitude deviation from 0 to 1."""
        return self._arnda

    @arnda.setter
    def arnda(self, x):
        self.setArnda(x)

    @property
    def fjit(self):
        """boolean. Jitter added to the parial frequencies."""
        return self._fjit

    @fjit.setter
    def fjit(self, x):
        self.setFjit(x)


class TableRead(PyoObject):
    """
    Simple waveform table reader.

    Read sampled sound from a table, with optional looping mode.

    The play() method starts the playback and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 1.
        loop: int {0, 1}, optional
            Looping mode, 0 means off, 1 means on.
            Defaults to 0.
        interp: int, optional
            Choice of the interpolation method. Defaults to 2.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic

    .. note::

        TableRead will send a trigger signal at the end of the playback if
        loop is off or any time it wraps around if loop is on. User can
        retreive the trigger streams by calling obj['trig']:

        >>> tabr = TableRead(SNDS_PATH + "/transparent.aif").out()
        >>> trig = TrigRand(tabr['trig'])

    .. seealso::

        :py:class:`Osc`

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH + '/transparent.aif')
    >>> freq = snd.getRate()
    >>> a = TableRead(table=snd, freq=[freq,freq*.99], loop=True, mul=.3).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        :Args:

            x: int {0, 1}
                new `loop` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                new `interp` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setKeepLast(self, x):
        """
        If anything but zero, the object will hold the last value when stopped.

        :Args:

            x: bool
                If 0, the object will be reset to 0 when stopped, otherwise
                it will hold its last value.

        """
        pyoArgsAssert(self, "b", x)
        self._keeplast = x
        x, lmax = convertArgsToLists(x)
        [obj.setKeepLast(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0001, 1000, "log", "freq", self._freq),
            SLMap(0, 1, "lin", "loop", self._loop, res="int", dataOnly=True),
            SLMap(1, 4, "lin", "interp", self._interp, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def loop(self):
        """int. Looping mode."""
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class Pulsar(PyoObject):
    """
    Pulsar synthesis oscillator.

    Pulsar synthesis produces a train of sound particles called pulsars
    that can make rhythms or tones, depending on the fundamental frequency
    of the train. Varying the `frac` parameter changes the portion of the
    period assigned to the waveform and the portion of the period assigned
    to its following silence, but maintain the overall pulsar period. This
    results in an effect much like a sweeping band-pass filter.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        env: PyoTableObject
            Table containing the envelope samples.
        freq: float or PyoObject, optional
            Frequency in cycles per second. Defaults to 100.
        frac: float or PyoObject, optional
            Fraction of the whole period (0 -> 1) given to the waveform.
            The rest will be filled with zeros. Defaults to 0.5.
        phase: float or PyoObject, optional
            Phase of sampling, expressed as a fraction of a cycle (0 to 1).
            Defaults to 0.
        interp: int, optional
            Choice of the interpolation method. Defaults to 2.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic

    .. seealso::

        :py:class:`Osc`

    >>> s = Server().boot()
    >>> s.start()
    >>> w = HarmTable([1,0,.33,0,2,0,.143,0,.111])
    >>> e = HannTable()
    >>> lfo = Sine([.1,.15], mul=.2, add=.5)
    >>> a = Pulsar(table=w, env=e, freq=80, frac=lfo, mul=.08).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.

        :Args:

            x: PyoTableObject
                new `env` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setFrac(self, x):
        """
        Replace the `frac` attribute.

        :Args:

            x: float or PyoObject
                new `frac` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._frac = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrac(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                Choice of the interpolation method.
                    1. no interpolation
                    2. linear
                    3. cosinus
                    4. cubic

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMapFreq(self._freq),
            SLMap(0.0, 1.0, "lin", "frac", self._frac),
            SLMapPhase(self._phase),
            SLMap(1, 4, "lin", "interp", self._interp, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the envelope samples."""
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second."""
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def frac(self):
        """float or PyoObject. Fraction of the period assigned to waveform."""
        return self._frac

    @frac.setter
    def frac(self, x):
        self.setFrac(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling."""
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class Pointer(PyoObject):
    """
    Table reader with control on the pointer position.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        index: PyoObject
            Normalized position in the table between 0 and 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SndTable(SNDS_PATH + '/transparent.aif')
    >>> freq = t.getRate()
    >>> p = Phasor(freq=[freq*0.5, freq*0.45])
    >>> a = Pointer(table=t, index=p, mul=.3).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: PyoObject
                new `index` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        """PyoObject. Index pointer position in the table."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class Pointer2(PyoObject):
    """
    High quality table reader with control on the pointer position.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        index: PyoObject
            Normalized position in the table between 0 and 1.
        interp: int {1, 2, 3, 4}, optional
            Choice of the interpolation method. Defaults to 4.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic
        autosmooth: boolean, optional
            If True, a lowpass filter, following the pitch, is applied on

            the output signal to reduce the quantization noise produced

            by very low transpositions. Defaults to True.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = SndTable(SNDS_PATH + '/transparent.aif')
    >>> freq = t.getRate()
    >>> p = Phasor(freq=[freq*0.5, freq*0.45])
    >>> a = Pointer2(table=t, index=p, mul=.3).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: PyoObject
                new `index` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                new `interp` attribute.
                    1. no interpolation
                    2. linear interpolation
                    3. cosine interpolation
                    4. cubic interpolation (default)

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setAutoSmooth(self, x):
        """
        Replace the `autosmooth` attribute.

        If True, a lowpass filter, following the playback speed, is applied on
        the output signal to reduce the quantization noise produced by very
        low transpositions.

        :Args:

            x: boolean
                new `autosmooth` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._autosmooth = x
        x, lmax = convertArgsToLists(x)
        [obj.setAutoSmooth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        """PyoObject. Index pointer position in the table."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)

    @property
    def autosmooth(self):
        """boolean. Quantization noise filter."""
        return self._autosmooth

    @autosmooth.setter
    def autosmooth(self, x):
        self.setAutoSmooth(x)


class TableIndex(PyoObject):
    """
    Table reader by sample position without interpolation.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the samples.
        index: PyoObject
            Position in the table, as integer audio stream,
            between 0 and table's size - 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> import random
    >>> notes = [midiToHz(random.randint(60,84)) for i in range(10)]
    >>> tab = DataTable(size=10, init=notes)
    >>> ind = RandInt(10, [4,8])
    >>> pit = TableIndex(tab, ind)
    >>> a = SineLoop(freq=pit, feedback = 0.05, mul=.2).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: PyoObject
                new `index` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        """PyoObject. Position in the table."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class Lookup(PyoObject):
    """
    Uses table to do waveshaping on an audio signal.

    Lookup uses a table to apply waveshaping on an input signal
    `index`. The index must be between -1 and 1, it is automatically
    scaled between 0 and len(table)-1 and is used as a position
    pointer in the table.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the transfert function.
        index: PyoObject
            Audio signal, between -1 and 1, internally converted to be
            used as the index position in the table.

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(freq=[.15,.2], mul=.2, add=.25)
    >>> a = Sine(freq=[100,150], mul=lfo)
    >>> t = CosTable([(0,-1),(3072,-0.85),(4096,0),(5520,.85),(8192,1)])
    >>> b = Lookup(table=t, index=a, mul=.5-lfo).out()

    """

    def __init__(self, table, index, mul=1, add=0):
        pyoArgsAssert(self, "toOO", table, index, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        self._index = index
        table, index, mul, add, lmax = convertArgsToLists(table, index, mul, add)
        self._base_objs = [Lookup_base(wrap(table, i), wrap(index, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: PyoObject
                new `index` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the transfert function."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def index(self):
        """PyoObject. Audio input used as the table index."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class TableRec(PyoObject):
    """
    TableRec is for writing samples into a previously created table.

    See `NewTable` to create an empty table.

    The play method is not called at the object creation time. It starts
    the recording into the table until the table is full. Calling the
    play method again restarts the recording and overwrites previously
    recorded samples. The stop method stops the recording. Otherwise, the
    default behaviour is to record through the end of the table.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the table.
        table: PyoTableObject
            The table where to write samples.
        fadetime: float, optional
            Fade time at the beginning and the end of the recording
            in seconds. Defaults to 0.

    .. note::

        The out() method is bypassed. TableRec returns no signal.

        TableRec has no `mul` and `add` attributes.

        TableRec will send a trigger signal at the end of the recording.
        User can retrieve the trigger streams by calling obj['trig']. In
        this example, the recorded table will be read automatically after
        a recording:

        >>> a = Input(0)
        >>> t = NewTable(length=1, chnls=1)
        >>> rec = TableRec(a, table=t, fadetime=0.01)
        >>> tr = TrigEnv(rec['trig'], table=t, dur=1).out()

        obj['time'] outputs an audio stream of the current recording time,
        in samples.

    .. seealso::

        :py:class:`NewTable`, :py:class:`TrigTableRec`

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = NewTable(length=2, chnls=1)
    >>> a = Input(0)
    >>> b = TableRec(a, t, .01)
    >>> amp = Iter(b["trig"], [.5])
    >>> freq = t.getRate()
    >>> c = Osc(t, [freq, freq*.99], mul=amp).out()
    >>> # to record in the empty table, call:
    >>> # b.play()

    """

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

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to write in the table."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        """NewTable. The table where to write samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class TableWrite(PyoObject):
    """
    TableWrite writes samples into a previously created table.

    See `NewTable` to create an empty table.

    TableWrite takes samples from its `input` stream and writes
    them at a normalized or raw position given by the `pos` stream.
    Position must be an audio stream, ie. a PyoObject. This
    object allows fast recording of values coming from an X-Y
    pad into a table object.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the table.
        pos: PyoObject
            Audio signal specifying the position where to write the
            `input` samples. It is a normalized position (in the range
            0 to 1) in mode=0 or the raw position (in samples) for any
            other value of mode.
        table: PyoTableObject
            The table where to write samples.
        mode: int, optional
            Sets the writing pointer mode. If 0, the position must be
            normalized between 0 (beginning of the table) and 1 (end
            of the table). For any other value, the position must be
            in samples between 0 and the length of the table. Available
            at initialization time only.
        maxwindow: int optional
            Maximum length, in samples, of the interpolated window when
            the position is moving fast. Useful to avoid interpolation
            over the entire table if using a circular writing position.
            Available at initialization time only. Defaults to 1024.

    .. note::

        The out() method is bypassed. TableWrite returns no signal.

        TableWrite has no `mul` and `add` attributes.

    .. seealso::

        :py:class:`NewTable`, :py:class:`TableRec`

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> tab = NewTable(8192/s.getSamplingRate())
    >>> tab.view()
    >>> pos = Phasor(.5)
    >>> val = Sine(.25)
    >>> rec = TableWrite(val, pos, tab)
    >>> pat = Pattern(tab.refreshView, 0.05).play()

    """

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

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.

        :Args:

            x: PyoObject
                new `pos` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to write in the table."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        """NewTable. The table where to write samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def pos(self):
        """PyoObject. Normalized position, as audio stream, where to write samples."""
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)


class TableMorph(PyoObject):
    """
    Morphs between multiple PyoTableObjects.

    Uses an index into a list of PyoTableObjects to morph between adjacent
    tables in the list. The resulting morphed function is written into the
    `table` object at the beginning of each buffer size. The tables in the
    list and the resulting table must be equal in size.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Morphing index between 0 and 1. 0 is the first table in the list
            and 1 is the last.
        table: PyoTableObject
            The table where to write morphed waveform.
        sources: list of PyoTableObject
            List of tables to interpolate from.

    .. note::

        The smaller size among the output table and the two current tables to
        interpolate is used to determine the number of samples to process. Using
        tables with different size can produce strange behaviours.

        The out() method is bypassed. TableMorph returns no signal.

        TableMorph has no `mul` and `add` attributes.

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t1 = HarmTable([1,.5,.33,.25,.2,.167,.143,.125,.111,.1,.091])
    >>> t2 = HarmTable([1,0,.33,0,.2,0,.143,0,.111,0,.091])
    >>> t3 = NewTable(length=8192./s.getSamplingRate(), chnls=1)
    >>> lfo = Sine(.25, 0, .5, .5)
    >>> mor = TableMorph(lfo, t3, [t1,t2])
    >>> osc = Osc(t3, freq=[199.5,200], mul=.08).out()

    """

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

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSources(self, x):
        """
         Replace the `sources` attribute.

        :Args:

            x: list of PyoTableObject
                new `sources` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._sources = x
        self._base_sources = [source[0] for source in x]
        [obj.setSources(self._base_sources) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Morphing index between 0 and 1."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        """NewTable. The table where to write samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def sources(self):
        """list of PyoTableObject. List of tables to interpolate from."""
        return self._sources

    @sources.setter
    def sources(self, x):
        self.setSources(x)


class Granulator(PyoObject):
    """
    Granular synthesis generator.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        env: PyoTableObject
            Table containing the grain envelope.
        pitch: float or PyoObject, optional
            Overall pitch of the granulator. This value transpose the
            pitch of all grains. Defaults to 1.
        pos: float or PyoObject, optional
            Pointer position, in samples, in the waveform table. Each
            grain sampled the current value of this stream at the beginning
            of its envelope and hold it until the end of the grain.
            Defaults to 0.
        dur: float or PyoObject, optional
            Duration, in seconds, of the grain. Each grain sampled the
            current value of this stream at the beginning of its envelope
            and hold it until the end of the grain. Defaults to 0.1.
        grains: int, optional
            Number of grains. Defaults to 8.
        basedur: float, optional
            Base duration used to calculate the speed of the pointer to
            read the grain at its original pitch. By changing the value of
            the `dur` parameter, transposition per grain can be generated.
            Defaults to 0.1.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH + "/transparent.aif")
    >>> env = HannTable()
    >>> pos = Phasor(snd.getRate()*.25, 0, snd.getSize())
    >>> dur = Noise(.001, .1)
    >>> g = Granulator(snd, env, [1, 1.001], pos, dur, 24, mul=.1).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.

        :Args:

            x: PyoTableObject
                new `env` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.

        :Args:

            x: float or PyoObject
                new `pos` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float or PyoObject
                new `dur` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setGrains(self, x):
        """
        Replace the `grains` attribute.

        :Args:

            x: int
                new `grains` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._grains = x
        x, lmax = convertArgsToLists(x)
        [obj.setGrains(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBaseDur(self, x):
        """
        Replace the `basedur` attribute.

        :Args:

            x: float
                new `basedur` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._basedur = x
        x, lmax = convertArgsToLists(x)
        [obj.setBaseDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.1, 2.0, "lin", "pitch", self._pitch),
            SLMap(0.01, 1.0, "lin", "dur", self._dur),
            SLMap(1, 256, "lin", "grains", self._grains, res="int", dataOnly=True),
            SLMap(0.001, 1, "log", "basedur", self._basedur, dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def pitch(self):
        """float or PyoObject. Overall pitch of the granulator."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def grains(self):
        """int. Number of grains."""
        return self._grains

    @grains.setter
    def grains(self, x):
        self.setGrains(x)

    @property
    def basedur(self):
        """float. Duration to read the grain at its original pitch."""
        return self._basedur

    @basedur.setter
    def basedur(self, x):
        self.setBaseDur(x)


class TrigTableRec(PyoObject):
    """
    TrigTableRec is for writing samples into a previously created table.

    See `NewTable` to create an empty table.

    Each time a "trigger" is received in the `trig` input, TrigTableRec
    starts the recording into the table until the table is full.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the table.
        trig: PyoObject
            Audio signal sending triggers.
        table: PyoTableObject
            The table where to write samples.
        fadetime: float, optional
            Fade time at the beginning and the end of the recording
            in seconds. Defaults to 0.

    .. note::

        The out() method is bypassed. TrigTableRec returns no signal.

        TrigTableRec has no `mul` and `add` attributes.

        TrigTableRec will send a trigger signal at the end of the recording.
        User can retrieve the trigger streams by calling obj['trig'].

        obj['time'] outputs an audio stream of the current recording time,
        in samples.

    .. seealso::

        :py:class:`NewTable`, :py:class:`TableRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SNDS_PATH + '/transparent.aif'
    >>> dur = sndinfo(snd)[1]
    >>> t = NewTable(length=dur)
    >>> src = SfPlayer(snd, mul=.3).out()
    >>> trec = TrigTableRec(src, trig=Trig().play(), table=t)
    >>> rep = TrigEnv(trec["trig"], table=t, dur=dur).out(1)

    """

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

    def setTrig(self, x, fadetime=0.05):
        """
        Replace the `trig` attribute.

        :Args:

            x: PyoObject
                New trigger signal.
            fadetime: float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._trig = x
        self._in_fader2.setInput(x, fadetime)

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to write in the table."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def trig(self):
        """PyoObject. Audio signal sending triggers."""
        return self._trig

    @trig.setter
    def trig(self, x):
        self.setTrig(x)

    @property
    def table(self):
        """NewTable. The table where to write samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class Looper(PyoObject):
    """
    Crossfading looper.

    Looper reads audio from a PyoTableObject and plays it back in a loop with
    user-defined pitch, start time, duration and crossfade time. The `mode`
    argument allows the user to choose different looping modes.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        pitch: float or PyoObject, optional
            Transposition factor. 1 is normal pitch, 0.5 is one octave lower, 2 is
            one octave higher. Negative values are not allowed. Defaults to 1.
        start: float or PyoObject, optional
            Starting point, in seconds, of the loop, updated only once per loop cycle.
            Defaults to 0.
        dur: float or PyoObject, optional
            Duration, in seconds, of the loop, updated only once per loop cycle.
            Defaults to 1.
        xfade: float or PyoObject {0 -> 50}, optional
            Percent of the loop time used to crossfade readers, updated only once per
            loop cycle and clipped between 0 and 50. Defaults to 20.
        mode: int {0, 1, 2, 3}, optional
            Loop modes. Defaults to 1.
                0. no loop
                1. forward
                2. backward
                3. back-and-forth
        xfadeshape: int {0, 1, 2}, optional
            Crossfade envelope shape. Defaults to 0.
                0. linear
                1. equal power
                2. sigmoid
        startfromloop: boolean, optional
            If True, reading will begin directly at the loop start point. Otherwise, it
            begins at the beginning of the table. Defaults to False.
        interp: int {1, 2, 3, 4}, optional
            Choice of the interpolation method. Defaults to 2.
                1. no interpolation
                2. linear
                3. cosinus
                4. cubic
        autosmooth: boolean, optional
            If True, a lowpass filter, following the pitch, is applied on the output signal
            to reduce the quantization noise produced by very low transpositions.
            Defaults to False.

    .. note::

        Looper will send a trigger signal every new playback starting point
        (i.e. when the object is activated and at the beginning of the crossfade
        of a loop. User can retrieve the trigger stream by calling obj['trig'].

        Looper also outputs a time stream, given the current position of the
        reading pointer, normalized between 0.0 and 1.0 (1.0 means the beginning
        of the crossfade), inside the loop. User can retrieve the time stream
        by calling obj['time']. New in version 0.8.1.

    .. seealso::

        :py:class:`Granulator`, :py:class:`Pointer`

    >>> s = Server().boot()
    >>> s.start()
    >>> tab = SndTable(SNDS_PATH + '/transparent.aif')
    >>> pit = Choice(choice=[.5,.75,1,1.25,1.5], freq=[3,4])
    >>> start = Phasor(freq=.2, mul=tab.getDur())
    >>> dur = Choice(choice=[.0625,.125,.125,.25,.33], freq=4)
    >>> a = Looper(table=tab, pitch=pit, start=start, dur=dur, startfromloop=True, mul=.25).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStart(self, x):
        """
        Replace the `start` attribute.

        :Args:

            x: float or PyoObject
                new `start` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._start = x
        x, lmax = convertArgsToLists(x)
        [obj.setStart(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float or PyoObject
                new `dur` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setXfade(self, x):
        """
        Replace the `xfade` attribute.

        :Args:

            x: float or PyoObject
                new `xfade` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._xfade = x
        x, lmax = convertArgsToLists(x)
        [obj.setXfade(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setXfadeShape(self, x):
        """
        Replace the `xfadeshape` attribute.

        :Args:

            x: int
                new `xfadeshape` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._xfadeshape = x
        x, lmax = convertArgsToLists(x)
        [obj.setXfadeShape(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setStartFromLoop(self, x):
        """
        Replace the `startfromloop` attribute.

        :Args:

            x: boolean
                new `startfromloop` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._startfromloop = x
        x, lmax = convertArgsToLists(x)
        [obj.setStartFromLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMode(self, x):
        """
        Replace the `mode` attribute.

        :Args:

            x: int
                new `mode` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int
                new `interp` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setAutoSmooth(self, x):
        """
        Replace the `autosmooth` attribute.

        :Args:

            x: boolean
                new `autosmooth` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._autosmooth = x
        x, lmax = convertArgsToLists(x)
        [obj.setAutoSmooth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets internal reading pointers to 0.

        """
        [obj.reset() for obj in self._base_objs]

    def loopnow(self):
        """
        Sarts a new loop immediately.

        """
        [obj.loopnow() for obj in self._base_objs]

    def appendFadeTime(self, x):
        """
        Change the position of the crossfade regarding loop duration.

        :Args:

            x: boolean
                If 0 (the default), the crossfade duration lies inside the
                loop duration, producing a loop that is shorter than the
                `dur` value.

                If 1, the crossfade starts after the loop duration, expecting
                samples after the loop to perform the fadeout. This mode gives
                a loop of a length of the `dur` value.

        """
        pyoArgsAssert(self, "b", x)
        self._appendfade = x
        x, lmax = convertArgsToLists(x)
        [obj.appendFadeTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def fadeInSeconds(self, x):
        """
        Change the crossfade time unit (`xfade` unit type).

        :Args:

            x: boolean
                If 0 (the default), the crossfade duration (`xfade` value) is
                a percent of the loop time, between 0 and 50.

                If 1, the crossfade duration (`xfade` value) is set in seconds,
                between 0 and half the loop length.

        """
        pyoArgsAssert(self, "b", x)
        self._fadeinseconds = x
        x, lmax = convertArgsToLists(x)
        [obj.fadeInSeconds(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.1, 2.0, "lin", "pitch", self._pitch),
            SLMap(0.0, self._table.getDur(), "lin", "start", self._start),
            SLMap(0.01, self._table.getDur(), "lin", "dur", self._dur),
            SLMap(1, 50, "lin", "xfade", self._xfade),
            SLMap(0, 3, "lin", "mode", self._mode, res="int", dataOnly=True),
            SLMap(0, 2, "lin", "xfadeshape", self._xfadeshape, res="int", dataOnly=True),
            SLMap(1, 4, "lin", "interp", self._interp, res="int", dataOnly=True),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def pitch(self):
        """float or PyoObject. Transposition factor."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def start(self):
        """float or PyoObject. Loop start position in seconds."""
        return self._start

    @start.setter
    def start(self, x):
        self.setStart(x)

    @property
    def dur(self):
        """float or PyoObject. Loop duration in seconds."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def xfade(self):
        """float or PyoObject. Crossfade duration in percent."""
        return self._xfade

    @xfade.setter
    def xfade(self, x):
        self.setXfade(x)

    @property
    def xfadeshape(self):
        """int. Crossfade envelope."""
        return self._xfadeshape

    @xfadeshape.setter
    def xfadeshape(self, x):
        self.setXfadeShape(x)

    @property
    def startfromloop(self):
        """boolean. Starts from loop point if True, otherwise starts from beginning of the sound."""
        return self._startfromloop

    @startfromloop.setter
    def startfromloop(self, x):
        self.setStartFromLoop(x)

    @property
    def mode(self):
        """int. Looping mode."""
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)

    @property
    def interp(self):
        """int. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)

    @property
    def autosmooth(self):
        """boolean. Activates a lowpass filter applied on output signal."""
        return self._autosmooth

    @autosmooth.setter
    def autosmooth(self, x):
        self.setAutoSmooth(x)


class TablePut(PyoObject):
    """
    Writes values, without repetitions, from an audio stream into a table.

    See :py:class:`DataTable` to create an empty table.

    TablePut takes an audio input and writes values into a table, typically
    a DataTable, but only when value changes. This allow to record only new
    values, without repetitions.

    The play method is not called at the object creation time. It starts
    the recording into the table until the table is full. Calling the
    play method again restarts the recording and overwrites previously
    recorded values. The stop method stops the recording. Otherwise, the
    default behaviour is to record through the end of the table.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the table.
        table: PyoTableObject
            The table where to write values.

    .. note::

        The out() method is bypassed. TablePut returns no signal.

        TablePut has no `mul` and `add` attributes.

        TablePut will send a trigger signal at the end of the recording.
        User can retrieve the trigger streams by calling obj['trig'].

    .. seealso::

        :py:class:`DataTable`, :py:class:`NewTable`, :py:class:`TableRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> t = DataTable(size=16)
    >>> rnd = Choice(list(range(200, 601, 50)), freq=16)
    >>> rec = TablePut(rnd, t).play()
    >>> met = Metro(.125).play()
    >>> ind = Counter(met, max=16)
    >>> fr = TableIndex(t, ind, mul=[1,1.005])
    >>> osc = SineLoop(fr, feedback=.08, mul=.3).out()

    """

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

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to write in the table."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        """DataTable. The table where to write values."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class TableFill(PyoObject):
    """
    Continuously fills a table with incoming samples.

    See :py:class:`DataTable` or :py:class:`NewTable` to create an empty table.

    TableFill takes an audio input and writes values into a PyoTableObject,
    samples by samples. It wraps around the table length when reaching the end
    of the table.

    Calling the play method reset the writing position to 0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Audio signal to write in the table.
        table: PyoTableObject
            The table where to write values.

    .. note::

        The out() method is bypassed. TableFill returns no signal.

        TableFill has no `mul` and `add` attributes.

    .. seealso::

        :py:class:`TableWrite`, :py:class:`TablePut`, :py:class:`TableRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> table = DataTable(s.getBufferSize(), chnls=2)
    >>> sig = Sine([400,500], mul=0.3)
    >>> fill = TableFill(sig, table)
    >>> read = TableRead(table, table.getRate(), loop=True).out()

    """

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

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def getCurrentPos(self, all=False):
        """
        Returns the current pointer position, in samples, in the table.

        :Args:

            all: boolean, optional
                If True, returns the current position of all
                internal objects as a list.

                If False, only the current position of the first object's
                stream will be returned as a float.

        """
        pyoArgsAssert(self, "B", all)
        if not all:
            return self._base_objs[0].getCurrentPos()
        else:
            return [obj.getCurrentPos() for obj in self._base_objs]

    @property
    def input(self):
        """PyoObject. Audio signal to write in the table."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        """PyoTableObject. The table where to write values."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class Granule(PyoObject):
    """
    Another granular synthesis generator.

    As of pyo 0.7.4, users can call the `setSync` method to change the
    granulation mode (either synchronous or asynchronous) of the object.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        env: PyoTableObject
            Table containing the grain envelope.
        dens: float or PyoObject, optional
            Density of grains per second. Defaults to 50.
        pitch: float or PyoObject, optional
            Pitch of the grains. A new grain gets the current value
            of `pitch` as its reading speed. Defaults to 1.
        pos: float or PyoObject, optional
            Pointer position, in samples, in the waveform table. Each
            grain samples the current value of this stream at the beginning
            of its envelope and holds it until the end of the grain.
            Defaults to 0.
        dur: float or PyoObject, optional
            Duration, in seconds, of the grain. Each grain samples the
            current value of this stream at the beginning of its envelope
            and hold it until the end of the grain. Defaults to 0.1.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH+"/transparent.aif")
    >>> end = snd.getSize() - s.getSamplingRate() * 0.2
    >>> env = HannTable()
    >>> pos = Randi(min=0, max=1, freq=[0.25, 0.3], mul=end)
    >>> dns = Randi(min=10, max=30, freq=.1)
    >>> pit = Randi(min=0.99, max=1.01, freq=100)
    >>> grn = Granule(snd, env, dens=dns, pitch=pit, pos=pos, dur=.2, mul=.1).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.

        :Args:

            x: PyoTableObject
                new `env` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDens(self, x):
        """
        Replace the `dens` attribute.

        :Args:

            x: float or PyoObject
                new `dens` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.

        :Args:

            x: float or PyoObject
                new `pos` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float or PyoObject
                new `dur` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSync(self, x):
        """
        Sets the granulation mode, synchronous or asynchronous.

        :Args:

            x: boolean
                True means synchronous granulation (the default)
                while False means asynchronous.

        """
        pyoArgsAssert(self, "b", x)
        self._sync = x
        x, lmax = convertArgsToLists(x)
        [obj.setSync(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(1, 250, "lin", "dens", self._dens),
            SLMap(0.25, 2.0, "lin", "pitch", self._pitch),
            SLMap(0.001, 2.0, "lin", "dur", self._dur),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        """float or PyoObject. Density of grains per second."""
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        """float or PyoObject. Transposition factor of the grain."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)


class TableScale(PyoObject):
    """
    Scales all the values contained in a PyoTableObject.

    TableScale scales the values of `table` argument according
    to `mul` and `add` arguments and writes the new values in
    `outtable`.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the original values.
        outtable: PyoTableObject
            Table where to write the scaled values.

    >>> s = Server().boot()
    >>> s.start()
    >>> t = DataTable(size=12, init=midiToHz(range(48, 72, 2)))
    >>> t2 = DataTable(size=12)
    >>> m = Metro(.2).play()
    >>> c = Counter(m ,min=0, max=12)
    >>> f1 = TableIndex(t, c)
    >>> syn1 = SineLoop(f1, feedback=0.08, mul=0.3).out()
    >>> scl = TableScale(t, t2, mul=1.5)
    >>> f2 = TableIndex(t2, c)
    >>> syn2 = SineLoop(f2, feedback=0.08, mul=0.3).out(1)

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOuttable(self, x):
        """
        Replace the `outtable` attribute.

        :Args:

            x: PyoTableObject
                new `outtable` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._outtable = x
        x, lmax = convertArgsToLists(x)
        [obj.setOuttable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul), SLMap(0, 1, "lin", "add", self._add)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the original values."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def outtable(self):
        """PyoTableObject. Scaled output table."""
        return self._outtable

    @outtable.setter
    def outtable(self, x):
        self.setOuttable(x)


class Particle(PyoObject):
    """
    A full control granular synthesis generator.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        env: PyoTableObject
            Table containing the grain envelope.
        dens: float or PyoObject, optional
            Density of grains per second. Defaults to 50.
        pitch: float or PyoObject, optional
            Pitch of the grains. Each grain samples the current value
            of this stream at the beginning of its envelope and holds
            it until the end of the grain. Defaults to 1.
        pos: float or PyoObject, optional
            Pointer position, in samples, in the waveform table. Each
            grain sampled the current value of this stream at the beginning
            of its envelope and holds it until the end of the grain.
            Defaults to 0.
        dur: float or PyoObject, optional
            Duration, in seconds, of the grain. Each grain samples the
            current value of this stream at the beginning of its envelope
            and holds it until the end of the grain. Defaults to 0.1.
        dev: float or PyoObject, optional
            Maximum deviation of the starting time of the grain, between 0 and
            1 (relative to the current duration of the grain). Each grain samples
            the current value of this stream at the beginning of its envelope
            and holds it until the end of the grain. Defaults to 0.01.
        pan: float or PyoObject, optional
            Panning factor of the grain (if chnls=1, this value is skipped).
            Each grain samples the current value of this stream at the beginning
            of its envelope and holds it until the end of the grain. Defaults to 0.5.
        chnls: integer, optional
            Number of output channels per audio stream (if chnls=2 and a stereo sound
            table is given at the table argument, the objet will create 4 output
            streams, 2 per table channel). Available at initialization only. Defaults to 1.

    .. note::

        Particle object compensate for the difference between sampling rate of the
        loaded sound and the current sampling rate of the Server.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH+"/transparent.aif")
    >>> end = snd.getSize() - s.getSamplingRate() * 0.25
    >>> env = HannTable()
    >>> dns = Randi(min=5, max=100, freq=.1)
    >>> pit = Randh(min=0.99, max=1.01, freq=100)
    >>> pos = Randi(min=0, max=1, freq=0.25, mul=end)
    >>> dur = Randi(min=0.01, max=0.25, freq=0.15)
    >>> dev = Randi(min=0, max=1, freq=0.2)
    >>> pan = Noise(0.5, 0.5)
    >>> grn = Particle(snd, env, dns, pit, pos, dur, dev, pan, chnls=2, mul=.2).out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.

        :Args:

            x: PyoTableObject
                new `env` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDens(self, x):
        """
        Replace the `dens` attribute.

        :Args:

            x: float or PyoObject
                new `dens` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.

        :Args:

            x: float or PyoObject
                new `pos` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float or PyoObject
                new `dur` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDev(self, x):
        """
        Replace the `dev` attribute.

        :Args:

            x: float or PyoObject
                new `dev` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dev = x
        x, lmax = convertArgsToLists(x)
        [obj.setDev(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPan(self, x):
        """
        Replace the `pan` attribute.

        :Args:

            x: float or PyoObject
                new `pan` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        tablesize = self._table.getSize(False)
        self._map_list = [
            SLMap(1, 250, "lin", "dens", self._dens),
            SLMap(0.25, 2.0, "lin", "pitch", self._pitch),
            SLMap(0, tablesize, "lin", "pos", self._pos, res="int"),
            SLMap(0.001, 1.0, "lin", "dur", self._dur),
            SLMap(0.0, 1.0, "lin", "dev", self._dev),
            SLMap(0.0, 1.0, "lin", "pan", self._pan),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        """float or PyoObject. Density of grains per second."""
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        """float or PyoObject. Transposition factor of the grain."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def dev(self):
        """float or PyoObject. Deviation of the starting point of the grain in the table."""
        return self._dev

    @dev.setter
    def dev(self, x):
        self.setDev(x)

    @property
    def pan(self):
        """float or PyoObject.Panning of the grain."""
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)


class Particle2(PyoObject):
    """
    An even more full control granular synthesis generator.

    This granulator object offers all the same controls as the
    Particle object with additionally an independently controllable
    filter per grain. The filters use the same implementation as the
    Biquad object.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.
        env: PyoTableObject
            Table containing the grain envelope.
        dens: float or PyoObject, optional
            Density of grains per second. Defaults to 50.
        pitch: float or PyoObject, optional
            Pitch of the grains. Each grain samples the current value
            of this stream at the beginning of its envelope and holds
            it until the end of the grain. Defaults to 1.
        pos: float or PyoObject, optional
            Pointer position, in samples, in the waveform table. Each
            grain samples the current value of this stream at the beginning
            of its envelope and holds it until the end of the grain.
            Defaults to 0.
        dur: float or PyoObject, optional
            Duration, in seconds, of the grain. Each grain samples the
            current value of this stream at the beginning of its envelope
            and holds it until the end of the grain. Defaults to 0.1.
        dev: float or PyoObject, optional
            Maximum deviation of the starting time of the grain, between 0 and
            1 (relative to the current duration of the grain). Each grain samples
            the current value of this stream at the beginning of its envelope
            and holds it until the end of the grain. Defaults to 0.01.
        pan: float or PyoObject, optional
            Panning factor of the grain (if chnls=1, this value is skipped).
            Each grain samples the current value of this stream at the beginning
            of its envelope and holds it until the end of the grain. Defaults to 0.5.
        filterfreq: float or PyoObject, optional
            Center or cutoff frequency of the grain filter.
            Each grain samples the current value of this stream at the beginning
            of its envelope and hold it until the end of the grain. Defaults to 18000.
        filterq: float or PyoObject, optional
            Q of the grain filter.
            Each grain samples the current value of this stream at the beginning
            of its envelope and hold it until the end of the grain. Defaults to 0.7.
        filtertype: float or PyoObject, optional
            Type of the grain filter.
            Each grain samples the current value of this stream at the beginning
            of its envelope and hold it until the end of the grain. Thw value is
            rounded to the nearest integer. Possible values are:

            0. lowpass (default)
            1. highpass
            2. bandpass
            3. bandstop
            4. allpass

        chnls: integer, optional
            Number of output channels per audio stream (if chnls=2 and a stereo sound
            table is given at the table argument, the objet will create 4 output
            streams, 2 per table channel). Available at initialization only. Defaults to 1.

    .. note::

        Particle object compensate for the difference between sampling rate of the
        loaded sound and the current sampling rate of the Server.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SndTable(SNDS_PATH+"/transparent.aif")
    >>> end = snd.getSize() - s.getSamplingRate() * 0.25
    >>> env = HannTable()
    >>> dns = Randi(min=8, max=24, freq=.1)
    >>> pit = Randh(min=0.49, max=0.51, freq=100)
    >>> pos = Sine(0.05).range(0, end)
    >>> dur = Randi(min=0.05, max=0.15, freq=0.15)
    >>> dev = 0.001
    >>> pan = Noise(0.5, 0.5)
    >>> cf = Sine(0.07).range(75, 125)
    >>> fcf = Choice(list(range(1, 40)), freq=150, mul=cf)
    >>> grn = Particle2(snd, env, dns, pit, pos, dur, dev, pan, fcf, 20, 2, chnls=2, mul=.3)
    >>> grn.out()

    """

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
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setEnv(self, x):
        """
        Replace the `env` attribute.

        :Args:

            x: PyoTableObject
                new `env` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._env = x
        x, lmax = convertArgsToLists(x)
        [obj.setEnv(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDens(self, x):
        """
        Replace the `dens` attribute.

        :Args:

            x: float or PyoObject
                new `dens` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dens = x
        x, lmax = convertArgsToLists(x)
        [obj.setDens(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        :Args:

            x: float or PyoObject
                new `pitch` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPos(self, x):
        """
        Replace the `pos` attribute.

        :Args:

            x: float or PyoObject
                new `pos` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pos = x
        x, lmax = convertArgsToLists(x)
        [obj.setPos(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float or PyoObject
                new `dur` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDev(self, x):
        """
        Replace the `dev` attribute.

        :Args:

            x: float or PyoObject
                new `dev` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._dev = x
        x, lmax = convertArgsToLists(x)
        [obj.setDev(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setPan(self, x):
        """
        Replace the `pan` attribute.

        :Args:

            x: float or PyoObject
                new `pan` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFilterfreq(self, x):
        """
        Replace the `filterfreq` attribute.

        :Args:

            x: float or PyoObject
                new `filterfreq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._filterfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFilterfreq(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFilterq(self, x):
        """
        Replace the `filterq` attribute.

        :Args:

            x: float or PyoObject
                new `filterq` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._filterq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFilterq(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFiltertype(self, x):
        """
        Replace the `filtertype` attribute.

        :Args:

            x: float or PyoObject
                new `filtertype` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._filtertype = x
        x, lmax = convertArgsToLists(x)
        [obj.setFiltertype(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        tablesize = self._table.getSize(False)
        self._map_list = [
            SLMap(1, 250, "lin", "dens", self._dens),
            SLMap(0.25, 2.0, "lin", "pitch", self._pitch),
            SLMap(0, tablesize, "lin", "pos", self._pos, res="int"),
            SLMap(0.001, 1.0, "lin", "dur", self._dur),
            SLMap(0.0, 1.0, "lin", "dev", self._dev),
            SLMap(0.0, 1.0, "lin", "pan", self._pan),
            SLMap(50.0, 18000.0, "log", "filterfreq", self._filterfreq),
            SLMap(0.25, 100.0, "log", "filterq", self._filterq),
            SLMap(0, 4.0, "lin", "filtertype", self._filtertype, res="int"),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)

    @property
    def env(self):
        """PyoTableObject. Table containing the grain envelope."""
        return self._env

    @env.setter
    def env(self, x):
        self.setEnv(x)

    @property
    def dens(self):
        """float or PyoObject. Density of grains per second."""
        return self._dens

    @dens.setter
    def dens(self, x):
        self.setDens(x)

    @property
    def pitch(self):
        """float or PyoObject. Transposition factor of the grain."""
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def pos(self):
        """float or PyoObject. Position of the pointer in the sound table."""
        return self._pos

    @pos.setter
    def pos(self, x):
        self.setPos(x)

    @property
    def dur(self):
        """float or PyoObject. Duration, in seconds, of the grain."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def dev(self):
        """float or PyoObject. Deviation of the starting point of the grain in the table."""
        return self._dev

    @dev.setter
    def dev(self, x):
        self.setDev(x)

    @property
    def pan(self):
        """float or PyoObject.Panning of the grain."""
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)

    @property
    def filterfreq(self):
        """float or PyoObject.Grain's filter center/cutoff frequency."""
        return self._filterfreq

    @filterfreq.setter
    def filterfreq(self, x):
        self.setFilterfreq(x)

    @property
    def filterq(self):
        """float or PyoObject.Grain's filter Q."""
        return self._filterq

    @filterq.setter
    def filterq(self, x):
        self.setFilterq(x)

    @property
    def filtertype(self):
        """float or PyoObject.Grain's filter type."""
        return self._filtertype

    @filtertype.setter
    def filtertype(self, x):
        self.setFiltertype(x)


class TableScan(PyoObject):
    """
    Reads the content of a table in loop, without interpolation.

    A simple table reader, sample by sample, with wrap-around when
    reaching the end of the table.

    :Parent: :py:class:`PyoObject`

    :Args:

        table: PyoTableObject
            Table containing the waveform samples.

    .. seealso::

        :py:class:`Osc`, :py:class:`TableRead`

    >>> s = Server().boot()
    >>> s.start()
    >>> tab = DataTable(s.getBufferSize(), 2)
    >>> sig = Sine([500, 600], mul=0.3)
    >>> fill = TableFill(sig, tab)
    >>> scan = TableScan(tab).out()

    """

    def __init__(self, table, mul=1, add=0):
        pyoArgsAssert(self, "tOO", table, mul, add)
        PyoObject.__init__(self, mul, add)
        self._table = table
        table, mul, add, lmax = convertArgsToLists(table, mul, add)
        self._base_objs = [TableScan_base(wrap(table, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setTable(self, x):
        """
        Replace the `table` attribute.

        :Args:

            x: PyoTableObject
                new `table` attribute.

        """
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        """
        Resets current phase to 0.

        """
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def table(self):
        """PyoTableObject. Table containing the waveform samples."""
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)
