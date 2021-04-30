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
from .pattern import Pattern


class Follower(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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


class Follower2(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setRisetime(self, x):
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRisetime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFalltime(self, x):
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFalltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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
        self.setRisetime(x)

    @property
    def falltime(self):
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFalltime(x)


class ZCross(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThresh(self, x):
        pyoArgsAssert(self, "n", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def thresh(self):
        return self._thresh

    @thresh.setter
    def thresh(self, x):
        self.setThresh(x)


class Yin(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTolerance(self, x):
        pyoArgsAssert(self, "n", x)
        self._tolerance = x
        x, lmax = convertArgsToLists(x)
        [obj.setTolerance(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMinfreq(self, x):
        pyoArgsAssert(self, "n", x)
        self._minfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinfreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMaxfreq(self, x):
        pyoArgsAssert(self, "n", x)
        self._maxfreq = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxfreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        pyoArgsAssert(self, "n", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def tolerance(self):
        return self._tolerance

    @tolerance.setter
    def tolerance(self, x):
        self.setTolerance(x)

    @property
    def minfreq(self):
        return self._minfreq

    @minfreq.setter
    def minfreq(self, x):
        self.setMinfreq(x)

    @property
    def maxfreq(self):
        return self._maxfreq

    @maxfreq.setter
    def maxfreq(self, x):
        self.setMaxfreq(x)

    @property
    def cutoff(self):
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)


class Centroid(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class AttackDetector(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDeltime(self, x):
        pyoArgsAssert(self, "n", x)
        self._deltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setDeltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        pyoArgsAssert(self, "n", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMaxthresh(self, x):
        pyoArgsAssert(self, "n", x)
        self._maxthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxthresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMinthresh(self, x):
        pyoArgsAssert(self, "n", x)
        self._minthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinthresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setReltime(self, x):
        pyoArgsAssert(self, "n", x)
        self._reltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setReltime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def readyToDetect(self):
        [obj.readyToDetect() for obj in self._base_objs]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def deltime(self):
        return self._deltime

    @deltime.setter
    def deltime(self, x):
        self.setDeltime(x)

    @property
    def cutoff(self):
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)

    @property
    def maxthresh(self):
        return self._maxthresh

    @maxthresh.setter
    def maxthresh(self, x):
        self.setMaxthresh(x)

    @property
    def minthresh(self):
        return self._minthresh

    @minthresh.setter
    def minthresh(self, x):
        self.setMinthresh(x)

    @property
    def reltime(self):
        return self._reltime

    @reltime.setter
    def reltime(self, x):
        self.setReltime(x)


class PeakAmp(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFunction(self, x):
        pyoArgsAssert(self, "C", x)
        if callable(x):
            self._function = getWeakMethodRef(x)

    def polltime(self, x):
        pyoArgsAssert(self, "N", x)
        self._timer.time = x

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def _buildList(self):
        if self._function is not None:
            values = [obj.getValue() for obj in self._base_objs]
            self._function(*values)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def function(self):
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)


class RMS(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFunction(self, x):
        pyoArgsAssert(self, "C", x)
        if callable(x):
            self._function = getWeakMethodRef(x)

    def polltime(self, x):
        pyoArgsAssert(self, "N", x)
        self._timer.time = x

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def _buildList(self):
        if self._function is not None:
            values = [obj.getValue() for obj in self._base_objs]
            self._function(*values)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def function(self):
        return self._function

    @function.setter
    def function(self, x):
        self.setFunction(x)
