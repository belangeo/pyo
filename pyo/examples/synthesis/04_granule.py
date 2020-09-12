"""
Hand-written granulation module.

"""
from pyo import *

s = Server(duplex=0).boot()

t1 = SndTable("../snds/baseballmajeur_m.aif")
t2 = WinTable(7)
t3 = LinTable()

snd_dur = t1.getDur()
num_of_grains = 50
grain_dur = 0.25
jitter_amp = 0.2

met = Metro(time=grain_dur / num_of_grains, poly=num_of_grains).play()
mvt = Randi(freq=0.25)
pos = TrigRand(met, min=mvt, max=mvt + 0.05)
jit = TrigRand(met, min=1 - jitter_amp, max=1 + jitter_amp)
env = TrigEnv(met, t2, dur=grain_dur, mul=0.05)
ind = TrigEnv(met, t3, dur=grain_dur, mul=jit * (grain_dur / snd_dur), add=pos)
snd = Pointer(t1, ind, env).out()

s.gui(locals())
