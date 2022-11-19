Audio Setup
======================================

Set of functions to inspect the system's audio configuration.

.. note::

    These functions are available only if pyo is built with portaudio support.
 
.. currentmodule:: pyo

Functions in this category
------------------------------

- :py:func:`pa_get_version` :     Returns the version number, as an integer, of the current portaudio installation.
- :py:func:`pa_get_version_text` :     Returns the textual description of the current portaudio installation.
- :py:func:`pa_count_host_apis` :     Returns the number of host apis found by Portaudio.
- :py:func:`pa_list_host_apis` :     Prints a list of all host apis found by Portaudio.
- :py:func:`pa_get_default_host_api` :     Returns the index number of Portaudio’s default host api.
- :py:func:`pa_get_default_devices_from_host` :     Returns the default input and output devices for a given audio host.
- :py:func:`pa_count_devices` :     Returns the number of devices found by Portaudio.
- :py:func:`pa_list_devices` :     Prints a list of all devices found by Portaudio.
- :py:func:`pa_get_devices_infos` :     Returns informations about all devices found by Portaudio.
- :py:func:`pa_get_input_devices` :     Returns input devices (device names, device indexes) found by Portaudio.
- :py:func:`pa_get_output_devices` :     Returns output devices (device names, device indexes) found by Portaudio.
- :py:func:`pa_get_default_input` :     Returns the index number of Portaudio’s default input device.
- :py:func:`pa_get_default_output` :     Returns the index number of Portaudio’s default output device.
- :py:func:`pa_get_input_max_channels` :     Retrieve the maximum number of input channels for the specified device.
- :py:func:`pa_get_output_max_channels` :     Retrieve the maximum number of output channels for the specified device.

*pa_get_version*
---------------------------------

.. autofunction:: pa_get_version

*pa_get_version_text*
---------------------------------

.. autofunction:: pa_get_version_text

*pa_count_host_apis*
---------------------------------

.. autofunction:: pa_count_host_apis

*pa_list_host_apis*
---------------------------------

.. autofunction:: pa_list_host_apis

*pa_get_default_host_api*
---------------------------------

.. autofunction:: pa_get_default_host_api

*pa_get_default_devices_from_host*
-------------------------------------

.. autofunction:: pa_get_default_devices_from_host

*pa_count_devices*
---------------------------------

.. autofunction:: pa_count_devices

*pa_list_devices*
---------------------------------

.. autofunction:: pa_list_devices

*pa_get_devices_infos*
---------------------------------

.. autofunction:: pa_get_devices_infos

*pa_get_input_devices*
---------------------------------

.. autofunction:: pa_get_input_devices

*pa_get_output_devices*
---------------------------------

.. autofunction:: pa_get_output_devices

*pa_get_default_input*
---------------------------------

.. autofunction:: pa_get_default_input

*pa_get_default_output*
---------------------------------

.. autofunction:: pa_get_default_output

*pa_get_input_max_channels*
---------------------------------

.. autofunction:: pa_get_input_max_channels(x)

*pa_get_output_max_channels*
---------------------------------

.. autofunction:: pa_get_output_max_channels(x)

