:class:`Fader` --- Envelope generator
=====================================

.. class:: Fader(fadein=0.01, fadeout=0.1, dur=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Generate an amplitude envelope between 0 and 1 with control on fade times and total duration of the envelope.
    
    The play() method starts the envelope and is not called at the object creation time.

    :param fadein: float or :class:`PyoObject`, optional
    
    Rising time of the envelope in seconds. Defaults to 0.01.
    
    :param fadeout: float or :class:`PyoObject`, optional
    
    Falling time of the envelope in seconds. Defaults to 0.1.
    
    :param dur: float or :class:`PyoObject`, optional
    
    Total duration of the envelope. Defaults to 0, which means wait for the stop() method to start the fadeout.

.. method:: Fader.play()

    Start processing without sending samples to the output. Triggers the envelope.
        
.. method:: Fader.stop()
    
    Stop processing. Triggers the envelope's fadeout if `dur` is set to 0.
 
.. method:: Fader.setFadein(x)

    Replace the *fadein* attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: Fader.setFadeout(x)

    Replace the *fadeout* attribute.

    :param x: float or :class:`PyoObject`
    
.. method:: Fader.setDur(x)

    Replace the *dur* attribute.

    :param x: float or :class:`PyoObject`

.. note::

    The out() method is bypassed. :class:`Fader`'s signal can't be sent to audio outs.
    
.. attribute:: Fader.fadein

    float or :class:`PyoObject`. Rising time of the envelope in seconds.

.. attribute:: Fader.fadeout

    float or :class:`PyoObject`. Falling time of the envelope in seconds.

.. attribute:: Fader.dur

    float or :class:`PyoObject`. Total duration of the envelope.
