from pyo import *

s = Server().boot()
s.start()

t1 = SndTable('demos/transparent.aif')
t2 = HannTable()
t3 = LinTable()

num_of_grains = 50
grain_dur = .1
snd_dur = 1./t1.getRate()

met = Metro(time=grain_dur/num_of_grains, poly=num_of_grains).play()
mvt = Phasor(freq=.1)
pos = TrigRand(met, min=mvt, max=mvt+.05)
jit = TrigRand(met, min=.95, max=1.05)
env = TrigEnv(met, t2, dur=grain_dur, mul=.1)
ind = TrigEnv(met, t3, dur=grain_dur, mul=jit*(grain_dur/snd_dur), add=pos)
snd = Pointer(t1, ind, env).out()
