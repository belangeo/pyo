:class:`BandSplit` --- Frequency splitter
=========================================

.. class:: BandSplit(input, num=6, min=20, max=20000, q=1, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Split an input signal into multiple frequency bands.

    The signal will be filtered into `num` bands between `min` and `max` frequencies.
    Each band will then be assigned to an independent audio stream. Useful for multiband
    processing.

    :param input: :class:`PyoObject`
    
    Audio signal to filter.

    :param num: int, optional
    
    Number of frequency bands created. Initialization time only. Defaults to 6.
    
    :param min: float, optional
    
    Lowest frequency. Initialization time only. Defaults to 20.
    
    :param max: float, optional
    
    Highest frequency. Initialization time only. Defaults to 20000.
    
    :param q: float or :class:`PyoObject`, optional
    
    Q of the filter, defined as bandwidth/cutoff. 
    Should be between 1 and 500. Defaults to 1.



.. method:: BandSplit.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Defaults to 0.05.


.. method:: BandSplit.setQ(x)

    Replace the `q` attribute. Should be between 1 and 500.

    :param x: float or :class:`PyoObject`
    
    New `q` attribute.


.. attribute:: BandSplit.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: BandSplit.q

    float or :class:`PyoObject`. Q of the filter.

**Example**

>>> s = Server().boot()
>>> s.start()
>>> lfos = Sine(freq=[.3,.4,.5,.6,.7,.8], mul=.5, add=.5)
>>> a = BandSplit(Noise(.5), num=6, min=250, max=4000, q=5, mul=lfos).out()
