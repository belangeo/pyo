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

import sys
from ._core import *


class Pan(PyoObject):

    def __init__(self, input, outs=2, pan=0.5, spread=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oIOOOO", input, outs, pan, spread, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._pan = pan
        self._outs = outs
        self._spread = spread
        self._in_fader = InputFader(input)
        in_fader, pan, spread, mul, add, lmax = convertArgsToLists(self._in_fader, pan, spread, mul, add)
        self._base_players = [Panner_base(wrap(in_fader, i), outs, wrap(pan, i), wrap(spread, i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(Pan_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPan(self, x):
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setSpread(self, x):
        pyoArgsAssert(self, "O", x)
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def pan(self):
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)

    @property
    def spread(self):
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)


class SPan(PyoObject):

    def __init__(self, input, outs=2, pan=0.5, mul=1, add=0):
        pyoArgsAssert(self, "oIOOO", input, outs, pan, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._outs = outs
        self._pan = pan
        self._in_fader = InputFader(input)
        in_fader, pan, mul, add, lmax = convertArgsToLists(self._in_fader, pan, mul, add)
        self._base_players = [SPanner_base(wrap(in_fader, i), outs, wrap(pan, i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(SPan_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setPan(self, x):
        pyoArgsAssert(self, "O", x)
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def pan(self):
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)


class Switch(PyoObject):

    def __init__(self, input, outs=2, voice=0.0, mul=1, add=0):
        pyoArgsAssert(self, "oIOOO", input, outs, voice, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._outs = outs
        self._voice = voice
        self._in_fader = InputFader(input)
        in_fader, voice, mul, add, lmax = convertArgsToLists(self._in_fader, voice, mul, add)
        self._base_players = [Switcher_base(wrap(in_fader, i), outs, wrap(voice, i)) for i in range(lmax)]
        self._base_objs = []
        for j in range(outs):
            for i in range(lmax):
                self._base_objs.append(Switch_base(wrap(self._base_players, i), j, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setVoice(self, x):
        pyoArgsAssert(self, "O", x)
        self._voice = x
        x, lmax = convertArgsToLists(x)
        [obj.setVoice(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def voice(self):
        return self._voice

    @voice.setter
    def voice(self, x):
        self.setVoice(x)


class Selector(PyoObject):

    def __init__(self, inputs, voice=0.0, mul=1, add=0):
        pyoArgsAssert(self, "lOOO", inputs, voice, mul, add)
        PyoObject.__init__(self, mul, add)
        self._inputs = inputs
        self._voice = voice
        self._mode = 0
        voice, mul, add, self._lmax = convertArgsToLists(voice, mul, add)
        self._length = 1
        for obj in self._inputs:
            try:
                if len(obj) > self._length:
                    self._length = len(obj)
            except:
                pass
        self._base_objs = []
        for i in range(self._lmax):
            for j in range(self._length):
                choice = []
                for obj in self._inputs:
                    try:
                        choice.append(obj[j % len(obj)])
                    except:
                        choice.append(obj)
                self._base_objs.append(Selector_base(choice, wrap(voice, i), wrap(mul, i), wrap(add, i)))
        self._init_play()

    def setInputs(self, x):
        pyoArgsAssert(self, "l", x)
        self._inputs = x
        for i in range(self._lmax):
            for j in range(self._length):
                choice = []
                for obj in self._inputs:
                    try:
                        choice.append(obj[j % len(obj)])
                    except:
                        choice.append(obj)
                self._base_objs[i + j * self._lmax].setInputs(choice)

    def setVoice(self, x):
        pyoArgsAssert(self, "O", x)
        self._voice = x
        x, lmax = convertArgsToLists(x)
        for i, obj in enumerate(self._base_objs):
            obj.setVoice(wrap(x, i // self._length))

    def setMode(self, x):
        pyoArgsAssert(self, "i", x)
        self._mode = x
        [obj.setMode(x) for obj in self._base_objs]

    @property
    def inputs(self):
        return self._inputs

    @inputs.setter
    def inputs(self, x):
        self.setInputs(x)

    @property
    def voice(self):
        return self._voice

    @voice.setter
    def voice(self, x):
        self.setVoice(x)


class VoiceManager(PyoObject):

    def __init__(self, input, triggers=None, mul=1, add=0):
        # pyoArgsAssert(self, "ooOO", input, triggers, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._triggers = triggers
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        if triggers is not None:
            if type(triggers) == list:
                try:
                    t_streams = [obj[0] for obj in triggers]
                except TypeError:
                    t_streams = triggers
            elif isinstance(triggers, PyoObject):
                t_streams = triggers.getBaseObjects()
            else:
                t_streams = None
        else:
            t_streams = None
        self._base_objs = [
            VoiceManager_base(wrap(in_fader, i), t_streams, wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setInput(self, x, fadetime=0.05):
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setTriggers(self, x):
        # pyoArgsAssert(self, "o", x)
        self._triggers = x
        if x is not None:
            if type(x) == list:
                try:
                    t_streams = [obj[0] for obj in x]
                except TypeError:
                    t_streams = x
            elif isinstance(x, PyoObject):
                t_streams = x.getBaseObjects()
            else:
                t_streams = None
        [obj.setTriggers(t_streams) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def triggers(self):
        return self._triggers

    @triggers.setter
    def triggers(self, x):
        self.setTriggers(x)


class Mixer(PyoObject):

    def __init__(self, outs=2, chnls=1, time=0.025, mul=1, add=0):
        pyoArgsAssert(self, "IInOO", outs, chnls, time, mul, add)
        PyoObject.__init__(self, mul, add)
        self._outs = outs
        self._chnls = chnls
        self._time = time
        self._inputs = {}
        time, mul, add, lmax = convertArgsToLists(time, mul, add)
        self._base_players = [Mixer_base(outs, wrap(time, i)) for i in range(chnls)]
        self._base_objs = [
            MixerVoice_base(self._base_players[j], i, wrap(mul, i), wrap(add, i))
            for i in range(outs)
            for j in range(chnls)
        ]
        self._init_play()

    def __getitem__(self, x):
        if type(x) == slice:
            return [
                self._base_objs[j * self._chnls + i]
                for j in range(x.start or 0, x.stop or sys.maxsize, x.step or 1)
                for i in range(self._chnls)
            ]
        elif x < len(self._base_objs):
            return [self._base_objs[x * self._chnls + i] for i in range(self._chnls)]
        else:
            print("'x' too large!")

    def setTime(self, x):
        pyoArgsAssert(self, "n", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def addInput(self, voice, input):
        pyoArgsAssert(self, "o", input)
        if voice is None:
            voice = get_random_integer(mx=32767)
            while voice in self._inputs:
                voice = get_random_integer(mx=32767)
        if voice in self._inputs:
            print("Mixer has already a key named %s" % voice, file=sys.stderr)
            return
        self._inputs[voice] = input
        input, lmax = convertArgsToLists(input)
        [obj.addInput(str(voice), wrap(input, i)) for i, obj in enumerate(self._base_players)]
        return voice

    def delInput(self, voice):
        if voice in self._inputs:
            del self._inputs[voice]
            [obj.delInput(str(voice)) for i, obj in enumerate(self._base_players)]

    def clear(self):
        for voice in list(self._inputs.keys()):
            del self._inputs[voice]
            [obj.delInput(str(voice)) for i, obj in enumerate(self._base_players)]

    def setAmp(self, vin, vout, amp):
        if vin in self._inputs and vout < self._outs:
            [obj.setAmp(str(vin), vout, amp) for i, obj in enumerate(self._base_players)]

    def getChannels(self):
        return self._inputs

    def getKeys(self):
        return list(self._inputs.keys())

    @property
    def time(self):
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)
