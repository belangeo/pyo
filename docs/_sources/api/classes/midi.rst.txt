Midi Handling
===================================

.. currentmodule:: pyo

Objects to retrieve Midi informations for a specific Midi port and channel.

Objects creates and returns audio streams from the value in their Midi input.

The audio streams of these objects are essentially intended to be
used as controls and can't be sent to the output soundcard.

Objects in this category
------------------------------

- :py:class:`Bendin` :     Get the current value of the pitch bend controller.
- :py:class:`CtlScan` :     Scan the Midi controller's number in input.
- :py:class:`CtlScan2` :     Scan the Midi channel and controller number in input.
- :py:class:`MidiAdsr` :     Midi triggered ADSR envelope generator.
- :py:class:`MidiDelAdsr` :     Midi triggered ADSR envelope generator with pre-delay.
- :py:class:`Midictl` :     Get the current value of a Midi controller.
- :py:class:`Notein` :     Generates Midi note messages.
- :py:class:`Programin` :     Get the current value of a program change Midi controller.
- :py:class:`Touchin` :     Get the current value of an after-touch Midi controller.
- :py:class:`RawMidi` :     Raw Midi handler.
- :py:class:`MidiLinseg` :     Line segments trigger.

*Bendin*
-----------------------------------

.. autoclass:: Bendin
   :members:

   .. autoclasstoc::

*CtlScan*
-----------------------------------

.. autoclass:: CtlScan
   :members:

   .. autoclasstoc::

*CtlScan2*
-----------------------------------

.. autoclass:: CtlScan2
   :members:

   .. autoclasstoc::

*MidiAdsr*
-----------------------------------

.. autoclass:: MidiAdsr
   :members:

   .. autoclasstoc::

*MidiDelAdsr*
-----------------------------------

.. autoclass:: MidiDelAdsr
   :members:

   .. autoclasstoc::

*Midictl*
-----------------------------------

.. autoclass:: Midictl
   :members:

   .. autoclasstoc::

*Notein*
-----------------------------------

.. autoclass:: Notein
   :members:

   .. autoclasstoc::

*Programin*
-----------------------------------

.. autoclass:: Programin
   :members:

   .. autoclasstoc::

*Touchin*
-----------------------------------

.. autoclass:: Touchin
   :members:

   .. autoclasstoc::

*RawMidi*
-----------------------------------

.. autoclass:: RawMidi
   :members:

   .. autoclasstoc::

*MidiLinseg*
-----------------------------------

.. autoclass:: MidiLinseg
   :members:

   .. autoclasstoc::
