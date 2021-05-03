# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_dsl.py 
# There should not be any definitely lost bytes.

###
### Expr and MML should be done in master (need more testing) and merge back in stripped branch afterward.
###

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

#a = Expr(Sig(0), "sin (* twopi (~ (sah (* (randi 5 10) 100) (~ 4 0)) 0))").out()

song1 = "#0 t600 o12 v80 x0.5 y0.5 z0.5 c3 r5 ?[c e g b-]1 |: v?{40 70} x+0.01 y+0.01 z+0.01 v+20 o+2 t-2 d-2 r3 d+6 (c d e)5 v-20 x-0.01 y-0.01 z-0.01 :|12 z?{0.1 0.2}"

env1 = CosTable([(0,0), (8,1), (1024,1), (8000, 1.0), (8191,0)])


mml1 = MML(song1, voices=1, loop=True, poly=1, updateAtEnd=False).play()
tr1 = TrigEnv(mml1, table=env1, dur=mml1['dur'], mul=mml1["amp"])
osc1 = SineLoop(freq=mml1["freq"], feedback=mml1['x'], mul=tr1)

s.process()
s.process()
s.process()
s.process()
s.process()
s.process()
s.process()
s.process()
s.stop()
s.shutdown()
