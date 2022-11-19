Table Processing
===================================

.. currentmodule:: pyo

Set of objects to perform operations on PyoTableObjects.

PyoTableObjects are 1 dimension containers. They can be used to 
store audio samples or algorithmic sequences for future uses.

Objects in this category
------------------------------

- :py:class:`Granulator` :     Granular synthesis generator.
- :py:class:`Granule` :     Another granular synthesis generator.
- :py:class:`Lookup` :     Uses table to do waveshaping on an audio signal.
- :py:class:`Looper` :     Crossfading looper.
- :py:class:`Osc` :     A simple oscillator reading a waveform table.
- :py:class:`OscBank` :     Any number of oscillators reading a waveform table.
- :py:class:`OscLoop` :     A simple oscillator with feedback reading a waveform table.
- :py:class:`OscTrig` :     An oscillator reading a waveform table with sample accurate reset signal.
- :py:class:`Particle` :     A full control granular synthesis generator.
- :py:class:`Particle2` :     An even more full control granular synthesis generator.
- :py:class:`Pointer` :     Table reader with control on the pointer position.
- :py:class:`Pointer2` :     High quality table reader with control on the pointer position.
- :py:class:`Pulsar` :     Pulsar synthesis oscillator.
- :py:class:`TableIndex` :     Table reader by sample position without interpolation.
- :py:class:`TableMorph` :     Morphs between multiple PyoTableObjects.
- :py:class:`TablePut` :     Writes values, without repetitions, from an audio stream into a table.
- :py:class:`TableRead` :     Simple waveform table reader.
- :py:class:`TableRec` :     TableRec is for writing samples into a previously created table.
- :py:class:`TableWrite` :     TableWrite writes samples into a previously created table.
- :py:class:`TableScale` :     Scales all the values contained in a PyoTableObject.
- :py:class:`TableFill` :     Continuously fills a table with incoming samples.
- :py:class:`TableScan` :     Reads the content of a table in loop, without interpolation.

*Granulator*
-----------------------------------

.. autoclass:: Granulator
   :members:

   .. autoclasstoc::

*Granule*
-----------------------------------

.. autoclass:: Granule
   :members:

   .. autoclasstoc::

*Lookup*
-----------------------------------

.. autoclass:: Lookup
   :members:

   .. autoclasstoc::

*Looper*
-----------------------------------

.. autoclass:: Looper
   :members:

   .. autoclasstoc::

*Osc*
-----------------------------------

.. autoclass:: Osc
   :members:

   .. autoclasstoc::

*OscBank*
-----------------------------------

.. autoclass:: OscBank
   :members:

   .. autoclasstoc::

*OscLoop*
-----------------------------------

.. autoclass:: OscLoop
   :members:

   .. autoclasstoc::

*OscTrig*
-----------------------------------

.. autoclass:: OscTrig
   :members:

   .. autoclasstoc::

*Particle*
-----------------------------------

.. autoclass:: Particle
   :members:

   .. autoclasstoc::

*Particle2*
-----------------------------------

.. autoclass:: Particle2
   :members:

   .. autoclasstoc::

*Pointer*
-----------------------------------

.. autoclass:: Pointer
   :members:

   .. autoclasstoc::

*Pointer2*
-----------------------------------

.. autoclass:: Pointer2
   :members:

   .. autoclasstoc::

*Pulsar*
-----------------------------------

.. autoclass:: Pulsar
   :members:

   .. autoclasstoc::

*TableIndex*
-----------------------------------

.. autoclass:: TableIndex
   :members:

   .. autoclasstoc::

*TableMorph*
-----------------------------------

.. autoclass:: TableMorph
   :members:

   .. autoclasstoc::

*TablePut*
-----------------------------------

.. autoclass:: TablePut
   :members:

   .. autoclasstoc::

*TableRead*
-----------------------------------

.. autoclass:: TableRead
   :members:

   .. autoclasstoc::

*TableRec*
-----------------------------------

.. autoclass:: TableRec
   :members:

   .. autoclasstoc::

*TableWrite*
-----------------------------------

.. autoclass:: TableWrite
   :members:

   .. autoclasstoc::

*TableScale*
-----------------------------------

.. autoclass:: TableScale
   :members:

   .. autoclasstoc::

*TableFill*
-----------------------------------

.. autoclass:: TableFill
   :members:

   .. autoclasstoc::

*TableScan*
-----------------------------------

.. autoclass:: TableScan
   :members:

   .. autoclasstoc::
