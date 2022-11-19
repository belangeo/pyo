Control Signals
============================

.. currentmodule:: pyo

Objects designed to create parameter's control at audio rate. 

These objects can be used to create envelopes, line segments 
and conversion from python number to audio signal. 

The audio streams of these objects can't be sent to the output 
soundcard.

Objects in this category
------------------------------

- :py:class:`Fader` :     Fadein - fadeout envelope generator.
- :py:class:`Adsr` :     Attack - Decay - Sustain - Release envelope generator.
- :py:class:`Linseg` :     Draw a series of line segments between specified break-points.
- :py:class:`Expseg` :     Draw a series of exponential segments between specified break-points.
- :py:class:`Sig` :     Convert numeric value to PyoObject signal.
- :py:class:`SigTo` :     Convert numeric value to PyoObject signal with portamento.

*Fader*
----------

.. autoclass:: Fader
   :members:

   .. autoclasstoc::

*Adsr*
------------

.. autoclass:: Adsr
   :members:

   .. autoclasstoc::

*Linseg*
------------

.. autoclass:: Linseg
   :members:

   .. autoclasstoc::

*Expseg*
------------

.. autoclass:: Expseg
   :members:

   .. autoclasstoc::

*Sig*
------------

.. autoclass:: Sig
   :members:

   .. autoclasstoc::

*SigTo*
------------

.. autoclass:: SigTo
   :members:

   .. autoclasstoc::

