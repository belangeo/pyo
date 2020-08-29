"""
State variable inside an expression

**04-let-function.py**

User can create state variable with the keyword `let`. This is
useful to set an intermediate state to be used in multiple places
in the processing chain. The syntax is:

.. code-block:: scheme

    (let #var (body))

The variable name must begin with a `#`.

The prefix expression `documentation`_ gives common use cases where we
may want to use state variables.

.. _documentation: http://ajaxsoundstudio.com/pyodoc/api/classes/expression.html

The following example implements a phase-aligned formant (PAF) generator, as 
described by Miller Puckette in "The Theory and Technique of Electronic
Music", p158.

"""
from pyo import *

s = Server().boot()

expression = """// PAF generator (single formant) //
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

expr = Expr(Sig(0), expression, mul=0.5)
expr.editor()

sp = Spectrum(expr)

pan = Pan(expr).out()

s.gui(locals())
