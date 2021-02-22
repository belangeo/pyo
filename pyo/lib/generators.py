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

######################################################################
### Sources
######################################################################
class Sine(PyoObject):

    def __init__(self, freq=1000, phase=0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, phase, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._phase = phase
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Sine_base(wrap(freq, i), wrap(phase, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPhase(self, x):
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)


class FastSine(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setQuality(self, x):
        pyoArgsAssert(self, "i", x)
        self._quality = x
        x, lmax = convertArgsToLists(x)
        [obj.setQuality(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def quality(self):
        return self._quality

    @quality.setter
    def quality(self, x):
        self.setQuality(x)


class SineLoop(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        pyoArgsAssert(self, "O", x)
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def feedback(self):
        return self._feedback

    @feedback.setter
    def feedback(self, x):
        self.setFeedback(x)


class Phasor(PyoObject):

    def __init__(self, freq=100, phase=0, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, phase, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._phase = phase
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Phasor_base(wrap(freq, i), wrap(phase, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setPhase(self, x):
        pyoArgsAssert(self, "O", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)


class Input(PyoObject):

    def __init__(self, chnl=0, mul=1, add=0):
        pyoArgsAssert(self, "iOO", chnl, mul, add)
        PyoObject.__init__(self, mul, add)
        self._chnl = chnl
        chnl, mul, add, lmax = convertArgsToLists(chnl, mul, add)
        self._base_objs = [Input_base(wrap(chnl, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()


class Noise(PyoObject):

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        self._type = 0
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [Noise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class PinkNoise(PyoObject):

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [PinkNoise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()


class BrownNoise(PyoObject):

    def __init__(self, mul=1, add=0):
        pyoArgsAssert(self, "OO", mul, add)
        PyoObject.__init__(self, mul, add)
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [BrownNoise_base(wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()


class FM(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def carrier(self):
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class CrossFM(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInd1(self, x):
        pyoArgsAssert(self, "O", x)
        self._ind1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd1(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInd2(self, x):
        pyoArgsAssert(self, "O", x)
        self._ind2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd2(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def carrier(self):
        return self._carrier

    @carrier.setter
    def carrier(self, x):
        self.setCarrier(x)

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def ind1(self):
        return self._ind1

    @ind1.setter
    def ind1(self, x):
        self.setInd1(x)

    @property
    def ind2(self):
        return self._ind2

    @ind2.setter
    def ind2(self, x):
        self.setInd2(x)


class Blit(PyoObject):

    def __init__(self, freq=100, harms=40, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, harms, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._harms = harms
        freq, harms, mul, add, lmax = convertArgsToLists(freq, harms, mul, add)
        self._base_objs = [Blit_base(wrap(freq, i), wrap(harms, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setHarms(self, x):
        pyoArgsAssert(self, "O", x)
        self._harms = x
        x, lmax = convertArgsToLists(x)
        [obj.setHarms(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def harms(self):
        return self._harms

    @harms.setter
    def harms(self, x):
        self.setHarms(x)


class Rossler(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class Lorenz(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class ChenLee(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        pyoArgsAssert(self, "O", x)
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def pitch(self):
        return self._pitch

    @pitch.setter
    def pitch(self, x):
        self.setPitch(x)

    @property
    def chaos(self):
        return self._chaos

    @chaos.setter
    def chaos(self, x):
        self.setChaos(x)


class LFO(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSharp(self, x):
        pyoArgsAssert(self, "O", x)
        self._sharp = x
        x, lmax = convertArgsToLists(x)
        [obj.setSharp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        pyoArgsAssert(self, "i", x)
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def sharp(self):
        return self._sharp

    @sharp.setter
    def sharp(self, x):
        self.setSharp(x)

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, x):
        self.setType(x)


class SumOsc(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        pyoArgsAssert(self, "O", x)
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        pyoArgsAssert(self, "O", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def ratio(self):
        return self._ratio

    @ratio.setter
    def ratio(self, x):
        self.setRatio(x)

    @property
    def index(self):
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)


class SuperSaw(PyoObject):

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
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDetune(self, x):
        pyoArgsAssert(self, "O", x)
        self._detune = x
        x, lmax = convertArgsToLists(x)
        [obj.setDetune(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setBal(self, x):
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setBal(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def detune(self):
        return self._detune

    @detune.setter
    def detune(self, x):
        self.setDetune(x)

    @property
    def bal(self):
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class RCOsc(PyoObject):

    def __init__(self, freq=100, sharp=0.25, mul=1, add=0):
        pyoArgsAssert(self, "OOOO", freq, sharp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._freq = freq
        self._sharp = sharp
        freq, sharp, mul, add, lmax = convertArgsToLists(freq, sharp, mul, add)
        self._base_objs = [RCOsc_base(wrap(freq, i), wrap(sharp, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
        self._init_play()

    def setFreq(self, x):
        pyoArgsAssert(self, "O", x)
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSharp(self, x):
        pyoArgsAssert(self, "O", x)
        self._sharp = x
        x, lmax = convertArgsToLists(x)
        [obj.setSharp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def reset(self):
        [obj.reset() for i, obj in enumerate(self._base_objs)]

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, x):
        self.setFreq(x)

    @property
    def sharp(self):
        return self._sharp

    @sharp.setter
    def sharp(self, x):
        self.setSharp(x)
