from _core import *

######################################################################
### Proxy of Server object
######################################################################
class Server:
    """
    The Server object handles all communications with Portaudio and Portmidi.
    
    An instance of the Server object must be booted before definning any signal processing chain.

    **Parameters**

    sr : int, optional
        Sampling rate used by Portaudio and any PyoObject to compute samples. Default to 44100.
    nchnls : int, optional
        Number of input and output channels. Default to 1.
    buffersize : int, optional
        Number of samples that Portaudio will request from the callback. This value has an impact
        on CPU use (small buffer size is harder to compute) and on the latency of the system. Latency
        is `buffer size / sampling rate` in seconds. Default to 256.
    duplex : int {0, 1}, optional
        Input - output mode. 0 is output only and 1 is both ways. Default to 0.

    **Methods**

    boot() : Boot the server. Must be called before definning any signal processing chain.
    shutdown() : Shut down and clear the server.
    start() : Start the audio callback loop.
    stop() : Stop the audio callback loop.
    recstart() : Begin recording sound sent to output. Create a fie called `pyo_rec.aif` 
        in the home directory.
    recstop() : Stop previously started recording.
    getSamplingRate() : Return the current sampling rate.
    getNchnls() : Return the current number of channels.
    getBufferSize() : Retrun the current buffer size.    

    * The next methods must be called before booting the server
    
    setInputDevice(x) : Set the audio input device number. See `pa_list_devices()`.
    setOutputDevice(x) : Set the audio output device number. See `pa_list_devices()`.
    setMidiInputDevice(x) : Set the MIDI input device number. See `pm_list_devices()`.
    setSamplingRate(x) : Set the sampling rate used by the server.
    setBufferSize(x) : Set the buffer size used by the server.
    setNchnls(x) : Set the number of channels used by the server.
    setDuplex(x) : Set the duplex mode used by the server.

    """
    def __init__(self, sr=44100, nchnls=1, buffersize=256, duplex=0):
        self._server = Server_base(sr, nchnls, buffersize, duplex)

    def setInputDevice(self, x):
        """
        Set the audio input device number. See `pa_list_devices()`.
        
        **Parameters**

        x : int
            Number of the audio device listed by Portaudio.

        """
        self._server.setInputDevice(x)

    def setOutputDevice(self, x):
        """
        Set the audio output device number. See `pa_list_devices()`.
        
        **Parameters**

        x : int
            Number of the audio device listed by Portaudio.

        """
        self._server.setOutputDevice(x)

    def setMidiInputDevice(self, x):
        """
        Set the MIDI input device number. See `pm_list_devices()`.
        
        **Parameters**

        x : int
            Number of the MIDI device listed by Portmidi.

        """
        self._server.setMidiInputDevice(x)
 
    def setSamplingRate(self, x):
        """
        Set the sampling rate used by the server.
        
        **Parameters**

        x : int
            New sampling rate, must be supported by the soundcard.

        """  
        self._server.setSamplingRate(x)

    def setBufferSize(self, x):
        """
        Set the buffer size used by the server.
        
        **Parameters**

        x : int
            New buffer size.

        """        
        self._server.setBufferSize(x)
  
    def setNchnls(self, x):
        """
        Set the number of channels used by the server.
        
        **Parameters**

        x : int
            New number of channels.

        """
        self._server.setNchnls(x)

    def setDuplex(self, x):
        """
        Set the duplex mode used by the server.
        
        **Parameters**

        x : int {0 or 1}
            New mode. 0 is output only, 1 is both ways.

        """        
        self._server.setDuplex(x)
 
    def shutdown(self):
        """
        Shut down and clear the server. This method will erase all objects from the callback loop.
        Call this method before changing server's parameter like `samplingrate`, `buffersize`, `nchnls`...

        """
        self._server.shutdown()
        
    def boot(self):
        """
        Boot the server. Must be called before definning any signal processing chain. Server's parameter
        like `samplingrate`, `buffersize` or `nchnls` will be effective after a call to this method.

        """
        self._server.boot()
        return self._server
        
    def start(self):
        """
        Start the audio callback loop and begin processing.
        
        """
        self._server.start()
        return self._server
    
    def stop(self):
        """
        Stop the audio callback loop.
        
        """
        self._server.stop()
        
    def recstart(self):
        """
        Begin a default recording of the sound that is sent to output. This will create a file
        called `pyo_rec.aif` in the home directory.
        
        """
        self._server.recstart()
        
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
