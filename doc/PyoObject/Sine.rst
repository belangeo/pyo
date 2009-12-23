:class:`Sine` --- Simple oscillator
===================================

.. class:: Sine(freq=1000, phase=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    A simple oscillator.
    
    :param freq: float or :class:`PyoObject`, optional
    
    Frequency in cycles per second. Defaults to 1000.
    
    :param phase: float or :class:`PyoObject`, optional
    
     Phase of sampling, expressed as a fraction of a cycle (0 to 1). Defaults to 0.

.. method:: Sine.setFreq(x)

    Replace the `freq` attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: Sine.setPhase(x)

    Replace the `phase` attribute.

    :param x: float or :class:`PyoObject`
    
.. attribute:: Sine.freq

    float or :class:`PyoObject`. Frequency in cycles per second.

.. attribute:: Sine.phase

    float or :class:`PyoObject`. Phase of sampling, expressed as a fraction of a cycle (0 to 1).

**Example**

>>> s = Server().boot()
>>> s.start()
>>> sine = Sine(freq=500).out()

.. seealso:: :class:`Osc`
