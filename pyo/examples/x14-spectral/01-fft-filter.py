"""
01-fft-filter.py - Filter a sound in the spectral domain.

The FFT object analyses an input signal and converts it into the
spectral domain. Three audio signals are sent out of the object,
the real part, from bin 0 (DC) to bin size/2 (Nyquist frequency),
the imaginary part, from bin 0 to bin size/2-1, and the bin number,
an increasing count from 0 to size-1.

This script uses the increasing count to extract amplitude values
from a table, applies them to the real and imaginary parts of the
signal in its spectral representation, and converts it back to a
time domain signal with IFFT.

For a simpler and more efficient process, see the Phase Vocoder
implementation of the spectral filter: `PVFilter`.

"""

from pyo import *

s = Server(duplex=0).boot()

# A spectrally rich source to filter.
a = Noise(0.1).mix(2)

# FFT size in samples
size = 1024
# Number of overlaps
olaps = 4

# List of points to initialize the filter's table,
# from 0 to half the FFT size.
filter_init = [
    (0, 0.0000),
    (5, 0.9337),
    (13, 0.0000),
    (21, 0.4784),
    (32, 0.0000),
    (37, 0.1927),
    (size // 2, 0.0000),
]

# Converts the source signal into its spectral representation.
fin = FFT(a, size=size, overlaps=olaps, wintype=2)

# Creates the filter's table.
t = ExpTable(filter_init, size=size // 2)
t.graph(title="Filter shape")

# Reads the table cyclically, in sync with the FFT analysis.
# Table values will be used as the new amplitude of analysis bins.
amp = TableIndex(t, fin["bin"])

# Applies the amplitudes to the real/imaginary values of analysis bins.
re = fin["real"] * amp
im = fin["imag"] * amp

# Converts back the spectral representation to a time domain signal.
fout = IFFT(re, im, size=size, overlaps=olaps, wintype=2).mix(2).out()

s.gui(locals())
