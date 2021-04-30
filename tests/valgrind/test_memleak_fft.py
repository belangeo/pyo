# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_fft.py 
# There should not be any definitely lost bytes.

import os
os.environ["PYO_GUI_WX"] = "0"

from pyo import *

s = Server(audio="manual").boot().start()

i1 = Sig(0)
i2 = Sig(0)

fin = FFT(i1)
fin.setInput(i2)
fin.setSize(512)
fin.setSize(2048)

a = CarToPol(fin["real"], fin["imag"])
b = PolToCar(a["mag"], a["ang"])

c = FrameDelta(a["ang"])
c.input = a["mag"]
c.framesize = 512
c.framesize = 2048

d = FrameAccum(Sig([0,0,0,0]))
d.setInput(Sig([1,1,1,1]))
d.framesize = 512
d.framesize = 2048

e = Vectral(a["mag"])
e.input = a["ang"]
e.framesize = 512
e.framesize = 2048
e.up = Sig(0.9)
e.down = Sig(0.6)
e.damp = Sig(0.99)

def callback(x):
    pass
def callback2(x):
    pass

f = Spectrum(i1, function=callback)
f.size = 512
f.size = 2048
f.setFunction(callback2)


last = b
fout = IFFT(last["real"], last["imag"])
fout.setInReal(fin["real"])
fout.setInImag(fin["imag"])
fout.setSize(512)
fout.setSize(2048)

m2 = NewMatrix(512, 512)
m2.genSineTerrain(1, 0.15)
index2 = Phasor([0.4, 0.5])
phase2 = Noise(0.7)
fout2 = IFFTMatrix(m2, index2, phase2, size=2048, overlaps=16, wintype=2).mix(2).out()

s.process()
s.process()
s.stop()
s.shutdown()
