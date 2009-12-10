:class:`Biquad` --- Biquadratic filter
======================================

.. class:: Biquad(input, freq=1000, q=1, type=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    A sweepable general purpose biquadratic digital filter.

    :param input: :class:`PyoObject`
    
    Audio signal to filter.
    
    :param freq: float or :class:`PyoObject`, optional
    
    Cutoff or center frequency of the filter. Default to 1000.
    
    :param q: float or :class:`PyoObject`, optional
    
    Q of the filter, defined, for bandpass filters, as bandwidth/cutoff. 
    Should be between 1 and 500. Default to 1.

    :param type: int, optional
    
    Filter type. Four possible values :
    
        0 = lowpass (default)

        1 = highpass

        2 = bandpass

        3 = bandstop


.. method:: Biquad.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Default to 0.05.

.. method:: Biquad.setFreq(x)

    Replace the `freq` attribute.

    :param x: float or :class:`PyoObject`
    
    New `freq` attribute.
     
.. method:: Biquad.setQ(x)

    Replace the `q` attribute. Should be between 1 and 500.

    :param x: float or :class:`PyoObject`
    
    New `q` attribute.

.. method:: Biquad.setType(x)

    Replace the `type` attribute.

    :param x: int
    
    Filter type. Four possible values :
    
        0 = lowpass (default)

        1 = highpass

        2 = bandpass

        3 = bandstop
    
.. attribute:: Biquad.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: Biquad.freq

    float or :class:`PyoObject`. Cutoff or center frequency of the filter.

.. attribute:: Biquad.q

    float or :class:`PyoObject`. Q of the filter.

.. attribute:: Biquad.type

    int. Filter type.
