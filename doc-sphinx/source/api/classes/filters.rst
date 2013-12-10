Filters
============================

.. module:: pyo

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

*Biquadx*
------------

.. autoclass:: Biquadx
   :members:

*Biquada*
------------

.. autoclass:: Biquada
   :members:

*EQ*
------------

.. autoclass:: EQ
   :members:

*Tone*
------------

.. autoclass:: Tone
   :members:

*Atone*
------------

.. autoclass:: Atone
   :members:

*Port*
------------

.. autoclass:: Port
   :members:

*DCBlock*
------------

.. autoclass:: DCBlock
   :members:

*BandSplit*
------------

.. autoclass:: BandSplit
   :members:

*FourBand*
------------

.. autoclass:: FourBand
   :members:

*Hilbert*
------------

.. autoclass:: Hilbert
   :members:

*Allpass*
------------

.. autoclass:: Allpass
   :members:

*Allpass2*
------------

.. autoclass:: Allpass2
   :members:

*Phaser*
------------

.. autoclass:: Phaser
   :members:

*Vocoder*
------------

.. autoclass:: Vocoder
   :members:

*IRWinSinc*
------------

.. autoclass:: IRWinSinc
   :members:

*IRAverage*
------------

.. autoclass:: IRAverage
   :members:

*IRPulse*
------------

.. autoclass:: IRPulse
   :members:

*IRFM*
------------

.. autoclass:: IRFM
   :members:

*SVF*
------------

.. autoclass:: SVF
   :members:

*Average*
------------

.. autoclass:: Average
   :members:

*Reson*
------------

.. autoclass:: Reson
   :members:

*Resonx*
------------

.. autoclass:: Resonx
   :members:

*ButLP*
------------

.. autoclass:: ButLP
   :members:

*ButHP*
------------

.. autoclass:: ButHP
   :members:

*ButBP*
------------

.. autoclass:: ButBP
   :members:

*ButBR*
------------

.. autoclass:: ButBR
   :members:

*ComplexRes*
------------

.. autoclass:: ComplexRes
   :members:

