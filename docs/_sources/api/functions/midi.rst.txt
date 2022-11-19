Midi Setup
======================================

Set of functions to inspect the system's midi configuration.

.. note::

    These functions are available only if pyo is built with portmidi support.

.. currentmodule:: pyo

Functions in this category
------------------------------

- :py:func:`pm_get_default_output` :     Returns the index number of Portmidi’s default output device.
- :py:func:`pm_get_default_input` :     Returns the index number of Portmidi’s default input device.
- :py:func:`pm_get_output_devices` :     Returns midi output devices (device names, device indexes) found by Portmidi.
- :py:func:`pm_get_input_devices` :     Returns midi input devices (device names, device indexes) found by Portmidi.
- :py:func:`pm_list_devices` :     Prints a list of all devices found by Portmidi.
- :py:func:`pm_count_devices` :     Returns the number of devices found by Portmidi.

*pm_get_default_output*
---------------------------------

.. autofunction:: pm_get_default_output

*pm_get_default_input*
---------------------------------

.. autofunction:: pm_get_default_input

*pm_get_output_devices*
---------------------------------

.. autofunction:: pm_get_output_devices

*pm_get_input_devices*
---------------------------------

.. autofunction:: pm_get_input_devices

*pm_list_devices*
---------------------------------

.. autofunction:: pm_list_devices

*pm_count_devices*
---------------------------------

.. autofunction:: pm_count_devices

