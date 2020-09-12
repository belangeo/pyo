"""
Loading functions from an external file

**08-load-function.py**

With the `load` function, the user can use functions defined in
an external file within its script. This makes possible to create
frequently used custom functions only once and use them as often
as desired. The syntax is:

.. code-block:: scheme

    (load path/to/the/file)
 
The content of the file will be inserted where the load function
is called and all functions defined inside the file will then be
accessible. The path can be absolute or relative to the current
working directory.

See the content of those files for the function implementations.

- `utils.expr`_
- `filters.expr`_
- `generators.expr`_

.. _utils.expr: https://github.com/belangeo/pyo/blob/master/pyo/examples/23-expression/utils.expr
.. _filters.expr: https://github.com/belangeo/pyo/blob/master/pyo/examples/23-expression/filters.expr
.. _generators.expr: https://github.com/belangeo/pyo/blob/master/pyo/examples/23-expression/generators.expr

"""
from pyo import *

s = Server().boot()

expression = """
(load utils.expr)       // scalef
(load filters.expr)     // peak
(load generators.expr)  // square

// This expression uses functions defined in the loaded files.
(peak (* (square 172 0) 0.2) (scalef (osc 0.2) 1000 5000) 0.9)

"""

expr = Expr(Sig(0), expression, mul=0.5)
expr.editor()

sc = Scope(expr)
sp = Spectrum(expr)

pan = Pan(expr).out()

s.gui(locals())
