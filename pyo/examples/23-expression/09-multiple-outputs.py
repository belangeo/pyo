"""
How to send multiple outputs from a single expression

**09-multiple-outputs.py**

If the object generates only one channel output (the default), the last
expression in the script is the output signal. Otherwise, output signals
must be created with the `out` function.

Here is an example of a script that output four sine waves to four
different output channels.

.. code-block:: scheme

    (define osc (sin (* twopi (~ $1))))
    (out 0 (* (osc 250) 0.2))
    (out 1 (* (osc 500) 0.2))
    (out 2 (* (osc 750) 0.2))
    (out 3 (* (osc 1000) 0.2))

We retrieve the different channels from the Expr object with the
bracket syntax (obj[0] is the first channel, obj[1] the second,
and so on...).

"""
from pyo import *

s = Server().boot()

expression = """// Lorenz strange attractor.
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

# We must tell the Expr object how many signals to output with the `outs` argument.
# The `initout` argument sets the initial value og output signals (defaults to 0).
expr = Expr(Sig(0), expression, outs=3, initout=1.0, mul=[0.044, 0.0328, 0.0])
expr.editor()

sc = Scope(expr)
sp = Spectrum(expr)

pan = Pan(expr, mul=0.3).out()

s.gui(locals())
