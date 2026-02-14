"""
03-fft-morphing.py - Performs the spectral morphing of two sounds.

The FFT object analyses an input signal and converts it into the
spectral domain. Three audio signals are sent out of the object,
the real part, from bin 0 (DC) to bin size/2 (Nyquist frequency),
the imaginary part, from bin 0 to bin size/2-1, and the bin number,
an increasing count from 0 to size-1.

This script performs the spectral morphing of two sounds, by multiplying the
magnitudes, and adding the phases of both sounds.

For a simpler and more efficient process, see the Phase Vocoder
implementation of the spectral morphing: `PVMorph`.

"""

from pyo import *

s = Server(duplex=0).boot()

# First source
snd1 = SfPlayer("../snds/baseballmajeur_m.aif", loop=True, mul=0.3).mix(2)
# Second source
snd2 = FM(carrier=[75, 100, 125, 150], ratio=[0.999, 0.5005], index=20, mul=0.2).mix(2)

# FFT size in samples
size = 1024
# Number of overlaps
olaps = 4

# Converts the source signals into their spectral representation.
fin1 = FFT(snd1, size=size, overlaps=olaps)
fin2 = FFT(snd2, size=size, overlaps=olaps)

# Gets magnitudes and phases of input sounds.
pol1 = CarToPol(fin1["real"], fin1["imag"])
pol2 = CarToPol(fin2["real"], fin2["imag"])

# Times magnitudes and adds phases
mag = pol1["mag"] * pol2["mag"] * 200
pha = pol1["ang"] + pol2["ang"]

# Converts back polar coordinates to cartesian coordinates.
car = PolToCar(mag, pha)

# Converts the new real and imag parts into a time domain signal.
fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps).mix(2).out()


# Change of FFT size must be done on all FFT and IFFT objects at the same time!
def setSize(x):
    fin1.size = size
    fin2.size = size
    fout.size = size


s.gui(locals())
