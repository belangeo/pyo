:class:`LinTable` --- Segments of straight lines
================================================

.. class:: LinTable(list=[(0, 0.), (8191, 1.)], size=8192)

    Parent class : :class:`PyoTableObject`

    Construct a table from segments of straight lines in breakpoint fashion. 
    
    :param list: list, optional
        
    List of tuples indicating location and value of each point in the table. 
    The default, [(0,0.), (8191, 1.)], creates a straight line from 0.0 at location 0
    to 1.0 at the end of the table (size - 1). Location must be an integer.

    :param size: int, optional

    Table size in samples. Defaults to 8192.
    
.. method:: LinTable.setSize(size)

    Change the size of the table and rescale the envelope.
    
    :param size: int
    
    New table size in samples.

.. method:: LinTable.replace(list)

    Draw a new envelope according to the new `list` parameter.
    
    :param list: list
    
    List of tuples indicating location and value of each point in the table. 
    Location must be an integer.

.. note::

    Locations in the list must be in increasing order. If the last value is less 
    than size, then the rest will be set to zero. 
    
.. attribute:: LinTable.size

    int. Table size in samples.
    
.. attribute:: LinTable.list
    
    list. List of tuples [(location, value), ...].
