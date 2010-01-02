:class:`Server` --- Handling input - output communications
==========================================================

.. class:: Server(sr=44100, nchnls=1, buffersize=256, duplex=0)

    The :class:`Server` object handles all communications with Portaudio and Portmidi.
    
    An instance of the :class:`Server` must be booted before defining any signal processing chain.

    :param sr: int, optional
    
    Sampling rate used by Portaudio and the :class:`Server` to compute samples. Defaults to 44100.
    
    :param nchnls: int, optional
    
    Number of input and output channels. Defaults to 2.
    
    :param buffersize: int, optional
    
    Number of samples that Portaudio will request from the callback loop. This value has an impact
    on CPU use (a small buffer size is harder to compute) and on the latency of the system. Latency
    is `buffer size / sampling rate` in seconds. Defaults to 256.
    
    :param duplex: int {0, 1}, optional
    
    Input - output mode. 0 is output only and 1 is both ways. Defaults to 0.

.. method:: Server.boot()

    Boot the server. Must be called before defining any signal processing chain. The :class:`Server`'s parameters
    like `samplingrate`, `buffersize` or `nchnls` will be effective after a call to this method.
          
.. method:: Server.shutdown()
    
    Shut down and clear the server. This method will erase all objects from the callback loop.
    This method needs to be called before changing the :class:`Server`'s parameters like `samplingrate`, `buffersize`, `nchnls`...
    
.. method:: Server.start()

    Start the audio callback loop and begin processing.
    
.. method:: Server.stop()

    Stop the audio callback loop.

.. method:: Server.setAmp(x)

    Set the overall amplitude.

    :param x: float, amplitude.
     
.. method:: Server.recstart()

    Begin a default recording of the sound that is sent to output. This will create a file
    called `pyo_rec.aif` in the user's home directory.
    
.. method:: Server.recstop()

    Stop the previously started recording.
    
.. method:: Server.getStreams()

    Return the list of streams loaded in the server.
    
.. method:: Server.getSamplingRate()

    Return the current sampling rate.
    
.. method:: Server.getNchnls()

    Return the current number of channels.

.. method:: Server.getBufferSize()

    Return the current buffer size.
  
.. note::

    The next methods must be called before booting the server.
    
.. method:: Server.setInputDevice(x)

    Set the audio input device number. See :func:`pyo.pa_list_devices`.

    :param x: int, Number of the audio device listed by Portaudio.
    
.. method:: Server.setOutputDevice(x)

    Set the audio output device number. See :func:`pyo.pa_list_devices`.

    :param x: int, Number of the audio device listed by Portaudio.
    
.. method:: Server.setMidiInputDevice(x)

    Set the MIDI input device number. See :func:`pyo.pm_list_devices`.

    :param x: int, Number of the MIDI device listed by Portmidi.
    
.. method:: Server.setSamplingRate(x)

    Set the sampling rate used by the server.

    :param x: int, sampling rate.
    
.. method:: Server.setBufferSize(x)

    Set the buffer size used by the server.

    :param x: int, buffer size.
    
.. method:: Server.setNchnls(x)

    Set the number of channels used by the server.

    :param x: int, number of channels.
    
.. method:: Server.setDuplex(x)

    Set the duplex mode used by the server. 0 is output only, 1 is both ways.

    :param x: int {0, 1}, duplex mode.

.. attribute:: Server.amp

    float. Amplitude.
    
