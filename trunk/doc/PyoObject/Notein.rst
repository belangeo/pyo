:class:`Notein` --- MIDI note messages
======================================

.. class:: Notein(poly=10, scale=0, first=0, last=127, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Generate MIDI note messages.
    
    From a MIDI device, :class:`Notein` takes the notes in the range defined with `first` and `last` parameters,
    and outputs `poly` noteon - noteoff streams in the `scale` format (MIDI, hertz or transpo).
    
    :param poly: int, optional
    
    Number of streams of polyphony generated. Defaults to 10.

    :param scale: int, optional
    
    Pitch output format. 0 = MIDI, 1 = Hertz, 2 = transpo. In the transpo mode, the central key (the key where there is no transposition) is (`first` + `last`) / 2.

    :param first: int, optional
    
    Lowest MIDI value. Defaults to 0.

    :param last: int, optional
    
    Highest MIDI value. Defaults to 127.

.. note::

    Pitch and velocity are two separated set of streams. The user should call :

    Notein['pitch'] to retrieve pitch streams.

    Notein['velocity'] to retrieve velocity streams.    

.. note::

    Velocity is automatically scaled between 0 and 1.

.. note::

    The out() method is bypassed. :class:`Notein`'s signal can't be sent to audio outs.

**Example**

>>> s = Server().boot()
>>> s.start()
>>> mid = Notein(poly=10, scale=1, mul=.5)
>>> amp = Port(mid['velocity'], .001, .5)
>>> si1 = Sine(mid['pitch'], 0, amp).out()
>>> si2 = Sine(mid['pitch'] * 0.997, 0, amp).out()
