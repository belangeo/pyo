"""
Using the conditional function

**07-condition.py**

The `if` function let the user set different processing branches
according to the result of a condition. The syntax is:

.. code-block:: scheme

    (if (condition) (then) (else))

The example below uses two conditional statements. The first one
adjusts the duty cycle of a pulse-width-modulation waveform and 
the second one split the positive and negative parts of the wave.
A slider, controlling a user variable, sets the gain of the
negative part. 

"""
from pyo import *

s = Server().boot()

expression = """
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

expr = Expr(Sig(0), expression, mul=0.5)
expr.editor()

threshold = Sig(0.5)
threshold.ctrl([SLMap(0.01, 0.5, "lin", "value", 0.5)], title="Duty Cycle")
rectifier = Sig(1.0)
rectifier.ctrl([SLMap(0.0, 1.0, "lin", "value", 1.0)], title="Rectifier")


def change():
    "Sends new values to user variables in the expression."
    expr.setVar(["#thresh", "#gain"], [threshold.get(), rectifier.get()])


# Calls the change() function every 20 ms to update the user variables.
pat = Pattern(change, 0.025).play()

sc = Scope(expr)

pan = Pan(expr).out()

s.gui(locals())
