"""
PyoObjects to perform operations on PyoMatrixObjects.

PyoMatrixObjects are 2 dimensions table containers. They can be used
to store audio samples or algorithmic sequences. Writing and reading
are done by giving row and column positions.

"""

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
from types import SliceType

class MatrixRec(PyoObject):
    """
    MatrixRec is for writing samples into a previously created NewMatrix.
     
    See `NewMatrix` to create an empty matrix.

    The play method is not called at the object creation time. It starts
    the recording into the matrix, row after row, until the matrix is full. 
    Calling the play method again restarts the recording and overwrites 
    previously recorded samples.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal to write in the matrix.
    matrix : PyoMatrixObject
        The matrix where to write samples.
    fadetime : float, optional
        Fade time at the beginning and the end of the recording 
        in seconds. Defaults to 0.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMatrix(x) : Replace the `matrix` attribute.
    play() : Start the recording at the beginning of the matrix.
    stop() : Stop the recording. Otherwise, record through the 
        end of the matrix.

    Attributes:
    
    input : PyoObject. Audio signal to write in the matrix.
    matrix : PyoMatrixObject. The matrix where to write samples.
    
    Notes:

    The out() method is bypassed. MatrixRec returns no signal.
    
    MatrixRec has no `mul` and `add` attributes.
    
    MatrixRec will sends a trigger signal at the end of the recording. 
    User can retreive the trigger streams by calling obj['trig']. See
    `TableRec` documentation for an example.

    See also: NewMatrix
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> fmind = Sine(.2, 0, 2, 2.5)
    >>> fmrat = Sine(.33, 0, .05, .5)
    >>> aa = FM(carrier=10, ratio=fmrat, index=fmind)
    >>> rec = MatrixRec(aa, mm, 0).play()
    >>> lfx = Sine(.1, 0, .24, .25)
    >>> lfy = Sine(.15, 0, .124, .25)
    >>> x = Sine(1000, 0, lfx, .5)
    >>> y = Sine(1.5, 0, lfy, .5)
    >>> c = MatrixPointer(mm, x, y, .5).out()
    
    """
    def __init__(self, input, matrix, fadetime=0):
        PyoObject.__init__(self)
        self._input = input
        self._matrix = matrix
        self._in_fader = InputFader(input)
        in_fader, matrix, fadetime, lmax = convertArgsToLists(self._in_fader, matrix, fadetime)
        self._base_objs = [MatrixRec_base(wrap(in_fader,i), wrap(matrix,i), wrap(fadetime,i)) for i in range(len(matrix))]
        self._trig_objs = [MatrixRecTrig_base(obj) for obj in self._base_objs]

    def __dir__(self):
        return ['input', 'matrix', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._trig_objs:
            obj.deleteStream()
            del obj

    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs
        
        if type(i) == SliceType:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        self._trig_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._trig_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMatrix(self, x):
        """
        Replace the `matrix` attribute.
        
        Parameters:

        x : NewMatrix
            new `matrix` attribute.
        
        """
        self._matrix = x
        x, lmax = convertArgsToLists(x)
        [obj.setMatrix(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Audio signal to write in the matrix.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def matrix(self):
        """PyoMatrixObject. The matrix where to write samples."""
        return self._matrix
    @matrix.setter
    def matrix(self, x): self.setMatrix(x)

class MatrixPointer(PyoObject):
    """
    Matrix reader with control on the 2D pointer position.
    
    Parent class: PyoObject
    
    Parameters:
    
    matrix : PyoMatrixObject
        Matrix containing the waveform samples.
    x : PyoObject
        Normalized X position in the matrix between 0 and 1.
    y : PyoObject
        Normalized Y position in the matrix between 0 and 1.
        
    Methods:

    setMatrix(x) : Replace the `matrix` attribute.
    setX(x) : Replace the `x` attribute.
    setY(x) : Replace the `y` attribute

    Attributes:
    
    matrix : PyoMatrixObject. Matrix containing the waveform samples.
    x : PyoObject. X pointer position in the matrix.
    y : PyoObject. Y pointer position in the matrix.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> SIZE = 256
    >>> mm = NewMatrix(SIZE, SIZE)
    >>> fmind = Sine(.2, 0, 2, 2.5)
    >>> fmrat = Sine(.33, 0, .05, .5)
    >>> aa = FM(carrier=10, ratio=fmrat, index=fmind)
    >>> rec = MatrixRec(aa, mm, 0).play()
    >>> lfx = Sine(.1, 0, .24, .25)
    >>> lfy = Sine(.15, 0, .124, .25)
    >>> x = Sine(1000, 0, lfx, .5)
    >>> y = Sine(1.5, 0, lfy, .5)
    >>> c = MatrixPointer(mm, x, y, .5).out()

    """
    def __init__(self, matrix, x, y, mul=1, add=0):
        PyoObject.__init__(self)
        self._matrix = matrix
        self._x = x
        self._y = y
        self._mul = mul
        self._add = add
        matrix, x, y, mul, add, lmax = convertArgsToLists(matrix, x, y, mul, add)
        self._base_objs = [MatrixPointer_base(wrap(matrix,i), wrap(x,i), wrap(y,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['matrix', 'x', 'y', 'mul', 'add']

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

    def setX(self, x):
        """
        Replace the `x` attribute.
        
        Parameters:

        x : PyoObject
            new `x` attribute.
        
        """
        self._x = x
        x, lmax = convertArgsToLists(x)
        [obj.setX(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setY(self, x):
        """
        Replace the `y` attribute.
        
        Parameters:

        y : PyoObject
            new `y` attribute.
        
        """
        self._y = x
        x, lmax = convertArgsToLists(x)
        [obj.setY(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def matrix(self):
        """PyoMatrixObject. Matrix containing the samples.""" 
        return self._matrix
    @matrix.setter
    def matrix(self, x): self.setMatrix(x)

    @property
    def x(self):
        """PyoObject. Normalized X position in the matrix.""" 
        return self._x
    @x.setter
    def x(self, x): self.setX(x)

    @property
    def y(self):
        """PyoObject. Normalized Y position in the matrix.""" 
        return self._y
    @y.setter
    def y(self, x): self.setY(x)
