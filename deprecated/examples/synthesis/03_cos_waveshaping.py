"""
Cos waveshaping synthesis.

"""
from pyo import *
import math

s = Server(sr=44100, nchnls=2, duplex=0).boot()

### Controls ###
drv = Sig(0)
drv.ctrl(title="Drive")
phi = Sig(0, mul=math.pi / 2)
phi.ctrl(title="Odd harmonics <----> Even harmonics")
frs = Sig([40.04, 39.41, 41.09, 38.7])
frs.ctrl([SLMap(10.0, 1000.0, "log", "value", frs.value)], title="Input osc frequencies")

# Amplitude and phase scaling
amp = drv * 2 * math.pi + 0.5
phiscl = Scale(phi, inmin=0, inmax=math.pi / 2, outmin=1, outmax=0.5)

# Amplitude envelope
f = Linseg([(0, 0), (0.5, 0), (1, 1)], mul=amp).play()

# Signal with lot of harmonics
t = HarmTable([1, 0, 0, 0, 0, 0.33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.2, 0, 0, 0, 0, 0, 0, 0, 0.143,])
a = OscBank(t, freq=frs, spread=0.0001, slope=1, num=24, fjit=True, mul=f)

# Cos waveshaping
b = Cos(math.pi * a + phi)
b1 = DCBlock(b * phiscl)
c = Sig(b1 / amp, mul=0.2).out()

s.gui(locals())
