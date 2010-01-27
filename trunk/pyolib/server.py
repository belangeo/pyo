from _core import *

from Tkinter import *
import math

######################################################################
### Server Object User Interface
######################################################################
class Application(Frame):
    def __init__(self, master=None, nchnls=2, startf=None, stopf=None, 
                 recstartf=None, recstopf=None, ampf=None):
        Frame.__init__(self, master, padx=10, pady=10, bd=2, relief=GROOVE)
        self.nchnls = nchnls
        self.startf = startf
        self.stopf = stopf
        self.recstartf = recstartf
        self.recstopf = recstopf
        self.ampf = ampf
        self._started = False
        self._recstarted = False
        self.grid(ipadx=5)
        self.rowconfigure(0, pad=20)
        self.createWidgets()

    def createWidgets(self):
        self.startStringVar = StringVar()
        self.startStringVar.set('Start')
        self.startButton = Button(self, textvariable=self.startStringVar, command=self.start)
        self.startButton.grid(ipadx=5)

        self.recStringVar = StringVar()
        self.recStringVar.set('Rec Start')
        self.recButton = Button(self, textvariable=self.recStringVar, command=self.record)
        self.recButton.grid(ipadx=5, row=0, column=1)

        self.quitButton = Button(self, text='Quit', command=self.on_quit)
        self.quitButton.grid(ipadx=5, row=0, column=2)

        self.ampScale = Scale(self, command=self.setAmp, digits=4, label='Amplitude (dB)',
                              orient=HORIZONTAL, relief=GROOVE, from_=-60.0, to=18.0, 
                              resolution=.01, bd=1, length=250)
        self.ampScale.set(0.0)
        self.ampScale.grid(ipadx=5, ipady=5, row=1, column=0, columnspan=3)

        self.vumeter = Canvas(self, height=5*self.nchnls+1, width=250, relief=SUNKEN, bd=1)
        self.bars = []
        for i in range(self.nchnls):
            y = 5 * (i + 1) + 2
            self.bars.append(self.vumeter.create_line(0, y, 1, y, width=4, fill='blue', dash=(9,1), dashoff=5))
        self.vumeter.grid(ipadx=5, row=2, column=0, columnspan=3)
        
    def on_quit(self):
        if self._started:
            self.stopf()
        self.quit()

    def start(self):
        if self._started == False:
            self.startf()
            self._started = True
            self.startStringVar.set('Stop')
        else:
            self.stopf()
            self._started = False
            self.startStringVar.set('Start')

    def record(self):
        if self._recstarted == False:
            self.recstartf()
            self._recstarted = True
            self.recStringVar.set('Rec Stop')
        else:
            self.recstopf()
            self._recstarted = False
            self.recStringVar.set('Rec Start')

    def setAmp(self, value):
        self.ampf(math.pow(10.0, float(value) * 0.05))

    def setRms(self, *args):
        for i in range(self.nchnls):
            y = 5 * (i + 1) + 2
            self.vumeter.coords(self.bars[i], 0, y, int(args[i]*250), y)
        
######################################################################
### Proxy of Server object
######################################################################
class Server(object):
    """
    The Server object handles all communications with Portaudio and Portmidi.
    
    An instance of the Server must be booted before defining any signal processing chain.

    Parameters:

    sr : int, optional
        Sampling rate used by Portaudio and the Server to compute samples. Defaults to 44100.
    nchnls : int, optional
        Number of input and output channels. Defaults to 2.
    buffersize : int, optional
        Number of samples that Portaudio will request from the callback loop. This value has an impact
        on CPU use (a small buffer size is harder to compute) and on the latency of the system. Latency
        is `buffer size / sampling rate` in seconds. Defaults to 256.
    duplex : int {0, 1}, optional
        Input - output mode. 0 is output only and 1 is both ways. Defaults to 0.

    Methods:

    setAmp(x) : Set the overall amplitude.
    boot() : Boot the server. Must be called before defining any signal processing chain.
    shutdown() : Shut down and clear the server.
    start() : Start the audio callback loop.
    stop() : Stop the audio callback loop.
    recstart() : Begin recording sound sent to output. Create a fie called `pyo_rec.aif` 
        in the user's home directory.
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

    Attributes:
    
    amp : Overall amplitude of the Server. This value is applied on any stream sent to 
        the output.
        
    Examples:
    
    >>> # For an 8 channels server in duplex mode with
    >>> # a sampling rate of 48000 Hz and buffer size of 512
    >>> s = Server(sr=48000, nchnls=8, buffersize=512, duplex=1).boot()
    >>> s.start()
        
    """
    def __init__(self, sr=44100, nchnls=2, buffersize=256, duplex=0):
        self._nchnls = nchnls
        self._amp = 1.
        self._server = Server_base(sr, nchnls, buffersize, duplex)

    def gui(self):
        app = Application(None, self._nchnls, self.start, self.stop, self.recstart, self.recstop, self.setAmp)
        app.master.title("pyo Server")
        self._server.setAmpCallable(app)
        app.mainloop()
        
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
        Set the MIDI input device number. See `pm_list_devices()`.
        
        Parameters:

        x : int
            Number of the MIDI device listed by Portmidi.

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
        Shut down and clear the server. This method will erase all objects from the callback loop.
        This method need to be called before changing server's parameters like `samplingrate`, 
        `buffersize`, `nchnls`...

        """
        self._server.shutdown()
        
    def boot(self):
        """
        Boot the server. Must be called before defining any signal processing chain. Server's parameters
        like `samplingrate`, `buffersize` or `nchnls` will be effective after a call to this method.

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
        
    def recstart(self):
        """
        Begin a default recording of the sound that is sent to output. This will create a file
        called `pyo_rec.aif` in the user's home directory.
        
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

    #def demo():
    #    execfile(DEMOS_PATH + "/Server_demo.py")
    #demo = Call_example(demo)

    def args():
        return('Server(sr=44100, nchnls=2, buffersize=256, duplex=0)')
    args = Print_args(args)

    @property
    def amp(self):
        """float. Overall amplitude.""" 
        return self._amp
    @amp.setter
    def amp(self, x): self.setAmp(x) 
