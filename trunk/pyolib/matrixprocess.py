"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from _core import *
from types import SliceType

class MatrixPointer(PyoObject):
    """
    Table reader with control on the pointer position.
    
    Parent class: PyoObject
    
    Parameters:
    
    table : PyoTableObject
        Table containing the waveform samples.
    index : PyoObject
        Normalized position in the table between 0 and 1.
        
    Methods:

    setTable(x) : Replace the `table` attribute.
    setIndex(x) : Replace the `index` attribute.

    Attributes:
    
    table : PyoTableObject. Table containing the waveform samples.
    index : PyoObject. Pointer position in the table.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = SndTable('pyodemos/transparent.aif')
    >>> p = Phasor(freq=t.getRate())
    >>> a = Pointer(table=t, index=p).out()

    """
    def __init__(self, matrix, indexrow, indexcol, mul=1, add=0):
        self._matrix = matrix
        self._indexrow = indexrow
        self._indexcol = indexcol
        self._mul = mul
        self._add = add
        matrix, indexrow, indexcol, mul, add, lmax = convertArgsToLists(matrix, indexrow, indexcol, mul, add)
        print matrix
        self._base_objs = [MatrixPointer_base(wrap(matrix,i), wrap(indexrow,i), wrap(indexcol,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['matrix', 'indexrow', 'indexcol', 'mul', 'add']

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.
        
        Parameters:

        x : PyoTableObject
            new `matrix` attribute.
        
        """
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setIndexRow(self, x):
        """
        Replace the `indexrow` attribute.
        
        Parameters:

        x : PyoObject
            new `indexrow` attribute.
        
        """
        self._indexrow = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndexRow(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setIndexCol(self, x):
        """
        Replace the `indexcol` attribute.
        
        Parameters:

        x : PyoObject
            new `indexcol` attribute.
        
        """
        self._indexcol = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndexCol(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title)

    @property
    def matrix(self):
        """PyoMatrixObject. Matrix containing the samples.""" 
        return self._matrix
    @matrix.setter
    def matrix(self, x): self.setMatrix(x)

    @property
    def indexrow(self):
        """PyoObject. Row index pointer position in the matrix.""" 
        return self._indexrow
    @indexrow.setter
    def indexrow(self, x): self.setIndexRow(x)

    @property
    def indexcol(self):
        """PyoObject. Column index pointer position in the matrix.""" 
        return self._indexcol
    @indexcol.setter
    def indexcol(self, x): self.setIndexCol(x)
