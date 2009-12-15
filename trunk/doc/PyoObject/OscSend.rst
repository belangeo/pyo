:class:`OscSend` --- Send values via OSC
========================================

.. class:: OscSend(input, port, address, host='127.0.0.1')

    Parent class : :class:`PyoObject`

    Sends values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to share values to other software or other computer.
    Only the first value of each input buffersize will be sent on the OSC port.

    :param input: :class:`PyoObject`
    
    Input signal.
    
    :param port: int
    
    Port on which values are sent. Receiver should listen on the same port.
    
    :param address: string
    
    Address used on the port to identified value. Address is in the form 
    of a Unix path (ex.: '/pitch')

    :param host: string, optional
    
    IP address of the target computer. The default ('127.0.0.1') is the localhost.

.. method:: OscSend.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal.


.. attribute:: OscSend.input

    :class:`PyoObject`. Input signal to process.

.. note::

    Methods out() is bypassed. OscSend signal can't be sent to audio outs.
    
    OscSend has no `mul` and `add` attributes.

.. seealso:: :class:`OscReceive`
