Open Sound Control
===================================

.. currentmodule:: pyo

Objects to manage values on an Open Sound Control port.

OscSend takes the first value of each buffersize and send it on an
OSC port.

OscReceive creates and returns audio streams from the value in its 
input port.

The audio streams of these objects are essentially intended to be
controls and can't be sent to the output soundcard.

.. note::
    
    These objects are available only if pyo is built with OSC (Open Sound 
    Control) support.

Objects in this category
------------------------------

- :py:class:`OscDataReceive` :     Receives data values over a network via the Open Sound Control protocol.
- :py:class:`OscDataSend` :     Sends data values over a network via the Open Sound Control protocol.
- :py:class:`OscListReceive` :     Receives list of values over a network via the Open Sound Control protocol.
- :py:class:`OscReceive` :     Receives values over a network via the Open Sound Control protocol.
- :py:class:`OscSend` :     Sends values over a network via the Open Sound Control protocol.

*OscDataReceive*
-----------------------------------

.. autoclass:: OscDataReceive
   :members:

   .. autoclasstoc::

*OscDataSend*
-----------------------------------

.. autoclass:: OscDataSend
   :members:

   .. autoclasstoc::

*OscListReceive*
-----------------------------------

.. autoclass:: OscListReceive
   :members:

   .. autoclasstoc::

*OscReceive*
-----------------------------------

.. autoclass:: OscReceive
   :members:

   .. autoclasstoc::

*OscSend*
-----------------------------------

.. autoclass:: OscSend
   :members:

   .. autoclasstoc::

