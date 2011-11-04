"""
Algorithmic melody patterns snapped on a specific scale...

"""

from pyo import *

s = Server().boot()

env = CosTable([(0,0),(100,1),(500,.3),(8191,0)])
wav = SquareTable(7)

met = Metro(.125, 8).play()
a = TrigXnoiseMidi(met, "loopseg", x1=1, x2=.2, mrange=(48,97))
b = Snap(a, [0,4,7,11,13,17,21,25,29,33], scale=1)

c = TrigEnv(met, env, mul=.2)
d = Osc(wav, b, mul=c).out()

s.gui(locals())


