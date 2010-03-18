from _core import *
from types import SliceType

class Print(PyoObject):
    """
    Print PyoObject's current value.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    method : int {0, 1}, optional
        There is two methods to set when a value is printed (Defaults to 0):
        0 : at a periodic interval.
        1 : everytime the value changed.
    interval : float, optional
        Interval, in seconds, between each print. Used by method 0.
        Defaults to 0.25.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMethod(x) : Replace the `method` attribute.
    setInterval(x) : Replace the `interval` attribute.

    Attributes:

    input : PyoObject. Input signal.
    method : int. Controls when a value is printed.
    interval : float. For method 0, interval, in seconds, between each print.

    Notes:

    The out() method is bypassed. Print's signal can not be sent to 
    audio outs.
    
    Print has no `mul` and `add` attributes.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + '/transparent.aif', loop=True, mul=.5).out()
    >>> b = Follower(a)
    >>> p = Print(b, method=0, interval=.1)

    """
    def __init__(self, input, method=0, interval=0.25):
        self._input = input
        self._method = method
        self._interval = interval
        self._in_fader = InputFader(input)
        in_fader, method, interval, lmax = convertArgsToLists(self._in_fader, method, interval)
        self._base_objs = [Print_base(wrap(in_fader,i), wrap(method,i), wrap(interval,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'method', 'interval']

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

    def setMethod(self, x):
        """
        Replace the `method` attribute.
        
        Parameters:

        x : int {0, 1}
            New `method` attribute.

        """
        self._method = x
        x, lmax = convertArgsToLists(x)
        [obj.setMethod(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInterval(self, x):
        """
        Replace the `interval` attribute.

        Parameters:

        x : float
            New `interval` attribute.

        """
        self._interval = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterval(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1):
        pass

    def ctrl(self, map_list=None, title=None):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title)
      
    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def method(self):
        """int. Controls when a value is printed.""" 
        return self._method
    @method.setter
    def method(self, x): self.setMethod(x)

    @property
    def interval(self):
        """float. For method 0, interval, in seconds, between each print.""" 
        return self._interval
    @interval.setter
    def interval(self, x): self.setInterval(x)

