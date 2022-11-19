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

Objects in this category
------------------------------

- :py:class:`Biquad` :     A sweepable general purpose biquadratic digital filter.
- :py:class:`Biquadx` :     A multi-stages sweepable general purpose biquadratic digital filter.
- :py:class:`Biquada` :     A general purpose biquadratic digital filter (floating-point arguments).
- :py:class:`EQ` :     Equalizer filter.
- :py:class:`Tone` :     A first-order recursive low-pass filter with variable frequency response.
- :py:class:`Atone` :     A first-order recursive high-pass filter with variable frequency response.
- :py:class:`Port` :     Exponential portamento.
- :py:class:`DCBlock` :     Implements the DC blocking filter.
- :py:class:`BandSplit` :     Splits an input signal into multiple frequency bands.
- :py:class:`FourBand` :     Splits an input signal into four frequency bands.
- :py:class:`MultiBand` :     Splits an input signal into multiple frequency bands.
- :py:class:`Hilbert` :     Hilbert transform.
- :py:class:`Allpass` :     Delay line based allpass filter.
- :py:class:`Allpass2` :     Second-order phase shifter allpass.
- :py:class:`Phaser` :     Multi-stages second-order phase shifter allpass filters.
- :py:class:`Vocoder` :     Applies the spectral envelope of a first sound to the spectrum of a second sound.
- :py:class:`IRWinSinc` :     Windowed-sinc filter using circular convolution.
- :py:class:`IRAverage` :     Moving average filter using circular convolution.
- :py:class:`IRPulse` :     Comb-like filter using circular convolution.
- :py:class:`IRFM` :     Filters a signal with a frequency modulation spectrum using circular convolution.
- :py:class:`SVF` :     Fourth-order state variable filter allowing continuous change of the filter type.
- :py:class:`SVF2` :     Second-order state variable filter allowing continuous change of the filter type.
- :py:class:`Average` :     Moving average filter.
- :py:class:`Reson` :     A second-order resonant bandpass filter.
- :py:class:`Resonx` :     A multi-stages second-order resonant bandpass filter.
- :py:class:`ButLP` :     A second-order Butterworth lowpass filter.
- :py:class:`ButHP` :     A second-order Butterworth highpass filter.
- :py:class:`ButBP` :     A second-order Butterworth bandpass filter.
- :py:class:`ButBR` :     A second-order Butterworth band-reject filter.
- :py:class:`ComplexRes` :     Complex one-pole resonator filter.
- :py:class:`MoogLP` :     A fourth-order resonant lowpass filter.

*Biquad*
----------

.. autoclass:: Biquad
   :members:

   .. autoclasstoc::

*Biquadx*
------------

.. autoclass:: Biquadx
   :members:

   .. autoclasstoc::

*Biquada*
------------

.. autoclass:: Biquada
   :members:

   .. autoclasstoc::

*EQ*
------------

.. autoclass:: EQ
   :members:

   .. autoclasstoc::

*Tone*
------------

.. autoclass:: Tone
   :members:

   .. autoclasstoc::

*Atone*
------------

.. autoclass:: Atone
   :members:

   .. autoclasstoc::

*Port*
------------

.. autoclass:: Port
   :members:

   .. autoclasstoc::

*DCBlock*
------------

.. autoclass:: DCBlock
   :members:

   .. autoclasstoc::

*BandSplit*
------------

.. autoclass:: BandSplit
   :members:

   .. autoclasstoc::

*FourBand*
------------

.. autoclass:: FourBand
   :members:

   .. autoclasstoc::

*MultiBand*
------------

.. autoclass:: MultiBand
   :members:

   .. autoclasstoc::

*Hilbert*
------------

.. autoclass:: Hilbert
   :members:

   .. autoclasstoc::

*Allpass*
------------

.. autoclass:: Allpass
   :members:

   .. autoclasstoc::

*Allpass2*
------------

.. autoclass:: Allpass2
   :members:

   .. autoclasstoc::

*Phaser*
------------

.. autoclass:: Phaser
   :members:

   .. autoclasstoc::

*Vocoder*
------------

.. autoclass:: Vocoder
   :members:

   .. autoclasstoc::

*IRWinSinc*
------------

.. autoclass:: IRWinSinc
   :members:

   .. autoclasstoc::

*IRAverage*
------------

.. autoclass:: IRAverage
   :members:

   .. autoclasstoc::

*IRPulse*
------------

.. autoclass:: IRPulse
   :members:

   .. autoclasstoc::

*IRFM*
------------

.. autoclass:: IRFM
   :members:

   .. autoclasstoc::

*SVF*
------------

.. autoclass:: SVF
   :members:

   .. autoclasstoc::

*SVF2*
------------

.. autoclass:: SVF2
   :members:

   .. autoclasstoc::

*Average*
------------

.. autoclass:: Average
   :members:

   .. autoclasstoc::

*Reson*
------------

.. autoclass:: Reson
   :members:

   .. autoclasstoc::

*Resonx*
------------

.. autoclass:: Resonx
   :members:

   .. autoclasstoc::

*ButLP*
------------

.. autoclass:: ButLP
   :members:

   .. autoclasstoc::

*ButHP*
------------

.. autoclass:: ButHP
   :members:

   .. autoclasstoc::

*ButBP*
------------

.. autoclass:: ButBP
   :members:

   .. autoclasstoc::

*ButBR*
------------

.. autoclass:: ButBR
   :members:

   .. autoclasstoc::

*ComplexRes*
------------

.. autoclass:: ComplexRes
   :members:

   .. autoclasstoc::

*MoogLP*
------------

.. autoclass:: MoogLP
   :members:

   .. autoclasstoc::
