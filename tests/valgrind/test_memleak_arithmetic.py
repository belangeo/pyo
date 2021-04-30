# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_arithmetic.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = Sin(i1)
a.input = i2

b = Cos(i1)
b.input = i2

c = Tan(i1)
c.input = i2

d = Tanh(i1)
d.input = i2

e = Abs(i1)
e.input = i2

f = Sqrt(i1)
f.input = i2

g = Log(i1)
g.input = i2

h = Log2(i1)
h.input = i2

i = Log10(i1)
i.input = i2

j = Atan2(b=0.5, a=0.5)
j.b = Sig(0.5)
j.a = Sig(0.5)

k = Floor(i1)
k.input = i2

l = Ceil(i1)
l.input = i2

m = Round(i1)
m.input = i2

n = Pow(base=2.5, exponent=1.5)
n.base = Sig(2.5)
n.exponent = Sig(1.5)

o = Exp(i1)
o.input = i2

p = Div(a=0.5, b=0.5)
p.a = Sig(0.5)
p.b = Sig(0.5)

q = Sub(a=0.5, b=0.5)
q.a = Sig(0.5)
q.b = Sig(0.5)

s.process()
s.process()
s.stop()
s.shutdown()
