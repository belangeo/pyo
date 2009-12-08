:class:`SndTable` --- Transfers data from soundfile
===================================================

.. class:: SndTable(path, chnl=None)

    Parent class : :class:`PyoTableObject`

    Load data from a soundfile into a function table.

    If `chnl` is None, the table will contain as many sub tables as necessary 
    to read all channels of the sound.    
    
    :param path: string
        
    Full path name of the sound.
    
    :param chnl: int, optional

    Channel number to read in. The default, None, denotes read all channels.
    
.. method:: SndTable.getRate()

    Return the frequency in cps at which the sound will be read 
    at its original pitch.
