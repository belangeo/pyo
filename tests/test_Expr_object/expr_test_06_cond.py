from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 1

if TEST == 0:
    t = """
(define osc (
    sin (* twopi (~ $1))
    )
)

(let #sig (osc 200))

// half-wave rectifier
(if (>= #sig 0)
    (#sig) 
    (0)
)

"""
    ex = Expr(Sig(0), t).out()
elif TEST == 1:
    t = """
(define osc (
        sin (* twopi $1)
    )
)

(define env (
        (+ (* (cos (* twopi $1)) -0.5) 0.5)
    )
)

(define pulsar (
        (let #phase (~ $1))
        (let #adjusted (/ #phase $2))
        (* (< #phase $2) (* (osc #adjusted) (env #adjusted)))
    )
)

+ (pulsar 172 (+ (* (osc (~ .1)) 0.4) 0.5))
  (pulsar 86 (+ (* (osc (~ .15)) 0.4) 0.5))

"""
    ex = Expr(Sig(0), t).out()

ex.editor()
sc = Scope(ex)

s.gui(locals())