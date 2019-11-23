"""
Using more than one input signal in a single expression

**03-multiple-inputs.py**

It is possible to give a list of audio signals to the `input`
argument if you need more than one signal in your expression.

You can either pass a list of audio objects (as in the complete
example below) or a single multi-streams object, as in::

    sig = Sine(freq=[10.7647, 10.0000, 87.9023, 296.4831])
    ex = Expr(sig, "(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])").out()

If multiple signals are used, the position in the list must be
given between the `$x` and the sample position `[0]`. So, for the
second audio signal, one would retrieve it with `$x1[0]`. The 0
for the first signal can be ommited, `$x[0]` is the same as `$x0[0]`.

Complete example
----------------
"""
from pyo import *

s = Server().boot()

expression = """
// Four signals ring-modulation.
(* (* (* $x[0] $x1[0]) $x2[0]) $x3[0])
"""

# Multiple input signals.
s1 = Sine(freq=10.7647)
s2 = Sine(freq=10.0)
s3 = Sine(freq=87.9023)
s4 = Sine(freq=296.4831)

# Create the Expr object and show its expression editor. To
# re-evaluate the expression hit Ctrl+Enter when the editor
# has the focus.
expr = Expr([s1, s2, s3, s4], expression, mul=0.5)
expr.editor()

# Shows the generated signal.
sc = Scope(expr)

# Converts the mono signal to stereo and sends it to the soundcard.
pan = Pan(expr).out()

s.gui(locals())
