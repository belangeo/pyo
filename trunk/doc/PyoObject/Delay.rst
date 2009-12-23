:class:`Delay` --- Sweepable recursive delay
============================================

.. class:: Delay(input, delay=0.25, feedback=0, maxdelay=1, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Sweepable recursive delay.

    :param input: :class:`PyoObject`
    
    Audio signal to process.
    
    :param delay: float or :class:`PyoObject`, optional
    
    Delay time in seconds. Defaults to 0.25.    
    
    :param feedback: float or :class:`PyoObject`, optional
    
    Amount of output signal sent back into the delay line, between 0 and 1. 
    Defaults to 0.

    :param maxdelay:, float, optional
    
    Maximum delay length in seconds. Available only at initialization. 
    Defaults to 1.

.. method:: Delay.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Defaults to 0.05.

.. method:: Delay.setDelay(x)

    Replace the `delay` attribute.

    :param x: float or :class:`PyoObject`
    
    New `delay` attribute.
     
.. method:: Delay.setFeedback(x)

    Replace the `feedback` attribute.

    :param x: float or :class:`PyoObject`
    
    New `feedback` attribute, between 0 and 1.


.. attribute:: Delay.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: Delay.delay

    float or :class:`PyoObject`. Delay time in seconds.

.. attribute:: Delay.feedback

    float or :class:`PyoObject`. Recursive multiplier.
