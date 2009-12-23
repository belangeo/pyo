:class:`Port` --- Exponential portamento
========================================

.. class:: Port(input, risetime=0.05, falltime=0.05, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Perform an exponential portamento on an audio signal with different rising and falling times.

    :param input: :class:`PyoObject`
    
    Audio signal to filter.
    
    :param risetime: float or :class:`PyoObject`, optional
    
    Time to reach upward value in seconds. Defaults to 0.05.
    
    :param falltime: float or :class:`PyoObject`, optional
    
    Time to reach downward value in seconds. Defaults to 0.05.


.. method:: Port.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Defaults to 0.05.

.. method:: Port.setRiseTime(x)

    Replace the `risetime` attribute.

    :param x: float or :class:`PyoObject`
     
.. method:: Port.setFallTime(x)

    Replace the `falltime` attribute.

    :param x: float or :class:`PyoObject`


.. attribute:: Port.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: Port.risetime

    float or :class:`PyoObject`. Time to reach upward value in seconds.

.. attribute:: Port.falltime

    float or :class:`PyoObject`. Time to reach downward value in seconds.
