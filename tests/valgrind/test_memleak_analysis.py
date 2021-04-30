# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_analysis.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server(audio="manual").boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = Follower(i1)
a.setInput(i2)
a.setFreq(Sig(20))

b = Follower2(i1, 0.01, 0.1)
b.setInput(i2)
b.setRisetime(Sig(0.01))
b.setFalltime(Sig(0.1))

c = ZCross(i1, thresh=0.5)
c.setInput(i2)

d = Yin(i1)
d.setInput(i2)
d.setTolerance(0.1)
d.setMaxfreq(2000)
d.setCutoff(5000)

e = Centroid(i1)
e.setInput(i2)

f = AttackDetector(i1)
f.setInput(i2)
f.setDeltime(0.12)
f.setMaxthresh(-12.5)
f.setCutoff(2500)

def callback(x):
    print(x)
def callback2(x):
    print(x)

g = PeakAmp(i1, function=callback)
g.setInput(Sig(0.5))
g.setFunction(callback2)

h = RMS(i1, function=callback)
h.setInput(Sig(0.5))
h.setFunction(callback2)

s.process()
s.process()
s.stop()
s.shutdown()
