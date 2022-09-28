"""
Fast Fourier Transform.

A Fast Fourier Transform (FFT) is an efficient algorithm to compute
the discrete Fourier transform (DFT) and its inverse (IFFT).

The objects below can be used to perform sound processing in the
spectral domain.

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


class FFT(PyoObject):
    """
    Fast Fourier Transform.

    FFT analyses an input signal and converts it into the spectral
    domain. Three audio signals are sent out of the object, the
    `real` part, from bin 0 (DC) to bin size/2 (Nyquist), the
    `imaginary` part, from bin 0 to bin size/2-1, and the bin
    number, an increasing count from 0 to size-1. `real` and
    `imaginary` buffer's left samples  up to size-1 are filled
    with zeros. See notes below for an example of how to retrieve
    each signal component.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        size: int {pow-of-two > 4}, optional
            FFT size. Must be a power of two greater than 4.
            The FFT size is the number of samples used in each
            analysis frame. Defaults to 1024.
        overlaps: int, optional
            The number of overlaped analysis block. Must be a
            positive integer. More overlaps can greatly improved
            sound quality synthesis but it is also more CPU
            expensive. Defaults to 4.
        wintype: int, optional
            Shape of the envelope used to filter each input frame.
            Possible shapes are :

            0. rectangular (no windowing)
            1. Hamming
            2. Hanning
            3. Bartlett (triangular)
            4. Blackman 3-term
            5. Blackman-Harris 4-term
            6. Blackman-Harris 7-term
            7. Tuckey (alpha = 0.66)
            8. Sine (half-sine window)

    .. note::

        FFT has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

        FFT has no `mul` and `add` attributes.

        Real, imaginary and bin_number parts are three separated set
        of audio streams. The user should call :

        |  FFT['real'] to retrieve the real part.
        |  FFT['imag'] to retrieve the imaginary part.
        |  FFT['bin'] to retrieve the bin number part.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.25).mix(2)
    >>> fin = FFT(a, size=1024, overlaps=4, wintype=2)
    >>> t = ExpTable([(0,0),(3,0),(10,1),(20,0),(30,.8),(50,0),(70,.6),(150,0),(512,0)], size=512)
    >>> amp = TableIndex(t, fin["bin"])
    >>> re = fin["real"] * amp
    >>> im = fin["imag"] * amp
    >>> fout = IFFT(re, im, size=1024, overlaps=4, wintype=2).mix(2).out()

    """

    def __init__(self, input, size=1024, overlaps=4, wintype=2):
        pyoArgsAssert(self, "oiIi", input, size, overlaps, wintype)
        PyoObject.__init__(self)
        self._real_dummy = []
        self._imag_dummy = []
        self._bin_dummy = []
        self._input = input
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        self._in_fader = InputFader(input)
        in_fader, size, wintype, lmax = convertArgsToLists(self._in_fader, size, wintype)
        self._base_players = []
        for j in range(overlaps):
            for i in range(lmax):
                hopsize = wrap(size, i) * j // overlaps
                self._base_players.append(FFTMain_base(wrap(in_fader, i), wrap(size, i), hopsize, wrap(wintype, i)))
        self._real_objs = []
        self._imag_objs = []
        self._bin_objs = []
        for j in range(len(self._base_players)):
            self._real_objs.append(FFT_base(wrap(self._base_players, j), 0, self._mul, self._add))
            self._imag_objs.append(FFT_base(wrap(self._base_players, j), 1, self._mul, self._add))
            self._bin_objs.append(FFT_base(wrap(self._base_players, j), 2, self._mul, self._add))
        self._base_objs = [Sig(0)]  # Dummy objs to prevent PyoObjectBase methods to fail.
        self._init_play()

    def __len__(self):
        return len(self._real_objs)

    def __getitem__(self, str):
        if str == "real":
            self._real_dummy.append(Dummy([obj for i, obj in enumerate(self._real_objs)]))
            return self._real_dummy[-1]
        if str == "imag":
            self._imag_dummy.append(Dummy([obj for i, obj in enumerate(self._imag_objs)]))
            return self._imag_dummy[-1]
        if str == "bin":
            self._bin_dummy.append(Dummy([obj for i, obj in enumerate(self._bin_objs)]))
            return self._bin_dummy[-1]

    def get(self, identifier="real", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "real", "imag" or "bin" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"real", "imag", "bin"}
                Address string parameter identifying audio stream.
                Defaults to "real".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

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

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._autoplay(dur, delay)
        self._in_fader.play(dur, delay)
        self._base_players = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._base_players)]
        self._real_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._real_objs)]
        self._imag_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._imag_objs)]
        self._bin_objs = [obj.play(wrap(dur, i), wrap(delay, i)) for i, obj in enumerate(self._bin_objs)]
        return self

    def stop(self, wait=0):
        self._autostop(wait)
        self._in_fader.stop(wait)
        [obj.stop(wait) for obj in self._base_players]
        [obj.stop(wait) for obj in self._real_objs]
        [obj.stop(wait) for obj in self._imag_objs]
        [obj.stop(wait) for obj in self._bin_objs]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

    def setSize(self, x):
        """
        Replace the `size` attribute.

        :Args:

            x: int
                new `size` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        poly = len(self._base_players) // self._overlaps
        for j in range(self._overlaps):
            for i in range(poly):
                hopsize = wrap(x, i) * j // self._overlaps
                self._base_players[j * poly + i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        """
        Replace the `wintype` attribute.

        :Args:

            x: int
                new `wintype` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def size(self):
        """int. FFT size."""
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        """int. Windowing method."""
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class IFFT(PyoObject):
    """
    Inverse Fast Fourier Transform.

    IFFT takes a signal in the spectral domain and converts it to a
    real audio signal using an inverse fast fourier transform.
    IFFT takes two signals in input, the `real` and `imaginary` parts
    of an FFT analysis and returns the corresponding real signal.
    These signals must correspond to `real` and `imaginary` parts
    from an FFT object.

    :Parent: :py:class:`PyoObject`

    :Args:

        inreal: PyoObject
            Input `real` signal.
        inimag: PyoObject
            Input `imaginary` signal.
        size: int {pow-of-two > 4}, optional
            FFT size. Must be a power of two greater than 4.
            The FFT size is the number of samples used in each
            analysis frame. This value must match the `size`
            attribute of the former FFT object. Defaults to 1024.
        overlaps: int, optional
            The number of overlaped analysis block. Must be a
            positive integer. More overlaps can greatly improved
            sound quality synthesis but it is also more CPU
            expensive. This value must match the `overlaps`
            atribute of the former FFT object. Defaults to 4.
        wintype: int, optional
            Shape of the envelope used to filter each output frame.
            Possible shapes are :

            0. rectangular (no windowing)
            1. Hamming
            2. Hanning
            3. Bartlett (triangular)
            4. Blackman 3-term
            5. Blackman-Harris 4-term
            6. Blackman-Harris 7-term
            7. Tuckey (alpha = 0.66)
            8. Sine (half-sine window)

    .. note::

        The number of streams in `inreal` and `inimag` attributes
        must be egal to the output of the former FFT object. In
        most case, it will be `channels of processed sound` * `overlaps`.

        The output of IFFT must be mixed to reconstruct the real
        signal from the overlapped streams. It is left to the user
        to call the mix(channels of the processed sound) method on
        an IFFT object.

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(.25).mix(2)
    >>> fin = FFT(a, size=1024, overlaps=4, wintype=2)
    >>> t = ExpTable([(0,0),(3,0),(10,1),(20,0),(30,.8),(50,0),(70,.6),(150,0),(512,0)], size=512)
    >>> amp = TableIndex(t, fin["bin"])
    >>> re = fin["real"] * amp
    >>> im = fin["imag"] * amp
    >>> fout = IFFT(re, im, size=1024, overlaps=4, wintype=2).mix(2).out()

    """

    def __init__(self, inreal, inimag, size=1024, overlaps=4, wintype=2, mul=1, add=0):
        pyoArgsAssert(self, "ooiIiOO", inreal, inimag, size, overlaps, wintype, mul, add)
        PyoObject.__init__(self, mul, add)
        self._inreal = inreal
        self._inimag = inimag
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        self._in_fader = InputFader(inreal)
        self._in_fader2 = InputFader(inimag)
        in_fader, in_fader2, size, wintype, mul, add, lmax = convertArgsToLists(
            self._in_fader, self._in_fader2, size, wintype, mul, add
        )
        self._base_objs = []
        ratio = lmax // overlaps
        for i in range(lmax):
            hopsize = wrap(size, i) * ((i // ratio) % overlaps) // overlaps
            self._base_objs.append(
                IFFT_base(
                    wrap(in_fader, i),
                    wrap(in_fader2, i),
                    wrap(size, i),
                    hopsize,
                    wrap(wintype, i),
                    wrap(mul, i),
                    wrap(add, i),
                )
            )
        self._init_play()

    def __len__(self):
        return len(self._inreal)

    def setInReal(self, x, fadetime=0.05):
        """
        Replace the `inreal` attribute.

        :Args:

            x: PyoObject
                New input `real` signal.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inreal = x
        self._in_fader.setInput(x, fadetime)

    def setInImag(self, x, fadetime=0.05):
        """
        Replace the `inimag` attribute.

        :Args:

            x: PyoObject
                New input `imag` signal.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inimag = x
        self._in_fader2.setInput(x, fadetime)

    def setSize(self, x):
        """
        Replace the `size` attribute.

        :Args:

            x: int
                new `size` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        ratio = len(self._base_objs) // self._overlaps
        for i, obj in enumerate(self._base_objs):
            hopsize = wrap(x, i) * ((i // ratio) % self._overlaps) // self._overlaps
            self._base_objs[i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        """
        Replace the `wintype` attribute.

        :Args:

            x: int
                new `wintype` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        [obj.setWinType(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def inreal(self):
        """PyoObject. Real input signal."""
        return self._inreal

    @inreal.setter
    def inreal(self, x):
        self.setInReal(x)

    @property
    def inimag(self):
        """PyoObject. Imaginary input signal."""
        return self._inimag

    @inimag.setter
    def inimag(self, x):
        self.setInImag(x)

    @property
    def size(self):
        """int. FFT size."""
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        """int. Windowing method."""
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)


class CarToPol(PyoObject):
    """
    Performs the cartesian to polar conversion.

    The Cartesian system locates points on a plane by measuring the  horizontal and
    vertical distances from an arbitrary origin to a point.  These are usually denoted
    as a pair of values (X,Y).

    The Polar system locates the point by measuring the straight line distance, usually
    denoted by R, from the origin to the point and the angle of an imaginary line from
    the origin to the point measured counterclockwise from the positive X axis.

    :Parent: :py:class:`PyoObject`

    :Args:

        inreal: PyoObject
            Real input signal.
        inimag: PyoObject
            Imaginary input signal.

    .. note::

        Polar coordinates can be retrieve by calling :

        |  CarToPol['mag'] to retrieve the magnitude part.
        |  CarToPol['ang'] to retrieve the angle part.

        CarToPol has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

    >>> s = Server().boot()
    >>> snd1 = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.7).mix(2)
    >>> snd2 = FM(carrier=[75,100,125,150], ratio=[.499,.5,.501,.502], index=20, mul=.1).mix(2)
    >>> fin1 = FFT(snd1, size=1024, overlaps=4)
    >>> fin2 = FFT(snd2, size=1024, overlaps=4)
    >>> # get magnitudes and phases of input sounds
    >>> pol1 = CarToPol(fin1["real"], fin1["imag"])
    >>> pol2 = CarToPol(fin2["real"], fin2["imag"])
    >>> # times magnitudes and adds phases
    >>> mag = pol1["mag"] * pol2["mag"] * 100
    >>> pha = pol1["ang"] + pol2["ang"]
    >>> # converts back to rectangular
    >>> car = PolToCar(mag, pha)
    >>> fout = IFFT(car["real"], car["imag"], size=1024, overlaps=4).mix(2).out()
    >>> s.start()

    """

    def __init__(self, inreal, inimag, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", inreal, inimag, mul, add)
        PyoObject.__init__(self, mul, add)
        self._mag_dummy = []
        self._ang_dummy = []
        self._inreal = inreal
        self._inimag = inimag
        self._in_fader = InputFader(inreal)
        self._in_fader2 = InputFader(inimag)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = []
        for i in range(lmax):
            self._base_objs.append(CarToPol_base(wrap(in_fader, i), wrap(in_fader2, i), 0, wrap(mul, i), wrap(add, i)))
            self._base_objs.append(CarToPol_base(wrap(in_fader, i), wrap(in_fader2, i), 1, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def __len__(self):
        return len(self._inreal)

    def __getitem__(self, str):
        if str == "mag":
            self._mag_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 0]))
            return self._mag_dummy[-1]
        if str == "ang":
            self._ang_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 1]))
            return self._ang_dummy[-1]

    def get(self, identifier="mag", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "mag" or "ang" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"mag", "ang"}
                Address string parameter identifying audio stream.
                Defaults to "mag".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInReal(self, x, fadetime=0.05):
        """
        Replace the `inreal` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inreal = x
        self._in_fader.setInput(x, fadetime)

    def setInImag(self, x, fadetime=0.05):
        """
        Replace the `inimag` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inimag = x
        self._in_fader2.setInput(x, fadetime)

    @property
    def inreal(self):
        """PyoObject. Real input signal."""
        return self._inreal

    @inreal.setter
    def inreal(self, x):
        self.setInReal(x)

    @property
    def inimag(self):
        """PyoObject. Imaginary input signal."""
        return self._inimag

    @inimag.setter
    def inimag(self, x):
        self.setInImag(x)


class PolToCar(PyoObject):
    """
    Performs the polar to cartesian conversion.

    The Polar system locates the point by measuring the straight line distance, usually
    denoted by R, from the origin to the point and the angle of an imaginary line from
    the origin to the point measured counterclockwise from the positive X axis.

    The Cartesian system locates points on a plane by measuring the  horizontal and
    vertical distances from an arbitrary origin to a point.  These are usually denoted
    as a pair of values (X,Y).

    :Parent: :py:class:`PyoObject`

    :Args:

        inmag: PyoObject
            Magintude input signal.
        inang: PyoObject
            Angle input signal.

    .. note::

        Cartesians coordinates can be retrieve by calling :

        |  PolToCar['real'] to retrieve the real part.
        |  CarToPol['imag'] to retrieve the imaginary part.

        PolToCar has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

    >>> s = Server().boot()
    >>> snd1 = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.7).mix(2)
    >>> snd2 = FM(carrier=[75,100,125,150], ratio=[.499,.5,.501,.502], index=20, mul=.1).mix(2)
    >>> fin1 = FFT(snd1, size=1024, overlaps=4)
    >>> fin2 = FFT(snd2, size=1024, overlaps=4)
    >>> # get magnitudes and phases of input sounds
    >>> pol1 = CarToPol(fin1["real"], fin1["imag"])
    >>> pol2 = CarToPol(fin2["real"], fin2["imag"])
    >>> # times magnitudes and adds phases
    >>> mag = pol1["mag"] * pol2["mag"] * 100
    >>> pha = pol1["ang"] + pol2["ang"]
    >>> # converts back to rectangular
    >>> car = PolToCar(mag, pha)
    >>> fout = IFFT(car["real"], car["imag"], size=1024, overlaps=4).mix(2).out()
    >>> s.start()

    """

    def __init__(self, inmag, inang, mul=1, add=0):
        pyoArgsAssert(self, "ooOO", inmag, inang, mul, add)
        PyoObject.__init__(self, mul, add)
        self._real_dummy = []
        self._imag_dummy = []
        self._inmag = inmag
        self._inang = inang
        self._in_fader = InputFader(inmag)
        self._in_fader2 = InputFader(inang)
        in_fader, in_fader2, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, mul, add)
        self._base_objs = []
        for i in range(lmax):
            self._base_objs.append(PolToCar_base(wrap(in_fader, i), wrap(in_fader2, i), 0, wrap(mul, i), wrap(add, i)))
            self._base_objs.append(PolToCar_base(wrap(in_fader, i), wrap(in_fader2, i), 1, wrap(mul, i), wrap(add, i)))
        self._init_play()

    def __len__(self):
        return len(self._inmag)

    def __getitem__(self, str):
        if str == "real":
            self._real_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 0]))
            return self._real_dummy[-1]
        if str == "imag":
            self._imag_dummy.append(Dummy([obj for i, obj in enumerate(self._base_objs) if i % 2 == 1]))
            return self._imag_dummy[-1]

    def get(self, identifier="real", all=False):
        """
        Return the first sample of the current buffer as a float.

        Can be used to convert audio stream to usable Python data.

        "real" or "imag" must be given to `identifier` to specify
        which stream to get value from.

        :Args:

            identifier: string {"real", "imag"}
                Address string parameter identifying audio stream.
                Defaults to "mag".
            all: boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]

    def setInMag(self, x, fadetime=0.05):
        """
        Replace the `inmag` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inmag = x
        self._in_fader.setInput(x, fadetime)

    def setInAng(self, x, fadetime=0.05):
        """
        Replace the `inang` attribute.

        :Args:

            x: PyoObject
                New signal to process.
            fadetime: float, optional
                Crossfade time between old and new input. Default to 0.05.

        """
        pyoArgsAssert(self, "oN", x, fadetime)
        self._inang = x
        self._in_fader2.setInput(x, fadetime)

    @property
    def inmag(self):
        """PyoObject. Magnitude input signal."""
        return self._inmag

    @inmag.setter
    def inmag(self, x):
        self.setInMag(x)

    @property
    def inang(self):
        """PyoObject. Angle input signal."""
        return self._inang

    @inang.setter
    def inang(self, x):
        self.setInAng(x)


class FrameDelta(PyoObject):
    """
    Computes the phase differences between successive frames.

    The difference between the phase values of successive FFT frames for a given bin
    determines the exact frequency of the energy centered in that bin. This is often
    known as the phase difference (and sometimes also referred to as phase derivative
    or instantaneous frequency if it's been subjected to a few additional calculations).

    In order to reconstruct a plausible playback of re-ordered FFT frames, we need to
    calculate the phase difference between successive frames and use it to construct a
    `running phase` (by simply summing the successive differences with FrameAccum) for
    the output FFT frames.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Phase input signal, usually from an FFT analysis.
        framesize: int, optional
            Frame size in samples. Usually the same as the FFT size.
            Defaults to 1024.
        overlaps: int, optional
            Number of overlaps in incomming signal. Usually the same
            as the FFT overlaps. Defaults to 4.

    .. note::

        FrameDelta has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SNDS_PATH + '/transparent.aif'
    >>> size, hop = 1024, 256
    >>> nframes = int(sndinfo(snd)[0] / size) + 1
    >>> a = SfPlayer(snd, mul=.3)
    >>> m_mag = [NewMatrix(width=size, height=nframes) for i in range(4)]
    >>> m_pha = [NewMatrix(width=size, height=nframes) for i in range(4)]
    >>> fin = FFT(a, size=size, overlaps=4)
    >>> pol = CarToPol(fin["real"], fin["imag"])
    >>> delta = FrameDelta(pol["ang"], framesize=size, overlaps=4)
    >>> m_mag_rec = MatrixRec(pol["mag"], m_mag, 0, [i*hop for i in range(4)]).play()
    >>> m_pha_rec = MatrixRec(delta, m_pha, 0, [i*hop for i in range(4)]).play()
    >>> m_mag_read = MatrixPointer(m_mag, fin["bin"]/size, Sine(freq=0.25, mul=.5, add=.5))
    >>> m_pha_read = MatrixPointer(m_pha, fin["bin"]/size, Sine(freq=0.25, mul=.5, add=.5))
    >>> accum = FrameAccum(m_pha_read, framesize=size, overlaps=4)
    >>> car = PolToCar(m_mag_read, accum)
    >>> fout = IFFT(car["real"], car["imag"], size=size, overlaps=4).mix(1).out()
    >>> right = Delay(fout, delay=0.013).out(1)

    """

    def __init__(self, input, framesize=1024, overlaps=4, mul=1, add=0):
        pyoArgsAssert(self, "oiiOO", input, framesize, overlaps, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(FrameDeltaMain_base(objs_list, wrap(framesize, j), wrap(overlaps, j)))
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(
                FrameDelta_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i))
            )
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

    def setFrameSize(self, x):
        """
        Replace the `framesize` attribute.

        :Args:

            x: int
                new `framesize` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        """PyoObject. Phase input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        """PyoObject. Frame size in samples."""
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)


class FrameAccum(PyoObject):
    """
    Accumulates the phase differences between successive frames.

    The difference between the phase values of successive FFT frames for a given bin
    determines the exact frequency of the energy centered in that bin. This is often
    known as the phase difference (and sometimes also referred to as phase derivative
    or instantaneous frequency if it's been subjected to a few additional calculations).

    In order to reconstruct a plausible playback of re-ordered FFT frames, we need to
    calculate the phase difference between successive frames, with FrameDelta, and use
    it to construct a `running phase` (by simply summing the successive differences) for
    the output FFT frames.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Phase input signal.
        framesize: int, optional
            Frame size in samples. Usually same as the FFT size.
            Defaults to 1024.
        overlaps: int, optional
            Number of overlaps in incomming signal. Usually the same
            as the FFT overlaps. Defaults to 4.

    .. note::

        FrameAccum has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

    >>> s = Server().boot()
    >>> s.start()
    >>> snd = SNDS_PATH + '/transparent.aif'
    >>> size, hop = 1024, 256
    >>> nframes = int(sndinfo(snd)[0] / size) + 1
    >>> a = SfPlayer(snd, mul=.3)
    >>> m_mag = [NewMatrix(width=size, height=nframes) for i in range(4)]
    >>> m_pha = [NewMatrix(width=size, height=nframes) for i in range(4)]
    >>> fin = FFT(a, size=size, overlaps=4)
    >>> pol = CarToPol(fin["real"], fin["imag"])
    >>> delta = FrameDelta(pol["ang"], framesize=size, overlaps=4)
    >>> m_mag_rec = MatrixRec(pol["mag"], m_mag, 0, [i*hop for i in range(4)]).play()
    >>> m_pha_rec = MatrixRec(delta, m_pha, 0, [i*hop for i in range(4)]).play()
    >>> m_mag_read = MatrixPointer(m_mag, fin["bin"]/size, Sine(freq=0.25, mul=.5, add=.5))
    >>> m_pha_read = MatrixPointer(m_pha, fin["bin"]/size, Sine(freq=0.25, mul=.5, add=.5))
    >>> accum = FrameAccum(m_pha_read, framesize=size, overlaps=4)
    >>> car = PolToCar(m_mag_read, accum)
    >>> fout = IFFT(car["real"], car["imag"], size=size, overlaps=4).mix(1).out()
    >>> right = Delay(fout, delay=0.013).out(1)

    """

    def __init__(self, input, framesize=1024, overlaps=4, mul=1, add=0):
        pyoArgsAssert(self, "oiiOO", input, framesize, overlaps, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(FrameAccumMain_base(objs_list, wrap(framesize, j), wrap(overlaps, j)))
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(
                FrameAccum_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i))
            )
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

    def setFrameSize(self, x):
        """
        Replace the `framesize` attribute.

        :Args:

            x: int
                new `framesize` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    @property
    def input(self):
        """PyoObject. Phase input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        """PyoObject. Frame size in samples."""
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)


class Vectral(PyoObject):
    """
    Performs magnitude smoothing between successive frames.

    Vectral applies filter with different coefficients for increasing
    and decreasing magnitude vectors, bin by bin.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Magnitude input signal, usually from an FFT analysis.
        framesize: int, optional
            Frame size in samples. Usually the same as the FFT size.
            Defaults to 1024.
        overlaps: int, optional
            Number of overlaps in incomming signal. Usually the same
            as the FFT overlaps. Defaults to 4.
        up: float or PyoObject, optional
            Filter coefficient for increasing bins, between 0 and 1.
            Lower values results in a longer ramp time for bin magnitude.
            Defaults to 1.
        down: float or PyoObject, optional
            Filter coefficient for decreasing bins, between 0 and 1.
            Lower values results in a longer decay time for bin magnitude.
            Defaults to 0.7
        damp: float or PyoObject, optional
            High frequencies damping factor, between 0 and 1. Lower values
            mean more damping. Defaults to 0.9.

    .. note::

        Vectral has no `out` method. Signal must be converted back to time domain,
        with IFFT, before being sent to output.

    >>> s = Server().boot()
    >>> snd = SNDS_PATH + '/accord.aif'
    >>> size, olaps = 1024, 4
    >>> snd = SfPlayer(snd, speed=[.75,.8], loop=True, mul=.3)
    >>> fin = FFT(snd, size=size, overlaps=olaps)
    >>> pol = CarToPol(fin["real"], fin["imag"])
    >>> vec = Vectral(pol["mag"], framesize=size, overlaps=olaps, down=.2, damp=.6)
    >>> car = PolToCar(vec, pol["ang"])
    >>> fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps).mix(2).out()
    >>> s.start()

    """

    def __init__(self, input, framesize=1024, overlaps=4, up=1.0, down=0.7, damp=0.9, mul=1, add=0):
        pyoArgsAssert(self, "oiiOOOOO", input, framesize, overlaps, up, down, damp, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._framesize = framesize
        self._overlaps = overlaps
        self._up = up
        self._down = down
        self._damp = damp
        self._in_fader = InputFader(input)
        in_fader, framesize, overlaps, up, down, damp, mul, add, lmax = convertArgsToLists(
            self._in_fader, framesize, overlaps, up, down, damp, mul, add
        )
        num_of_mains = len(self._in_fader) // self._overlaps
        self._base_players = []
        for j in range(num_of_mains):
            objs_list = []
            for i in range(len(self._in_fader)):
                if (i % num_of_mains) == j:
                    objs_list.append(self._in_fader[i])
            self._base_players.append(
                VectralMain_base(
                    objs_list, wrap(framesize, j), wrap(overlaps, j), wrap(up, j), wrap(down, j), wrap(damp, j)
                )
            )
        self._base_objs = []
        for i in range(lmax):
            base_player = i % num_of_mains
            overlap = i // num_of_mains
            self._base_objs.append(Vectral_base(self._base_players[base_player], overlap, wrap(mul, i), wrap(add, i)))
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

    def setFrameSize(self, x):
        """
        Replace the `framesize` attribute.

        :Args:

            x: int
                new `framesize` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._framesize = x
        x, lmax = convertArgsToLists(x)
        [obj.setFrameSize(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setUp(self, x):
        """
        Replace the `up` attribute.

        :Args:

            x: float or PyoObject
                new `up` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._up = x
        x, lmax = convertArgsToLists(x)
        [obj.setUp(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDown(self, x):
        """
        Replace the `down` attribute.

        :Args:

            x: float or PyoObject
                new `down` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._down = x
        x, lmax = convertArgsToLists(x)
        [obj.setDown(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def setDamp(self, x):
        """
        Replace the `damp` attribute.

        :Args:

            x: float or PyoObject
                new `damp` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._damp = x
        x, lmax = convertArgsToLists(x)
        [obj.setDamp(wrap(x, i)) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [
            SLMap(0.0, 1.0, "lin", "up", self._up),
            SLMap(0.0, 1.0, "lin", "down", self._down),
            SLMap(0.0, 1.0, "lin", "damp", self._damp),
            SLMapMul(self._mul),
        ]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Magnitude input signal."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def framesize(self):
        """int. Frame size in samples."""
        return self._framesize

    @framesize.setter
    def framesize(self, x):
        self.setFrameSize(x)

    @property
    def up(self):
        """float or PyoObject. Filter coefficient for increasing bins."""
        return self._up

    @up.setter
    def up(self, x):
        self.setUp(x)

    @property
    def down(self):
        """float or PyoObject. Filter coefficient for decreasing bins."""
        return self._down

    @down.setter
    def down(self, x):
        self.setDown(x)

    @property
    def damp(self):
        """float or PyoObject. High frequencies damping factor."""
        return self._damp

    @damp.setter
    def damp(self, x):
        self.setDamp(x)


class CvlVerb(PyoObject):
    """
    Convolution based reverb.

    CvlVerb implements convolution based on a uniformly partitioned overlap-save
    algorithm. This object can be used to convolve an input signal with an
    impulse response soundfile to simulate real acoustic spaces.

    :Parent: :py:class:`PyoObject`

    :Args:

        input: PyoObject
            Input signal to process.
        impulse: string, optional
            Path to the impulse response soundfile. The file must have the same
            sampling rate as the server to get the proper convolution. Available at
            initialization time only. Defaults to 'IRMediumHallStereo.wav', located
            in pyo SNDS_PATH folder.
        size: int {pow-of-two}, optional
            The size in samples of each partition of the impulse file. Small size means
            smaller latency but more computation time. If not a power-of-2, the object
            will find the next power-of-2 greater and use that as the actual partition size.
            This value must also be greater or equal than the server's buffer size.
            Available at initialization time only. Defaults to 1024.
        bal: float or PyoObject, optional
            Balance between wet and dry signal, between 0 and 1. 0 means no
            reverb. Defaults to 0.25.

    .. seealso::

        :py:class:`WGVerb`, :py:class:`STRev`, :py:class:`Freeverb`

    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=0.5)
    >>> cv = CvlVerb(sf, SNDS_PATH+"/IRMediumHallStereo.wav", size=1024, bal=0.4).out()

    """

    def __init__(self, input, impulse=SNDS_PATH + "/IRMediumHallStereo.wav", bal=0.25, size=1024, mul=1, add=0):
        pyoArgsAssert(self, "osOiOO", input, impulse, bal, size, mul, add)
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._impulse = impulse
        self._bal = bal
        self._size = size
        self._in_fader = InputFader(input)
        in_fader, bal, size, mul, add, lmax = convertArgsToLists(self._in_fader, bal, size, mul, add)
        impulse, lmax2 = convertArgsToLists(impulse)
        self._base_objs = []
        for file in impulse:
            _size, _dur, _snd_sr, _snd_chnls, _format, _type = sndinfo(file, raise_on_failure=True)
            lmax3 = max(lmax, _snd_chnls)
            self._base_objs.extend(
                [
                    CvlVerb_base(
                        wrap(in_fader, i),
                        stringencode(file),
                        wrap(bal, i),
                        wrap(size, i),
                        i % _snd_chnls,
                        wrap(mul, i),
                        wrap(add, i),
                    )
                    for i in range(lmax3)
                ]
            )
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

    def setBal(self, x):
        """
        Replace the `bal` attribute.

        :Args:

            x: float or PyoObject
                new `bal` attribute.

        """
        pyoArgsAssert(self, "O", x)
        self._bal = x
        x, lmax = convertArgsToLists(x)
        [obj.setBal(wrap(x, i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.0, 1.0, "lin", "bal", self._bal), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input

    @input.setter
    def input(self, x):
        self.setInput(x)

    @property
    def bal(self):
        """float or PyoObject. Wet / dry balance."""
        return self._bal

    @bal.setter
    def bal(self, x):
        self.setBal(x)


class IFFTMatrix(PyoObject):
    """
    Inverse Fast Fourier Transform with a PyoMatrixObject as input.

    IFFTMatrix takes a matrix as input and read it as it is a sonogram.
    On the current column, given by the `index` argument, the cells
    at the bottom represent the lower frequencies of the spectrum and
    the cells at the top, the higher frequencies of the spectrum.

    Because a matrix is usually used to store bipolar signals (with the
    amplitude between -1 and 1), a cell value of -1 represent a frequency
    bin with no amplitude and a cell value of 1 represents the maximum
    amplitude for the given frequency bin.

    The instantaneous angle value (in polar coordinates) of each frequency
    bin is given by the current sample in the audio signal given to the
    `phase` argument. Generally speaking, the more noisy this signal is,
    the more energy a bin with a positive amplitude value will have.

    :Parent: :py:class:`PyoObject`

    :Args:

        matrix: PyoMatrixObject
            The matrix used like a sonogram.
        index: PyoObject
            Normalized horizontal position in the matrix. 0 is the
            first column and 1 is the last. Positions between two
            columns are interpolated. If this signal is a Phasor,
            the matrix is read from left to right.
        phase: PyoObject
            Instantaneous angle value used to compute the inverse
            FFT. Try different signals like white noise or an oscillator
            with a frequency slightly detuned in relation to the
            frequency of the FFT (sr / fftsize).
        size: int {pow-of-two > 4}, optional
            FFT size. Must be a power of two greater than 4.
            The FFT size is the number of samples used in each
            analysis frame. This value must match the `size`
            attribute of the former FFT object. Defaults to 1024.
        overlaps: int, optional
            The number of overlaped analysis block. Must be a
            positive integer. More overlaps can greatly improved
            sound quality synthesis but it is also more CPU
            expensive. This value must match the `overlaps`
            atribute of the former FFT object. Defaults to 4.
        wintype: int, optional
            Shape of the envelope used to filter each output frame.
            Possible shapes are :

            0. rectangular (no windowing)
            1. Hamming
            2. Hanning
            3. Bartlett (triangular)
            4. Blackman 3-term
            5. Blackman-Harris 4-term
            6. Blackman-Harris 7-term
            7. Tuckey (alpha = 0.66)
            8. Sine (half-sine window)

    .. note::

        The output of IFFTMatrix must be mixed to reconstruct the real
        signal from the overlapped streams. It is left to the user
        to call the mix(number of channels) method on an IFFTMatrix object.

    >>> s = Server().boot()
    >>> s.start()
    >>> m = NewMatrix(512, 512)
    >>> m.genSineTerrain(1, 0.15)
    >>> index = Phasor([0.4, 0.5])
    >>> phase = Noise(0.7)
    >>> fout = IFFTMatrix(m, index, phase, size=2048, overlaps=16, wintype=2).mix(2).out()

    """

    def __init__(self, matrix, index, phase, size=1024, overlaps=4, wintype=2, mul=1, add=0):
        pyoArgsAssert(self, "mooiIiOO", matrix, index, phase, size, overlaps, wintype, mul, add)
        PyoObject.__init__(self, mul, add)
        self._matrix = matrix
        self._index = index
        self._phase = phase
        self._size = size
        self._overlaps = overlaps
        self._wintype = wintype
        matrix, index, phase, size, wintype, mul, add, self._lmax = convertArgsToLists(
            matrix, index, phase, size, wintype, mul, add
        )
        self._base_objs = []
        for j in range(overlaps):
            for i in range(self._lmax):
                hopsize = int(wrap(size, i) / overlaps) * j
                self._base_objs.append(
                    IFFTMatrix_base(
                        wrap(matrix, i),
                        wrap(index, i),
                        wrap(phase, i),
                        wrap(size, i),
                        hopsize,
                        wrap(wintype, i),
                        wrap(mul, i),
                        wrap(add, i),
                    )
                )
        self._init_play()

    def __len__(self):
        return int(len(self._base_objs) / self._overlaps)

    def setIndex(self, x):
        """
        Replace the `index` attribute.

        :Args:

            x: PyoObject
                new `index` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._index = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setIndex(wrap(x, i))

    def setPhase(self, x):
        """
        Replace the `phase` attribute.

        :Args:

            x: PyoObject
                new `phase` attribute.

        """
        pyoArgsAssert(self, "o", x)
        self._phase = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setPhase(wrap(x, i))

    def setSize(self, x):
        """
        Replace the `size` attribute.

        :Args:

            x: int
                new `size` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._size = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                hopsize = int(wrap(x, i) / self._overlaps) * j
                self._base_objs[j * self._overlaps + i].setSize(wrap(x, i), hopsize)

    def setWinType(self, x):
        """
        Replace the `wintype` attribute.

        :Args:

            x: int
                new `wintype` attribute.

        """
        pyoArgsAssert(self, "i", x)
        self._wintype = x
        x, lmax = convertArgsToLists(x)
        for j in range(overlaps):
            for i in range(self._lmax):
                self._base_objs[j * self._overlaps + i].setWinType(wrap(x, i))

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def index(self):
        """PyoObject. Normalized horizontal position."""
        return self._index

    @index.setter
    def index(self, x):
        self.setIndex(x)

    @property
    def phase(self):
        """PyoObject. Instantaneous bin angle value."""
        return self._phase

    @phase.setter
    def phase(self, x):
        self.setPhase(x)

    @property
    def size(self):
        """int. FFT size."""
        return self._size

    @size.setter
    def size(self, x):
        self.setSize(x)

    @property
    def wintype(self):
        """int. Windowing method."""
        return self._wintype

    @wintype.setter
    def wintype(self, x):
        self.setWinType(x)
