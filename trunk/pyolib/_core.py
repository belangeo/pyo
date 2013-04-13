# -*- coding: utf-8 -*-
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
from types import ListType, SliceType, FloatType, StringType, UnicodeType
import random, os, sys, inspect, tempfile
from subprocess import call

import __builtin__
if hasattr(__builtin__, 'pyo_use_double'):
    import pyo64 as current_pyo
    from _pyo64 import *
else:    
    import pyo as current_pyo
    from _pyo import *
    
from _maps import *
from _widgets import createCtrlWindow, createViewTableWindow, createViewMatrixWindow

######################################################################
### Utilities
######################################################################
SNDS_PATH = os.path.join(os.path.dirname(current_pyo.__file__), "pyolib", "snds")
XNOISE_DICT = {'uniform': 0, 'linear_min': 1, 'linear_max': 2, 'triangle': 3, 
                'expon_min': 4, 'expon_max': 5, 'biexpon': 6, 'cauchy': 7, 
                'weibull': 8, 'gaussian': 9, 'poisson': 10, 'walker': 11, 
                'loopseg': 12}

def convertStringToSysEncoding(str):
    """
    Convert a string to the current platform file system encoding.

    Parameter:
    
    str: string
        String to convert.

    """
    if type(str) != UnicodeType:
        str = str.decode("utf-8")
    str = str.encode(sys.getfilesystemencoding())
    return str
        
def convertArgsToLists(*args):
    """
    Convert all arguments to list if not already a list or a PyoObject. 
    Return new args and maximum list length.
    
    """
    converted = []
    for i in args:
        if isinstance(i, PyoObjectBase) or type(i) == ListType:
            converted.append(i)
        else:
            converted.append([i])
            
    max_length = max(len(i) for i in converted)
    return tuple(converted + [max_length])

def wrap(arg, i):
    """
    Return value at position `i` from `arg` with wrap around `arg` length.
    
    """
    x = arg[i % len(arg)]
    if isinstance(x, PyoObjectBase):
        return x[0]
    else:
        return x

def example(cls, dur=5, toprint=True, double=False):
    """
    Execute the example given in the documentation of the object as an argument.

    example(cls, dur=5)

    Parameters:

    cls : PyoObject class
        Class reference of the desired object example.
    dur : float, optional
        Duration of the example.
    toprint : boolean, optional
        If True, the example script will be printed to the console.
        Defaults to True.
    double : boolean, optional
        If True, force the example to run in double precision (64-bit)
        Defaults to False.

    Examples:

    >>> example(Sine)

    """
    doc = cls.__doc__.split("Examples:")
    if len(doc) < 2:
        print "There is no manual example for %s object." % cls.__name__
        return
    if "Server" in doc[1]:
        with_server = True
    else:
        with_server = False
    lines = doc[1].splitlines()
    ex_lines = [line.lstrip("    ") for line in lines if ">>>" in line or "..." in line]
    if hasattr(__builtin__, 'pyo_use_double') or double:
        ex = "import time\nfrom pyo64 import *\n"
    else:
        ex = "import time\nfrom pyo import *\n"
    for line in ex_lines:
        if ">>>" in line: line = line.lstrip(">>> ")
        if "..." in line: line = "    " +  line.lstrip("... ")
        ex += line + "\n"
    if with_server:
        ex += "time.sleep(%f)\ns.stop()\ntime.sleep(0.25)\ns.shutdown()\n" % dur
    if sys.version_info[:2] <= (2, 5):
        f = open('/tmp/pyo_example.py', 'w')
    else:
        f = tempfile.NamedTemporaryFile(delete=False)
    if toprint:
        f.write('print """\n%s\n"""\n' % ex)
    f.write(ex)
    f.close()    
    p = call(["python", f.name])
      
def removeExtraDecimals(x):
    if type(x) == FloatType:
        return "=%.2f" % x
    elif type(x) == StringType:
        return '="%s"' % x
    else:
        return "=" + str(x)

def class_args(cls):
    """
    Returns the init line of a class reference.

    class_args(cls)

    This function takes a class reference (not an instance of that class) 
    as input and returns the init line of that class with the default values.

    Parameters:

    cls : PyoObject class
        Class reference of the desired object's init line.

    Examples:

    >>> print class_args(Sine)
    >>> 'Sine(freq=1000, phase=0, mul=1, add=0)'

    """
    name = cls.__name__
    try:
        # Try for a class __init__ function
        arg, varargs, varkw, defaults = inspect.getargspec(getattr(cls, "__init__"))
        arg = inspect.formatargspec(arg, varargs, varkw, defaults, formatvalue=removeExtraDecimals)
        arg = arg.replace("self, ", "")
        return name + arg
    except TypeError:
        try:
            # Try for a function
            lines = cls.__doc__.splitlines()
            for line in lines:
                if "%s(" % name in line:
                    return line
        except:
            print "class_args was unable to retrieve the init line of the object as an argument."
            return ""

def getVersion():
    """
    Returns the version number of the current pyo installation.

    getVersion()

    This function returns the version number of the current pyo
    installation as a 3-ints tuple (major, minor, rev). 

    The returned tuple for version '0.4.1' will look like :

    (0, 4, 1)

    Examples:

    >>> print getVersion()
    >>> (0, 5, 1)

    """
    major, minor, rev = PYO_VERSION.split('.')
    return (int(major), int(minor), int(rev))

class PyoError(Exception):
    """Base class for all pyo exceptions."""

class PyoServerStateException(PyoError):
    """Error raised when an operation requires the server to be booted."""

