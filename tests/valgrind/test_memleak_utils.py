# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_utils.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

l1 = [48, 50, 52]
l2 = [54, 55, 57]

a = Input()

b1 = Print(a, message="method 0")
b2 = Print(a, 1, message="method 1")

c = Snap(i1, l1, scale=2)
c.input = i2
c.choice = l2

d = Interp(i1, i2)
d.input = i2
d.input = i1
d.interp = Sig(0.5)

e = SampHold(i1, Sig(0))
e.input = i2
e.controlsig = Sig(1)
e.value = Sig(0.5)

f = TrackHold(i1, Sig(0))
f.input = i2
f.controlsig = Sig(1)
f.value = Sig(0.5)

g = Compare(i1, 0)
g.input = i2
g.comp = Sig(0)
g.mode = "=="

h = Between(i1)
h.input = i2
h.min = Sig(-0.5)
h.max = Sig(0.5)

i = Denorm(i1)
i.input = i2

j = DBToA(i1)
j.input = i2

k = AToDB(i1)
k.input = i2

l = Scale(i1)
l.inmin = Sig(0.1)
l.inmax = Sig(1.1)
l.outmin = Sig(0.1)
l.outmax = Sig(1.1)
#l.exp = Sig(3)

m = CentsToTranspo(i1)
m.input = i2

n = TranspoToCents(i1)
n.input = i2

o = MToF(i1)
o.input = i2

p = FToM(i1)
p.input = i2

q = MToT(i1)
q.input = i2

s.beginResamplingBlock(4)
r1 = Resample(i1)
r1.input = i2
r1.setMode(2)
r1.setMode(4)
r1.setMode(64)
s.endResamplingBlock()
r2 = Resample(r1)

s.process()
s.process()
s.stop()
s.shutdown()
