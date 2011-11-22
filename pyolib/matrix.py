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
from _maps import *

######################################################################
### Matrix
######################################################################
class NewMatrix(PyoMatrixObject):
    """
    Create a new matrix ready for recording. 

    Optionally, the matrix can be filled with the contents of the 
    `init` parameter. 
    
    See `MatrixRec` to write samples in the matrix.
    
    Parent class: PyoMatrixObject
    
    Parameters:
    
    width : int
        Desired matrix width in samples.
    height : int
        Desired matrix height in samples.
    init : list of list of floats, optional
        Initial matrix. Defaults to None.
        
    Methods:    
    
    replace() : Replaces the actual matrix.
    getRate() : Returns the frequency (cycle per second) to give 
        to an oscillator to read a row at its original pitch.
    genSineTerrain(freq, phase) : Generates a modulated sinusoidal terrain.
    
    See also: MatrixRec

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> fmind = Sine(.2, 0, 2, 2.5)
    >>> fmrat = Sine(.33, 0, .05, .5)
    >>> aa = FM(carrier=250, ratio=fmrat, index=fmind)
    >>> rec = MatrixRec(aa, mm, 0).play()
    >>> lfw = Sine([.1,.11], 0, .124, .25)
    >>> lfh = Sine([.15,.16], 0, .124, .25)
    >>> w = Sine(10, 0, lfw, .5)
    >>> h = Sine(1.5, 0, lfh, .5)
    >>> c = MatrixPointer(mm, w, h, mul=.3).out()

    """
    def __init__(self, width, height, init=None):
        PyoMatrixObject.__init__(self)
        self._size = (width, height)
        if init == None:
            self._base_objs = [NewMatrix_base(width, height)]
        else:
            self._base_objs = [NewMatrix_base(width, height, init)]
            
    def __dir__(self):
        return []

    def replace(self, x):
        """
        Replaces the actual matrix.
        
        Parameters:
        
        x : list of list of floats
            New matrix. Must be of the same size as the actual matrix.

        """
        [obj.setMatrix(x) for obj in self._base_objs]
         
    def getRate(self):
        """
        Returns the frequency (cycle per second) to give to an 
        oscillator to read the sound at its original pitch.
        
        """
        return self._base_objs[0].getRate()

    def genSineTerrain(self, freq=1, phase=0.0625):
        """
        Generates a modulated sinusoidal terrain.

        Parameters:

        freq : float
            Frequency of sinusoids used to created the terrain.
            Defaults to 1.
        phase : float
            Phase deviation between rows of the terrain. Should be in
            the range 0 -> 1. Defaults to 0.0625.

        """
        [obj.genSineTerrain(freq, phase) for obj in self._base_objs]

