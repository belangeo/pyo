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


class Spectrum(PyoObject):

    def __init__(self, input, size=1024, wintype=2, function=None, wintitle="Spectrum"):
        pyoArgsAssert(self, "oiiCS", input, size, wintype, function, wintitle)
        PyoObject.__init__(self)
        self.points = None
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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setSize(self, x):
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setWinType(self, x):
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFunction(self, function):
        pyoArgsAssert(self, "C", function)
        self._function = getWeakMethodRef(function)

    def poll(self, active):
        pyoArgsAssert(self, "B", active)
        if active:
            self._timer.play()
        else:
            self._timer.stop()

    def polltime(self, time):
        pyoArgsAssert(self, "N", time)
        self._timer.time = time

    def setLowFreq(self, x):
        pyoArgsAssert(self, "n", x)
        x /= self.getServer().getSamplingRate()
        self._lowbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setLowbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHighFreq(self, x):
        pyoArgsAssert(self, "n", x)
        x /= self.getServer().getSamplingRate()
        self._highbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setHighbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLowbound(self, x):
        pyoArgsAssert(self, "n", x)
        self._lowbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setLowbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return tmp[0]

    def setHighbound(self, x):
        pyoArgsAssert(self, "n", x)
        self._highbound = x
        x, lmax = convertArgsToLists(x)
        tmp = [obj.setHighbound(wrap(x, i)) for i, obj in enumerate(self._base_objs)]
        return tmp[0]

    def getLowfreq(self):

        return self._base_objs[0].getLowfreq()

    def getHighfreq(self):
        return self._base_objs[0].getHighfreq()

    def setWidth(self, x):
        pyoArgsAssert(self, "i", x)
        self._width = x
        x, lmax = convertArgsToLists(x)
        [obj.setWidth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHeight(self, x):
        pyoArgsAssert(self, "i", x)
        self._height = x
        x, lmax = convertArgsToLists(x)
        [obj.setHeight(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFscaling(self, x):
        pyoArgsAssert(self, "b", x)
        self._fscaling = x
        x, lmax = convertArgsToLists(x)
        [obj.setFscaling(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMscaling(self, x):
        pyoArgsAssert(self, "b", x)
        self._mscaling = x
        x, lmax = convertArgsToLists(x)
        [obj.setMscaling(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def getFscaling(self):
        return self._fscaling

    def getMscaling(self):
        return self._mscaling

    def setGain(self, x):
        pyoArgsAssert(self, "n", x)
        self._gain = x
        x, lmax = convertArgsToLists(x)
        [obj.setGain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)

    @property
    def gain(self):
        return self._gain

    @gain.setter
    def gain(self, x):
        self.setGain(x)

    @property
    def lowbound(self):
        return self._lowbound

    @lowbound.setter
    def lowbound(self, x):
        self.setLowbound(x)

    @property
    def highbound(self):
        return self._highbound

    @highbound.setter
    def highbound(self, x):
        self.setHighbound(x)

    @property
    def width(self):
        return self._width

    @width.setter
    def width(self, x):
        self.setWidth(x)

    @property
    def height(self):
        return self._height

    @height.setter
    def height(self, x):
        self.setHeight(x)

    @property
    def fscaling(self):
        return self._fscaling

    @fscaling.setter
    def fscaling(self, x):
        self.setFscaling(x)

    @property
    def mscaling(self):
        return self._mscaling

    @mscaling.setter
    def mscaling(self, x):
        self.setMscaling(x)


class Scope(PyoObject):

    def __init__(self, input, length=0.05, gain=0.67, function=None, wintitle="Scope"):
        pyoArgsAssert(self, "oNNCS", input, length, gain, function, wintitle)
        PyoObject.__init__(self)
        self.points = None
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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setLength(self, x):
        pyoArgsAssert(self, "N", x)
        self._length = x
        [obj.setLength(x) for obj in self._base_objs]

    def setGain(self, x):
        pyoArgsAssert(self, "n", x)
        self._gain = x
        x, lmax = convertArgsToLists(x)
        [obj.setGain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def poll(self, active):
        pyoArgsAssert(self, "B", active)
        [obj.setPoll(active) for obj in self._base_objs]

    def setWidth(self, x):
        pyoArgsAssert(self, "I", x)
        self._width = x
        [obj.setWidth(x) for obj in self._base_objs]

    def setHeight(self, x):
        pyoArgsAssert(self, "I", x)
        self._height = x
        [obj.setHeight(x) for obj in self._base_objs]

    def setFunction(self, function):
        pyoArgsAssert(self, "C", function)
        self._function = getWeakMethodRef(function)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def length(self):
        return self._length

    @length.setter
    def length(self, x):
        self.setLength(x)

    @property
    def gain(self):
        return self._gain

    @gain.setter
    def gain(self, x):
        self.setGain(x)


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
