"""
Controlling user variables in an expression from python

**06-var-function.py**

User variables are created with the keyword `var`:

.. code-block:: scheme

    (var #var (init))

The variable name must begin with a `#`.

They are computed only at initialization, but can be changed from the 
python script with method calls (`varname` is a string and `value` is a 
float)::

    obj.setVar(varname, value)

The following example illustrates a self-modulated oscillator of which
the frequency and the feedback are controlled from the python side with
method calls.

"""
from pyo import *

s = Server().boot()

expression = """
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

expr = Expr(Sig(0), expression, mul=0.5)
expr.editor()

# Shows two sliders, one for the frequency and one for the feedback parameter.
pit = Sig(200)
pit.ctrl([SLMap(100, 1000, "log", "value", 200)], title="Frequency")
feed = Sig(0.5)
feed.ctrl([SLMap(0, 1, "lin", "value", 0.5)], title="Feedback")


def change():
    "Sends new values to user variables in the expression."
    expr.setVar(["#pitch", "#feed"], [pit.get(), feed.get()])


# Calls the change() function every 20 ms to update the user variables.
pat = Pattern(change, 0.025).play()

sc = Scope(expr)

pan = Pan(expr).out()

s.gui(locals())
