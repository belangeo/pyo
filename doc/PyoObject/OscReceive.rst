:class:`OscReceive` --- Receive values via OSC
==============================================

.. class:: OscReceive(port, address, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Receive values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive values from other softwares or other computers.
    Gets a value at the beginning of each buffersize and fill it's buffer with it.
  
    :param port: int
    
    Port on which values are received. The OSC sender should output on the same port.
    
    :param address: string
    
    Address used on the port to identify values. Address is in the form 
    of a Unix path (ex.: '/pitch')


.. note::

    Audio streams are accessed with the `address` string parameter. The user should call :

    OscReceive['/pitch'] to retreive streams named '/pitch'.

.. note::

    The out() method is bypassed. :class:`OscReceive`'s signal can't be sent to audio outs.
    

.. seealso:: :class:`OscSend`
