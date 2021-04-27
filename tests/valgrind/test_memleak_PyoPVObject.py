# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_PyoPVObject.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

a = Sig(1)

def callback(magn, freq):
    pass
def callback2(magn, freq):
    pass

pva = PVAnal(a, callback=callback)

pvt = PVTranspose(pva, transpo=0.75)
pvt.transpo = Sig(0.75)

pvv = PVVerb(pvt)
pvv.revtime = Sig(0.75)
pvv.damp = Sig(0.75)

pvg = PVGate(pvv, thresh=-9.5, damp=0.2)
pvg.thresh = Sig(-9.5)
pvg.damp = Sig(0.2)
pvg.inverse = True

pvc = PVCross(pvv, pvg, fade=0.5)
pvc.fade = Sig(0.5)

pvm = PVMult(pvv, pvg)

f1table = HarmTable()
f2table = HarmTable()
pvf = PVFilter(pvm, f1table, gain=0.5)
pvf.table = f2table

pvd = PVDelay(pvf, f1table, f2table)
pvd.deltable = f2table
pvd.feedtable = f1table
pvd.mode = 1

pvb = PVBuffer(pvd, Sig(0), pitch=0.75, length=0.5)
pvb.pitch = Sig(0.75)
pvb.length = 1.5

pvsh = PVShift(pvb, shift=22.5)
pvsh.shift = Sig(22.5)

pvam = PVAmpMod(pvsh, basefreq=0.5, spread=0.5, shape=0)
pvam.basefreq = Sig(0.5)
pvam.spread = Sig(0.5)
pvam.shape = 2

pvfm = PVFreqMod(pvam, basefreq=0.5, spread=0.5, depth=0.1, shape=0)
pvfm.basefreq = Sig(0.5)
pvfm.spread = Sig(0.5)
pvfm.depth = Sig(0.1)
pvfm.shape = 2

pvbl = PVBufLoops(pvfm, low=0.5, high=0.5, mode=0, length=0.5)
pvbl.low = Sig(0.5)
pvbl.high = Sig(0.5)
pvbl.mode = 1
pvbl.length = 1.5

pvbtl = PVBufTabLoops(pvbl, speed=f1table, length=0.5)
pvbtl.speed = f2table
pvbtl.length = 1.5

pvmix = PVMix(pvbl, pvbtl)
pvmix.input = pvfm
pvmix.input2 = pvam

pvlast = pvmix
pvs = PVSynth(pvlast)
pvs2 = PVAddSynth(pvlast, pitch=0.75, mul=0.5, add=0.01)
pvs2.pitch = Sig(0.75)
pvs2.mul = Sig(0.5)
pvs2.add = Sig(0.01)
pvs2.num = 50
pvs2.first = 5
pvs2.inc = 2

pva.size = 2048
pva.overlaps = 8
pva.wintype = 4
pva.callback = callback2

s.process()
s.stop()
s.shutdown()