######################################################################
### PyoObjectBase -> abstract class for pyo objects
######################################################################
class PyoObjectBase(object):
    """
    Base class for all pyo objects.

    This object encapsulates some common behaviors for all pyo objects.
    One typically inherits from a more specific subclass of this class
    instead of using it directly.
    
    Methods:
    
    dump() : Print infos about the current state of the object.
    getBaseObjects() : Return a list of Stream objects managed by the instance.

    """

    # Descriptive word for this kind of object, for use in printing
    # descriptions of the object. Subclasses need to set this.
    _STREAM_TYPE = ''
    
    def __init__(self):
        if not serverCreated():
            raise PyoServerStateException("You must create and boot a Server before creating any audio object.")
        if not serverBooted():
            raise PyoServerStateException("The Server must be booted before creating any audio object.")

    def dump(self):
        """
        Print infos about the current state of the object.

        Print the number of Stream objects managed by the instance 
        and the current status of the object's attributes.

        """
        attrs = dir(self)
        pp =  '< Instance of %s class >' % self.__class__.__name__
        pp += '\n-----------------------------'
        pp += '\nNumber of %s streams: %d' % (self._STREAM_TYPE, len(self))
        pp += '\n--- Attributes ---'
        for attr in attrs:
            pp += '\n' + attr + ': ' + str(getattr(self, attr))
        pp += '\n-----------------------------'
        return pp

    def getBaseObjects(self):
        """
        Return a list of Stream objects managed by the instance.

        """
        return self._base_objs

    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs
        if type(i) == SliceType or i < len(self._base_objs):
            return self._base_objs[i]
        else:
            if type(i) == StringType:
                print "Object %s has no stream named '%s'!" % (self.__class__.__name__, i)
            else:
                print "'i' too large in slicing %s object %s!" % (self._STREAM_TYPE, self.__class__.__name__)

    def __len__(self):
        return len(self._base_objs)

    def __repr__(self):
        return '< Instance of %s class >' % self.__class__.__name__

    def __dir__(self):
        args, varargs, varkw, defaults = inspect.getargspec(getattr(self.__class__, "__init__"))
        args = [a for a in args if hasattr(self.__class__, a) and a != "self"]
        return args

