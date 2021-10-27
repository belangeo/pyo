#!/usr/bin/env python
# encoding: utf-8
"""
Time stretching using FFT/IFFT.

"""
from pyo import *

s = Server(duplex=0).boot()

# Settings #
snd = "../snds/flute.aif"
stretch_factor = 10
size = 4096
olaps = 8
wintype = 7

info = sndinfo(snd)
chnls = info[3]
hop = size // olaps
nframes = info[0] // size

a = SfPlayer(snd, mul=0.1)

# "overlaps * chnls" Matrices of width FFTSIZE and height NUM_OF_FRAMES
# to record magnitude and phase analysis frames
m_mag = [NewMatrix(width=size, height=nframes) for i in range(olaps * chnls)]
m_pha = [NewMatrix(width=size, height=nframes) for i in range(olaps * chnls)]

fin = FFT(a, size=size, overlaps=olaps, wintype=wintype)
pol = CarToPol(fin["real"], fin["imag"])
delta = FrameDelta(pol["ang"], framesize=size, overlaps=olaps)

m_mag_rec = MatrixRec(pol["mag"], m_mag, 0, [i * hop for i in range(olaps) for j in range(chnls)]).play()
m_pha_rec = MatrixRec(delta, m_pha, 0, [i * hop for i in range(olaps) for j in range(chnls)]).play()

# Playback pointer
pos = Phasor(1.0 / info[1] / stretch_factor, mul=nframes)

m_mag_read = MatrixPointer(m_mag, fin["bin"] / size, pos / nframes)
m_pha_read = MatrixPointer(m_pha, fin["bin"] / size, pos / nframes)

# Smoothing magnitude and angle rate of changes
m_mag_smo = Vectral(m_mag_read, framesize=size, overlaps=olaps, up=0.5, down=0.5, damp=1)
m_pha_smo = Vectral(m_pha_read, framesize=size, overlaps=olaps, up=0.5, down=0.5, damp=1)

accum = FrameAccum(m_pha_smo, framesize=size, overlaps=olaps)
car = PolToCar(m_mag_smo, accum)
fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps, wintype=wintype).mix(chnls).mix(2).out()

s.gui(locals())
