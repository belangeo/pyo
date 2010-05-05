from pyo import *

s = Server().boot()

env = CosTable([(0,0),(100,1),(500,.3),(8191,0)])
wav = SquareTable(7)

met = Metro(.125, 8).play()
a = TrigXnoiseMidi(met, "loopseg", x1=1, x2=.2, mrange=(60,84))
b = Snap(a, [0,2,4,5,7,9,11], scale=1)

c = TrigEnv(met, env, mul=.2)
d = Osc(wav, b, mul=c).out()

s.gui(locals())


