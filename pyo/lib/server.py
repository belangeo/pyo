# -*- coding: utf-8 -*-

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
import os, sys, time
from ._core import *
from ._widgets import createServerGUI

######################################################################
### Proxy of Server object
######################################################################
class Server(object):
    """
    Main processing audio loop callback handler.

    The Server object handles all communications with Portaudio and
    Portmidi. It keeps track of all audio streams created as well as
    connections between them.

    An instance of the Server must be booted before defining any
    signal processing chain.

    :Args:

        sr: int, optional
            Sampling rate used by Portaudio and the Server to compute samples.
            Defaults to 44100.
        nchnls: int, optional
            Number of output channels. The number of input channels will be the
            same if `ichnls` argument is not defined. Defaults to 2.
        buffersize: int, optional
            Number of samples that Portaudio will request from the callback loop.
            Defaults to 256.

            This value has an impact on CPU use (a small buffer size is harder
            to compute) and on the latency of the system.

            Latency is `buffer size / sampling rate` in seconds.
        duplex: int {0, 1}, optional
            Input - output mode. 0 is output only and 1 is both ways.
            Defaults to 1.
        audio: string {'portaudio', 'pa', 'jack', 'coreaudio', 'offline', 'offline_nb', 'embedded'}, optional
            Audio backend to use. 'pa' is equivalent to 'portaudio'. Default is 'portaudio'.

            'offline' save the audio output in a soundfile as fast as possible in blocking mode,

            ie. the main program doesn't respond until the end of the computation.

            'offline_nb' save the audio output in a soundfile as fast as possible in non-blocking
            mode,

            ie. the computation is executed in a separated thread, allowing the program to
            respond while the computation goes on.

            It is the responsibility of the user to make sure that the program doesn't exit before
            the computation is done.

            'embedded' should be used when pyo is embedded inside an host environment via its C api.

            If 'jack' is selected but jackd is not already started when the program is executed, pyo
            will ask jack to start in the background. Note that pyo never ask jack to close. It is
            the user's responsability to manage the audio configuration of its system.

            If 'manual' is selected, the server waits for a call to its `process` method to compute
            a single buffer size of audio samples. Successive calls to `process` can simulate a
            real computation (probably only useful in the context of internal testing).

            User can set an environment variable named PYO_SERVER_AUDIO to set this value globally.
        jackname: string, optional
            Name of jack client. Defaults to 'pyo'
        ichnls: int, optional
            Number of input channels if different of output channels. If None (default), ichnls = nchnls.
        winhost: string, optional
            Under Windows, pyo's Server will try to use the default devices of the given host.
            This behaviour can be changed with the SetXXXDevice methods. Defaults to "directsound".

            User can set an environment variable named PYO_SERVER_WINHOST to set this value globally.
        midi: string {'portmidi', 'pm', 'jack'}, optional
            Midi backend to use. 'pm' is equivalent to 'portmidi'. Default is 'portmidi'.

            If 'jack' is selected but jackd is not already started when the program is executed, pyo
            will ask jack to start in the background. Note that pyo never ask jack to close. It is
            the user's responsability to manage the audio/midi configuration of its system.

            User can set an environment variable named PYO_SERVER_MIDI to set this value globally.

    .. note::

        The following methods must be called **before** booting the server

        - setInOutDevice(x): Set both input and output devices. See `pa_list_devices()`.
        - setInputDevice(x): Set the audio input device number. See `pa_list_devices()`.
        - setOutputDevice(x): Set the audio output device number. See `pa_list_devices()`.
        - setInputOffset(x): Set the first physical input channel.
        - setOutputOffset(x): Set the first physical output channel.
        - setInOutOffset(x): Set the first physical input and output channels.
        - setSamplingRate(x): Set the sampling rate used by the server.
        - setBufferSize(x): Set the buffer size used by the server.
        - setNchnls(x): Set the number of output (and input if `ichnls` = None) channels used by the server.
        - setIchnls(x): Set the number of input channels (if different of output channels) used by the server.
        - setDuplex(x): Set the duplex mode used by the server.
        - setVerbosity(x): Set the server's verbosity.
        - reinit(sr, nchnls, buffersize, duplex, audio, jackname): Reinit the server's settings.

    >>> # For an 8 channels server in duplex mode with
    >>> # a sampling rate of 48000 Hz and buffer size of 512
    >>> s = Server(sr=48000, nchnls=8, buffersize=512, duplex=1).boot()
    >>> s.start()

    """

    def __init__(
        self,
        sr=44100,
        nchnls=2,
        buffersize=256,
        duplex=1,
        audio="portaudio",
        jackname="pyo",
        ichnls=None,
        winhost="directsound",
        midi="portmidi",
    ):
        if "PYO_SERVER_AUDIO" in os.environ and "offline" not in audio and "embedded" not in audio:
            audio = os.environ["PYO_SERVER_AUDIO"]
        if "PYO_SERVER_WINHOST" in os.environ:
            winhost = os.environ["PYO_SERVER_WINHOST"]

        self._time = time
        self._nchnls = nchnls
        if ichnls is None:
            self._ichnls = nchnls
        else:
            self._ichnls = ichnls
        self._winhost = winhost
        self._amp = 1.0
        self._verbosity = 7
        self._startoffset = 0
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._globalseed = 0
        self._resampling = 1
        self._isJackTransportSlave = False
        self._server = Server_base(sr, nchnls, buffersize, duplex, audio, jackname, self._ichnls, midi)

        if sys.platform.startswith("win"):
            host_default_in, host_default_out = pa_get_default_devices_from_host(winhost)
            self._server.setInputDevice(host_default_in)
            self._server.setOutputDevice(host_default_out)

    def __del__(self):
        self.setTime = None
        self.setRms = None
        if self.getIsBooted():
            if self.getIsStarted():
                self.stop()
                self._time.sleep(0.25)
            self.shutdown()
            self._time.sleep(0.25)

    def reinit(
        self,
        sr=44100,
        nchnls=2,
        buffersize=256,
        duplex=1,
        audio="portaudio",
        jackname="pyo",
        ichnls=None,
        winhost="directsound",
        midi="portmidi",
    ):
        """
        Reinit the server'settings. Useful to alternate between real-time and offline server.

        :Args:

            Same as in the __init__ method.

        """
        if "PYO_SERVER_AUDIO" in os.environ and "offline" not in audio and "embedded" not in audio:
            audio = os.environ["PYO_SERVER_AUDIO"]
        if "PYO_SERVER_WINHOST" in os.environ:
            winhost = os.environ["PYO_SERVER_WINHOST"]

        self._gui_frame = None
        self._nchnls = nchnls
        if ichnls is None:
            self._ichnls = nchnls
        else:
            self._ichnls = ichnls
        self._winhost = winhost
        self._amp = 1.0
        self._verbosity = 7
        self._startoffset = 0
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._globalseed = 0
        self._resampling = 1
        self._isJackTransportSlave = False
        self._server.__init__(sr, nchnls, buffersize, duplex, audio, jackname, self._ichnls, midi)

        if sys.platform.startswith("win"):
            host_default_in, host_default_out = pa_get_default_devices_from_host(winhost)
            self._server.setInputDevice(host_default_in)
            self._server.setOutputDevice(host_default_out)

    def setCallback(self, callback):
        """
        Register a custom process callback.

        The function given as argument will be called every computation
        block, just before the computation of the audio object tree.
        Inside the callback, one can process the data of a table with
        numpy calls for example.

        """
        if callable(callback):
            self._server.setCallback(callback)

    def setInputDevice(self, x):
        """
        Set the audio input device number. See `pa_list_devices()`.

        :Args:

            x: int
                Number of the audio device listed by Portaudio.

        """
        self._server.setInputDevice(x)

    def setOutputDevice(self, x):
        """
        Set the audio output device number. See `pa_list_devices()`.

        :Args:

            x: int
                Number of the audio device listed by Portaudio.

        """
        self._server.setOutputDevice(x)

    def setInputOffset(self, x):
        """
        Set the first physical input channel.

        Channel number `x` from the soundcard will be assigned to
        server's channel one, channel number `x` + 1 to server's
        channel two and so on.

        :Args:

            x: int
                Channel number.

        """
        self._server.setInputOffset(x)

    def setOutputOffset(self, x):
        """
        Set the first physical output channel.

        Server's channel one will be assigned to soundcard's channel
        number `x`, server's channel two will be assigned to soundcard's
        channel number `x` + 1 and so on.

        :Args:

            x: int
                Channel number.

        """
        self._server.setOutputOffset(x)

    def setInOutOffset(self, x):
        """
        Set the first physical input and output channels.

        Set both offsets to the same value. See `setInputOffset` and
        `setOutputOffset` documentation for more details.

        :Args:

            x: int
                Channel number.

        """
        self._server.setInputOffset(x)
        self._server.setOutputOffset(x)

    def setSamplingRate(self, x):
        """
        Set the sampling rate used by the server.

        :Args:

            x: int
                New sampling rate, must be supported by the soundcard.

        """
        self._server.setSamplingRate(x)

    def setBufferSize(self, x):
        """
        Set the buffer size used by the server.

        :Args:

            x: int
                New buffer size.

        """
        self._server.setBufferSize(x)

    def setNchnls(self, x):
        """
        Set the number of output (and input if `ichnls` = None) channels used by the server.

        :Args:

            x: int
                New number of channels.

        """
        self._nchnls = x
        self._server.setNchnls(x)

    def setIchnls(self, x):
        """
        Set the number of input channels (if different of output channels) used by the server.

        :Args:

            x: int
                New number of input channels.

        """
        self._ichnls = x
        self._server.setIchnls(x)

    def setDuplex(self, x):
        """
        Set the duplex mode used by the server.

        :Args:

            x: int {0 or 1}
                New mode. 0 is output only, 1 is both ways.

        """
        self._server.setDuplex(x)

    def setVerbosity(self, x):
        """
        Set the server's verbosity.

        :Args:

            x: int
                A sum of values to display different levels:
                    - 1 = error
                    - 2 = message
                    - 4 = warning
                    - 8 = debug

        """
        self._verbosity = x
        self._server.setVerbosity(x)

    def setGlobalDur(self, x):
        """
        Set the global object duration (time to wait before stopping the object).

        This value, if not 0, will override the `dur` argument of object's
        play() and out() methods.

        :Args:

            x: float
                New global duration.

        """
        self._server.setGlobalDur(x)

    def setGlobalDel(self, x):
        """
        Set the global object delay time (time to wait before activating the object).

        This value, if not 0, will override the `del` argument of object's
        play() and out() methods.

        :Args:

            x: float
                New global delay time.

        """
        self._server.setGlobalDel(x)

    def setGlobalSeed(self, x):
        """
        Set the server's global seed used by random objects.

        :Args:

            x: int
                A positive integer that will be used as the seed by random objects.

                If zero, randoms will be seeded with the system clock current value.

        """
        self._globalseed = x
        self._server.setGlobalSeed(x)

    def setStartOffset(self, x):
        """
        Set the server's starting time offset. First `x` seconds will be rendered
        offline as fast as possible.

        :Args:

            x: float
                Starting time of the real-time processing.

        """
        self._startoffset = x
        self._server.setStartOffset(x)

    def setAmp(self, x):
        """
        Set the overall amplitude.

        :Args:

            x: float
                New amplitude.

        """
        self._amp = x
        self._server.setAmp(x)

    def beginResamplingBlock(self, x):
        """
        Starts a resampling block.

        This factor must be a power-of-two. A positive value means
        upsampling and a negative value means downsampling. After this
        call, every PyoObject will be created with an internal sampling
        rate and buffer size relative to the resampling factor. The method
        `endResamplingBlock()` should be called at the end of the code
        block using the resampling factor.

        The `Resample` object can be used inside the resampling block to
        perform up or down resampling of audio signal created before the
        block.

        :Args:

            x: int, power-of-two
                Resampling factor. Must be a power-of-two. A positive
                value starts an upsampling block while a negative value
                starts a downsampling block.

        """
        realx = x
        x = abs(x)
        if ((x & (x - 1)) == 0) and x != 0:
            self._resampling = realx
            self._server.beginResamplingBlock(realx)
        else:
            print("Resampling factor must be a power-of-two (positive or negative).")

    def endResamplingBlock(self):
        """
        Ends a resampling block.

        This call ends a code block using a sample rate different from
        the current sampling rate of the system.

        The `Resample` object can be used after the resampling blick to
        perform up or down resampling of audio signal created inside the
        block.

        """
        self._resampling = 1
        self._server.endResamplingBlock()

    def shutdown(self):
        """
        Shut down and clear the server. This method will erase all objects
        from the callback loop. This method need to be called before changing
        server's parameters like `samplingrate`, `buffersize`, `nchnls`, ...

        """
        self._server.shutdown()

    def boot(self, newBuffer=True):
        """
        Boot the server. Must be called before defining any signal processing
        chain. Server's parameters like `samplingrate`, `buffersize` or
        `nchnls` will be effective after a call to this method.

        :Args:

            newBuffer: bool
                Specify if the buffers need to be allocated or not. Useful to limit
                the allocation of new buffers when the buffer size hasn't change.

                Therefore, this is useful to limit calls to the Python interpreter
                to get the buffers addresses when using Pyo inside a
                C/C++ application with the embedded server.

                Defaults to True.

        """
        self._server.boot(newBuffer)
        return self

    def start(self):
        """
        Start the audio callback loop and begin processing.

        """
        self._server.start()
        return self

    def stop(self):
        """
        Stop the audio callback loop.

        """
        self._server.stop()

    def getStreams(self):
        """
        Return the list of streams loaded in the server.

        """
        return self._server.getStreams()

    def getSamplingRate(self):
        """
        Return the current sampling rate.

        """
        return self._server.getSamplingRate()

    def getNchnls(self):
        """
        Return the current number of channels.

        """
        return self._server.getNchnls()

    def getBufferSize(self):
        """
        Return the current buffer size.

        """
        return self._server.getBufferSize()

    def getGlobalDur(self):
        """
        Return the current global duration.

        """
        return self._server.getGlobalDur()

    def getGlobalDel(self):
        """
        Return the current global delay time.

        """
        return self._server.getGlobalDel()

    def getGlobalSeed(self):
        """
        Return the current global seed.

        """
        return self._server.getGlobalSeed()

    def getIsStarted(self):
        """
        Returns 1 if the server is started, otherwise returns 0.

        """
        return self._server.getIsStarted()

    def getIsBooted(self):
        """
        Returns 1 if the server is booted, otherwise returns 0.

        """
        return self._server.getIsBooted()

    def getStreams(self):
        """
        Returns the list of Stream objects currently in the Server memory.

        """
        return self._server.getStreams()

    def getNumberOfStreams(self):
        """
        Returns the number of streams currently in the Server memory.

        """
        return len(self._server.getStreams())

    def setServer(self):
        """
        Sets this server as the one to use for new objects when using
        the embedded device.

        """
        return self._server.setServer()

    def getInputAddr(self):
        """
        Return the address of the input buffer.

        """
        return self._server.getInputAddr()

    def getOutputAddr(self):
        """
        Return the address of the output buffer.

        """
        return self._server.getOutputAddr()

    def getServerID(self):
        """
        Return the server ID.

        """
        return self._server.getServerID()

    def getServerAddr(self):
        """
        Return the address of the server.

        """
        return self._server.getServerAddr()

    def getEmbedICallbackAddr(self):
        """
        Return the address of the interleaved embedded
        callback function.

        """
        return self._server.getEmbedICallbackAddr()

    def getCurrentTime(self):
        """
        Return the current time as a formatted string.
        """
        return self._server.getCurrentTime()

    def getCurrentTimeInSamples(self):
        """
        Return the current time in number of elapsed samples since the server was started.
        """
        return self._server.getCurrentTimeInSamples()

    def getCurrentAmp(self):
        """
        Return the current amplitudes as a tuple of `nchnls` length.
        """
        return self._server.getCurrentAmp()

    def setAutoStartChildren(self, state):
        """
        Giving True to this method tells pyo that a call to the `play`,
        `out` or `stop` method of audio objects should propagate to the
        other audio objects given as arguments. This can be used to
        control an entire dsp chain just by calling methods on the
        very last object.

        With setAutoStartChildren(True), a call to the `out` method
        will automatically triggers the `play` method of the Fader
        object given as `mul` argument.

        >>> a = RCOsc(freq=150, mul=Fader(1, 1, mul=.3)).out()

        With setAutoStartChildren(True), a call to the `stop` method
        will also propagate to audio objects given as arguments. The
        `stop` method has an argument `wait`, useful to postpone the
        moment when to really stop the process. The `waiting value is
        also propagated to the other audio objects, but not for the
        objects assigned to a `mul` argument. This property allows to
        do things like the next snippet. On a call `a.stop(1)` , while
        the Linseg assigned to the frequency waits for 1 second before
        actually stopping its process, the Fader assigned to the `mul`
        argument starts its fadeout immediately.

        >>> freq = Linseg([(0,500),(1, 750),(2, 500)], loop=True)
        >>> a = Sine(freq=freq, mul=Fader(1, 1, mul=.2)).out()

        Sometime, we still want the process assigned to a `mul` attribute
        to wait before stopping its process. See the next case. The
        amplitude envelope is itself modulated in amplitude by another
        envelope. We still want the fadeout to start immediately, but
        its own envelope has to wait, otherwise, the amplitude will cut
        off before having the time to complete the fadeout. To do this,
        we call `useWaitTimeOnStop()` on the object we want to force to
        wait.

        >>> lf = Linseg([(0,0),(0.2,0.1),(0.4,0)], loop=True)
        >>> lf.useWaitTimeOnStop()
        >>> freq = Linseg([(0,500),(1, 750),(2, 500)], loop=True)
        >>> a = Sine(freq=freq, mul=Fader(1, 1, mul=lf)).out()

        .. Note::

            The waiting time doesn't propagate to objects used in audio
            arithmetic. Code like this (for which you want to call out.stop()):

            >>> a = Sine(500)
            >>> a.setStopDelay(1)
            >>> b = Sine(750)
            >>> b.setStopDelay(2)
            >>> out = Freeverb(a+b, size=0.8, mul=0.3).out()
            >>> out.setStopDelay(4)

            Should be written as:

            >>> a = Sine(500)
            >>> a.setStopDelay(1)
            >>> b = Sine(750)
            >>> b.setStopDelay(2)
            >>> c = Mix([a, b], voices=1)
            >>> c.setStopDelay(2)
            >>> out = Freeverb(c, size=0.8, mul=0.3).out()
            >>> out.setStopDelay(4)

        """
        self._server.setAutoStartChildren(state)

    def process(self):
        """
        Tell the server to compute a single buffer size of audio samples.

        The audio backend of the server must be set to `manual` to use this method.
        It's probably only useful in the context of internal testing.

        """
        self._server.process()

    @property
    def amp(self):
        """float. Overall amplitude."""
        return self._amp

    @amp.setter
    def amp(self, x):
        self.setAmp(x)

    @property
    def startoffset(self):
        """float. Starting time of the real-time processing."""
        return self._startoffset

    @startoffset.setter
    def startoffset(self, x):
        self.setStartOffset(x)

    @property
    def verbosity(self):
        """int. Server verbosity."""
        return self._verbosity

    @verbosity.setter
    def verbosity(self, x):
        if type(x) == int:
            self.setVerbosity(x)
        else:
            raise Exception("verbosity must be an integer")

    @property
    def globalseed(self):
        """int. Server global seed."""
        return self._globalseed

    @globalseed.setter
    def globalseed(self, x):
        if type(x) == int:
            self.setGlobalSeed(x)
        else:
            raise Exception("global seed must be an integer")
