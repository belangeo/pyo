"""
Creating our own functions

**05-define-function.py**

The define keyword starts the definition of a custom function. The syntax is:

.. code-block:: scheme

    (define funcname (body))

`funcname` is the name used to call the function in the expression and body
is the sequence of expressions to execute. Arguments of the function are
extracted directly from the body. They must be named $1, $2, $3, …, $9.

The following example defines two functions, a simple oscillator and a
frequency modulation function using the former one.

"""
from pyo import *

s = Server().boot()

expression = """
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

expr = Expr(Sig(0), expression, mul=0.5)
expr.editor()

sc = Scope(expr)

pan = Pan(expr).out()

s.gui(locals())
