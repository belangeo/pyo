Soundfile
=============================

Functions to inspect or write an audio file on disk.

.. currentmodule:: pyo

Functions in this category
------------------------------

- :py:func:`sndinfo` :     Retrieve informations about a soundfile.
- :py:func:`savefile` :     Creates an audio file from a list of floats.
- :py:func:`savefileFromTable` :     Creates an audio file from the content of a table.

*sndinfo*
---------------------------------

.. autofunction:: sndinfo(path, print=False)

*savefile*
---------------------------------

.. autofunction:: savefile(samples, path, sr=44100, channels=1, fileformat=0, sampletype=0, quality=0.4)

*savefileFromTable*
---------------------------------

.. autofunction:: savefileFromTable(table, path, fileformat=0, sampletype=0, quality=0.4)

