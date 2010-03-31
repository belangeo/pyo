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

class MatrixRec(PyoObject):
    """
    TableRec is for writing samples into a previously created NewTable.
     
    See `NewTable` to create an empty table.

    The play method is not called at the object creation time. It starts
    the recording into the table until the table is full. Calling the 
    play method again restarts the recording and overwrites previously
    recorded samples.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal to write in the table.
    table : PyoTableObject
        The table where to write samples.
    fadetime : float, optional
        Fade time at the beginning and the end of the recording 
        in seconds. Defaults to 0.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setTable(x) : Replace the `table` attribute.
    play() : Start the recording at the beginning of the table.
    stop() : Stop the recording. Otherwise, record through the 
        end of the table.

    Attributes:
    
    input : PyoObject. Audio signal to write in the table.
    table : PyoTableObject. The table where to write samples.
    
    Notes:

    The out() method is bypassed. TableRec returns no signal.
    
    TableRec has no `mul` and `add` attributes.
    
    TableRec will sends a trigger signal at the end of the recording. 
    User can retreive the trigger streams by calling obj['trig']. In
    this example, the recorded table will be read automatically after
    a recording:
    
    >>> a = Input(0)
    >>> t = NewTable(length=1, chnls=1)
    >>> rec = TableRec(a, table=t, fadetime=0.01)
    >>> tr = TrigEnv(rec['trig'], table=t, dur=1).out()

    See also: NewTable
    
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
    def __init__(self, input, matrix, fadetime=0):
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

    def play(self):
        self._base_objs = [obj.play() for obj in self._base_objs]
        self._trig_objs = [obj.play() for obj in self._trig_objs]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self

    def out(self, chnl=0, inc=1):
        pass

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

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Audio signal to write in the table.""" 
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
