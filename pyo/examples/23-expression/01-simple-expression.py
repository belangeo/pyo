"""
Introduction to prefix expression evaluator

**01-simple-expression.py**

The Expr object implements a tiny functional programming language
that can be used to write synthesis or signal processing algorithms.

The language uses prefix notation (also known as "Polish notation")
to express audio algorithms. In prefix notation, Operators are written 
before their operands, between braces. To add two number together, we
will write:

.. code-block:: scheme

    (+ 2 2)

If we want to multiply by 10 the result of this addition, then:

.. code-block:: scheme

    (* (+ 2 2) 10)

And so on, it's how we build our algorithm. To generate a sine wave, we do:

.. code-block:: scheme

    (sin (* twopi (~ 440)))
 
The `~` operator generates a ramp from 0 to 1 at a frequency given to its first
argument.

If the object generates only one channel output (the default), the last expression
in the script is the output signal. Otherwise, output signals must be created with 
the `out` function (we will cover this topic in a later example).

For the detail of all available functions, see the prefix expression
`documentation`_.

.. _documentation: http://ajaxsoundstudio.com/pyodoc/api/classes/expression.html

Here is a program that use the Expr object, with its own editor, to generate two
sine waves with frequency and gain control.

"""
from pyo import *

s = Server().boot()

# The expression to evaluate. It produces two sine waves, of
# frequency 500 and 600 Hz, with an amplitude of -12 dB.
# The ~ operator generates a ramp from 0 to 1 at a frequency
# given to its first argument.
expression = """
// sum of two sine waves with controlled gain.
* (+ (sin (* twopi (~ 500)))
     (sin (* twopi (~ 600))))
  0.25
"""

# We dont use any input with this expression (synthesis only),
# so we pass a signal filled with zeros.
input = Sig(0)

# Create the Expr object and show its expression editor. To
# re-evaluate the expression hit Ctrl+Enter when the editor
# has the focus.
expr = Expr(input, expression)
expr.editor()

# Shows the generated signal.
sc = Scope(expr)

# Converts the mono signal to stereo and sends it to the soundcard.
pan = Pan(expr).out()

s.gui(locals())
