#!/usr/bin/env python
# encoding: utf-8
from pyo import *
import math

twopi = math.pi * 2

s = Server(duplex=0).boot()


class PAF:
    def __init__(self, f0, fc, bw, fs, amp):
        self.bell = WinTable(8)  # half-sine
        self.shape = WinTable(4)  # {4, 5 or 6}
        self.f0, self.fc, self.bw, self.fs, self.amp = (
            Sig(f0),
            Sig(fc),
            Sig(bw),
            Sig(fs),
            Sig(amp),
        )
        self.if0 = 1.0 / self.f0
        self.relcf = self.fc * self.if0
        self.relbw = self.bw * self.if0
        self.fshift = Phasor(self.fs, mul=twopi)
        self.phase = Phasor(self.f0)
        self.piphase = self.phase * twopi
        self.bw_phase = Pointer(self.bell, self.phase, mul=self.relbw)
        self.bw_value = Pointer(self.shape, Clip(self.bw_phase, 0, 1))
        self.frac = Wrap(self.relcf)
        self.int = self.relcf - self.frac
        self.a1_phase = self.piphase * self.int
        self.a2_phase = self.a1_phase + self.piphase
        self.a1 = Cos(self.a1_phase + self.fshift)
        self.a2 = Cos(self.a2_phase + self.fshift)
        self.olaps = self.a1 + (self.a2 - self.a1) * self.frac
        self.out = Sig(self.olaps * self.bw_value, mul=DBToA(self.amp, mul=self.bw_value + 1)).out()


for_tenor = {
    "ou": [290, 750, 2300, 3080],
    "o": [360, 770, 2530, 3200],
    "oo": [520, 900, 2510, 3310,],
    "aa": [710, 1230, 2700, 3700],
    "a": [750, 1450, 2590, 3280],
    "ee": [590, 1770, 2580, 3480],
    "oee": [570, 1560, 2560, 3450],
    "oe": [500, 1330, 2370, 3310],
    "eu": [350, 1350, 2250, 3170],
    "e": [420, 2050, 2630, 3340],
    "u": [250, 1750, 2160, 3060],
    "i": [250, 2250, 2980, 3280],
}

amp_tenor = {
    "ou": [-10, -21, -52, -64],
    "o": [-10, -21, -52, -64],
    "oo": [-10, -20, -53, -65],
    "aa": [-8, -27, -52, -60],
    "a": [-8, -25, -50, -59],
    "ee": [-10, -30, -47, -57],
    "oee": [-8, -27, -52, -60],
    "oe": [-10, -44, -49, -64],
    "eu": [-10, -46, -49, -64],
    "e": [-10, -30, -52, -60],
    "u": [-10, -44, -49, -64],
    "i": [-10, -40, -52, -57],
}

bw_tenor = {
    "ou": [30, 35, 45, 50],
    "o": [30, 35, 54, 59],
    "oo": [35, 40, 54, 59],
    "aa": [40, 45, 59, 64],
    "a": [40, 45, 59, 64],
    "ee": [40, 45, 59, 64],
    "oee": [40, 45, 59, 64],
    "oe": [45, 54, 64, 69],
    "eu": [40, 54, 59, 64],
    "e": [35, 54, 59, 64],
    "u": [35, 50, 50, 59],
    "i": [25, 50, 54, 59],
}

vib = Sine(Randi(4, 5, 0.1), mul=0.005, add=1)
fund = Sig(100, mul=vib)
fund.ctrl([SLMap(20, 400, "log", "value", 100)], "Fundamental frequency")
fshift = Sig(0, mul=vib)
fshift.ctrl([SLMap(-2000, 2000, "lin", "value", 0)], title="Frequency shift")

a = PAF([fund, fund * 1.005], 750, 40, fshift, -8)
b = PAF([fund, fund * 1.004], 1450, 45, fshift, -25)
c = PAF([fund, fund * 1.003], 2590, 60, fshift, -50)
d = PAF([fund, fund * 1.006], 3280, 65, fshift, -59)
e = PAF([fund, fund * 1.007], 4280, 75, fshift, -65)


def set(new):
    vo = for_tenor[new]
    a.fc.value = vo[0]
    b.fc.value = vo[1]
    c.fc.value = vo[2]
    d.fc.value = vo[3]
    e.fc.value = vo[3] + 1000
    vo = amp_tenor[new]
    a.amp.value = vo[0]
    b.amp.value = vo[1]
    c.amp.value = vo[2]
    d.amp.value = vo[3]
    e.amp.value = vo[3] - 6
    vo = bw_tenor[new]
    a.bw.value = vo[0]
    b.bw.value = vo[1]
    c.bw.value = vo[2]
    d.bw.value = vo[3]
    e.bw.value = vo[3] + 10


s.gui(locals())
