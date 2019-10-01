from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    t = """
(let #freq 500)
(let #nz (randf -1 1))
(let #coeff (
    ^ e (/ (* (neg twopi) #freq) 44100)
    )
)
+ #nz (* (- $y[-1] #nz) #coeff)
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 1:
    t = """
(let #freq 200)
(let #feed 0.7)
(let #xsin
    (sin (+ (* (~ #freq) twopi) (* #xsin #feed))) // #xsin used before...
) // ... "let" statement finished!
#xsin // outputs #xsin variable
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 2:
    t = """
///////////////////
// PAF generator //
///////////////////
(let #fund 100)
(let #band 300)
(let #form 600)
(let #bw (/ #band #fund))
(let #cf (/ #form #fund))
(let #ph (* (- (~ #fund) 0.5) twopi))
(let #car (
    (cos (max (min (* #ph #bw) pi) (- 0 pi)))
    )
)
(let #mod (
    (let #oct (floor #cf))
    (let #frac (% #cf 1))
    (+ (* (cos (* #ph #oct)) (- 1 #frac))
       (* (cos (* #ph (+ #oct 1))) #frac)
       )
    )
)
* (* #car #mod) 0.5
"""
    ex = Expr(Sig(0), t).out()
elif TEST == 3:
    t = """
// global #coef variable
(let #coef 500)

(define osc (
        sin (* twopi (~ $1))
    )
)

(define lp (
        // local #coef variable
        (let #coef (exp (/ (* (neg twopi) $2) sr)))
        rpole (* $1 (- 1 #coef)) #coef
    )
)

+ (lp (randf -0.5 0.5) 1000)
  (* (osc #coef) 0.25)

"""
    ex = Expr(Sig(0), t).out()
elif TEST == 4:
    t = """
(let #freq 500) // global #freq variable
(let #spread 1)

(define osc (
        // local #freq variable
        (let #freq (* $2 (^ $1 #spread)))
        sin (* twopi (~ #freq))
    )
)

(* (+ (osc 1 #freq) (osc 2 #freq)) 0.5)

"""
    ex = Expr(Sig(0), t).out()

ex.editor()
sc = Scope(ex)
sp = Spectrum(ex)

s.gui(locals())