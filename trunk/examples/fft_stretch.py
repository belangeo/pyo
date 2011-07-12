#!/usr/bin/env python
# encoding: utf-8
"""
Time stretching using FFT/IFFT.
Created by belangeo on 2011-07-11.

"""
from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0).boot()

snd = 'baseballmajeur_m.aif'
info = sndinfo(snd)
chnls = info[3]
size = 1024
olaps = 4
hop = size/olaps
nframes = info[0] / size

a = SfPlayer(snd, mul=.5).out()

m_mag = [NewMatrix(width=size, height=nframes) for i in range(olaps*chnls)]
m_pha = [NewMatrix(width=size, height=nframes) for i in range(olaps*chnls)]

fin = FFT(a, size=size, overlaps=olaps)
pol = CarToPol(fin["real"], fin["imag"])
delta = FrameDelta(pol["ang"], framesize=size, overlaps=olaps)

m_mag_rec = MatrixRec(pol["mag"], m_mag, 0, [i*hop for i in range(olaps) for j in range(chnls)]).play()
m_pha_rec = MatrixRec(delta, m_pha, 0, [i*hop for i in range(olaps) for j in range(chnls)]).play()

pos = Sig(0)
pos.ctrl([SLMap(0, nframes, "lin", "value", 0, "float", 0.025)], title="Position in frames")

m_mag_read = MatrixPointer(m_mag, fin["bin"]/size, pos/nframes)
m_pha_read = MatrixPointer(m_pha, fin["bin"]/size, pos/nframes)

accum = FrameAccum(m_pha_read, framesize=size, overlaps=olaps)
car = PolToCar(m_mag_read, accum)
fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps).mix(chnls).out()

s.gui(locals())