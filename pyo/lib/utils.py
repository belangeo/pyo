"""
Miscellaneous objects.

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
from ._core import *
from ._maps import *
import threading, time


class Clean_objects(threading.Thread):
    """
    Stops and deletes PyoObjects after a given amount of time.

    The start() method starts the thread timer (must be called).

    :Args:

        time: float
            Time, in seconds, to wait before calling stop on the given
            objects and deleting them.
        args: PyoObject(s)
            Objects to delete. As much as desired, separated by commas.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.5).mix(2)
    >>> b = Fader(fadein=.5, fadeout=1, dur=5).play()
    >>> c = Biquad(a, freq=500, q=2, mul=b).out()
    >>> dump = Clean_objects(6, a, b, c)
    >>> dump.start()

    """

    def __init__(self, time, *args):
        threading.Thread.__init__(self)
        self.t = time
        self.args = args

    def run(self):
        time.sleep(self.t)
        for arg in self.args:
            try:
                arg.stop()
            except:
                pass
        for arg in self.args:
            del arg


class Print(PyoObject):
    """
    Print PyoObject's current value.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to filter.
        method: int {0, 1}, optional
            There is two methods to set when a value is printed (Defaults to 0):
            0. at a periodic interval.
            1. everytime the value changed.
        interval: float, optional
            Interval, in seconds, between each print. Used by method 0.
            Defaults to 0.25.
        message: str, optional
            Message to print before the current value. Defaults to "".

    .. note::

        The out() method is bypassed. Print's signal can not be sent to
        audio outs.

        Print has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + '/transparent.aif', loop=True, mul=.3).out()
    >>> b = Follower(a)
    >>> p = Print(b, method=0, interval=.1, message="RMS")

    """

    def __init__(self, input, method=0, interval=0.25, message=""):
        pyoArgsAssert(self, "oins", input, method, interval, message)
        PyoObject.__init__(self)
        self._input = input
        self._method = method
        self._interval = interval
        self._message = message
        self._in_fader = InputFader(input)
        in_fader, method, interval, message, lmax = convertArgsToLists(self._in_fader, method, interval, message)
        self._base_objs = [
            Print_base(wrap(in_fader, i), wrap(method, i), wrap(interval, i), wrap(message, i)) for i in range(lmax)
        ]
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

    def setMethod(self, x):
        """
        Replace the `method` attribute.

        :Args:

            x: int {0, 1}
                New `method` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._method = x
        x, lmax = convertArgsToLists(x)
        [obj.setMethod(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInterval(self, x):
        """
        Replace the `interval` attribute.

        :Args:

            x: float
                New `interval` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._interval = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterval(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMessage(self, x):
        """
        Replace the `message` attribute.

        :Args:

            x: str
                New `message` attribute.

        """
        pyoArgsAssert(self, "s", x)
        self._message = x
        x, lmax = convertArgsToLists(x)
        [obj.setMessage(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    @property
    def input(self):
        """PyoObject. Input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def method(self):
        """int. Controls when a value is printed."""
        return self._method

    @method.setter
    def method(self, x):
        self.setMethod(x)

    @property
    def interval(self):
        """float. For method 0, interval, in seconds, between each print."""
        return self._interval

    @interval.setter
    def interval(self, x):
        self.setInterval(x)

    @property
    def message(self):
        """str. Message to print before the current value."""
        return self._message

    @message.setter
    def message(self, x):
        self.setMessage(x)


class Snap(PyoObject):
    """
    Snap input values on a user's defined midi scale.

    Snap takes an audio input of floating-point values from 0
    to 127 and output the nearest value in the `choice` parameter.
    `choice` can be defined on any number of octaves and the real
    snapping values will be automatically expended. The object
    will take care of the input octave range. According to `scale`
    parameter, output can be in midi notes, hertz or transposition
    factor (centralkey = 60).

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Incoming Midi notes as an audio stream.
        choice: list of floats
            Possible values, as midi notes, for output.
        scale: int {0, 1, 2}, optional
            Pitch output format.
                0. MIDI (default)
                1. Hertz
                2. transposition factor

            In the transpo mode, the central key (the key where there
            is no transposition) is 60.

    >>> s = Server().boot()
    >>> s.start()
    >>> wav = SquareTable()
    >>> env = CosTable([(0,0), (100,1), (500,.3), (8191,0)])
    >>> met = Metro(.125, 8).play()
    >>> amp = TrigEnv(met, table=env, mul=.2)
    >>> pit = TrigXnoiseMidi(met, dist=4, x1=20, mrange=(48,84))
    >>> hertz = Snap(pit, choice=[0,2,3,5,7,8,10], scale=1)
    >>> a = Osc(table=wav, freq=hertz, phase=0, mul=amp).out()

    """

    def __init__(self, input, choice, scale=0, mul=1, add=0):
        pyoArgsAssert(self, "oliOO", input, choice, scale, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._choice = choice
        self._scale = scale
        self._in_fader = InputFader(input)
        in_fader, scale, mul, add, lmax = convertArgsToLists(self._in_fader, scale, mul, add)
        if type(choice[0]) != list:
            self._base_objs = [
                Snap_base(wrap(in_fader, i), choice, wrap(scale, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
            ]
        else:
            choicelen = len(choice)
            lmax = max(choicelen, lmax)
            self._base_objs = [
                Snap_base(wrap(in_fader, i), wrap(choice, i), wrap(scale, i), wrap(mul, i), wrap(add, i))
                for i in range(lmax)
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

    def setChoice(self, x):
        """
        Replace the `choice` attribute.

        :Args:

            x: list of floats
                new `choice` attribute.

        """
        pyoArgsAssert(self, "l", x)
        self._choice = x
        [obj.setChoice(x) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        """
        Replace the `scale` attribute.

        Possible values are:
            0. Midi notes
            1. Hertz
            2. transposition factor (centralkey = 60)

        :Args:

            x: int {0, 1, 2}
                new `scale` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Audio signal to transform."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def choice(self):
        """list of floats. Possible values."""
        return self._choice

    @choice.setter
    def choice(self, x):
        self.setChoice(x)

    @property
    def scale(self):
        """int. Output format."""
        return self._scale

    @scale.setter
    def scale(self, x):
        self.setScale(x)


class Interp(PyoObject):
    """
    Interpolates between two signals.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            First input signal.
        input2: PyoObject
            Second input signal.
        interp: float or PyoObject, optional
            Averaging value. 0 means only first signal, 1 means only second
            signal. Default to 0.5.

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + '/accord.aif', speed=[.99,1], loop=True, mul=.3)
    >>> sf2 = SfPlayer(SNDS_PATH + '/transparent.aif', speed=[.99,1], loop=True, mul=.3)
    >>> lfo = Osc(table=SquareTable(20), freq=5, mul=.5, add=.5)
    >>> a = Interp(sf, sf2, lfo).out()

    """

    def __init__(self, input, input2, interp=0.5, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, input2, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._input2 = input2
        self._interp = interp
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, interp, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, interp, mul, add
        )
        self._base_objs = [
            Interp_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(interp, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
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

    def setInput2(self, x, fadetime=0.05):
        """
        Replace the `input2` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: float or PyoObject
                New `interp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.0, 1.0, "lin", "interp", self._interp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. First input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def input2(self):
        """PyoObject. Second input signal."""
        return self._input2

    @input2.setter
    def input2(self, x):
        self.setInput2(x)

    @property
    def interp(self):
        """float or PyoObject. Averaging value."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class SampHold(PyoObject):
    """
    Performs a sample-and-hold operation on its input.

    SampHold performs a sample-and-hold operation on its input according
    to the value of `controlsig`. If `controlsig` equals `value`, the input
    is sampled and held until next sampling.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal.
        controlsig: PyoObject
            Controls when to sample the signal.
        value: float or PyoObject, optional
            Sampling target value. Default to 0.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(500,1000)
    >>> b = Sine([3,4])
    >>> c = SampHold(input=a, controlsig=b, value=0)
    >>> d = Sine(c, mul=.2).out()

    """

    def __init__(self, input, controlsig, value=0.0, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, controlsig, value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._controlsig = controlsig
        self._value = value
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(controlsig)
        in_fader, in_fader2, value, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, value, mul, add
        )
        self._base_objs = [
            SampHold_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(value, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
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

    def setControlsig(self, x, fadetime=0.05):
        """
        Replace the `controlsig` attribute.

        :Args:

            x: PyoObject
                New control signal.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._controlsig = x
        self._in_fader2.setInput(x, fadetime)

    def setValue(self, x):
        """
        Replace the `value` attribute.

        :Args:

            x: float or PyoObject
                New `value` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def controlsig(self):
        """PyoObject. Control signal."""
        return self._controlsig

    @controlsig.setter
    def controlsig(self, x):
        self.setControlsig(x)

    @property
    def value(self):
        """float or PyoObject. Target value."""
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Record(PyoObject):
    """
    Writes input sound in an audio file on the disk.

    `input` parameter must be a valid PyoObject or an addition of
    PyoObjects, parameters can't be in list format.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to record.
        filename: string
            Full path of the file to create.
        chnls: int, optional
            Number of channels in the audio file. Defaults to 2.
        fileformat: int, optional
            Format type of the audio file. Defaults to 0.

            Record will first try to set the format from the filename extension.

            If it's not possible, it uses the fileformat parameter. Supported formats are:
                0. WAV - Microsoft WAV format (little endian) {.wav, .wave}
                1. AIFF - Apple/SGI AIFF format (big endian) {.aif, .aiff}
                2. AU - Sun/NeXT AU format (big endian) {.au}
                3. RAW - RAW PCM data {no extension}
                4. SD2 - Sound Designer 2 {.sd2}
                5. FLAC - FLAC lossless file format {.flac}
                6. CAF - Core Audio File format {.caf}
                7. OGG - Xiph OGG container {.ogg}
        sampletype: int, optional
            Bit depth encoding of the audio file.

            SD2 and FLAC only support 16 or 24 bit int. Supported types are:
                0. 16 bits int (default)
                1. 24 bits int
                2. 32 bits int
                3. 32 bits float
                4. 64 bits float
                5. U-Law encoded
                6. A-Law encoded
        buffering: int, optional
            Number of bufferSize to wait before writing samples to disk.

            High buffering uses more memory but improves performance.
            Defaults to 4.
        quality: float, optional
            The encoding quality value, between 0.0 (lowest quality) and
            1.0 (highest quality). This argument has an effect only with
            FLAC and OGG compressed formats. Defaults to 0.4.

    .. note::

        All parameters can only be set at intialization time.

        The stop() method must be called on the object to close the file
        properly.

        The out() method is bypassed. Record's signal can not be sent to
        audio outs.

        Record has no `mul` and `add` attributes.

    >>> s = Server().boot()
    >>> s.start()
    >>> from random import uniform
    >>> import os
    >>> t = HarmTable([1, 0, 0, .2, 0, 0, 0, .1, 0, 0, .05])
    >>> amp = Fader(fadein=.05, fadeout=2, dur=4, mul=.05).play()
    >>> osc = Osc(t, freq=[uniform(350,360) for i in range(10)], mul=amp).out()
    >>> home = os.path.expanduser('~')
    >>> # Records an audio file called "example_synth.aif" in the home folder
    >>> rec = Record(osc, filename=home+"/example_synth.aif", fileformat=1, sampletype=1)
    >>> clean = Clean_objects(4.5, rec)
    >>> clean.start()

    """

    def __init__(self, input, filename, chnls=2, fileformat=0, sampletype=0, buffering=4, quality=0.4):
        pyoArgsAssert(self, "oSIIIIN", input, filename, chnls, fileformat, sampletype, buffering, quality)
        PyoObject.__init__(self)
        self._input = input
        self._in_fader = InputFader(input)
        ext = filename.rsplit(".")
        if len(ext) >= 2:
            ext = ext[-1].lower()
            if ext in FILE_FORMATS:
                fileformat = FILE_FORMATS[ext]
            else:
                print("Warning: Unknown file extension. Using fileformat value.")
        else:
            print("Warning: Filename has no extension. Using fileformat value.")
        self._base_objs = [
            Record_base(
                self._in_fader.getBaseObjects(),
                stringencode(filename),
                chnls,
                fileformat,
                sampletype,
                buffering,
                quality,
            )
        ]
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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

    @property
    def input(self):
        """PyoObject. Input signal to filter."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Denorm(PyoObject):
    """
    Mixes low level noise to an input signal.

    Mixes low level (~1e-24 for floats, and ~1e-60 for doubles) noise to a an input signal.
    Can be used before IIR filters and reverbs to avoid denormalized numbers which may
    otherwise result in significantly increased CPU usage.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.

    >>> s = Server().boot()
    >>> s.start()
    >>> amp = Linseg([(0,0),(2,1),(4,0)], loop=True).play()
    >>> a = Sine(freq=[800,1000], mul=0.01*amp)
    >>> den = Denorm(a)
    >>> rev = Freeverb(den, size=.9).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [Denorm_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to filter."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class ControlRec(PyoObject):
    """
    Records control values and writes them in a text file.

    `input` parameter must be a valid PyoObject managing any number
    of streams, other parameters can't be in list format. The user
    must call the `write` method to create text files on the disk.

    Each line in the text files contains two values, the absolute time
    in seconds and the sampled value.

    The play() method starts the recording and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to sample.
        filename: string
            Full path (without extension) used to create the files.

            "_000" will be added to file's names with increasing digits
            according to the number of streams in input.

            The same filename can be passed to a ControlRead object to
            read all related files.
        rate: int, optional
            Rate at which the input values are sampled. Defaults to 1000.
        dur: float, optional
            Duration of the recording, in seconds. If 0.0, the recording
            won't stop until the end of the performance.

            If greater than 0.0, the `stop` method is automatically called
            at the end of the recording.

    .. note::

        All parameters can only be set at intialization time.

        The write() method must be called on the object to write the files
        on the disk.

        The out() method is bypassed. ControlRec's signal can not be sent to
        audio outs.

        ControlRec has no `mul` and `add` attributes.

    .. seealso::

        :py:class:`ControlRead`

    >>> s = Server().boot()
    >>> s.start()
    >>> rnds = Randi(freq=[1,2], min=200, max=400)
    >>> sines = SineLoop(freq=rnds, feedback=.05, mul=.2).out()
    >>> home = os.path.expanduser('~')
    >>> rec = ControlRec(rnds, home+"/test", rate=100, dur=4).play()
    >>> # call rec.write() to save "test_000" and "test_001" in the home directory.
    >>> def write_files():
    ...     sines.mul = 0
    ...     rec.write()
    >>> call = CallAfter(function=write_files, time=4.5)

    """

    def __init__(self, input, filename, rate=1000, dur=0.0):
        pyoArgsAssert(self, "oSIN", input, filename, rate, dur)
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._dur = dur
        self._in_fader = InputFader(input).stop()
        in_fader, lmax = convertArgsToLists(self._in_fader)
        self._base_objs = [ControlRec_base(wrap(in_fader, i), rate, dur) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def write(self):
        """
        Writes recorded values in text files on the disk.

        """
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f\n" % p) for p in obj.getData()]
            f.close()


class ControlRead(PyoObject):
    """
    Reads control values previously stored in text files.

    Read sampled sound from a table, with optional looping mode.

    :Parent: :py:class:`PyoObject`

    :Args:

        filename: string
            Full path (without extension) used to create the files.

            Usually the same filename as the one given to a ControlRec
            object to record automation.

            The directory will be scaned and all files
            named "filename_xxx" will add a new stream in the object.
        rate: int, optional
            Rate at which the values are sampled. Defaults to 1000.
        loop: boolean, optional
            Looping mode, False means off, True means on.
            Defaults to False.
        interp: int, optional
            Choice of the interpolation method.
                1. no interpolation
                2. linear (default)
                3. cosinus
                4. cubic

    .. note::

        ControlRead will send a trigger signal at the end of the playback if
        loop is off or any time it wraps around if loop is on. User can
        retreive the trigger streams by calling obj['trig']:

        >>> rnds = ControlRead(home+"/freq_auto", loop=True)
        >>> t = SndTable(SNDS_PATH+"/transparent.aif")
        >>> loop = TrigEnv(rnds["trig"], t, dur=[.2,.3,.4,.5], mul=.5).out()

        The out() method is bypassed. ControlRead's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`ControlRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> rnds = ControlRead(SNDS_PATH+"/ControlRead_example_test", rate=100, loop=True)
    >>> sines = SineLoop(freq=rnds, feedback=.05, mul=.15).out()

    """

    def __init__(self, filename, rate=1000, loop=False, interp=2, mul=1, add=0):
        pyoArgsAssert(self, "SIBIOO", filename, rate, loop, interp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._loop = loop
        self._interp = interp
        files = sorted([f for f in os.listdir(self._path) if self._name + "_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        for i in range(len(files)):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            values = [float(l.split()[1]) for l in f.readlines() if l.strip() != ""]
            f.close()
            self._base_objs.append(ControlRead_base(values, rate, loop, interp, wrap(mul, i), wrap(add, i)))
        self._trig_objs = Dummy([TriggerDummy_base(obj) for obj in self._base_objs])
        self._init_play()

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setRate(self, x):
        """
        Replace the `rate` attribute.

        :Args:

            x: int
                new `rate` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._rate = x
        x, lmax = convertArgsToLists(x)
        [obj.setRate(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

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

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        :Args:

            x: int {1, 2, 3, 4}
                new `interp` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def rate(self):
        """int. Sampling frequency in cycles per second."""
        return self._rate

    @rate.setter
    def rate(self, x):
        self.setRate(x)

    @property
    def loop(self):
        """boolean. Looping mode."""
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)

    @property
    def interp(self):
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp

    @interp.setter
    def interp(self, x):
        self.setInterp(x)


class NoteinRec(PyoObject):
    """
    Records Notein inputs and writes them in a text file.

    `input` parameter must be a Notein object managing any number
    of streams, other parameters can't be in list format. The user
    must call the `write` method to create text files on the disk.

    Each line in the text files contains three values, the absolute time
    in seconds, the Midi pitch and the normalized velocity.

    The play() method starts the recording and is not called at the
    object creation time.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: Notein
            Notein signal to sample.
        filename: string
            Full path (without extension) used to create the files.

            "_000" will be added to file's names with increasing digits
            according to the number of streams in input.

            The same filename can be passed to a NoteinRead object to read
            all related files.

    .. note::

        All parameters can only be set at intialization time.

        The `write` method must be called on the object to write the files
        on the disk.

        The out() method is bypassed. NoteinRec's signal can not be sent to
        audio outs.

        NoteinRec has no `mul` and `add` attributes.

    .. seealso::

        :py:class:`NoteinRead`

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=2)
    >>> home = os.path.expanduser('~')
    >>> rec = NoteinRec(notes, home+"/test").play()
    >>> # call rec.write() to save "test_000" and "test_001" in the home directory.

    """

    def __init__(self, input, filename):
        pyoArgsAssert(self, "oS", input, filename)
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._in_pitch = self._input["pitch"]
        self.in_velocity = self._input["velocity"]
        in_pitch, in_velocity, lmax = convertArgsToLists(self._in_pitch, self.in_velocity)
        self._base_objs = [NoteinRec_base(wrap(in_pitch, i), wrap(in_velocity, i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def write(self):
        """
        Writes recorded values in text files on the disk.

        """
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f %f\n" % p) for p in obj.getData()]
            f.close()


class NoteinRead(PyoObject):
    """
    Reads Notein values previously stored in text files.

    :Parent: :py:class:`PyoObject`

    :Args:

        filename: string
            Full path (without extension) used to create the files.

            Usually the same filename as the one given to a NoteinRec
            object to record automation.

            The directory will be scaned and all files
            named "filename_xxx" will add a new stream in the object.
        loop: boolean, optional
            Looping mode, False means off, True means on.
            Defaults to False.

    .. note::

        NoteinRead will send a trigger signal at the end of the playback if
        loop is off or any time it wraps around if loop is on. User can
        retreive the trigger streams by calling obj['trig']:

        >>> notes = NoteinRead(home+"/notes_rec", loop=True)
        >>> t = SndTable(SNDS_PATH+"/transparent.aif")
        >>> loop = TrigEnv(notes["trig"], t, dur=[.2,.3,.4,.5], mul=.25).out()

        The out() method is bypassed. NoteinRead's signal can not be sent to
        audio outs.

    .. seealso::

        :py:class:`NoteinRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = NoteinRead(SNDS_PATH+"/NoteinRead_example_test", loop=True)
    >>> amps = Port(notes['velocity'], 0.001, 0.45, mul=.3)
    >>> sines = SineLoop(freq=MToF(notes['pitch']), feedback=.05, mul=amps).out()

    """

    def __init__(self, filename, loop=False, mul=1, add=0):
        pyoArgsAssert(self, "SBOO", filename, loop, mul, add)
        PyoObject.__init__(self, mul, add)
        self._pitch_dummy = []
        self._velocity_dummy = []
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._loop = loop
        files = sorted([f for f in os.listdir(self._path) if self._name + "_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        _trig_objs_tmp = []
        self._poly = len(files)
        for i in range(self._poly):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            vals = [l.split() for l in f.readlines() if l.strip() != ""]
            timestamps = [float(v[0]) for v in vals]
            pitches = [float(v[1]) for v in vals]
            amps = [float(v[2]) for v in vals]
            f.close()
            self._base_objs.append(NoteinRead_base(pitches, timestamps, loop))
            self._base_objs.append(NoteinRead_base(amps, timestamps, loop, wrap(mul, i), wrap(add, i)))
            _trig_objs_tmp.append(TriggerDummy_base(self._base_objs[-1]))
        self._trig_objs = Dummy(_trig_objs_tmp)
        self._init_play()

    def __getitem__(self, str):
        if str == "trig":
            return self._trig_objs
        if str == "pitch":
            self._pitch_dummy.append(Dummy([self._base_objs[i * 2] for i in range(self._poly)]))
            return self._pitch_dummy[-1]
        if str == "velocity":
            self._velocity_dummy.append(Dummy([self._base_objs[i * 2 + 1] for i in range(self._poly)]))
            return self._velocity_dummy[-1]

    def get(self, identifier="pitch", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "pitch" or "velocity" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"pitch", "velocity"}
                Address string parameter identifying audio stream.
                Defaults to "pitch".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list.

                If False, only the value of the first object's stream
                will be returned as a float.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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

    @property
    def loop(self):
        """boolean. Looping mode."""
        return self._loop

    @loop.setter
    def loop(self, x):
        self.setLoop(x)


class DBToA(PyoObject):
    """
    Returns the amplitude equivalent of a decibel value.

    Returns the amplitude equivalent of a decibel value, 0 dB = 1.
    The `input` values are internally clipped to -120 dB so -120 dB
    returns 0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, decibel value.

    >>> s = Server().boot()
    >>> s.start()
    >>> # amplitude modulation 6 dB around -18 dB
    >>> db = Sine(freq=1, phase=[0,.5], mul=6, add=-18)
    >>> amp = DBToA(db)
    >>> b = SineLoop(freq=[350,400], feedback=.1, mul=amp).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [DBToA_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class AToDB(PyoObject):
    """
    Returns the decibel equivalent of an amplitude value.

    Returns the decibel equivalent of an amplitude value, 1 = 0 dB.
    The `input` values are internally clipped to 0.000001 so values
    less than or equal to 0.000001 return -120 dB.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, amplitude value.

    >>> s = Server().boot()
    >>> s.start()
    >>> # amplitude modulation of a notch around 1000 Hz, from -120 db to 0dB
    >>> amp = Sine([1,1.5], mul=.5, add=.5)
    >>> db = AToDB(amp)
    >>> a = PinkNoise(.2)
    >>> b = EQ(a, freq=1000, q=2, boost=db).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [AToDB_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class Scale(PyoObject):
    """
    Maps an input range of audio values to an output range.

    Scale maps an input range of audio values to an output range.
    The ranges can be specified with `min` and `max` reversed for
    invert-mapping. If specified, the mapping can also be exponential.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        inmin: float or PyoObject, optional
            Minimum input value. Defaults to 0.
        inmax: float or PyoObject, optional
            Maximum input value. Defaults to 1.
        outmin: float or PyoObject, optional
            Minimum output value. Defaults to 0.
        outmax: float or PyoObject, optional
            Maximum output value. Defaults to 1.
        exp: float or PyoObject, optional
            Exponent value, specifies the nature of the scaling curve.
            Values between 0 and 1 give a reversed curve.  Defaults to 1.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> met = Metro(.125, poly=2).play()
    >>> rnd = TrigRand(met, min=0, max=1, port=.005)
    >>> omlf = Sine(.5, mul=700, add=1000)
    >>> fr = Scale(rnd, inmin=0, inmax=1, outmin=250, outmax=omlf, exp=1)
    >>> amp = TrigEnv(met, table=HannTable(), dur=.25, mul=.2)
    >>> out = SineLoop(fr, feedback=.07, mul=amp).out()

    """

    def __init__(self, input, inmin=0, inmax=1, outmin=0, outmax=1, exp=1, mul=1, add=0):
        pyoArgsAssert(self, "oOOOOOOO", input, inmin, inmax, outmin, outmax, exp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._inmin = inmin
        self._inmax = inmax
        self._outmin = outmin
        self._outmax = outmax
        self._exp = exp
        self._in_fader = InputFader(input)
        in_fader, inmin, inmax, outmin, outmax, exp, mul, add, lmax = convertArgsToLists(
            self._in_fader, inmin, inmax, outmin, outmax, exp, mul, add
        )
        self._base_objs = [
            Scale_base(
                wrap(in_fader, i),
                wrap(inmin, i),
                wrap(inmax, i),
                wrap(outmin, i),
                wrap(outmax, i),
                wrap(exp, i),
                wrap(mul, i),
                wrap(add, i),
            )
            for i in range(lmax)
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

    def setInMin(self, x):
        """
        Replace the `inmin` attribute.

        :Args:

            x: float or PyoObject
                New `inmin` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._inmin = x
        x, lmax = convertArgsToLists(x)
        [obj.setInMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setInMax(self, x):
        """
        Replace the `inmax` attribute.

        :Args:

            x: float or PyoObject
                New `inmax` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._inmax = x
        x, lmax = convertArgsToLists(x)
        [obj.setInMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOutMin(self, x):
        """
        Replace the `outmin` attribute.

        :Args:

            x: float or PyoObject
                New `outmin` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._outmin = x
        x, lmax = convertArgsToLists(x)
        [obj.setOutMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setOutMax(self, x):
        """
        Replace the `outmax` attribute.

        :Args:

            x: float or PyoObject
                New `outmax` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._outmax = x
        x, lmax = convertArgsToLists(x)
        [obj.setOutMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setExp(self, x):
        """
        Replace the `exp` attribute.

        :Args:

            x: float or PyoObject
                New `exp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._exp = x
        x, lmax = convertArgsToLists(x)
        [obj.setExp(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 127.0, "lin", "inmin", self._inmin),
            SLMap(0.0, 127.0, "lin", "inmax", self._inmax),
            SLMap(0.0, 127.0, "lin", "outmin", self._outmin),
            SLMap(0.0, 127.0, "lin", "outmax", self._outmax),
            SLMap(1.0, 10.0, "lin", "exp", self._exp),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def inmin(self):
        """float or PyoObject. Minimum input value."""
        return self._inmin

    @inmin.setter
    def inmin(self, x):
        self.setInMin(x)

    @property
    def inmax(self):
        """float or PyoObject. Maximum input value."""
        return self._inmax

    @inmax.setter
    def inmax(self, x):
        self.setInMax(x)

    @property
    def outmin(self):
        """float or PyoObject. Minimum output value."""
        return self._outmin

    @outmin.setter
    def outmin(self, x):
        self.setOutMin(x)

    @property
    def outmax(self):
        """float or PyoObject. Maximum output value."""
        return self._outmax

    @outmax.setter
    def outmax(self, x):
        self.setOutMax(x)

    @property
    def exp(self):
        """float or PyoObject. Exponent value (nature of the scaling curve)."""
        return self._exp

    @exp.setter
    def exp(self, x):
        self.setExp(x)


class CentsToTranspo(PyoObject):
    """
    Returns the transposition factor equivalent of a given cents value.

    Returns the transposition factor equivalent of a given cents value, 0 cents = 1.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, cents value.

    >>> s = Server().boot()
    >>> s.start()
    >>> met = Metro(.125, poly=2).play()
    >>> cts = TrigRandInt(met, max=12, mul=100)
    >>> trans = CentsToTranspo(cts)
    >>> sf = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, speed=trans, mul=.25).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [CentsToTranspo_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class TranspoToCents(PyoObject):
    """
    Returns the cents value equivalent of a transposition factor.

    Returns the cents value equivalent of a transposition factor, 1 = 0 cents.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal, transposition factor.

    >>> s = Server().boot()
    >>> s.start()
    >>> met = Metro(.125, poly=2).play()
    >>> trans = TrigChoice(met, choice=[.25,.5,.5,.75,1,1.25,1.5])
    >>> semi = TranspoToCents(trans, mul=0.01)
    >>> sf = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.3).out()
    >>> harm = Harmonizer(sf, transpo=semi).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [TranspoToCents_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class MToF(PyoObject):
    """
    Returns the frequency (Hz) equivalent to a midi note.

    Returns the frequency (Hz) equivalent to a midi note,
    60 = 261.62556530066814 Hz.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal as midi note.

    >>> s = Server().boot()
    >>> s.start()
    >>> met = Metro(.125, poly=2).play()
    >>> mid = TrigChoice(met, choice=[60, 63, 67, 70], port=.005)
    >>> hz = MToF(mid)
    >>> syn = SineLoop(freq=hz, feedback=.07, mul=.2).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [MToF_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class FToM(PyoObject):
    """
    Returns the midi note equivalent to a frequency in Hz.

    Returns the midi note equivalent to a frequency in Hz,
    440.0 (hz) = 69.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal as frequency in Hz.

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine([0.2,0.25], mul=300, add=600)
    >>> src = SineLoop(freq=lfo, feedback=0.05)
    >>> hz = Yin(src, minfreq=100, maxfreq=1000, cutoff=500)
    >>> mid = FToM(hz)
    >>> fr = Snap(mid, choice=[0,2,5,7,9], scale=1)
    >>> freq = Port(fr, risetime=0.01, falltime=0.01)
    >>> syn = SineLoop(freq, feedback=0.05, mul=0.3).out()

    """

    def __init__(self, input, mul=1, add=0):
        pyoArgsAssert(self, "oOO", input, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [FToM_base(wrap(in_fader, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)]
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

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)


class MToT(PyoObject):
    """
    Returns the transposition factor equivalent to a midi note.

    Returns the transposition factor equivalent to a midi note. If the midi
    note equal the `centralkey` argument, the output is 1.0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal as midi note.
        centralkey: float, optional
            The midi note that returns a transposition factor of 1,
            that is to say no transposition. Defaults to 60.

    >>> s = Server().boot()
    >>> s.start()
    >>> tsnd = SndTable(SNDS_PATH+"/accord.aif")
    >>> tenv = CosTable([(0,0), (100,1), (1000,.5), (8192,0)])
    >>> met = Metro(.125, poly=2).play()
    >>> amp = TrigEnv(met, table=tenv, dur=.25, mul=.7)
    >>> mid = TrigChoice(met, choice=[43, 45, 60, 63], port=.0025)
    >>> sp = MToT(mid)
    >>> snd = Osc(tsnd, freq=tsnd.getRate()/sp, mul=amp).out()

    """

    def __init__(self, input, centralkey=60.0, mul=1, add=0):
        pyoArgsAssert(self, "onOO", input, centralkey, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._centralkey = centralkey
        self._in_fader = InputFader(input)
        in_fader, centralkey, mul, add, lmax = convertArgsToLists(self._in_fader, centralkey, mul, add)
        self._base_objs = [
            MToT_base(wrap(in_fader, i), wrap(centralkey, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
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

    def setCentralKey(self, x):
        """
        Replace the `centralkey` attribute.

        :Args:

            x: float
                New `centralkey` attribute.

        """
        pyoArgsAssert(self, "n", x)
        self._centralkey = x
        x, lmax = convertArgsToLists(x)
        [obj.setCentralKey(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def centralkey(self):
        """float. The midi note that returns no transposition."""
        return self._centralkey

    @centralkey.setter
    def centralkey(self, x):
        self.setCentralKey(x)


class Between(PyoObject):
    """
    Informs when an input signal is contained in a specified range.

    Outputs a value of 1.0 if the input signal is greater or egal
    than `min` and less than `max`. Otherwise, outputs a value of 0.0.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        min: float or PyoObject, optional
            Minimum range value. Defaults to 0.
        max: float or PyoObject, optional
            Maximum range value. Defaults to 1.

    >>> s = Server().boot()
    >>> s.start()
    >>> ph = Phasor(freq=[7,8])
    >>> tr = Between(ph, min=0, max=.25)
    >>> amp = Port(tr, risetime=0.002, falltime=0.002, mul=.2)
    >>> a = SineLoop(freq=[245,250], feedback=.1, mul=amp).out()

    """

    def __init__(self, input, min=-1.0, max=1.0, mul=1, add=0):
        pyoArgsAssert(self, "oOOOO", input, min, max, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [
            Between_base(wrap(in_fader, i), wrap(min, i), wrap(max, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
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

    def setMin(self, x):
        """
        Replace the `min` attribute.

        :Args:

            x: float or PyoObject
                New `min` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        :Args:

            x: float or PyoObject
                New `max` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "min", self._min),
            SLMap(0.0, 1.0, "lin", "max", self._max),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def min(self):
        """float or PyoObject. Minimum range value."""
        return self._min

    @min.setter
    def min(self, x):
        self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum range value."""
        return self._max

    @max.setter
    def max(self, x):
        self.setMax(x)


class TrackHold(PyoObject):
    """
    Performs a track-and-hold operation on its input.

    TrackHold lets pass the signal in `input` without modification but hold
    a sample according to the value of `controlsig`. If `controlsig` equals
    `value`, the input is sampled and held, otherwise, it passes thru.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal.
        controlsig: PyoObject
            Controls when to sample the signal.
        value: float or PyoObject, optional
            Sampling target value. Default to 0.0.

    >>> s = Server().boot()
    >>> s.start()
    >>> ph = Phasor([3,4])
    >>> lf = Sine(.2, mul=.5, add=.5)
    >>> th = TrackHold(lf, ph > 0.5, 1, mul=500, add=300)
    >>> a = Sine(th, mul=.3).out()

    """

    def __init__(self, input, controlsig, value=0.0, mul=1, add=0):
        pyoArgsAssert(self, "ooOOO", input, controlsig, value, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._controlsig = controlsig
        self._value = value
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(controlsig)
        in_fader, in_fader2, value, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, value, mul, add
        )
        self._base_objs = [
            TrackHold_base(wrap(in_fader, i), wrap(in_fader2, i), wrap(value, i), wrap(mul, i), wrap(add, i))
            for i in range(lmax)
        ]
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

    def setControlsig(self, x, fadetime=0.05):
        """
        Replace the `controlsig` attribute.

        :Args:

            x: PyoObject
                New control signal.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._controlsig = x
        self._in_fader2.setInput(x, fadetime)

    def setValue(self, x):
        """
        Replace the `value` attribute.

        :Args:

            x: float or PyoObject
                New `value` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def controlsig(self):
        """PyoObject. Control signal."""
        return self._controlsig

    @controlsig.setter
    def controlsig(self, x):
        self.setControlsig(x)

    @property
    def value(self):
        """float or PyoObject. Target value."""
        return self._value

    @value.setter
    def value(self, x):
        self.setValue(x)


class Resample(PyoObject):
    """
    Realtime upsampling or downsampling of an audio signal.

    This object should be used in the context of a resampling block
    created with the Server's methods `beginResamplingBlock` and
    `EndResamplingBlock`.

    If used inside the block, it will resample its input signal according
    to the resampling factor given to `beginResamplingFactor`. If the factor
    is a negative value, the new virtual sampling rate will be
    `current sr / abs(factor)`. If the factor is a postive value, the new
    virtual sampling rate will be `current sr * factor`.

    If used after `endResamplingBlock`, it will resample its input signal
    to the current sampling rate of the server.

    The `mode` argument specifies the interpolation/decimation mode used
    internally.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to resample.
        mode: int, optional
            The interpolation/decimation mode. Defaults to 1.
            For the upsampling process, possible values are:

            - 0: zero-padding
            - 1: sample-and-hold
            - 2 or higher: the formula `mode * resampling factor` gives
              the FIR lowpass kernel length used to interpolate.

            For the downsampling process, possible values are:

            - 0 or 1: discard extra samples
            - 2 or higher: the formula `mode * abs(resampling factor)`
              gives the FIR lowpass kernel length used for the decimation.

    >>> s = Server().boot()
    >>> s.start()
    >>> drv = Sine(.5, phase=[0, 0.5], mul=0.49, add=0.5)
    >>> sig = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True)
    >>> s.beginResamplingBlock(8)
    >>> sigup = Resample(sig, mode=32)
    >>> drvup = Resample(drv, mode=1)
    >>> disto = Disto(sigup, drive=drvup, mul=0.5)
    >>> s.endResamplingBlock()
    >>> sigdown = Resample(disto, mode=32, mul=0.4).out()

    """

    def __init__(self, input, mode=1, mul=1, add=0):
        pyoArgsAssert(self, "oiOO", input, mode, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._mode = mode
        _input, mode, mul, add, lmax = convertArgsToLists(input, mode, mul, add)
        self._base_objs = [
            Resample_base(wrap(_input, i), wrap(mode, i), wrap(mul, i), wrap(add, i)) for i in range(lmax)
        ]
        self._init_play()

    def setMode(self, x):
        """
        Replace the `mode` attribute.

        :Args:

            x: int
                New `mode` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    @property
    def mode(self):
        """int. The interpolation/decimation mode."""
        return self._mode

    @mode.setter
    def mode(self, x):
        self.setMode(x)
