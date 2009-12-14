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
    
.. method:: SndTable.setSound(path)

    Load a new sound in the table.
        
    Keeps the number of channels of the sound loaded at initialisation.
    If the new sound has less channels, it will wrap around and load the 
    same channels many times. If the new sound has more channels, latest 
    channels will be skipped.

    :param path: string
    
    Full path name of the new sound.
    
.. method:: SndTable.getRate()

    Return the frequency in cps at which the sound will be read 
    at its original pitch.
