from pyo import *

s = Server().boot()
s.amp = 0.1

TEST = 0

if TEST == 0:
    t = """
// #f1 and #f2 are changed from python side
(var #f1 500)
(var #f2 750)
(define osc (
        sin (* (~ $1) twopi)
    )
)

(* (+ (osc #f1) 
      (osc #f2))
   0.2)

"""
    lfo = Sine([.1,.15], mul=300, add=700)
    ex = Expr(Sig(0), t).out()

    def change():
        ex.setVar(["#f1", "#f2"], lfo.get(all=True))

    pat = Pattern(change, 0.02).play()
elif TEST == 1:
    t = """
(var #feed 0.5)
(var #pitch 200)
(define oscloop (
        (let #xsin
            (sin (+ (* (~ $1) twopi) (* #xsin $2))) // #xsin used before...
        ) // ... "let" statement finished!
        #xsin // oscloop function outputs #xsin variable
    )
)

(oscloop #pitch #feed)
"""
    ex = Expr(Sig(0), t).out()

    lfo = Sig(0.5)
    lfo.ctrl([SLMap(0, 1, "lin", "value", 0.5)], title="Feedback")
    pit = Sig(200)
    pit.ctrl([SLMap(100, 1000, "log", "value", 200)], title="Frequency")

    def change():
        ex.setVar(["#feed", "#pitch"], [lfo.get(), pit.get()])

    pat = Pattern(change, 0.02).play()

ex.editor()
sc = Scope(ex)

s.gui(locals())