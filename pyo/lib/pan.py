"""
Set of objects to manage audio voice routing and spread of a sound
signal into a new stereo or multi-channel sound field.

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
import sys
from ._core import *
from ._maps import *


class Pan(PyoObject):
    """
    Cosinus panner with control on the spread factor.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        outs: int, optional
            Number of channels on the panning circle. Available at
            initialization time only. Defaults to 2.
        pan: float or PyoObject
            Position of the sound on the panning circle, between 0 and 1.
            Defaults to 0.5.
        spread: float or PyoObject
            Amount of sound leaking to the surrounding channels,
            between 0 and 1. Defaults to 0.5.

    .. note::

        When used with two output channels, the algorithm used is the
        cosine/sine constant power panning law. The SPan object uses
        the square root of intensity constant power panning law.

    .. seealso::

        :py:class:`SPan`, :py:class:`Switch`, :py:class:`Selector`

    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.2)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = Pan(a, outs=2, pan=lfo).out()

    """

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
        [obj.setSpread(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapPan(self._pan), SLMap(0.0, 1.0, "lin", "spread", self._spread), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def pan(self):
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)

    @property
    def spread(self):
        """float or PyoObject. Amount of sound leaking to the surrounding channels."""
        return self._spread

    @spread.setter
    def spread(self, x):
        self.setSpread(x)


class SPan(PyoObject):
    """
    Simple equal power panner.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        outs: int, optional
            Number of channels on the panning circle. Available at
            initialization time only. Defaults to 2.
        pan: float or PyoObject
            Position of the sound on the panning circle, between 0 and 1.
            Defaults to 0.5.

    .. note::

        When used with two output channels, the algorithm used is the
        square root of intensity constant power panning law. The Pan
        object uses the cosine/sine constant power panning law.

    .. seealso::

        :py:class:`Pan`, :py:class:`Switch`, :py:class:`Selector`

    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.2)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = SPan(a, outs=2, pan=lfo).out()

    """

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
        self._map_list = [SLMapPan(self._pan), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def pan(self):
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan

    @pan.setter
    def pan(self, x):
        self.setPan(x)


class Switch(PyoObject):
    """
    Audio switcher.

    Switch takes an audio input and interpolates between multiple outputs.

    User can retrieve the different streams by calling the output number
    between brackets. obj[0] retrieve the first stream, obj[outs-1] the
    last one.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        outs: int, optional
            Number of outputs. Available at initialization time only.
            Defaults to 2.
        voice: float or PyoObject
            Voice position pointer, between 0 and (outs-1) / len(input).
            Defaults to 0.

    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", speed=[.999,1], loop=True, mul=.3)
    >>> lf = Sine(freq=.25, mul=1, add=1)
    >>> b = Switch(a, outs=6, voice=lf)
    >>> c = WGVerb(b[0:2], feedback=.8).out()
    >>> d = Disto(b[2:4], drive=.9, mul=.1).out()
    >>> e = Delay(b[4:6], delay=.2, feedback=.6).out()

    """

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

    def setVoice(self, x):
        """
        Replace the `voice` attribute.

        :Args:

            x: float or PyoObject
                new `voice` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._voice = x
        x, lmax = convertArgsToLists(x)
        [obj.setVoice(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, self._outs - 1, "lin", "voice", self._voice), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def voice(self):
        """float or PyoObject. Voice position pointer."""
        return self._voice

    @voice.setter
    def voice(self, x):
        self.setVoice(x)


class Selector(PyoObject):
    """
    Audio selector.

    Selector takes multiple PyoObjects in input and interpolates between
    them to generate a single output.

    :Parent: :py:class:`PyoObject`

    :Args:

        inputs: list of PyoObject
            Audio objects to interpolate from.
        voice: float or PyoObject, optional
            Voice position pointer, between 0 and len(inputs)-1.
            Defaults to 0.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", speed=[.999,1], loop=True, mul=.3)
    >>> b = Noise(mul=.1)
    >>> c = SfPlayer(SNDS_PATH + "/accord.aif", speed=[.999,1], loop=True, mul=.5)
    >>> lf = Sine(freq=.1, add=1)
    >>> d = Selector(inputs=[a,b,c], voice=lf).out()

    """

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
        """
        Replace the `inputs` attribute.

        :Args:

            x: list of PyoObject
                new `inputs` attribute.

        """
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
        """
        Replace the `voice` attribute.

        :Args:

            x: float or PyoObject
                new `voice` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._voice = x
        x, lmax = convertArgsToLists(x)
        for i, obj in enumerate(self._base_objs):
            obj.setVoice(wrap(x, i // self._length))

    def setMode(self, x):
        """
        Change the algorithm used to interpolate between inputs.

        if inputs are phase correlated you should use a linear fade.

        :Args:

            x: int {0, 1}
                If 0 (the default) the equal power law is used to
                interpolate bewtween sources. If 1, linear fade is
                used instead.

        """
        pyoArgsAssert(self, "i", x)
        self._mode = x
        [obj.setMode(x) for obj in self._base_objs]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, len(self._inputs) - 1, "lin", "voice", self._voice), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def inputs(self):
        """List of PyoObjects. Audio objects to interpolate from."""
        return self._inputs

    @inputs.setter
    def inputs(self, x):
        self.setInputs(x)

    @property
    def voice(self):
        """float or PyoObject. Voice position pointer."""
        return self._voice

    @voice.setter
    def voice(self, x):
        self.setVoice(x)


class VoiceManager(PyoObject):
    """
    Polyphony voice manager.

    A trigger in input ask the object for a voice number and the object returns
    the first free one. The voice number is then disable until a trig comes at
    the same position in the list of triggers given at the argument `triggers`.

    Usually, the trigger enabling the voice number will come from the process
    started with the object output. So, it's common to leave the `triggers`
    argument to None and set the list of triggers afterward with the `setTriggers`
    method. The maximum number of voices generated by the object is the length
    of the trigger list.

    If there is no free voice, the object outputs -1.0 continuously.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Trigger stream asking for new voice numbers.
        triggers: PyoObject or list of PyoObject, optional
            List of mono PyoObject sending triggers. Can be a multi-streams
            PyoObject but not a mix of both.

            Ordering in the list corresponds to voice numbers. Defaults to None.

    >>> s = Server().boot()
    >>> s.start()
    >>> env = CosTable([(0,0),(100,1),(500,.5),(8192,0)])
    >>> delta = RandDur(min=.05, max=.1)
    >>> vm = VoiceManager(Change(delta))
    >>> sel = Select(vm, value=[0,1,2,3])
    >>> pit = TrigChoice(sel, choice=[midiToHz(x) for x in [60,63,67,70,72]])
    >>> amp = TrigEnv(sel, table=env, dur=.5, mul=.25)
    >>> synth1 = SineLoop(pit, feedback=.07, mul=amp).out()
    >>> vm.setTriggers(amp["trig"])

    """

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

    def setTriggers(self, x):
        """
        Replace the `triggers` attribute.

        :Args:

            x: PyoObject or list of PyoObject
                New `triggers` attribute.

        """
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
        """PyoObject. Trigger stream asking for new voice numbers."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def triggers(self):
        """list of PyoObject. Trigger streams enabling voices."""
        return self._triggers

    @triggers.setter
    def triggers(self, x):
        self.setTriggers(x)


class Mixer(PyoObject):
    """
    Audio mixer.

    Mixer mixes multiple inputs to an arbitrary number of outputs
    with independant amplitude values per mixing channel and a
    user defined portamento applied on amplitude changes.

    :Parent: :py:class:`PyoObject`

    :Args:

        outs: int, optional
            Number of outputs of the mixer. Available at initialization
            time only. Defaults to 2.
        chnls: int, optional
            Number of channels per output. Available at initialization
            time only. Defaults to 1.
        time: float, optional
            Duration, in seconds, of a portamento applied on
            a new amplitude value for a mixing channel.
            Defaults to 0.025.

    .. note::

        User can retrieve each of the output channels by calling the Mixer
        object with the desired channel between square brackets (see example).

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.2)
    >>> b = FM(carrier=200, ratio=[.5013,.4998], index=6, mul=.2)
    >>> mm = Mixer(outs=3, chnls=2, time=.025)
    >>> fx1 = Disto(mm[0], drive=.9, slope=.9, mul=.1).out()
    >>> fx2 = Freeverb(mm[1], size=.8, damp=.8, mul=.5).out()
    >>> fx3 = Harmonizer(mm[2], transpo=1, feedback=.75, mul=.5).out()
    >>> mm.addInput(0, a)
    >>> mm.addInput(1, b)
    >>> mm.setAmp(0,0,.5)
    >>> mm.setAmp(0,1,.5)
    >>> mm.setAmp(1,2,.5)
    >>> mm.setAmp(1,1,.5)

    """

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
        """
        Sets the portamento duration in seconds.

        :Args:

            x: float
                New portamento duration.

        """
        pyoArgsAssert(self, "n", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def addInput(self, voice, input):
        """
        Adds an audio object in the mixer's inputs.

        This method returns the key (voice argument or generated key if voice=None).

        :Args:

            voice: int or string
                Key in the mixer dictionary for this input. If None, a unique key
                between 0 and 32767 will be automatically generated.
            input: PyoObject
                Audio object to add to the mixer.

        """
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
        """
        Removes an audio object from the mixer's inputs.

        :Args:

            voice: int or string
                Key in the mixer dictionary assigned to the input to remove.

        """
        if voice in self._inputs:
            del self._inputs[voice]
            [obj.delInput(str(voice)) for i, obj in enumerate(self._base_players)]

    def clear(self):
        """
        Remove all mixer's inputs at once.

        """
        for voice in list(self._inputs.keys()):
            del self._inputs[voice]
            [obj.delInput(str(voice)) for i, obj in enumerate(self._base_players)]

    def setAmp(self, vin, vout, amp):
        """
        Sets the amplitude of a mixing channel.

        :Args:

            vin: int or string
                Key in the mixer dictionary of the desired input.
            vout: int
                Ouput channel where to send the signal.
            amp: float
                Amplitude value for this mixing channel.

        """
        if vin in self._inputs and vout < self._outs:
            [obj.setAmp(str(vin), vout, amp) for i, obj in enumerate(self._base_players)]

    def getChannels(self):
        """
        Returns the Mixer's channels dictionary.

        """
        return self._inputs

    def getKeys(self):
        """
        Returns the list of current keys in the Mixer's channels dictionary.

        """
        return list(self._inputs.keys())

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 10, "lin", "time", self._time, dataOnly=True), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def time(self):
        """float. Portamento."""
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)
