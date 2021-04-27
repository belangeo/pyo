# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_triggers.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

t1 = Trig().play()
t2 = Trig().play()

a = TrigRandInt(t1)
a.input = t2
a.max = Sig(100)

b = TrigRand(t1)
b.input = t2
b.min = Sig(0.5)
b.max = Sig(0.7)
b.port = 0.5

l1 = [1.1,2.2,3.3,4.4,5.5]
l2 = [1.2,2.3,3.4,4.5,5.6]

c = TrigChoice(t1, l1)
c.port = 0.5
c.input = t2
c.choice = l2

def call1(): pass
def call2(x): pass

d = TrigFunc(t1, call1)
d.input = t2

e = TrigFunc(t1, call2, 1)
e.input = t2

e1 = WinTable()
e2 = WinTable(3)

f = TrigEnv(t1, e1)
f.input = t2
f.table = e2
f.dur = Sig(0.5)

g = TrigLinseg(t1, [(0,0), (0.1,1), (0.2,0)])
g.input = t2
g.setList([(0,1), (0.1,0), (0.2,1), (0.3, 0)])

h = TrigExpseg(t1, [(0,0), (0.1,1), (0.2,0)])
h.input = t2
h.setList([(0,1), (0.1,0), (0.2,1), (0.3, 0)])

i = TrigXnoise(t1)
i.input = t2
i.dist = 10
i.x1 = Sig(0.5)
i.x2 = Sig(0.5)

j = TrigXnoiseMidi(t1)
j.input = t2
j.dist = 10
j.x1 = Sig(0.5)
j.x2 = Sig(0.5)
j.scale = 2
j.setRange(48, 64)

k = Counter(t1)
k.input = t2
k.min = 1
k.max = 3
k.dir = 2

i1 = Sig(0)
i2 = Sig(0)

l = Thresh(i1)
l.input = i2
l.threshold = Sig(0.5)

m = Percent(t1)
m.input = t2
m.percent = Sig(50)

n = Timer(t1, t2)
n.input = t2
n.input2 = t1

o = Iter(t1, l1)
o.input = t2
o.choice = l2

p = Count(t1)
p.min = 1
p.max = 5

q = NextTrig(t1, t2)
q.input = t2
q.input2 = t1

r = TrigVal(t1)
r.input = t2
r.value = Sig(1)

s.process()
s.process()
s.stop()
s.shutdown()
