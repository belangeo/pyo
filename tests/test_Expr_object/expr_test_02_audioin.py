from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    # self-modulated sine wave with running phase given in input.
    ph = Phasor(86)
    ex = Expr(ph, "(sin (+ (* twopi $x[0]) (* $y[-1] 0.8)))").out()
elif TEST == 1:
    # First-order IIR lowpass filter.
    n = Noise()
    ex = Expr(n, "+ $x[0] (* (- $y[-1] $x[0]) 0.95)").out()
elif TEST == 2:
    # Create a triangle wave whose frequency is given in input.
    fr = SigTo(86)
    fr.ctrl([SLMap(50, 1000, "log", "value", 100)])
    t = """
(let #ph (~ $x[0]))
(- (* (min #ph (- 1 #ph)) 4) 1)
"""
    ex = Expr(fr, t).out()
elif TEST == 3:
    # Multi-input signals (from a single PyoObject).
    sig = Sine(freq=[10.7647, 10.0000, 87.9023, 296.4831])
    sig.ctrl([SLMap(0.1, 1000, "log", "freq", sig.freq)])
    t = """
(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])
"""
    ex = Expr(sig, t).out()
elif TEST == 4:
    # Multi-input signals (from a list of PyoObjects).
    s1 = Sine(freq=10.7647)
    s2 = Sine(freq=10.0)
    s3 = Sine(freq=87.9023)
    s4 = Sine(freq=296.4831)
    t = """
(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])
"""
    ex = Expr([s1, s2, s3, s4], t).out()
elif TEST == 5:
    # Multi-input signals (from a list of multi-stream PyoObjects).
    s1 = Sine(freq=[10.7647, 10.0])
    s2 = Sine(freq=[87.9023, 296.4831])
    t = """
(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])
"""
    ex = Expr([s1, s2], t).out()

ex.editor()
sc = Scope(ex)

s.gui(locals())
