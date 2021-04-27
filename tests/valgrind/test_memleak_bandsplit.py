# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_bandsplit.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = BandSplit(i1, num=8, q=0.5)
a.input = i2
a.q = Sig(0.5)

b  =FourBand(i1)
b.input = i2
b.freq1 = Sig(500)
b.freq2 = Sig(1000)
b.freq3 = Sig(3000)

c = MultiBand(i1, num=12)
c.input = i2
c.setFrequencies(freqs=[100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100])

s.process()
s.process()
s.stop()
s.shutdown()
