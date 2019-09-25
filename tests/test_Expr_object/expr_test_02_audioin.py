from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    # self-modulated sine wave with running phase given in input.
    ph = Phasor(500)
    ex = Expr(ph, "(sin (+ (* (* 2 (pi)) $x[0]) (* $y[-1] 0.8)))").out()
elif TEST == 1:
    # First-order IIR lowpass filter.
    sf = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True)
    ex = Expr(sf, "+ $x[0] (* (- $y[-1] $x[0]) 0.95)").out()
elif TEST == 2:
    # Create a triangle wave whose frequency is given in input.
    fr = SigTo(100)
    fr.ctrl([SLMap(50, 1000, "log", "value", 100)])
    t = """
(let #ph (~ $x[0]))
(- (* (min #ph (- 1 #ph)) 4) 1)
"""
    ex = Expr(fr, t).out()

ex.editor()
sc = Scope(ex)

s.gui(locals())