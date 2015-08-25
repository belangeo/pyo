from _core import *
from multiprocessing import Process, Array
import time, threading

class MidiListener(Process):
    """
    Self-contained midi listener process.

    This object allows to setup a Midi server that is independent
    of the audio server (mainly to be able to receive Midi data even
    when the audio server is stopped). Although it runs in a separated
    process, the same device can't be used by this object and the audio
    server at the same time. It is adviced to call the deactivateMidi() 
    method on the audio server to avoid conflicts.

    :Parent: multiprocessing.Process

    :Args:

        function : Python function (can't be a list)
            Function that will be called when a new midi event is available. 
            This function is called without arguments. The Midi data is 
            stored in the array returned by the getValueArray() method.
        mididev : int, optional
            Sets the midi input device (see `pm_list_devices()` for the 
            available devices). The default, -1, means the system default
            device. A number greater than the highest portmidi device index 
            will opened all available input devices. 

    >>> s = Server().boot()
    >>> s.deactivateMidi()
    >>> def midicall():
    ...     print value[0], value[1], value[2]
    >>> listen = MidiListener(midicall, 5)
    >>> value = listen.getValueArray()
    >>> listen.start()

    """
    def __init__(self, function, mididev=-1):
        Process.__init__(self)
        self.daemon = True
        self.value = Array("i", [0,0,0])
        self._function = WeakMethod(function)
        self._mididev = mididev
        self._listener = MidiListener_base(self._midirecv, self._mididev)

    def getValueArray(self):
        """
        Returns the array containing midi values.
        
        This array must be kept in the main thread for inspection when
        the function is called from the process. Stored values are:
            
        array[0] = status byte
        array[1] = first data byte
        array[2] = second data byte

        """
        return self.value

    def _midirecv(self, status, data1, data2):
        self.value[0], self.value[1], self.value[2] = status, data1, data2
        self._function()
        
    def run(self):
        """
        Starts the process. The process runs as daemon, so no need to stop it.
        
        """
        self._listener.play()
        while True:
            time.sleep(0.001)

OscListenerLock = threading.Lock()

class OscListener(threading.Thread):
    """
    Self-contained OSC listener thread.

    This object allows to setup an OSC server that is independent
    of the audio server (mainly to be able to receive OSC data even
    when the audio server is stopped). 

    :Parent: threadind.Thread

    :Args:

        function : Python function (can't be a list)
            Function that will be called when a new OSC event is available. 
            This function is called the incoming address and values as 
            arguments. The signature of the function must be:
                
            def myfunc(address, *args)

        port : int, optional
            The OSC port on which the values are received. Defaults to 9000. 

    >>> s = Server().boot()
    >>> def call(address, *args):
    ...     print address, args
    >>> listen = OscListener(call, 9901)
    >>> listen.start()

    """
    def __init__(self, function, port=9000):
        threading.Thread.__init__(self)
        self.daemon = True
        self._function = WeakMethod(function)
        self._port = port
        self._listener = OscListener_base(self._oscrecv, self._port)

    def _oscrecv(self, address, *args):
        with OscListenerLock:
            self._function(address, *args)

    def run(self):
        """
        Starts the process. The thread runs as daemon, so no need to stop it.
        
        """
        while True:
            self._listener.get()
            time.sleep(0.001)
