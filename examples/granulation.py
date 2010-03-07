from pyo import *
s = Server(buffersize=512).boot()

t = HarmTable([1,0,.3,0,.2])

snd = SndTable(DEMOS_PATH + '/transparent.aif')
env = HannTable()

pos = Phasor(snd.getRate()*.1, 0, snd.getSize())
pnz = Noise(5)

trs = Choice([.5,.75,1,1.25], 1000)
dur = Noise(.002, .1)

gran = Granulator(table=snd, env=env, pitch=[1.999, 2.0011], pos=pos+pnz, 
                  dur=dur*trs, grains=100, basedur=.1, mul=.04).out()
         
s.gui(locals())

