# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_dsl.py 
# There should not be any definitely lost bytes.

###
### Expr and MML should be done in master (need more testing) and merge back in stripped branch afterward.
###

from pyo import *

s = Server().boot().start()

i1 = Sig(0)
i2 = Sig(0)

a = Expr(Sig(0), "sin (* twopi (~ (sah (* (randi 5 10) 100) (~ 4 0)) 0))").out()

s.process()
s.process()
s.stop()
s.shutdown()
