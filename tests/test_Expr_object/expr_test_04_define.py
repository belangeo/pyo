from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    t = """
(define nz (randf -1 1))

* (nz) 0.5
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 1:
    t = """
(define osc (
    sin (* twopi (~ $1))
    )
)

(define lfo (
    + (* (osc $1) (- $2 1)) $2
    )
)

tanh (*  (+ (osc 400) (osc 300)) (lfo .25 10))

"""
    ex = Expr(Sig(0), t).out()
elif TEST == 2:
    t = """
(define osc (
        sin (* twopi (~ $1))
    )
)

(define fm (
        (let #1 $1) // carrier
        (let #2 $2) // ratio
        (let #3 (+ (* (osc $3) 5) 5)) // lfo on the index
        (osc (+ #1 (* (osc (* #1 #2)) (* #1 (* #2 #3)))))
    )
)

* (+ (fm 172 0.251 .1)
     (fm 86 0.499 .15)) 
  0.5

"""
    ex = Expr(Sig(0), t).out()
elif TEST == 3:
    t = """
(define hp (
        (let #1 (exp (/ (* (neg twopi) $2) sr)))
        (let #2 (* (+ #1 1) 0.5))
        rpole (rzero (* $1 #2) #2) #1
    )
)

(let #freq 1000)

(hp (hp (randf -0.5 0.5) #freq) #freq)

"""
    ex = Expr(Sig(0), t).out()

ex.editor()
sc = Scope(ex)
sp = Spectrum(ex)

s.gui(locals())
