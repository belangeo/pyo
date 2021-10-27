"""
Tools to analyze audio signals.

These objects are designed to retrieve specific informations
from an audio stream. Analysis are sent at audio rate, user
can use them for controlling parameters of others objects.

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
from ._widgets import createSpectrumWindow, createScopeWindow
from .pattern import Pattern


class Follower(PyoObject):
    """
    Envelope follower.

    Output signal is the continuous mean amplitude of an input signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        freq: float or PyoObject, optional
            Cutoff frequency of the filter in hertz. Default to 20.

    .. note::

        The out() method is bypassed. Follower's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`Follower2`, :py:class:`Balance`, :py:class:`RMS`, :py:class:`PeakAmp`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> fol = Follower(sf, freq=30)
    >>> n = Noise(mul=fol).out(1)

    """

    def __init__(self, input, freq=20, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, freq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._in_fader = InputFader(input)
        in_fader, freq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, mul, add)
        self._base_objs = [
            Follower_base(wrap(in_fader, i), wrap(freq, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
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

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1.0, 500.0, "log", "freq", self._freq)]
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


class Follower2(PyoObject):
    """
    Envelope follower with different attack and release times.

    Output signal is the continuous mean amplitude of an input signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        risetime: float or PyoObject, optional
            Time to reach upward value in seconds. Default to 0.01.
        falltime: float or PyoObject, optional
            Time to reach downward value in seconds. Default to 0.1.

    .. note::

        The out() method is bypassed. Follower's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`Follower`, :py:class:`Balance`, :py:class:`RMS`, :py:class:`PeakAmp`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> fol2 = Follower2(sf, risetime=0.002, falltime=.1, mul=.5)
    >>> n = Noise(fol2).out(1)

    """

    def __init__(self, input, risetime=0.01, falltime=0.1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, risetime, falltime, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._risetime = risetime
        self._falltime = falltime
        self._in_fader = InputFader(input)
        in_fader, risetime, falltime, mul, add, lmax = convertArgsToLists(self._in_fader, risetime, falltime, mul, add)
        self._base_objs = [
            Follower2_base(wrap(in_fader, i), wrap(risetime, i), wrap(falltime, i), wrap(mul, i), wrap(add, i))
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

    def setRisetime(self, x):
        """
        Replace the `risetime` attribute.

        :Args:

            x: float or PyoObject
                New `risetime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRisetime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFalltime(self, x):
        """
        Replace the `falltime` attribute.

        :Args:

            x: float or PyoObject
                New `falltime` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFalltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.001, 1.0, "log", "risetime", self._risetime)]
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
        self.setRisetime(x)

    @property
    def falltime(self):
        """float or PyoObject. Time to reach downward value in seconds."""
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFalltime(x)


class ZCross(PyoObject):
    """
    Zero-crossing counter.

    Output signal is the number of zero-crossing occured during each
    buffer size, normalized between 0 and 1.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        thresh: float, optional
            Minimum amplitude difference allowed between adjacent samples
            to be included in the zeros count. Defaults to 0.

    .. note::

        The out() method is bypassed. ZCross's signal can not be sent to
        audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> b = ZCross(a, thresh=.02)
    >>> n = Noise(b).out(1)

    """

    def __init__(self, input, thresh=0.0, mul=1, add=0):
        pyoArgsAssert(self, "onOO", input, thresh, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._thresh = thresh
        self._in_fader = InputFader(input)
        in_fader, thresh, mul, add, lmax = convertArgsToLists(self._in_fader, thresh, mul, add)
        self._base_objs = [
            ZCross_base(wrap(in_fader, i), wrap(thresh, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
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

    def setThresh(self, x):
        """
        Replace the `thresh` attribute.

        :Args:

            x: float
                New amplitude difference threshold.

        """
        pyoArgsAssert(self, "n", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.0, 0.5, "lin", "thresh", self._thresh)]
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
        """float. Amplitude difference threshold."""
        return self._thresh

    @thresh.setter
    def thresh(self, x):
        self.setThresh(x)


class Yin(PyoObject):
    """
    Pitch tracker using the Yin algorithm.

    Pitch tracker using the Yin algorithm based on the implementation in C of aubio.
    This algorithm was developped by A. de Cheveigne and H. Kawahara and published in

    de Cheveigne, A., Kawahara, H. (2002) 'YIN, a fundamental frequency estimator for
    speech and music', J. Acoust. Soc. Am. 111, 1917-1930.

    The audio output of the object is the estimated frequency, in Hz, of the input sound.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        tolerance: float, optional
            Parameter for minima selection, between 0 and 1. Defaults to 0.2.
        minfreq: float, optional
            Minimum estimated frequency in Hz. Frequency below this threshold will
            be ignored. Defaults to 40.
        maxfreq: float, optional
            Maximum estimated frequency in Hz. Frequency above this threshold will
            be ignored. Defaults to 1000.
        cutoff: float, optional
            Cutoff frequency, in Hz, of the lowpass filter applied on the input sound.
            Defaults to 1000.

            The lowpass filter helps the algorithm to detect the fundamental frequency by filtering
            higher harmonics.
        winsize: int, optional
            Size, in samples, of the analysis window. Must be higher that two period
            of the lowest desired frequency.

            Available at initialization time only.  Defaults to 1024.


    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> lfo = Randh(min=100, max=500, freq=3)
    >>> src = SineLoop(freq=lfo, feedback=0.1, mul=.3).out()
    >>> pit = Yin(src, tolerance=0.2, winsize=1024)
    >>> freq = Tone(pit, freq=10)
    >>> # fifth above
    >>> a = LFO(freq*1.5, type=2, mul=0.2).out(1)

    """

    def __init__(self, input, tolerance=0.2, minfreq=40, maxfreq=1000, cutoff=1000, winsize=1024, mul=1, add=0):
        pyoArgsAssert(self, "onnnniOO", input, tolerance, minfreq, maxfreq, cutoff, winsize, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._tolerance = tolerance
        self._minfreq = minfreq
        self._maxfreq = maxfreq
        self._cutoff = cutoff
        self._in_fader = InputFader(input)
        in_fader, tolerance, minfreq, maxfreq, cutoff, winsize, mul, add, lmax = convertArgsToLists(
            self._in_fader, tolerance, minfreq, maxfreq, cutoff, winsize, mul, add
        )
        self._base_objs = [
            Yin_base(
                wrap(in_fader, i),
                wrap(tolerance, i),
                wrap(minfreq, i),
                wrap(maxfreq, i),
                wrap(cutoff, i),
                wrap(winsize, i),
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

    def setTolerance(self, x):
        """
        Replace the `tolerance` attribute.

        :Args:

            x: float
                New parameter for minima selection, between 0 and 1.

        """
        pyoArgsAssert(self, "n", x)
        self._tolerance = x
        x, lmax = convertArgsToLists(x)
        [obj.setTolerance(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMinfreq(self, x):
        """
        Replace the `minfreq` attribute.

        :Args:

            x: float
                New minimum frequency detected.

        """
        pyoArgsAssert(self, "n", x)
        self._minfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinfreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMaxfreq(self, x):
        """
        Replace the `maxfreq` attribute.

        :Args:

            x: float
                New maximum frequency detected.

        """
        pyoArgsAssert(self, "n", x)
        self._maxfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxfreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        """
        Replace the `cutoff` attribute.

        :Args:

            x: float
                New input lowpass filter cutoff frequency.

        """
        pyoArgsAssert(self, "n", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 1, "lin", "tolerance", self._tolerance, dataOnly=True),
            SLMap(20, 400, "log", "minfreq", self._minfreq, dataOnly=True),
            SLMap(500, 5000, "log", "maxfreq", self._maxfreq, dataOnly=True),
            SLMap(200, 15000, "log", "cutoff", self._cutoff, dataOnly=True),
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
    def tolerance(self):
        """float. Parameter for minima selection."""
        return self._tolerance

    @tolerance.setter
    def tolerance(self, x):
        self.setTolerance(x)

    @property
    def minfreq(self):
        """float. Minimum frequency detected."""
        return self._minfreq

    @minfreq.setter
    def minfreq(self, x):
        self.setMinfreq(x)

    @property
    def maxfreq(self):
        """float. Maximum frequency detected."""
        return self._maxfreq

    @maxfreq.setter
    def maxfreq(self, x):
        self.setMaxfreq(x)

    @property
    def cutoff(self):
        """float. Input lowpass filter cutoff frequency."""
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)


class Centroid(PyoObject):
    """
    Computes the spectral centroid of an input signal.

    Output signal is the spectral centroid, in Hz, of the input signal.
    It indicates where the "center of mass" of the spectrum is. Perceptually,
    it has a robust connection with the impression of "brightness" of a sound.

    Centroid does its computation with two overlaps, so a new output value
    comes every half of the FFT window size.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        size: int, optional
            Size, as a power-of-two, of the FFT used to compute the centroid.

            Available at initialization time only.  Defaults to 1024.


    .. note::

        The out() method is bypassed. Centroid's signal can not be sent to
        audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> b = Centroid(a, 1024)
    >>> c = Port(b, 0.05, 0.05)
    >>> d = ButBP(Noise(0.2), freq=c, q=5).out(1)

    """

    def __init__(self, input, size=1024, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, size, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._size = size
        self._in_fader = InputFader(input)
        in_fader, size, mul, add, lmax = convertArgsToLists(self._in_fader, size, mul, add)
        self._base_objs = [
            Centroid_base(wrap(in_fader, i), wrap(size, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
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

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class AttackDetector(PyoObject):
    """
    Audio signal onset detection.

    AttackDetector analyses an audio signal in input and output a trigger each
    time an onset is detected. An onset is a sharp amplitude rising while the
    signal had previously fall below a minimum threshold. Parameters must be
    carefully tuned depending on the nature of the analysed signal and the level
    of the background noise.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        deltime: float, optional
            Delay time, in seconds, between previous and current rms analysis to compare.
            Defaults to 0.005.
        cutoff: float, optional
            Cutoff frequency, in Hz, of the amplitude follower's lowpass filter.
            Defaults to 10.

            Higher values are more responsive and also more likely to give false onsets.
        maxthresh: float, optional
            Attack threshold in positive dB (current rms must be higher than previous
            rms + maxthresh to be reported as an attack). Defaults to 3.0.
        minthresh: float, optional
            Minimum threshold in dB (signal must fall below this threshold to allow
            a new attack to be detected). Defaults to -30.0.
        reltime: float, optional
            Time, in seconds, to wait before reporting a new attack. Defaults to 0.1.


    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input()
    >>> d = AttackDetector(a, deltime=0.005, cutoff=10, maxthresh=4, minthresh=-20, reltime=0.05)
    >>> exc = TrigEnv(d, HannTable(), dur=0.005, mul=BrownNoise(0.3))
    >>> wgs = Waveguide(exc, freq=[100,200.1,300.3,400.5], dur=30).out()

    """

    def __init__(self, input, deltime=0.005, cutoff=10, maxthresh=3, minthresh=-30, reltime=0.1, mul=1, add=0):
        pyoArgsAssert(self, "onnnnnOO", input, deltime, cutoff, maxthresh, minthresh, reltime, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._deltime = deltime
        self._cutoff = cutoff
        self._maxthresh = maxthresh
        self._minthresh = minthresh
        self._reltime = reltime
        self._in_fader = InputFader(input)
        in_fader, deltime, cutoff, maxthresh, minthresh, reltime, mul, add, lmax = convertArgsToLists(
            self._in_fader, deltime, cutoff, maxthresh, minthresh, reltime, mul, add
        )
        self._base_objs = [
            AttackDetector_base(
                wrap(in_fader, i),
                wrap(deltime, i),
                wrap(cutoff, i),
                wrap(maxthresh, i),
                wrap(minthresh, i),
                wrap(reltime, i),
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

    def setDeltime(self, x):
        """
        Replace the `deltime` attribute.

        :Args:

            x: float
                New delay between rms analysis.

        """
        pyoArgsAssert(self, "n", x)
        self._deltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setDeltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        """
        Replace the `cutoff` attribute.

        :Args:

            x: float
                New cutoff for the follower lowpass filter.

        """
        pyoArgsAssert(self, "n", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMaxthresh(self, x):
        """
        Replace the `maxthresh` attribute.

        :Args:

            x: float
                New attack threshold in dB.

        """
        pyoArgsAssert(self, "n", x)
        self._maxthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxthresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMinthresh(self, x):
        """
        Replace the `minthresh` attribute.

        :Args:

            x: float
                New minimum threshold in dB.

        """
        pyoArgsAssert(self, "n", x)
        self._minthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinthresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setReltime(self, x):
        """
        Replace the `reltime` attribute.

        :Args:

            x: float
                Time, in seconds, to wait before reporting a new attack.

        """
        pyoArgsAssert(self, "n", x)
        self._reltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setReltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def readyToDetect(self):
        """
        Initializes variables in the ready state to detect an attack.

        """
        [obj.readyToDetect() for obj in self._base_objs]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.001, 0.05, "lin", "deltime", self._deltime, dataOnly=True),
            SLMap(1.0, 1000.0, "log", "cutoff", self._cutoff, dataOnly=True),
            SLMap(0.0, 18.0, "lin", "maxthresh", self._maxthresh, dataOnly=True),
            SLMap(-90.0, 0.0, "lin", "minthresh", self._minthresh, dataOnly=True),
            SLMap(0.001, 1.0, "log", "reltime", self._reltime, dataOnly=True),
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
    def deltime(self):
        """float. Delay between rms analysis."""
        return self._deltime

    @deltime.setter
    def deltime(self, x):
        self.setDeltime(x)

    @property
    def cutoff(self):
        """float. Cutoff for the follower lowpass filter."""
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)

    @property
    def maxthresh(self):
        """float. Attack threshold in dB."""
        return self._maxthresh

    @maxthresh.setter
    def maxthresh(self, x):
        self.setMaxthresh(x)

    @property
    def minthresh(self):
        """float. Minimum threshold in dB."""
        return self._minthresh

    @minthresh.setter
    def minthresh(self, x):
        self.setMinthresh(x)

    @property
    def reltime(self):
        """float. Time to wait before reporting a new attack."""
        return self._reltime

    @reltime.setter
    def reltime(self, x):
        self.setReltime(x)


class Spectrum(PyoObject):
    """
    Spectrum analyzer and display.

    Spectrum measures the magnitude of an input signal versus frequency
    within a user defined range. It can show both magnitude and frequency
    on linear or logarithmic scale.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        size: int {pow-of-two > 4}, optional
            FFT size. Must be a power of two greater than 4.
            The FFT size is the number of samples used in each
            analysis frame. Defaults to 1024.
        wintype: int, optional
            Shape of the envelope used to filter each input frame.
            Possible shapes are :

            0. rectangular (no windowing)
            1. Hamming
            2. Hanning
            3. Bartlett (triangular)
            4. Blackman 3-term
            5. Blackman-Harris 4-term
            6. Blackman-Harris 7-term
            7. Tuckey (alpha = 0.66)
            8. Sine (half-sine window)
        function: python callable, optional
            If set, this function will be called with magnitudes (as
            list of lists, one list per channel). Useful if someone
            wants to save the analysis data into a text file.
            Defaults to None.
        wintitle: string, optional
            GUI window title. Defaults to "Spectrum".

    .. note::

        Spectrum has no `out` method.

        Spectrum has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SuperSaw(freq=[500,750], detune=0.6, bal=0.7, mul=0.5).out()
    >>> spec = Spectrum(a, size=1024)

    """

    def __init__(self, input, size=1024, wintype=2, function=None, wintitle="Spectrum"):
        pyoArgsAssert(self, "oiiCS", input, size, wintype, function, wintitle)
        PyoObject.__init__(self)
        self.points = None
        self.viewFrame = None
        self.channelNamesVisible = True
        self.channelNames = []
        self._input = input
        self._size = size
        self._wintype = wintype
        self._function = getWeakMethodRef(function)
        self._fscaling = 0
        self._mscaling = 1
        self._lowbound = 0
        self._highbound = 0.5
        self._width = 500
        self._height = 400
        self._gain = 1
        self._in_fader = InputFader(input)
        in_fader, size, wintype, lmax = convertArgsToLists(self._in_fader, size, wintype)
        self._base_objs = [Spectrum_base(wrap(in_fader, i), wrap(size, i), wrap(wintype, i)) for i in range(lmax)]
        self._timer = Pattern(self.refreshView, 0.05).play()
        if function is None:
            self.view(wintitle)
        self._init_play()

    def play(self, dur=0, delay=0):
        self._timer.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        self._timer.stop(wait)
        return PyoObject.stop(self, wait)

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
                new `size` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setWinType(self, x):
        """
        Replace the `wintype` attribute.

        :Args:

            x: int
                new `wintype` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFunction(self, function):
        """
        Sets the function to be called to retrieve the analysis data.

        :Args:

            function: python callable
                The function called by the internal timer to retrieve the
                analysis data. The function must be created with one argument
                and will receive the data as a list of lists (one list per channel).

        """
        pyoArgsAssert(self, "C", function)
        self._function = getWeakMethodRef(function)

    def poll(self, active):
        """
        Turns on and off the analysis polling.

        :Args:

            active: boolean
                If True, starts the analysis polling, False to stop it.
                defaults to True.

        """
        pyoArgsAssert(self, "B", active)
        if active:
            self._timer.play()
        else:
            self._timer.stop()

    def polltime(self, time):
        """
        Sets the polling time in seconds.

        :Args:

            time: float
                Adjusts the frequency of the internal timer used to
                retrieve the current analysis frame. defaults to 0.05.

        """
        pyoArgsAssert(self, "N", time)
        self._timer.time = time

    def setLowFreq(self, x):
        """
        Sets the lower frequency, in Hz, returned by the analysis.

        :Args:

            x: float
                New low frequency in Hz. Adjusts the `lowbound` attribute, as `x / sr`.

        """
        pyoArgsAssert(self, "n", x)
        x /= self.getServer().getSamplingRate()
        self._lowbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setLowbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHighFreq(self, x):
        """
        Sets the higher frequency, in Hz, returned by the analysis.

        :Args:

            x: float
                New high frequency in Hz. Adjusts the `highbound` attribute, as `x / sr`.

        """
        pyoArgsAssert(self, "n", x)
        x /= self.getServer().getSamplingRate()
        self._highbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setHighbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLowbound(self, x):
        """
        Sets the lower frequency, as multiplier of sr, returned by the analysis.

        Returns the real low frequency in Hz.

        :Args:

            x: float {0 <= x <= 0.5}
                new `lowbound` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._lowbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setLowbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return tmp[0]

    def setHighbound(self, x):
        """
        Sets the higher frequency, as multiplier of sr, returned by the analysis.

        Returns the real high frequency in Hz.

        :Args:

            x: float {0 <= x <= 0.5}
                new `highbound` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._highbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setHighbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return tmp[0]

    def getLowfreq(self):
        """
        Returns the current lower frequency, in Hz, used by the analysis.

        """

        return self._base_objs[0].getLowfreq()

    def getHighfreq(self):
        """
        Returns the current higher frequency, in Hz, used by the analysis.

        """
        return self._base_objs[0].getHighfreq()

    def setWidth(self, x):
        """
        Sets the width, in pixels, of the current display.

        Used internally to build the list of points to draw.

        :Args:

            x: int
                new `width` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._width = x
        x, lmax = convertArgsToLists(x)
        [obj.setWidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHeight(self, x):
        """
        Sets the height, in pixels, of the current display.

        Used internally to build the list of points to draw.

        :Args:

            x: int
                new `height` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._height = x
        x, lmax = convertArgsToLists(x)
        [obj.setHeight(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFscaling(self, x):
        """
        Sets the frequency display to linear or logarithmic.

        :Args:

            x: boolean
                If True, the frequency display is logarithmic. False turns
                it back to linear. Defaults to False.

        """
        pyoArgsAssert(self, "b", x)
        self._fscaling = x
        x, lmax = convertArgsToLists(x)
        [obj.setFscaling(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        if self.viewFrame is not None:
            self.viewFrame.setFscaling(self._fscaling)

    def setMscaling(self, x):
        """
        Sets the magnitude display to linear or logarithmic.

        :Args:

            x: boolean
                If True, the magnitude display is logarithmic (which means in dB).
                False turns it back to linear. Defaults to True.

        """
        pyoArgsAssert(self, "b", x)
        self._mscaling = x
        x, lmax = convertArgsToLists(x)
        [obj.setMscaling(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        if self.viewFrame is not None:
            self.viewFrame.setMscaling(self._mscaling)

    def getFscaling(self):
        """
        Returns the scaling of the frequency display.

        Returns True for logarithmic or False for linear.

        """
        return self._fscaling

    def getMscaling(self):
        """
        Returns the scaling of the magnitude display.

        Returns True for logarithmic or False for linear.

        """
        return self._mscaling

    def setGain(self, x):
        """
        Set the gain of the analysis data. For drawing purpose.

        :Args:

            x: float
                new `gain` attribute, as linear values.

        """
        pyoArgsAssert(self, "n", x)
        self._gain = x
        x, lmax = convertArgsToLists(x)
        [obj.setGain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def view(self, title="Spectrum", wxnoserver=False):
        """
        Opens a window showing the result of the analysis.

        :Args:

            title: string, optional
                Window title. Defaults to "Spectrum".
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        pyoArgsAssert(self, "SB", title, wxnoserver)
        createSpectrumWindow(self, title, wxnoserver)

    def showChannelNames(self, visible=True):
        """
        If True (the default), channel names will be displayed in the window.

        """
        self.channelNamesVisible = visible
        if self.viewFrame is not None:
            self.viewFrame.showChannelNames(visible)

    def setChannelNames(self, names):
        """
        Change the stream names displayed in the spectrum window.

        If given a list of strings, these will be displayed as the name of the
        consecutive audio streams instead of the generic 'chan X' names.

        """
        self.channelNames = names
        if self.viewFrame is not None:
            self.viewFrame.setChannelNames(names)

    def _setViewFrame(self, frame):
        self.viewFrame = frame

    def refreshView(self):
        """
        Updates the graphical display of the spectrum.

        Called automatically by the internal timer.

        """
        self.points = [obj.display() for obj in self._base_objs]
        if self._function is not None:
            self._function(self.points)
        if self.viewFrame is not None:
            self.viewFrame.update(self.points)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        """int. FFT size."""
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        """int. Windowing method."""
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)

    @property
    def gain(self):
        """float. Sets the gain of the analysis data."""
        return self._gain

    @gain.setter
    def gain(self, x):
        self.setGain(x)

    @property
    def lowbound(self):
        """float. Lowest frequency (multiplier of sr) to output."""
        return self._lowbound

    @lowbound.setter
    def lowbound(self, x):
        self.setLowbound(x)

    @property
    def highbound(self):
        """float. Highest frequency (multiplier of sr) to output."""
        return self._highbound

    @highbound.setter
    def highbound(self, x):
        self.setHighbound(x)

    @property
    def width(self):
        """int. Width, in pixels, of the current display."""
        return self._width

    @width.setter
    def width(self, x):
        self.setWidth(x)

    @property
    def height(self):
        """int. Height, in pixels, of the current display."""
        return self._height

    @height.setter
    def height(self, x):
        self.setHeight(x)

    @property
    def fscaling(self):
        """boolean. Scaling of the frequency display."""
        return self._fscaling

    @fscaling.setter
    def fscaling(self, x):
        self.setFscaling(x)

    @property
    def mscaling(self):
        """boolean. Scaling of the magnitude display."""
        return self._mscaling

    @mscaling.setter
    def mscaling(self, x):
        self.setMscaling(x)


class Scope(PyoObject):
    """
    Oscilloscope - audio waveform display.

    Oscilloscopes are used to observe the change of an electrical
    signal over time.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        length: float, optional
            Length, in seconds, of the displayed window. Can't be a list.
            Defaults to 0.05.
        gain: float, optional
            Linear gain applied to the signal to be displayed.
            Can't be a list. Defaults to 0.67.
        function: python callable, optional
            If set, this function will be called with samples (as
            list of lists, one list per channel). Useful if someone
            wants to save the analysis data into a text file.
            Defaults to None.
        wintitle: string, optional
            GUI window title. Defaults to "Scope".

    .. note::

        Scope has no `out` method.

        Scope has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine([100,100.2], mul=0.7)
    >>> b = Noise(0.1)
    >>> scope = Scope(a+b)

    """

    def __init__(self, input, length=0.05, gain=0.67, function=None, wintitle="Scope"):
        pyoArgsAssert(self, "oNNCS", input, length, gain, function, wintitle)
        PyoObject.__init__(self)
        self.points = None
        self.viewFrame = None
        self._input = input
        self._length = length
        self._gain = gain
        self._function = function
        self._width = 500
        self._height = 400
        self.channelNamesVisible = True
        self.channelNames = []
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        self._base_objs = [Scope_base(wrap(in_fader, i), length) for i in range(lmax)]
        self._base_objs[0].setFunc(self.refreshView)
        if function is None:
            self.view(wintitle)
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

    def setLength(self, x):
        """
        Replace the `length` attribute.

        :Args:

            x: float
                new `length` attribute.

        """
        pyoArgsAssert(self, "N", x)
        self._length = x
        [obj.setLength(x) for obj in self._base_objs]

    def setGain(self, x):
        """
        Set the gain boost applied to the analysed data. For drawing purpose.

        :Args:

            x: float
                new `gain` attribute, as linear values.

        """
        pyoArgsAssert(self, "n", x)
        self._gain = x
        x, lmax = convertArgsToLists(x)
        [obj.setGain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def poll(self, active):
        """
        Turns on and off the analysis polling.

        :Args:

            active: boolean
                If True, starts the analysis polling, False to stop it.
                defaults to True.

        """
        pyoArgsAssert(self, "B", active)
        [obj.setPoll(active) for obj in self._base_objs]

    def setWidth(self, x):
        """
        Gives the width of the display to the analyzer.

        The analyzer needs this value to construct the list
        of points to draw on the display.

        :Args:

            x: int
                Width of the display in pixel value. The default
                width is 500.

        """
        pyoArgsAssert(self, "I", x)
        self._width = x
        [obj.setWidth(x) for obj in self._base_objs]

    def setHeight(self, x):
        """
        Gives the height of the display to the analyzer.

        The analyzer needs this value to construct the list
        of points to draw on the display.

        :Args:

            x: int
                Height of the display in pixel value. The default
                height is 400.

        """
        pyoArgsAssert(self, "I", x)
        self._height = x
        [obj.setHeight(x) for obj in self._base_objs]

    def view(self, title="Scope", wxnoserver=False):
        """
        Opens a window showing the incoming waveform.

        :Args:

            title: string, optional
                Window title. Defaults to "Scope".
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for
        the server GUI before showing the controller window.

        """
        pyoArgsAssert(self, "SB", title, wxnoserver)
        createScopeWindow(self, title, wxnoserver)

    def setFunction(self, function):
        """
        Sets the function to be called to retrieve the analysis data.

        :Args:

            function: python callable
                The function called by the internal timer to retrieve the
                analysis data. The function must be created with one argument
                and will receive the data as a list of lists (one list per channel).

        """
        pyoArgsAssert(self, "C", function)
        self._function = getWeakMethodRef(function)

    def showChannelNames(self, visible=True):
        """
        If True (the default), channel names will be displayed in the window.

        """
        self.channelNamesVisible = visible
        if self.viewFrame is not None:
            self.viewFrame.showChannelNames(visible)

    def setChannelNames(self, names):
        """
        Change the stream names displayed in the spectrum window.

        If given a list of strings, these will be displayed as the name of the
        consecutive audio streams instead of the generic 'chan X' names.

        """
        self.channelNames = names
        if self.viewFrame is not None:
            self.viewFrame.setChannelNames(names)

    def _setViewFrame(self, frame):
        self.viewFrame = frame

    def refreshView(self):
        """
        Updates the graphical display of the scope.

        Called automatically by the internal timer.

        """
        self.points = [obj.display() for obj in self._base_objs]
        if self.viewFrame is not None:
            self.viewFrame.update(self.points)
        if self._function is not None:
            self._function(self.points)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def length(self):
        """float. Window length."""
        return self._length

    @length.setter
    def length(self, x):
        self.setLength(x)

    @property
    def gain(self):
        """float. Sets the gain of the analysis data."""
        return self._gain

    @gain.setter
    def gain(self, x):
        self.setGain(x)


class PeakAmp(PyoObject):
    """
    Peak amplitude follower.

    Output signal is the continuous peak amplitude of an input signal.
    A new peaking value is computed every buffer size. If `function`
    argument is not None, it should be a function that will be called
    periodically with a variable-length argument list containing
    the peaking values of all object's streams. Useful for meter drawing.
    Function definition must look like this:

    >>> def getValues(*args):
    ...     print(args)

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        function: callable, optional
            Function that will be called with amplitude values in arguments.
            Default to None.

    .. note::

        The out() method is bypassed. PeakAmp's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`Follower`, :py:class:`Follower2`, :py:class:`Balance`, :py:class:`RMS`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> amp = PeakAmp(sf)
    >>> n = Noise(mul=Port(amp)).out(1)

    """

    def __init__(self, input, function=None, mul=1, add=0):
        pyoArgsAssert(self, "oCOO", input, function, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        if callable(function):
            self._function = getWeakMethodRef(function)
        else:
            self._function = None
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [PeakAmp_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        sr = self.getSamplingRate()
        bs = self.getBufferSize()
        self._timer = Pattern(self._buildList, 0.06).play()
        self._init_play()

    def play(self, dur=0, delay=0):
        self._timer.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        self._timer.stop(wait)
        return PyoObject.stop(self, wait)

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

    def setFunction(self, x):
        """
        Replace the `function` attribute.

        :Args:

            x: callable
                New function to call with amplitude values in arguments.

        """
        pyoArgsAssert(self, "C", x)
        if callable(x):
            self._function = getWeakMethodRef(x)

    def polltime(self, x):
        """
        Sets the delay, in seconds, between each call of the function.

        :Args:

            x: float
                New polling time in seconds.

        """
        pyoArgsAssert(self, "N", x)
        self._timer.time = x

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def _buildList(self):
        if self._function is not None:
            values = [obj.getValue() for obj in self._base_objs]
            self._function(*values)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def function(self):
        """PyoObject. function signal to process."""
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)


class RMS(PyoObject):
    """
    Returns the RMS (Root-Mean-Square) value of a signal.

    Output signal is the continuous rms of an input signal. A new rms
    value is computed every buffer size. If `function` argument is not
    None, it should be a function that will be called periodically
    with a variable-length argument list containing the rms values
    of all object's streams. Useful for meter drawing. Function
    definition must look like this:

    >>> def getValues(*args):
    ...     print(args)

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        function: callable, optional
            Function that will be called with amplitude values in arguments.
            Default to None.

    .. note::

        The out() method is bypassed. RMS's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`Follower`, :py:class:`Follower2`, :py:class:`Balance`, :py:class:`PeakAmp`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True, mul=.4).out()
    >>> amp = RMS(sf)
    >>> n = Noise(mul=Port(amp)).out(1)

    """

    def __init__(self, input, function=None, mul=1, add=0):
        pyoArgsAssert(self, "oCOO", input, function, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        if callable(function):
            self._function = getWeakMethodRef(function)
        else:
            self._function = None
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [RMS_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        sr = self.getSamplingRate()
        bs = self.getBufferSize()
        self._timer = Pattern(self._buildList, 0.06).play()
        self._init_play()

    def play(self, dur=0, delay=0):
        self._timer.play(dur, delay)
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        self._timer.stop(wait)
        return PyoObject.stop(self, wait)

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

    def setFunction(self, x):
        """
        Replace the `function` attribute.

        :Args:

            x: callable
                New function to call with amplitude values in arguments.

        """
        pyoArgsAssert(self, "C", x)
        if callable(x):
            self._function = getWeakMethodRef(x)

    def polltime(self, x):
        """
        Sets the delay, in seconds, between each call of the function.

        :Args:

            x: float
                New polling time in seconds.

        """
        pyoArgsAssert(self, "N", x)
        self._timer.time = x

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def _buildList(self):
        if self._function is not None:
            values = [obj.getValue() for obj in self._base_objs]
            self._function(*values)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def function(self):
        """PyoObject. function signal to process."""
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)
