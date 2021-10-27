#! /usr/bin/env python
# encoding: utf-8
"""
Apply spectral delays on a sound.

"""
from pyo import *

s = Server(duplex=0).boot()

snd = "../snds/ounkmaster.aif"
chnls = sndinfo(snd)[3]
size = 1024
olaps = 4
num = olaps * chnls  # number of streams for ffts

src = SfPlayer(snd, loop=True, mul=0.15)
delsrc = Delay(src, delay=size / s.getSamplingRate() * 2).out()

# duplicates bin regions and delays to match the number of channels * overlaps
def duplicate(li, num):
    tmp = [x for x in li for i in range(num)]
    return tmp


binmin = duplicate([3, 10, 20, 27, 55, 100], num)
binmax = duplicate([5, 15, 30, 40, 80, 145], num)
delays = duplicate([80, 20, 40, 100, 60, 120], num)
# delays conversion : number of frames -> seconds
for i in range(len(delays)):
    delays[i] = delays[i] * (size // 2) / s.getSamplingRate()

fin = FFT(src * 1.25, size=size, overlaps=olaps)

# splits regions between `binmins` and `binmaxs` with time variation
lfo = Sine(0.1, mul=0.65, add=1.35)
bins = Between(fin["bin"], min=binmin, max=binmax * lfo)
swre = fin["real"] * bins
swim = fin["imag"] * bins
# apply delays with mix to match `num` audio streams
delre = Delay(swre, delay=delays, feedback=0.7, maxdelay=2).mix(num)
delim = Delay(swim, delay=delays, feedback=0.7, maxdelay=2).mix(num)

fout = IFFT(delre, delim, size=size, overlaps=olaps).mix(chnls).out()

s.gui(locals())
