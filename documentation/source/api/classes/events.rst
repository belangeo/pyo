Events framework
============================

.. currentmodule:: pyo

Set of tools to generate sequence of events.

The purpose of the Event framework is to allow the user to generate a
sequence of events with as few as possible parameters to specify.

:py:class:`Events` is the heart of the framework. An Events object computes
parameters, generally designed with event generator objects, builds the events
and plays the sequence.

See the **Events framework** examples in the documentation for different use cases.

Objects in this category
------------------------------

- :py:class:`EventCall` :     Calls a function, with any number of arguments, and uses its return value.
- :py:class:`EventChoice` :     Plays values randomly chosen from a list.
- :py:class:`EventConditional` :     Executes one generator or the other depending on the result of a condition.
- :py:class:`EventDrunk` :     Performs a random walk over a list of values.
- :py:class:`EventDummy` : An EventGenerator created internally to handle arithmetic on Events.
- :py:class:`EventFilter` : An EventGenerator created internally to handle simple filter on Events.
- :py:class:`EventGenerator` :     Base class for all event generators.
- :py:class:`EventIndex` :     Plays values from a list based on a position index.
- :py:class:`EventInstrument` :     Base class for an Events instrument. All attributes given to the Events
- :py:class:`EventKey` :     An EventGenerator that allow to retrieve the value of another parameter.
- :py:class:`EventMarkov` :     Applies a Markov algorithm to a list of values.
- :py:class:`EventNoise` :     Return a random value between -1.0 and 1.0.
- :py:class:`EventScale` :     Musical scale builder.
- :py:class:`EventSeq` :     Plays through an entire list of values many times.
- :py:class:`EventSlide` :     Plays overlapping segments from a list of values.
- :py:class:`Events` :     Sequencing user-defined events to form musical phrases.

*EventScale*
-------------

.. autoclass:: EventScale
   :members:

   .. autoclasstoc::

*EventGenerator*
------------------

.. autoclass:: EventGenerator
   :members:

   .. autoclasstoc::

*EventDummy*
------------------

.. autoclass:: EventDummy
   :members:

   .. autoclasstoc::

*EventFilter*
------------------

.. autoclass:: EventFilter
   :members:

   .. autoclasstoc::

*EventKey*
------------------

.. autoclass:: EventKey
   :members:

   .. autoclasstoc::

*EventSeq*
------------------

.. autoclass:: EventSeq
   :members:

   .. autoclasstoc::

*EventSlide*
------------------

.. autoclass:: EventSlide
   :members:

   .. autoclasstoc::

*EventIndex*
------------------

.. autoclass:: EventIndex
   :members:

   .. autoclasstoc::

*EventMarkov*
------------------

.. autoclass:: EventMarkov
   :members:

   .. autoclasstoc::

*EventChoice*
------------------

.. autoclass:: EventChoice
   :members:

   .. autoclasstoc::

*EventDrunk*
------------------

.. autoclass:: EventDrunk
   :members:

   .. autoclasstoc::

*EventNoise*
------------------

.. autoclass:: EventNoise
   :members:

   .. autoclasstoc::

*EventCall*
------------------

.. autoclass:: EventCall
   :members:

   .. autoclasstoc::

*EventConditional*
------------------

.. autoclass:: EventConditional
   :members:

   .. autoclasstoc::

*EventInstrument*
------------------

.. autoclass:: EventInstrument
   :members:

   .. autoclasstoc::

*Events*
------------------

.. autoclass:: Events
   :members:

   .. autoclasstoc::
