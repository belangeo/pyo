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

    Methods:

    setAmp(x) : Set the overall amplitude.
    boot() : Boot the server. Must be called before defining any signal 
        processing chain.
    shutdown() : Shut down and clear the server.
    start() : Start the audio callback loop.
    stop() : Stop the audio callback loop.
    recstart(str) : Begins recording of the sound sent to output. 
        This method creates a file called `pyo_rec.aif` in the 
        user's home directory if a path is not supplied.
    recstop() : Stops previously started recording.
    getSamplingRate() : Returns the current sampling rate.
    getNchnls() : Returns the current number of channels.
    getBufferSize() : Returns the current buffer size.
    getIsStarted() : Returns 1 if the server is started, otherwise returns 0.

    The next methods must be called before booting the server

    setInOutDevice(x) : Set both input and output devices. See `pa_list_devices()`.
    setInputDevice(x) : Set the audio input device number. See `pa_list_devices()`.
    setOutputDevice(x) : Set the audio output device number. See `pa_list_devices()`.
    setMidiInputDevice(x) : Set the MIDI input device number. See `pm_list_devices()`.
    setSamplingRate(x) : Set the sampling rate used by the server.
    setBufferSize(x) : Set the buffer size used by the server.
    setNchnls(x) : Set the number of channels used by the server.
    setDuplex(x) : Set the duplex mode used by the server.

    Attributes:
    
    amp : Overall amplitude of the Server. This value is applied on any 
        stream sent to the soundcard.
        
    Examples:
    
    >>> # For an 8 channels server in duplex mode with
    >>> # a sampling rate of 48000 Hz and buffer size of 512
    >>> s = Server(sr=48000, nchnls=8, buffersize=512, duplex=1).boot()
    >>> s.start()
        
    """
    def __init__(self, sr=44100, nchnls=2, buffersize=256, duplex=1):
        self._nchnls = nchnls
        self._amp = 1.
        self._server = Server_base(sr, nchnls, buffersize, duplex)

    def gui(self, locals=None):
        f, win = createServerGUI(self._nchnls, self.start, self.stop, self.recstart, self.recstop, self.setAmp, self.getIsStarted(), locals)
        self._server.setAmpCallable(f)
        win.mainloop()

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
        
    def recstart(self, filename=None):
        """
        Begins a default recording of the sound that is sent to the
        soundcard. This will create a file called `pyo_rec.aif` in 
        the user's home directory if no path is supplied.
        
        Parameters:
        
        filename : string, optional
            Name of the file to be created. Defaults to None.
        
        """
        if filename == None:
            self._server.recstart()
        else:
            self._server.recstart(filename)    
        
    def recstop(self):
        """
        Stop the previously started recording.
        
        """
        self._server.recstop()
        
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

    def getIsStarted(self):
        """
        Returns 1 if the server is started, otherwise returns 0.
        
        """
        return self._server.getIsStarted()

    @property
    def amp(self):
        """float. Overall amplitude.""" 
        return self._amp
    @amp.setter
    def amp(self, x): self.setAmp(x) 
