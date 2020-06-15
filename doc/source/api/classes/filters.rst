Filters
============================

.. currentmodule:: pyo

Different kinds of audio filtering operations.

An audio filter is designed to amplify, pass or attenuate (negative amplification) 
some frequency ranges. Common types include low-pass, which pass through 
frequencies below their cutoff frequencies, and progressively attenuates 
frequencies above the cutoff frequency. A high-pass filter does the opposite, 
passing high frequencies above the cutoff frequency, and progressively 
attenuating frequencies below the cutoff frequency. A bandpass filter passes 
frequencies between its two cutoff frequencies, while attenuating those outside 
the range. A band-reject filter, attenuates frequencies between its two cutoff 
frequencies, while passing those outside the 'reject' range.

An all-pass filter, passes all frequencies, but affects the phase of any given 
sinusoidal component according to its frequency.

*Biquad*
----------

.. autoclass:: Biquad
   :members:

   ..autoclasstoc::

*Biquadx*
------------

.. autoclass:: Biquadx
   :members:

   ..autoclasstoc::

*Biquada*
------------

.. autoclass:: Biquada
   :members:

   ..autoclasstoc::

*EQ*
------------

.. autoclass:: EQ
   :members:

   ..autoclasstoc::

*Tone*
------------

.. autoclass:: Tone
   :members:

   ..autoclasstoc::

*Atone*
------------

.. autoclass:: Atone
   :members:

   ..autoclasstoc::

*Port*
------------

.. autoclass:: Port
   :members:

   ..autoclasstoc::

*DCBlock*
------------

.. autoclass:: DCBlock
   :members:

   ..autoclasstoc::

*BandSplit*
------------

.. autoclass:: BandSplit
   :members:

   ..autoclasstoc::

*FourBand*
------------

.. autoclass:: FourBand
   :members:

   ..autoclasstoc::

*MultiBand*
------------

.. autoclass:: MultiBand
   :members:

   ..autoclasstoc::

*Hilbert*
------------

.. autoclass:: Hilbert
   :members:

   ..autoclasstoc::

*Allpass*
------------

.. autoclass:: Allpass
   :members:

   ..autoclasstoc::

*Allpass2*
------------

.. autoclass:: Allpass2
   :members:

   ..autoclasstoc::

*Phaser*
------------

.. autoclass:: Phaser
   :members:

   ..autoclasstoc::

*Vocoder*
------------

.. autoclass:: Vocoder
   :members:

   ..autoclasstoc::

*IRWinSinc*
------------

.. autoclass:: IRWinSinc
   :members:

   ..autoclasstoc::

*IRAverage*
------------

.. autoclass:: IRAverage
   :members:

   ..autoclasstoc::

*IRPulse*
------------

.. autoclass:: IRPulse
   :members:

   ..autoclasstoc::

*IRFM*
------------

.. autoclass:: IRFM
   :members:

   ..autoclasstoc::

*SVF*
------------

.. autoclass:: SVF
   :members:

   ..autoclasstoc::

*SVF2*
------------

.. autoclass:: SVF2
   :members:

   ..autoclasstoc::

*Average*
------------

.. autoclass:: Average
   :members:

   ..autoclasstoc::

*Reson*
------------

.. autoclass:: Reson
   :members:

   ..autoclasstoc::

*Resonx*
------------

.. autoclass:: Resonx
   :members:

   ..autoclasstoc::

*ButLP*
------------

.. autoclass:: ButLP
   :members:

   ..autoclasstoc::

*ButHP*
------------

.. autoclass:: ButHP
   :members:

   ..autoclasstoc::

*ButBP*
------------

.. autoclass:: ButBP
   :members:

   ..autoclasstoc::

*ButBR*
------------

.. autoclass:: ButBR
   :members:

   ..autoclasstoc::

*ComplexRes*
------------

.. autoclass:: ComplexRes
   :members:

   ..autoclasstoc::

*MoogLP*
------------

.. autoclass:: MoogLP
   :members:

   ..autoclasstoc::
