#!/usr/bin/env python
# encoding: utf-8

from pyo import *
s = Server(buffersize=512).boot()

snd = SndTable(DEMOS_PATH + '/transparent.aif')
snd.view()
env = HannTable()

pos = Phasor(snd.getRate()*.1, 0, snd.getSize())
pnz = Noise(5)

trs = Choice([.5,.75,1,1.25], 1000)
dur = Noise(.002, .1)

gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur*trs, grains=100, basedur=.1, mul=.01).out()
         
s.gui(locals())

