:class:`Osc` --- Table oscillator
=================================

.. class:: Osc(table, freq=1000, phase=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    A simple oscillator with linear interpolation reading a waveform table.
    
    :param table: :class:`PyoTableObject`
    
    Table containing the waveform samples.
    
    :param freq: float or :class:`PyoObject`, optional
    
    Frequency in cycles per second. Default to 1000.

    :param phase: float or :class:`PyoObject`, optional

     Phase of sampling, expressed as a fraction of a cycle (0 to 1). Default to 0.

.. method:: Osc.setFreq(x)

    Replace the *freq* attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: Osc.setPhase(x)

    Replace the *phase* attribute.

    :param x: float or :class:`PyoObject`
    
.. attribute:: Osc.freq

    float or :class:`PyoObject`. Frequency in cycles per second.

.. attribute:: Osc.phase

    float or :class:`PyoObject`. Phase of sampling, expressed as a fraction of a cycle (0 to 1).
