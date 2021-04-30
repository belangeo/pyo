# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_generators.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server(audio="manual").boot().start()

a = LFO()
a.freq = Sig(100)
a.sharp = Sig(0.5)
a.type = 2

b = Noise()

c = PinkNoise()

d = BrownNoise()

i1 = Sig(0)
i2 = Sig(0)

t1 = NewTable(2)
t2 = NewTable(2)

e = OscBank(t1, freq=100, spread=1, slope=0.9, frndf=1, frnda=0, arndf=1, arnda=0, num=24, fjit=False)
e.table = t2
e.freq = Sig(50.5)
e.spread = Sig(0.5)
e.slope = Sig(0.5)
e.frndf = Sig(0.75)
e.frnda = Sig(0.5)
e.arndf = Sig(0.5)
e.arnda = Sig(0.5)
e.fjit = True

f = Sine()
f.freq = Sig(100)
f.phase = Sig(0.5)

g = FastSine()
g.freq = Sig(100)
g.quality = 1

h = SineLoop()
h.freq = Sig(100)
h.feedback = Sig(0.5)

i = Osc(t1)
i.table = t2
i.freq = Sig(100)
i.phase = Sig(0.5)
i.interp = 4

j = OscLoop(t1)
j.table = t2
j.freq = Sig(100)
j.phase = Sig(0.5)

tr1 = Trig().play()
tr2 = Trig().play()
k = OscTrig(t1, tr1)
k.table = t2
k.trig = tr2
k.freq = Sig(100)
k.phase = Sig(0.5)
k.interp = 4

l = Phasor()
l.freq = Sig(100)
l.phase = Sig(0.5)

m = Pointer(t1, i1)
m.table = t2
m.index = i2

n = Pointer2(t1, i1)
n.table = t2
n.index = i2
n.interp = 4

o = TableIndex(t1, i1)
o.table = t2
o.index = i2

p = Lookup(t1, i1)
p.table = t2
p.index = i2

env1 = WinTable()
env2 = WinTable()

q = Pulsar(t1, env1)
q.table = t2
q.env = env2
q.freq = Sig(100)
q.frac = Sig(0.5)
q.phase = Sig(0.5)
q.interp = 4

r = TableRead(t1)
r.table = t2
r.freq = Sig(0.5)
r.loop = True
r.interp = 4

t = FM()
t.carrier = Sig(100)
t.ratio = Sig(0.5)
t.index = Sig(2.5)

u = CrossFM()
u.carrier = Sig(100)
u.ratio = Sig(0.5)
u.ind1 = Sig(2.5)
u.ind2 = Sig(0.5)

v = Blit()
v.freq = Sig(100)
v.harms = Sig(40.5)

w = Rossler(stereo=True)
w.pitch = Sig(1.5)
w.chaos = Sig(0.5)

x = Lorenz(stereo=True)
x.pitch = Sig(1.5)
x.chaos = Sig(0.5)

y = ChenLee(stereo=True)
y.pitch = Sig(1.5)
y.chaos = Sig(0.5)

z = SumOsc()
z.freq = Sig(100)
z.ratio = Sig(0.5)
z.index = Sig(10.5)

aa = SuperSaw()
aa.freq = Sig(99.5)
aa.detune = Sig(0.5)
aa.bal = Sig(0.5)

bb = RCOsc()
bb.freq = Sig(100)
bb.sharp = Sig(0.5)

cc = TableScale(t1, t2)
cc.table = t2
cc.outtable = t1

dd = TableFill(i1, t1)
dd.input = i2
dd.table = t2

ee = TableScan(t1)
ee.table = t2

s.process()
s.process()
s.stop()
s.shutdown()
