# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_functions.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot()

print(serverCreated())
print(serverBooted())
print(midiToHz(48))
print(midiToHz([48, 50, 52]))
print(midiToHz((48, 50, 52)))
print(midiToTranspo(48))
print(midiToTranspo([48, 50, 52]))
print(midiToTranspo((48, 50, 52)))
print(sampsToSec(44100))
print(sampsToSec([44100, 48000]))
print(sampsToSec((44100, 48000)))
print(secToSamps(1))
print(secToSamps([1, 1.5]))
print(secToSamps((1, 1.5)))
print(beatToDur(1/4, bpm=90))
print(beatToDur([1/4, 1/3, 1/2], bpm=90))
print(beatToDur((1/4, 1/3, 1/2), bpm=90))

a = [(0,0), (0.25, 1), (0.33, 1), (1,0)]
b = linToCosCurve(a, yrange=[0, 1], totaldur=1, points=8192)

print(rescale(0.5, 0, 1, 20, 20000, False, True))
print(rescale([0.25, 0.5, 0.75], 0, 1, 20, 20000, False, True))

print(floatmap(0.5, 0, 1, 4))

print(distanceToSegment([0.6, 0.5], [0, 0], [1, 1], xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False))
print(distanceToSegment((0.6, 0.5), (0, 0), (1, 1), xmin=0.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False))

l = [(0, 0), (0.1, 0.7), (0.2, 0.5), (0.7, 0.4), (0.18, 0.17), (0.62, 0.35), (0.9, 0.1), (0.4, 0.75), (0.3, 0.4)]
print(reducePoints(l, tolerance=0.5))
l = [[0, 0], [0.1, 0.7], [0.2, 0.5], [0.7, 0.4], [0.18, 0.17], [0.62, 0.35], [0.9, 0.1], [0.4, 0.75], [0.3, 0.4]]
print(reducePoints(l, tolerance=0.5))

s.shutdown()