######################################################################
### PyoObject -> base class for pyo sound objects
######################################################################
class PyoObject(PyoObjectBase):
    """
    Base class for all pyo objects that manipulate vectors of samples.
    
    The user should never instantiate an object of this class.
    
    Parentclass: PyoObjectBase

    Methods:

    play(dur, delay) : Start processing without sending samples to output. 
        This method is called automatically at the object creation.
    stop() : Stop processing.
    out(chnl, inc, dur, delay) : Start processing and send samples to audio 
        output beginning at `chnl`.
    mix(voices) : Mix object's audio streams into `voices` streams and 
        return the Mix object.
    range(min, max) : Adjust `mul` and `add` attributes according to a given range.
    setMul(x) : Replace the `mul` attribute.
    setAdd(x) : Replace the `add` attribute.
    setDiv(x) : Replace and inverse the `mul` attribute.
    setSub(x) : Replace and inverse the `add` attribute.
    set(attr, value, port) : Replace any attribute with portamento. 
    ctrl(map_list, title) : Opens a sliders window to control parameters.
    get(all) : Return the first sample of the current buffer as a float.
    isPlaying(all) : Returns True if the object is playing, otherwise, returns False.
    isOutputting(all) : Returns True if the object is sending samples to dac, 
        otherwise, returns False.

    Attributes:

    mul : float or PyoObject
        Multiplication factor.
    add : float or PyoObject
        Addition factor.
    
    Notes:

    - Other operations:
    
    len(obj) : Return the number of audio streams in an object.
    obj[x] : Return stream `x` of the object. `x` is a number 
        from 0 to len(obj) - 1.
    del obj : Perform a clean delete of the object.
    
    - Arithmetics:
    
    Multiplication, addition, division and substraction can be applied 
    between pyo objects or between pyo objects and numbers. Doing so 
    returns a Dummy object with the result of the operation.
    `b = a * 0.5` creates a Dummy object `b` with `mul` attribute set 
    to 0.5 and leave `a` unchanged.
    
    Inplace multiplication, addition, division and substraction can be 
    applied between pyo objects or between pyo objects and numbers. 
    These operations will replace the `mul` or `add` factor of the object. 
    `a *= 0.5` replaces the `mul` attribute of `a`.
    
    Next operators can be used with PyoObject (not with XXX_base objects).
    
    Exponent and modulo:

    `a ** 10` returns a Pow object created as : Pow(a, 10)
    `10 ** a` returns a Pow object created as : Pow(10, a)
    `a % 4` returns a Wrap object created as : Wrap(a, 0, 4)
    `a % b` returns a Wrap object created as : Wrap(a, 0, b)

    Unary negative (-):
    
    `-a` returns a Dummy object with negative values of streams in `a`.
    
    Comparison operators:
        
    `a < b` returns a Compare object created as : Compare(a, comp=b, mode="<")
    `a <= b` returns a Compare object created as : Compare(a, comp=b, mode="<=")
    `a == b` returns a Compare object created as : Compare(a, comp=b, mode="==")
    `a != b` returns a Compare object created as : Compare(a, comp=b, mode="!=")
    `a > b` returns a Compare object created as : Compare(a, comp=b, mode=">")
    `a >= b` returns a Compare object created as : Compare(a, comp=b, mode=">=")

    A special case concerns the comparison of a PyoObject with None. All operators
    return False except `a != None`, which returns True.

    """
    
    _STREAM_TYPE = 'audio'

    def __init__(self, mul=1.0, add=0.0):
        PyoObjectBase.__init__(self)
        self._target_dict = {}
        self._signal_dict = {}
        self._keep_trace = []
        self._mul = mul
        self._add = add
        self._op_duplicate = 1
        self._map_list = []
        self._zeros = None

    def __add__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj + wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _add_dummy = x + self
            else:
                _add_dummy = Dummy([wrap(self._base_objs,i) + obj for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy
        
    def __radd__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj + wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            _add_dummy = Dummy([wrap(self._base_objs,i) + obj for i, obj in enumerate(x)])                
        self._keep_trace.append(_add_dummy)
        return _add_dummy
            
    def __iadd__(self, x):
        self.setAdd(x)
        return self

    def __sub__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _add_dummy = Dummy([obj - wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _add_dummy = Dummy([wrap(self._base_objs,i) - wrap(x,i) for i in range(lmax)])
            else:
                _add_dummy = Dummy([wrap(self._base_objs,i) - obj for i, obj in enumerate(x)])
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __rsub__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            tmp = []
            for i, obj in enumerate(self._base_objs):
                sub_upsamp = Sig(wrap(x, i/self._op_duplicate))
                self._keep_trace.append(sub_upsamp)
                tmp.append(sub_upsamp - obj)
            _add_dummy = Dummy(tmp)
        else:
            tmp = []
            for i, obj in enumerate(x):
                sub_upsamp = Sig(obj)
                self._keep_trace.append(sub_upsamp)
                tmp.append(sub_upsamp - wrap(self._base_objs,i))
            _add_dummy = Dummy(tmp)
        self._keep_trace.append(_add_dummy)
        return _add_dummy

    def __isub__(self, x):
        self.setSub(x)
        return self
 
    def __mul__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj * wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _mul_dummy = x * self
            else:
                _mul_dummy = Dummy([wrap(self._base_objs,i) * obj for i, obj in enumerate(x)])  
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy
        
    def __rmul__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj * wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            _mul_dummy = Dummy([wrap(self._base_objs,i) * obj for i, obj in enumerate(x)])                
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy
            
    def __imul__(self, x):
        self.setMul(x)
        return self
 
    def __div__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            _mul_dummy = Dummy([obj / wrap(x,i/self._op_duplicate) for i, obj in enumerate(self._base_objs)])
        else:
            if isinstance(x, PyoObject):
                _mul_dummy = Dummy([wrap(self._base_objs,i) / wrap(x,i) for i in range(lmax)])
            else:
                _mul_dummy = Dummy([wrap(self._base_objs,i) / obj for i, obj in enumerate(x)])
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __rdiv__(self, x):
        x, lmax = convertArgsToLists(x)
        if self.__len__() >= lmax:
            tmp = []
            for i, obj in enumerate(self._base_objs):
                div_upsamp = Sig(wrap(x, i/self._op_duplicate))
                self._keep_trace.append(div_upsamp)
                tmp.append(div_upsamp / obj)
            _mul_dummy = Dummy(tmp)
        else:
            tmp = []
            for i, obj in enumerate(x):
                div_upsamp = Sig(obj)
                self._keep_trace.append(div_upsamp)
                tmp.append(div_upsamp / wrap(self._base_objs,i))
            _mul_dummy = Dummy(tmp)
        self._keep_trace.append(_mul_dummy)
        return _mul_dummy

    def __idiv__(self, x):
        self.setDiv(x)
        return self

    def __pow__(self, x):
        return Pow(self, x)

    def __rpow__(self, x):
        return Pow(x, self)

    def __mod__(self, x):
        return Wrap(self, 0, x)
    
    def __neg__(self):
        if self._zeros == None:
            self._zeros = Sig(0)
        return self._zeros - self

    def __lt__(self, x):
        return self.__do_comp__(comp=x, mode="<")

    def __le__(self, x):
        return self.__do_comp__(comp=x, mode="<=")

    def __eq__(self, x):
        return self.__do_comp__(comp=x, mode="==")

    def __ne__(self, x):
        return self.__do_comp__(comp=x, mode="!=", default=True)

    def __gt__(self, x):
        return self.__do_comp__(comp=x, mode=">")

    def __ge__(self, x):
        return self.__do_comp__(comp=x, mode=">=")

    def __do_comp__(self, comp, mode, default=False):
        if comp == None:
            return default
        else:
            return Compare(self, comp=comp, mode=mode)

    def isPlaying(self, all=False):
        """
        Returns True if the object is playing, otherwise, returns False.

        Parameters:

            all : boolean, optional
                If True, the object returns a list with the state of all
                streams managed by the object. If False, it return a 
                boolean corresponding to the state of the first stream.
                Defaults to False.

        """
        if all:
            return [obj._getStream().isPlaying() for obj in self._base_objs]
        else:
            return self._base_objs[0]._getStream().isPlaying()

    def isOutputting(self, all=False):
        """
        Returns True if the object is sending samples to dac, otherwise, returns False.

        Parameters:

            all : boolean, optional
                If True, the object returns a list with the state of all
                streams managed by the object. If False, it return a 
                boolean corresponding to the state of the first stream.
                Defaults to False.

        """
        if all:
            return [obj._getStream().isOutputting() for obj in self._base_objs]
        else:
            return self._base_objs[0]._getStream().isOutputting()
            
    def get(self, all=False):
        """
        Return the first sample of the current buffer as a float.
        
        Can be used to convert audio stream to usable Python data.
        
        Object that implements string identifier for specific audio 
        streams must use the corresponding string to specify which stream
        to get value. See get() method definition in these object's man pages.
        
        Parameters:

            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.

        """
        if not all:
            return self._base_objs[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self._base_objs]

    def play(self, dur=0, delay=0):
        """
        Start processing without sending samples to output. 
        This method is called automatically at the object creation.

        This method returns "self" allowing it to be applied at the object
        creation.
        
        Parameters:
        
        dur : float, optional
            Duration, in seconds, of the object's activation. The default is 0
            and means infinite duration.
        delay : float, optional
            Delay, in seconds, before the object's activation. Defaults to 0.
        
        """
        dur, delay, lmax = convertArgsToLists(dur, delay)
        if hasattr(self, "_trig_objs"):
            self._trig_objs.play(dur, delay)
        if hasattr(self, "_base_players"):
            [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        """
        Start processing and send samples to audio output beginning at `chnl`.

        This method returns "self" allowing it to be applied at the object
        creation.
        
        Parameters:

        chnl : int, optional
            Physical output assigned to the first audio stream of the object. 
            Defaults to 0.

            If `chnl` is an integer equal or greater than 0, successive 
            streams increment the output number by `inc` and wrap around 
            the global number of channels.
            
            If `chnl` is a negative integer: streams begin at 0, increment 
            the output number by `inc` and wrap around the global number of 
            channels. Then, the list of streams is scrambled.
            
            If `chnl` is a list: successive values in the list will be 
            assigned to successive streams.
        inc : int, optional
            Output increment value.
        dur : float, optional
            Duration, in seconds, of the object's activation. The default is 0
            and means infinite duration.
        delay : float, optional
            Delay, in seconds, before the object's activation. Defaults to 0.
        
        """
        dur, delay, lmax = convertArgsToLists(dur, delay)
        if hasattr(self, "_trig_objs"):
            self._trig_objs.play(dur, delay)
        if hasattr(self, "_base_players"):
            [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        """
        Stop processing.

        This method returns "self" allowing it to be applied at the object
        creation.
        
        """
        if hasattr(self, "_trig_objs"):
            self._trig_objs.stop()
        if hasattr(self, "_base_players"):
            [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def mix(self, voices=1):
        """
        Mix the object's audio streams into `voices` streams and return 
        a Mix object.
        
        Parameters:

        voices : int, optional
            Number of audio streams of the Mix object created by this method. 
            If more than 1, object's streams are alternated and added into 
            Mix object's streams. Defaults to 1.
            
        """
        return Mix(self, voices)

    def range(self, min, max):
        """
        Adjust `mul` and `add` attributes according to a given range.
        This function assumes a signal between -1 and 1. Arguments may be
        list of floats for multi-streams objects.

        This method returns "self" allowing it to be applied at the object
        creation:

        >>> lfo = Sine(freq=1).range(500, 1000)

        Parameters:

        min : float
            Minimum value of the output signal.
        max : float
            Maximum value of the output signal.

        """
        min, max, lmax = convertArgsToLists(min, max)
        if lmax > 1:
            mul = [(wrap(max,i) - wrap(min,i)) * 0.5 for i in range(lmax)]
            add = [(wrap(max,i) + wrap(min,i)) * 0.5 for i in range(lmax)]
        else:
            mul = (max[0] - min[0]) * 0.5
            add = (max[0] + min[0]) * 0.5
        self.setMul(mul)
        self.setAdd(add)
        return self
        
    def setMul(self, x):
        """
        Replace the `mul` attribute.
        
        Parameters:

        x : float or PyoObject
            New `mul` attribute.
        
        """
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setMul(wrap(x,i/self._op_duplicate)) for i, obj in enumerate(self._base_objs)]
        
    def setAdd(self, x):
        """
        Replace the `add` attribute.
                
        Parameters:

        x : float or PyoObject
            New `add` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setAdd(wrap(x,i/self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def setSub(self, x):
        """
        Replace and inverse the `mul` attribute.
        
        Parameters:

        x : float or PyoObject
            New inversed `mul` attribute.
        
        """
        self._add = x
        x, lmax = convertArgsToLists(x)
        [obj.setSub(wrap(x,i/self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def setDiv(self, x):
        """
        Replace and inverse the `add` attribute.
                
        Parameters:

        x : float or PyoObject
            New inversed `add` attribute.

        """
        self._mul = x
        x, lmax = convertArgsToLists(x)
        [obj.setDiv(wrap(x,i/self._op_duplicate)) for i, obj in enumerate(self._base_objs)]

    def set(self, attr, value, port=0.025):
        """
        Replace any attribute with portamento.

        This method is intended to be applied on attributes that are not
        already assigned to PyoObjects. It will work only with floats or
        list of floats.

        Parameters:

        attr : string
            Name of the attribute as a string.
        value : float
            New value.
        port : float, optional
            Time, in seconds, to reach the new value

        """
        self._target_dict[attr] = value
        init = getattr(self, attr)
        if self._signal_dict.has_key(attr):
            if isinstance(self._signal_dict[attr], VarPort):
                if self._signal_dict[attr].isPlaying():
                    init = self._signal_dict[attr].get(True)
                    self._signal_dict[attr].stop()
        self._signal_dict[attr] = VarPort(value, port, init, self._reset_from_set, attr)
        setattr(self, attr, self._signal_dict[attr])

    def _reset_from_set(self, attr=None):
        if isinstance(getattr(self, attr), VarPort):
            setattr(self, attr, self._target_dict[attr])
        self._signal_dict[attr].stop()
        
    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        """
        Opens a sliders window to control the parameters of the object. 
        Only parameters that can be set to a PyoObject are allowed 
        to be mapped on a slider.

        If a list of values are given to a parameter, a multisliders 
        will be used to control each stream independently.

        Parameters:

        map_list : list of SLMap objects, optional
            Users defined set of parameters scaling. There is default 
            scaling for each object that accept `ctrl` method.
        title : string, optional
            Title of the window. If none is provided, the name of the 
            class is used.
        wxnoserver : boolean, optional
            With wxPython graphical toolkit, if True, tells the 
            interpreter that there will be no server window and not 
            to wait for it before showing the controller window. 
            Defaults to False.

        """
        if map_list == None:
            map_list = self._map_list
        if map_list == []:
            print("There is no controls for %s object." % self.__class__.__name__)
            return
        createCtrlWindow(self, map_list, title, wxnoserver)

    @property
    def mul(self):
        """float or PyoObject. Multiplication factor.""" 
        return self._mul
    @mul.setter
    def mul(self, x): self.setMul(x)

    @property
    def add(self):
        """float or PyoObject. Addition factor.""" 
        return self._add
    @add.setter
    def add(self, x): self.setAdd(x)
           
######################################################################
### PyoTableObject -> base class for pyo table objects
######################################################################
class PyoTableObject(PyoObjectBase):
    """
    Base class for all pyo table objects. 
    
    A table object is a buffer memory to store precomputed samples. 
    
    The user should never instantiate an object of this class.
    
    Parentclass: PyoObjectBase
 
    Methods:
    
    setSize(size) : Change the size of the table. This will erase the 
        previously drawn waveform.
    getSize() : Return table size in samples.
    view() : Opens a window showing the contents of the table.
    save(path, format, sampletype) : Writes the content of the table in an audio file.
    write(path, oneline) : Writes the content of the table in a text file.
    read(path) : Sets the content of the table from a text file.
    normalize() : Normalize table samples between -1 and 1.
    reset() : Resets table samples to 0.0.
    removeDC() : Remove DC offset from the table's data.
    reverse() : Reverse the table's data.
    copy() : Returns a deep copy of the object.
    put(value, pos) : Puts a value at specified position in the table.
    get(pos) : Returns the value at specified position in the table.
    getTable(all) : Returns the content of the table as list of floats.
    refreshView() : Updates the graphical display of the table, if applicable.

    Attributes:

    size : int, optional
        Table size in samples.

    Notes:
    
    Operations allowed on all table objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number 
        from 0 to len(obj) - 1.

    """
    
    _STREAM_TYPE = 'table'

    def __init__(self, size=0):
        PyoObjectBase.__init__(self)
        self._size = size
        self.viewFrame = None

    def save(self, path, format=0, sampletype=0):
        """
        Writes the content of the table in an audio file.
        
        The sampling rate of the file is the sampling rate of the server
        and the number of channels is the number of table streams of the
        object.

        Parameters:
        
        path : string
            Full path (including extension) of the new file.
        format : int, optional
            Format type of the new file. Defaults to 0. Supported formats are:
                0 : WAVE - Microsoft WAV format (little endian) {.wav, .wave}
                1 : AIFF - Apple/SGI AIFF format (big endian) {.aif, .aiff}
        sampletype : int, optional
            Bit depth encoding of the audio file. Defaults to 0. Supported types are:
                0 : 16 bit int
                1 : 24 bit int
                2 : 32 bit int
                3 : 32 bit float
                4 : 64 bit float

        """
        savefileFromTable(self, path, format, sampletype)
    
    def write(self, path, oneline=True):
        """
        Writes the content of the table in a text file.
        
        This function can be used to store the table data as a
        list of floats into a text file.
        
        Parameters:
        
        path : string
            Full path of the generated file.
        oneline : boolean, optional
            If True, list of samples will inserted on one line.
            If False, list of samples will be truncated to 8 floats
            per line. Defaults to True.

        """
        f = open(path, "w")
        if oneline:
            f.write(str([obj.getTable() for obj in self._base_objs]))
        else:
            text = "["
            for obj in self._base_objs:
                text += "["
                for i, val in enumerate(obj.getTable()):
                    if (i % 8) == 0:
                        text += "\n"
                    text += str(val) + ", "
                text += "]"
            text += "]"
            f.write(text)
        f.close()

    def read(self, path):
        """
        Reads the content of a text file and replaces the table data
        with the values in the file.
        
        Format is a list of lists of floats. For example, A two 
        tablestreams object must be given a content like this:
        
        [[0.0,1.0,0.5,...], [1.0,0.99,0.98,0.97,...]]
        
        Each object's tablestream will be resized according to the 
        length of the lists.
        
        """
        f = open(path, "r")
        f_list = eval(f.read())
        f_len = len(f_list)
        f.close()
        [obj.setData(f_list[i%f_len]) for i, obj in enumerate(self._base_objs)]
        self.refreshView()

    def setSize(self, size):
        """
        Change the size of the table. This will erase the previously 
        drawn waveform.
        
        Parameters:
        
        size : int
            New table size in samples.
        
        """
        self._size = size
        [obj.setSize(size) for obj in self._base_objs]
        self.refreshView()

    def getSize(self):
        """
        Return table size in samples.
        
        """
        return self._size

    def put(self, value, pos=0):
        """
        Puts a value at specified position in the table.
        
        If the object has more than 1 tablestream, the default is to
        record the value in each table. User can call obj[x].put() 
        to record in a specific table.
        
        Parameters:
        
        value : float
            Value, as floating-point, to record in the table.
        pos : int, optional
            Position where to record value. Defaults to 0.
        
        """
        [obj.put(value, pos) for obj in self._base_objs]
        self.refreshView()

    def get(self, pos):
        """
        Returns the value, as float, at specified position in the table.
        
        If the object has more than 1 tablestream, the default is to
        return a list with the value of each tablestream. User can call 
        obj[x].get() to get the value of a specific table.
        
        Parameters:
        
        pos : int, optional
            Position where to get value. Defaults to 0.
        
        """
        values = [obj.get(pos) for obj in self._base_objs]
        if len(values) == 1: return values[0]
        else: return values

    def getTable(self, all=False):
        """
        Returns the content of the table as list of floats.
        
        Parameters:
            
        all : boolean, optional
            If True, all sub tables are retrieved and returned as a list
            of list of floats. Otherwise, a single list containing the
            content of the first subtable (or the only one) is returned.
            Defaults to False.

        """
        if all:
            return [obj.getTable() for obj in self._base_objs]
        else:
            return self._base_objs[0].getTable()

    def normalize(self):
        """
        Normalize table samples between -1 and 1.

        """
        [obj.normalize() for obj in self._base_objs]
        self.refreshView()
        return self

    def reset(self):
        """
        Resets table samples to 0.0.

        """
        [obj.reset() for obj in self._base_objs]
        self.refreshView()
        return self

    def removeDC(self):
        """
        Filter out DC offset from the table's data.

        """
        [obj.removeDC() for obj in self._base_objs]
        self.refreshView()
        return self

    def reverse(self):
        """
        Reverse the table's data.

        """
        [obj.reverse() for obj in self._base_objs]
        self.refreshView()
        return self

    def copy(self):
        """
        Returns a deep copy of the object.
        
        """
        args = [getattr(self, att) for att in self.__dir__()]
        if self.__class__.__name__ == "SndTable":
            _size = self.getSize()
            if type(_size) != ListType:
                _size = [_size]
            _chnls = len(self._base_objs)
            args[0] = None
            args.append(_chnls)
            newtable = getattr(current_pyo, self.__class__.__name__)(*args)
            [obj.setSize(_size[i%len(_size)]) for i, obj in enumerate(newtable.getBaseObjects())]
            [obj.copy(self[i]) for i, obj in enumerate(newtable.getBaseObjects())]
        else:
            newtable = getattr(current_pyo, self.__class__.__name__)(*args)
            [obj.copy(self[i]) for i, obj in enumerate(newtable.getBaseObjects())]
        return newtable

    def view(self, title="Table waveform", wxnoserver=False):
        """
        Opens a window showing the contents of the table.
        
        Parameters:
        
        title : string, optional
            Window title. Defaults to "Table waveform". 
        wxnoserver : boolean, optional
            With wxPython graphical toolkit, if True, tells the 
            interpreter that there will be no server window and not 
            to wait for it before showing the table window. 
            Defaults to False.
        
        """
        samples = self._base_objs[0].getViewTable()
        createViewTableWindow(samples, title, wxnoserver, self.__class__.__name__, self)

    def _setViewFrame(self, frame):
        self.viewFrame = frame
        
    def refreshView(self):
        """
        Updates the graphical display of the table, if applicable.

        """
        if self.viewFrame != None:
            samples = self._base_objs[0].getViewTable()
            self.viewFrame.update(samples)

    @property
    def size(self):
        """int. Table size in samples.""" 
        return self._size
    @size.setter
    def size(self, x): self.setSize(x)
        
######################################################################
### PyoMatrixObject -> base class for pyo matrix objects
######################################################################
class PyoMatrixObject(PyoObjectBase):
    """
    Base class for all pyo matrix objects. 
    
    A matrix object is a 2 dimensions buffer memory to store 
    precomputed samples. 
    
    The user should never instantiate an object of this class.
    
    Parentclass: PyoObjectBase
 
    Methods:
    
    getSize() : Return matrix size in samples (x, y).
    view() : Opens a window showing the contents of the matrix.
    write(path) : Writes the content of the matrix in a text file.
    read(path) : Sets the content of the matrix from a text file.
    normalize() : Normalize matrix samples between -1 and 1.
    blur() : Apply a simple gaussian blur on the matrix.
    boost(min, max, boost) : Boost the contrast of values in the matrix.
    put(value, x, y) : Puts a value at specified position in the matrix.
    get(x, y) : Returns the value at specified position in the matrix.
    refreshView() : Updates the graphical display of the matrix, if applicable.
    
    Notes:
    
    Operations allowed on all matrix objects :
    
    len(obj) : Return the number of table streams in an object.
    obj[x] : Return table stream `x` of the object. `x` is a number 
        from 0 to len(obj) - 1.

    """
    
    _STREAM_TYPE = 'matrix'

    def __init__(self):
        PyoObjectBase.__init__(self)
        self.viewFrame = None

    def write(self, path):
        """
        Writes the content of the matrix into a text file.
        
        This function can be used to store the matrix data as a
        list of list of floats into a text file.
         
        """
        f = open(path, "w")
        f.write(str([obj.getData() for obj in self._base_objs]))
        f.close()

    def read(self, path):
        """
        Reads the content of a text file and replaces the matrix data
        with the values in the file.
        
        Format is a list of lists of floats. For example, A two 
        matrixstreams object must be given a content like this:
        
        [[[0.0,1.0,0.5,...], [1.0,0.99,0.98,0.97,...]],
        [[0.0,1.0,0.5,...], [1.0,0.99,0.98,0.97,...]]]
        
        Each object's matrixstream will be resized according to the 
        length of the lists, but the number of matrixstreams must be
        the same.
        
        """
        f = open(path, "r")
        f_list = eval(f.read())
        f_len = len(f_list)
        f.close()
        [obj.setData(f_list[i%f_len]) for i, obj in enumerate(self._base_objs)]

    def getSize(self):
        """
        Returns matrix size in samples. Size is a tuple (x, y).
        
        """
        return self._size

    def normalize(self):
        """
        Normalize matrix samples between -1 and 1.

        """
        [obj.normalize() for obj in self._base_objs]
        return self

    def blur(self):
        """
        Apply a simple gaussian blur on the matrix.

        """
        [obj.blur() for obj in self._base_objs]

    def boost(self, min=-1.0, max=1.0, boost=0.01):
        """
        Boost the constrast of values in the matrix.
        
        Parameters:
        
        min : float, optional
            Minimum value. Defaults to -1.0.
        max : float, optional
            Maximum value. Defaults to 1.0.
        boost : float, optional
            Amount of boost applied on each value. Defaults to 0.01.

        """
        [obj.boost(min, max, boost) for obj in self._base_objs]

    def put(self, value, x=0, y=0):
        """
        Puts a value at specified position in the matrix.
        
        If the object has more than 1 matrixstream, the default is to
        record the value in each matrix. User can call obj[x].put() 
        to record in a specific matrix.
        
        Parameters:
        
        value : float
            Value, as floating-point, to record in the matrix.
        x : int, optional
            X position where to record value. Defaults to 0.
        y : int, optional
            Y position where to record value. Defaults to 0.
        
        """
        [obj.put(value, x, y) for obj in self._base_objs]

    def get(self, x, y):
        """
        Returns the value, as float, at specified position in the matrix.
        
        If the object has more than 1 matrixstream, the default is to
        return a list with the value of each matrixstream. User can call 
        obj[x].get() to get the value of a specific matrix.
        
        Parameters:
        
        x : int, optional
            X position where to get value. Defaults to 0.
        y : int, optional
            Y position where to get value. Defaults to 0.
        
        """
        values = [obj.get(x, y) for obj in self._base_objs]
        if len(values) == 1: return values[0]
        else: return values

    def view(self, title="Matrix viewer", wxnoserver=False):
        """
        Opens a window showing the contents of the matrix.
        
        Parameters:
        
        title : string, optional
            Window title. Defaults to "Matrix viewer". 
        wxnoserver : boolean, optional
            With wxPython graphical toolkit, if True, tells the 
            interpreter that there will be no server window and not 
            to wait for it before showing the matrix window. 
            Defaults to False.
        
        """        
        samples = self._base_objs[0].getViewData()
        createViewMatrixWindow(samples, self.getSize(), title, wxnoserver, self)

    def _setViewFrame(self, frame):
        self.viewFrame = frame
        
    def refreshView(self):
        """
        Updates the graphical display of the matrix, if applicable.

        """
        if self.viewFrame != None:
            samples = self._base_objs[0].getViewData()
            self.viewFrame.update(samples)
        
######################################################################
### Internal classes -> Used by pyo
######################################################################
class Mix(PyoObject):
    """
    Mix audio streams to arbitrary number of streams.

    Mix the object's audio streams as `input` argument into `voices` 
    streams.

    Parentclass: PyoObject

    Parameters:

    input : PyoObject or list of PyoObjects
        Input signal(s) to mix the streams.
    voices : int, optional
        Number of streams of the Mix object. If more than 1, input 
        object's streams are alternated and added into Mix object's 
        streams. Defaults to 1.

    Notes:

    The mix method of PyoObject creates and returns a new Mix object
    with mixed streams of the object that called the method. User
    don't have to instantiate this class directly. These two calls
    are identical:

    >>> b = a.mix()
    >>> b = Mix(a)

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine([random.uniform(400,600) for i in range(50)], mul=.02)
    >>> b = Mix(a, voices=2).out()
    >>> print len(a)
    50
    >>> print len(b)
    1

    """
    def __init__(self, input, voices=1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        mul, add, lmax = convertArgsToLists(mul, add)
        if type(input) == ListType:
            input_objs = []
            input_objs = [obj for pyoObj in input for obj in pyoObj.getBaseObjects()]
        else:    
            input_objs = input.getBaseObjects()
        input_len = len(input_objs)
        if voices < 1: 
            voices = 1
            num = 1
        elif voices > input_len and voices > lmax: 
            num = voices
        elif lmax > input_len:
            num = lmax    
        else:
            num = input_len   
        sub_lists = []
        for i in range(voices):
            sub_lists.append([])
        for i in range(num):
            obj = input_objs[i % input_len]
            sub_lists[i % voices].append(obj)
        self._base_objs = [Mix_base(l, wrap(mul,i), wrap(add,i)) for i, l in enumerate(sub_lists)]

class Dummy(PyoObject):
    """
    Dummy object used to perform arithmetics on PyoObject.

    The user should never instantiate an object of this class.

    Parentclass: PyoObject

    Parameters:

    objs_list : list of audio Stream objects
        List of Stream objects return by the PyoObject hidden method 
        getBaseObjects().

    Notes:

    Multiplication, addition, division and substraction don't changed
    the PyoObject on which the operation is performed. A dummy object
    is created, which is just a copy of the audio Streams of the object,
    and the operation is applied on the Dummy, leaving the original
    object unchanged. This lets the user performs multiple different 
    arithmetic operations on an object without conficts. Here, `b` is
    a Dummy object with `a` as its input with a `mul` attribute of 0.5. 
    attribute:

    >>> a = Sine()
    >>> b = a * .5
    >>> print a
    <pyolib.input.Sine object at 0x11fd610>
    >>> print b
    <pyolib._core.Dummy object at 0x11fd710>

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> m = Metro(time=0.25).play()
    >>> p = TrigChoice(m, choice=[midiToHz(n) for n in [60,62,65,67,69]])
    >>> a = SineLoop(p, feedback=.05, mul=.1).mix(2).out()
    >>> b = SineLoop(p*1.253, feedback=.05, mul=.06).mix(2).out()
    >>> c = SineLoop(p*1.497, feedback=.05, mul=.03).mix(2).out()
    
    """
    def __init__(self, objs_list):
        PyoObject.__init__(self)
        self._objs_list = objs_list
        tmp_list = []
        for x in objs_list:
            if isinstance(x, Dummy):
                tmp_list.extend(x.getBaseObjects())
            else:
                tmp_list.append(x)
        self._base_objs = tmp_list
        
class InputFader(PyoObject):
    """
    Audio streams crossfader.

    Parameters:

    input : PyoObject
        Input signal.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal.

    Notes:

    The setInput method, available to object with `input` attribute, 
    uses an InputFader object internally to perform crossfade between 
    the old and the new audio input assigned to the object. 

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SineLoop([449,450], feedback=0.05, mul=.2)
    >>> b = SineLoop([650,651], feedback=0.05, mul=.2)
    >>> c = InputFader(a).out()
    >>> # to created a crossfade, assign a new audio input:
    >>> c.setInput(b, fadetime=5)

    """
    def __init__(self, input):
        PyoObject.__init__(self)
        self._input = input
        input, lmax = convertArgsToLists(input)
        self._base_objs = [InputFader_base(wrap(input,i)) for i in range(lmax)]

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
        x, lmax = convertArgsToLists(x)
        [obj.setInput(wrap(x,i), fadetime) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class Sig(PyoObject):
    """
    Convert numeric value to PyoObject signal.
    
    Parentclass: PyoObject

    Parameters:

    value : float or PyoObject
        Numerical value to convert.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    
    Attributes:
    
    value : float or PyoObject. Numerical value to convert.
    
    Examples:
    
    >>> import random
    >>> s = Server().boot()
    >>> s.start()
    >>> fr = Sig(value=400)
    >>> p = Port(fr, risetime=0.001, falltime=0.001)
    >>> a = SineLoop(freq=p, feedback=0.08, mul=.3).out()
    >>> b = SineLoop(freq=p*1.005, feedback=0.08, mul=.3).out(1)
    >>> def pick_new_freq():
    ...     fr.value = random.randrange(300,601,50)
    >>> pat = Pattern(function=pick_new_freq, time=0.5).play()

    """
    def __init__(self, value, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._value = value
        value, mul ,add, lmax = convertArgsToLists(value, mul, add)
        self._base_objs = [Sig_base(wrap(value,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setValue(self, x):
        """
        Changes the value of the signal stream.

        Parameters:

        x : float or PyoObject
            Numerical value to convert.

        """
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, 1, "lin", "value", self._value)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
    
    @property
    def value(self):
        """float or PyoObject. Numerical value to convert.""" 
        return self._value
    @value.setter
    def value(self, x): self.setValue(x)

class VarPort(PyoObject):
    """
    Convert numeric value to PyoObject signal with portamento.

    When `value` attribute is changed, a smoothed ramp is applied from the
    current value to the new value. If a callback is provided as `function`
    argument, it will be called at the end of the line.

    Parentclass: PyoObject

    Parameters:

    value : float
        Numerical value to convert.
    time : float, optional
        Ramp time, in seconds, to reach the new value. Defaults to 0.025.
    init : float, optional
        Initial value of the internal memory. Defaults to 0.
    function : Python callable, optional
        If provided, it will be called at the end of the line. 
        Defaults to None.
    arg : any Python object, optional
        Optional argument sent to the function called at the end of the line.
        Defaults to None.

    Methods:

    setValue(x) : Changes the value of the signal stream.
    setTime(x) : Changes the ramp time.

    Attributes:

    value : float. Numerical value to convert.
    time : float. Ramp time.

    Notes:

    The out() method is bypassed. VarPort's signal can not be sent to audio outs.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> def callback(arg):
    ...     print "end of line"
    ...     print arg
    ... 
    >>> fr = VarPort(value=500, time=2, init=250, function=callback, arg="YEP!")
    >>> a = SineLoop(freq=[fr,fr*1.01], feedback=0.05, mul=.2).out()

    """
    def __init__(self, value, time=0.025, init=0.0, function=None, arg=None, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._value = value
        self._time = time
        value, time, init, function, arg, mul ,add, lmax = convertArgsToLists(value, time, init, function, arg, mul, add)
        self._base_objs = [VarPort_base(wrap(value,i), wrap(time,i), wrap(init,i), wrap(function,i), wrap(arg,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def setValue(self, x):
        """
        Changes the value of the signal stream.

        Parameters:

        x : float
            Numerical value to convert.

        """
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setTime(self, x):
        """
        Changes the ramp time of the object.

        Parameters:

        x : float
            New ramp time.

        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    @property
    def value(self):
        """float. Numerical value to convert.""" 
        return self._value
    @value.setter
    def value(self, x): self.setValue(x)

    @property
    def time(self):
        """float. Ramp time.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)

class Pow(PyoObject):
    """
    Performs a power function on audio signal.

    Parentclass: PyoObject

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
    >>> # Exponential amplitude envelope
    >>> a = LFO(freq=1, type=3, mul=0.5, add=0.5)
    >>> b = Pow(Clip(a, 0, 1), 4, mul=.3)
    >>> c = SineLoop(freq=[300,301], feedback=0.05, mul=b).out()

    """
    def __init__(self, base=10, exponent=1, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._base = base
        self._exponent = exponent
        base, exponent, mul, add, lmax = convertArgsToLists(base, exponent, mul, add)
        self._base_objs = [M_Pow_base(wrap(base,i), wrap(exponent,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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
    
class Wrap(PyoObject):
    """
    Wraps-around the signal that exceeds the `min` and `max` thresholds.

    This object is useful for table indexing, phase shifting or for 
    clipping and modeling an audio signal.

    Parentclass : PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    min : float or PyoObject, optional
        Minimum possible value. Defaults to 0.
    max : float or PyoObject, optional
        Maximum possible value. Defaults to 1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMin(x) : Replace the `min` attribute.
    setMax(x) : Replace the `max` attribute.

    Attributes:

    input : PyoObject. Input signal to process.
    min : float or PyoObject. Minimum possible value.
    max : float or PyoObject. Maximum possible value.

    Notes:

    If `min` is higher than `max`, then the output will be the average of the two.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> # Time-varying overlaping envelopes
    >>> env = HannTable()
    >>> lff = Sine(.5, mul=3, add=4)
    >>> ph1 = Phasor(lff)
    >>> ph2 = Wrap(ph1+0.5, min=0, max=1)
    >>> amp1 = Pointer(env, ph1, mul=.25)
    >>> amp2 = Pointer(env, ph2, mul=.25)
    >>> a = SineLoop(250, feedback=.1, mul=amp1).out()
    >>> b = SineLoop(300, feedback=.1, mul=amp2).out(1)

    """
    def __init__(self, input, min=0.0, max=1.0, mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._min = min
        self._max = max
        self._in_fader = InputFader(input)
        in_fader, min, max, mul, add, lmax = convertArgsToLists(self._in_fader, min, max, mul, add)
        self._base_objs = [Wrap_base(wrap(in_fader,i), wrap(min,i), wrap(max,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

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

    def setMin(self, x):
        """
        Replace the `min` attribute.

        Parameters:

        x : float or PyoObject
            New `min` attribute.

        """
        self._min = x
        x, lmax = convertArgsToLists(x)
        [obj.setMin(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setMax(self, x):
        """
        Replace the `max` attribute.

        Parameters:

        x : float or PyoObject
            New `max` attribute.

        """
        self._max = x
        x, lmax = convertArgsToLists(x)
        [obj.setMax(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., 'lin', 'min', self._min),
                          SLMap(0., 1., 'lin', 'max', self._max),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def min(self):
        """float or PyoObject. Minimum possible value.""" 
        return self._min
    @min.setter
    def min(self, x): self.setMin(x)

    @property
    def max(self):
        """float or PyoObject. Maximum possible value.""" 
        return self._max
    @max.setter
    def max(self, x): self.setMax(x)

class Compare(PyoObject):
    """
    Comparison object.

    Compare evaluates a comparison between a PyoObject and a number or
    between two PyoObjects and outputs 1.0, as audio stream, if the
    comparison is true, otherwise outputs 0.0.

    Parentclass: PyoObject

    Parameters:

    input : PyoObject
        Input signal.
    comp : float or PyoObject
        comparison signal.
    mode : string, optional
        Comparison operator as a string. Allowed operator are "<", "<=",
        ">", ">=", "==", "!=". Default to "<".

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setComp(x, fadetime) : Replace the `comp` attribute.
    setMode(x) : Replace the `mode` attribute.

    Attributes:

    input : PyoObject. Input signal.
    comp : float or PyoObject. Comparison signal.
    mode : string. Comparison operator.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SineLoop(freq=[199,200], feedback=.1, mul=.2)
    >>> b = SineLoop(freq=[149,150], feedback=.1, mul=.2)
    >>> ph = Phasor(freq=1)
    >>> ch = Compare(input=ph, comp=0.5, mode="<=")
    >>> out = Selector(inputs=[a,b], voice=Port(ch)).out()

    """
    def __init__(self, input, comp, mode="<", mul=1, add=0):
        PyoObject.__init__(self, mul, add)
        self._input = input
        self._comp = comp
        self._mode = mode
        self._in_fader = InputFader(input)
        self.comp_dict = {"<": 0, "<=": 1, ">": 2, ">=": 3, "==": 4, "!=": 5}
        in_fader, comp, mode, mul, add, lmax = convertArgsToLists(self._in_fader, comp, mode, mul, add)
        self._base_objs = [Compare_base(wrap(in_fader,i), wrap(comp,i), self.comp_dict[wrap(mode,i)], wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self.play(dur, delay)

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

    def setComp(self, x):
        """
        Replace the `comp` attribute.
        
        Parameters:

        x : PyoObject
            New comparison signal.

        """
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setMode(self, x):
        """
        Replace the `mode` attribute. 
        
        Allowed operator are "<", "<=", ">", ">=", "==", "!=".
        
        Parameters:

        x : string
            New `mode` attribute.

        """
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(self.comp_dict[wrap(x,i)]) for i, obj in enumerate(self._base_objs)]

    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def comp(self):
        """PyoObject. Comparison signal.""" 
        return self._comp
    @comp.setter
    def comp(self, x): self.setComp(x)

    @property
    def mode(self):
        """string. Comparison operator.""" 
        return self._mode
    @mode.setter
    def mode(self, x): self.setMode(x)
