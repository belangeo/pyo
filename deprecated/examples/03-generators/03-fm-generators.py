"""
03-fm-generators.py - Frequency modulation synthesis.

There two objects in the library that implement frequency
modulation algorithms. These objects are very simple, although
powerful. It is also relatively simple to build a custom FM
algorithm, this will be covered in the tutorials on custom
synthesis algorithms.

Use the "voice" slider of the window "Input interpolator" to
interpolate between the two sources. Use the controller windows
to change the parameters of the FM algorithms.

Note what happened in the controller window when we give a
list of floats to an object's argument.

"""
from pyo import *

s = Server().boot()

# FM implements the basic Chowning algorithm
fm1 = FM(carrier=250, ratio=[1.5, 1.49], index=10, mul=0.3)
fm1.ctrl()

# CrossFM implements a frequency modulation synthesis where the
# output of both oscillators modulates the frequency of the other one.
fm2 = CrossFM(carrier=250, ratio=[1.5, 1.49], ind1=10, ind2=2, mul=0.3)
fm2.ctrl()

# Interpolates between input objects to produce a single output
sel = Selector([fm1, fm2]).out()
sel.ctrl(title="Input interpolator (0=FM, 1=CrossFM)")

# Displays the spectrum contents of the chosen source
sp = Spectrum(sel)

s.gui(locals())
