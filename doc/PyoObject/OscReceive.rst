:class:`OscReceive` --- Receive values via OSC
==============================================

.. class:: OscReceive(port, address, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Receives values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive values from other software or other computer.
    Get a value at the beginning of each buffersize and fill his buffer with it.
  
    :param port: int
    
    Port on which values are received. Sender should output on the same port.
    
    :param address: string
    
    Address used on the port to identified value. Address is in the form 
    of a Unix path (ex.: '/pitch')


.. note::

    Audio streams are accessed with the `address` string parameter. User should call :

    OscReceive['/pitch'] to retreive streams named '/pitch'.

.. note::

    Methods out() is bypassed. OscReceive signal can't be sent to audio outs.
    

.. seealso:: :class:`OscSend`
