# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_PyoTableObject.py 
# There should not be any definitely lost bytes.

from pyo import *

s = Server().boot().start()

t0 = DataTable(size=4, init=[4,3,2,1])

t1 = DataTable(size=8, init=[1,2,3,4,5,6,7,8])

t1.add(1)
t1.add([1,1,1,1,1,1,1,1])
t1.add(DataTable(size=8, init=[1,1,1,1,1,1,1,1]))
t1.sub(1)
t1.sub([1,1,1,1,1,1,1,1])
t1.sub(DataTable(size=8, init=[1,1,1,1,1,1,1,1]))
t1.mul(1)
t1.mul([1,1,1,1,1,1,1,1])
t1.mul(DataTable(size=8, init=[1,1,1,1,1,1,1,1]))
t1.div(1)
t1.div([1,1,1,1,1,1,1,1])
t1.div(DataTable(size=8, init=[1,1,1,1,1,1,1,1]))

t1.copyData(t0, 0, 0, -1)
t1.lowpass()
t1.fadein(dur=0.0001)
t1.fadeout(dur=0.0001)
t1_0 = t1.get(0)
t1_content = t1.getTable()

t2 = t1.copy()

t3 = HarmTable()
t3.replace([1,.5,.3])

a = Osc(t3, freq=50.5, phase=0.5, interp=2)
a.freq = Sig(50.5)
a.phase = Sig(0.5)
a.interp = 1
a.table = t2

t4 = ChebyTable([1,0.5,0.3])
t4.replace([1])

t5 = HannTable()
t5.setSize(512)

t6 = SincTable()
t6.setFreq(3.14159)
t6.setWindowed(True)
t6.setSize(512)

t7 = WinTable()
t7.setType(1)
t7.setSize(512)

t8 = ParaTable()
t8.setSize(512)

t9 = LinTable()
t9.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t9.setSize(512)

t10 = LogTable()
t10.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t10.setSize(512)

t11 = CosTable()
t11.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t11.setSize(512)

t12 = CosLogTable()
t12.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t12.setSize(512)

t13 = CurveTable()
t13.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t13.setTension(1.5)
t13.setBias(0.6)
t13.setSize(512)

t14 = ExpTable()
t14.replace([(0,0.1), (4096,0.9), (8192,0.1)])
t14.setExp(3.5)
t14.setInverse(False)
t14.setSize(512)

t15 = NewTable(1.5)
t15.setFeedback(0.5)
t16 = NewTable(1.0, init=[0.0 for i in range(int(s.getSamplingRate()))])

t17 = AtanTable()
t17.setSlope(0.75)
t17.setSize(512)

t18 = PadSynthTable(basefreq=440, spread=1, bw=50, bwscl=1, nharms=64, damp=0.7, size=262144)
t18.setBaseFreq(405.5)
t18.setSize(2<<15)
t18.setSize(2<<18)

t19 = SharedTable("Dummy", True, 1024)

s.stop()
s.shutdown()
