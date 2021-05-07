# Run this file in valgrind with:
#   PYTHONMALLOC=malloc valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=definite --track-origins=yes --num-callers=12 --suppressions=valgrind-python.supp python3 test_memleak_dsl.py 
# There should not be any definitely lost bytes.

###
### Expr is still leaking a bit...
###

import os
os.environ["PYO_GUI_WX"] = "0"

from pyo import *

s = Server(audio="manual").boot().start()

e1 = "sin (* twopi (~ (sah (* (randi 5 10) 100) (~ 4 0)) 0))"
a1 = Expr(Sig(0), e1).out()

e2 = """
// sum of two sine waves with controlled gain.
* (+ (sin (* twopi (~ 500)))
     (sin (* twopi (~ 600))))
  0.25
"""
a2 = Expr(Sig(0), e2).out()

e3 = """
// Self-modulated sine wave with running phase given in input.
// Try different feedback factors between 0 and 1. 
(sin (+ (* twopi $x[0]) (* $y[-1] 0.8)))
"""
a3 = Expr(Phasor(86), e3).out()

s1 = Sine(freq=10.7647)
s2 = Sine(freq=10.0)
s3 = Sine(freq=87.9023)
s4 = Sine(freq=296.4831)

e4 = """
// Four signals ring-modulation.
(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])
"""
a4 = Expr([s1, s2, s3, s4], e4).out()

e5 = """// PAF generator (single formant) //
// User variables
(let #fund 100) // fundamental frequency
(let #band 600) // formant bandwidth
(let #form 600) // formant center frequency
// Intermediate variables
(let #bw (/ #band #fund))
(let #cf (/ #form #fund))
(let #ph (* (- (~ #fund) 0.5) twopi))
(let #car ( // Carrier
    (cos (max (min (* #ph #bw) pi) (- 0 pi)))
    )
)
(let #mod ( // Modulator
    (let #oct (floor #cf))
    (let #frac (% #cf 1))
    (+ (* (cos (* #ph #oct)) (- 1 #frac))
       (* (cos (* #ph (+ #oct 1))) #frac)
       )
    )
)
* (* #car #mod) 0.5
"""
a5 = Expr(Sig(0), e5).out()

e6 = """
// usage: (osc freq)
(define osc (
        sin (* twopi (~ $1))
    )
)

// usage: (fm carrier ratio lfo_freq)
(define fm (
        (let #car $1) // carrier
        (let #rat $2) // ratio
        (let #ind (+ (* (osc $3) 5) 5)) // lfo on the index
        (osc (+ #car (* (osc (* #car #rat)) (* #car (* #rat #ind)))))
    )
)

// Two FM generators.
* (+ (fm 172 0.251 .1)
     (fm 86 0.499 .15)) 
  0.5
"""
a6 = Expr(Sig(0), e6).out()

e7 = """
(var #pitch 200)
(var #feed 0.5)
(define oscloop (
        (let #xsin
            (sin (+ (* twopi (~ $1)) (* #xsin $2))) // #xsin used before...
        ) // ... "let" statement finished = one-delay feedback loop!
        #xsin // oscloop function outputs #xsin variable
    )
)

(oscloop #pitch #feed)
"""
a7 = Expr(Sig(0), e7).out()

# range method causes leaks...
pit = Sine(0.1)#.range(200, 300)
feed = Sine(0.5)#.range(0.3, 0.6)

def change():
    "Sends new values to user variables in the expression."
    a7.setVar(["#pitch", "#feed"], [pit.get(), feed.get()])

pat = Pattern(change, 0.001).play()


e8 = """
// Width of the duty cycle.
(var #thresh 0.5)
// Gain of the negative part of the waveform.
(var #gain 1)

// Running phase.
(let #sig (~ 172))

// If running phase is below threshold,
// then 1.0, else -1.0
(let #rect (
    if (<= #sig #thresh) (1.0) (-1.0)
    )
)

// If positive, pass through, else,
// modulated by #gain variable. 
(if (>= #rect 0)
    (#rect) 
    (* #gain #rect)
)
"""
a8 = Expr(Sig(0), e8).out()

e9 = """
(load ../../pyo/examples/23-expression/utils.expr)       // scalef
(load ../../pyo/examples/23-expression/filters.expr)     // peak
(load ../../pyo/examples/23-expression/generators.expr)  // square

// This expression uses functions defined in the loaded files.
(peak (* (square 172 0) 0.2) (scalef (osc 0.2) 1000 5000) 0.9)
"""
a9 = Expr(Sig(0), e9).out()


e10 = """// Lorenz strange attractor.
// Control variables
(let #pit 500)      // 1.0 -> 750.0
(let #chaos 2.0)    // 0.5 -> 3.0
// Initial constants
(let #A 10.0)
(let #B 28.0)

// Computes the differential variables
(let #delta (* (/ 1.0 sr) #pit))
(let #vDX (* (- $y1[-1] $y0[-1]) #A))
(let #vDY (- (* $y0[-1] (- #B $y2[-1])) $y1[-1]))
(let #vDZ (- (* $y0[-1] $y1[-1]) (* #chaos $y2[-1])))

// Three differential equations (the first two are the
// desired audio signals).
(out 0 (+ $y0[-1] (* #vDX #delta)))
(out 1 (+ $y1[-1] (* #vDY #delta)))
(out 2 (+ $y2[-1] (* #vDZ #delta)))
"""
a10 = Expr(Sig(0), e10, outs=3, initout=1.0, mul=[0.044, 0.0328, 0.0]).out()

song1 = "#0 t600 o12 v80 x0.5 y0.5 z0.5 c3 r5 ?[c e g b-]1 |: v?{40 70} x+0.01 y+0.01 z+0.01 v+20 o+2 t-2 d-2 r3 d+6 (c d e)5 v-20 x-0.01 y-0.01 z-0.01 :|12 z?{0.1 0.2}"

mml1 = MML(song1, voices=1, loop=True, poly=1, updateAtEnd=False).play()
env1 = CosTable([(0,0), (8,1), (1024,1), (8000, 1.0), (8191,0)])
tr1 = TrigEnv(mml1, table=env1, dur=mml1['dur'], mul=mml1["amp"])
osc1 = SineLoop(freq=mml1["freq"], feedback=mml1['x'], mul=tr1)

s.process()
s.process()
s.process()
s.process()

a1.setExpr(e2)
a2.setExpr(e1)

s.process()
s.process()
s.process()
s.process()
s.stop()
s.shutdown()
