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
### Controls
######################################################################
class Fader(PyoObject):

    def __init__(self, fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0):
        pyoArgsAssert(self, "nnnOO", fadein, fadeout, dur, mul, add)
        PyoObject.__init__(self, mul, add)
        self._fadein = fadein
        self._fadeout = fadeout
        self._dur = dur
        self._exp = 1.0
        fadein, fadeout, dur, mul, add, lmax = convertArgsToLists(fadein, fadeout, dur, mul, add)
        self._base_objs = [
            Fader_base(wrap(fadein, i), wrap(fadeout, i), wrap(dur, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        if self.isPlaying() or self.isOutputting():
            self._autostop(wait)
        if self._is_mul_attribute and not self._use_wait_time_on_stop:
            wait = 0
        if self._stop_delay != -1:
            wait = self._stop_delay

        # Don't call stop on the self._trig_objs because it has to wait
        # until the fadeout finish to send its trigger. This will leave
        # a running stream in the server...

        [obj.stop(wait) for obj in self._base_objs]

        return self

    def setFadein(self, x):
        pyoArgsAssert(self, "n", x)
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        pyoArgsAssert(self, "n", x)
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "n", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def fadein(self):
        return self._fadein

    @fadein.setter
    def fadein(self, x):
        self.setFadein(x)

    @property
    def fadeout(self):
        return self._fadeout

    @fadeout.setter
    def fadeout(self, x):
        self.setFadeout(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class Adsr(PyoObject):

    def __init__(self, attack=0.01, decay=0.05, sustain=0.707, release=0.1, dur=0, mul=1, add=0):
        pyoArgsAssert(self, "nnnnnOO", attack, decay, sustain, release, dur, mul, add)
        PyoObject.__init__(self, mul, add)
        self._attack = attack
        self._decay = decay
        self._sustain = sustain
        self._release = release
        self._dur = dur
        self._exp = 1.0
        attack, decay, sustain, release, dur, mul, add, lmax = convertArgsToLists(
            attack, decay, sustain, release, dur, mul, add
        )
        self._base_objs = [
            Adsr_base(
                wrap(attack, i),
                wrap(decay, i),
                wrap(sustain, i),
                wrap(release, i),
                wrap(dur, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
        ]
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self, wait=0):
        if self.isPlaying() or self.isOutputting():
            self._autostop(wait)
        if self._is_mul_attribute and not self._use_wait_time_on_stop:
            wait = 0
        if self._stop_delay != -1:
            wait = self._stop_delay

        # Don't call stop on the self._trig_objs because it has to wait
        # until the fadeout finish to send its trigger. This will leave
        # a running stream in the server...

        [obj.stop(wait) for obj in self._base_objs]

        return self

    def setAttack(self, x):
        pyoArgsAssert(self, "n", x)
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        pyoArgsAssert(self, "n", x)
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        pyoArgsAssert(self, "n", x)
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        pyoArgsAssert(self, "n", x)
        self._release = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        pyoArgsAssert(self, "n", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def attack(self):
        return self._attack

    @attack.setter
    def attack(self, x):
        self.setAttack(x)

    @property
    def decay(self):
        return self._decay

    @decay.setter
    def decay(self, x):
        self.setDecay(x)

    @property
    def sustain(self):
        return self._sustain

    @sustain.setter
    def sustain(self, x):
        self.setSustain(x)

    @property
    def release(self):
        return self._release

    @release.setter
    def release(self, x):
        self.setRelease(x)

    @property
    def dur(self):
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class Linseg(PyoObject):

    def __init__(self, list, loop=False, initToFirstVal=False, mul=1, add=0):
        pyoArgsAssert(self, "lbbOO", list, loop, initToFirstVal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._list = list
        self._loop = loop
        initToFirstVal, loop, mul, add, lmax = convertArgsToLists(initToFirstVal, loop, mul, add)
        if type(list[0]) != list:
            self._base_objs = [
                Linseg_base(list, wrap(loop, i), wrap(initToFirstVal, i), wrap(mul, i), wrap(add, i))
                for i in range(lmax)
            ]
        else:
            listlen = len(list)
            lmax = max(listlen, lmax)
            self._base_objs = [
                Linseg_base(wrap(list, i), wrap(loop, i), wrap(initToFirstVal, i), wrap(mul, i), wrap(add, i))
                for i in range(lmax)
            ]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setList(self, x):
        pyoArgsAssert(self, "l", x)
        self._list = x
        if type(x[0]) != list:
            [obj.setList(x) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setList(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        self.setList(x)

    def getPoints(self):
        return self._list

    def setLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def pause(self):
        [obj.pause() for obj in self._base_objs]

    def clear(self):
        [obj.clear() for obj in self._base_objs]

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)

    @property
    def loop(self):
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)


class Expseg(PyoObject):

    def __init__(self, list, loop=False, exp=10, inverse=True, initToFirstVal=False, mul=1, add=0):
        pyoArgsAssert(self, "lbnbbOO", list, loop, exp, inverse, initToFirstVal, mul, add)
        PyoObject.__init__(self, mul, add)
        self._list = list
        self._loop = loop
        self._exp = exp
        self._inverse = inverse
        loop, exp, inverse, initToFirstVal, mul, add, lmax = convertArgsToLists(
            loop, exp, inverse, initToFirstVal, mul, add
        )
        if type(list[0]) != list:
            self._base_objs = [
                Expseg_base(
                    list,
                    wrap(loop, i),
                    wrap(exp, i),
                    wrap(inverse, i),
                    wrap(initToFirstVal, i),
                    wrap(mul, i),
                    wrap(add, i),
                )
                for i in range(lmax)
            ]
        else:
            listlen = len(list)
            lmax = max(listlen, lmax)
            self._base_objs = [
                Expseg_base(
                    wrap(list, i),
                    wrap(loop, i),
                    wrap(exp, i),
                    wrap(inverse, i),
                    wrap(initToFirstVal, i),
                    wrap(mul, i),
                    wrap(add, i),
                )
                for i in range(lmax)
            ]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setList(self, x):
        pyoArgsAssert(self, "l", x)
        self._list = x
        if type(x[0]) != list:
            [obj.setList(x) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setList(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInverse(self, x):
        pyoArgsAssert(self, "b", x)
        self._inverse = x
        x, lmax = convertArgsToLists(x)
        [obj.setInverse(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        self.setList(x)

    def pause(self):
        [obj.pause() for obj in self._base_objs]

    def clear(self):
        [obj.clear() for obj in self._base_objs]

    def getPoints(self):
        return self._list

    @property
    def list(self):
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)

    @property
    def loop(self):
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def exp(self):
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)

    @property
    def inverse(self):
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)


class SigTo(PyoObject):

    def __init__(self, value, time=0.025, init=0.0, mul=1, add=0):
        pyoArgsAssert(self, "OOnOO", value, time, init, mul, add)
        PyoObject.__init__(self, mul, add)
        self._value = value
        self._time = time
        value, time, init, mul, add, lmax = convertArgsToLists(value, time, init, mul, add)
        self._base_objs = [
            SigTo_base(wrap(value, i), wrap(time, i), wrap(init, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setValue(self, x):
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def value(self):
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)
