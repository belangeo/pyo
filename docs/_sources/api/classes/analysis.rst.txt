Audio Signal Analysis
============================

.. currentmodule:: pyo

Tools to analyze audio signal characteristics.

These objects are designed to extract specific information and meaning from an audio stream.
Analysis data is stored in the object's internal buffer, as an audio rate signal (except for
the Scope and Spectrum objects, which are mainly used for visualization, and can share their
results via function handlers). The user can use these objects for controlling parameters of others objects.

Objects in this category
------------------------------

- :py:class:`Follower` :     Envelope follower.
- :py:class:`Follower2` :     Envelope follower with different attack and release times.
- :py:class:`ZCross` :     Zero-crossing counter.
- :py:class:`Yin` :     Pitch tracker using the Yin algorithm.
- :py:class:`Centroid` :     Computes the spectral centroid of an input signal.
- :py:class:`AttackDetector` :     Audio signal onset detection.
- :py:class:`Spectrum` :     Spectrum analyzer and display.
- :py:class:`Scope` :     Oscilloscope - audio waveform display.
- :py:class:`PeakAmp` :     Peak amplitude follower.
- :py:class:`RMS` :     Returns the RMS (Root-Mean-Square) value of a signal.

*Follower*
----------

.. autoclass:: Follower
   :members:

   .. autoclasstoc::

*Follower2*
------------

.. autoclass:: Follower2
   :members:

   .. autoclasstoc::

*ZCross*
------------

.. autoclass:: ZCross
   :members:

   .. autoclasstoc::

*Yin*
------------

.. autoclass:: Yin
   :members:

   .. autoclasstoc::

*Centroid*
------------

.. autoclass:: Centroid
   :members:

   .. autoclasstoc::

*AttackDetector*
-----------------

.. autoclass:: AttackDetector
   :members:

   .. autoclasstoc::

*Spectrum*
-----------------------------------

.. autoclass:: Spectrum
   :members:

   .. autoclasstoc::

*Scope*
-----------------------------------

.. autoclass:: Scope
   :members:

   .. autoclasstoc::

*PeakAmp*
-----------------------------------

.. autoclass:: PeakAmp
   :members:

   .. autoclasstoc::

*RMS*
-----------------------------------

.. autoclass:: RMS
   :members:

   .. autoclasstoc::
