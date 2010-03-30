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

######################################################################
### Matrix
######################################################################                                       
class NewMatrix(PyoMatrixObject):
    """
    Create an empty table ready for recording. 
    
    See `TableRec` to write samples in the table.
    
    Parent class: PyoTableObject
    
    Parameters:
    
    length : float
        Length of the table in seconds.
    chnls : int, optional
        Number of channels that will be handled by the table. 
        Defaults to 1.
        
    Methods:    
    
    getSize() : Returns the length of the table in samples.
    getLength() : Returns the length of the table in seconds.
    getRate() : Returns the frequency (cycle per second) to give 
        to an oscillator to read the sound at its original pitch.

    See also: TableRec

    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = NewTable(length=2, chnls=1)
    >>> a = Input(0)
    >>> b = TableRec(a, t, .01)
    >>> c = Osc(t, [t.getRate(), t.getRate()*.99]).out()
    >>> # to record in the empty table, call:
    >>> # b.play()

    """
    def __init__(self, rows, cols, init=None):
        if init == None:
            self._base_objs = [NewMatrix_base(rows, cols)]
        else:
            self._base_objs = [NewMatrix_base(rows, cols, init)]
            
    def __dir__(self):
        return []
     
    def getSize(self):
        """
        Returns the length of the table in samples.
        
        """
        return self._base_objs[0].getSize()
          
    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an 
        oscillator to read the sound at its original pitch.
        
        """
        return self._base_objs[0].getRate()

