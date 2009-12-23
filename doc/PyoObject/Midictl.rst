:class:`Midictl` --- MIDI channel controller (CC)
=================================================

.. class:: Midictl(ctlnumber, minscale=0, maxscale=1, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Get the current value of a MIDI channel controller.
    
    Get the current value of a controller and optionally map it inside a specified range.
    
    :param ctlnumber: int
    
    MIDI channel. Available at initialization time only.

    :param minscale: float, optional
    
    Low range value for mapping. Available at initialization time only.

    :param maxscale: float, optional
    
    High range value for mapping. Available at initialization time only.

.. note::

    The out() method is bypassed. :class:`Midictl`'s signal can't be sent to audio outs.
