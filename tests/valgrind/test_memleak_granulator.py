# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_granulator.py 
# There should not be any definitely lost bytes.

from pyo import *
import random

s = Server().boot().start()
sr = int(s.getSamplingRate())

t1 = DataTable(sr, init=[random.uniform(-1,1) for i in range(sr)])
t2 = DataTable(sr, init=[random.uniform(-1,1) for i in range(sr)])

env1 = WinTable()
env2 = WinTable(3)

a = Granulator(t1, env1)
a.table = t2
a.env = env2
a.pitch = Sig(0.5)
a.pos = Sig(0.5)
a.dur = Sig(0.1)
a.grains = 4
a.grains = 12

b = Looper(t1)
b.table = t2
b.pitch = Sig(1.5)
b.start = Sig(0.5)
b.dur = Sig(0.5)
b.xfade = Sig(10.5)
b.xfadeshape = 2

c = Granule(t1, env1)
c.table = t2
c.env = env2
c.dens = Sig(50.5)
c.pitch = Sig(0.5)
c.pos = Sig(0.5)
c.dur = Sig(0.7)

d = Particle(t1, env1, chnls=2)
d.table = t2
d.env = env2
d.dens = Sig(50.5)
d.pitch = Sig(0.5)
d.pos = Sig(0.5)
d.dur = Sig(0.7)
d.dev = Sig(0.5)
d.pan = Sig(0.5)

e = Particle2(t1, env1, chnls=2)
e.table = t2
e.env = env2
e.dens = Sig(50.5)
e.pitch = Sig(0.5)
e.pos = Sig(0.5)
e.dur = Sig(0.7)
e.dev = Sig(0.5)
e.pan = Sig(0.5)
e.filterfreq = Sig(2000)
e.filterq = Sig(10.5)
e.filtertype = Sig(0.5)

s.process()
s.process()
s.stop()
s.shutdown()
