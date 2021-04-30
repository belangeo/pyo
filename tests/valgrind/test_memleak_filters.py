# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_fft.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = Biquad(i1)
a.input = i2
a.freq = Sig(1000)
a.q = Sig(10)
a.type = 2

b = Biquadx(i1)
b.input = i2
b.freq = Sig(1000)
b.q = Sig(10)
b.type = 2
b.stages = 2
b.stages = 6

c = Biquada(i1)
c.input = i2
c.a0 = Sig(0)
c.a1 = Sig(0)
c.a2 = Sig(0)
c.b0 = Sig(0)
c.b1 = Sig(0)
c.b2 = Sig(0)

d = EQ(i1)
d.input = i2
d.freq = Sig(1000)
d.q = Sig(10)
d.boost = Sig(-9)
d.type = 1

e = Port(i1)
e.input = i2
e.risetime = Sig(0.01)
e.falltime = Sig(0.1)

f = Tone(i1)
f.input = i2
f.freq = Sig(1000)

g = Atone(i1)
g.input = i2
g.freq = Sig(1000)

h = DCBlock(i1)
h.input = i2

i = Allpass(i1)
i.input = i2
i.delay = Sig(0.25)
i.feedback = Sig(0.5)

j = Allpass2(i1)
j.input = i2
j.freq = Sig(1000)
j.bw = Sig(500)

k = Phaser(i1, num=12)
k.input = i2
k.freq = Sig(100)
k.spread = Sig(1.23)
k.q = Sig(2.5)
k.feedback = Sig(0.75)

l = Vocoder(i1, i2)
l.input = i2
l.input2 = i1
l.freq = Sig(100)
l.spread = Sig(1.23)
l.q = Sig(4.5)
l.slope = Sig(0.9)
l.stages = 12
l.stages = 32

m = SVF(i1)
m.input = i2
m.freq = Sig(1000)
m.q = Sig(0.5)
m.type = Sig(1)

n = SVF2(i1)
n.input = i2
n.freq = Sig(1000)
n.q = Sig(0.5)
n.shelf = Sig(-3.5)
n.type = Sig(1)
n.setOrder([9,8,7,6,5,4,3,2,1,0])

o = Average(i1)
o.input = i2
o.setSize(8)
o.setSize(12)

p = Reson(i1)
p.input = i2
p.freq = Sig(1000)
p.q = Sig(5.5)

q = Resonx(i1)
q.input = i2
q.freq = Sig(1000)
q.q = Sig(5.5)
q.stages = 2
q.stages = 6

r = ButLP(i1)
r.input = i2
r.freq = Sig(1000)

t = ButHP(i1)
t.input = i2
t.freq = Sig(1000)

u = ButBP(i1)
u.input = i2
u.freq = Sig(1000)
u.q = Sig(5.5)

v = ButBR(i1)
v.input = i2
v.freq = Sig(1000)
v.q = Sig(5.5)

w = ComplexRes(i1)
w.input = i2
w.freq = Sig(1000)
w.decay = Sig(0.43)

x = MoogLP(i1)
x.input = i2
x.freq = Sig(1000)
x.res = Sig(0.5)

y = Hilbert(i1)
y.input = i2

s.process()
s.process()
s.stop()
s.shutdown()
