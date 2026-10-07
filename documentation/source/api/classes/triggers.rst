Sample Accurate Timing (Triggers)
===================================

.. currentmodule:: pyo

A trigger is a one-sample audio signal whose value is 1, surrounded by 0s.
It represents an event at a precisely defined instant in the audio stream.

pyo uses triggers for sample-accurate timing and event-driven processing.
A trigger can start an envelope, choose a new random value, advance through
a sequence, play a sound, or execute a Python callback.

Triggers generators
------------------------------

Trigger generators produce streams of trigger pulses.

- :py:class:`Beat` :     Generates algorithmic trigger patterns.
- :py:class:`Change` :     Sends trigger that informs when input value has changed.
- :py:class:`Cloud` :     Generates random triggers.
- :py:class:`Euclide` :     Euclidean rhythm generator.
- :py:class:`Metro` :     Generates isochronous trigger signals.
- :py:class:`NextTrig` :     A trigger in the second stream opens a gate only for the next one in the first stream.
- :py:class:`Percent` :     Lets pass a certain percentage of the input triggers.
- :py:class:`Select` :     Sends trigger on matching integer values.
- :py:class:`Seq` :     Generates a rhythmic sequence of trigger signals.
- :py:class:`Thresh` :     Informs when a signal crosses a threshold.
- :py:class:`Trig` :     Sends one trigger.

Trigger-reactive objects
---------------------------

Trigger-reactive objects receive a trigger stream through their input argument.
Each incoming trigger starts, restarts, or updates their process.

- :py:class:`Count` :     Counts integers at audio rate.
- :py:class:`Counter` :     Integer count generator.
- :py:class:`Iter` :     Triggers iterate over a list of values.
- :py:class:`Timer` :     Reports elapsed time between two trigs.
- :py:class:`TrigBurst` :     Generates a time/amplitude expandable trigger pattern.
- :py:class:`TrigChoice` :     Random generator from user's defined values.
- :py:class:`TrigEnv` :     Envelope reader generator.
- :py:class:`TrigExpseg` :     Exponential segments trigger.
- :py:class:`TrigFunc` :     Python function callback.
- :py:class:`TrigLinseg` :     Line segments trigger.
- :py:class:`TrigRandInt` :     Pseudo-random integer generator.
- :py:class:`TrigRand` :     Pseudo-random number generator.
- :py:class:`TrigTableRec` :     TrigTableRec is for writing samples into a previously created table.
- :py:class:`TrigVal` :     Outputs a previously defined value on a trigger signal.
- :py:class:`TrigXnoise` :     Triggered X-class pseudo-random generator.
- :py:class:`TrigXnoiseMidi` :     Triggered X-class midi notes pseudo-random generator.

Generators
==========

*Beat*
-----------------------------------

.. autoclass:: Beat
   :members:

   .. autoclasstoc::

*Change*
-----------------------------------

.. autoclass:: Change
   :members:

   .. autoclasstoc::

*Cloud*
-----------------------------------

.. autoclass:: Cloud
   :members:

   .. autoclasstoc::


*Euclide*
-----------------------------------

.. autoclass:: Euclide
   :members:

   .. autoclasstoc::


*Metro*
-----------------------------------

.. autoclass:: Metro
   :members:

   .. autoclasstoc::

*NextTrig*
-----------------------------------

.. autoclass:: NextTrig
   :members:

   .. autoclasstoc::

*Percent*
-----------------------------------

.. autoclass:: Percent
   :members:

   .. autoclasstoc::

*Select*
-----------------------------------

.. autoclass:: Select
   :members:

   .. autoclasstoc::

*Seq*
-----------------------------------

.. autoclass:: Seq
   :members:

   .. autoclasstoc::

*Thresh*
-----------------------------------

.. autoclass:: Thresh
   :members:

   .. autoclasstoc::


*Trig*
-----------------------------------

.. autoclass:: Trig
   :members:

   .. autoclasstoc::

Triggered Processors
====================

*Count*
-----------------------------------

.. autoclass:: Count
   :members:

   .. autoclasstoc::

*Counter*
-----------------------------------

.. autoclass:: Counter
   :members:

   .. autoclasstoc::

*Iter*
-----------------------------------

.. autoclass:: Iter
   :members:

   .. autoclasstoc::

*Timer*
-----------------------------------

.. autoclass:: Timer
   :members:

   .. autoclasstoc::

*TrigBurst*
-----------------------------------

.. autoclass:: TrigBurst
   :members:

   .. autoclasstoc::

*TrigChoice*
-----------------------------------

.. autoclass:: TrigChoice
   :members:

   .. autoclasstoc::

*TrigEnv*
-----------------------------------

.. autoclass:: TrigEnv
   :members:

   .. autoclasstoc::

*TrigExpseg*
-----------------------------------

.. autoclass:: TrigExpseg
   :members:

   .. autoclasstoc::

*TrigFunc*
-----------------------------------

.. autoclass:: TrigFunc
   :members:

   .. autoclasstoc::

*TrigLinseg*
-----------------------------------

.. autoclass:: TrigLinseg
   :members:

   .. autoclasstoc::

*TrigRand*
-----------------------------------

.. autoclass:: TrigRand
   :members:

   .. autoclasstoc::

*TrigRandInt*
-----------------------------------

.. autoclass:: TrigRandInt
   :members:

   .. autoclasstoc::

*TrigTableRec*
-----------------------------------

.. autoclass:: TrigTableRec
   :members:

   .. autoclasstoc::

*TrigVal*
-----------------------------------

.. autoclass:: TrigVal
   :members:

   .. autoclasstoc::

*TrigXnoise*
-----------------------------------

.. autoclass:: TrigXnoise
   :members:

   .. autoclasstoc::

*TrigXnoiseMidi*
-----------------------------------

.. autoclass:: TrigXnoiseMidi
   :members:

   .. autoclasstoc::

