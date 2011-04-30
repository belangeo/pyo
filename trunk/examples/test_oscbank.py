#!/usr/bin/env python
# encoding: utf-8
"""
Created by Olivier Belanger on 2011-04-22.
"""

from pyo import *
import random

s = Server(sr=44100, nchnls=2, buffersize=512, duplex=0)
#s.setOutputDevice(3)
s.boot()

X = 7

t = HarmTable([1,0,.2,0,.3,0,0,.2,0,0,0,.1,0,.2,0,0,.05,0,0,0,.03,0,0,.02])
#t = HarmTable([1,0,0,0,0,.1,0,0,0,0,0,0,0,.05])
t.normalize()

# Simple chorus
if X == 0:
    freqs = [random.uniform(150,170) for i in range(1000)]
    a = Osc(t, freqs, mul=.001).out()
elif X == 1:
    a = OscBank(t, freq=150, spread=.0001, slope=1, num=400, fjit=True).out()
# Moving frequencies
elif X == 2:
    freqs = [random.uniform(150,170) for i in range(400)]
    lf = Sine(.1, mul=50)
    a = Osc(t, lf+freqs, mul=.02).out()
elif X == 3:
    lf = Sine(.1, mul=50, add=150)
    a = OscBank(t, freq=lf, spread=.0001, slope=1, num=400, fjit=True).out()
# Randomized frequencies
elif X == 4:
    freqs = [random.uniform(150,170) for i in range(400)]
    rnds = Randi(.9, 1.1, freq=[1]*400)
    a = Osc(t, freqs*rnds, mul=.01).out()
elif X == 5:
    a = OscBank(t, freq=150, spread=0, slope=1, frndf=1, frnda=.1, num=400, fjit=False).out()
# Chorus
elif X == 6:
    tt = SndTable('/Users/olivier/Desktop/snds/flute.aif')
    rnds = Randi(.99, 1.01, freq=[3]*60)
    a = Osc(tt, tt.getRate()*rnds, mul=.04).out()
elif X == 7:
    tt = SndTable('/Users/olivier/Desktop/snds/flute.aif')
    a = OscBank(tt, freq=tt.getRate(), spread=0, slope=1, frndf=3, frnda=.01, num=[30,30]).out()
# Randomized amplitudes
elif X == 8:
    freqs = [random.uniform(150,4400) for i in range(8)]
    rnds = Randi(.0, 1., freq=[2]*8)
    a = Osc(t, freqs, mul=.05*rnds).out()
elif X == 9:
    a = OscBank(t, freq=250, spread=0.5, slope=1, frndf=2, frnda=.25, arndf=2, arnda=0., num=6, fjit=False).out()
# Randomized frequencies and amplitudes
elif X == 10:
    freqs = [random.uniform(150,170) for i in range(400)]
    rnds = Randi(.9, 1.1, freq=[1]*400)
    rnds2 = Randi(.0, 1., freq=[2]*400)
    a = Osc(t, freqs*rnds, mul=.05*rnds2).out()
elif X == 11:
    a = OscBank(t, freq=150, spread=0.15, slope=.9, frndf=3, frnda=.02, arndf=.3, arnda=1., num=20, fjit=False).out()
elif X == 12:
    lff = Sine(.1, mul=.02, add=.02)
    a = OscBank(t, freq=250, spread=0, slope=1, frndf=3, frnda=lff, arndf=.3, arnda=1., num=60, fjit=False).out()
elif X == 13:
    lff = Sine(.1, mul=.02, add=.02)
    lfa = Sine(.05, mul=.5, add=.5)
    a = OscBank(t, freq=100, spread=.5, slope=1, frndf=3, frnda=lff, arndf=1, arnda=lfa, num=8, fjit=False).out()
elif X == 14:
    ta = HarmTable([1,.3,.2])
    tb = HarmTable([1])
    f = Fader(fadein=.1, fadeout=.5, dur=4).play()
    a = OscBank(ta, freq=100, spread=0, frndf=.25, frnda=.01, num=[10,10], fjit=True, mul=f*0.5).out()
    b = OscBank(tb, freq=250, spread=.25, slope=.8, arndf=4, arnda=1, num=[10,10], fjit=False, mul=f*0.5).out()
    
    
    
    
s.gui(locals())    
    
    
    
    
    
    
    
    

