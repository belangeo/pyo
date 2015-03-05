"""
Objects to retrieve Midi informations for a specific Midi port and channel.

Objects creates and returns audio streams from the value in their Midi input.

The audio streams of these objects are essentially intended to be
used as controls and can't be sent to the output soundcard.

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
from _core import *
from _maps import *

######################################################################
### MIDI
######################################################################
class Midictl(PyoObject):
    """
    Get the current value of a Midi controller.

    Get the current value of a controller and optionally map it
    inside a specified range.

    :Parent: :py:class:`PyoObject`

    :Args:

        ctlnumber : int
            Controller number.
        minscale : float, optional
            Low range value for mapping. Defaults to 0.
        maxscale : float, optional
            High range value for mapping. Defaults to 1.
        init : float, optional
            Initial value. Defaults to 0.
        channel : int, optional
            Midi channel. 0 means all channels. Defaults to 0.

    .. note::

        The out() method is bypassed. Midictl's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> m = Midictl(ctlnumber=[107,102], minscale=250, maxscale=1000)
    >>> p = Port(m, .02)
    >>> a = Sine(freq=p, mul=.3).out()
    >>> a1 = Sine(freq=p*1.25, mul=.3).out()
    >>> a2 = Sine(freq=p*1.5, mul=.3).out()

    """
    def __init__(self, ctlnumber, minscale=0, maxscale=1, init=0, channel=0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._ctlnumber = ctlnumber
        self._minscale = minscale
        self._maxscale = maxscale
        self._channel = channel
        ctlnumber, minscale, maxscale, init, channel, mul, add, lmax = convertArgsToLists(ctlnumber, minscale, maxscale, init, channel, mul, add)
        self._base_objs = [Midictl_base(wrap(ctlnumber,i), wrap(minscale,i), wrap(maxscale,i), wrap(init,i), wrap(channel,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setCtlNumber(self, x):
        """
        Replace the `ctlnumber` attribute.

        :Args:

            x : int
                new `ctlnumber` attribute.

        """
        self._ctlnumber = x
        x, lmax = convertArgsToLists(x)
        [obj.setCtlNumber(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMinScale(self, x):
        """
        Replace the `minscale` attribute.

        :Args:

            x : int
                new `minscale` attribute.

        """
        self._minscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMaxScale(self, x):
        """
        Replace the `maxscale` attribute.

        :Args:

            x : int
                new `maxscale` attribute.

        """
        self._maxscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setChannel(self, x):
        """
        Replace the `channel` attribute.

        :Args:

            x : int
                new `channel` attribute.

        """
        self._channel = x
        x, lmax = convertArgsToLists(x)
        [obj.setChannel(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setValue(self, x):
        """
        Reset audio stream to value in argument.

        :Args:

            x : float
                new current value.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInterpolation(self, x):
        """
        Activate/Deactivate interpolation. Activated by default.

        :Args:

            x : boolean
                True activates the interpolation, False deactivates it.

        """
        x, lmax = convertArgsToLists(x)
        [obj.setInterpolation(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def ctlnumber(self):
        """int. Controller number."""
        return self._ctlnumber
    @ctlnumber.setter
    def ctlnumber(self, x):
        self.setCtlNumber(x)

    @property
    def minscale(self):
        """float. Minimum value for scaling."""
        return self._minscale
    @minscale.setter
    def minscale(self, x):
        self.setMinScale(x)

    @property
    def maxscale(self):
        """float. Maximum value for scaling."""
        return self._maxscale
    @maxscale.setter
    def maxscale(self, x):
        self.setMaxScale(x)

    @property
    def channel(self):
        """int. Midi channel. 0 means all channels."""
        return self._channel
    @channel.setter
    def channel(self, x):
        self.setChannel(x)

class CtlScan(PyoObject):
    """
    Scan the Midi controller's number in input.

    Scan the Midi controller's number in input and send it to
    a standard python `function`. Useful to implement a MidiLearn
    algorithm.

    :Parent: :py:class:`PyoObject`

    :Args:

        function : Python function (can't be a list)
            Function to be called. The function must be declared
            with an argument for the controller number in input. Ex.:

            def ctl_scan(ctlnum):
                print ctlnum

        toprint : boolean, optional
            If True, controller number and value will be print to
            the console.

    .. note::

        The out() method is bypassed. CtlScan's signal can not be sent
        to audio outs.

    >>> s = Server()
    >>> s.setMidiInputDevice(0) # enter your device number (see pm_list_devices())
    >>> s.boot()
    >>> s.start()
    >>> def ctl_scan(ctlnum):
    ...     print ctlnum
    >>> a = CtlScan(ctl_scan)

    """
    def __init__(self, function, toprint=True):
        PyoObject.__init__(self)
        if not callable(function):
            print >> sys.stderr, 'TypeError: "function" argument of %s must be callable.\n' % self.__class__.__name__
            exit()
        self._function = WeakMethod(function)
        self._toprint = toprint
        self._base_objs = [CtlScan_base(self._function, self._toprint)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def reset(self):
        """
        Resets the scanner.

        """
        [obj.reset() for obj in self._base_objs]

    def setFunction(self, x):
        """
        Replace the `function` attribute.

        :Args:

            x : Python function
                new `function` attribute.

        """
        self._function = WeakMethod(x)
        [obj.setFunction(self._function) for i, obj in enumerate(self._base_objs)]

    def setToprint(self, x):
        """
        Replace the `toprint` attribute.

        :Args:

            x : int
                new `toprint` attribute.

        """
        self._toprint = x
        x, lmax = convertArgsToLists(x)
        [obj.setToprint(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def function(self):
        """Python function. Function to be called."""
        return self._function
    @function.setter
    def function(self, x):
        self.setFunction(x)
    @property
    def toprint(self):
        """boolean. If True, print values to the console."""
        return self._toprint
    @toprint.setter
    def toprint(self, x):
        self.setToprint(x)

class CtlScan2(PyoObject):
    """
    Scan the Midi channel and controller number in input.

    Scan the Midi channel and controller number in input and send them
    to a standard python `function`. Useful to implement a MidiLearn
    algorithm.

    :Parent: :py:class:`PyoObject`

    :Args:

        function : Python function (can't be a list)
            Function to be called. The function must be declared
            with two arguments, one for the controller number and
            one for the midi channel. Ex.:

            def ctl_scan(ctlnum, midichnl):
                print ctlnum, midichnl

        toprint : boolean, optional
            If True, controller number and value will be print to
            the console.

    .. note::

        The out() method is bypassed. CtlScan2's signal can not be sent
        to audio outs.

    >>> s = Server()
    >>> s.setMidiInputDevice(0) # enter your device number (see pm_list_devices())
    >>> s.boot()
    >>> s.start()
    >>> def ctl_scan(ctlnum, midichnl):
    ...     print ctlnum, midichnl
    >>> a = CtlScan2(ctl_scan)

    """
    def __init__(self, function, toprint=True):
        PyoObject.__init__(self)
        if not callable(function):
            print >> sys.stderr, 'TypeError: "function" argument of %s must be callable.\n' % self.__class__.__name__
            exit()
        self._function = WeakMethod(function)
        self._toprint = toprint
        self._base_objs = [CtlScan2_base(self._function, self._toprint)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def reset(self):
        """
        Resets the scanner.

        """
        [obj.reset() for obj in self._base_objs]

    def setFunction(self, x):
        """
        Replace the `function` attribute.

        :Args:

            x : Python function
                new `function` attribute.

        """
        self._function = WeakMethod(x)
        [obj.setFunction(self._function) for i, obj in enumerate(self._base_objs)]

    def setToprint(self, x):
        """
        Replace the `toprint` attribute.

        :Args:

            x : int
                new `toprint` attribute.

        """
        self._toprint = x
        x, lmax = convertArgsToLists(x)
        [obj.setToprint(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def function(self):
        """Python function. Function to be called."""
        return self._function
    @function.setter
    def function(self, x):
        self.setFunction(x)
    @property
    def toprint(self):
        """boolean. If True, print values to the console."""
        return self._toprint
    @toprint.setter
    def toprint(self, x):
        self.setToprint(x)

class Notein(PyoObject):
    """
    Generates Midi note messages.

    From a Midi device, takes the notes in the range defined with
    `first` and `last` parameters, and outputs up to `poly`
    noteon - noteoff streams in the `scale` format (Midi, hertz
    or transpo).

    :Parent: :py:class:`PyoObject`

    :Args:

        poly : int, optional
            Number of streams of polyphony generated. Defaults to 10.
        scale : int, optional
            Pitch output format.
                0. Midi
                1. Hertz
                2. transpo

            In the transpo mode, the default central key (the key where
            there is no transposition) is (`first` + `last`) / 2.

            The central key can be changed with the setCentralKey method.
        first : int, optional
            Lowest Midi value. Defaults to 0.
        last : int, optional
            Highest Midi value. Defaults to 127.
        channel : int, optional
            Midi channel. 0 means all channels. Defaults to 0.

    .. note::

        Pitch and velocity are two separated set of streams.
        The user should call :

        |  Notein['pitch'] to retrieve pitch streams.
        |  Notein['velocity'] to retrieve velocity streams.

        Velocity is automatically scaled between 0 and 1.

        Notein also outputs trigger streams on noteon and noteoff.
        These streams can be retrieved with :

        |  Notein['trigon'] to retrieve noteon trigger streams.
        |  Notein['trigoff'] to retrieve noteoff trigger streams.

        The out() method is bypassed. Notein's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'], mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * 1.005, mul=p).out()

    """
    def __init__(self, poly=10, scale=0, first=0, last=127, channel=0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._pitch_dummy = []
        self._velocity_dummy = []
        self._trigon_dummy = []
        self._trigoff_dummy = []
        self._poly = poly
        self._scale = scale
        self._first = first
        self._last = last
        self._channel = channel
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_handler = MidiNote_base(self._poly, self._scale, self._first, self._last, self._channel)
        self._base_objs = []
        self._trig_objs = []
        for i in range(lmax * poly):
            self._base_objs.append(Notein_base(self._base_handler, i, 0, 1, 0))
            self._base_objs.append(Notein_base(self._base_handler, i, 1, wrap(mul,i), wrap(add,i)))
            self._trig_objs.append(NoteinTrig_base(self._base_handler, i, 0, 1, 0))
            self._trig_objs.append(NoteinTrig_base(self._base_handler, i, 1, 1, 0))

    def __getitem__(self, str):
        if str == 'pitch':
            self._pitch_dummy.append(Dummy([self._base_objs[i*2] for i in range(self._poly)]))
            return self._pitch_dummy[-1]
        elif str == 'velocity':
            self._velocity_dummy.append(Dummy([self._base_objs[i*2+1] for i in range(self._poly)]))
            return self._velocity_dummy[-1]
        elif str == 'trigon':
            self._trigon_dummy.append(Dummy([self._trig_objs[i*2] for i in range(self._poly)]))
            return self._trigon_dummy[-1]
        elif str == 'trigoff':
            self._trigoff_dummy.append(Dummy([self._trig_objs[i*2+1] for i in range(self._poly)]))
            return self._trigoff_dummy[-1]

    def setChannel(self, x):
        """
        Replace the `channel` attribute.

        :Args:

            x : int
                new `channel` attribute.

        """
        self._channel = x
        self._base_handler.setChannel(x)

    def setCentralKey(self, x):
        """
        Set the midi key where there is no transposition.

        Used for transpo conversion. This value must be greater than or
        equal to `first` and lower than or equal to `last`.

        :Args:

            x : int
                new centralkey value.

        """
        self._base_handler.setCentralKey(x)

    def setStealing(self, x):
        """
        Activates the stealing mode if True. Defaults to False.

        In stealing mode, a new note will overwrite the oldest one
        according to the polyphony. In non-stealing mode, if the
        polyphony is already full, the new notes will be ignored.

        :Args:

            x : boolean
                True for stealing mode, False for non-stealing.

        """
        self._base_handler.setStealing(x)

    def get(self, identifier="pitch", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "pitch" or "velocity" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier : string {"pitch", "velocity"}
                Address string parameter identifying audio stream.
                Defaults to "pitch".
            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list.

                Otherwise, only the value of the first object's
                stream will be returned as a float.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def play(self, dur=0, delay=0):
        self._base_handler.play()
        return PyoObject.play(self, dur, delay)

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def stop(self):
        self._base_handler.stop()
        return PyoObject.stop(self)

    @property
    def channel(self):
        """int. Midi channel. 0 means all channels."""
        return self._channel
    @channel.setter
    def channel(self, x):
        self.setChannel(x)

class Bendin(PyoObject):
    """
    Get the current value of the pitch bend controller.

    Get the current value of the pitch bend controller and optionally
    maps it inside a specified range.

    :Parent: :py:class:`PyoObject`

    :Args:

        brange : float, optional
            Bipolar range of the pitch bend in semitones. Defaults to 2.
            -brange <= value < brange.
        scale : int, optional
            Output format. Defaults to 0.
                0. Midi
                1. transpo.

            The transpo mode is useful if you want to transpose values that
            are in a frequency (Hz) format.
        channel : int, optional
            Midi channel. 0 means all channels. Defaults to 0.

    .. note::

        The out() method is bypassed. Bendin's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> bend = Bendin(brange=2, scale=1)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'] * bend, mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * bend * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * bend * 1.005, mul=p).out()

    """
    def __init__(self, brange=2, scale=0, channel=0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._brange = brange
        self._scale = scale
        self._channel = channel
        brange, scale, channel, mul, add, lmax = convertArgsToLists(brange, scale, channel, mul, add)
        self._base_objs = [Bendin_base(wrap(brange,i), wrap(scale,i), wrap(channel,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setBrange(self, x):
        """
        Replace the `brange` attribute.

        :Args:

            x : int
                new `brange` attribute.

        """
        self._brange = x
        x, lmax = convertArgsToLists(x)
        [obj.setBrange(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        """
        Replace the `scale` attribute.

        :Args:

            x : int
                new `scale` attribute.

        """
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setChannel(self, x):
        """
        Replace the `channel` attribute.

        :Args:

            x : int
                new `channel` attribute.

        """
        self._channel = x
        x, lmax = convertArgsToLists(x)
        [obj.setChannel(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def brange(self):
        """float. Bipolar range of the pitch bend in semitones."""
        return self._brange
    @brange.setter
    def brange(self, x): self.setBrange(x)

    @property
    def scale(self):
        """int. Output format. 0 = Midi, 1 = transpo."""
        return self._scale
    @scale.setter
    def scale(self, x): self.setScale(x)

    @property
    def channel(self):
        """int. Midi channel. 0 means all channels."""
        return self._channel
    @channel.setter
    def channel(self, x): self.setChannel(x)

class Touchin(PyoObject):
    """
    Get the current value of an after-touch Midi controller.

    Get the current value of an after-touch Midi controller and optionally
    maps it inside a specified range.

    :Parent: :py:class:`PyoObject`

    :Args:

        minscale : float, optional
            Low range value for mapping. Defaults to 0.
        maxscale : float, optional
            High range value for mapping. Defaults to 1.
        init : float, optional
            Initial value. Defaults to 0.
        channel : int, optional
            Midi channel. 0 means all channels. Defaults to 0.

    .. note::

        The out() method is bypassed. Touchin's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> touch = Touchin(minscale=1, maxscale=2, init=1)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'] * touch, mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * touch * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * touch * 1.005, mul=p).out()

    """
    def __init__(self, minscale=0, maxscale=1, init=0, channel=0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._minscale = minscale
        self._maxscale = maxscale
        self._channel = channel
        minscale, maxscale, init, channel, mul, add, lmax = convertArgsToLists(minscale, maxscale, init, channel, mul, add)
        self._base_objs = [Touchin_base(wrap(minscale,i), wrap(maxscale,i), wrap(init,i), wrap(channel,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setMinScale(self, x):
        """
        Replace the `minscale` attribute.

        :Args:

            x : int
                new `minscale` attribute.

        """
        self._minscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setMinScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMaxScale(self, x):
        """
        Replace the `maxscale` attribute.

        :Args:

            x : int
                new `maxscale` attribute.

        """
        self._maxscale = x
        x, lmax = convertArgsToLists(x)
        [obj.setMaxScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setChannel(self, x):
        """
        Replace the `channel` attribute.

        :Args:

            x : int
                new `channel` attribute.

        """
        self._channel = x
        x, lmax = convertArgsToLists(x)
        [obj.setChannel(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def minscale(self):
        """float. Minimum value for scaling."""
        return self._minscale
    @minscale.setter
    def minscale(self, x):
        self.setMinScale(x)

    @property
    def maxscale(self):
        """float. Maximum value for scaling."""
        return self._maxscale
    @maxscale.setter
    def maxscale(self, x):
        self.setMaxScale(x)

    @property
    def channel(self):
        """int. Midi channel. 0 means all channels."""
        return self._channel
    @channel.setter
    def channel(self, x):
        self.setChannel(x)

class Programin(PyoObject):
    """
    Get the current value of a program change Midi controller.

    Get the current value of a program change Midi controller.

    :Parent: :py:class:`PyoObject`

    :Args:

        channel : int, optional
            Midi channel. 0 means all channels. Defaults to 0.

    .. note::

        The out() method is bypassed. Programin's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> pchg = Programin(mul=1./12, add=1)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'] * pchg, mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * pchg * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * pchg * 1.005, mul=p).out()

    """
    def __init__(self, channel=0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._channel = channel
        channel, mul, add, lmax = convertArgsToLists(channel, mul, add)
        self._base_objs = [Programin_base(wrap(channel,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setChannel(self, x):
        """
        Replace the `channel` attribute.

        :Args:

            x : int
                new `channel` attribute.

        """
        self._channel = x
        x, lmax = convertArgsToLists(x)
        [obj.setChannel(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def channel(self):
        """int. Midi channel. 0 means all channels."""
        return self._channel
    @channel.setter
    def channel(self, x):
        self.setChannel(x)

class MidiAdsr(PyoObject):
    """
    Midi triggered ADSR envelope generator.

    Calculates the classical ADSR envelope using linear segments.
    The envelope starts when it receives a positive value in input,
    this value is used as the peak amplitude of the envelope. The
    `sustain` parameter is a fraction of the peak value and sets
    the real sustain value. A 0 in input (note off) starts the
    release part of the envelope.

    :Parent: :py:class:`PyoObject`

    :Args:

        input : PyoObject
            Input signal used to trigger the envelope. A positive value
            sets the peak amplitude and starts the envelope. A 0 starts
            the release part of the envelope.
        attack : float, optional
            Duration of the attack phase in seconds. Defaults to 0.01.
        decay : float, optional
            Duration of the decay phase in seconds. Defaults to 0.05.
        sustain : float, optional
            Amplitude of the sustain phase, as a fraction of the peak
            amplitude at the start of the envelope. Defaults to 0.7.
        release : float, optional
            Duration of the release phase in seconds. Defaults to 0.1.

    .. note::

        The out() method is bypassed. MidiAdsr's signal can not be sent to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> mid = Notein(scale=1)
    >>> env = MidiAdsr(mid['velocity'], attack=.005, decay=.1, sustain=.4, release=1)
    >>> a = SineLoop(freq=mid['pitch'], feedback=.1, mul=env).out()
    >>> b = SineLoop(freq=mid['pitch']*1.005, feedback=.1, mul=env).out(1)

    """
    def __init__(self, input, attack=0.01, decay=0.05, sustain=0.7, release=0.1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._attack = attack
        self._decay = decay
        self._sustain = sustain
        self._release = release
        self._in_fader = InputFader(input)
        in_fader, attack, decay, sustain, release, mul, add, lmax = convertArgsToLists(self._in_fader, attack, decay, sustain, release, mul, add)
        self._base_objs = [MidiAdsr_base(wrap(in_fader,i), wrap(attack,i), wrap(decay,i), wrap(sustain,i), wrap(release,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x : PyoObject
                New signal used to trigger the envelope.
            fadetime : float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setAttack(self, x):
        """
        Replace the `attack` attribute.

        :Args:

            x : float
                new `attack` attribute.

        """
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.

        :Args:

            x : float
                new `decay` attribute.

        """
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x : float
                new `sustain` attribute.

        """
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x : float
                new `sustain` attribute.

        """
        self._release = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def attack(self):
        """float. Duration of the attack phase in seconds."""
        return self._attack
    @attack.setter
    def attack(self, x): self.setAttack(x)

    @property
    def decay(self):
        """float. Duration of the decay phase in seconds."""
        return self._decay
    @decay.setter
    def decay(self, x): self.setDecay(x)

    @property
    def sustain(self):
        """float. Amplitude of the sustain phase, as fraction of the peak amplitude."""
        return self._sustain
    @sustain.setter
    def sustain(self, x): self.setSustain(x)

    @property
    def release(self):
        """float. Duration of the release phase in seconds."""
        return self._release
    @release.setter
    def release(self, x): self.setRelease(x)

class MidiDelAdsr(PyoObject):
    """
    Midi triggered ADSR envelope generator with pre-delay.

    Calculates the classical ADSR envelope using linear segments.
    The envelope starts after `delay` seconds when it receives a
    positive value in input, this value is used as the peak amplitude
    of the envelope. The `sustain` parameter is a fraction of the
    peak value and sets the real sustain value. A 0 in input (note off)
    starts the release part of the envelope.

    :Parent: :py:class:`PyoObject`

    :Args:

        input : PyoObject
            Input signal used to trigger the envelope. A positive value
            sets the peak amplitude and starts the envelope. A 0 starts
            the release part of the envelope.
        delay : float, optional
            Duration of the delay phase, before calling the envelope
            in seconds. Defaults to 0.
        attack : float, optional
            Duration of the attack phase in seconds. Defaults to 0.01.
        decay : float, optional
            Duration of the decay phase in seconds. Defaults to 0.05.
        sustain : float, optional
            Amplitude of the sustain phase, as a fraction of the peak
            amplitude at the start of the envelope. Defaults to 0.7.
        release : float, optional
            Duration of the release phase in seconds. Defaults to 0.1.

    .. note::

        The out() method is bypassed. MidiDelAdsr's signal can not be sent
        to audio outs.

    >>> s = Server().boot()
    >>> s.start()
    >>> mid = Notein(scale=1)
    >>> env = MidiDelAdsr(mid['velocity'], delay=.25, attack=.005, decay=.1, sustain=.4, release=1)
    >>> a = SineLoop(freq=mid['pitch'], feedback=.1, mul=env).out()
    >>> b = SineLoop(freq=mid['pitch']*1.005, feedback=.1, mul=env).out(1)

    """
    def __init__(self, input, delay=0, attack=0.01, decay=0.05, sustain=0.7, release=0.1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._delay = delay
        self._attack = attack
        self._decay = decay
        self._sustain = sustain
        self._release = release
        self._in_fader = InputFader(input)
        in_fader, delay, attack, decay, sustain, release, mul, add, lmax = convertArgsToLists(self._in_fader, delay, attack, decay, sustain, release, mul, add)
        self._base_objs = [MidiDelAdsr_base(wrap(in_fader,i), wrap(delay,i), wrap(attack,i), wrap(decay,i), wrap(sustain,i), wrap(release,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        :Args:

            x : PyoObject
                New signal used to trigger the envelope.
            fadetime : float, optional
                Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setDelay(self, x):
        """
        Replace the `delay` attribute.

        :Args:

            x : float
                new `delay` attribute.

        """
        self._delay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDelay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setAttack(self, x):
        """
        Replace the `attack` attribute.

        :Args:

            x : float
                new `attack` attribute.

        """
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.

        :Args:

            x : float
                new `decay` attribute.

        """
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x : float
                new `sustain` attribute.

        """
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        """
        Replace the `sustain` attribute.

        :Args:

            x : float
                new `sustain` attribute.

        """
        self._release = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def delay(self):
        """float. Duration of the delay phase in seconds."""
        return self._delay
    @delay.setter
    def delay(self, x): self.setDelay(x)

    @property
    def attack(self):
        """float. Duration of the attack phase in seconds."""
        return self._attack
    @attack.setter
    def attack(self, x): self.setAttack(x)

    @property
    def decay(self):
        """float. Duration of the decay phase in seconds."""
        return self._decay
    @decay.setter
    def decay(self, x): self.setDecay(x)

    @property
    def sustain(self):
        """float. Amplitude of the sustain phase, as fraction of the peak amplitude."""
        return self._sustain
    @sustain.setter
    def sustain(self, x): self.setSustain(x)

    @property
    def release(self):
        """float. Duration of the release phase in seconds."""
        return self._release
    @release.setter
    def release(self, x): self.setRelease(x)