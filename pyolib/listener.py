from __future__ import absolute_import
from ._core import *
import time
import threading

class MidiListener(threading.Thread):
    """
    Self-contained midi listener thread.

    This object allows to setup a Midi server that is independent
    of the audio server (mainly to be able to receive Midi data even
    when the audio server is stopped). Although it runs in a separated
    thread, the same device can't be used by this object and the audio
    server at the same time. It is adviced to call the deactivateMidi() 
    method on the audio server to avoid conflicts.

    :Parent: threading.Thread

    :Args:

        function : Python function (can't be a list)
            Function that will be called when a new midi event is available. 
            This function is called with the incoming midi data as 
            arguments. The signature of the function must be:
                
            def myfunc(status, data1, data2)

        mididev : int, optional
            Sets the midi input device (see `pm_list_devices()` for the 
            available devices). The default, -1, means the system default
            device. A number greater than the highest portmidi device index 
            will open all available input devices. 

    .. note::

        This object is available only if pyo is built with portmidi support
        (see withPortmidi function).

    >>> s = Server().boot()
    >>> s.deactivateMidi()
    >>> def midicall(status, data1, data2):
    ...     print status, data1, data2
    >>> listen = MidiListener(midicall, 5)
    >>> listen.start()

    """
    def __init__(self, function, mididev=-1):
        threading.Thread.__init__(self)
        self.daemon = True
        self._function = WeakMethod(function)
        self._mididev = mididev
        self._listener = MidiListener_base(self._function, self._mididev)
        
    def run(self):
        """
        Starts the process. The thread runs as daemon, so no need to stop it.
        
        """
        self._listener.play()
        while True:
            try:
                time.sleep(0.001)
            except:
                pass

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
            This function is called with the incoming address and values as 
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
            try:
                time.sleep(0.001)
            except:
                pass
