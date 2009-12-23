:class:`NewTable` --- Empty table
=================================

.. class:: NewTable(length, chnls=1)

    Parent class : :class:`PyoTableObject`

    Create an empty table ready for recording. See :class:`TableRec` to write samples
    in the table. 
    
    :param length: float
        
    Length of the table in seconds.
    
    :param chnls: int, optional

    Number of channels that will be handled by the table. Defaults to 1.

.. method:: NewTable.getSize()

    Return the length of the table in samples.

.. method:: NewTable.getLength()

    Return the length of the table in seconds.

.. method:: NewTable.getRate()

    Return the frequency (cycles per second) at which the sound will be read when 
    at it's original pitch.
