"""
Tools to perform arithmetic operation on audio signals.

"""
from _core import *

class M_Sin(PyoObject):
    """
    Performs a sine function on audio signal.

    Returns the sine of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to filter.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = M_Sin(a).out()

    """

    def __init__(self, input, mul=1, add=0):
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

class M_Cos(PyoObject):
    """
    Performs a cosine function on audio signal.

    Returns the cosine of input.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to filter.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.

    Examples:

    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> import math
    >>> a = Phasor(500, mul=math.pi*2)
    >>> b = M_Cos(a).out()

    """
    def __init__(self, input, mul=1, add=0):
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
