"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from pyo import *
import random

s = Server().boot()

t1 = SndTable(DEMOS_PATH + '/transparent.aif')
t2 = HannTable()
t3 = LinTable()

num_of_grains = 50
grain_dur = .1
snd_dur = 1./t1.getRate()

met = Metro(time=grain_dur/num_of_grains, poly=num_of_grains)
mvt = Phasor(freq=.1)
pos = TrigRand(met, min=mvt, max=mvt+.05)
jit = TrigRand(met, min=.95, max=1.05)
env = TrigEnv(met, t2, dur=grain_dur, mul=.1)
ind = TrigEnv(met, t3, dur=grain_dur, mul=jit*(grain_dur/snd_dur), add=pos)
snd = Pointer(t1, ind, env).out()

def play():
    met.play()
    jit.max = random.uniform(1, 1.05)

def stop():
    met.stop()
    
s.gui(locals())    