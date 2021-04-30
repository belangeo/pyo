# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_randoms.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server(audio="manual").boot().start()

a = Randi()
a.min = Sig(0.5)
a.max = Sig(0.7)
a.freq = Sig(0.5)

b = Randh()
b.min = Sig(0.5)
b.max = Sig(0.7)
b.freq = Sig(0.5)

l1 = [1.1,2.2,3.3,4.4,5.5]
l2 = [1.2,2.3,3.4,4.5,5.6]

c = Choice(l1)
c.choice = l2
c.freq = Sig(0.5)

d = RandInt()
d.max = Sig(10.5)
d.freq = Sig(0.5)

e = RandDur()
e.min = Sig(0.5)
e.max = Sig(0.7)

f = Xnoise()
f.freq = Sig(0.5)
f.x1 = Sig(0.1)
f.x2 = Sig(0.7)
f.dist = 10

g = XnoiseMidi()
g.freq = Sig(0.5)
g.x1 = Sig(0.1)
g.x2 = Sig(0.7)
g.scale = 2
g.setRange(48, 64)
g.dist = 10

h = XnoiseDur()
h.min = Sig(0.5)
h.max = Sig(0.7)
h.x1 = Sig(0.1)
h.x2 = Sig(0.7)
h.dist = 10

i = Urn()
i.max = (75)
i.freq = Sig(0.5)

j = LogiMap()
j.chaos = Sig(0.5)
j.freq = Sig(0.5)

s.process()
s.process()
s.stop()
s.shutdown()
