"""
Objects designed to create parameter's control at audio rate.

These objects can be used to create envelopes, line segments
and conversion from python number to audio signal.

The audio streams of these objects can't be sent to the output
soundcard.

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
from ._widgets import createGraphWindow

######################################################################
### Controls
######################################################################
class Fader(PyoObject):
    """
    Fadein - fadeout envelope generator.

    Generate an amplitude envelope between 0 and 1 with control on fade
    times and total duration of the envelope.

    The play() method starts the envelope and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        fadein: float, optional
            Rising time of the envelope in seconds. Defaults to 0.01.
        fadeout: float, optional
            Falling time of the envelope in seconds. Defaults to 0.1.
        dur: float, optional
            Total duration of the envelope in seocnds. Defaults to 0,
            which means wait for the stop() method to start the fadeout.

    .. note::

        The out() method is bypassed. Fader's signal can not be sent to audio outs.

        The play() method starts the envelope.

        The stop() method calls the envelope's release phase if `dur` = 0.

        As of version 0.8.0, exponential or logarithmic envelopes can be created
        with the exponent factor (see setExp() method).

        As of version 0.9.2, Fader will send a trigger signal at the end of the playback.
        User can retreive the trigger streams by calling obj['trig'].
        Useful to synchronize other processes.

    .. seealso::

        :py:class:`Adsr`, :py:class:`Linseg`, :py:class:`Expseg`

    >>> s = Server().boot()
    >>> s.start()
    >>> f = Fader(fadein=0.5, fadeout=0.5, dur=2, mul=.5)
    >>> a = BrownNoise(mul=f).mix(2).out()
    >>> def repeat():
    ...     f.play()
    >>> pat = Pattern(function=repeat, time=2).play()

    """

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
        """
        Replace the `fadein` attribute.

        :Args:

            x: float
                new `fadein` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._fadein = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadein(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setFadeout(self, x):
        """
        Replace the `fadeout` attribute.

        :Args:

            x: float
                new `fadeout` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._fadeout = x
        x, lmax = convertArgsToLists(x)
        [obj.setFadeout(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float
                new `dur` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        """
        Sets an exponent factor to create exponential or logarithmic envelope.

        The default value is 1.0, which means linear segments. A value
        higher than 1.0 will produce exponential segments while a value
        between 0 and 1 will produce logarithmic segments. Must be > 0.0.

        :Args:

            x: float
                new `exp` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 10.0, "lin", "fadein", self._fadein, dataOnly=True),
            SLMap(0, 10.0, "lin", "fadeout", self._fadeout, dataOnly=True),
            SLMap(0, 20.0, "lin", "dur", self._dur, dataOnly=True),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def fadein(self):
        """float. Rising time of the envelope in seconds."""
        return self._fadein

    @fadein.setter
    def fadein(self, x):
        self.setFadein(x)

    @property
    def fadeout(self):
        """float. Falling time of the envelope in seconds."""
        return self._fadeout

    @fadeout.setter
    def fadeout(self, x):
        self.setFadeout(x)

    @property
    def dur(self):
        """float. Total duration of the envelope."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def exp(self):
        """float. Exponent factor of the envelope."""
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class Adsr(PyoObject):
    """
    Attack - Decay - Sustain - Release envelope generator.

    Calculates the classical ADSR envelope using linear segments.
    Duration can be set to 0 to give an infinite sustain. In this
    case, the stop() method calls the envelope release part.

    The play() method starts the envelope and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        attack: float, optional
            Duration of the attack phase in seconds. Defaults to 0.01.
        decay: float, optional
            Duration of the decay in seconds. Defaults to 0.05.
        sustain: float, optional
            Amplitude of the sustain phase. Defaults to 0.707.
        release: float, optional
            Duration of the release in seconds. Defaults to 0.1.
        dur: float, optional
            Total duration of the envelope in seconds. Defaults to 0,
            which means wait for the stop() method to start the release phase.


    .. note::

        The out() method is bypassed. Adsr's signal can not be sent to audio outs.

        The play() method starts the envelope.

        The stop() method calls the envelope's release phase if `dur` = 0.

        As of version 0.8.0, exponential or logarithmic envelopes can be created
        with the exponent factor (see setExp() method).

        As of version 0.9.2, Adsr will send a trigger signal at the end of the playback.
        User can retreive the trigger streams by calling obj['trig'].
        Useful to synchronize other processes.

    .. seealso::

        :py:class:`Fader`, :py:class:`Linseg`, :py:class:`Expseg`

    >>> s = Server().boot()
    >>> s.start()
    >>> f = Adsr(attack=.01, decay=.2, sustain=.5, release=.1, dur=2, mul=.5)
    >>> a = BrownNoise(mul=f).mix(2).out()
    >>> def repeat():
    ...     f.play()
    >>> pat = Pattern(function=repeat, time=2).play()

    """

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
        """
        Replace the `attack` attribute.

        :Args:

            x: float
                new `attack` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.

        :Args:

            x: float
                new `decay` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x: float
                new `sustain` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x: float
                new `sustain` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._release = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setDur(self, x):
        """
        Replace the `dur` attribute.

        :Args:

            x: float
                new `dur` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._dur = x
        x, lmax = convertArgsToLists(x)
        [obj.setDur(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        """
        Sets an exponent factor to create exponential or logarithmic envelope.

        The default value is 1.0, which means linear segments. A value
        higher than 1.0 will produce exponential segments while a value
        between 0 and 1 will produce logarithmic segments. Must be > 0.0.

        :Args:

            x: float
                new `exp` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0, 5, "lin", "attack", self._attack, dataOnly=True),
            SLMap(0, 5, "lin", "decay", self._decay, dataOnly=True),
            SLMap(0, 1, "lin", "sustain", self._sustain, dataOnly=True),
            SLMap(0, 10, "lin", "release", self._release, dataOnly=True),
            SLMap(0, 20.0, "lin", "dur", self._dur, dataOnly=True),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def attack(self):
        """float. Duration of the attack phase in seconds."""
        return self._attack

    @attack.setter
    def attack(self, x):
        self.setAttack(x)

    @property
    def decay(self):
        """float. Duration of the decay phase in seconds."""
        return self._decay

    @decay.setter
    def decay(self, x):
        self.setDecay(x)

    @property
    def sustain(self):
        """float. Amplitude of the sustain phase."""
        return self._sustain

    @sustain.setter
    def sustain(self, x):
        self.setSustain(x)

    @property
    def release(self):
        """float. Duration of the release phase in seconds."""
        return self._release

    @release.setter
    def release(self, x):
        self.setRelease(x)

    @property
    def dur(self):
        """float. Total duration of the envelope."""
        return self._dur

    @dur.setter
    def dur(self, x):
        self.setDur(x)

    @property
    def exp(self):
        """float. Exponent factor of the envelope."""
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class Linseg(PyoObject):
    """
    Draw a series of line segments between specified break-points.

    The play() method starts the envelope and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        list: list of tuples
            Points used to construct the line segments. Each tuple is a
            new point in the form (time, value).

            Times are given in seconds and must be in increasing order.
        loop: boolean, optional
            Looping mode. Defaults to False.
        initToFirstVal: boolean, optional
            If True, audio buffer will be filled at initialization with the
            first value of the line. Defaults to False.

    .. note::

        The out() method is bypassed. Linseg's signal can not be sent to audio outs.

    .. seealso::

        :py:class:`Fader`, :py:class:`Adsr`, :py:class:`Expseg`

    >>> s = Server().boot()
    >>> s.start()
    >>> l = Linseg([(0,500),(.03,1000),(.1,700),(1,500),(2,500)], loop=True)
    >>> a = Sine(freq=l, mul=.3).mix(2).out()
    >>> # then call:
    >>> l.play()

    """

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
        """
        Replace the `list` attribute.

        :Args:

            x: list of tuples
                new `list` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._list = x
        if type(x[0]) != list:
            [obj.setList(x) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setList(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        """
        Alias for `setList` method.

        :Args:

            x: list of tuples
                new `list` attribute.

        """
        self.setList(x)

    def getPoints(self):
        return self._list

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        :Args:

            x: boolean
                new `loop` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def pause(self):
        """
        Toggles between play and stop mode without reset.

        """
        [obj.pause() for obj in self._base_objs]

    def clear(self):
        """
        Resets the internal audio buffer to 0.

        """
        [obj.clear() for obj in self._base_objs]

    def graph(self, xlen=None, yrange=None, title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            xlen: float, optional
                Set the maximum value of the X axis of the graph. If None, the
                maximum value is retrieve from the current list of points.
            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph. If
                None, min and max are retrieve from the current list of points.
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for the
        server GUI before showing the controller window.

        """
        if xlen is None:
            xlen = float(self._list[-1][0])
        else:
            xlen = float(xlen)
        if yrange is None:
            ymin = float(min([x[1] for x in self._list]))
            ymax = float(max([x[1] for x in self._list]))
            if ymin == ymax:
                yrange = (0, ymax)
            else:
                yrange = (ymin, ymax)
        createGraphWindow(self, 0, xlen, yrange, title, wxnoserver)

    @property
    def list(self):
        """float. List of points (time, value)."""
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)

    @property
    def loop(self):
        """boolean. Looping mode."""
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)


