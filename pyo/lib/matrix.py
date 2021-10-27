"""
Copyright 2009-2015 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from ._core import *
from ._maps import *

######################################################################
### Matrix
######################################################################
class NewMatrix(PyoMatrixObject):
    """
    Create a new matrix ready for recording.

    Optionally, the matrix can be filled with the contents of the
    `init` parameter.

    See :py:class:`MatrixRec` to write samples in the matrix.

    :Parent: :py:class:`PyoMatrixObject`

    :Args:

        width: int
            Desired matrix width in samples.
        height: int
            Desired matrix height in samples.
        init: list of list of floats, optional
            Initial matrix. Defaults to None.

    .. seealso::

        :py:class:`MatrixRec`

    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> mm.genSineTerrain(freq=2, phase=16)
    >>> lfw = Sine([.1,.11], 0, .124, .25)
    >>> lfh = Sine([.15,.16], 0, .124, .25)
    >>> w = Sine(100, 0, lfw, .5)
    >>> h = Sine(10.5, 0, lfh, .5)
    >>> c = MatrixPointer(mm, w, h, mul=.2).out()

    """

    def __init__(self, width, height, init=None):
        pyoArgsAssert(self, "IIL", width, height, init)
        PyoMatrixObject.__init__(self)
        self._size = (width, height)
        if init is None:
            self._base_objs = [NewMatrix_base(width, height)]
        else:
            self._base_objs = [NewMatrix_base(width, height, init)]

    def replace(self, x):
        """
        Replaces the actual matrix.

        :Args:

            x: list of list of floats
                New matrix. Must be of the same size as the actual matrix.

        """
        pyoArgsAssert(self, "l", x)
        [obj.setMatrix(x) for obj in self._base_objs]
        self.refreshView()

    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an
        oscillator to read the sound at its original pitch.

        """
        return self._base_objs[0].getRate()

    def genSineTerrain(self, freq=1, phase=0.0625):
        """
        Generates a modulated sinusoidal terrain.

        :Args:

            freq: float
                Frequency of sinusoids used to created the terrain.
                Defaults to 1.
            phase: float
                Phase deviation between rows of the terrain. Should be in
                the range 0 -> 1. Defaults to 0.0625.

        """
        pyoArgsAssert(self, "NN", freq, phase)
        [obj.genSineTerrain(freq, phase) for obj in self._base_objs]
        self.refreshView()
