"""
Tools to perform arithmetic operations on audio signals.

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

class Sin(PyoObject):
    """
    Performs a sine function on audio signal.

    Returns the sine of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = Sin(a).out()

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Sin_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Cos(PyoObject):
    """
    Performs a cosine function on audio signal.

    Returns the cosine of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = Cos(a).out()

    """
    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Cos_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Tan(PyoObject):
    """
    Performs a tangent function on audio signal.

    Returns the tangent of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Osc(t, 1, 0, .5, .5)
    >>> b = Tan(a)
    >>> c = Osc(t, 100, mul=b).out()

    """
    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Tan_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
