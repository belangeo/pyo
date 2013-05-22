Open Sound Control
===================================

.. module:: pyo

Objects to manage values on an Open Sound Control port.

OscSend takes the first value of each buffersize and send it on an
OSC port.

OscReceive creates and returns audio streams from the value in its 
input port.

The audio streams of these objects are essentially intended to be
controls and can't be sent to the output soundcard.

*OscDataReceive*
-----------------------------------

.. autoclass:: OscDataReceive
   :members:

*OscDataSend*
-----------------------------------

.. autoclass:: OscDataSend
   :members:

*OscListReceive*
-----------------------------------

.. autoclass:: OscListReceive
   :members:

*OscReceive*
-----------------------------------

.. autoclass:: OscReceive
   :members:

*OscSend*
-----------------------------------

.. autoclass:: OscSend
   :members:

