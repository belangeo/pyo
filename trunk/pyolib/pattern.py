"""
Call Python functions from triggers or number counts. Useful for event sequencing.

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

class Pattern(PyoObject):
    """
    Periodically calls a Python function.

    The play() method starts the pattern timer and is not called 
    at the object creation time.
            
    Parent class: PyoObject
    
    Parameters:

    function : Python function
        Function to be called.
    time : float or PyoObject, optional
        Time, in seconds, between each call. Default to 1.
        
    Methods:

    setFunction(x) : Replace the `function` attribute.
    setTime(x) : Replace the `time` attribute.

    Attributes:
    
    function : Python function. Function to be called.
    time : Time, in seconds, between each call.
    
    Notes:

    The out() method is bypassed. Pattern doesn't return signal.
    
    Pattern has no `mul` and `add` attributes.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> t = HarmTable([1,0,.33,0,.2,0,.143,0,.111])
    >>> a = Osc(table=t, freq=250, mul=.5).out()
    >>> def pat():
    ...     a.freq = random.randint(200, 400)
    ...    
    >>> p = Pattern(pat, .125)
    >>> p.play()
    
    """
    def __init__(self, function, time=1):
        PyoObject.__init__(self)
        self._function = function
        self._time = time
        function, time, lmax = convertArgsToLists(function, time)
        self._base_objs = [Pattern_base(wrap(function,i), wrap(time,i)) for i in range(lmax)]

    def __dir__(self):
        return ['function', 'time']

    def setFunction(self, x):
        """
        Replace the `function` attribute.

        Parameters:

        x : Python function
            new `function` attribute.

        """
        self._function = x
        x, lmax = convertArgsToLists(x)
        [obj.setFunction(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        """
        Replace the `time` attribute.
        
        Parameters:
        
        x : float or PyoObject
            New `time` attribute.
        
        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, x=0, inc=1, dur=0, delay=0):
        return self
        
    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0.125, 4., 'lin', 'time', self._time)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
         
    @property
    def function(self): return self._function
    @function.setter
    def function(self, x): self.setFunction(x)   
    @property
    def time(self):
        """float or PyoObject. Time, in seconds, between each call.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)

class Score(PyoObject):
    """
    Calls functions by incrementation of a preformatted name.
    
    Score takes audio stream containning integers in input and calls
    a function whose name is the concatenation of `fname` and the changing 
    integer.
    
    Can be used to sequence events, first by creating functions p0, p1, 
    p2, etc. and then, by passing a counter to a Score object with "p" 
    as `fname` argument. Functions are called without parameters.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Audio signal. Must contains integer numbers. Integer must change
        before calling its function again.
    fname : string, optional
        Name of the functions to be called. Defaults to "event_", meaning
        that the object will call the function "event_0", "event_1", "event_2" 
        and so on... Available at initialization time only.
    
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:
    
    input : PyoObject. Audio signal.

    Notes:

    The out() method is bypassed. Score's signal can not be sent 
    to audio outs.

    Score has no `mul` and `add` attributes.

    See also: Pattern, TrigFunc
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(1).play()
    >>> c = Counter(m, min=0, max=2)
    >>> def event_0(): print "event 0"
    >>> def event_1(): print "event 1"
    >>> def event_2(): print "event 2"
    >>> sc = Score(c)
    
    """
    def __init__(self, input, fname="event_"):
        PyoObject.__init__(self)
        self._input = input
        self._fname = fname
        self._in_fader = InputFader(input)
        in_fader, fname, lmax = convertArgsToLists(self._in_fader, fname)
        self._base_objs = [Score_base(wrap(in_fader,i), wrap(fname,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input']

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

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class CallAfter(PyoObject):
    """
    Calls a Python function after a given time.
        
    Parent class: PyoObject
    
    Parameters:

    function : Python function
    time : float, optional
        Time, in seconds, before the call. Default to 1.
    arg : any Python object, optional
        Argument sent to the called function. Default to None.
  
    Notes:

    The out() method is bypassed. CallAfter doesn't return signal.
    
    CallAfter has no `mul` and `add` attributes.
    
    The object is not deleted after the call. The User must delete it himself.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> print "just started"
    >>> def callback(arg):
    ...     print arg
    ...
    >>> a = CallAfter(callback, 2, "YEP!")
    
    """
    def __init__(self, function, time=1, arg=None):
        PyoObject.__init__(self)
        self._function = function
        function, time, arg, lmax = convertArgsToLists(function, time, arg)
        self._base_objs = [CallAfter_base(wrap(function,i), wrap(time,i), wrap(arg,i)) for i in range(lmax)]

    def __dir__(self):
        return []

    def out(self, x=0, inc=1, dur=0, delay=0):
        return self
        
    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass

    def setSub(self, x):
        pass

    def setDiv(self, x):
        pass

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
