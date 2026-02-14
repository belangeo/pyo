"""
04-fft-spectral-delay.py - Applies different delays to frequency ranges of a sound.

The FFT object analyses an input signal and converts it into the
spectral domain. Three audio signals are sent out of the object,
the real part, from bin 0 (DC) to bin size/2 (Nyquist frequency),
the imaginary part, from bin 0 to bin size/2-1, and the bin number,
an increasing count from 0 to size-1.

This script splits an audio signal (converted to its spectral representation)
into 6 frequency ranges, applies different delay times to each band, and converts
it back to a time domain signal.

For a simpler and more efficient process, see the Phase Vocoder
implementation of the spectral delay: `PVDelay`.

"""

from pyo import *

s = Server(duplex=0).boot()

# The source sound
snd = "../snds/ounkmaster.aif"

# Number of audio channels in the source sound.
chnls = sndinfo(snd)[3]

# FFT size in samples
size = 1024
# Number of overlaps
olaps = 4

# Number of audio streams per FFT object.
num_streams = olaps * chnls

# The source to delay.
src = SfPlayer(snd, loop=True, mul=0.15)
# Delays the original sound to take account for the delay implied by the FFTs.
delsrc = Delay(src, delay=size / s.getSamplingRate() * 2).out()


# Utility function to duplicates bin regions and delays to match
# the number of audio streams per FFT object (overlaps * channels).
def duplicate(lst, how_many):
    return [x for x in lst for i in range(how_many)]


# Frequency ranges, in bin numbers (0 to Nyquist), for the 6 frequency bands.
binmin = duplicate([3, 10, 20, 27, 55, 100], num_streams)
binmax = duplicate([5, 15, 30, 40, 80, 145], num_streams)
# Delay times, in number of frames (FFT size), for the 6 frequency bands.
delays = duplicate([80, 20, 40, 100, 60, 120], num_streams)

# Delay conversion: number of frames -> seconds
for i in range(len(delays)):
    delays[i] = delays[i] * (size // 2) / s.getSamplingRate()

# Converts the source signal into its spectral representation.
fin = FFT(src * 1.25, size=size, overlaps=olaps)

# Splits regions between `binmin` and `binmax` with time variation.
# Between outputs 1 if the bin number is between its min and max arguments.
# With multi-channel expansion, this is done for each frequency range.

# Time variation applied on the max bin number
lfo = Sine(0.1, mul=0.65, add=1.35)
# Condition to allow the signal to pass (1) or not (0)
bins = Between(fin["bin"], min=binmin, max=binmax * lfo)
swre = fin["real"] * bins
swim = fin["imag"] * bins

# Apply delays on the frequency ranges (again, thanks to the multi-channel expansion),
# and mix the channels to match `num_streams` audio streams.
delre = Delay(swre, delay=delays, feedback=0.7, maxdelay=2).mix(num_streams)
delim = Delay(swim, delay=delays, feedback=0.7, maxdelay=2).mix(num_streams)

# Converts back the spectral representation to a time domain signal.
fout = IFFT(delre, delim, size=size, overlaps=olaps).mix(chnls).out()

s.gui(locals())
