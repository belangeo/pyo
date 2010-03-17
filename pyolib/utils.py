from _core import *
from types import SliceType

class Print(PyoObject):
    """
    Print PyoObject current value.
    
    Output signal is the number of zero-crossing occured during each 
    buffer size, normalized between 0 and 1.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    thresh : float, optional
        Minimum amplitude difference allowed between adjacent samples 
        to be included in the zeros count.
         

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setThresh(x) : Replace the `thresh` attribute.

    Attributes:

    input : PyoObject. Input signal to filter.
    thresh : float. Amplitude difference threshold.

    Notes:

    The out() method is bypassed. Print's signal can not be sent to 
    audio outs.
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input()
    >>> b = ZCross(a, thresh=.02)
    >>> n = Noise(b).out()

    """
    def __init__(self, input, method=0, interval=1):
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

        x : int
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
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def method(self):
        """int. Amplitude difference threshold.""" 
        return self._method
    @method.setter
    def method(self, x): self.setMethod(x)

    @property
    def interval(self):
        """float. Amplitude difference threshold.""" 
        return self._interval
    @interval.setter
    def interval(self, x): self.setInterval(x)

