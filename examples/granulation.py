from pyo import *
s = Server(buffersize=512).boot()

t = HarmTable([1,0,.3,0,.2])

snd = SndTable("/Users/olipet/Desktop/sons/baseballmajeur_s.aif")
env = HannTable()
pos = Phasor(snd.getRate()*.2, 0, snd.getSize())
pnz = Noise(5)
dur = Noise(.002, .1)
gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur, grains=100, basedur=.1, mul=.1).out()
de = Delay(gran, delay=.1, feedback=.5, maxdelay=1).out()

s.gui(locals())