class Expseg(PyoObject):
    """
    Draw a series of exponential segments between specified break-points.

    The play() method starts the envelope and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        list: list of tuples
            Points used to construct the line segments. Each tuple is a
            new point in the form (time, value).

            Times are given in seconds and must be in increasing order.
        loop: boolean, optional
            Looping mode. Defaults to False.
        exp: float, optional
            Exponent factor. Used to control the slope of the curves.
            Defaults to 10.
        inverse: boolean, optional
            If True, downward slope will be inversed. Useful to create
            biexponential curves. Defaults to True.
        initToFirstVal: boolean, optional
            If True, audio buffer will be filled at initialization with the
            first value of the line. Defaults to False.

    .. note::

        The out() method is bypassed. Expseg's signal can not be sent to audio outs.

    .. seealso::

        :py:class:`Fader`, :py:class:`Adsr`, :py:class:`linseg`

    >>> s = Server().boot()
    >>> s.start()
    >>> l = Expseg([(0,500),(.03,1000),(.1,700),(1,500),(2,500)], loop=True)
    >>> a = Sine(freq=l, mul=.3).mix(2).out()
    >>> # then call:
    >>> l.play()

    """

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
        """
        Replace the `list` attribute.

        :Args:

            x: list of tuples
                new `list` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._list = x
        if type(x[0]) != list:
            [obj.setList(x) for i, obj in enumerate(self._base_objs)]
        else:
            [obj.setList(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        :Args:

            x: boolean
                new `loop` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        """
        Replace the `exp` attribute.

        :Args:

            x: float
                new `exp` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInverse(self, x):
        """
        Replace the `inverse` attribute.

        :Args:

            x: boolean
                new `inverse` attribute.

        """
        pyoArgsAssert(self, "b", x)
        self._inverse = x
        x, lmax = convertArgsToLists(x)
        [obj.setInverse(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def replace(self, x):
        """
        Alias for `setList` method.

        :Args:

            x: list of tuples
                new `list` attribute.

        """
        self.setList(x)

    def pause(self):
        """
        Toggles between play and stop mode without reset.

        """
        [obj.pause() for obj in self._base_objs]

    def clear(self):
        """
        Resets the internal audio buffer to 0.

        """
        [obj.clear() for obj in self._base_objs]

    def getPoints(self):
        return self._list

    def graph(self, xlen=None, yrange=None, title=None, wxnoserver=False):
        """
        Opens a grapher window to control the shape of the envelope.

        When editing the grapher with the mouse, the new set of points
        will be send to the object on mouse up.

        Ctrl+C with focus on the grapher will copy the list of points to the
        clipboard, giving an easy way to insert the new shape in a script.

        :Args:

            xlen: float, optional
                Set the maximum value of the X axis of the graph. If None, the
                maximum value is retrieve from the current list of points.
                Defaults to None.
            yrange: tuple, optional
                Set the min and max values of the Y axis of the graph. If
                None, min and max are retrieve from the current list of points.
                Defaults to None.
            title: string, optional
                Title of the window. If none is provided, the name of the
                class is used.
            wxnoserver: boolean, optional
                With wxPython graphical toolkit, if True, tells the
                interpreter that there will be no server window.

        If `wxnoserver` is set to True, the interpreter will not wait for the
        server GUI before showing the controller window.

        """
        if xlen is None:
            xlen = float(self._list[-1][0])
        else:
            xlen = float(xlen)
        if yrange is None:
            ymin = float(min([x[1] for x in self._list]))
            ymax = float(max([x[1] for x in self._list]))
            if ymin == ymax:
                yrange = (0, ymax)
            else:
                yrange = (ymin, ymax)
        createGraphWindow(self, 2, xlen, yrange, title, wxnoserver)

    @property
    def list(self):
        """float. List of points (time, value)."""
        return self._list

    @list.setter
    def list(self, x):
        self.setList(x)

    @property
    def loop(self):
        """boolean. Looping mode."""
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def exp(self):
        """float. Exponent factor."""
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)

    @property
    def inverse(self):
        """boolean. Inverse downward slope."""
        return self._inverse

    @inverse.setter
    def inverse(self, x):
        self.setInverse(x)


class SigTo(PyoObject):
    """
    Convert numeric value to PyoObject signal with portamento.

    When `value` is changed, a ramp is applied from the current
    value to the new value. Can be used with PyoObject to apply
    a linear portamento on an audio signal.

    :Parent: :py:class:`PyoObject`

    :Args:

        value: float or PyoObject
            Numerical value to convert.
        time: float or PyoObject, optional
            Ramp time, in seconds, to reach the new value. Defaults to 0.025.
        init: float, optional
            Initial value of the internal memory. Defaults to 0.

    .. note::

        The out() method is bypassed. SigTo's signal can not be sent to audio outs.

    >>> import random
    >>> s = Server().boot()
    >>> s.start()
    >>> fr = SigTo(value=200, time=0.5, init=200)
    >>> a = SineLoop(freq=fr, feedback=0.08, mul=.3).out()
    >>> b = SineLoop(freq=fr*1.005, feedback=0.08, mul=.3).out(1)
    >>> def pick_new_freq():
    ...     fr.value = random.randrange(200,501,50)
    >>> pat = Pattern(function=pick_new_freq, time=1).play()

    """

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
        """
        Changes the value of the signal stream.

        :Args:

            x: float or PyoObject
                Numerical value to convert.

        """
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        """
        Changes the ramp time of the object.

        :Args:

            x: float or PyoObject
                New ramp time in seconds.

        """
        pyoArgsAssert(self, "O", x)
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 10, "lin", "time", self._time)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def value(self):
        """float or PyoObject. Numerical value to convert."""
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)

    @property
    def time(self):
        """floator PyoObject. Ramp time in seconds."""
        return self._time

    @time.setter
    def time(self, x):
        self.setTime(x)
