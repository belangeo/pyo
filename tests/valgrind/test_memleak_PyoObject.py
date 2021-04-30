# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_PyoObject.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

a = Sig(1)
a1 = Sig(Sig(1))
a2 = Sig(1, mul=Sig(1))
a3 = Sig(1, mul=Sig(1), add=Sig(0))
a.value = 0
a.value = Sig(0)
a.mul = 1
a.mul = Sig(1)
a.add = 0
a.add = Sig(0)
a.mul = Sig(1) * 1
a.mul = 1 * Sig(1)
a.mul = Sig(1) * Sig(1)
a.add = Sig(0) * 0.5
a.add = 1 * Sig(0)
a.add = Sig(0) * Sig(1)
a.mul = Sig(0) + 0.5
a.mul = 1 + Sig(0)
a.mul = Sig(0) + Sig(1)
a.mul = Sig(1) - 0.5
a.mul = 1 - Sig(1)
a.mul = Sig(1) - Sig(1)
a.mul = Sig(1) / 2.0
a.mul = Sig(1) / Sig(2.0)
a.mul = 1.1 / Sig(2.0)
a *= 0.5
a *= Sig(0.5)
a += 0.5
a += Sig(0.5)
a /= 0.5
a /= Sig(0.5)
a -= 0.5
a -= Sig(0.5)

#a[0] *= 0.5

b = Sig(1)
b.mul = a[0] * 0.5
b.mul = a[0] * a[0]
b.mul = a[0] + 0.5
b.mul = a[0] + a[0]
b.mul = a[0] / 0.5
b.mul = a[0] / a[0]
b.mul = a[0] - 0.5
b.mul = a[0] - a[0]


#a = RCOsc(mul=b)

#a = RCOsc(); a.sharp = Sig(0.5)
#a = RCOsc(); a.sharp = Sig(0.5); a.freq = Sig(100)
#a = RCOsc(sharp=Sig(0.5))
#a = RCOsc(freq=Sig(100), sharp=Sig(0.5))
#_a = Sig(0.5); a = RCOsc(sharp=_a)

#a = RCOsc(); b = Sig(a)
#a = RCOsc(); b = Sig(a); c = Sig(b)

s.process()
s.stop()
s.shutdown()
