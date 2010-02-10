from pyo import *
s = Server(buffersize=512).boot()

t = HarmTable([1,0,.3,0,.2])

snd = SndTable(DEMOS_PATH + '/accord.aif')
env = HannTable()

pos = Phasor(snd.getRate()*.02, 0, snd.getSize())
pnz = Noise(5)

met = Metro(.0001).play()
trs = TrigChoice(met, [.5,.75,1,1.25])
dur = Noise(.002, .1)

gran = Granulator(table=snd, env=env, pitch=[.999, 1.0011], pos=pos+pnz, 
                  dur=dur*trs, grains=100, basedur=.1, mul=.01).out()

s.gui(locals())

