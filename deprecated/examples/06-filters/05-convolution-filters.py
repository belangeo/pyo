"""
05-convolution-filters.py - Circular convolution.

A circular convolution is defined as the integral of the
product of two functions after one is reversed and shifted.

Circular convolution allows to implement very complex FIR
filters, at a CPU cost that is related to the filter impulse
response (kernel) length.

Within pyo, there is a family of IR* filter objects using
circular convolution with predefined kernel:

- IRAverage : moving average filter
- IRFM : FM-like filter
- IRPulse : comb-like filter
- RWinSinc : break wall filters (lp, hp, hp, br)

For general circular convolution, use the Convolve object
with a PyoTableObject as the kernel, as in this example:

A white noise is filtered by four impulses taken from the input mic. 

Call r1.play(), r2.play(), r3.play() or r4.play() in the Interpreter 
field while making some noise in the mic to fill the impulse response 
tables. The slider handles the morphing between the four kernels.

Call t1.view(), t2.view(), t3.view() or t4.view() to view impulse 
response tables.

Because circular convolution is very expensive, TLEN (in samples) 
should be keep small.

"""
from pyo import *

# duplex=1 to tell the Server we need both input and output sounds.
s = Server(duplex=1).boot()

# Length of the impulse response in samples.
TLEN = 512

# Conversion to seconds for NewTable objects.
DUR = sampsToSec(TLEN)

# Excitation signal for the filters.
sf = Noise(0.5)

# Signal from the mic to record the kernels.
inmic = Input()

# Four tables and recorders.
t1 = NewTable(length=DUR, chnls=1)
r1 = TableRec(inmic, table=t1, fadetime=0.001)

t2 = NewTable(length=DUR, chnls=1)
r2 = TableRec(inmic, table=t2, fadetime=0.001)

t3 = NewTable(length=DUR, chnls=1)
r3 = TableRec(inmic, table=t3, fadetime=0.001)

t4 = NewTable(length=DUR, chnls=1)
r4 = TableRec(inmic, table=t4, fadetime=0.001)

# Interpolation control between the tables.
pha = Sig(0)
pha.ctrl(title="Impulse responses morphing")

# Morphing between the four impulse responses.
res = NewTable(length=DUR, chnls=1)
morp = TableMorph(pha, res, [t1, t2, t3, t4])

# Circular convolution between the excitation and the morphed kernel.
a = Convolve(sf, table=res, size=res.getSize(), mul=0.1).mix(2).out()

s.gui(locals())
