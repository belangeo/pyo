#! /usr/bin/env python
# encoding: utf-8
"""
Spectral morphing of two sounds.
created by belangeo, 31-05-2011

"""
from pyo import *

s = Server().boot()

snd1 = SfPlayer(SNDS_PATH+"/accord.aif", loop=True, mul=.7).mix(2)
lfos = Sine(freq=[.05,.04], mul=.05, add=[1,.5])
snd2 = FM(carrier=[75,100.03,125.5,149], ratio=lfos, index=20, mul=.25).mix(2)

size = 1024
olaps = 4

inter = Sig(1.)
inter.ctrl([SLMap(0, 2, "lin", "value", 1.)], title="Morphing (0 = snd1, 1 = morph, 2 = snd2)")

fin1 = FFT(snd1, size=size, overlaps=olaps)
fin2 = FFT(snd2, size=size, overlaps=olaps)

# get magnitudes and phases of input sounds
mag1 = Sqrt(fin1["real"]*fin1["real"] + fin1["imag"]*fin1["imag"], mul=5)
mag2 = Sqrt(fin2["real"]*fin2["real"] + fin2["imag"]*fin2["imag"], mul=5)
pha1 = Atan2(fin1["imag"], fin1["real"])
pha2 = Atan2(fin2["imag"], fin2["real"])

# times magnitudes and adds phases
mag3 = mag1 * mag2
pha3 = pha1 + pha2
# interpolation between dry and morphed sounds
mag = Selector([mag1*.03, mag3, mag2*.03], voice=inter)
pha = Selector([pha1, pha3, pha2], voice=inter)
# converts back to rectangular
real = mag * Cos(pha)
imag = mag * Sin(pha)

fout = IFFT(real, imag, size=size, overlaps=olaps)
ffout = fout.mix(2).out()

s.gui(locals())
