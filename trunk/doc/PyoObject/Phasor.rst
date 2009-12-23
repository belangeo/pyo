:class:`Phasor` --- Phase incrementor
=====================================

.. class:: Phasor(freq=100, phase=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    A simple phase incrementor.
    
    Output is a periodic ramp from 0 to 1.

    :param freq: float or :class:`PyoObject`, optional
    
    Frequency in cycles per second. Defaults to 100.
    
    :param phase: float or :class:`PyoObject`, optional
    
     Phase of sampling, expressed as a fraction of a cycle (0 to 1). Defaults to 0.

.. method:: Phasor.setFreq(x)

    Replace the `freq` attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: Phasor.setPhase(x)

    Replace the `phase` attribute.

    :param x: float or :class:`PyoObject`
    
.. attribute:: Phasor.freq

    float or :class:`PyoObject`. Frequency in cycles per second.

.. attribute:: Phasor.phase

    float or :class:`PyoObject`. Phase of sampling, expressed as a fraction of a cycle (0 to 1).

**Example**

>>> s = Server().boot()
>>> s.start()
>>> ph = Phasor(freq=.25, mul=100, add=500)
>>> sine = Sine(freq=ph).out()

.. seealso:: :class:`Osc`, :class:`Sine`
