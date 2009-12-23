:class:`PyoTableObject` --- Base class for all pyo table objects
================================================================

.. class:: PyoTableObject()

    Base class for all pyo table objects. A table object is a buffer memory
    to store precomputed samples.

    The user should never instantiate an object of this class.

.. method:: PyoTableObject.getSize()

    Return table size in samples.

.. note::

    Operations allowed on all table objects :

    len(obj) : Return the number of table streams in an object.

    obj[x] : Return table stream `x` of the object. `x` is a number from 0 to len(obj) - 1.


**Child objects**

.. toctree::
    :glob:
    
    *
