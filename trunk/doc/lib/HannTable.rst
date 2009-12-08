:class:`HannTable` --- Hanning envelope
=======================================

.. class:: HannTable(size=8192)

    Parent class : :class:`PyoTableObject`

    Generate Hanning window. 

    :param size: int, optional

    Table size in samples. Default to 8192.
    
.. method:: HannTable.setSize(size)

    Change the size of the table. This will redraw the envelope.
    
    :param size: int
    
    New table size in samples.

.. attribute:: HannTable.size

    int. Table size in samples.