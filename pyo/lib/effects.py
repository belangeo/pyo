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
from .generators import Sine
from .filters import Hilbert


class Disto(PyoObject):

    def __init__(self, input, drive=0.75, slope=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, drive, slope, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._drive = drive
        self._slope = slope
        self._in_fader = InputFader(input)
        in_fader, drive, slope, mul, add, lmax = convertArgsToLists(self._in_fader, drive, slope, mul, add)
        self._base_objs = [
            Disto_base(wrap(in_fader, i), wrap(drive, i), wrap(slope, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDrive(self, x):
        pyoArgsAssert(self, "O", x)
        self._drive = x
        x, lmax = convertArgsToLists(x)
        [obj.setDrive(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSlope(self, x):
        pyoArgsAssert(self, "O", x)
        self._slope = x
        x, lmax = convertArgsToLists(x)
        [obj.setSlope(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def drive(self):
        return self._drive

    @drive.setter
    def drive(self, x):
        self.setDrive(x)

    @property
    def slope(self):
        return self._slope

    @slope.setter
    def slope(self, x):
        self.setSlope(x)


class Delay(PyoObject):

    def __init__(self, input, delay=0.25, feedback=0, maxdelay=1, mul=1, add=0):
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
            Delay_base(
                wrap(in_fader, i), wrap(delay, i), wrap(feedback, i), wrap(maxdelay, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDelay(self, x):
        pyoArgsAssert(self, "O", x)
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def delay(self):
        return self._delay

    @delay.setter
    def delay(self, x):
        self.setDelay(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class SDelay(PyoObject):

    def __init__(self, input, delay=0.25, maxdelay=1, mul=1, add=0):
        pyoArgsAssert(self, "oOnOO", input, delay, maxdelay, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._delay = delay
        self._maxdelay = maxdelay
        self._in_fader = InputFader(input)
        in_fader, delay, maxdelay, mul, add, lmax = convertArgsToLists(self._in_fader, delay, maxdelay, mul, add)
        self._base_objs = [
            SDelay_base(wrap(in_fader, i), wrap(delay, i), wrap(maxdelay, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDelay(self, x):
        pyoArgsAssert(self, "O", x)
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def delay(self):
        return self._delay

    @delay.setter
    def delay(self, x):
        self.setDelay(x)


class Waveguide(PyoObject):

    def __init__(self, input, freq=100, dur=10, minfreq=20, mul=1, add=0):
        pyoArgsAssert(self, "oOOnOO", input, freq, dur, minfreq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._dur = dur
        self._in_fader = InputFader(input)
        in_fader, freq, dur, minfreq, mul, add, lmax = convertArgsToLists(self._in_fader, freq, dur, minfreq, mul, add)
        self._base_objs = [
            Waveguide_base(wrap(in_fader, i), wrap(freq, i), wrap(dur, i), wrap(minfreq, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
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

    def setDur(self, x):
        pyoArgsAssert(self, "O", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

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

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)


class AllpassWG(PyoObject):

    def __init__(self, input, freq=100, feed=0.95, detune=0.5, minfreq=20, mul=1, add=0):
        pyoArgsAssert(self, "oOOOnOO", input, freq, feed, detune, minfreq, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._freq = freq
        self._feed = feed
        self._detune = detune
        self._in_fader = InputFader(input)
        in_fader, freq, feed, detune, minfreq, mul, add, lmax = convertArgsToLists(
            self._in_fader, freq, feed, detune, minfreq, mul, add
        )
        self._base_objs = [
            AllpassWG_base(
                wrap(in_fader, i),
                wrap(freq, i),
                wrap(feed, i),
                wrap(detune, i),
                wrap(minfreq, i),
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

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeed(self, x):
        pyoArgsAssert(self, "O", x)
        self._feed = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeed(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDetune(self, x):
        pyoArgsAssert(self, "O", x)
        self._detune = x
        x, lmax = convertArgsToLists(x)
        [obj.setDetune(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

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

    @property
    def feed(self):
        return self._feed

    @feed.setter
    def feed(self, x):
        self.setFeed(x)

    @property
    def detune(self):
        return self._detune

    @detune.setter
    def detune(self, x):
        self.setDetune(x)


class Freeverb(PyoObject):

    def __init__(self, input, size=0.5, damp=0.5, bal=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOO", input, size, damp, bal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._size = size
        self._damp = damp
        self._bal = bal
        self._in_fader = InputFader(input)
        in_fader, size, damp, bal, mul, add, lmax = convertArgsToLists(self._in_fader, size, damp, bal, mul, add)
        self._base_objs = [
            Freeverb_base(wrap(in_fader, i), wrap(size, i), wrap(damp, i), wrap(bal, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setSize(self, x):
        pyoArgsAssert(self, "O", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        [obj.setSize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDamp(self, x):
        pyoArgsAssert(self, "O", x)
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

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
    def damp(self):
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)

    @property
    def bal(self):
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class Convolve(PyoObject):

    def __init__(self, input, table, size, mul=1, add=0):
        pyoArgsAssert(self, "otiOO", input, table, size, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._table = table
        self._size = size
        self._in_fader = InputFader(input)
        in_fader, table, size, mul, add, lmax = convertArgsToLists(self._in_fader, table, size, mul, add)
        self._base_objs = [
            Convolve_base(wrap(in_fader, i), wrap(table, i), wrap(size, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTable(self, x):
        pyoArgsAssert(self, "t", x)
        self._table = x
        x, lmax = convertArgsToLists(x)
        [obj.setTable(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def table(self):
        return self._table

    @table.setter
    def table(self, x):
        self.setTable(x)


class WGVerb(PyoObject):

    def __init__(self, input, feedback=0.5, cutoff=5000, bal=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOO", input, feedback, cutoff, bal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._feedback = feedback
        self._cutoff = cutoff
        self._bal = bal
        self._in_fader = InputFader(input)
        in_fader, feedback, cutoff, bal, mul, add, lmax = convertArgsToLists(
            self._in_fader, feedback, cutoff, bal, mul, add
        )
        self._base_objs = [
            WGVerb_base(wrap(in_fader, i), wrap(feedback, i), wrap(cutoff, i), wrap(bal, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCutoff(self, x):
        pyoArgsAssert(self, "O", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

    @property
    def cutoff(self):
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)

    @property
    def bal(self):
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class Chorus(PyoObject):

    def __init__(self, input, depth=1, feedback=0.25, bal=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOO", input, depth, feedback, bal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._depth = depth
        self._feedback = feedback
        self._bal = bal
        self._in_fader = InputFader(input)
        in_fader, depth, feedback, bal, mul, add, lmax = convertArgsToLists(
            self._in_fader, depth, feedback, bal, mul, add
        )
        self._base_objs = [
            Chorus_base(wrap(in_fader, i), wrap(depth, i), wrap(feedback, i), wrap(bal, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDepth(self, x):
        pyoArgsAssert(self, "O", x)
        self._depth = x
        x, lmax = convertArgsToLists(x)
        [obj.setDepth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def depth(self):
        return self._depth

    @depth.setter
    def depth(self, x):
        self.setDepth(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

    @property
    def bal(self):
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class Harmonizer(PyoObject):

    def __init__(self, input, transpo=-7.0, feedback=0, winsize=0.1, mul=1, add=0):
        pyoArgsAssert(self, "oOOnOO", input, transpo, feedback, winsize, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._transpo = transpo
        self._feedback = feedback
        self._winsize = winsize
        self._in_fader = InputFader(input)
        in_fader, transpo, feedback, winsize, mul, add, lmax = convertArgsToLists(
            self._in_fader, transpo, feedback, winsize, mul, add
        )
        self._base_objs = [
            Harmonizer_base(
                wrap(in_fader, i), wrap(transpo, i), wrap(feedback, i), wrap(winsize, i), wrap(mul, i), wrap(add, i)
            )
            for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTranspo(self, x):
        pyoArgsAssert(self, "O", x)
        self._transpo = x
        x, lmax = convertArgsToLists(x)
        [obj.setTranspo(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setWinsize(self, x):
        pyoArgsAssert(self, "n", x)
        self._winsize = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinsize(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def transpo(self):
        return self._transpo

    @transpo.setter
    def transpo(self, x):
        self.setTranspo(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

    @property
    def winsize(self):
        return self._winsize

    @winsize.setter
    def winsize(self, x):
        self.setWinsize(x)


class Delay1(PyoObject):

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [Delay1_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class STRev(PyoObject):

    def __init__(self, input, inpos=0.5, revtime=1, cutoff=5000, bal=0.5, roomSize=1, firstRefGain=-3, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOnnOO", input, inpos, revtime, cutoff, bal, roomSize, firstRefGain, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._inpos = inpos
        self._revtime = revtime
        self._cutoff = cutoff
        self._bal = bal
        self._roomSize = roomSize
        self._firstRefGain = firstRefGain
        self._in_fader = InputFader(input)
        in_fader, inpos, revtime, cutoff, bal, roomSize, firstRefGain, mul, add, lmax = convertArgsToLists(
            self._in_fader, inpos, revtime, cutoff, bal, roomSize, firstRefGain, mul, add
        )
        self._base_players = [
            STReverb_base(
                wrap(in_fader, i),
                wrap(inpos, i),
                wrap(revtime, i),
                wrap(cutoff, i),
                wrap(bal, i),
                wrap(roomSize, i),
                wrap(firstRefGain, i),
            )
            for i in range(lmax)
        ]
        self._base_objs = [
            STRev_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i))
            for i in range(lmax)
            for j in range(2)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInpos(self, x):
        pyoArgsAssert(self, "O", x)
        self._inpos = x
        x, lmax = convertArgsToLists(x)
        [obj.setInpos(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setRevtime(self, x):
        pyoArgsAssert(self, "O", x)
        self._revtime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRevtime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setCutoff(self, x):
        pyoArgsAssert(self, "O", x)
        self._cutoff = x
        x, lmax = convertArgsToLists(x)
        [obj.setCutoff(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setBal(self, x):
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setMix(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setRoomSize(self, x):
        pyoArgsAssert(self, "n", x)
        self._roomSize = x
        x, lmax = convertArgsToLists(x)
        [obj.setRoomSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setFirstRefGain(self, x):
        pyoArgsAssert(self, "n", x)
        self._firstRefGain = x
        x, lmax = convertArgsToLists(x)
        [obj.setFirstRefGain(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def reset(self):
        [obj.reset() for obj in self._base_players]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def inpos(self):
        return self._inpos

    @inpos.setter
    def inpos(self, x):
        self.setInpos(x)

    @property
    def revtime(self):
        return self._revtime

    @revtime.setter
    def revtime(self, x):
        self.setRevtime(x)

    @property
    def cutoff(self):
        return self._cutoff

    @cutoff.setter
    def cutoff(self, x):
        self.setCutoff(x)

    @property
    def bal(self):
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)

    @property
    def roomSize(self):
        return self._roomSize

    @roomSize.setter
    def roomSize(self, x):
        self.setRoomSize(x)

    @property
    def firstRefGain(self):
        return self._firstRefGain

    @firstRefGain.setter
    def firstRefGain(self, x):
        self.setFirstRefGain(x)


class SmoothDelay(PyoObject):

    def __init__(self, input, delay=0.25, feedback=0, crossfade=0.05, maxdelay=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOnnOO", input, delay, feedback, crossfade, maxdelay, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._delay = delay
        self._feedback = feedback
        self._crossfade = crossfade
        self._maxdelay = maxdelay
        self._in_fader = InputFader(input)
        in_fader, delay, feedback, crossfade, maxdelay, mul, add, lmax = convertArgsToLists(
            self._in_fader, delay, feedback, crossfade, maxdelay, mul, add
        )
        self._base_objs = [
            SmoothDelay_base(
                wrap(in_fader, i),
                wrap(delay, i),
                wrap(feedback, i),
                wrap(crossfade, i),
                wrap(maxdelay, i),
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

    def setDelay(self, x):
        pyoArgsAssert(self, "O", x)
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setCrossfade(self, x):
        pyoArgsAssert(self, "n", x)
        self._crossfade = x
        x, lmax = convertArgsToLists(x)
        [obj.setCrossfade(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for obj in self._base_objs]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def delay(self):
        return self._delay

    @delay.setter
    def delay(self, x):
        self.setDelay(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)

    @property
    def crossfade(self):
        return self._crossfade

    @crossfade.setter
    def crossfade(self, x):
        self.setCrossfade(x)


class FreqShift(PyoObject):

    def __init__(self, input, shift=100, mul=1, add=0):
        pyoArgsAssert(self, "oOOO", input, shift, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._shift = shift
        self._in_fader = InputFader(input)
        in_fader, shift, mul, add, lmax = convertArgsToLists(self._in_fader, shift, mul, add)

        self._hilb_objs = []
        self._sin_objs = []
        self._cos_objs = []
        self._mod_objs = []
        self._base_objs = []
        for i in range(lmax):
            self._hilb_objs.append(Hilbert(wrap(in_fader, i)))
            self._sin_objs.append(Sine(freq=wrap(shift, i), mul=0.707))
            self._cos_objs.append(Sine(freq=wrap(shift, i), phase=0.25, mul=0.707))
            self._mod_objs.append(
                Mix(
                    self._hilb_objs[-1]["real"] * self._sin_objs[-1] - self._hilb_objs[-1]["imag"] * self._cos_objs[-1],
                    mul=wrap(mul, i),
                    add=wrap(add, i),
                )
            )
            self._base_objs.extend(self._mod_objs[-1].getBaseObjects())
        self._init_play()

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._hilb_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._sin_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._cos_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._mod_objs)]
        return PyoObject.play(self, dur, delay)

    def stop(self, wait=0):
        [obj.stop(wait) for obj in self._hilb_objs]
        [obj.stop(wait) for obj in self._sin_objs]
        [obj.stop(wait) for obj in self._cos_objs]
        [obj.stop(wait) for obj in self._mod_objs]
        return PyoObject.stop(self, wait)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._hilb_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._sin_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._cos_objs)]
        [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._mod_objs)]
        return PyoObject.out(self, chnl, inc, dur, delay)

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setShift(self, x):
        pyoArgsAssert(self, "O", x)
        self._shift = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._sin_objs)]
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._cos_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def shift(self):
        return self._shift

    @shift.setter
    def shift(self, x):
        self.setShift(x)
