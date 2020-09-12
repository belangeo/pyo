#! /usr/bin/env python
# encoding: utf-8
"""
Spectral morphing of two sounds.

"""
from pyo import *

s = Server(duplex=0).boot()

snd1 = SfPlayer("../snds/baseballmajeur_m.aif", loop=True, mul=0.3).mix(2)
lfos = Sine(freq=[0.05, 0.04], mul=0.05, add=[1, 0.5])
snd2 = FM(carrier=[75, 100.03, 125.5, 149], ratio=lfos, index=20, mul=0.1).mix(2)

size = 1024
olaps = 4

inter = Sig(1.0)
inter.ctrl([SLMap(0, 2, "lin", "value", 1.0)], title="Morphing (0 = snd1, 1 = morph, 2 = snd2)")

fin1 = FFT(snd1, size=size, overlaps=olaps)
fin2 = FFT(snd2, size=size, overlaps=olaps)

# get magnitudes and phases of input sounds
pol1 = CarToPol(fin1["real"], fin1["imag"])
pol2 = CarToPol(fin2["real"], fin2["imag"])

# times magnitudes and adds phases
mag3 = pol1["mag"] * pol2["mag"] * 200
pha3 = pol1["ang"] + pol2["ang"]

# interpolation between dry and morphed sounds
mag = Selector([pol1["mag"], mag3, pol2["mag"]], voice=inter)
pha = Selector([pol1["ang"], pha3, pol2["ang"]], voice=inter)

# converts back to rectangular
car = PolToCar(mag, pha)

fout = IFFT(car["real"], car["imag"], size=size, overlaps=olaps)
ffout = fout.mix(2).out()


def siz(size=1024):
    fin1.size = size
    fin2.size = size
    fout.size = size


s.gui(locals())
