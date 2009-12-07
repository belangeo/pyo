:mod:`pyo` --- module functions
===============================

.. function:: pyo.pa_count_devices()

    Return how many audio devices are available.

.. function:: pyo.pa_list_devices()

    List all available audio devices.

.. function:: pyo.pa_get_default_input()

	Return Portaudio default input.
	
.. function:: pyo.pa_get_default_output()

	Return Portaudio default output.
	
.. function:: pyo.pm_count_devices()

	Return how many MIDI devices are available.
	
.. function:: pyo.pm_list_devices()

	List all available MIDI devices.

.. function:: pyo.sndinfo(path)

    :param path: file's path

    Return information about a soundfile in the form of a tuple (# frames, sampling rate, # channels).
