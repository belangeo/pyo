# -*- coding: utf-8 -*-
"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
import os, time
from _core import *
from _widgets import createServerGUI
        
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

    Parameters:

    sr : int, optional
        Sampling rate used by Portaudio and the Server to compute samples. 
        Defaults to 44100.
    nchnls : int, optional
        Number of input and output channels. Defaults to 2.
    buffersize : int, optional
        Number of samples that Portaudio will request from the callback loop. 
        This value has an impact on CPU use (a small buffer size is harder 
        to compute) and on the latency of the system. Latency is 
        `buffer size / sampling rate` in seconds. Defaults to 256.
    duplex : int {0, 1}, optional
        Input - output mode. 0 is output only and 1 is both ways. 
        Defaults to 1.
    audio : string {'portaudio', 'pa', 'jack', 'coreaudio', 'offline', 'offline_nb}, optional
        Audio backend to use. 'pa' is equivalent to 'portaudio'.
        'offline' save the audio output in a soundfile as fast as possible in blocking mode, 
        ie. the main program doesn't respond until the end of the computation.
        'offline_nb' save the audio output in a soundfile as fast as possible in non-blocking 
        mode, ie. the computation is executed in a separated thread, allowing the program to
        respond while the computation goes on. It is the responsibility of the user to make
        sure that the program doesn't exit before the computation is done.
        Default is 'portaudio'.
    jackname : string, optional
        Name of jack client. Defaults to 'pyo'

    Methods:

    setAmp(x) : Set the overall amplitude.
    boot() : Boot the server. Must be called before defining any signal 
        processing chain.
    shutdown() : Shut down and clear the server.
    setStartOffset(x) : Set the starting time of the real-time processing.
    setGlobalSeed(x) : Set the server's global seed used by random objects.
    start() : Start the audio callback loop.
    stop() : Stop the audio callback loop.
    gui(locals, meter, timer) : Show the server's user interface.
    recordOptions(dur, filename, fileformat, sampletype) : Rendering settings.
    recstart(str) : Begins recording of the sound sent to output. 
        This method creates a file called `pyo_rec.aif` in the 
        user's home directory if a path is not supplied.
    recstop() : Stops previously started recording.
    getSamplingRate() : Returns the current sampling rate.
    getNchnls() : Returns the current number of channels.
    getBufferSize() : Returns the current buffer size.
    getGlobalSeed() : Returns the server's global seed.
    getIsStarted() : Returns 1 if the server is started, otherwise returns 0.
    getIsBooted() : Returns 1 if the server is booted, otherwise returns 0.
    getMidiActive() : Returns 1 if Midi callback is active, otherwise returns 0.
    getStreams() : Returns the list of Stream objects currently in the Server memory.
    getNumberOfStreams() : Returns the number of streams currently in the Server memory.
    sendMidiNote(pitch, velocity, channel, timestamp) : Send a MIDI note message to the 
        selected output device.

    The next methods must be called before booting the server

    setInOutDevice(x) : Set both input and output devices. See `pa_list_devices()`.
    setInputDevice(x) : Set the audio input device number. See `pa_list_devices()`.
    setOutputDevice(x) : Set the audio output device number. See `pa_list_devices()`.
    setMidiInputDevice(x) : Set the MIDI input device number. See `pm_list_devices()`.
    setMidiOutputDevice(x) : Set the MIDI output device number. See `pm_list_devices()`.
    setSamplingRate(x) : Set the sampling rate used by the server.
    setBufferSize(x) : Set the buffer size used by the server.
    setNchnls(x) : Set the number of channels used by the server.
    setDuplex(x) : Set the duplex mode used by the server.
    setVerbosity(x) : Set the server's verbosity.
    reinit(sr, nchnls, buffersize, duplex, audio, jackname) : Reinit the server's settings.
        
    Attributes:
    
    amp : Overall amplitude of the Server. This value is applied on any 
        stream sent to the soundcard.
    verbosity : Control the messages printed by the server. It is a sum of 
        values to display different levels: 1 = error, 2 = message, 
        4 = warning , 8 = debug.
    startoffset : Starting time of the real-time processing.
    globalseed : Global seed used by random objects. Defaults to 0 (means always seed from the system clock). 
        
    Examples:
    
    >>> # For an 8 channels server in duplex mode with
    >>> # a sampling rate of 48000 Hz and buffer size of 512
    >>> s = Server(sr=48000, nchnls=8, buffersize=512, duplex=1).boot()
    >>> s.start()
        
    """
    def __init__(self, sr=44100, nchnls=2, buffersize=256, duplex=1, audio='portaudio', jackname='pyo'):
        if os.environ.has_key("PYO_SERVER_AUDIO") and "offline" not in audio:
            audio = os.environ["PYO_SERVER_AUDIO"]
        self._time = time
        self._nchnls = nchnls
        self._amp = 1.
        self._verbosity = 7
        self._startoffset = 0
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._server = Server_base(sr, nchnls, buffersize, duplex, audio, jackname)
        self._server._setDefaultRecPath(os.path.join(os.path.expanduser("~"), "pyo_rec.wav"))

    def __del__(self):
        if self.getIsBooted():
            if self.getIsStarted():
                self.stop()
                self._time.sleep(.25)
            self.shutdown()
            self._time.sleep(.25)

    def reinit(self, sr=44100, nchnls=2, buffersize=256, duplex=1, audio='portaudio', jackname='pyo'):
        """
        Reinit the server'settings. Useful to alternate between real-time and offline server.
        
        Parameters:
        
        Same as in the __init__ method.
        
        """
        self._nchnls = nchnls
        self._amp = 1.
        self._verbosity = 7
        self._startoffset = 0
        self._dur = -1
        self._filename = None
        self._fileformat = 0
        self._sampletype = 0
        self._globalseed = 0
        self._server.__init__(sr, nchnls, buffersize, duplex, audio, jackname)

    def gui(self, locals=None, meter=True, timer=True):
        """
        Show the server's user interface.
        
        Parameters:
        
        locals : locals namespace {locals(), None}, optional
            If locals() is given, the interface will show an interpreter extension,
            giving a way to interact with the running script. Defaults to None.
        meter : boolean, optinal
            If True, the interface will show a vumeter of the global output signal. 
            Defaults to True.
        timer : boolean, optional
            If True, the interface will show a clock of the current time.
            Defaults to True.
            
        """
        f, win = createServerGUI(self._nchnls, self.start, self.stop, self.recstart, self.recstop,
                                 self.setAmp, self.getIsStarted(), locals, self.shutdown, meter, timer, self._amp)
        if meter:
            self._server.setAmpCallable(f)
        if timer:
            self._server.setTimeCallable(f)
        try:
            win.mainloop()
        except:
            if win != None:
                win.MainLoop()

    def setTimeCallable(self, func):
        self.setTime = func
        self._server.setTimeCallable(self)
        
    def setInOutDevice(self, x):
        """
        Set both input and output audio devices. See `pa_list_devices()`.
        
        Parameters:

        x : int
            Number of the audio input and output devices.

        """
        self._server.setInOutDevice(x)
        
    def setInputDevice(self, x):
        """
        Set the audio input device number. See `pa_list_devices()`.
        
        Parameters:

        x : int
            Number of the audio device listed by Portaudio.

        """
        self._server.setInputDevice(x)

    def setOutputDevice(self, x):
        """
        Set the audio output device number. See `pa_list_devices()`.
        
        Parameters:

        x : int
            Number of the audio device listed by Portaudio.

        """
        self._server.setOutputDevice(x)

    def setMidiInputDevice(self, x):
        """
        Set the Midi input device number. See `pm_list_devices()`.
        
        Parameters:

        x : int
            Number of the Midi device listed by Portmidi.

        """
        self._server.setMidiInputDevice(x)

    def setMidiOutputDevice(self, x):
        """
        Set the Midi output device number. See `pm_list_devices()`.
        
        Parameters:

        x : int
            Number of the Midi device listed by Portmidi.

        """
        self._server.setMidiOutputDevice(x)
 
    def setSamplingRate(self, x):
        """
        Set the sampling rate used by the server.
        
        Parameters:

        x : int
            New sampling rate, must be supported by the soundcard.

        """  
        self._server.setSamplingRate(x)
        
    def setBufferSize(self, x):
        """
        Set the buffer size used by the server.
        
        Parameters:

        x : int
            New buffer size.

        """        
        self._server.setBufferSize(x)
  
    def setNchnls(self, x):
        """
        Set the number of channels used by the server.
        
        Parameters:

        x : int
            New number of channels.

        """
        self._nchnls = x
        self._server.setNchnls(x)

    def setDuplex(self, x):
        """
        Set the duplex mode used by the server.
        
        Parameters:

        x : int {0 or 1}
            New mode. 0 is output only, 1 is both ways.

        """        
        self._server.setDuplex(x)

    def setVerbosity(self, x):
        """
        Set the server's verbosity.
        
        Parameters:

        x : int
            A sum of values to display different levels: 1 = error, 2 = message, 
            4 = warning , 8 = debug.
        """        
        self._verbosity = x
        self._server.setVerbosity(x)

    def setGlobalSeed(self, x):
        """
        Set the server's global seed used by random objects.

        Parameters:

        x : int
            A positive integer that will be used as the seed by random objects.
            If zero, randoms will be seeded with the system clock current value.

        """        
        self._globalseed = x
        self._server.setGlobalSeed(x)

    def setStartOffset(self, x):
        """
        Set the server's starting time offset. First `x` seconds will be rendered
        offline as fast as possible.

        Parameters:

        x : float
            Starting time of the real-time processing.
            
        """        
        self._startoffset = x
        self._server.setStartOffset(x)

    def setAmp(self, x):
        """
        Set the overall amplitude.
        
        Parameters:

        x : float
            New amplitude.

        """
        self._amp = x
        self._server.setAmp(x)
 
    def shutdown(self):
        """
        Shut down and clear the server. This method will erase all objects
        from the callback loop. This method need to be called before changing 
        server's parameters like `samplingrate`, `buffersize`, `nchnls`, ...

        """
        self._server.shutdown()
        
    def boot(self):
        """
        Boot the server. Must be called before defining any signal processing 
        chain. Server's parameters like `samplingrate`, `buffersize` or 
        `nchnls` will be effective after a call to this method.

        """
        self._server.boot()
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

    def recordOptions(self, dur=-1, filename=None, fileformat=0, sampletype=0):
        """
        Sets options for soundfile created by offline rendering or global recording.

        Parameters:

        dur : float
            Duration, in seconds, of the recorded file. Only used by
            offline rendering. Must be positive. Defaults to -1.
        filename : string
            Full path of the file to create. If None, a file called
            `pyo_rec.wav` will be created in the user's home directory.
            Defaults to None.
        fileformat : int, optional
            Format type of the audio file. This function will first try to
            set the format from the filename extension. If it's not possible,
            it uses the fileformat parameter. Defaults to 0. 
            Supported formats are:
                0 : WAV - Microsoft WAV format (little endian) {.wav, .wave}
                1 : AIFF - Apple/SGI AIFF format (big endian) {.aif, .aiff}
        sampletype : int, optional
            Bit depth encoding of the audio file. Defaults to 0.
            Supported types are:
                0 : 16 bits int
                1 : 24 bits int
                2 : 32 bits int
                3 : 32 bits float
                4 : 64 bits float

        """
        
        FORMATS = {'wav': 0, 'wave': 0, 'aif': 1, 'aiff': 1}
        self._dur = dur
        if filename == None:
            filename = os.path.join(os.path.expanduser("~"), "pyo_rec.wav")
        self._filename = filename
        ext = filename.rsplit('.')
        if len(ext) >= 2:
            ext = ext[-1].lower()
            if FORMATS.has_key(ext):
                fileformat = FORMATS[ext]
            else:
                print 'Warning: Unknown file extension. Using fileformat value.'
        else:
            print 'Warning: Filename has no extension. Using fileformat value.'
        self._fileformat = fileformat
        self._sampletype = sampletype
        self._server.recordOptions(dur, filename, fileformat, sampletype)
        
    def recstart(self, filename=None):
        """
        Begins a default recording of the sound that is sent to the
        soundcard. This will create a file called `pyo_rec.wav` in 
        the user's home directory if no path is supplied or defined
        with recordOptions method. Uses file format and sample type 
        defined with recordOptions method. 
        
        Parameters:
        
        filename : string, optional
            Name of the file to be created. Defaults to None.
        
        """
        FORMATS = {'wav': 0, 'wave': 0, 'aif': 1, 'aiff': 1}
        if filename == None:
            if self._filename != None:
                filename = self._filename
            else:
                filename = os.path.join(os.path.expanduser("~"), "pyo_rec.wav")
        ext = filename.rsplit('.')
        if len(ext) >= 2:
            ext = ext[-1].lower()
            if FORMATS.has_key(ext):
                fileformat = FORMATS[ext]
                if fileformat != self._fileformat:
                    self._fileformat = fileformat
                    self._server.recordOptions(self._dur, filename, self._fileformat, self._sampletype)

        self._server.recstart(filename)    
        
    def recstop(self):
        """
        Stop the previously started recording.
        
        """
        self._server.recstop()

    def sendMidiNote(self, pitch, velocity, channel=0, timestamp=0):
        """
        Send a MIDI note message to the selected output device. 
        
        Parameters:
        
        pitch : int
            Midi pitch, between 0 and 127.
        velocity : int
            Amplitude of the note, between 0 and 127. A note
            with a velocity of 0 is equivalent to a note off.
        channel : int, optional
            The Midi channel, between 1 and 16, on which the 
            note is sent. A channel of 0 means all channels. 
        """
        self._server.sendMidiNote(pitch, velocity, channel, timestamp)

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

    def getMidiActive(self):
        """
        Returns 1 if Midi callback is active, otherwise returns 0.
        
        """
        return self._server.getMidiActive()

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

    @property
    def amp(self):
        """float. Overall amplitude.""" 
        return self._amp
    @amp.setter
    def amp(self, x): self.setAmp(x) 

    @property
    def startoffset(self):
        """float. Starting time of the real-time processing.""" 
        return self._startoffset
    @startoffset.setter
    def startoffset(self, x): self.setStartOffset(x) 

    @property
    def verbosity(self):
        """int. Server verbosity.""" 
        return self._verbosity
    @verbosity.setter
    def verbosity(self, x):
        if (type(x) == int):
            self.setVerbosity(x)
        else:
            raise Exception("verbosity must be an integer")

    @property
    def globalseed(self):
        """int. Server global seed.""" 
        return self._globalseed
    @globalseed.setter
    def globalseed(self, x):
        if (type(x) == int):
            self.setGlobalSeed(x)
        else:
            raise Exception("global seed must be an integer")
