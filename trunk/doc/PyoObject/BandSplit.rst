:class:`BandSplit` --- Frequency splitter
=========================================

.. class:: BandSplit(input, num=6, min=20, max=20000, q=1, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Split an input signal into multiple frequency bands.

    The signal will be filtered into `num` bands between `min` and `max` frequencies
    and each band will be assigned to an independent audio stream. Useful for multi-bands
    processing.

    :param input: :class:`PyoObject`
    
    Audio signal to filter.

    :param num: int, optional
    
    Number of frequency bands created. Initialisation time only. Default to 6.
    
    :param min: float, optional
    
    Lowest frequency. Initialisation time only. Default to 20.
    
    :param max: float, optional
    
    Highest frequency. Initialisation time only. Default to 20000.
    
    :param q: float or :class:`PyoObject`, optional
    
    Q of the filter, defined as bandwidth/cutoff. 
    Should be between 1 and 500. Default to 1.



.. method:: BandSplit.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Default to 0.05.


.. method:: BandSplit.setQ(x)

    Replace the `q` attribute. Should be between 1 and 500.

    :param x: float or :class:`PyoObject`
    
    New `q` attribute.


.. attribute:: BandSplit.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: BandSplit.q

    float or :class:`PyoObject`. Q of the filter.
