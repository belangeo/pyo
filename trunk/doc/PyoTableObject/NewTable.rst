:class:`NewTable` --- Empty table
=================================

.. class:: NewTable(length, chnls=1)

    Parent class : :class:`PyoTableObject`

    Create an empty table ready for recording. See `TableRec` to write samples
    in the table. 
    
    :param length: float
        
    Length of the table in seconds.
    
    :param chnls: int, optional

    How many channels will be handled by the table. Default to 1.

.. method:: NewTable.getSize()

    Return the length of the table in samples.

.. method:: NewTable.getLength()

    Return the length of the table in seconds.

.. method:: NewTable.getRate()

    Return the frequency in cps at which the sound will be read 
    at its original pitch.
