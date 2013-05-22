from pyo import *

cats = OBJECTS_TREE['PyoObjectBase']['PyoTableObject']

cats.sort()

module = """%s
===================================

.. module:: pyo


"""

template = """*%s*
-----------------------------------

.. autoclass:: %s
   :members:

"""

with open("tables.rst", "w") as f:
    f.write(module % "Tables")
    for obj in cats:
        f.write(template % (obj, obj))

