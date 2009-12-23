:class:`HarmTable` --- Composite waveform
=========================================

.. class:: HarmTable(list=[1], size=8192)

    Parent class : :class:`PyoTableObject`

    Generate composite waveforms made up of weighted sums of simple sinusoids. 
    
    :param list: list, optional
        
    Relative strengths of the fixed harmonic partial numbers 1,2,3, etc. Defaults to [1].

    :param size: int, optional

    Table size in samples. Defaults to 8192.
    
.. method:: HarmTable.setSize(size)

    Change the size of the table. This will erase the previously drawn waveform.
    
    :param size: int
    
    New table size in samples.

.. method:: HarmTable.replace(list)

    Redraw the waveform according to a new set of harmonic relative strengths.
    
    :param list: list
    
    Relative strengths of the fixed harmonic partial numbers 1,2,3, etc.
    
.. attribute:: HarmTable.size

    int. Table size in samples.
    
.. attribute:: HarmTable.list
    
    list. Relative strengths of the fixed harmonic partial numbers.
