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


class Clip(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Mirror(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMin(self, x):
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class Degrade(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setBitdepth(self, x):
        pyoArgsAssert(self, "O", x)
        self._bitdepth = x
        x, lmax = convertArgsToLists(x)
        [obj.setBitdepth(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSrscale(self, x):
        pyoArgsAssert(self, "O", x)
        self._srscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setSrscale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def bitdepth(self):
        return self._bitdepth

    @bitdepth.setter
    def bitdepth(self, x):
        self.setBitdepth(x)

    @property
    def srscale(self):
        return self._srscale

    @srscale.setter
    def srscale(self, x):
        self.setSrscale(x)


class Compress(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThresh(self, x):
        pyoArgsAssert(self, "O", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setKnee(self, x):
        pyoArgsAssert(self, "n", x)
        self._knee = x
        x, lmax = convertArgsToLists(x)
        [obj.setKnee(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def risetime(self):
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)

    @property
    def knee(self):
        return self._knee

    @knee.setter
    def knee(self, x):
        self.setKnee(x)


class Gate(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setThresh(self, x):
        pyoArgsAssert(self, "O", x)
        self._thresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    @property
    def risetime(self):
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)


class Balance(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)


class Min(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        pyoArgsAssert(self, "O", x)
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def comp(self):
        return self._comp

    @comp.setter
    def comp(self, x):
        self.setComp(x)


class Max(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        pyoArgsAssert(self, "O", x)
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def comp(self):
        return self._comp

    @comp.setter
    def comp(self, x):
        self.setComp(x)


class Expand(PyoObject):

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
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDownThresh(self, x):
        pyoArgsAssert(self, "O", x)
        self._downthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setDownThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setUpThresh(self, x):
        pyoArgsAssert(self, "O", x)
        self._upthresh = x
        x, lmax = convertArgsToLists(x)
        [obj.setUpThresh(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRiseTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._risetime = x
        x, lmax = convertArgsToLists(x)
        [obj.setRiseTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFallTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._falltime = x
        x, lmax = convertArgsToLists(x)
        [obj.setFallTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLookAhead(self, x):
        pyoArgsAssert(self, "n", x)
        self._lookahead = x
        x, lmax = convertArgsToLists(x)
        [obj.setLookAhead(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def downthresh(self):
        return self._downthresh

    @downthresh.setter
    def downthresh(self, x):
        self.setDownThresh(x)

    @property
    def upthresh(self):
        return self._upthresh

    @upthresh.setter
    def upthresh(self, x):
        self.setUpThresh(x)

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def risetime(self):
        return self._risetime

    @risetime.setter
    def risetime(self, x):
        self.setRiseTime(x)

    @property
    def falltime(self):
        return self._falltime

    @falltime.setter
    def falltime(self, x):
        self.setFallTime(x)

    @property
    def lookahead(self):
        return self._lookahead

    @lookahead.setter
    def lookahead(self, x):
        self.setLookAhead(x)
