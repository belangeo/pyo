:class:`Input` --- Read samples from an external device
=======================================================

.. class:: Input(chnl=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Read from a numbered channel in an external audio signal or stream.
    
    :param chnl: int, optional
    
    Input channel to read from. Defaults to 0.

.. note:: Requires that the :class:`Server`'s duplex mode is set to 1.
