# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_PyoMatrixObject.py 
# There should not be any definitely lost bytes.

import os
from pyo import *

s = Server().boot().start()

m = NewMatrix(2, 2, [[1,2], [3,4]])
m.write(os.path.join(os.getcwd(),"matrix_write_temp.txt"))
m.read(os.path.join(os.getcwd(),"matrix_write_temp.txt"))
os.remove(os.path.join(os.getcwd(),"matrix_write_temp.txt"))
m.getSize()
m.normalize()
m.blur()
m.boost()
m.put(0.5, 1, 1)
v1 = m.get(1, 1)
v2 = m.getInterpolated(0.5, 0.5)

mp = MatrixPointer(m, Sig(0.5), Sig(0.5), mul=0.5, add=0.5)
mp2 = MatrixPointer(m, Sig(0.5), Sig(0.5), mul=Sig(0.5), add=Sig(0.5))

m2 = NewMatrix(1024, 16)

mr = MatrixRec(Sig(0), m2, fadetime=0.01)
mr.matrix = NewMatrix(1024, 16)
mr.input = Sig(1)

mrl = MatrixRecLoop(Sig(0), m2)
mrl.matrix = NewMatrix(1024, 16)
mrl.input = Sig(1)

sm1 = NewMatrix(1024, 16)
sm2 = NewMatrix(1024, 16)
sm3 = NewMatrix(1024, 16)
mm = MatrixMorph(Sig(0), m2, [sm1, sm2])
mm.matrix = NewMatrix(1024, 16)
mm.sources = [sm1, sm2, sm3]

s.process()
s.stop()
s.shutdown()
