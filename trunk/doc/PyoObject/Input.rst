:class:`Input` --- Read samples from an external device
=======================================================

.. class:: Input(chnl=0, mul=1, add=0)

    Parent class : :class:`PyoObject`

    Reads from a numbered channel in an external audio signal or stream.
    
    :param chnl: int, optional
    
    Input channel to read from. Default to 0.

.. note:: Requires Server's duplex mode set to 1.