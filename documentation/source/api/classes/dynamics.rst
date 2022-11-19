Dynamic management
============================

.. currentmodule:: pyo

Objects to modify the dynamic range and sample quality of audio signals.

Objects in this category
------------------------------

- :py:class:`Clip` :     Clips a signal to a predefined limit.
- :py:class:`Degrade` :     Signal quality reducer.
- :py:class:`Mirror` :     Reflects the signal that exceeds the `min` and `max` thresholds.
- :py:class:`Compress` :     Reduces the dynamic range of an audio signal.
- :py:class:`Gate` :     Allows a signal to pass only when its amplitude is above a set threshold.
- :py:class:`Balance` :     Adjust rms power of an audio signal according to the rms power of another.
- :py:class:`Min` :     Outputs the minimum of two values.
- :py:class:`Max` :     Outputs the maximum of two values.
- :py:class:`Wrap` :     Wraps-around the signal that exceeds the `min` and `max` thresholds.
- :py:class:`Expand` :     Expand the dynamic range of an audio signal.

*Clip*
----------

.. autoclass:: Clip
   :members:

   .. autoclasstoc::

*Degrade*
----------

.. autoclass:: Degrade
   :members:

   .. autoclasstoc::

*Mirror*
------------

.. autoclass:: Mirror
   :members:

   .. autoclasstoc::

*Compress*
------------

.. autoclass:: Compress
   :members:

   .. autoclasstoc::

*Gate*
------------

.. autoclass:: Gate
   :members:

   .. autoclasstoc::

*Balance*
------------

.. autoclass:: Balance
   :members:

   .. autoclasstoc::

*Min*
------------

.. autoclass:: Min
   :members:

   .. autoclasstoc::

*Max*
------------

.. autoclass:: Max
   :members:

   .. autoclasstoc::

*Wrap*
------------

.. autoclass:: Wrap
   :members:

   .. autoclasstoc::

*Expand*
------------

.. autoclass:: Expand
   :members:

   .. autoclasstoc::
