from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    # Play a sine wave.
    ex = Expr(Sig(0), "sin (* twopi (~ 500)) ").out()
elif TEST == 1:
    # Up rising stepped frequencies.
    ex = Expr(Sig(0), "sin (* twopi (~ (+ (* (floor (* (~ .25) 16)) 100) 500)))").out()
elif TEST == 2:
    # Sampled and held frequencies.
    ex = Expr(Sig(0), "sin (* twopi (~ (sah (* (randi 5 10) 100) (~ 4 0)) 0))").out()
elif TEST == 3:
    # Pulse-Width-Modulation generated wih two sligthly detuned ramps.
    ex = Expr(Sig(0), "< (~ 80 0) (~ 79.9 0)", add=-0.5).out()

ex.editor()
sc = Scope(ex)

s.gui(locals())