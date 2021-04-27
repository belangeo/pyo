# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_TableProcess.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

t1 = NewTable(2)
t2 = NewTable(2)
t3 = NewTable(2)

i1 = Sig(0)
i2 = Sig(0)

a = TableRec(i1, t1)
a.setInput(i2)
a.setTable(t2)

outT1 = NewTable(2)
outT2 = NewTable(2)
b = TableMorph(Sig(0.5), outT1, [t1, t2])
b.setInput(i2)
b.setTable(outT2)
b.setSources([t1, t2, t3])

trig1 = Trig().play()
trig2 = Trig().play()
c = TrigTableRec(i1, trig1, t1)

d = TablePut(i1, t1)
d.setInput(i2)
d.setTable(t2)

e = TableWrite(i1, i2, t1, 0, 1024)
e.setInput(i2)
e.setPos(i1)
e.setTable(t2)

s.process()
s.process()
s.stop()
s.shutdown()
