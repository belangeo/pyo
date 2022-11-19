Matrix Processing
===================================

.. currentmodule:: pyo

PyoObjects to perform operations on PyoMatrixObjects.

PyoMatrixObjects are 2 dimensions table containers. They can be used
to store audio samples or algorithmic sequences. Writing and reading
are done by giving row and column positions.

Objects in this category
------------------------------

- :py:class:`MatrixMorph` :     Morphs between multiple PyoMatrixObjects.
- :py:class:`MatrixPointer` :     Matrix reader with control on the 2D pointer position.
- :py:class:`MatrixRec` :     MatrixRec records samples into a previously created NewMatrix.
- :py:class:`MatrixRecLoop` :     MatrixRecLoop records samples in loop into a previously created NewMatrix.

*MatrixMorph*
-----------------------------------

.. autoclass:: MatrixMorph
   :members:

   .. autoclasstoc::

*MatrixPointer*
-----------------------------------

.. autoclass:: MatrixPointer
   :members:

   .. autoclasstoc::

*MatrixRec*
-----------------------------------

.. autoclass:: MatrixRec
   :members:

   .. autoclasstoc::

*MatrixRecLoop*
-----------------------------------

.. autoclass:: MatrixRecLoop
   :members:

   .. autoclasstoc::

