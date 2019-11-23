"""
Manipulating audio signal in input

**02-audio-in.py**

The expression illustrated in this example use a phasor signal
in input as the running phase of a self-modulated sine wave.

Here is the complete expression:

.. code-block:: scheme

    (sin (+ (* twopi $x[0]) (* $y[-1] 0.8)))

From inner to outer expression, we have:

.. code-block:: scheme

    (* $y[-1] 0.8)

Where `$y[-1]` is the last output sample multiplied by a feedback
factor (0.8). Then we have the running phase, `$x[0]` is the current
input sample, rescaled to the range 0 -> 2pi for the `sin` function:

.. code-block:: scheme

    (* twopi $x[0])

We add the output delay to the running phase:

.. code-block:: scheme

    (+ (* twopi $x[0]) (* $y[-1] 0.8))

This gives the modulated running phase driving the `sin` function:

.. code-block:: scheme

    (sin (+ (* twopi $x[0]) (* $y[-1] 0.8)))

Complete example
----------------
"""
from pyo import *

s = Server().boot()

expression = """
// Self-modulated sine wave with running phase given in input.
// Try different feedback factors between 0 and 1. 
(sin (+ (* twopi $x[0]) (* $y[-1] 0.8)))
"""

# External signal used as the running phase.
input = Phasor(86)
input.ctrl()

# Create the Expr object and show its expression editor. To
# re-evaluate the expression hit Ctrl+Enter when the editor
# has the focus.
expr = Expr(input, expression, mul=0.5)
expr.editor()

# Shows the generated signal.
sc = Scope(expr)

# Converts the mono signal to stereo and sends it to the soundcard.
pan = Pan(expr).out()

s.gui(locals())
