#!/usr/bin/env python
# encoding: utf-8
"""
Split-Sideband Synthesis. Variation of three well known distortion techniques,
Waveshaping, Singlesideband modulation and Frequency Modulation.

Based on the article :
"SPLIT-SIDEBAND SYNTHESIS" from Victor Lazzarini, Joseph Timoney, Thomas Lysaght
Ann Arbor, MPublishing, University of Michigan Library, August 2008

"""
from pyo import *
import math

s = Server(duplex=0).boot()

twopi = 2 * math.pi
oneOverTwoPi = 1.0 / twopi
sinus = HarmTable()

mod = RandInt(max=8, freq=0.25, mul=0.05, add=29.8)

index = Sig([12.649, 9], mul=oneOverTwoPi)
index.ctrl(map_list=[SLMap(0, 20, "lin", "value", [12.649, 9])], title="Indexes")
car = Sig([150, 484.324])
car.ctrl(
    map_list=[SLMap(20, 2000, "log", "value", [150, 484.324])], title="Carrier frequencies",
)

a1 = Sine(freq=mod, mul=index[0])
a2 = Pointer(table=sinus, index=a1 + 0.25)
a3 = Pointer(table=sinus, index=a1)
even = Hilbert(a2)
odd = Hilbert(a3)
ac = Sine(freq=car[0], mul=0.5)
ad = Sine(freq=car[0], phase=0.25, mul=0.5)
evenReal = even["real"] * ac
oddReal = odd["real"] * ac
evenImag = even["imag"] * ad
oddImag = odd["imag"] * ad
evenUpper = evenReal + evenImag
oddUpper = oddReal + oddImag
evenLower = evenReal - evenImag
oddLower = oddReal - oddImag

sa1 = Sine(freq=oddLower * car[0], mul=index[1])
sa2 = Pointer(table=sinus, index=sa1 + 0.25)
sa3 = Pointer(table=sinus, index=sa1)
seven = Hilbert(sa2)
sodd = Hilbert(sa3)
sac = Sine(freq=car[1], mul=0.5)
sad = Sine(freq=car[1], phase=0.25, mul=0.5)
sevenReal = seven["real"] * sac
soddReal = sodd["real"] * sac
sevenImag = seven["imag"] * sad
soddImag = sodd["imag"] * sad
sevenUpper = sevenReal + sevenImag
soddUpper = soddReal + soddImag
sevenLower = sevenReal - sevenImag
soddLower = soddReal - soddImag

mix = Mix([sevenLower, sevenUpper], voices=2, mul=0.2).out()

s.gui(locals())
