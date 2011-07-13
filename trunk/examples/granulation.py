#!/usr/bin/env python
# encoding: utf-8

"""
Classic granulation stretching...

"""

from pyo import *
s = Server(buffersize=512).boot()

snd = SndTable(SNDS_PATH + '/transparent.aif')
snd.view()
env = HannTable()

pos = Phasor(snd.getRate()*.1, 0, snd.getSize())
pnz = Noise(5)

dur = Noise(.002, .1)

gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur, grains=40, basedur=.1, mul=.1).out()
         
s.gui(locals())

