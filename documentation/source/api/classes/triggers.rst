Sample Accurate Timing (Triggers)
===================================

.. currentmodule:: pyo

Set of objects to manage triggers streams.

A trigger is an audio signal with a value of 1 surrounded by 0s.

TrigXXX objects use this kind of signal to generate different 
processes with sample rate timing accuracy.

Objects in this category
------------------------------

- :py:class:`Beat` :     Generates algorithmic trigger patterns.
- :py:class:`Change` :     Sends trigger that informs when input value has changed.
- :py:class:`Cloud` :     Generates random triggers.
- :py:class:`Count` :     Counts integers at audio rate.
- :py:class:`Counter` :     Integer count generator.
- :py:class:`Euclide` :     Euclidean rhythm generator.
- :py:class:`Iter` :     Triggers iterate over a list of values.
- :py:class:`Metro` :     Generates isochronous trigger signals.
- :py:class:`NextTrig` :     A trigger in the second stream opens a gate only for the next one in the first stream.
- :py:class:`Percent` :     Lets pass a certain percentage of the input triggers.
- :py:class:`Select` :     Sends trigger on matching integer values.
- :py:class:`Seq` :     Generates a rhythmic sequence of trigger signals.
- :py:class:`Thresh` :     Informs when a signal crosses a threshold.
- :py:class:`Timer` :     Reports elapsed time between two trigs.
- :py:class:`Trig` :     Sends one trigger.
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

*Euclide*
-----------------------------------

.. autoclass:: Euclide
   :members:

   .. autoclasstoc::

*Iter*
-----------------------------------

.. autoclass:: Iter
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

*Timer*
-----------------------------------

.. autoclass:: Timer
   :members:

   .. autoclasstoc::

*Trig*
-----------------------------------

.. autoclass:: Trig
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

