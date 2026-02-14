"""
05-fft-time-stretch.py - Time stretching using FFT/IFFT.

The FFT object analyses an input signal and converts it into the
spectral domain. Three audio signals are sent out of the object,
the real part, from bin 0 (DC) to bin size/2 (Nyquist frequency),
the imaginary part, from bin 0 to bin size/2-1, and the bin number,
an increasing count from 0 to size-1.

This script analyses an audio signal in the spectral domain, save the
magnitudes and the phase deltas of each analysis frame in matrices, and
then reads these matrices at a slower speed to do time stretching of the
sound in the spectral domain.

For a simpler and more efficient process, see the Phase Vocoder implementations
of the spectral stretching: `PVBuffer`, `PVBufLoops`, and `PVBufTabLoops`.

"""

from pyo import *

s = Server(duplex=0).boot()

# The source sound
snd = "../snds/flute.aif"

# The stretching factor (x times slower than normal speed).
stretch_factor = 10

# FFT size in samples
size = 4096
# Number of overlaps
olaps = 8
# The hopsize, i.e. the time, in samples, between two overlapping frames.
hop = size // olaps
# Envelope type for windowing (see wintype argument of FFT/IFFT).
wintype = 7

# Retrieve information about the soundfile
info = sndinfo(snd)
# The number of channels
chnls = info[3]
# How many analysis frames to cover the entire sound duration.
nframes = info[0] // size

# Reads the soundfile once.
a = SfPlayer(snd, mul=0.1)

# Creates "overlaps * chnls" number of matrices of width "FFT size", and
# of height "number of frames" to record magnitude and phase analysis frames.
m_mag = [NewMatrix(width=size, height=nframes) for i in range(olaps * chnls)]
m_pha = [NewMatrix(width=size, height=nframes) for i in range(olaps * chnls)]

# Converts the source signal into its spectral representation.
fin = FFT(a, size=size, overlaps=olaps, wintype=wintype)

# Cartesian to polar conversion.
pol = CarToPol(fin["real"], fin["imag"])
# The delta of phases between successive analysis frame.
delta = FrameDelta(pol["ang"], framesize=size, overlaps=olaps)

# Record the magnitude frames.
m_mag_rec = MatrixRec(
    pol["mag"], m_mag, 0, [i * hop for i in range(olaps) for j in range(chnls)]
).play()
# Record the phase delta frames.
m_pha_rec = MatrixRec(delta, m_pha, 0, [i * hop for i in range(olaps) for j in range(chnls)]).play()

# The playback pointer.
pos = Phasor(1.0 / info[1] / stretch_factor, mul=nframes)

# Reads magnitude and phase delta matrices.
m_mag_read = MatrixPointer(m_mag, fin["bin"] / size, pos / nframes)
m_pha_read = MatrixPointer(m_pha, fin["bin"] / size, pos / nframes)

# Smoothing magnitude and phase delta rate of changes/
m_mag_smo = Vectral(m_mag_read, framesize=size, overlaps=olaps, up=0.5, down=0.5, damp=1)
m_pha_smo = Vectral(m_pha_read, framesize=size, overlaps=olaps, up=0.5, down=0.5, damp=1)

# Phase delta accumulator (inverse of FrameDelta).
accum = FrameAccum(m_pha_smo, framesize=size, overlaps=olaps)

# Polar to cartesian conversion.
car = PolToCar(m_mag_smo, accum)

# Converts the new real and imag parts into a time domain signal.
fout = (
    IFFT(car["real"], car["imag"], size=size, overlaps=olaps, wintype=wintype)
    .mix(chnls)
    .mix(2)
    .out()
)

s.gui(locals())
