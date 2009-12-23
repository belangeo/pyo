:class:`Disto` --- Arctan Distortion
====================================

.. class:: Disto(input, drive=0.75, slope=0.5, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Apply an arctan distortion with controllable drive to the input signal.

    :param input: :class:`PyoObject`
    
    Audio signal to process.
    
    :param drive: float or :class:`PyoObject`, optional
    
    Amount of distortion applied to the signal, between 0 and 1. 
    Defaults to 0.75.
    
    :param slope: float or :class:`PyoObject`, optional
    
    Slope of the lowpass filter applied after distortion, between 0 and 1. 
    Defaults to 0.5.


.. method:: Disto.setInput(x, fadetime=0.05)

    Replace the `input` attribute.

    :param x: :class:`PyoObject`

    New input signal to process.

    :param fadetime: float, optional

    Crossfade time between old and new input. Defaults to 0.05.

.. method:: Disto.setDrive(x)

    Replace the `drive` attribute.

    :param x: float or :class:`PyoObject`
    
    New `drive` attribute, between 0 and 1.
     
.. method:: Disto.setSlope(x)

    Replace the `slope` attribute.

    :param x: float or :class:`PyoObject`
    
    New `slope` attribute, between 0 and 1.


.. attribute:: Disto.input

    :class:`PyoObject`. Input signal to process.

.. attribute:: Disto.drive

    float or :class:`PyoObject`. Amount of distortion.

.. attribute:: Disto.slope

    float or :class:`PyoObject`. Lowpass filter slope.
