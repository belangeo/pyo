"""
02-fft-cross-synth.py - Performs the cross-synthesis of two sounds.

The FFT object analyses an input signal and converts it into the
spectral domain. Three audio signals are sent out of the object,
the real part, from bin 0 (DC) to bin size/2 (Nyquist frequency),
the imaginary part, from bin 0 to bin size/2-1, and the bin number,
an increasing count from 0 to size-1.

This script performs the cross-synthesis of two sounds, by applying the
amplitude of each bin of a sound to the frequency of each bin of a second sound.

For a simpler and more efficient process, see the Phase Vocoder
implementations of the spectral cross synthesis: `PVCross` and `PVMult`.

"""

from pyo import *

s = Server(duplex=0).boot()

# First source, providing the amplitude envelope (usually with lot of dynamics and variations).
snd1 = SfPlayer("../snds/baseballmajeur_m.aif", loop=True).mix(2)
# Second source, the spectral content (usually a rich frequency spectrum with few dynamic variations).
snd2 = FM(carrier=[75, 100, 125, 150], ratio=[0.999, 0.5005], index=20, mul=0.4).mix(2)

# FFT size in samples
size = 1024
# Number of overlaps
olaps = 4

# Converts the source signals into their spectral representation.
fin1 = FFT(snd1, size=size, overlaps=olaps)
fin2 = FFT(snd2, size=size, overlaps=olaps)

# Gets the bin magnitude of the first sound.
mag = Sqrt(fin1["real"] * fin1["real"] + fin1["imag"] * fin1["imag"], mul=10)
# Scales `real` and `imag` parts of the second sound by the magnitude of the first one.
real = fin2["real"] * mag
imag = fin2["imag"] * mag

# Converts the new real and imag parts into a time domain signal.
fout = IFFT(real, imag, size=size, overlaps=olaps).mix(2).out()


# Change of FFT size must be done on all FFT and IFFT objects at the same time!
def setSize(x):
    fin1.size = x
    fin2.size = x
    fout.size = x


s.gui(locals())
