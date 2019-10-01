from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 2

if TEST == 0:
    t = """
(load utils.expr)       // scalef
(load generators.expr)  // osc, pwm

(pwm 110 
     (scalef (osc 0.2) 0.05 0.95)
)
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 1:
    t = """
(load generators.expr)  // square
(load filters.expr)     //notch

(notch (square 86) 5000 0.75)
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 2:
    t = """
(load utils.expr)       // scalef
(load generators.expr)  // noise
(load filters.expr)     // peak

(peak (noise 0.5) (scalef (osc 0.2) 1000 5000) 0.9)
"""
    ex = Expr(Sig(0), t).out()

ex.editor()
sc = Scope(ex)
sp = Spectrum(ex)

s.gui(locals())