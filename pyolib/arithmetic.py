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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

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
    >>> a = Osc(table=t, freq=1, mul=.5, add=.5)
    >>> b = Tan(a)
    >>> c = Osc(table=t, freq=100, mul=b).out()

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Abs(PyoObject):
    """
    Performs a absolute function on audio signal.

    Returns the absolute value of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> t = SndTable(SNDS_PATH + "/transparent.aif")
    >>> a = Phasor(freq=t.getRate()*0.5, mul=2, add=-1)
    >>> b = Pointer(table=t, index=Abs(a), mul=0.5).out()

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Abs_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Sqrt(PyoObject):
    """
    Performs a square-root function on audio signal.

    Returns the square-root value of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> src = Sine(mul=.5)
    >>> a = Abs(Phasor(freq=1, mul=2, add=-1))
    >>> left = Sqrt(1.0 - a)
    >>> right = Sqrt(a)
    >>> oL = src * left
    >>> oR = src * right
    >>> oL.out()
    >>> oR.out(1)

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Sqrt_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Log(PyoObject):
    """
    Performs a natural log function on audio signal.

    Returns the natural log value of input. Values less than 0.0
    return 0.0.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = RandInt(max=1000, freq=4)
    >>> b = Log(a)
    >>> c = Print(input=b, method=1)

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Log2(PyoObject):
    """
    Performs a base 2 log function on audio signal.

    Returns the base 2 log value of input. Values less than 0.0
    return 0.0.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = RandInt(max=1000, freq=4)
    >>> b = Log2(a)
    >>> c = Print(input=b, method=1)

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log2_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Log10(PyoObject):
    """
    Performs a base 10 log function on audio signal.

    Returns the base 10 log value of input. Values less than 0.0
    return 0.0.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal, angle in radians.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = RandInt(max=1000, freq=4)
    >>> b = Log10(a)
    >>> c = Print(input=b, method=1)

    """

    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [M_Log10_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Pow(PyoObject):
    """
    Performs a power function on audio signal.

    Parent class: PyoObject

    Parameters:

    base : float or PyoObject, optional
        Base composant. Defaults to 10.
    exponent : float or PyoObject, optional
        Exponent composant. Defaults to 1.

    Methods:

    setBase(x) : Replace the `base` attribute.
    setExponent(x) : Replace the `exponent` attribute.

    Attributes:

    base : float or PyoObject, Base composant.
    exponent : float or PyoObject, Exponent composant.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Phasor(freq=.2, mul=10)
    >>> b = Pow(10, a)
    >>> c = Print(input=b, interval=0.1)

    """
    def __init__(self, base=10, exponent=1, mul=1, add=0):
        PyoObject.__init__(self)
        self._base = base
        self._exponent = exponent
        self._mul = mul
        self._add = add
        base, exponent, mul, add, lmax = convertArgsToLists(base, exponent, mul, add)
        self._base_objs = [M_Pow_base(wrap(base,i), wrap(exponent,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['base', 'exponent', 'mul', 'add']

    def setBase(self, x):
        """
        Replace the `base` attribute.

        Parameters:

        x : float or PyoObject
            new `base` attribute.

        """
        self._base = x
        x, lmax = convertArgsToLists(x)
        [obj.setBase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setExponent(self, x):
        """
        Replace the `exponent` attribute.

        Parameters:

        x : float or PyoObject
            new `exponent` attribute.

        """
        self._exponent = x
        x, lmax = convertArgsToLists(x)
        [obj.setExponent(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def base(self):
        """float or PyoObject. Base composant.""" 
        return self._base
    @base.setter
    def base(self, x): self.setBase(x)

    @property
    def exponent(self):
        """float or PyoObject. Exponent composant.""" 
        return self._exponent
    @exponent.setter
    def exponent(self, x): self.setExponent(x)
