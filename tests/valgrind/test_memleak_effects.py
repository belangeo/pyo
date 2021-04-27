# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_effects.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = Chorus(i1)
a.input = i2
a.setDepth(Sig(1.5))
a.setFeedback(Sig(0.5))
a.setBal(Sig(0.5))

b = Compress(i1)
b.input = i2
b.thresh = Sig(-22.5)
b.ratio = Sig(3.5)
b.risetime = Sig(0.01)
b.falltime = Sig(0.15)

c = Gate(i1)
c.input = i2
c.thresh = Sig(-22.5)
c.risetime = Sig(0.01)
c.falltime = Sig(0.15)

d = Balance(i1, i2)
d.input = i2
d.input2 = i1
d.freq = Sig(12.5)

e = Expand(i1)
e.input = i2
e.downthresh = Sig(-22.5)
e.upthresh = Sig(-2.5)
e.ratio = Sig(3.5)
e.risetime = Sig(0.01)
e.falltime = Sig(0.15)

t1 = DataTable(10, init=[-1,1,-1,1,-1,1,-1,1,-1,1])
t2 = DataTable(10, init=[1,-1,1,-1,1,-1,1,-1,1,-1])

f = Convolve(i1, t1, 10)
f.setInput(i2)
f.setTable(t2)

g = IRWinSinc(i1)
g.input = i2
g.freq = Sig(2000)
g.bw = Sig(500)
g.type = 1

h = IRAverage(i1)
h.input = i2

i = IRPulse(i1)
i.input = i2
i.freq = Sig(2000)
i.bw = Sig(250)
i.type = 1

j = IRFM(i1)
j.input = i2
j.carrier = Sig(1000)
j.ratio = Sig(1.5)
j.index = Sig(20.5)

k = Delay(i1)
k.input = i2
k.delay = Sig(0.5)
k.feedback = Sig(0.5)

l = SDelay(i1)
l.input = i2
l.delay = Sig(0.5)

m = Waveguide(i1)
m.input = i2
m.freq = Sig(100.5)
m.dur = Sig(20.5)

n = AllpassWG(i1)
n.input = i2
n.freq = Sig(200)
n.feed = Sig(0.5)
n.detune = Sig(0.5)

o = Delay1(i1)
o.input = i2

p = SmoothDelay(i1)
p.input = i2
p.delay = Sig(0.25)
p.feedback = Sig(0.5)

q = Disto(i1)
q.input = i2
q.drive = Sig(0.9)
q.slope = Sig(0.9)

r = Clip(i1)
r.input = i2
r.min = Sig(0.1)
r.max = Sig(0.9)

t = Mirror(i1)
t.input = i2
t.min = Sig(0.1)
t.max = Sig(0.9)

u = Wrap(i1)
u.input = i2
u.min = Sig(0.1)
u.max = Sig(0.9)

v = Degrade(i1)
v.input = i2
v.bitdepth = Sig(5.5)
v.srscale = Sig(0.5)

w = Min(i1)
w.inpuut = i2
w.comp = Sig(0.5)

x = Max(i1)
x.inpuut = i2
x.comp = Sig(0.5)

y = Freeverb(i1)
y.input = i2
y.size = Sig(0.9)
y.damp = Sig(0.9)
y.bal = Sig(0.5)

z = Harmonizer(i1)
z.input = i2
z.transpo = Sig(1.5)
z.feedback = Sig(0.5)
z.winsize = 0.05
z.winsize = 0.2

aa = WGVerb(i1)
aa.input = i2
aa.feedback = Sig(0.8)
aa.cutoff = Sig(4000)
aa.bal = Sig(0.5)

bb = STRev(i1)
bb.input = i2
bb.inpos = Sig(0.5)
bb.revtime = Sig(1.5)
bb.cutoff = Sig(4000)
bb.bal = Sig(0.5)

s.process()
s.process()
s.stop()
s.shutdown()
