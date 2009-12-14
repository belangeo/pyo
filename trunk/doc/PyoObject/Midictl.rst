:class:`Midictl` --- MIDI channel controller
============================================

.. class:: Midictl(ctlnumber, minscale=0, maxscale=1, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Get the current value of a MIDI channel controller.
    
    Get the current value of a controller and optionally map it onto specified range.
    
    :param ctlnumber: int
    
    Midi channel. Initialisation time only.

    :param minscale: float, optional
    
    Low range for mapping. Initialisation time only.

    :param maxscale: float, optional
    
    High range for mapping. Initialisation time only.

.. note::

    Methods out() is bypassed. Midictl signal can't be sent to audio outs.
