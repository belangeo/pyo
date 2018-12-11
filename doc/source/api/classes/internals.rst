Internal objects
===================================

.. module:: pyo

These objects are mainly used by pyo itself, inside other objects.

*Dummy*
-----------------------------------

.. autoclass:: Dummy
   :members:

*InputFader*
-----------------------------------

.. autoclass:: InputFader
   :members:

*Mix*
-----------------------------------

.. autoclass:: Mix
   :members:

*VarPort*
-----------------------------------

.. autoclass:: VarPort
   :members:

*Stream*
-----------------------------------

*class* **Stream**

Audio stream objects. For internal use only.

A Stream object must never be instantiated by the user.

A Stream is a mono buffer of audio samples. It is used to pass audio between objects and the server. 
A PyoObject can manage many streams if, for example, a list is given to a parameter.

A Sine object with only one stream:

    >>> a = Sine(freq=1000)
    >>> print len(a)
    1

A Sine object with four streams:

    >>> a = Sine(freq=[250,500,750,100])
    >>> print len(a)
    4

The first stream of this object contains the samples from the 250Hz waveform. The second stream contains the samples from the 500Hz waveform, and so on.

User can call a specific stream of an object by giving the position of the stream between brackets, beginning at 0. To retrieve only the third stream of our object:

    >>> a[2].out()

The method getStreamObject() can be called on a Stream object to retrieve the XXX_base object associated with this Stream. 
This method can be used by developers who are debugging their programs! 

