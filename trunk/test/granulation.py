from pyo import *
s = Server().boot()

snd = SndTable("demos/transparent.aif")
env = HannTable()
pos = Phasor(snd.getRate()*.5, 0, snd.getSize())
pnz = Noise(5)
dur = Noise(.001, .1)
gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur, grains=100, basedur=.1, mul=.1).out()

s.gui()
