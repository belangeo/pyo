from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 1

if TEST == 0:
    t = """// Play multiple sine waves.
(load generators.expr)
(out 0 (* (osc 250) 0.2))
(out 1 (* (osc 500) 0.2))
(out 2 (* (osc 750) 0.2))
(out 3 (* (osc 1000) 0.2))
"""
    ex = Expr(Sig(0), t, outs=4).out()
if TEST == 1:
    t = """// Lorenz strange attractor.
(let #pit 500)      // 1.0 -> 750.0
(let #chaos 2.0)    // 0.5 -> 3.0
(let #delta (* (/ 1.0 sr) #pit))
(let #A 10.0)
(let #B 28.0)

(let #vDX (* (- $y1[-1] $y0[-1]) #A))
(let #vDY (- (* $y0[-1] (- #B $y2[-1])) $y1[-1]))
(let #vDZ (- (* $y0[-1] $y1[-1]) (* #chaos $y2[-1])))

(out 0 (+ $y0[-1] (* #vDX #delta)))
(out 1 (+ $y1[-1] (* #vDY #delta)))
(out 2 (+ $y2[-1] (* #vDZ #delta)))

"""
    ex = Expr(Sig(0), t, outs=3, initout=1.0, mul=[0.044, 0.0328, 0.0]).out()

ex.editor()
sc = Scope(ex)
sp = Spectrum(ex)

s.gui(locals())
